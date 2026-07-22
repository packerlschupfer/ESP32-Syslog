/*
 * Syslog.cpp - part of the ESP32-Syslog library
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
 * @file Syslog.cpp
 * @brief RFC 3164 UDP syslog client implementation
 * @author packerlschupfer
 * @license GPL-3.0
 */

#include "Syslog.h"
#include <time.h>

// RFC 3164 max message size
static constexpr size_t SYSLOG_MAX_MESSAGE_SIZE = 1024;

// Mutex timeout for thread safety
static constexpr TickType_t MUTEX_TIMEOUT_MS = 100;

Syslog::Syslog(const char* hostname, const char* appName)
    : port_(514)
    , facility_(USER)
    , minLevel_(ESP_LOG_VERBOSE)
    , initialized_(false)
    , udp_(nullptr)
    , mutex_(nullptr)
    , sentCount_(0)
    , failedCount_(0)
{
    // Create mutex for thread safety
    mutex_ = xSemaphoreCreateMutex();

    setHostname(hostname);
    setAppName(appName);
}

Syslog::~Syslog() {
    end();

    if (mutex_) {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
}

bool Syslog::begin(IPAddress server, uint16_t port, Facility facility) {
    if (!mutex_) {
        return false;
    }

    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return false;
    }

    // Clean up existing state
    if (initialized_) {
        if (udp_) {
            udp_->stop();
        }
        initialized_ = false;
    }

    server_ = server;
    port_ = port;
    facility_ = facility;

    // Lazy initialization of UDP object (avoids static init issues with EthernetUDP)
    if (!udp_) {
        udp_ = new SyslogUDP();
        if (!udp_) {
            xSemaphoreGive(mutex_);
            return false;
        }
    }

    // Start UDP on random local port
    if (!udp_->begin(0)) {
        xSemaphoreGive(mutex_);
        return false;
    }

    initialized_ = true;
    xSemaphoreGive(mutex_);
    return true;
}

void Syslog::end() {
    if (!mutex_) {
        return;
    }

    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return;
    }

    if (initialized_) {
        if (udp_) {
            udp_->stop();
            delete udp_;
            udp_ = nullptr;
        }
        initialized_ = false;
    }

    xSemaphoreGive(mutex_);
}

bool Syslog::send(esp_log_level_t level, const char* tag, const char* message) {
    // Check minimum level filter (lower value = higher severity)
    if (level > minLevel_) {
        return true;  // Filtered out, not an error
    }
    return sendInternal(level, tag, message);
}

bool Syslog::sendUnfiltered(esp_log_level_t level, const char* tag, const char* message) {
    return sendInternal(level, tag, message);
}

bool Syslog::sendInternal(esp_log_level_t level, const char* tag, const char* message) {
    if (!mutex_) {
        return false;
    }

    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        failedCount_++;
        return false;
    }

    if (!initialized_ || !udp_) {
        failedCount_++;
        xSemaphoreGive(mutex_);
        return false;
    }

    // Format syslog message
    char buffer[SYSLOG_MAX_MESSAGE_SIZE];
    size_t msgLen = formatMessage(buffer, sizeof(buffer), level, tag, message);

    if (msgLen == 0) {
        failedCount_++;
        xSemaphoreGive(mutex_);
        return false;
    }

    // Send via UDP
    if (!udp_->beginPacket(server_, port_)) {
        failedCount_++;
        xSemaphoreGive(mutex_);
        return false;
    }

    size_t written = udp_->write((const uint8_t*)buffer, msgLen);

    if (!udp_->endPacket() || written != msgLen) {
        failedCount_++;
        xSemaphoreGive(mutex_);
        return false;
    }

    sentCount_++;
    xSemaphoreGive(mutex_);
    return true;
}

void Syslog::setMinLevel(esp_log_level_t minLevel) {
    minLevel_ = minLevel;
}

esp_log_level_t Syslog::getMinLevel() const {
    return minLevel_;
}

void Syslog::setFacility(Facility facility) {
    facility_ = facility;
}

Syslog::Facility Syslog::getFacility() const {
    return facility_;
}

void Syslog::setHostname(const char* hostname) {
    if (hostname) {
        strncpy(hostname_, hostname, sizeof(hostname_) - 1);
        hostname_[sizeof(hostname_) - 1] = '\0';
    }
}

void Syslog::setAppName(const char* appName) {
    if (appName) {
        strncpy(appName_, appName, sizeof(appName_) - 1);
        appName_[sizeof(appName_) - 1] = '\0';
    }
}

void Syslog::getStats(uint32_t& sent, uint32_t& failed) const {
    sent = sentCount_;
    failed = failedCount_;
}

void Syslog::resetStats() {
    sentCount_ = 0;
    failedCount_ = 0;
}

bool Syslog::isInitialized() const {
    return initialized_;
}

int Syslog::getSeverity(esp_log_level_t level) const {
    // Map ESP log levels to syslog severity (0=Emergency ... 7=Debug)
    // RFC 3164 severity codes:
    //   0 = Emergency (system unusable)
    //   1 = Alert (action must be taken)
    //   2 = Critical (critical conditions)
    //   3 = Error (error conditions)
    //   4 = Warning (warning conditions)
    //   5 = Notice (normal but significant)
    //   6 = Informational (informational messages)
    //   7 = Debug (debug-level messages)
    switch (level) {
        case ESP_LOG_NONE:    return 0;  // Emergency
        case ESP_LOG_ERROR:   return 3;  // Error
        case ESP_LOG_WARN:    return 4;  // Warning
        case ESP_LOG_INFO:    return 6;  // Informational
        case ESP_LOG_DEBUG:   return 7;  // Debug
        case ESP_LOG_VERBOSE: return 7;  // Debug
        default:              return 6;  // Informational (fallback)
    }
}

int Syslog::getPriority(esp_log_level_t level) const {
    // Priority = Facility * 8 + Severity
    return (facility_ * 8) + getSeverity(level);
}

const char* Syslog::getTimestamp(char* buffer, size_t bufferSize) const {
    // RFC 3164 timestamp format: "Mmm dd hh:mm:ss"
    // Example: "Jan  1 12:34:56" or "Dec 25 08:15:30"
    // Note: Day is space-padded to 2 chars (not zero-padded)

    if (bufferSize < 16) {
        if (bufferSize > 0) {
            buffer[0] = '\0';
        }
        return buffer;
    }

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // %b = abbreviated month, %e = day (space-padded), %T = HH:MM:SS
    strftime(buffer, bufferSize, "%b %e %H:%M:%S", &timeinfo);
    return buffer;
}

size_t Syslog::formatMessage(char* buffer, size_t bufferSize,
                             esp_log_level_t level, const char* tag,
                             const char* message) const {
    if (!buffer || bufferSize < 64) {
        return 0;
    }

    // RFC 3164 format: <PRI>TIMESTAMP HOSTNAME APP[TAG]: MESSAGE
    // Example: <131>Dec 21 12:34:56 esp32-boiler boiler[Safety]: Pressure high!
    //
    // PRI calculation: (Facility * 8) + Severity
    //   LOCAL0 (16) + ERROR (3) = 131

    char timestamp[32];
    getTimestamp(timestamp, sizeof(timestamp));

    int priority = getPriority(level);

    // Format message
    int written = snprintf(buffer, bufferSize,
                          "<%d>%s %s %s[%s]: %s",
                          priority,
                          timestamp,
                          hostname_,
                          appName_,
                          tag ? tag : "unknown",
                          message ? message : "");

    if (written < 0 || written >= (int)bufferSize) {
        return 0;  // Buffer overflow or error
    }

    return (size_t)written;
}
