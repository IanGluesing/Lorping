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

// ============================================================
// Tx/Rx Ring buffers
// ============================================================

// Total ring size and LoRa transmit chunk size
constexpr size_t RING_SIZE = 65565;
constexpr size_t CHUNK_SIZE = 200;

// Bytes that need to be transmitted through LoRa
uint8_t txRing[RING_SIZE];

// Bytes that were received over LoRa
uint8_t rxRing[RING_SIZE];

// Starting index of bytes to transmit
size_t txHead = 0;
// Ending index of bytes to transmit
size_t txTail = 0;
// Current number of bytes needing to be transmitted
size_t txCount = 0;

// Starting index of bytes received over LoRa
size_t rxHead = 0;
// Ending index of bytes received over LoRa
size_t rxTail = 0;
// Current number of bytes received over LoRa
size_t rxCount = 0;

// Temp buffer of bytes that will be transmitted
uint8_t radioTxBuffer[CHUNK_SIZE];

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
    // Read in characters and store in TxRing
    while (Serial.available()) {
        // Add to current tail position
        txRing[txTail] = Serial.read();

        // Update tail position and number of bytes in buffer
        txTail = (txTail + 1) % RING_SIZE;
        txCount++;
    }

    // Rx Radio packet in, Tx to Serial
    if (radioPacketReceived) {

        noInterrupts();
        radioPacketReceived = false;
        interrupts();

        // Get the actual number of bytes received
        size_t packetLength = radio.getPacketLength();

        // Read data
        int radio_call_status = radio.readData(
            rxRing,
            packetLength
        );

        // Return to receive mode
        radio.startReceive();

        if (radio_call_status == RADIOLIB_ERR_NONE) {
            // Senfd received bytes over sereal
            Serial.write(rxRing, packetLength);

            // Clear serial
            Serial.flush();
        }
        else {
            Serial.print("RX FAILED: ");
            Serial.println(radio_call_status);
        }
    }

    // Handle Interrupt
    if (dio1Triggered) {
        // Reset flag
        dio1Triggered = false;

        // Read the hardware IRQ status register via SPI
        uint32_t irqStatus = radio.getIrqFlags();

        // Check if Data was Received
        if (irqStatus & RADIOLIB_SX126X_IRQ_RX_DONE) {
            radioPacketReceived = true;
        }

        // Clear flags
        radio.clearIrqFlags(RADIOLIB_SX126X_IRQ_TX_DONE | RADIOLIB_SX126X_IRQ_RX_DONE);
    }

    // Determine number of bytes to transmit
    auto bytes_to_transmit = min(txCount, CHUNK_SIZE);

    // Read in bytes to buffer, only update the real params if transmit is successful
    auto tmp_head = txHead;
    for (int i = 0; i < bytes_to_transmit; i++) {
        radioTxBuffer[i] = txRing[tmp_head];
        // Handle potential wrap around case
        tmp_head = (tmp_head + 1) % RING_SIZE;
    }

    // Transmit if valid
    if (bytes_to_transmit > 0) {
        // Transmit logic
        radio.standby();
        int transmit_status = radio.transmit(
            radioTxBuffer,
            bytes_to_transmit
        );
        radio.startReceive();

        // Update flags if valid transmit
        if (transmit_status == RADIOLIB_ERR_NONE) {
            txCount -= bytes_to_transmit;
            txHead = tmp_head;
        }
    }
}