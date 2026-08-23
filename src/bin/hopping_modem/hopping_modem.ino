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
#include <hop_table.h>

using namespace mbed;

#define SX1262_MODE_FSK false
#define FREQUENCY_HOPPING_ENABLED false

// ============================================================
// Tx/Rx Ring buffers
// ============================================================

// Total ring size and LoRa transmit chunk size
constexpr size_t RING_SIZE = UINT16_MAX;
constexpr size_t CHUNK_SIZE = 100; // This needs to be 200 to not fall behind in non-hopping LoRa VoIP

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

// HW Timer used to determine hop timing
Ticker hw_interrupt_timer;

// General timer used for logging
Timer logging_timer;

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
    // PPS Setup
    // --------------------------------------------------------

    // Declare PPS Pin and attach the callback to trigger on signal rising edge
    pinMode(PPS_PIN, INPUT);
    attachInterrupt(
        digitalPinToInterrupt(PPS_PIN),
        ppsISR,
        RISING
    );

    // --------------------------------------------------------
    // Radio Setup
    // --------------------------------------------------------

    int16_t radio_call_status = RADIOLIB_ERR_NONE;
    if (SX1262_MODE_FSK) {
        radio_call_status = radio.beginFSK(
            INITIAL_LORA_FREQUENCY,     // frequency MHz
            FSK_BIT_RATE_KBPS,          // bit rate kbps
            FSK_FREQ_DEVIATION_KHZ,     // frequency deviation kHz
            FSK_RX_BANDWIDTH_KHZ,       // RX bandwidth kHz
            FSK_POWER_DBM,              // power dBm
            FSK_NUM_PREAMBLE_BITS,      // preamble bits
            FSK_TXCO_VOLTAGE,           // TCXO voltage
            FSK_DCDC_REGULATOR          // DC-DC regulator
        );
    } else {
        radio_call_status = radio.begin();

        // These settings can be used to communicate to the Waveshare USB module, or another
        // SX1262 running the TX code that this code is paired with.
        // Found here: https://www.amazon.com/dp/B0DTKDXMN2?ref=ppx_yo2ov_dt_b_fed_asin_title
        radio_call_status &= radio.setFrequency(INITIAL_LORA_FREQUENCY);
        radio_call_status &= radio.setBandwidth(INITIAL_LORA_BANDWIDTH);
        radio_call_status &= radio.setSpreadingFactor(LORA_SPREADING_FACTOR);
        radio_call_status &= radio.setCodingRate(LORA_CODING_RATE);
        radio_call_status &= radio.setSyncWord(SYNC_WORD);
        radio_call_status &= radio.setCRC(true);
    }

    if (radio_call_status != RADIOLIB_ERR_NONE) {
        Serial.print("FAILED, code ");
        Serial.println(radio_call_status);

        while (true) {
            delay(1000);
        }
    }

    radio.setDio1Action(radioReceiveISR);

    // --------------------------------------------------------
    // Hop Hardware Timer and Logging Setup
    // --------------------------------------------------------

    // Start logging timer, start hop interrupt handler
    logging_timer.start();
    hw_interrupt_timer.attach(&hopTickOccurred, HOP_PERIOD_MS);

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
    // --------------------------------------------------------
    // Handle PPS Received
    // --------------------------------------------------------

    if (FREQUENCY_HOPPING_ENABLED && ppsReceived) {
        // Reset PPS flag
        noInterrupts();
        ppsReceived = false;
        interrupts();

        // Clear hop counts, pause radio, update freq, start receive
        radio.standby();
        hopCount = 0;
        radio.setFrequency(HOP_TABLE[hopCount]);
        radio.startReceive();

        hw_interrupt_timer.detach();
        hw_interrupt_timer.attach(&hopTickOccurred, HOP_PERIOD_MS);
    }

    // --------------------------------------------------------
    // Handle Hop Pending
    // --------------------------------------------------------

    if (FREQUENCY_HOPPING_ENABLED && hopPending) {
        // Reset Hop flag
        noInterrupts();
        hopPending = false;
        interrupts();

        hopCount++;

        // Determine new frequency
        size_t next_frequency_index = (hopCount - 1) % HOP_TABLE_SIZE;
        float next_frequency = HOP_TABLE[next_frequency_index];

        // Stop current RX operation
        radio.standby();

        // Change frequency
        int radio_call_status = radio.setFrequency(next_frequency);
        last_hop_time_micros = micros();

        // Resume asynchronous RX on the new frequency
        radio.startReceive();

        if (radio_call_status != RADIOLIB_ERR_NONE) {
            Serial.print("Frequency change failed: ");
            Serial.println(radio_call_status);
        }
    }

    // Read in characters and store in TxRing
    if (Serial.available()) {
        while (Serial.available()) {
            // Add to current tail position
            txRing[txTail] = Serial.read();

            // Update tail position and number of bytes in buffer
            txTail = (txTail + 1) % RING_SIZE;
            txCount++;
        }
    }

    // Rx Radio packet in, Tx to Serial
    if (radioPacketReceived) {
        radioPacketReceived = false;

        // Get the actual number of bytes received
        size_t packetLength = radio.getPacketLength();

        // Read data
        int radio_call_status = radio.readData(
            rxRing,
            packetLength
        );

        if (radio_call_status == RADIOLIB_ERR_NONE) {
            // Senfd received bytes over sereal
            Serial.write(rxRing, packetLength);

            // Wait until data sent
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

    // Determine number of bytes to transmit, if we are at the end of buffer
    // transmit the difference from the head to the end, even if its below the chunk size.
    // This safegaurd allows us to avoid a loop each time we want to transmit something
    auto bytes_to_transmit = min(
        txCount,
        min(
            CHUNK_SIZE,
            RING_SIZE - txHead
        )
    );

    // Transmit if valid
    if (bytes_to_transmit > 0) {
        if (FREQUENCY_HOPPING_ENABLED) {
            // Get time on air
            RadioLibTime_t time_on_air_micros = radio.getTimeOnAir(bytes_to_transmit);

            // Determine predicted time for the transmit and radio state transitions
            auto predicted_time_micros = time_on_air_micros + DEFAULT_RADIOLIB_TRANSMIT_CYCLE_TIME_MICROS + (DEFAULT_RADIOLIB_PER_CHARACTER_TIME_MICROS * bytes_to_transmit);

            // Calculate upper/lower bounds determining if current time is valid to transmit
            auto lower_bound_micros = (last_hop_time_micros + HOP_GRACE_PERIOD_MICROS);
            auto upper_bound_micros = (last_hop_time_micros + (HOP_PERIOD_MS.count() * 1000UL) - HOP_GRACE_PERIOD_MICROS);
            
            // Estimate transmit done time
            auto transmit_done_time_micros = micros() + predicted_time_micros;

            // Skip transmit if we dont meet this threshold
            if (!((lower_bound_micros <= transmit_done_time_micros) && (transmit_done_time_micros <= upper_bound_micros))) {
                return;
            }
        }
        
        // Start Transmit logic block
        radio.standby();
        int transmit_status = radio.transmit(
            &txRing[txHead],
            bytes_to_transmit
        );
        radio.startReceive();
        // End Transmit logic block

        // Update flags if valid transmit
        if (transmit_status == RADIOLIB_ERR_NONE) {
            txCount -= bytes_to_transmit;
            txHead = (txHead + bytes_to_transmit) % RING_SIZE;
        }
    }
}