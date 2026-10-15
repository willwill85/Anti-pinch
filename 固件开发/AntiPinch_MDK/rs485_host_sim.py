#!/usr/bin/env python3
"""
防夹模块 RS485 主机模拟器 (协议 V0.3)
- 串口参数: 115200 8N1, 小端字节序
- 校验算法: CRC-16/MODBUS (poly=0x8005, init=0xFFFF, refin=True, refout=True)
- 支持功能码:
    0x01 查询检测状态 (回复 4 个防夹位置异常标志)
    0x02 手动触发基准线参数校准
    0x04 触发阈值设置 (0~4096)
    0x05 触发阈值读取 (0~4096)
    0xF0 查询固件版本
"""

import argparse
import glob
import struct
import sys
import time
from dataclasses import dataclass
from typing import List, Optional

import serial

SOF = 0xAA
STATUS_REQUEST = 0x01
STATUS_REPLY = 0x00
STATUS_RESEND = 0xFF

HOST_ID = 0x01
MODULE_ID = 0x30

CMD_DETECT_STATE = 0x01
CMD_CALIBRATE = 0x02
CMD_THRESH_SET = 0x04
CMD_THRESH_GET = 0x05
CMD_FW_VERSION = 0xF0

HEADER_SIZE = 7  # SOF..DATA_LEN


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


@dataclass
class Frame:
    status: int
    frame_id: int
    src_id: int
    dst_id: int
    data: bytes
    raw: bytes = b""


def build_frame(
    data: bytes,
    frame_id: int,
    status: int = STATUS_REQUEST,
    src_id: int = HOST_ID,
    dst_id: int = MODULE_ID,
) -> bytes:
    header = struct.pack(
        "<BBBBB H",
        SOF,
        status & 0xFF,
        frame_id & 0xFF,
        src_id & 0xFF,
        dst_id & 0xFF,
        len(data),
    )
    head_crc = struct.pack("<H", crc16_modbus(header))
    data_crc = struct.pack("<H", crc16_modbus(data))
    return header + head_crc + data + data_crc


class FrameParser:
    """滑动窗口逐字节解析器，抗总线噪声与伪 0xAA"""

    def __init__(self) -> None:
        self.buf = bytearray()

    def feed(self, chunk: bytes) -> List[Frame]:
        self.buf.extend(chunk)
        frames: List[Frame] = []
        while True:
            while self.buf and self.buf[0] != SOF:
                del self.buf[0]
            if len(self.buf) < HEADER_SIZE + 2:
                break
            data_len = struct.unpack_from("<H", self.buf, 5)[0]
            head_crc_rx = struct.unpack_from("<H", self.buf, HEADER_SIZE)[0]
            head_crc_calc = crc16_modbus(bytes(self.buf[:HEADER_SIZE]))
            if head_crc_rx != head_crc_calc or data_len > 64:
                del self.buf[0]
                continue
            total_len = HEADER_SIZE + 2 + data_len + 2
            if len(self.buf) < total_len:
                break
            payload = bytes(self.buf[HEADER_SIZE + 2 : HEADER_SIZE + 2 + data_len])
            data_crc_rx = struct.unpack_from("<H", self.buf, HEADER_SIZE + 2 + data_len)[0]
            if data_crc_rx != crc16_modbus(payload):
                del self.buf[0]
                continue
            raw_frame = bytes(self.buf[:total_len])
            frames.append(
                Frame(
                    status=self.buf[1],
                    frame_id=self.buf[2],
                    src_id=self.buf[3],
                    dst_id=self.buf[4],
                    data=payload,
                    raw=raw_frame,
                )
            )
            del self.buf[:total_len]
        return frames


def auto_detect_port(wait_seconds: float = 0.0) -> Optional[str]:
    deadline = time.monotonic() + wait_seconds
    while True:
        candidates = [
            p
            for p in sorted(glob.glob("/dev/cu.*"))
            if not any(
                skip in p
                for skip in ("Bluetooth-Incoming-Port", "debug-console", "wlan")
            )
        ]
        if candidates:
            return candidates[0]
        if time.monotonic() >= deadline:
            return None
        time.sleep(0.3)


def format_hex(b: bytes) -> str:
    return " ".join(f"{x:02X}" for x in b)


def decode_frame_summary(frame: Frame) -> str:
    if not frame.data:
        return "EMPTY DATA"
    cmd = frame.data[0]
    if cmd == CMD_DETECT_STATE and len(frame.data) >= 5:
        pos = frame.data[1:5]
        pos_str = " ".join(
            f"位置{i+1}:{'异常(夹)' if v else '正常'}" for i, v in enumerate(pos)
        )
        return f"[0x01 检测状态] {pos_str}"
    if cmd == CMD_CALIBRATE and len(frame.data) >= 2:
        res = "成功(0)" if frame.data[1] == 0 else f"失败({frame.data[1]})"
        return f"[0x02 参数校准] 结果={res}"
    if cmd == CMD_THRESH_SET and len(frame.data) >= 3:
        val = struct.unpack_from("<H", frame.data, 1)[0]
        return f"[0x04 阈值设置回显] 阈值={val}"
    if cmd == CMD_THRESH_GET and len(frame.data) >= 3:
        val = struct.unpack_from("<H", frame.data, 1)[0]
        return f"[0x05 阈值读取] 阈值={val}"
    if cmd == CMD_FW_VERSION and len(frame.data) >= 4:
        return f"[0xF0 固件版本] v{frame.data[1]}.{frame.data[2]}.{frame.data[3]}"
    return f"[未知功能码 0x{cmd:02X}] data={format_hex(frame.data)}"


class Rs485HostSimulator:
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 0.25) -> None:
        self.ser = serial.Serial(port=port, baudrate=baudrate, timeout=0.01)
        self.timeout = timeout
        self.frame_id = 1
        self.parser = FrameParser()

    def request(self, data: bytes) -> tuple[Optional[Frame], float, bytes]:
        fid = self.frame_id & 0xFF
        self.frame_id = (self.frame_id + 1) & 0xFF
        tx_bytes = build_frame(data=data, frame_id=fid)

        self.ser.reset_input_buffer()
        t0 = time.perf_counter()
        self.ser.write(tx_bytes)
        self.ser.flush()

        deadline = t0 + self.timeout
        while time.perf_counter() < deadline:
            chunk = self.ser.read(128)
            if chunk:
                for frame in self.parser.feed(chunk):
                    # 忽略半双工自发自收的回显帧 (src_id == HOST_ID)
                    if frame.src_id == MODULE_ID and frame.status == STATUS_REPLY:
                        elapsed_ms = (time.perf_counter() - t0) * 1000.0
                        return frame, elapsed_ms, tx_bytes
        return None, (time.perf_counter() - t0) * 1000.0, tx_bytes

    def run_all_tests(self) -> int:
        tests = [
            ("1. 查询固件版本 (0xF0)", bytes([CMD_FW_VERSION])),
            ("2. 查询检测状态 (0x01)", bytes([CMD_DETECT_STATE])),
            ("3. 设置触发阈值 1234 (0x04)", struct.pack("<BH", CMD_THRESH_SET, 1234)),
            ("4. 读取触发阈值 (0x05)", bytes([CMD_THRESH_GET])),
            ("5. 恢复触发阈值 0 (0x04)", struct.pack("<BH", CMD_THRESH_SET, 0)),
            ("6. 手动触发基线校准 (0x02)", bytes([CMD_CALIBRATE])),
        ]

        passed = 0
        print(f"=== 开始防夹模块 RS485 协议 V0.3 全功能测试 (串口: {self.ser.port}) ===\n")
        for title, payload in tests:
            reply, ms, tx_raw = self.request(payload)
            print(f"▶ {title}")
            print(f"  TX ({len(tx_raw):2d}B): {format_hex(tx_raw)}")
            if reply is None:
                print(f"  RX: ❌ 超时未收到回复 ({ms:.1f} ms)\n")
            else:
                ok_50ms = "✅ <50ms" if ms <= 50.0 else "⚠️ >50ms"
                print(f"  RX ({len(reply.raw):2d}B): {format_hex(reply.raw)}  [{ms:.2f} ms, {ok_50ms}]")
                print(f"  解析: {decode_frame_summary(reply)}\n")
                passed += 1

        print(f"=== 测试汇总: {passed}/{len(tests)} 项通过 ===")
        return 0 if passed == len(tests) else 1

    def monitor_loop(self, poll_interval: float = 0.5, duration: float = 0.0) -> None:
        print(f"=== 进入实时轮询与主动上报监听模式 (轮询间隔 {poll_interval*1000:.0f} ms) ===")
        t_start = time.monotonic()
        next_poll = time.monotonic()
        try:
            while True:
                now = time.monotonic()
                if duration > 0 and (now - t_start) >= duration:
                    break
                if now >= next_poll:
                    next_poll = now + poll_interval
                    reply, ms, _ = self.request(bytes([CMD_DETECT_STATE]))
                    ts = time.strftime("%H:%M:%S")
                    if reply:
                        print(f"[{ts}] 轮询回复 ({ms:5.2f} ms): {decode_frame_summary(reply)} | {format_hex(reply.raw)}")
                    else:
                        print(f"[{ts}] 轮询超时 ({ms:5.1f} ms)")

                chunk = self.ser.read(128)
                if chunk:
                    for frame in self.parser.feed(chunk):
                        if frame.src_id == MODULE_ID:
                            ts = time.strftime("%H:%M:%S")
                            tag = "主动上报" if frame.status == STATUS_REQUEST else "模块回复"
                            print(f"[{ts}] ⚡ {tag}: {decode_frame_summary(frame)} | {format_hex(frame.raw)}")
                time.sleep(0.01)
        except KeyboardInterrupt:
            print("\n已停止监听。")


def main() -> int:
    ap = argparse.ArgumentParser(description="防夹模块 RS485 主机模拟器 (V0.3)")
    ap.add_argument("-p", "--port", help="串口路径，例如 /dev/cu.usbserial-xxx (默认自动检测)")
    ap.add_argument("-w", "--wait", type=float, default=0.0, help="等待串口插入的秒数")
    ap.add_argument("-m", "--monitor", action="store_true", help="全功能测试后继续实时轮询监听")
    ap.add_argument("-d", "--duration", type=float, default=0.0, help="监听持续秒数 (0 为一直监听)")
    args = ap.parse_args()

    port = args.port or auto_detect_port(wait_seconds=args.wait)
    if not port:
        print("❌ 未检测到 USB 转 485 串口 (/dev/cu.usbserial* 或 /dev/cu.wchusbserial*)。")
        print("   请插入 485 转串口模块，或用 -p 指定串口。")
        return 2

    sim = Rs485HostSimulator(port=port)
    rc = sim.run_all_tests()
    if args.monitor or args.duration > 0:
        sim.monitor_loop(poll_interval=0.5, duration=args.duration)
    return rc


if __name__ == "__main__":
    sys.exit(main())
