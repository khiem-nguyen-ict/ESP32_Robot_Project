# ESP32 WiFi Mobile Robot

> **Language:** C (C11 / gnu11)  
> **Framework:** Arduino + FreeRTOS  
> **Build system:** PlatformIO

---

## Project structure

```
ESP32_Robot_Project/
│
├── platformio.ini              PlatformIO build configuration
│
├── include/
│   ├── config.h                All pin definitions and constants
│   ├── robot_types.h           Shared enums and structs
│   ├── motor.h                 Motor driver API
│   ├── sensor.h                HC-SR04 + SG90 servo API
│   ├── wifi_manager.h          WiFi / WebSocket manager API
│   ├── command_parser.h        JSON command parser API
│   ├── auto_pilot.h            Obstacle avoidance API
│   └── webpage.h               Embedded HTML/CSS/JS (PROGMEM)
│
└── src/
    ├── main.c                  Entry point, FreeRTOS tasks
    ├── motor.c                 L298N driver implementation
    ├── sensor.c                HC-SR04 + SG90 implementation
    ├── wifi_manager.c          WiFi / AsyncWebServer / WS
    ├── command_parser.c        JSON → state transitions
    └── auto_pilot.c            Autonomous navigation logic
```

---

## Hardware

| Component        | Description                            |
|------------------|----------------------------------------|
| ESP32 Dev Module | Dual-core 240 MHz, WiFi 802.11 b/g/n  |
| L298N            | Dual H-bridge DC motor driver          |
| 2× DC Motor      | Left and right drive wheels            |
| HC-SR04          | Ultrasonic distance sensor             |
| SG90             | 180° micro servo (carries HC-SR04)     |
| 12V battery      | Motor power supply                     |
| 5V regulator     | Logic power (or USB for ESP32 only)    |

---

## Pin mapping

### L298N ↔ ESP32

| L298N pin | ESP32 GPIO | Notes              |
|-----------|------------|--------------------|
| IN1       | 25         | Motor A direction  |
| IN2       | 26         | Motor A direction  |
| ENA       | 27         | Motor A PWM (LEDC) |
| IN3       | 14         | Motor B direction  |
| IN4       | 12         | Motor B direction  |
| ENB       | 13         | Motor B PWM (LEDC) |
| 12V       | Battery+   | Motor supply       |
| GND       | GND        | Common ground      |

### HC-SR04 ↔ ESP32

| HC-SR04 pin | ESP32 GPIO | Notes                          |
|-------------|------------|--------------------------------|
| VCC         | 5V         |                                |
| TRIG        | 5          | OUTPUT                         |
| ECHO        | 18         | ⚠ Use 1kΩ/2kΩ voltage divider |
| GND         | GND        |                                |

> **Important:** The ECHO pin outputs 5 V.  
> Connect a 1 kΩ resistor in series between ECHO and GPIO 18,  
> then a 2 kΩ resistor from GPIO 18 to GND.  
> This divides 5 V → 3.33 V (safe for ESP32).

### SG90 ↔ ESP32

| SG90 wire  | ESP32 GPIO | Notes        |
|------------|------------|--------------|
| Red (VCC)  | 5V         |              |
| Yellow/Orange (SIG) | 19 | PWM signal |
| Brown (GND)| GND        |              |

---

## Software architecture

```
Core 0 — Protocol CPU                Core 1 — App CPU
┌─────────────────────────┐          ┌─────────────────────────┐
│  task_web  (priority 1) │          │  task_motion (priority 2)│
│  Stack: 8192 bytes      │          │  Stack: 4096 bytes       │
│                         │          │                          │
│  Every 250 ms:          │          │  MANUAL mode:            │
│  • Snapshot g_state     │          │  • motor_execute(cmd)    │
│  • Serialize → JSON     │          │  • HC-SR04 center read   │
│  • ws->textAll()        │          │                          │
│                         │          │  AUTO mode:              │
│  Every 5 s:             │          │  • servo scan 3 dirs     │
│  • ws->cleanupClients() │          │  • auto_pilot_run()      │
└─────────────────────────┘          └─────────────────────────┘
            │                                     │
            └──────── shared_state_t ─────────────┘
                  (protected by g_mutex)
```

### WebSocket JSON protocol

**Client → ESP32**

```json
{ "type": "move",  "cmd":   "forward" }
{ "type": "move",  "cmd":   "backward" }
{ "type": "move",  "cmd":   "left" }
{ "type": "move",  "cmd":   "right" }
{ "type": "move",  "cmd":   "stop" }
{ "type": "mode",  "value": "auto" }
{ "type": "mode",  "value": "manual" }
{ "type": "speed", "value": 200 }
```

**ESP32 → Client (broadcast every 250 ms)**

```json
{
  "type":     "status",
  "mode":     "manual",
  "lastCmd":  "FORWARD",
  "speed":    180,
  "obstacle": false,
  "uptime":   12345,
  "left":     80,
  "center":   120,
  "right":    95
}
```

---

## Autonomous obstacle avoidance

```
servo scans LEFT (45°) → CENTER (90°) → RIGHT (135°)
             │
     center > 30 cm?
     ┌── YES ──┐        NO
     │         │         │
   FORWARD   STOP   check left + right
                    ├── both clear → turn toward farther side
                    ├── only left  → turn LEFT
                    ├── only right → turn RIGHT
                    └── all blocked → BACK UP + spin 180°
```

---

## Quick start

### 1. Install PlatformIO

```bash
pip install platformio
```

Or install the **PlatformIO IDE** extension in VS Code.

### 2. Configure WiFi

Edit `include/config.h`:

```c
#define WIFI_SSID     "your_network_name"
#define WIFI_PASSWORD "your_password"
```

If the ESP32 cannot connect within 15 seconds it falls back to  
**Access Point mode**: connect to `ESP32_Robot_AP` (password: `robot1234`).

### 3. Build and upload

```bash
cd ESP32_Robot_Project
pio run --target upload
pio device monitor
```

### 4. Open the web interface

Look for the IP address in the Serial monitor output:

```
[WiFi] Connected — IP: 192.168.1.42
```

Open `http://192.168.1.42` in any browser on the same network.

---

## Required libraries (auto-installed by PlatformIO)

| Library | Version | Purpose |
|---------|---------|---------|
| esphome/AsyncTCP-esphome | ^2.1.4 | TCP backend for AsyncWebServer |
| mathieucarbou/ESPAsyncWebServer | ^3.3.12 | Non-blocking HTTP + WebSocket |
| bblanchon/ArduinoJson | ^7.2.1 | JSON serialisation / parsing |
| madhephaestus/ESP32Servo | ^3.0.6 | SG90 servo control via LEDC |

---

## Notes

- All source files are compiled as **C11** (`-x c -std=gnu11`).  
  The only exception is object instantiation inside `sensor.c`  
  and `wifi_manager.c` which must use `new` / `delete` to wrap  
  the underlying C++ library objects — those two files are  
  compiled as C++ by the compiler but expose a pure-C API.
- Set `DEBUG_ENABLED 0` in `config.h` for production builds  
  to eliminate all `Serial.print` overhead.
- Use separate power supplies for motors (12 V) and the ESP32  
  (5 V via USB or regulator) to prevent reset spikes.
