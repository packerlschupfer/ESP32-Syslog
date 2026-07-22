/*
 * Syslog.h - part of the ESP32-Syslog library
 *
 * Copyright (C) 2025-2026 packerlschupfer
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * @file Syslog.h
 * @brief RFC 3164 UDP syslog client for ESP32
 * @author packerlschupfer
 * @license GPL-3.0
 *
 * Lightweight syslog client that sends log messages to remote syslog servers
 * over UDP. Integrates with ESP32-Logger via callback mechanism.
 *
 * Features:
 * - RFC 3164 (BSD syslog) compliant message format
 * - UDP transport (fire-and-forget, low overhead)
 * - Thread-safe with FreeRTOS mutex protection
 * - Configurable log level filtering
 * - Statistics tracking (sent/failed counts)
 * - Supports both WiFi and Ethernet (define SYSLOG_USE_ETHERNET)
 */

#pragma once

#include <Arduino.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Support both WiFi and Ethernet UDP
#if defined(SYSLOG_USE_ETHERNET)
#include <EthernetUdp.h>
typedef EthernetUDP SyslogUDP;
#else
#include <WiFiUdp.h>
typedef WiFiUDP SyslogUDP;
#endif

/**
 * @brief RFC 3164 syslog client for ESP32
 *
 * Sends log messages to remote syslog server over UDP.
 * Integrates with ESP32-Logger via callback mechanism.
 *
 * @example
 * Syslog syslog("esp32-device", "myapp");
 * syslog.begin(IPAddress(192, 168, 1, 100), 514, Syslog::LOCAL0);
 * syslog.send(ESP_LOG_ERROR, "Safety", "Pressure too high!");
 */
class Syslog {
public:
    /**
     * @brief Syslog facility codes (RFC 3164)
     */
    enum Facility {
        KERN     = 0,   ///< Kernel messages
        USER     = 1,   ///< User-level messages (default)
        MAIL     = 2,   ///< Mail system
        DAEMON   = 3,   ///< System daemons
        AUTH     = 4,   ///< Security/authentication
        SYSLOG   = 5,   ///< Syslog internal
        LPR      = 6,   ///< Line printer subsystem
        NEWS     = 7,   ///< Network news
        UUCP     = 8,   ///< UUCP subsystem
        CRON     = 9,   ///< Clock daemon
        AUTHPRIV = 10,  ///< Security/authentication (private)
        FTP      = 11,  ///< FTP daemon
        LOCAL0   = 16,  ///< Local use 0
        LOCAL1   = 17,  ///< Local use 1
        LOCAL2   = 18,  ///< Local use 2
        LOCAL3   = 19,  ///< Local use 3
        LOCAL4   = 20,  ///< Local use 4
        LOCAL5   = 21,  ///< Local use 5
        LOCAL6   = 22,  ///< Local use 6
        LOCAL7   = 23   ///< Local use 7
    };

    /**
     * @brief Construct syslog client
     * @param hostname Device hostname (max 63 chars, default: "esp32")
     * @param appName Application name (max 47 chars, default: "app")
     */
    Syslog(const char* hostname = "esp32", const char* appName = "app");

    /**
     * @brief Destructor - cleans up resources
     */
    ~Syslog();

    /**
     * @brief Initialize syslog client and open UDP socket
     * @param server Syslog server IP address
     * @param port Syslog server port (default: 514)
     * @param facility Syslog facility code (default: USER)
     * @return true if initialized successfully
     */
    bool begin(IPAddress server, uint16_t port = 514, Facility facility = USER);

    /**
     * @brief Stop syslog client and close socket
     */
    void end();

    /**
     * @brief Send syslog message
     * @param level ESP32 log level (ESP_LOG_ERROR, ESP_LOG_WARN, etc.)
     * @param tag Log tag/component name
     * @param message Log message
     * @return true if sent successfully
     *
     * Thread-safe: Can be called from multiple FreeRTOS tasks.
     */
    bool send(esp_log_level_t level, const char* tag, const char* message);

    /**
     * @brief Send message bypassing minLevel filter
     *
     * Use for critical announcements that should always reach the syslog server
     * regardless of configured minimum level (e.g., boot complete, shutdown).
     *
     * @param level ESP log level (maps to syslog severity)
     * @param tag Log tag/component name
     * @param message Message to send
     * @return true if sent successfully
     */
    bool sendUnfiltered(esp_log_level_t level, const char* tag, const char* message);

    /**
     * @brief Set minimum log level to send
     * @param minLevel Only send messages at or above this level
     *
     * Example: setMinLevel(ESP_LOG_WARN) only sends WARN and ERROR
     * Note: Lower numeric value = higher severity in ESP-IDF
     */
    void setMinLevel(esp_log_level_t minLevel);

    /**
     * @brief Get minimum log level
     */
    esp_log_level_t getMinLevel() const;

    /**
     * @brief Set syslog facility
     * @param facility New facility code
     */
    void setFacility(Facility facility);

    /**
     * @brief Get current facility
     */
    Facility getFacility() const;

    /**
     * @brief Set device hostname
     * @param hostname New hostname (max 63 chars)
     */
    void setHostname(const char* hostname);

    /**
     * @brief Set application name
     * @param appName New app name (max 47 chars)
     */
    void setAppName(const char* appName);

    /**
     * @brief Get send statistics
     * @param sent Number of messages sent successfully
     * @param failed Number of failed sends
     */
    void getStats(uint32_t& sent, uint32_t& failed) const;

    /**
     * @brief Reset statistics counters
     */
    void resetStats();

    /**
     * @brief Check if syslog is initialized
     */
    bool isInitialized() const;

private:
    // Prevent copying
    Syslog(const Syslog&) = delete;
    Syslog& operator=(const Syslog&) = delete;

    // Configuration
    IPAddress server_;
    uint16_t port_;
    Facility facility_;
    esp_log_level_t minLevel_;
    char hostname_[64];
    char appName_[48];
    bool initialized_;

    // Transport (lazy initialization to avoid static init issues with EthernetUDP)
    SyslogUDP* udp_;

    // Thread safety
    SemaphoreHandle_t mutex_;

    // Statistics
    uint32_t sentCount_;
    uint32_t failedCount_;

    /**
     * @brief Internal send implementation
     * @param level ESP log level
     * @param tag Log tag
     * @param message Log message
     * @return true if sent successfully
     */
    bool sendInternal(esp_log_level_t level, const char* tag, const char* message);

    /**
     * @brief Map ESP log level to syslog severity
     * @param level ESP-IDF log level
     * @return RFC 3164 severity (0=Emergency ... 7=Debug)
     */
    int getSeverity(esp_log_level_t level) const;

    /**
     * @brief Calculate syslog priority (<PRI> field)
     * @param level ESP-IDF log level
     * @return Priority = Facility * 8 + Severity
     */
    int getPriority(esp_log_level_t level) const;

    /**
     * @brief Get RFC 3164 timestamp string
     * @param buffer Output buffer (minimum 16 bytes)
     * @param bufferSize Size of output buffer
     * @return Formatted timestamp (e.g., "Jan  1 12:34:56")
     */
    const char* getTimestamp(char* buffer, size_t bufferSize) const;

    /**
     * @brief Format complete syslog message per RFC 3164
     * @param buffer Output buffer
     * @param bufferSize Size of output buffer
     * @param level Log level
     * @param tag Component tag
     * @param message Log message
     * @return Length of formatted message, 0 on error
     */
    size_t formatMessage(char* buffer, size_t bufferSize,
                        esp_log_level_t level, const char* tag,
                        const char* message) const;
};
