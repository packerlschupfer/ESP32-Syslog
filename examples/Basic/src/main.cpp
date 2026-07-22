/**
 * @file main.cpp
 * @brief Basic ESP32-Syslog usage example
 *
 * Demonstrates standalone syslog client without ESP32-Logger integration.
 * Sends log messages directly to a syslog server over UDP.
 *
 * Configure your syslog server IP address before uploading.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Syslog.h>

// WiFi credentials
const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASSWORD = "your-password";

// Syslog server configuration
const IPAddress SYSLOG_SERVER(192, 168, 1, 100);
const uint16_t SYSLOG_PORT = 514;

// Create syslog client
Syslog syslog("esp32-basic", "demo");

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nESP32-Syslog Basic Example");
    Serial.println("===========================\n");

    // Connect to WiFi
    Serial.printf("Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println(" Connected!");
    Serial.printf("IP address: %s\n\n", WiFi.localIP().toString().c_str());

    // Initialize syslog client
    if (syslog.begin(SYSLOG_SERVER, SYSLOG_PORT, Syslog::LOCAL0)) {
        Serial.printf("Syslog initialized: %s:%d\n",
                     SYSLOG_SERVER.toString().c_str(), SYSLOG_PORT);

        // Set minimum level (only WARN and ERROR will be sent)
        syslog.setMinLevel(ESP_LOG_WARN);
        Serial.println("Min level set to WARN\n");
    } else {
        Serial.println("ERROR: Failed to initialize syslog!");
    }

    // Send some test messages
    Serial.println("Sending test messages...\n");

    // This will be sent (ERROR >= WARN)
    if (syslog.send(ESP_LOG_ERROR, "Test", "This is an error message")) {
        Serial.println("Sent: ERROR message");
    }

    // This will be sent (WARN >= WARN)
    if (syslog.send(ESP_LOG_WARN, "Test", "This is a warning message")) {
        Serial.println("Sent: WARN message");
    }

    // This will be filtered (INFO < WARN)
    if (syslog.send(ESP_LOG_INFO, "Test", "This info message is filtered")) {
        Serial.println("Filtered: INFO message (not sent to server)");
    }

    // Print statistics
    uint32_t sent, failed;
    syslog.getStats(sent, failed);
    Serial.printf("\nStatistics: %lu sent, %lu failed\n", sent, failed);
}

void loop() {
    static uint32_t lastSend = 0;
    static uint32_t counter = 0;

    // Send a heartbeat error every 10 seconds
    if (millis() - lastSend >= 10000) {
        lastSend = millis();
        counter++;

        char message[64];
        snprintf(message, sizeof(message), "Heartbeat #%lu - uptime: %lu sec",
                counter, millis() / 1000);

        // Send as warning level
        syslog.send(ESP_LOG_WARN, "Heartbeat", message);

        // Print stats
        uint32_t sent, failed;
        syslog.getStats(sent, failed);
        Serial.printf("[%lu] Sent heartbeat - total: %lu sent, %lu failed\n",
                     millis() / 1000, sent, failed);
    }
}
