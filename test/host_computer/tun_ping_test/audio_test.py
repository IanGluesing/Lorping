#!/usr/bin/env python3

# This file is chatgpt slop that I will be splitting out in the future

import socket
import struct
import fcntl
import select
import ctypes
import serial
import sys
import time
import threading
import queue
import argparse
import sounddevice as sd


# ============================================================
# Configuration
# ============================================================

BAUD_RATE = 115200

# Change these if necessary.
SERIAL_A = "/dev/cu.usbmodem2101"
SERIAL_B = "/dev/cu.usbmodem101"

TUN_A_IP = "10.99.0.1"
TUN_B_IP = "10.99.0.2"

# ============================================================
# Voice configuration
# ============================================================

SAMPLE_RATE = 2000
CHANNELS = 1

# 20 ms audio packets
FRAME_MS = 20

SAMPLES_PER_FRAME = SAMPLE_RATE * FRAME_MS // 1000
BYTES_PER_PCM_FRAME = SAMPLES_PER_FRAME * 2

# Voice packet:
#
#   1 byte  packet type
#   2 bytes sequence number
#   160 bytes μ-law audio
#
# Total = 163 bytes
#
VOICE_PACKET_SIZE = 1 + 2 + SAMPLES_PER_FRAME

PACKET_TYPE_IP = 0x01
PACKET_TYPE_VOICE = 0x02


# ============================================================
# macOS constants
# ============================================================

PF_SYSTEM = 32
SYSPROTO_CONTROL = 2
AF_SYS_CONTROL = 2

CTLIOCGINFO = 0xC0644E03

UTUN_CONTROL_NAME = b"com.apple.net.utun_control"

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
# Create macOS utun
# ============================================================

def create_utun():

    sock = socket.socket(
        PF_SYSTEM,
        socket.SOCK_DGRAM,
        SYSPROTO_CONTROL
    )

    ctl_info = CtlInfo()

    ctl_info.ctl_name = UTUN_CONTROL_NAME

    fcntl.ioctl(
        sock.fileno(),
        CTLIOCGINFO,
        ctl_info
    )

    control_id = ctl_info.ctl_id

    sock.connect(
        (control_id, 0)
    )

    name_buf = sock.getsockopt(
        SYSPROTO_CONTROL,
        UTUN_OPT_IFNAME,
        64
    )

    ifname = name_buf.split(
        b"\x00",
        1
    )[0].decode()

    return sock, ifname


# ============================================================
# Serial framing
#
# Every USB serial packet:
#
#   2 bytes: payload length, big endian
#   N bytes: payload
#
# ============================================================

def frame_packet(packet):

    if len(packet) > 65535:
        raise ValueError("Packet too large")

    return struct.pack(
        ">H",
        len(packet)
    ) + packet


class SerialPacketReceiver:

    def __init__(self, serial_port):

        self.serial = serial_port
        self.buffer = bytearray()

    def receive_packets(self):

        waiting = self.serial.in_waiting

        if waiting:

            data = self.serial.read(waiting)

            self.buffer.extend(data)

        packets = []
        
        # if self.serial.port == "/dev/cu.usbmodem101":
        #     print(f"{self.serial.port} buffer len: {len(self.buffer)}")
        #     # if len(self.buffer) != 0 and len(self.buffer) != 45:
        #     print(self.buffer.hex())

        while True:

            if len(self.buffer) < 2:
                break

            packet_length = struct.unpack(
                ">H",
                self.buffer[0:2]
            )[0]

            total_length = 2 + packet_length

            if len(self.buffer) < total_length:
                break

            packet = bytes(
                self.buffer[2:total_length]
            )

            del self.buffer[:total_length]

            packets.append(packet)
            # for x in packets:
            #     print(x.hex(" "))

        return packets


# ============================================================
# TUN packet handling
# ============================================================

def remove_utun_header(data):

    if len(data) < 4:
        return None

    address_family = struct.unpack(
        ">I",
        data[:4]
    )[0]

    if address_family != AF_INET:

        print(
            f"WARNING: received non-IPv4 TUN packet "
            f"AF={address_family}"
        )

        return None

    return data[4:]


def add_utun_header(packet):

    return struct.pack(
        ">I",
        AF_INET
    ) + packet


# ============================================================
# μ-law encoder / decoder
#
# G.711 μ-law.
#
# We implement this directly so the program does not depend
# on the deprecated Python audioop module.
# ============================================================

def linear_to_mulaw(sample):

    # sample is signed 16-bit PCM

    sample = max(-32768, min(32767, sample))

    sign = 0

    if sample < 0:
        sign = 0x80
        sample = -sample

    # μ-law clipping
    if sample > 32635:
        sample = 32635

    sample += 132

    exponent = 7

    mask = 0x4000

    while exponent > 0 and not (sample & mask):

        exponent -= 1
        mask >>= 1

    mantissa = (
        sample >> (exponent + 3)
    ) & 0x0F

    mulaw = ~(
        sign |
        (exponent << 4) |
        mantissa
    )

    return mulaw & 0xFF


def mulaw_to_linear(value):

    value = (~value) & 0xFF

    sign = value & 0x80

    exponent = (
        value >> 4
    ) & 0x07

    mantissa = value & 0x0F

    sample = (
        ((mantissa << 3) + 132)
        << exponent
    ) - 132

    if sign:
        sample = -sample

    return sample


def encode_mulaw(pcm_bytes):

    samples = struct.unpack(
        "<" + "h" * (len(pcm_bytes) // 2),
        pcm_bytes
    )

    encoded = bytearray(
        len(samples)
    )

    for i, sample in enumerate(samples):

        encoded[i] = linear_to_mulaw(
            sample
        )

    return bytes(encoded)


def decode_mulaw(mulaw_bytes):

    pcm = bytearray(
        len(mulaw_bytes) * 2
    )

    for i, value in enumerate(mulaw_bytes):

        sample = mulaw_to_linear(value)

        struct.pack_into(
            "<h",
            pcm,
            i * 2,
            sample
        )

    return bytes(pcm)


# ============================================================
# Voice transmitter
# ============================================================

class VoiceTransmitter:

    def __init__(self, serial_port):

        self.serial = serial_port

        self.sequence = 0

        self.running = True

        self.stream = None

    def audio_callback(
        self,
        indata,
        frames,
        time_info,
        status
    ):

        if status:
            print(
                f"Audio input: {status}",
                file=sys.stderr
            )

        pcm = bytes(indata)

        if len(pcm) != BYTES_PER_PCM_FRAME:
            print(
                f"WARNING: microphone gave "
                f"{len(pcm)} bytes, expected "
                f"{BYTES_PER_PCM_FRAME}"
            )
            return

        audio = encode_mulaw(pcm)

        sequence = self.sequence

        self.sequence = (
            self.sequence + 1
        ) & 0xFFFF

        packet = (
            bytes([PACKET_TYPE_VOICE])
            + struct.pack(">H", sequence)
            + audio
        )

        try:

            self.serial.write(
                frame_packet(packet)
            )
            self.serial.flush()

            # Print every 50th packet so we don't flood terminal.
            if sequence % 50 == 0:

                print(
                    f"VOICE TX: seq={sequence}, "
                    f"{len(packet)} byte payload"
                )

        except serial.SerialException as e:

            print(
                f"Serial transmit error: {e}",
                file=sys.stderr
            )

    def start(self):

        print()
        print("Starting microphone...")
        print(
            f"  Sample rate : {SAMPLE_RATE} Hz"
        )
        print(
            f"  Frame size  : {FRAME_MS} ms"
        )
        print(
            f"  Samples     : {SAMPLES_PER_FRAME}"
        )
        print(
            f"  Packet size : {VOICE_PACKET_SIZE} bytes"
        )
        print()

        self.stream = sd.RawInputStream(
            samplerate=SAMPLE_RATE,
            blocksize=SAMPLES_PER_FRAME,
            channels=CHANNELS,
            dtype="int16",
            callback=self.audio_callback
        )

        self.stream.start()

    def stop(self):

        if self.stream:

            self.stream.stop()
            self.stream.close()


# ============================================================
# Voice receiver
# ============================================================

class VoiceReceiver:

    def __init__(self):

        self.audio_queue = queue.Queue(
            maxsize=50
        )

        self.stream = None

        self.expected_sequence = None

        self.received_packets = 0
        self.lost_packets = 0

        self.lock = threading.Lock()

    def audio_callback(
        self,
        outdata,
        frames,
        time_info,
        status
    ):

        if status:
            print(
                f"Audio output: {status}",
                file=sys.stderr
            )

        try:
            pcm = self.audio_queue.get_nowait()

        except queue.Empty:

            # No audio available.
            # Fill the output buffer with silence.

            outdata[:] = b"\x00" * len(outdata)

            return

        if len(pcm) != len(outdata):

            # Packet isn't the expected size.
            # Output silence rather than bad audio.

            outdata[:] = b"\x00" * len(outdata)

            return

        outdata[:] = pcm
    
    def start(self):

        print(
            "Starting speaker..."
        )

        self.stream = sd.RawOutputStream(
            samplerate=SAMPLE_RATE,
            blocksize=SAMPLES_PER_FRAME,
            channels=CHANNELS,
            dtype="int16",
            callback=self.audio_callback
        )

        self.stream.start()
        
    def receive(self, packet):

        if len(packet) != VOICE_PACKET_SIZE:

            print(
                f"WARNING: invalid voice packet size "
                f"{len(packet)}"
            )

            return

        sequence = struct.unpack(
            ">H",
            packet[1:3]
        )[0]

        audio = packet[3:]

        # Print every 50th received packet.
        if sequence % 50 == 0:

            print(
                f"VOICE RX: seq={sequence}, "
                f"{len(packet)} byte packet"
            )

        # --------------------------------------------------------
        # Detect lost packets
        # --------------------------------------------------------

        if self.expected_sequence is not None:

            expected = self.expected_sequence

            if sequence != expected:

                lost = (
                    sequence - expected
                ) & 0xFFFF

                self.lost_packets += lost

                print(
                    f"VOICE LOSS: expected "
                    f"{expected}, got "
                    f"{sequence}, lost "
                    f"{lost}"
                )

        self.expected_sequence = (
            sequence + 1
        ) & 0xFFFF

        self.received_packets += 1

        # --------------------------------------------------------
        # Decode μ-law
        # --------------------------------------------------------

        pcm = decode_mulaw(audio)

        # --------------------------------------------------------
        # Queue for speaker
        # --------------------------------------------------------

        try:

            self.audio_queue.put_nowait(
                pcm
            )

        except queue.Full:
            print("queue full")

            try:
                self.audio_queue.get_nowait()
            except queue.Empty:
                pass

            try:
                self.audio_queue.put_nowait(
                    pcm
                )
            except queue.Full:
                pass

    def stop(self):

        if self.stream:

            self.stream.stop()
            self.stream.close()


# ============================================================
# Parse command line
# ============================================================

def parse_args():

    parser = argparse.ArgumentParser(
        description="SX1262 FSK voice/TUN modem"
    )

    parser.add_argument(
        "--side",
        choices=["A", "B"],
        required=True,
        help="Which radio endpoint this Mac is"
    )

    parser.add_argument(
        "--serial",
        default=None,
        help="Override serial device"
    )

    parser.add_argument(
        "--no-tun",
        action="store_true",
        help="Disable TUN/IP networking"
    )

    return parser.parse_args()


# ============================================================
# Main
# ============================================================

def main():

    print()
    print("========================================")
    print(" SX1262 FSK Voice/TUN modem")
    print("========================================")
    print()

    # ========================================================
    # Open GIGA serial ports
    # ========================================================

    print(f"Opening GIGA A: {SERIAL_A}")

    giga_a_serial = serial.Serial(
        SERIAL_A,
        BAUD_RATE,
        timeout=0
    )

    print(f"Opening GIGA B: {SERIAL_B}")

    giga_b_serial = serial.Serial(
        SERIAL_B,
        BAUD_RATE,
        timeout=0
    )

    # ========================================================
    # Create TUN A
    # ========================================================

    print("Creating TUN A...")

    tun_a, tun_a_name = create_utun()

    print(f"TUN A = {tun_a_name}")

    # ========================================================
    # Create TUN B
    # ========================================================

    print("Creating TUN B...")

    tun_b, tun_b_name = create_utun()

    print(f"TUN B = {tun_b_name}")

    # ========================================================
    # Serial packet receivers
    # ========================================================

    receive_serial_from_giga_a = SerialPacketReceiver(
        giga_a_serial
    )

    receive_serial_from_giga_b = SerialPacketReceiver(
        giga_b_serial
    )

    # ========================================================
    # Voice transmitter
    #
    # Mac microphone -> GIGA A -> FSK
    # ========================================================

    voice_tx = VoiceTransmitter(
        giga_a_serial
    )

    # ========================================================
    # Voice receiver
    #
    # FSK -> GIGA B -> Mac speaker
    # ========================================================

    voice_rx = VoiceReceiver()

    # ========================================================
    # Start audio
    # ========================================================

    print()
    print("Starting speaker...")

    voice_rx.start()

    print("Starting microphone...")

    voice_tx.start()

    # ========================================================
    # Display configuration
    # ========================================================

    print()
    print("========================================")
    print(" Interfaces")
    print("========================================")

    print("GIGA A:", giga_a_serial.port)
    print("GIGA B:", giga_b_serial.port)

    print()
    print("TUN A:", tun_a_name)
    print("TUN B:", tun_b_name)

    print()
    print("========================================")
    print(" Voice")
    print("========================================")

    print(
        f"Sample rate : {SAMPLE_RATE} Hz"
    )

    print(
        f"Frame size  : {FRAME_MS} ms"
    )

    print(
        f"Samples     : {SAMPLES_PER_FRAME}"
    )

    print(
        f"Voice packet: {VOICE_PACKET_SIZE} bytes"
    )

    print()
    print(
        "Voice path:"
    )

    print(
        "Mac microphone"
    )

    print(
        "    -> GIGA A"
    )

    print(
        "    -> SX1262 FSK"
    )

    print(
        "    -> SX1262 FSK"
    )

    print(
        "    -> GIGA B"
    )

    print(
        "    -> Mac speaker"
    )

    # ========================================================
    # TUN configuration
    # ========================================================

    print()
    print("Configure the TUN interfaces from another terminal:")
    print()

    print(
        f"  sudo ifconfig {tun_a_name} "
        f"{TUN_A_IP} {TUN_B_IP}"
    )

    print(
        f"  sudo ifconfig {tun_b_name} "
        f"{TUN_B_IP} {TUN_A_IP}"
    )

    print()
    print(
        f"Then test with:"
    )

    print()
    print(
        f"  ping {TUN_B_IP}"
    )

    print()
    print("Press Ctrl-C to stop.")
    print()

    # ========================================================
    # Main forwarding loop
    # ========================================================

    try:

        while True:

            # =================================================
            # GIGA A -> Mac
            #
            # This can contain:
            #
            #   0x01 = IPv4
            #   0x02 = Voice
            #
            # Normally voice transmitted by this Mac goes
            # GIGA A -> radio -> GIGA B, so voice received
            # here from GIGA A is handled too.
            # =================================================

            packets_from_a = (
                receive_serial_from_giga_a.receive_packets()
            )

            for packet in packets_from_a:

                if len(packet) == 0:
                    continue

                packet_type = packet[0]

                # ---------------------------------------------
                # IPv4 packet
                # ---------------------------------------------

                if packet_type == PACKET_TYPE_IP:

                    ip_packet = packet[1:]

                    print(
                        "Giga A Packet -> TunA:"
                    )

                    print(
                        " ".join(
                            f"{b:02X}"
                            for b in ip_packet
                        )
                    )

                    tun_a.send(
                        add_utun_header(
                            ip_packet
                        )
                    )

                # ---------------------------------------------
                # Voice packet
                # ---------------------------------------------

                elif packet_type == PACKET_TYPE_VOICE:
                    print("got voice")

                    voice_rx.receive(
                        packet
                    )

                # ---------------------------------------------
                # Unknown packet type
                # ---------------------------------------------

                else:

                    print(
                        f"WARNING: unknown packet type "
                        f"from GIGA A: "
                        f"0x{packet_type:02X}"
                    )

            # =================================================
            # GIGA B -> Mac
            #
            # This is the important voice receive path:
            #
            # GIGA B -> Python -> speaker
            # =================================================

            packets_from_b = (
                receive_serial_from_giga_b.receive_packets()
            )

            for packet in packets_from_b:

                if len(packet) == 0:
                    continue

                packet_type = packet[0]

                # ---------------------------------------------
                # IPv4 packet
                # ---------------------------------------------

                if packet_type == PACKET_TYPE_IP:

                    ip_packet = packet[1:]

                    print(
                        "Giga B Packet -> TunB:"
                    )

                    print(
                        " ".join(
                            f"{b:02X}"
                            for b in ip_packet
                        )
                    )

                    tun_b.send(
                        add_utun_header(
                            ip_packet
                        )
                    )

                # ---------------------------------------------
                # Voice packet
                # ---------------------------------------------

                elif packet_type == PACKET_TYPE_VOICE:

                    voice_rx.receive(
                        packet
                    )

                # ---------------------------------------------
                # Unknown packet type
                # ---------------------------------------------

                else:

                    print(
                        f"WARNING: unknown packet type "
                        f"from GIGA B: "
                        f"0x{packet_type:02X}"
                    )

            # =================================================
            # TUN A -> GIGA A
            #
            # Normal IPv4 traffic:
            #
            # TUN A -> GIGA A -> FSK -> GIGA B
            # =================================================

            readable, _, _ = select.select(
                [tun_a, tun_b],
                [],
                [],
                0.001
            )

            if tun_a in readable:

                raw = tun_a.recv(
                    65535
                )

                packet = remove_utun_header(
                    raw
                )

                if packet is not None:

                    print(
                        "TunA Packet -> Giga A:"
                    )

                    print(
                        " ".join(
                            f"{b:02X}"
                            for b in packet
                        )
                    )

                    # Add IPv4 packet type.

                    framed_packet = (
                        bytes([
                            PACKET_TYPE_IP
                        ])
                        + packet
                    )

                    giga_a_serial.write(
                        frame_packet(
                            framed_packet
                        )
                    )

            # =================================================
            # TUN B -> GIGA B
            #
            # Normal IPv4 traffic:
            #
            # TUN B -> GIGA B -> FSK -> GIGA A
            # =================================================

            if tun_b in readable:

                raw = tun_b.recv(
                    65535
                )

                packet = remove_utun_header(
                    raw
                )

                if packet is not None:

                    print(
                        "TunB Packet -> Giga B:"
                    )

                    print(
                        " ".join(
                            f"{b:02X}"
                            for b in packet
                        )
                    )

                    # Add IPv4 packet type.

                    framed_packet = (
                        bytes([
                            PACKET_TYPE_IP
                        ])
                        + packet
                    )

                    giga_b_serial.write(
                        frame_packet(
                            framed_packet
                        )
                    )

            # =================================================
            # Small yield
            #
            # Do NOT use the old 50 ms sleep here.
            #
            # Voice frames arrive every 20 ms, so a 50 ms
            # sleep can cause unnecessary buffering/latency.
            # =================================================

            time.sleep(0.001)

    except KeyboardInterrupt:

        print()
        print("Stopping...")

    finally:

        print("Stopping microphone...")

        voice_tx.stop()

        print("Stopping speaker...")

        voice_rx.stop()

        print("Closing GIGA A...")

        giga_a_serial.close()

        print("Closing GIGA B...")

        giga_b_serial.close()

        print("Closing TUN A...")

        tun_a.close()

        print("Closing TUN B...")

        tun_b.close()

        print("Done.")

# ============================================================
# Entry point
# ============================================================

if __name__ == "__main__":

    main()