#!/usr/bin/env python3

import socket
import struct
import fcntl
import select
import ctypes
import serial
import sys

# ============================================================
# Configuration
# ============================================================

# Serial path to each Giga board.
SERIAL_A = "/dev/cu.usbmodem2101"
SERIAL_B = "/dev/cu.usbmodem101"

BAUD_RATE = 115200

# These are the two IP addresses we will put on the TUNs.
TUN_A_IP = "10.99.0.1"
TUN_B_IP = "10.99.0.2"


# ============================================================
# macOS constants
# ============================================================

PF_SYSTEM = 32
SYSPROTO_CONTROL = 2
AF_SYS_CONTROL = 2

CTLIOCGINFO = 0xC0644E03

# com.apple.net.utun_control
UTUN_CONTROL_NAME = b"com.apple.net.utun_control"

# getsockopt option used to obtain the utun interface name
UTUN_OPT_IFNAME = 2

AF_INET = socket.AF_INET


# ============================================================
# Structures used by macOS utun
# ============================================================

class CtlInfo(ctypes.Structure):
    _fields_ = [
        ("ctl_id", ctypes.c_uint32),
        ("ctl_name", ctypes.c_char * 96),
    ]

# ============================================================
# Create a macOS utun interface
# ============================================================

def create_utun():

    sock = socket.socket(
        PF_SYSTEM,
        socket.SOCK_DGRAM,
        SYSPROTO_CONTROL
    )

    # Ask macOS for the control ID of utun_control.
    ctl_info = CtlInfo()
    ctl_info.ctl_name = UTUN_CONTROL_NAME

    fcntl.ioctl(
        sock.fileno(),
        CTLIOCGINFO,
        ctl_info
    )

    # macOS Python expects PF_SYSTEM addresses as:
    #
    #     (control_id, unit)
    #
    # unit = 0 means automatically select an available utun.

    control_id = ctl_info.ctl_id

    sock.connect(
        (control_id, 0)
    )

    # Get the name assigned by macOS.
    name_buf = sock.getsockopt(
        SYSPROTO_CONTROL,
        UTUN_OPT_IFNAME,
        64
    )

    ifname = name_buf.split(b"\x00", 1)[0].decode()

    return sock, ifname


# ============================================================
# Serial packet framing
#
# Every packet sent over USB serial is:
#
#   2 bytes: packet length, big endian
#   N bytes: complete IPv4 packet
#
# Example:
#
#   00 54
#   45 00 00 54 ...
#
# ============================================================

def frame_packet(packet):

    if len(packet) > 65535:
        raise ValueError("Packet too large")

    return struct.pack(">H", len(packet)) + packet


class SerialPacketReceiver:

    def __init__(self, serial_port):
        self.serial = serial_port
        self.buffer = bytearray()

    def receive_packets(self):

        waiting = self.serial.in_waiting

        if waiting:
            data = self.serial.read(waiting)

            print(
                f"\nRAW SERIAL {len(data)} bytes:"
            )
            print(
                " ".join(f"{b:02X}" for b in data)
            )

            self.buffer.extend(data)

        packets = []

        while True:

            if len(self.buffer) < 2:
                break

            packet_length = struct.unpack(
                ">H",
                self.buffer[0:2]
            )[0]

            print(
                f"FRAME: length={packet_length}, "
                f"buffer={len(self.buffer)}"
            )

            total_length = 2 + packet_length

            if len(self.buffer) < total_length:
                break

            packet = bytes(
                self.buffer[2:total_length]
            )

            del self.buffer[:total_length]

            packets.append(packet)

        return packets

# ============================================================
# utun packet handling
#
# macOS prepends a 4-byte address-family header.
#
# TUN -> Python:
#
#   [4-byte AF_INET][IPv4 packet]
#
# Python -> TUN:
#
#   [4-byte AF_INET][IPv4 packet]
#
# ============================================================

def remove_utun_header(data):

    if len(data) < 4:
        return None

    # macOS utun uses a 4-byte address-family header.
    # The AF value is in network byte order.
    address_family = struct.unpack(
        ">I",
        data[:4]
    )[0]

    if address_family != AF_INET:
        print(
            f"WARNING: received non-IPv4 TUN packet: "
            f"AF={address_family}"
        )
        return None

    return data[4:]


def add_utun_header(packet):

    return struct.pack(
        ">I",
        AF_INET
    ) + packet
    
import struct

# ============================================================
# Main
# ============================================================

def main():

    print()
    print("========================================")
    print(" LoRa TUN modem")
    print("========================================")
    print()

    # --------------------------------------------------------
    # Open GIGA serial ports
    # --------------------------------------------------------

    print(f"Opening GIGA A: {SERIAL_A}")

    to_giga_a_serial = serial.Serial(
        SERIAL_A,
        BAUD_RATE,
        timeout=0
    )

    print(f"Opening GIGA B: {SERIAL_B}")

    to_giga_b_serial = serial.Serial(
        SERIAL_B,
        BAUD_RATE,
        timeout=0
    )

    # --------------------------------------------------------
    # Create TUN A
    # --------------------------------------------------------

    print("Creating TUN A...")

    tun_a, tun_a_name = create_utun()

    print(f"TUN A = {tun_a_name}")

    # --------------------------------------------------------
    # Create TUN B
    # --------------------------------------------------------

    print("Creating TUN B...")

    tun_b, tun_b_name = create_utun()

    print(f"TUN B = {tun_b_name}")

    # --------------------------------------------------------
    # Serial packet receivers
    # --------------------------------------------------------

    rx_a = SerialPacketReceiver(to_giga_a_serial)
    rx_b = SerialPacketReceiver(to_giga_b_serial)

    print()
    print("========================================")
    print(" Interfaces")
    print("========================================")
    print()
    print(f"{tun_a_name} -> GIGA A -> LoRa")
    print(f"{tun_b_name} -> GIGA B -> LoRa")
    print()
    print("Configure them from another terminal with:")
    print()
    print(f"  sudo ifconfig {tun_a_name} {TUN_A_IP} {TUN_B_IP}")
    print(f"  sudo ifconfig {tun_b_name} {TUN_B_IP} {TUN_A_IP}")
    print()
    print(f"Then test with:")
    print()
    print(f"  ping {TUN_B_IP}")
    print()
    print("Press Ctrl-C to stop.")
    print()

    # --------------------------------------------------------
    # Main forwarding loop
    # --------------------------------------------------------

    try:

        while True:

            readable, _, _ = select.select(
                [
                    tun_a,
                    tun_b,
                    to_giga_a_serial,
                    to_giga_b_serial
                ],
                [],
                [],
                1.0
            )

            # =================================================
            # TUN A -> GIGA A
            # =================================================

            if tun_a in readable:

                raw = tun_a.recv(65535)

                packet = remove_utun_header(raw)

                if packet is not None:

                    # print(
                    #     f"TUN A -> GIGA A: "
                    #     f"{len(packet)} bytes"
                    # )
                    
                    print("TunA Sending Packet to Giga A:")
                    print(" ".join(f"{b:02X}" for b in packet))

                    to_giga_a_serial.write(
                        frame_packet(packet)
                    )

            # =================================================
            # TUN B -> GIGA B
            # =================================================

            if tun_b in readable:

                raw = tun_b.recv(65535)

                packet = remove_utun_header(raw)

                if packet is not None:

                    # print(
                    #     f"TUN B -> GIGA B: "
                    #     f"{len(packet)} bytes"
                    # )
                    
                    print("TunB Sending Packet to Giga B:")
                    print(" ".join(f"{b:02X}" for b in packet))

                    to_giga_b_serial.write(
                        frame_packet(packet)
                    )

            # =================================================
            # GIGA A -> TUN A
            #
            # This is data received over LoRa from GIGA B.
            # =================================================

            if to_giga_a_serial in readable:

                packets = rx_a.receive_packets()

                for packet in packets:

                    # print(
                    #     f"GIGA A -> TUN A: "
                    #     f"{len(packet)} bytes"
                    # )
                    
                    print("TunA Got Packet from Giga A:")
                    print(" ".join(f"{b:02X}" for b in packet))
                    
                    utun_packet = add_utun_header(packet)

                    try:
                        sent = tun_a.send(utun_packet)
                        print(f"UTUN A send() returned: {sent}/{len(utun_packet)} bytes")
                    except Exception as e:
                        print(f"UTUN A SEND ERROR: {e}")

            # =================================================
            # GIGA B -> TUN B
            # =================================================

            if to_giga_b_serial in readable:

                packets = rx_b.receive_packets()

                for packet in packets:

                    print("TunB Got Packet from Giga B:")
                    print(" ".join(f"{b:02X}" for b in packet))
                    
                    utun_packet = add_utun_header(packet)
                    
                    try:
                        sent = tun_b.send(utun_packet)
                        print(f"UTUN B send() returned: {sent}/{len(utun_packet)} bytes")
                    except Exception as e:
                        print(f"UTUN B SEND ERROR: {e}")

    except KeyboardInterrupt:

        print()
        print("Stopping...")

    finally:

        to_giga_a_serial.close()
        to_giga_b_serial.close()

        tun_a.close()
        tun_b.close()

        print("Done.")


# ============================================================
# Entry point
# ============================================================

if __name__ == "__main__":
    main()