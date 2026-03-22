# 🐾 Automatic Pet Feeder

An embedded IoT device for automatic pet feeding, built on **ESP32-S3** and **Zephyr RTOS**. It allows users to configure rule-based schedules (hour, minute, weekday, month day, month) that trigger a stepper motor to dispense food — all remotely configurable and monitorable via Wi-Fi and MQTT.

NOTE: This is a work in progress, so there wil be some TODOs in the code base/docs

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware](#hardware)
- [Software Architecture](#software-architecture)
- [Project Structure](#project-structure)
- [MQTT & Cloud Integration](#mqtt--cloud-integration)
- [Scheduler Rules](#scheduler-rules)

---

## Overview

The Automatic Pet Feeder is a connected embedded system designed to feed pets on a flexible, rule-based schedule without manual intervention. Users interact with it through a MQTT app — setting up feeding rules, monitoring food levels, and checking device status in real time.

---

## Features

- **Rule-based scheduler** — configure feeds by hour, minute, weekday, day of month, and month
- **Stepper motor control** — Dispensing actuation
- **Food level monitoring** — VL53L0X ToF (Time-of-Flight) I2C sensor measures remaining food volume
- **Persistent timekeeping** — DS3231 RTC over I2C with NTP synchronization (SNTP) for drift correction
- **Wi-Fi connectivity** — STA mode with DHCP, supports AP+STA for provisioning
- **MQTT integration** — publishes sensor data and device status; subscribes to commands
- **TLS security** — encrypted MQTT with mbedTLS (ECDHE-RSA-AES128-GCM-SHA256)
- **HTTP server** — serves a credential provisioning page for Wi-Fi setup (AP mode)
- **Flash storage** — persistent rule and configuration storage via Zephyr ZMS (Flash Map)
- **Watchdog** — task-level watchdog for system health monitoring
- **RGB LED** — visual status indicator via RGB LED

---

## Hardware

| Component | Description |
|---|---|
| **MCU** | ESP32-S3 |
| **RTOS** | Zephyr RTOS |
| **Actuator** | Stepper motor (food dispenser) |
| **Sensor** | VL53L0X — I2C Time-of-Flight (food volume) |
| **RTC** | DS3231 — I2C Real-Time Clock |
| **Connectivity** | Wi-Fi 802.11 b/g/n (built-in ESP32-S3) |
| **Indicator** | Addressable RGB LED strip |

---

## Software Architecture

The firmware is organized into five layers. The **Network Manager** runs alongside the Application layer and orchestrates all network-dependent modules beneath it.

```
┌────────────────────────────────────────────────────────────────────────┐
│  Layer 1 ·  APPLICATION                                                │
│            Scheduler — evaluates rules, triggers feeding cycles        │
├───────────────────────┬────────────────────────────────────────────────┤
│  Layer 2 ·  MOTOR     │  RTC      │  SENSOR      │  NET MANAGER        │
│            Stepper    │  DS3231   │  VL53L0X     │  (runs alongside    │
│            driver,    │  + SNTP   │  continuous  │   Application,      │
│            dispensing │  sync     │  sampling    │   manages L3)       │
│            actuation  │           │       │event  │       │            │
│                       │           │       └───────┼───────┘            │
├───────────────────────┴───────────┤               │                    │
│  Layer 3 ·                        │  WiFi STA+AP  │  MQTT Module       │
│                                   │  Module       │  pub: food level   │
│                                   │  credentials  │  sub: rules update │
│                                   │  via HTTP     │                    │
├───────────────────────────────────┴───────────────┴────────────────────┤
│  Layer 4 ·  ZEPHYR KERNEL                                              │
│            Threads · Timers · Workqueues · ZMS Flash · Watchdog        │
├────────────────────────────────────────────────────────────────────────┤
│  Layer 5 ·  MICROCONTROLLER  —  ESP32-S3                               │
│            Wi-Fi MAC · I2C · GPIO · Flash                              │
└────────────────────────────────────────────────────────────────────────┘
```

**Module responsibilities:**

- **Application / Scheduler (L1)** — evaluates active rules against the current RTC time and directly commands the Motor and RTC modules
- **Motor driver (L2)** — consumed directly by the Application; handles stepper motor step sequences for food dispensing
- **RTC module (L2)** — consumed directly by the Application; keeps time via DS3231 over I2C with periodic SNTP synchronization for drift correction
- **Sensor module (L2)** — continuously samples the VL53L0X ToF sensor over I2C; data is enqueued for the MQTT module and sent only when the broker connection is active (event-driven with Net Manager)
- **Network Manager (L2)** — runs concurrently with the Application; owns and orchestrates the Wi-Fi and MQTT modules, reacting to connection events and propagating state changes
- **Wi-Fi STA+AP module (L3)** — managed by Net Manager; handles association, DHCP, and hosts the HTTP server used exclusively for Wi-Fi credential provisioning
- **MQTT module (L3)** — managed by Net Manager; maintains the broker session, publishes food-level readings (publish), and receives rule updates from the cloud (subscribe + ACK)
- **Zephyr Kernel (L4)** — provides the foundational OS primitives consumed by all layers: software timers, thread creation, mutexes, workqueues, and device drivers
- **Microcontroller — ESP32-S3 (L5)** — hardware layer; all physical peripheral connections including I2C buses, GPIO pins, Wi-Fi MAC, and flash memory

---

## Project Structure

```
Automatic_pet_feeder/
├── src/                   # Main application source
├── common/                # Shared utilities and helpers
├── interface/             # Pure virtual interfaces for abstraction and unit tests
├── boards/                # Board-specific configurations and overlay
├── dts/bindings/          # Custom Devicetree bindings
├── tests/                 # Unit tests
├── CMakeLists.txt         # CMake build configuration
├── prj.conf               # Zephyr Kconfig project configuration
└── sections-rom.ld        # Linker script
```

---
## MQTT & Cloud Integration

The device integrates with **Ubidots** (or any compatible MQTT broker) over TLS.

| Direction | Topic | Payload |
|---|---|---|
| **Publish** | `/v1.6/devices/pet_feeder` | Food level reading from VL53L0X (sent only while broker is connected) |
| **Subscribe** | ACK topic | Feeding rules update received from the cloud |

TLS is enabled by default using mbedTLS. The MQTT keepalive interval is set to **30 seconds**.

The **HTTP server** runs in AP mode solely for Wi-Fi credential provisioning — it is not used for rule management.

---

## Scheduler Rules

A **rule** is a set of time constraints that, when all matched, trigger a feeding cycle. Rules are stored persistently in flash and evaluated continuously against the current RTC time (NTP-synchronized).

| Field | Description | Example |
|---|---|---|
| `hour` | Hour of the day (0–23) | `8` |
| `minute` | Minute of the hour (0–59) | `30` |
| `weekday` | Day of the week (0=Sun … 6=Sat) | `1,3,5` |
| `month_day` | Day of the month (1–31) | `15` |
| `month` | Month of the year (1–12) | `*` (any) |

Rules are delivered to the device via the MQTT subscribe channel and stored persistently in flash.

NOTE: weekday is a bit mask, which means that only one single byte is sent for validation and not list so the value needs to be between 1 and 0x7F. Example: for sunday and wednesday will be 0b00001001 = 9

---
