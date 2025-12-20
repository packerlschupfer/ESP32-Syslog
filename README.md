# ESP32-Syslog

RFC 3164 UDP syslog client for ESP32 with ESP32-Logger integration.

## Features

- **RFC 3164 compliant** - BSD syslog format, compatible with rsyslog, syslog-ng, Graylog
- **UDP transport** - Lightweight, fire-and-forget
- **ESP32-Logger integration** - Forward logs via callback mechanism
- **Thread-safe** - FreeRTOS mutex protection for multi-task environments
- **Configurable filtering** - Minimum log level, facility selection
- **Low overhead** - ~200 bytes static, ~1ms per log message

## Installation

### PlatformIO (recommended)

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/packerlschupfer/ESP32-Syslog.git
```

## Quick Start

### Standalone Usage

```cpp
#include <Syslog.h>

Syslog syslog("esp32-device", "myapp");

void setup() {
    // Initialize with server IP, port, and facility
    syslog.begin(IPAddress(192, 168, 1, 100), 514, Syslog::LOCAL0);

    // Set minimum level (only WARN and ERROR sent)
    syslog.setMinLevel(ESP_LOG_WARN);

    // Send log messages
    syslog.send(ESP_LOG_ERROR, "Safety", "Pressure too high!");
    syslog.send(ESP_LOG_WARN, "Temp", "Temperature rising");
}
```

### With ESP32-Logger

```cpp
#include <Logger.h>
#include <Syslog.h>

Logger& logger = Logger::getInstance();
Syslog syslog("esp32-boiler", "boiler");

void syslogCallback(esp_log_level_t level, const char* tag, const char* message) {
    syslog.send(level, tag, message);
}

void setup() {
    logger.begin(115200);

    syslog.begin(IPAddress(192, 168, 1, 100), 514, Syslog::LOCAL0);
    syslog.setMinLevel(ESP_LOG_WARN);

    // Register callback - all logs now forward to syslog
    logger.addLogSubscriber(syslogCallback);

    // This goes to serial AND syslog
    logger.log(ESP_LOG_ERROR, "Safety", "Critical error!");

    // This goes to serial only (filtered by min level)
    logger.log(ESP_LOG_INFO, "Status", "System running");
}
```

## API Reference

### Constructor

```cpp
Syslog(const char* hostname = "esp32", const char* appName = "app");
```

### Methods

| Method | Description |
|--------|-------------|
| `begin(server, port, facility)` | Initialize UDP socket |
| `end()` | Close UDP socket |
| `send(level, tag, message)` | Send syslog message (respects minLevel filter) |
| `sendUnfiltered(level, tag, message)` | Send bypassing minLevel filter |
| `setMinLevel(level)` | Set minimum log level to forward |
| `getMinLevel()` | Get current minimum level |
| `setFacility(facility)` | Set syslog facility |
| `getFacility()` | Get current facility |
| `setHostname(name)` | Set device hostname |
| `setAppName(name)` | Set application name |
| `getStats(sent, failed)` | Get send statistics |
| `resetStats()` | Reset statistics counters |
| `isInitialized()` | Check if initialized |

### Facility Codes

```cpp
Syslog::KERN      // 0  - Kernel messages
Syslog::USER      // 1  - User-level (default)
Syslog::DAEMON    // 3  - System daemons
Syslog::AUTH      // 4  - Security
Syslog::LOCAL0    // 16 - Local use 0
Syslog::LOCAL1    // 17 - Local use 1
// ... LOCAL2-LOCAL7 (18-23)
```

### Log Level Mapping

| ESP-IDF Level | Syslog Severity |
|---------------|-----------------|
| ESP_LOG_ERROR | 3 (Error) |
| ESP_LOG_WARN | 4 (Warning) |
| ESP_LOG_INFO | 6 (Informational) |
| ESP_LOG_DEBUG | 7 (Debug) |
| ESP_LOG_VERBOSE | 7 (Debug) |

## RFC 3164 Message Format

```
<PRI>TIMESTAMP HOSTNAME APP[TAG]: MESSAGE

Example:
<131>Dec 21 12:34:56 esp32-boiler boiler[Safety]: Pressure too high!

PRI = (Facility × 8) + Severity
  Facility: LOCAL0 = 16
  Severity: ERROR = 3
  PRI = (16 × 8) + 3 = 131
```

## Testing with rsyslog

Add to `/etc/rsyslog.conf`:

```conf
# Enable UDP syslog
module(load="imudp")
input(type="imudp" port="514")

# Log ESP32 messages to separate file
if $hostname contains 'esp32' then /var/log/esp32.log
& stop
```

Restart rsyslog: `sudo systemctl restart rsyslog`

Monitor logs: `tail -f /var/log/esp32.log`

## Performance

- **Memory**: ~200 bytes static, 1KB stack buffer per message
- **Latency**: ~1-2ms per message (UDP send)
- **Throughput**: Up to ~1000 messages/minute
- **Recommended**: Filter to WARN/ERROR only (reduce traffic)

## Thread Safety

The library uses FreeRTOS mutex to protect the UDP socket. Safe to call `send()` from multiple tasks simultaneously. Mutex timeout is 100ms.

## Dependencies

- [ESP32-Logger](https://github.com/packerlschupfer/ESP32-Logger.git) (optional, for callback integration)

## License

GPL-3.0

## Author

packerlschupfer
