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

#include <datalink_receive.h>
#include <device_config.h>
#include <hop_table.h>
#include <lora_config.h>

using namespace mbed;

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

    Serial.println("=== LORA TRANSMITTER ===");

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

    Serial.println("Initializing SX1262 Radio");

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
    radio_call_status = radio.setSyncWord(0x12);
    radio_call_status = radio.setCRC(true);

    // --------------------------------------------------------
    // Hop Hardware Timer and Logging Setup
    // --------------------------------------------------------

    // Start logging timer, start hop interrupt handler
    logging_timer.start();
    hw_interrupt_timer.attach(&hopTickOccurred, HOP_PERIOD_MS);

    // --------------------------------------------------------
    // Setup Done
    // --------------------------------------------------------

    Serial.println("Done initializing");
}

// ============================================================
// Main loop
// ============================================================

void loop()
{
    // --------------------------------------------------------
    // Handle PPS Received
    // --------------------------------------------------------

    if (ppsReceived) {
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

        Serial.println();
        Serial.println("PPS -> hop count RESET");
    }

    // --------------------------------------------------------
    // Handle Hop Pending
    // --------------------------------------------------------

    if (hopPending) {
        // Reset PPS flag
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

        if (radio_call_status != RADIOLIB_ERR_NONE) {
            Serial.print("Frequency change failed: ");
            Serial.println(radio_call_status);
        }

        // Resume asynchronous RX on the new frequency
        radio.startReceive();

        // Logging
        auto elapsed = logging_timer.elapsed_time();
        Serial.print("Timer: ");
        Serial.print(elapsed.count());
        Serial.print(" us, hop: ");
        Serial.println(hopCount);
    }

    // --------------------------------------------------------
    // TX Data received from Serial Driver
    // Serial Data -> Giga -> SX1262 -> )))))
    // --------------------------------------------------------
    
    if (Serial.available()) {

        // Demo uses single input lines
        String message = Serial.readStringUntil('\n');
        message.trim();

        if (message.length() == 0) {
            return;
        }
        
        // Logging
        Serial.print("TX: ");
        Serial.print(message);
        Serial.print(" at hop: ");
        Serial.println(hopCount);

        // Tx and validate
        int radio_call_status = radio.transmit(message);
        if (radio_call_status == RADIOLIB_ERR_NONE) {
            Serial.println("TX OK");
        } else {
            Serial.print("TX FAILED: ");
            Serial.println(radio_call_status);
        }
    }
}