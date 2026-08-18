// ============================================================
// Built-In Libraries
// ============================================================

#include <Arduino.h>
#include <RadioLib.h>
#include <mbed.h>
#include <chrono>

// ============================================================
// Local Libraries
// ============================================================

#include <device_config.h>
#include <lora_config.h>

using namespace mbed;

uint8_t rxBuffer[256];
uint8_t txBuffer[256];


// ============================================================
// Peripherals
// ============================================================

// Software connection to the SX1262 Module
SX1262 radio = new Module(
    PIN_CS,
    PIN_DIO1,
    PIN_RESET,
    PIN_BUSY
);

// ============================================================
// Radio receive state
// ============================================================

volatile bool dio1Triggered = false;
volatile bool radioPacketReceived = false;

void radioReceiveISR()
{
    dio1Triggered = true;
}

// ============================================================
// Setup
// ============================================================

void setup()
{
    // --------------------------------------------------------
    // Serial Setup
    // --------------------------------------------------------

    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial);

    // --------------------------------------------------------
    // Radio Setup
    // --------------------------------------------------------

    int radio_call_status = radio.begin();

    if (radio_call_status != RADIOLIB_ERR_NONE) {
        Serial.print("FAILED, code ");
        Serial.println(radio_call_status);

        while (true) {
            delay(1000);
        }
    }

    // These settings can be used to communicate to the Waveshare USB module, or another
    // SX1262 running the TX code that this code is paired with.
    // Found here: https://www.amazon.com/dp/B0DTKDXMN2?ref=ppx_yo2ov_dt_b_fed_asin_title
    radio_call_status = radio.setFrequency(INITIAL_LORA_FREQUENCY);
    radio_call_status = radio.setBandwidth(INITIAL_LORA_BANDWIDTH);
    radio_call_status = radio.setSpreadingFactor(LORA_SPREADING_FACTOR);
    radio_call_status = radio.setCodingRate(LORA_CODING_RATE);
    radio_call_status = radio.setSyncWord(SYNC_WORD);
    radio_call_status = radio.setCRC(true);

    radio.setDio1Action(radioReceiveISR);

    // ========================================================
    // Start SX1262 receive
    // ========================================================

    int radio_result = radio.startReceive();

    if (radio_result != RADIOLIB_ERR_NONE) {
        Serial.print("startReceive failed: ");
        Serial.println(radio_result);

        while (true) {
            delay(1000);
        }
    }

    // --------------------------------------------------------
    // Setup Done
    // --------------------------------------------------------
}

// ============================================================
// Main loop
// ============================================================

void loop()
{
    // Rx from Serial, Tx Radio Packet out
    if (Serial.available()) {
        // --------------------------------------------------------
        // Serial -> GIGA -> SX1262
        //
        // Serial format:
        //   [2-byte length, big endian]
        //   [raw packet bytes]
        // --------------------------------------------------------

        static uint16_t packetLength = 0;
        static uint16_t bytesReceived = 0;

        // Read packet length
        if (packetLength == 0 && Serial.available() >= 2) {

            uint8_t lenHigh = Serial.read();
            uint8_t lenLow  = Serial.read();

            packetLength =
                ((uint16_t)lenHigh << 8) |
                lenLow;

            bytesReceived = 0;

            if (packetLength > sizeof(txBuffer)) {
                Serial.println("ERROR: packet too large");
                packetLength = 0;
                return;
            }
        }

        // Read packet bytes
        if (packetLength > 0) {

            while (Serial.available() && bytesReceived < packetLength) {

                txBuffer[bytesReceived++] = Serial.read();
            }

            // Got the entire packet
            if (bytesReceived == packetLength) {

                radio.standby();
                int status = radio.transmit(
                    txBuffer,
                    packetLength
                );
                radio.startReceive();

                if (status != RADIOLIB_ERR_NONE) {
                    Serial.print("TX FAILED: ");
                    Serial.println(status);
                }

                // Reset state for next packet
                packetLength = 0;
                bytesReceived = 0;
            }
        }
    }

    // Rx Radio packet in, Tx to Serial
    if (radioPacketReceived) {

        noInterrupts();
        radioPacketReceived = false;
        interrupts();

        // Get the actual number of bytes received
        size_t packetLength = radio.getPacketLength();

        if (packetLength > 255) {
            Serial.println("RX ERROR: packet too large");
            radio.startReceive();
            return;
        }

        int radio_call_status = radio.readData(
            rxBuffer,
            packetLength
        );

        // Return to receive mode
        radio.startReceive();

        if (radio_call_status == RADIOLIB_ERR_NONE) {

            // Send actual packet length to Mac
            Serial.write((uint8_t)(packetLength >> 8));
            Serial.write((uint8_t)(packetLength & 0xFF));

            // Send exactly the received bytes
            Serial.write(rxBuffer, packetLength);

            // Clear serial
            Serial.flush();
        }
        else {
            Serial.print("RX FAILED: ");
            Serial.println(radio_call_status);
        }
    }

    // 3. Handle the event safely in the main loop
    if (dio1Triggered) {
        dio1Triggered = false; // Reset the flag immediately

        // Read the hardware IRQ status register via SPI
        uint32_t irqStatus = radio.getIrqFlags();

        // Check if Transmission Finished
        if (irqStatus & RADIOLIB_SX126X_IRQ_TX_DONE) {
            // Serial.println(F("\nEvent: TX Done! Packet sent successfully."));
            // Handle your next action after a successful send
        }

        // Check if Data was Received
        if (irqStatus & RADIOLIB_SX126X_IRQ_RX_DONE) {
            // Serial.println(F("\nEvent: Rx Done!"));
            radioPacketReceived = true;
        }

        // Optional: Always clear the hardware flags after reading them
        radio.clearIrqFlags(RADIOLIB_SX126X_IRQ_TX_DONE | RADIOLIB_SX126X_IRQ_RX_DONE);
    }
}