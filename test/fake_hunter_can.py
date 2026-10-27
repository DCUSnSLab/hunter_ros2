#!/usr/bin/env python3
"""
Fake Hunter base on a SocketCAN interface, for running hunter_base without a vehicle.

Sends AGX_V2 system state (0x211) and motion state (0x221) at 50 Hz and echoes the
last motion command (0x111) back as the motion state.

Usage:
  sudo modprobe vcan && sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0
  python3 test/fake_hunter_can.py [iface=vcan0] [duration_s=60]
  ros2 launch bring_up hunter_start.launch.py port_name:=vcan0
"""
import socket
import struct
import sys
import time

IFACE = sys.argv[1] if len(sys.argv) > 1 else 'vcan0'
DURATION = float(sys.argv[2]) if len(sys.argv) > 2 else 60.0
FMT = '=IB3x8s'

s = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
s.bind((IFACE,))
s.setblocking(False)


def send(can_id, data):
    s.send(struct.pack(FMT, can_id, len(data), data.ljust(8, b'\0')))


linear_mm, steer_mrad = 0, 0
n_cmd = 0
end = time.monotonic() + DURATION
next_tx = time.monotonic()
while time.monotonic() < end:
    try:
        while True:
            can_id, dlc, data = struct.unpack(FMT, s.recv(16))
            if can_id == 0x111:
                linear_mm, _, _, steer_mrad = struct.unpack('>hhhh', data)
                n_cmd += 1
    except BlockingIOError:
        pass
    now = time.monotonic()
    if now >= next_tx:
        # system state: vehicle normal, control mode CAN, battery 25.0 V
        send(0x211, bytes([0x00, 0x01]) + struct.pack('>H', 250) + bytes(4))
        send(0x221, struct.pack('>hhhh', linear_mm, 0, 0, steer_mrad))
        next_tx += 0.02
    time.sleep(0.001)
print('fake hunter: %d motion commands received, last linear %d mm/s steer %d mrad'
      % (n_cmd, linear_mm, steer_mrad))
