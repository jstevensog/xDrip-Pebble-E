import sys
from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.qemu import QemuTransport
from libpebble2.protocol.base import PebblePacket
from libpebble2.protocol.base.types import Uint8, BinaryArray
from libpebble2.services.appmessage import AppMessageService, \
    ByteArray
from libpebble2.services.install import AppInstaller, AppInstallError

from math import sin, pi
import struct
from datetime import datetime
from time import sleep
from uuid import UUID
import zipfile
import os
import subprocess
import asyncio
import re

# ud = UUID("51a6140e-92cc-420f-aef6-51b229666742") # Do not use, old uuid
ud = UUID("240ff2d2-a64a-11f1-9d00-c74dac4ca2e6")

if len(sys.argv) < 2:
    print("Need device type as arg")
    exit(1)


device_type = sys.argv[1]
port = 12344

IMAGE = None
if len(sys.argv) == 3:
    with open(sys.argv[2], "rb") as f:
        IMAGE = f.read()

qemu_cmd = [
    os.path.expanduser("~/.pebble-sdk/SDKs/4.33.1/toolchain/bin/qemu-pebble"),
    "-rtc", "base=localtime",
    "-serial", "null",
    "-serial", "tcp::%d,server=on,wait=off" % port,
    "-serial", "tcp::%d,server=on,wait=off" % (port+1),
    "-monitor", "tcp::%d,server=on,wait=off" % (port+2),
    "-kernel", os.path.expanduser("~/.pebble-sdk/SDKs/4.33.1/sdk-core/pebble/%s/qemu/qemu_micro_flash.bin" % device_type),
    "-machine", "pebble-%s" % device_type if device_type in ['emery', 'gabbro', 'flint'] else "pebble-silk-bb" if device_type == 'diorite' else "cortex-s4-bb" if device_type == "chalk" else "pebble-snowy-bb" if device_type == "basalt" else "pebble-bb2",
    "-cpu", "cortex-m33" if device_type in ["emery", "gabbro"] else "cortex-m3" if device_type in ['aplite'] else "cortex-m4",
    "-audio", "driver=none,id=audio0"
]
if device_type in ['gabbro', 'emery', 'flint']:
    qemu_cmd.append("-drive")
    qemu_cmd.append("if=mtd,format=raw,file=%s" % (os.path.expanduser("~/.pebble-sdk/4.33.1/%s/qemu_spi_flash.bin" % device_type)))
elif device_type in ["aplite", "diorite"]:
    qemu_cmd.append("-mtdblock")
    qemu_cmd.append(os.path.expanduser("~/.pebble-sdk/4.33.1/%s/qemu_spi_flash.bin" % device_type))
else:
    qemu_cmd.append("-drive")
    qemu_cmd.append('if=none,id=spi-flash,file=%s,format=raw' % (os.path.expanduser("~/.pebble-sdk/4.33.1/%s/qemu_spi_flash.bin" % device_type)))


def progress_callback(sent, total, length):
    pct = (total / length) * 100
    print(f"\rUploading: {pct:5.1f}% ({total}/{length} bytes)", end="", flush=True)


# Create sine wave


class RawAppRunState(PebblePacket):
    class Meta:
        endpoint = 0x34
        endianness = '<'
    command = Uint8()
    uuid = BinaryArray(length=16)




SYNC = b"\x55\x03\x50\x21"

def split_records(stream: bytes):
    parts = stream.split(SYNC)
    return parts[1:]

def parse_record(record: bytes):
    """Pull out filename, line number, and best-effort message text."""
    # print(record.hex())
    # 0x55 [2 Protocl]  * protocoldata * [4 FCS] 0x55
    # m = re.search(rb'[\x20-\x7e]{2,64}\.c\b', record)
    try:
        filename = record[8:24].replace(b"\x01", b"").decode('ascii')
        line_no = int.from_bytes(record[35:37], 'little')
        message = record[37:-5].decode('ascii')
    except:
        print(record.hex())
    return filename, line_no, message


class LogReassembler:
    def __init__(self):
        self._key = None
        self._buf = ""

    def feed(self, filename, line_no, message):
        if message is None:
            return None
        key = (filename, line_no)
        if key == self._key:
            self._buf += message           # continuation fragment: append
            return None
        else:
            flushed = self._flush()
            self._key = key
            self._buf = message
            return flushed

    def _flush(self):
        if self._key is None or not self._buf:
            return None
        filename, lineno = self._key
        line = f"[{filename}:{lineno}] {self._buf}"
        return line

    def close(self):
        return self._flush()


async def tcp_echo_client(port):
    reader, _ = await asyncio.open_connection('127.0.0.1', port)
    buf = b""
    reassembler = LogReassembler()
    while True:
        data = await reader.read(256)
        if not data:
            final = reassembler.close()
            if final:
                print(final)
            print("Log connection closed")
            return
        buf += data
        chunks = buf.split(SYNC)
        buf = SYNC + chunks[-1]           # keep last partial record buffered
        for raw in chunks[1:-1]:
            filename, line_no, message = parse_record(SYNC + raw)
            line = f"[{filename}:{line_no}] {message}"
            # line = reassembler.feed(filename, line_no, message)
            if line:
                print(line)
def FRAMEWORK_BGL_SERIES(lst):
    return struct.pack("<i", int(datetime.now().timestamp())) +  \
        struct.pack("<H", len(lst)) + \
        b"".join([struct.pack("<H", i) for i in lst])

def FRAMEWORK_BGL_DELTA(value, display):
    return struct.pack("<b", value) + struct.pack("<B", 0b11000000 if display else 0)

def FRAMEWORK_BGL_VALUE(value):
    return struct.pack("<i", int(datetime.now().timestamp())) + struct.pack("<H", value | 0x8000)


async def send_test_data(pebble):
    try:
        bgl_index = 0
        bgl_list = [int(220 + sin((4*pi) * (i/49)) * 180) for i in range(0, 49)]
        app_message_service = AppMessageService(pebble)
        print("Sending series and delta")
        payload = {}
        if (IMAGE is None):
            payload[117] = ByteArray(struct.pack("b", 0))
            payload[2009] = ByteArray(FRAMEWORK_BGL_SERIES(bgl_list))
        else:
            payload[117] = ByteArray(struct.pack("b", 1))

        # payload[312] = ByteArray(struct.pack("b", 0x35)) # l/h line style
        # payload[313] = ByteArray(struct.pack("b", 1)) # l/h line style
        # payload[320] = ByteArray(struct.pack("b", 1)) # enable hour lines
        # payload[321] = ByteArray(struct.pack("b", 4)) # hour lines width
        # payload[322] = ByteArray(struct.pack("b", 0x32)) # hour line style
        # payload[323] = ByteArray(struct.pack("b", 0)) # auto adjust
        payload[2001] = ByteArray(FRAMEWORK_BGL_DELTA(100, True))
        app_message_service.send_message(ud, payload)
        sv = 0
        while True:
            bgl_index = bgl_index + 1
            sv = sv + 1
            print("Sending %d" % (bgl_index))
            payload = {}
            payload[2002] = ByteArray(FRAMEWORK_BGL_VALUE(bgl_list[bgl_index % len(bgl_list)]))
            payload[2001] = ByteArray(FRAMEWORK_BGL_DELTA(os.urandom(1)[0] - 128, True))
            payload[2008] = ByteArray(struct.pack("b", sv % 8))

            # payload[313] = ByteArray(struct.pack("b", sv % 8)) # l/h line style
            if (IMAGE is not None):
                payload[2010] = ByteArray(IMAGE)
            app_message_service.send_message(ud, payload)
            await asyncio.sleep(1)
    except Exception as e:
        print(e)
        print(e.with_traceback(None))
        pass

async def main():

    p_qemu = subprocess.Popen(qemu_cmd)
    APP_PATH = "build/xDrip-Pebble-E.pbw"
    await asyncio.sleep(2)

    try:
        transport = QemuTransport("localhost", port)
        pebble = PebbleConnection(transport)
        pebble.connect()
        pebble.run_async()
        print("Successfully connected to Pebble QEMU bluetooth channel.")

        installer = AppInstaller(pebble, APP_PATH)
        installer.register_handler("progress", progress_callback)
        installer.install()
        print("App installed")
        await asyncio.sleep(2)

    except AppInstallError:
        print("Failed to install watchface")
        exit(1)
    except Exception as e:
        print(f"Failed to connect: {e}")
        exit(1)
    pebble.send_packet(RawAppRunState(command=1, uuid=ud.bytes))
    await asyncio.sleep(2)
    await asyncio.gather(
        tcp_echo_client(port + 1),
        send_test_data(pebble),
    )

asyncio.run(main())
