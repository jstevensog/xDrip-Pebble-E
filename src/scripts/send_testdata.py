import sys
from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.qemu import QemuTransport
from libpebble2.services.appmessage import AppMessageService, \
    Uint8, Uint16, ByteArray, Int32
from libpebble2.services.install import AppInstaller, AppInstallError

from math import sin, pi
import struct
from datetime import datetime
from time import sleep
from uuid import UUID
import zipfile
import os
import subprocess

ud = UUID("51a6140e-92cc-420f-aef6-51b229666742")

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


p_qemu = subprocess.Popen(qemu_cmd)
zf = zipfile.ZipFile('build/xDrip-Pebble-E.pbw')
APP_PATH = "build/xDrip-Pebble-E.pbw"
sleep(2)

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
    sleep(2)

except AppInstallError:
    print("Failed to install watchface")
    exit(1)
except Exception as e:
    print(f"Failed to connect: {e}")
    exit(1)
# Create sine wave
bgl_list = [int(126 + sin((4*pi) * (i/49)) * 108) for i in range(0, 49)]
bgl_index = 0

def FRAMEWORK_BGL_SERIES(lst):
    return struct.pack("<i", int(datetime.now().timestamp())) +  \
        struct.pack("<H", len(lst)) + \
        b"".join([struct.pack("<H", i) for i in lst])

def FRAMEWORK_BGL_DELTA(value, display):
    return struct.pack("<b", value) + struct.pack("<B", 0b11000000 if display else 0)

def FRAMEWORK_BGL_VALUE(value):
    return struct.pack("<i", int(datetime.now().timestamp())) + struct.pack("<H", value | 0x8000)


try:
    app_message_service = AppMessageService(pebble)
    print("Sending series and delta")
    payload = {}
    if (IMAGE is None):
        payload[117] = Uint8(0)
        payload[2009] = ByteArray(FRAMEWORK_BGL_SERIES(bgl_list))
    else:
        payload[117] = Uint8(1)
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
        payload[2008] = Uint8(sv % 8)
        if (IMAGE is not None):
            payload[2010] = ByteArray(IMAGE)
        app_message_service.send_message(ud, payload)
        sleep(1)
except Exception as e:
    print(e)
    print(e.with_traceback())
    pass
