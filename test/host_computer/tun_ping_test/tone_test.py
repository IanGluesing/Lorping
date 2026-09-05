#!/usr/bin/env python3

# Tone test for verifying that LoRa voice packets arrive
# with exactly the same payload bytes that were transmitted.

import serial
import struct
import time
import math
import sys

from audio_test import (
    SerialPacketReceiver,
    encode_mulaw,
    SERIAL_A,
    SERIAL_B,
    BAUD_RATE,
    SAMPLE_RATE,
    FRAME_MS,
    SAMPLES_PER_FRAME,
    VOICE_PACKET_SIZE,
    PACKET_TYPE_VOICE,
    frame_packet,
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
    start_sample = (
        sequence * SAMPLES_PER_FRAME
    )

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

        pcm += struct.pack(
            "<h",
            value
        )

    return bytes(pcm)


# ============================================================
# Tone transmitter
# ============================================================

class ToneTransmitter:

    def __init__(self, serial_port):

        self.serial = serial_port

        # Save the exact μ-law audio bytes that were sent.
        #
        # Key:
        #     sequence number
        #
        # Value:
        #     exact transmitted audio bytes
        #
        # This lets the receiver perform a byte-for-byte
        # comparison against what was actually transmitted.

        self.transmitted = {}

    def send(self, sequence):

        pcm = generate_tone_frame(
            sequence
        )

        # Convert PCM to the exact bytes that will
        # be placed into the LoRa packet.

        audio = encode_mulaw(
            pcm
        )

        packet = (
            bytes([PACKET_TYPE_VOICE])
            + struct.pack(">H", sequence)
            + audio
        )

        # Save the exact transmitted audio bytes.

        self.transmitted[sequence] = audio

        # Send packet.

        self.serial.write(
            frame_packet(packet)
        )

        self.serial.flush()


# ============================================================
# Tone verifier
# ============================================================

class ToneVerifier:

    def __init__(self, transmitted):

        self.transmitted = transmitted

        self.expected_sequence = None

        self.received = 0
        self.lost = 0

        # Number of packets whose audio payload matched
        # the transmitted bytes exactly.

        self.matches = 0

        # Number of packets whose audio payload differed.

        self.mismatches = 0

        # Total number of bytes compared.

        self.bytes_checked = 0

        # Total number of bytes that differed.

        self.bytes_mismatched = 0

        # Largest number of differing bytes in one packet.

        self.max_packet_byte_errors = 0

        # First few mismatches are useful for debugging.

        self.mismatch_details = []

    def receive(self, packet):

        # ----------------------------------------------------
        # Packet size
        # ----------------------------------------------------

        if len(packet) != VOICE_PACKET_SIZE:

            print(
                f"FAIL: unexpected packet size "
                f"{len(packet)}"
            )

            return

        # ----------------------------------------------------
        # Packet type
        # ----------------------------------------------------

        if packet[0] != PACKET_TYPE_VOICE:

            print(
                f"FAIL: unexpected packet type "
                f"0x{packet[0]:02X}"
            )

            return

        # ----------------------------------------------------
        # Sequence
        # ----------------------------------------------------

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
        # Look up the exact bytes that were transmitted
        # ----------------------------------------------------

        if sequence not in self.transmitted:

            print(
                f"FAIL: received sequence "
                f"{sequence}, but no transmitted "
                f"packet with that sequence exists"
            )

            self.mismatches += 1

            return

        expected_audio = (
            self.transmitted[sequence]
        )

        # ----------------------------------------------------
        # Byte-for-byte comparison
        # ----------------------------------------------------

        if audio == expected_audio:

            self.matches += 1

        else:

            self.mismatches += 1

            # Find every byte that differs.

            byte_errors = []

            for i, (
                received_byte,
                expected_byte
            ) in enumerate(
                zip(
                    audio,
                    expected_audio
                )
            ):

                if received_byte != expected_byte:

                    byte_errors.append(
                        (
                            i,
                            expected_byte,
                            received_byte
                        )
                    )

            self.bytes_checked += len(
                expected_audio
            )

            self.bytes_mismatched += len(
                byte_errors
            )

            self.max_packet_byte_errors = max(
                self.max_packet_byte_errors,
                len(byte_errors)
            )

            # Keep only the first 10 mismatching packets
            # so a bad link doesn't spam the terminal.

            if len(self.mismatch_details) < 10:

                self.mismatch_details.append(
                    (
                        sequence,
                        byte_errors
                    )
                )

        if audio == expected_audio:

            self.bytes_checked += len(
                expected_audio
            )

        # ----------------------------------------------------
        # Progress
        # ----------------------------------------------------

        if sequence % 50 == 0:

            status = (
                "MATCH"
                if audio == expected_audio
                else "MISMATCH"
            )

            print(
                f"RX seq={sequence}, "
                f"packets={self.received}, "
                f"lost={self.lost}, "
                f"matches={self.matches}, "
                f"mismatches={self.mismatches}, "
                f"status={status}"
            )

    def report(self):

        print()
        print("========================================")
        print(" Audio Tone Test Results")
        print("========================================")

        print(
            f"Packets transmitted : "
            f"{len(self.transmitted)}"
        )

        print(
            f"Packets received    : "
            f"{self.received}"
        )

        print(
            f"Packets lost        : "
            f"{self.lost}"
        )

        print(
            f"Packets matched     : "
            f"{self.matches}"
        )

        print(
            f"Packets mismatched  : "
            f"{self.mismatches}"
        )

        total_expected = (
            self.received
            + self.lost
        )

        loss_percent = 0.0

        if total_expected:

            loss_percent = (
                100.0
                * self.lost
                / total_expected
            )

        print(
            f"Packet loss         : "
            f"{loss_percent:.3f}%"
        )

        print(
            f"Bytes checked       : "
            f"{self.bytes_checked}"
        )

        print(
            f"Bytes mismatched    : "
            f"{self.bytes_mismatched}"
        )

        if self.bytes_checked:

            byte_error_percent = (
                100.0
                * self.bytes_mismatched
                / self.bytes_checked
            )

        else:

            byte_error_percent = 0.0

        print(
            f"Byte error rate     : "
            f"{byte_error_percent:.6f}%"
        )

        print(
            f"Max byte errors/"
            f"packet              : "
            f"{self.max_packet_byte_errors}"
        )

        # ----------------------------------------------------
        # Print mismatch details
        # ----------------------------------------------------

        if self.mismatch_details:

            print()
            print("Mismatch details:")
            print()

            for (
                sequence,
                errors
            ) in self.mismatch_details:

                print(
                    f"  Sequence {sequence}: "
                    f"{len(errors)} byte errors"
                )

                # Show first 16 byte errors.

                for (
                    index,
                    expected,
                    received
                ) in errors[:16]:

                    print(
                        f"    byte {index}: "
                        f"expected 0x{expected:02X}, "
                        f"received 0x{received:02X}"
                    )

                if len(errors) > 16:

                    print(
                        f"    ... "
                        f"{len(errors) - 16} more"
                    )

        # ----------------------------------------------------
        # Pass/fail
        # ----------------------------------------------------

        passed = (
            self.received > 0
            and self.lost == 0
            and self.mismatches == 0
            and self.matches == len(
                self.transmitted
            )
        )

        print()

        if passed:

            print(
                "PASS: every received packet "
                "matched the transmitted bytes exactly"
            )

        else:

            print(
                "FAIL: transmitted and received "
                "data did not match exactly"
            )

        return passed


# ============================================================
# Main
# ============================================================

def main():

    print(
        f"Tone frequency : "
        f"{TONE_FREQUENCY} Hz"
    )

    print(
        f"Sample rate    : "
        f"{SAMPLE_RATE} Hz"
    )

    print(
        f"Frame size     : "
        f"{FRAME_MS} ms"
    )

    print(
        f"Samples/frame  : "
        f"{SAMPLES_PER_FRAME}"
    )

    print(
        f"Packet size    : "
        f"{VOICE_PACKET_SIZE}"
    )

    print()

    # --------------------------------------------------------
    # Create Serial connections
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

    verifier = ToneVerifier(
        transmitter.transmitted
    )

    # --------------------------------------------------------
    # Send tone
    # --------------------------------------------------------

    total_frames = int(
        TEST_SECONDS
        * 1000
        / FRAME_MS
    )

    print(
        f"Sending {total_frames} "
        f"tone frames..."
    )

    start = time.monotonic()

    for sequence in range(total_frames):

        sequence &= 0xFFFF

        transmitter.send(
            sequence
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
                    and packet[0]
                    == PACKET_TYPE_VOICE
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
        time.monotonic() + 10
    )

    while (time.monotonic() < drain_until) and (verifier.received != total_frames):

        packets = (
            receiver.receive_packets()
        )

        for packet in packets:

            if (
                len(packet) > 0
                and packet[0]
                == PACKET_TYPE_VOICE
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