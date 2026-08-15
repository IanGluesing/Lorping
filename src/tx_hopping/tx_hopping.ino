#include <Arduino.h>
#include <RadioLib.h>
#include <mbed.h>
#include <chrono>

#include <hop_table.h>
#include <lora_config.h>

using namespace mbed;
using namespace std::chrono_literals;

// ============================================================
// Configuration
// ============================================================

// Baud Rate used for Serial connection
constexpr uint32_t SERIAL_BAUD_RATE = 115200; 

// User defined hop period Hz = 1 / Period => 10Hz Hop Rate
constexpr std::chrono::milliseconds HOP_PERIOD_MS = 100ms;

// Arduino Giga R1 Wifi and Waveshare SX1262 Pinouts can be found here:
// Arduino Pinout: https://content.arduino.cc/assets/ABX00063-full-pinout.pdf
// SX1262 Pinout: https://www.waveshare.com/core1262-868m.htm 

// Arduino Pin handling PPS input from GPS Source or Pseudo PPS Provider: D25
constexpr int PPS_PIN = 25;

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
// State
// ============================================================

// Flag denoting a PPS Signal was received
volatile bool ppsReceived = false;

// Flag denoting a hop needs to occur
volatile bool hopPending = false;

// Current hop count
volatile uint32_t hopCount = 0;

// ============================================================
// Interrupts
// ============================================================

// PPS Signal from GPS/Jetson
void ppsISR()
{
    ppsReceived = true;
}

// Hardware timer callback signalling a hop needs to occur
void hopTickOccurred()
{
    hopPending = true;
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

    Serial.println("=== LORA 10 Hz HOP RECEIVER ===");

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
    radio_call_status = radio.setFrequency(868.0);
    radio_call_status = radio.setBandwidth(125.0);
    radio_call_status = radio.setSpreadingFactor(7);
    radio_call_status = radio.setCodingRate(5);
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