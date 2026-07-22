# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-Syslog is an RFC 3164 compliant UDP syslog client library for ESP32 microcontrollers. It sends log messages to remote syslog servers and integrates with the ESP32-Logger library via callback mechanism.

## Build Commands

Build examples:
```bash
cd examples/Basic && pio run
cd examples/WithLogger && pio run
```

Upload to device:
```bash
pio run -t upload
```

Monitor serial output:
```bash
pio device monitor -b 115200
```

## Architecture

This is a single-class PlatformIO library:

- **`src/Syslog.h`** - Public API: `Syslog` class with `begin()`, `send()`, level filtering, facility codes
- **`src/Syslog.cpp`** - Implementation: RFC 3164 message formatting, UDP transport, FreeRTOS mutex protection

The library supports two transports via compile-time define:
- WiFiUDP (default)
- EthernetUDP (define `SYSLOG_USE_ETHERNET`)

## Integration Pattern

Standalone usage calls `syslog.send()` directly. For ESP32-Logger integration, register a callback with `logger.addLogSubscriber()` that forwards to `syslog.send()` - see `examples/WithLogger/src/main.cpp:40`.

## Dependencies

- ESP32-Logger (optional) - https://github.com/packerlschupfer/ESP32-Logger.git
