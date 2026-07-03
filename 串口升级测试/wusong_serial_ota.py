#!/usr/bin/env python3
"""
武松球盘串口 OTA 升级工具。

依赖：
    py -m pip install pyserial

示例：
    # 查询当前 APP 版本
    py wusong_serial_ota.py version --port COM4

    # 进入 Bootloader 并读取 Bootloader 状态
    py wusong_serial_ota.py bootloader --port COM4

    # 升级，版本号可写成 0x01000100、1.0.1 或 V1.0.1
    py wusong_serial_ota.py upgrade Wusong.bin --port COM4 --target-version 1.0.1

协议依据：
    APP 查询版本：
        AA 12 00 01 CRC16_H CRC16_L 55

    APP 版本回复：
        AA 21 Major Minor CRC16_H CRC16_L 55
        AA 22 Patch Reserved CRC16_H CRC16_L 55

    APP 进入 Bootloader：
        AA F0 42 4F CRC16_H CRC16_L 55

    Bootloader OTA：
        AA 5A Version Command SequenceLE PayloadLengthLE Payload CRC32LE
"""

from __future__ import annotations

import argparse
import binascii
import hashlib
import struct
import sys
import time
from pathlib import Path

try:
    import serial
except ImportError as exc:
    raise SystemExit("缺少 pyserial，请执行：py -m pip install pyserial") from exc


# 武松 STM32F407 Flash 分区。
APP_ADDR = 0x0800C000
APP_END_ADDR = 0x08080000
OTA_CACHE_ADDR = 0x08080000
OTA_META_ADDR = 0x080DFF00

APP_MAX_IMAGE = APP_END_ADDR - APP_ADDR
OTA_CACHE_MAX_IMAGE = OTA_META_ADDR - OTA_CACHE_ADDR
OTA_MAX_IMAGE = min(APP_MAX_IMAGE, OTA_CACHE_MAX_IMAGE)

SRAM_START = 0x20000000
SRAM_END = 0x20020000

TOOL_VERSION = "1.0.0-wusong"

# APP 固定 7 字节协议。
APP_HEAD = 0xAA
APP_TAIL = 0x55
APP_CMD_VERSION_QUERY = 0x12
APP_CMD_VERSION_HIGH = 0x21
APP_CMD_VERSION_LOW = 0x22
APP_CMD_ENTER_BOOT = 0xF0

# Bootloader OTA 协议。
OTA_PROTOCOL_VERSION = 0x01
OTA_MAX_DATA_SIZE = 1024
OTA_MAX_PAYLOAD_SIZE = OTA_MAX_DATA_SIZE + 6
OTA_TARGET_MAGIC = b"BOTA"

OTA_HELLO = 0x01
OTA_BEGIN = 0x02
OTA_DATA = 0x03
OTA_END = 0x04
OTA_INSTALL = 0x05
OTA_STATUS = 0x06
OTA_ABORT = 0x07
OTA_REBOOT = 0x08
OTA_ACK = 0x80
OTA_NACK = 0x81

STATE_NAMES = {
    0: "IDLE",
    1: "RECEIVING",
    2: "VERIFIED",
    3: "INSTALLING",
    4: "INSTALLED",
    5: "ERROR",
}

RESULT_NAMES = {
    0x00: "OK",
    0x01: "协议版本错误",
    0x02: "帧 CRC32 错误",
    0x03: "负载长度错误",
    0x04: "升级目标错误",
    0x05: "固件大小错误",
    0x06: "缓存擦除失败",
    0x07: "Flash 写入失败",
    0x08: "数据偏移错误",
    0x09: "固件 CRC32 错误",
    0x0A: "固件向量表无效",
    0x0B: "没有有效固件",
    0x0C: "安装失败",
    0x0D: "升级尚未开始",
    0x0E: "未知命令",
}


class OtaError(RuntimeError):
    """串口 OTA 通用错误。"""


class OtaTimeout(OtaError):
    """等待设备应答超时。"""


class OtaReject(OtaError):
    """设备使用 NACK 或非零结果码拒绝命令。"""


def crc16_modbus(data: bytes) -> int:
    """计算项目原协议使用的 Modbus CRC16。"""
    value = 0xFFFF
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = (value >> 1) ^ 0xA001 if value & 1 else value >> 1
    return value & 0xFFFF


def format_version(version: int) -> str:
    major = (version >> 24) & 0xFF
    minor = (version >> 16) & 0xFF
    patch = (version >> 8) & 0xFF
    reserved = version & 0xFF
    if reserved:
        return f"V{major}.{minor}.{patch}.{reserved}"
    return f"V{major}.{minor}.{patch}"


def parse_version(value: str) -> int:
    """
    接受：
      0x01000100
      16777472
      1.0.1
      V1.0.1
      1.0.1.2
    """
    text = value.strip()
    if text[:1].lower() == "v":
        text = text[1:]

    if "." not in text:
        number = int(text, 0)
        if not 0 <= number <= 0xFFFFFFFF:
            raise argparse.ArgumentTypeError("版本号必须在 0x00000000～0xFFFFFFFF 范围内")
        return number

    parts = text.split(".")
    if len(parts) not in (3, 4):
        raise argparse.ArgumentTypeError("点分版本号必须是 Major.Minor.Patch[.Reserved]")

    try:
        numbers = [int(part, 10) for part in parts]
    except ValueError as exc:
        raise argparse.ArgumentTypeError("版本号各字段必须是十进制整数") from exc

    if len(numbers) == 3:
        numbers.append(0)

    if any(number < 0 or number > 255 for number in numbers):
        raise argparse.ArgumentTypeError("版本号每个字段必须在 0～255 范围内")

    major, minor, patch, reserved = numbers
    return (major << 24) | (minor << 16) | (patch << 8) | reserved


def app_frame(command: int, data1: int, data2: int) -> bytes:
    """构造武松 APP 固定 7 字节协议帧。"""
    frame = bytearray((APP_HEAD, command & 0xFF, data1 & 0xFF, data2 & 0xFF, 0, 0, APP_TAIL))
    value = crc16_modbus(frame[:4])
    frame[4] = (value >> 8) & 0xFF
    frame[5] = value & 0xFF
    return bytes(frame)


def app_frame_valid(frame: bytes) -> bool:
    return (
        len(frame) == 7
        and frame[0] == APP_HEAD
        and frame[6] == APP_TAIL
        and ((frame[4] << 8) | frame[5]) == crc16_modbus(frame[:4])
    )


def read_exact(port: serial.Serial, size: int, deadline: float) -> bytes:
    result = bytearray()
    while len(result) < size and time.monotonic() < deadline:
        data = port.read(size - len(result))
        if data:
            result.extend(data)
    return bytes(result)


def read_app_frame(port: serial.Serial, timeout: float) -> bytes | None:
    """从连续字节流中寻找一个合法的 7 字节 APP 帧。"""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        first = port.read(1)
        if not first:
            continue
        if first[0] != APP_HEAD:
            continue

        rest = read_exact(port, 6, deadline)
        if len(rest) != 6:
            return None

        frame = first + rest
        if app_frame_valid(frame):
            return frame

        # 当前 7 字节不是合法帧时继续搜索下一个 0xAA。
    return None


def get_app_version(port: serial.Serial, timeout: float = 2.0) -> int:
    """
    发送 0x12 查询，并在同一个接收窗口内收齐 0x21、0x22 两帧。
    允许其他业务帧插在两个版本帧之间。
    """
    query = app_frame(APP_CMD_VERSION_QUERY, 0x00, 0x01)
    port.reset_input_buffer()
    port.write(query)
    port.flush()
    print("版本查询发送：", query.hex(" ").upper())

    high: tuple[int, int] | None = None
    low: tuple[int, int] | None = None
    deadline = time.monotonic() + timeout

    while time.monotonic() < deadline:
        frame = read_app_frame(port, max(0.05, deadline - time.monotonic()))
        if frame is None:
            break

        command, data1, data2 = frame[1], frame[2], frame[3]
        if command == APP_CMD_VERSION_HIGH:
            high = (data1, data2)
            print("版本高位应答：", frame.hex(" ").upper())
        elif command == APP_CMD_VERSION_LOW:
            low = (data1, data2)
            print("版本低位应答：", frame.hex(" ").upper())

        if high is not None and low is not None:
            major, minor = high
            patch, reserved = low
            return (major << 24) | (minor << 16) | (patch << 8) | reserved

    missing = []
    if high is None:
        missing.append("0x21")
    if low is None:
        missing.append("0x22")
    raise OtaError("未收齐 APP 版本应答：" + "、".join(missing))


def enter_bootloader(port: serial.Serial) -> bool:
    """
    发送 APP 升级入口帧。
    返回 True 表示收到了 APP 的相同 7 字节回显；
    如果设备本来就在 Bootloader 中，则可能没有该回显。
    """
    frame = app_frame(APP_CMD_ENTER_BOOT, 0x42, 0x4F)
    port.reset_input_buffer()
    port.write(frame)
    port.flush()
    print("进入 Bootloader 发送：", frame.hex(" ").upper())

    deadline = time.monotonic() + 0.8
    echoed = False
    while time.monotonic() < deadline:
        reply = read_app_frame(port, max(0.05, deadline - time.monotonic()))
        if reply is None:
            break
        if reply == frame:
            echoed = True
            print("进入 Bootloader 回显：", reply.hex(" ").upper())
            break

    # APP 回显后延时 100 ms 再复位，给 Bootloader 留出启动时间。
    time.sleep(0.35)
    port.reset_input_buffer()
    return echoed


def ota_frame(command: int, sequence: int, payload: bytes = b"") -> bytes:
    if len(payload) > OTA_MAX_PAYLOAD_SIZE:
        raise OtaError(f"OTA 负载过大：{len(payload)} > {OTA_MAX_PAYLOAD_SIZE}")

    header = struct.pack(
        "<BBHH",
        OTA_PROTOCOL_VERSION,
        command & 0xFF,
        sequence & 0xFFFF,
        len(payload),
    )
    crc = binascii.crc32(header + payload) & 0xFFFFFFFF
    return b"\xAA\x5A" + header + payload + struct.pack("<I", crc)


def read_ota_frame(port: serial.Serial, timeout: float):
    deadline = time.monotonic() + timeout

    while time.monotonic() < deadline:
        first = port.read(1)
        if first != b"\xAA":
            continue

        second = read_exact(port, 1, deadline)
        if second != b"\x5A":
            continue

        header = read_exact(port, 6, deadline)
        if len(header) != 6:
            return None

        version, command, sequence, payload_length = struct.unpack("<BBHH", header)
        if version != OTA_PROTOCOL_VERSION:
            continue
        if payload_length > OTA_MAX_PAYLOAD_SIZE:
            continue

        body = read_exact(port, payload_length + 4, deadline)
        if len(body) != payload_length + 4:
            return None

        payload = body[:payload_length]
        received_crc = struct.unpack_from("<I", body, payload_length)[0]
        calculated_crc = binascii.crc32(header + payload) & 0xFFFFFFFF
        if received_crc != calculated_crc:
            continue

        return command, sequence, payload

    return None


class OtaClient:
    def __init__(self, port: serial.Serial):
        self.port = port
        self.sequence = 0

    def request(
        self,
        command: int,
        payload: bytes = b"",
        timeout: float = 2.0,
        retries: int = 3,
    ) -> tuple[int, int, int]:
        self.sequence = self.sequence % 0xFFFF + 1
        request_frame = ota_frame(command, self.sequence, payload)

        for attempt in range(1, retries + 1):
            self.port.write(request_frame)
            self.port.flush()

            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                packet = read_ota_frame(
                    self.port,
                    max(0.05, deadline - time.monotonic()),
                )
                if packet is None:
                    break

                response_command, sequence, body = packet
                if sequence != self.sequence:
                    continue
                if response_command not in (OTA_ACK, OTA_NACK):
                    continue
                if len(body) != 10 or body[0] != command:
                    continue

                result = body[1]
                state = body[2]
                value = struct.unpack_from("<I", body, 4)[0]
                max_data = struct.unpack_from("<H", body, 8)[0]

                if response_command == OTA_NACK or result != 0:
                    result_name = RESULT_NAMES.get(result, f"0x{result:02X}")
                    raise OtaReject(
                        f"命令 0x{command:02X} 失败：{result_name}，"
                        f"state={STATE_NAMES.get(state, state)}，"
                        f"value=0x{value:08X}"
                    )

                return state, value, max_data

            print(f"命令 0x{command:02X} 第 {attempt}/{retries} 次等待超时")

        raise OtaTimeout(f"命令 0x{command:02X} 无应答")


def connect_bootloader(port: serial.Serial) -> tuple[OtaClient, tuple[int, int, int]]:
    client = OtaClient(port)

    # 不能先向仍在运行的 APP 发送 AA 5A OTA 帧，以免干扰原 7 字节协议同步。
    enter_bootloader(port)

    last_error: Exception | None = None
    for _ in range(25):
        try:
            hello = client.request(OTA_HELLO, timeout=0.6, retries=1)
            return client, hello
        except OtaError as exc:
            last_error = exc
            time.sleep(0.2)

    raise OtaError(f"无法连接 Bootloader：{last_error}")


def check_bin(path: Path) -> tuple[bytes, int]:
    if not path.is_file():
        raise OtaError(f"BIN 文件不存在：{path}")

    data = path.read_bytes()
    if len(data) < 8:
        raise OtaError(f"BIN 文件过小：{len(data)} 字节")
    if len(data) > OTA_MAX_IMAGE:
        raise OtaError(
            f"BIN 大小 {len(data)} 超过武松 OTA 上限 {OTA_MAX_IMAGE} "
            f"（0x{OTA_MAX_IMAGE:X}）"
        )

    msp, reset_handler = struct.unpack_from("<II", data)
    reset_address = reset_handler & ~1

    if not SRAM_START <= msp < SRAM_END:
        raise OtaError(f"初始 MSP 无效：0x{msp:08X}")

    if (reset_handler & 1) == 0:
        raise OtaError(f"Reset 向量不是 Thumb 地址：0x{reset_handler:08X}")

    if not APP_ADDR <= reset_address < APP_END_ADDR:
        raise OtaError(
            f"Reset 向量无效：0x{reset_handler:08X}，"
            f"APP 必须链接到 0x{APP_ADDR:08X} 分区"
        )

    image_crc = binascii.crc32(data) & 0xFFFFFFFF

    print("BIN 检查通过：")
    print("  文件：", path.resolve())
    print(f"  大小：{len(data)} 字节（{len(data) / 1024:.2f} KiB）")
    print(f"  初始 MSP：0x{msp:08X}")
    print(f"  Reset 向量：0x{reset_handler:08X}")
    print(f"  CRC32：0x{image_crc:08X}")
    print("  MD5：", hashlib.md5(data).hexdigest())

    return data, image_crc


def print_boot_info(hello: tuple[int, int, int]) -> None:
    state, boot_version, max_data = hello
    print(
        f"Bootloader 连接成功：版本 0x{boot_version:08X}，"
        f"状态 {STATE_NAMES.get(state, state)}，"
        f"最大数据块 {max_data} 字节"
    )


def upgrade(
    port: serial.Serial,
    bin_path: Path,
    target_version: int,
    chunk_size: int,
    skip_version_check: bool,
) -> None:
    data, image_crc = check_bin(bin_path)

    current_version: int | None = None
    try:
        current_version = get_app_version(port, timeout=2.0)
        print(
            f"当前 APP 版本：{format_version(current_version)} "
            f"（0x{current_version:08X}）"
        )
    except OtaError as exc:
        print("当前未读取到 APP 版本，将继续尝试进入 Bootloader：", exc)

    if target_version == 0:
        print("警告：未指定有效目标版本，Bootloader 元数据中的 version_code 将写入 0。")
        print("      建议使用 --target-version V1.0.1 等参数。")
    else:
        print(
            f"目标 APP 版本：{format_version(target_version)} "
            f"（0x{target_version:08X}）"
        )

    client, hello = connect_bootloader(port)
    print_boot_info(hello)

    _, _, boot_max_data = hello
    actual_chunk_size = min(chunk_size, boot_max_data, OTA_MAX_DATA_SIZE)
    if actual_chunk_size <= 0 or actual_chunk_size % 4 != 0:
        raise OtaError("chunk-size 必须是 4 的倍数，并且不超过 1024")

    print("[1/5] BEGIN：擦除升级缓存")
    begin_payload = OTA_TARGET_MAGIC + struct.pack(
        "<III",
        target_version,
        len(data),
        image_crc,
    )
    client.request(OTA_BEGIN, begin_payload, timeout=15.0, retries=2)

    print("[2/5] DATA：发送固件")
    offset = 0
    last_percent = -1

    while offset < len(data):
        block = data[offset : offset + actual_chunk_size]
        payload = struct.pack("<IH", offset, len(block)) + block
        _, next_offset, _ = client.request(
            OTA_DATA,
            payload,
            timeout=4.0,
            retries=5,
        )

        if next_offset <= offset or next_offset > len(data):
            raise OtaError(
                f"设备返回非法 nextOffset：{next_offset}，当前 offset：{offset}"
            )

        offset = next_offset
        percent = offset * 100 // len(data)
        if percent != last_percent:
            print(f"  {offset}/{len(data)} bytes（{percent}%）")
            last_percent = percent

    print("[3/5] END：校验整包 CRC32 和向量表")
    state, _, _ = client.request(OTA_END, timeout=12.0, retries=2)
    if state not in (2, 4):
        state, _, _ = client.request(OTA_STATUS, timeout=2.0, retries=2)
    if state not in (2, 4):
        raise OtaError(f"END 后状态异常：{STATE_NAMES.get(state, state)}")

    print("[4/5] INSTALL：安装固件")
    try:
        client.request(OTA_INSTALL, timeout=45.0, retries=1)
    except OtaTimeout as exc:
        # 安装成功后 Bootloader 会复位，串口应答可能因复位而丢失。
        print("  INSTALL 应答超时，设备可能已完成安装并复位：", exc)

    print("[5/5] 查询升级后的 APP 版本")
    if skip_version_check:
        print("已通过 --skip-version-check 跳过升级后版本查询。")
        print("升级数据发送和安装流程结束。")
        return

    deadline = time.monotonic() + 30.0
    last_error: Exception | None = None

    while time.monotonic() < deadline:
        try:
            installed_version = get_app_version(port, timeout=1.5)
            print(
                f"升级后 APP 版本：{format_version(installed_version)} "
                f"（0x{installed_version:08X}）"
            )

            if target_version and installed_version != target_version:
                raise OtaError(
                    f"升级后版本不一致：期望 0x{target_version:08X}，"
                    f"实际 0x{installed_version:08X}。"
                    "请确认固件中的 AppVersion.h 与 --target-version 一致。"
                )

            print("武松串口升级流程完成。")
            return
        except OtaError as exc:
            last_error = exc
            time.sleep(0.5)

    raise OtaError(f"升级后未读取到 APP 版本：{last_error}")


def open_port(name: str, baud: int) -> serial.Serial:
    try:
        port = serial.Serial(
            name,
            baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.05,
            write_timeout=5.0,
        )
    except serial.SerialException as exc:
        raise OtaError(f"无法打开串口 {name}：{exc}") from exc

    print(f"串口打开成功：{name}, {baud}, 8N1")
    return port


def add_serial_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--port", default="COM4", help="串口号，默认 COM4")
    parser.add_argument("--baud", type=int, default=115200, help="波特率，默认 115200")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="武松球盘串口 OTA 升级工具")
    commands = parser.add_subparsers(dest="command", required=True)

    version_cmd = commands.add_parser("version", help="查询当前 APP 版本")
    add_serial_args(version_cmd)

    boot_cmd = commands.add_parser("bootloader", help="进入并连接 Bootloader")
    add_serial_args(boot_cmd)

    status_cmd = commands.add_parser("status", help="进入 Bootloader 后查询 OTA 状态")
    add_serial_args(status_cmd)

    abort_cmd = commands.add_parser("abort", help="进入 Bootloader 后清除当前 OTA 会话状态")
    add_serial_args(abort_cmd)

    reboot_cmd = commands.add_parser("reboot", help="进入 Bootloader 后请求设备复位")
    add_serial_args(reboot_cmd)

    upgrade_cmd = commands.add_parser("upgrade", help="升级武松 APP BIN")
    add_serial_args(upgrade_cmd)
    upgrade_cmd.add_argument("bin", type=Path, help="链接地址为 0x0800C000 的 APP BIN")
    upgrade_cmd.add_argument(
        "--target-version",
        type=parse_version,
        default=0,
        help="目标版本，例如 V1.0.1、1.0.1 或 0x01000100",
    )
    upgrade_cmd.add_argument(
        "--chunk-size",
        type=int,
        default=1024,
        help="每包固件数据长度，必须是 4 的倍数，默认 1024",
    )
    upgrade_cmd.add_argument(
        "--skip-version-check",
        action="store_true",
        help="安装后不查询 APP 版本",
    )

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    try:
        print(f"工具版本：{TOOL_VERSION}")
        with open_port(args.port, args.baud) as port:
            if args.command == "version":
                version = get_app_version(port)
                print(
                    f"武松 APP 版本：{format_version(version)} "
                    f"（0x{version:08X}）"
                )

            elif args.command == "bootloader":
                _, hello = connect_bootloader(port)
                print_boot_info(hello)

            elif args.command == "status":
                client, hello = connect_bootloader(port)
                print_boot_info(hello)
                state, value, max_data = client.request(OTA_STATUS)
                print(
                    f"OTA 状态：{STATE_NAMES.get(state, state)}，"
                    f"value=0x{value:08X}，最大数据块={max_data}"
                )

            elif args.command == "abort":
                client, hello = connect_bootloader(port)
                print_boot_info(hello)
                state, _, _ = client.request(OTA_ABORT)
                print(f"OTA 会话已清除，当前状态：{STATE_NAMES.get(state, state)}")

            elif args.command == "reboot":
                client, hello = connect_bootloader(port)
                print_boot_info(hello)
                try:
                    client.request(OTA_REBOOT, timeout=2.0, retries=1)
                except OtaTimeout:
                    pass
                print("已发送 Bootloader 复位命令。")

            elif args.command == "upgrade":
                upgrade(
                    port=port,
                    bin_path=args.bin,
                    target_version=args.target_version,
                    chunk_size=args.chunk_size,
                    skip_version_check=args.skip_version_check,
                )

        return 0

    except KeyboardInterrupt:
        print("用户取消。", file=sys.stderr)
        return 130
    except (OtaError, OSError, serial.SerialException, ValueError) as exc:
        print("错误：", exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
