#!/usr/bin/env python3

# This file is chatgpt slop that I will be splitting out in the future

import serial
import struct
import time
import math
import sys

from audio_test import (
    SerialPacketReceiver,
    VoiceReceiver,
    encode_mulaw,
    decode_mulaw,
    frame_packet,
    SAMPLE_RATE,
    FRAME_MS,
    SAMPLES_PER_FRAME,
    VOICE_PACKET_SIZE,
    PACKET_TYPE_VOICE,
    SERIAL_A,
    SERIAL_B,
    BAUD_RATE,
)

# ============================================================
# Tone configuration
# ============================================================

TONE_FREQUENCY = 440       # Hz
TONE_AMPLITUDE = 0.5       # 50%

TEST_SECONDS = 10

# ============================================================
# Generate one tone frame
# ============================================================

def generate_tone_frame(sequence):

    pcm = bytearray()

    # Global sample index.
    start_sample = (sequence * SAMPLES_PER_FRAME)

    for i in range(SAMPLES_PER_FRAME):
        sample_index = start_sample + i
        t = sample_index / SAMPLE_RATE

        value = (
            TONE_AMPLITUDE
            * 32767
            * math.sin(
                2 * math.pi
                * TONE_FREQUENCY
                * t
            )
        )

        value = int(value)
        pcm += struct.pack("<h", value)

    return bytes(pcm)


# ============================================================
# Tone transmitter
# ============================================================

class ToneTransmitter:

    def __init__(self, serial_port):
        self.serial = serial_port

    def send(self, sequence):

        pcm = generate_tone_frame(sequence)

        # Use the EXACT same μ-law encoder
        # as the normal voice path.

        audio = encode_mulaw(pcm)

        packet = (
            bytes([PACKET_TYPE_VOICE])
            + struct.pack(">H", sequence)
            + audio
        )

        self.serial.write(frame_packet(packet))
        self.serial.flush()


# ============================================================
# Tone verifier
# ============================================================

class ToneVerifier:

    def __init__(self):

        self.expected_sequence = None

        self.received = 0
        self.lost = 0

        self.total_samples = 0

        self.total_squared_error = 0.0

        self.max_error = 0

    def receive(self, packet):

        if len(packet) != VOICE_PACKET_SIZE:

            print(
                f"FAIL: unexpected packet size "
                f"{len(packet)}"
            )

            return

        sequence = struct.unpack(
            ">H",
            packet[1:3]
        )[0]

        audio = packet[3:]

        # ----------------------------------------------------
        # Sequence verification
        # ----------------------------------------------------

        if self.expected_sequence is not None:

            expected = self.expected_sequence

            if sequence != expected:

                lost = (
                    sequence - expected
                ) & 0xFFFF

                self.lost += lost

                print(
                    f"LOSS: expected "
                    f"{expected}, got "
                    f"{sequence}, lost "
                    f"{lost}"
                )

        self.expected_sequence = (
            sequence + 1
        ) & 0xFFFF

        self.received += 1

        # ----------------------------------------------------
        # Decode received μ-law
        # ----------------------------------------------------

        received_pcm = decode_mulaw(
            audio
        )

        # ----------------------------------------------------
        # Generate what SHOULD have arrived
        # ----------------------------------------------------

        expected_pcm = generate_tone_frame(
            sequence
        )

        # Because μ-law is intentionally lossy,
        # compare against the expected μ-law round trip.

        expected_pcm = decode_mulaw(
            encode_mulaw(
                expected_pcm
            )
        )

        received_samples = struct.unpack(
            "<" + "h" * SAMPLES_PER_FRAME,
            received_pcm
        )

        expected_samples = struct.unpack(
            "<" + "h" * SAMPLES_PER_FRAME,
            expected_pcm
        )

        # ----------------------------------------------------
        # Calculate error
        # ----------------------------------------------------

        for received, expected in zip(
            received_samples,
            expected_samples
        ):

            error = (
                received - expected
            )

            self.total_squared_error += (
                error * error
            )

            self.max_error = max(
                self.max_error,
                abs(error)
            )

            self.total_samples += 1

        if sequence % 50 == 0:

            print(
                f"RX seq={sequence}, "
                f"packets={self.received}, "
                f"lost={self.lost}"
            )

    def report(self):

        if self.total_samples == 0:

            print()
            print("FAIL: no audio received")
            return False

        rmse = math.sqrt(
            self.total_squared_error
            / self.total_samples
        )

        total_expected = (
            self.received
            + self.lost
        )

        loss_percent = 0

        if total_expected:

            loss_percent = (
                100.0
                * self.lost
                / total_expected
            )

        print()
        print("========================================")
        print(" Audio Tone Test Results")
        print("========================================")

        print(f"Packets received : {self.received}")
        print(f"Packets lost     : {self.lost}")
        print(f"Packet loss      : {loss_percent:.3f}%")
        print(f"Samples checked  : {self.total_samples}")
        print(f"Audio RMSE       : {rmse:.2f}")
        print(f"Maximum error    : {self.max_error}")

        # For a byte-for-byte radio path,
        # there should be essentially zero error
        # relative to the μ-law encode/decode result.

        passed = (
            self.lost == 0
            and rmse == 0
        )

        print()

        if passed:
            print("PASS")
        else:
            print("FAIL")

        return passed


# ============================================================
# Main
# ============================================================

def main():

    print(f"Tone frequency : {TONE_FREQUENCY} Hz")
    print(f"Sample rate    : {SAMPLE_RATE} Hz")
    print(f"Frame size     : {FRAME_MS} ms")
    print(f"Samples/frame  : {SAMPLES_PER_FRAME}")
    print(f"Packet size    : {VOICE_PACKET_SIZE}")
    print()

    # --------------------------------------------------------
    # Create Serial connections and wrappers
    # --------------------------------------------------------

    giga_a = serial.Serial(
        SERIAL_A,
        BAUD_RATE,
        timeout=0
    )

    giga_b = serial.Serial(
        SERIAL_B,
        BAUD_RATE,
        timeout=0
    )

    receiver = SerialPacketReceiver(
        giga_b
    )

    transmitter = ToneTransmitter(
        giga_a
    )

    verifier = ToneVerifier()

    # --------------------------------------------------------
    # Send tone
    # --------------------------------------------------------

    total_frames = int(
        TEST_SECONDS
        * 1000
        / FRAME_MS
    )

    print(f"Sending {total_frames} tone frames...")
    start = time.monotonic()

    for sequence in range(total_frames):

        transmitter.send(
            sequence & 0xFFFF
        )

        # Allow receiver to process data
        # while maintaining 20 ms audio timing.

        target = (
            start
            + (sequence + 1)
            * FRAME_MS
            / 1000.0
        )

        while time.monotonic() < target:

            packets = (
                receiver.receive_packets()
            )

            for packet in packets:

                if (
                    len(packet) > 0
                    and packet[0] == PACKET_TYPE_VOICE
                ):

                    verifier.receive(
                        packet
                    )

            time.sleep(0.001)

    # --------------------------------------------------------
    # Drain remaining packets
    # --------------------------------------------------------

    print()
    print("Draining receiver...")

    drain_until = (
        time.monotonic() + 2
    )

    while time.monotonic() < drain_until:

        packets = (
            receiver.receive_packets()
        )

        for packet in packets:

            if (
                len(packet) > 0
                and packet[0] == PACKET_TYPE_VOICE
            ):

                verifier.receive(
                    packet
                )

        time.sleep(0.001)

    # --------------------------------------------------------
    # Results
    # --------------------------------------------------------

    passed = verifier.report()

    giga_a.close()
    giga_b.close()

    sys.exit(
        0 if passed else 1
    )

if __name__ == "__main__":

    main()