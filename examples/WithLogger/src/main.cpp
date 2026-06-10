/**
 * @file main.cpp
 * @brief ESP32-Syslog integration with ESP32-Logger
 *
 * Demonstrates how to forward log messages from ESP32-Logger to a
 * remote syslog server using the Logger's subscriber callback mechanism.
 *
 * All logging goes through Logger, which automatically forwards to:
 * - Serial console (always)
 * - Syslog server (WARN and ERROR only)
 *
 * This pattern provides centralized logging for multi-device systems.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Logger.h>
#include <Syslog.h>

// WiFi credentials
const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASSWORD = "your-password";

// Syslog server configuration
const IPAddress SYSLOG_SERVER(192, 168, 1, 100);
const uint16_t SYSLOG_PORT = 514;

// Create instances
Logger& logger = Logger::getInstance();
Syslog syslog("esp32-logger", "demo");

/**
 * @brief Callback function to forward logs to syslog
 *
 * This callback is registered with Logger and receives all log messages.
 * It forwards messages to the syslog server based on severity.
 *
 * @note Keep callbacks fast! Heavy processing blocks the logging task.
 */
void syslogCallback(esp_log_level_t level, const char* tag, const char* message) {
    // Forward to syslog (level filtering is done in Syslog::send)
    syslog.send(level, tag, message);
}

void setup() {
    // Initialize Logger
    logger.init();
    logger.setLogLevel(ESP_LOG_DEBUG);

    logger.log(ESP_LOG_INFO, "Setup", "ESP32-Syslog + Logger Example");
    logger.log(ESP_LOG_INFO, "Setup", "============================");

    // Connect to WiFi
    logger.log(ESP_LOG_INFO, "WiFi", "Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    logger.log(ESP_LOG_INFO, "WiFi", "Connected! IP: %s",
              WiFi.localIP().toString().c_str());

    // Initialize syslog client
    if (syslog.begin(SYSLOG_SERVER, SYSLOG_PORT, Syslog::LOCAL0)) {
        // Only forward WARN and ERROR to reduce network traffic
        syslog.setMinLevel(ESP_LOG_WARN);

        // Register callback with Logger
        if (logger.addLogSubscriber(syslogCallback)) {
            logger.log(ESP_LOG_INFO, "Setup",
                      "Syslog subscriber registered - forwarding WARN/ERROR");
        } else {
            logger.log(ESP_LOG_ERROR, "Setup",
                      "Failed to register syslog subscriber!");
        }
    } else {
        logger.log(ESP_LOG_ERROR, "Setup",
                  "Failed to initialize syslog client!");
    }

    // Test logging at different levels
    logger.log(ESP_LOG_INFO, "Setup", "");
    logger.log(ESP_LOG_INFO, "Setup", "Testing log levels:");

    // DEBUG - serial only (filtered by syslog)
    logger.log(ESP_LOG_DEBUG, "Test", "DEBUG: This goes to serial only");

    // INFO - serial only (filtered by syslog)
    logger.log(ESP_LOG_INFO, "Test", "INFO: This also goes to serial only");

    // WARN - serial AND syslog
    logger.log(ESP_LOG_WARN, "Test", "WARN: This goes to serial AND syslog!");

    // ERROR - serial AND syslog
    logger.log(ESP_LOG_ERROR, "Test", "ERROR: This goes to serial AND syslog!");

    // Print syslog statistics
    uint32_t sent, failed;
    syslog.getStats(sent, failed);
    logger.log(ESP_LOG_INFO, "Setup", "Syslog stats: %lu sent, %lu failed",
              sent, failed);

    logger.log(ESP_LOG_INFO, "Setup", "");
    logger.log(ESP_LOG_INFO, "Setup", "Setup complete! Running...");
}

void loop() {
    static uint32_t lastHeartbeat = 0;
    static uint32_t loopCounter = 0;

    loopCounter++;

    // Every 30 seconds: send heartbeat
    if (millis() - lastHeartbeat >= 30000) {
        lastHeartbeat = millis();

        // These go to serial only (INFO level)
        logger.log(ESP_LOG_INFO, "Status", "Uptime: %lu sec, loops: %lu",
                  millis() / 1000, loopCounter);

        // Simulate occasional warnings (go to syslog)
        if ((millis() / 1000) % 60 == 0) {
            logger.log(ESP_LOG_WARN, "Monitor", "Periodic check - all systems OK");
        }

        // Print syslog statistics
        uint32_t sent, failed;
        syslog.getStats(sent, failed);
        logger.log(ESP_LOG_DEBUG, "Syslog", "Messages: %lu sent, %lu failed",
                  sent, failed);
    }

    delay(10);
}
