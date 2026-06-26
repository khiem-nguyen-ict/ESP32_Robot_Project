# ESP32 WiFi Mobile Robot 🤖

**Autonomous robot with real-time WebSocket control, obstacle avoidance, and web interface**

> **Language:** C (C11 / gnu11)  
> **Framework:** Arduino + FreeRTOS  
> **Build system:** PlatformIO

A complete educational project demonstrating embedded systems, IoT, real-time OS, sensor integration, and autonomous navigation on ESP32 microcontroller.

---

## ✨ Features

- 🎮 **Real-time WebSocket Control** - Browser-based control interface
- 🤖 **Autonomous Obstacle Avoidance** - Ultrasonic sensor-based navigation
- 📡 **WiFi Connectivity** - WiFi or AccessPoint mode
- 🔧 **FreeRTOS Multi-tasking** - Dual-core parallel execution
- 🕸️ **Embedded Web Interface** - HTML/CSS/JS served directly from ESP32
- 🔌 **PWM Motor Control** - L298N H-bridge integration
- 📊 **Real-time Status Broadcasting** - JSON status updates to clients
- 🛡️ **Robust Sensor Integration** - HC-SR04 ultrasonic + SG90 servo

---

## 🏗️ Project Structure

```
ESP32_Robot_Project/
│
├── platformio.ini              # PlatformIO build configuration
│
├── include/
│   ├── config.h                # Pin definitions & constants
│   ├── robot_types.h           # Enums and structs
│   ├── motor.h                 # Motor driver API
│   ├── sensor.h                # HC-SR04 + SG90 API
│   ├── wifi_manager.h          # WiFi/WebSocket API
│   ├── command_parser.h        # JSON command parser
│   ├── auto_pilot.h            # Obstacle avoidance logic
│   └── webpage.h               # Embedded HTML/CSS/JS
│
└── src/
    ├── main.c                  # FreeRTOS task setup
    ├── motor.c                 # L298N implementation
    ├── sensor.c                # HC-SR04 + SG90 impl
    ├── wifi_manager.c          # WiFi + WebSocket
    ├── command_parser.c        # JSON parsing
    └── auto_pilot.c            # Autonomous navigation
```

---

## 🛠️ Hardware Components

| Component | Description | Purpose |
|-----------|-------------|---------|
| **ESP32 Dev Module** | Dual-core 240 MHz, WiFi 802.11 b/g/n | Main microcontroller |
| **L298N** | Dual H-bridge DC motor driver | Motor speed/direction control |
| **2× DC Motor** | Left and right drive wheels | Movement |
| **HC-SR04** | Ultrasonic distance sensor | Obstacle detection |
| **SG90** | 180° micro servo | Sensor rotation (scanning) |
| **12V Battery** | Power supply | Motor power |
| **5V Regulator** | Logic power | ESP32 & sensors |

---

## 📍 Pin Mapping

### L298N ↔ ESP32

| L298N | ESP32 | Function |
|-------|-------|----------|
| IN1   | GPIO 25 | Motor A direction |
| IN2   | GPIO 26 | Motor A direction |
| ENA   | GPIO 27 | Motor A PWM |
| IN3   | GPIO 14 | Motor B direction |
| IN4   | GPIO 12 | Motor B direction |
| ENB   | GPIO 13 | Motor B PWM |

### HC-SR04 ↔ ESP32

| HC-SR04 | ESP32 | Notes |
|---------|-------|-------|
| VCC | 5V | Power |
| TRIG | GPIO 5 | Trigger signal |
| ECHO | GPIO 18 | ⚠ Use voltage divider (5V → 3.3V) |
| GND | GND | Ground |

### SG90 ↔ ESP32

| SG90 | ESP32 | Function |
|------|-------|----------|
| Red | 5V | Power |
| Orange/Yellow | GPIO 19 | PWM signal |
| Brown | GND | Ground |

---

## 🧠 Software Architecture

```
Core 0 — Protocol CPU              Core 1 — App CPU
┌──────────────────────┐            ┌──────────────────────┐
│ task_web (priority 1)│            │ task_motion (priority 2)
│ Stack: 8192 bytes    │            │ Stack: 4096 bytes     
│                      │            │                       
│ Every 250 ms:        │            │ MANUAL mode:          
│ • Snapshot state     │            │ • motor_execute(cmd) 
│ • Serialize JSON     │            │ • HC-SR04 center read
│ • ws->textAll()      │            │                       
│                      │            │ AUTO mode:            
│ Every 5 s:           │            │ • servo scan 3 dirs   
│ • ws->cleanupClients │            │ • auto_pilot_run()    
└──────────────────────┘            └──────────────────────┘
         │                                    │
         └────── shared_state_t ─────────────┘
              (protected by g_mutex)
```

---

## 📡 WebSocket JSON Protocol

### Client → ESP32

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

### ESP32 → Client (Every 250ms)

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

## 🤖 Autonomous Obstacle Avoidance Algorithm

```
servo scans LEFT (45°) → CENTER (90°) → RIGHT (135°)
           │
   center > 30 cm?
   ┌── YES ──┐         NO
   │         │          │
 FORWARD   STOP   check left + right
                  ├── both clear → turn toward farther side
                  ├── only left  → turn LEFT
                  ├── only right → turn RIGHT
                  └── all blocked → BACK UP + spin 180°
```

---

## 🚀 Quick Start

### 1. Install PlatformIO

```bash
pip install platformio
# or install PlatformIO IDE extension in VS Code
```

### 2. Configure WiFi

Edit `include/config.h`:

```c
#define WIFI_SSID     "your_network_name"
#define WIFI_PASSWORD "your_password"
```

**Fallback:** If connection fails, access point `ESP32_Robot_AP` with password `robot1234`

### 3. Build & Upload

```bash
cd ESP32_Robot_Project
pio run --target upload
pio device monitor
```

### 4. Open Web Interface

Look for the IP address in serial output:
```
[WiFi] Connected — IP: 192.168.1.42
```

Open `http://192.168.1.42` in your browser.

---

## 📚 Required Libraries (Auto-installed by PlatformIO)

| Library | Version | Purpose |
|---------|---------|---------|
| esphome/AsyncTCP-esphome | ^2.1.4 | TCP backend |
| mathieucarbou/ESPAsyncWebServer | ^3.3.12 | HTTP + WebSocket |
| bblanchon/ArduinoJson | ^7.2.1 | JSON serialization |
| madhephaestus/ESP32Servo | ^3.0.6 | Servo control |

---

## ⚙️ Configuration

### Motor Speed Control

In `include/config.h`:
```c
#define PWM_FREQ    1000      // 1 kHz
#define PWM_RESOLUTION 8      // 8-bit (0-255)
#define DEFAULT_SPEED 200     // Default motor speed
```

### Sensor Thresholds

In `src/auto_pilot.c`:
```c
#define OBSTACLE_DISTANCE 30  // cm
#define SCAN_LEFT_ANGLE   45  // degrees
#define SCAN_RIGHT_ANGLE  135 // degrees
```

---

## 🔧 Troubleshooting

| Issue | Solution |
|-------|----------|
| **ESP32 won't upload** | Check USB cable, install CH340 driver, reset board |
| **WiFi connection fails** | Verify SSID/password in config.h |
| **Motor not moving** | Check L298N connections, verify power supply (12V for motors) |
| **Sensor not detecting** | Verify HC-SR04 wiring, check voltage divider on ECHO |
| **Web interface not loading** | Check ESP32 IP address in serial monitor, ensure on same network |
| **Servo not moving** | Verify GPIO 19 connection, check 5V power |

---

## 🔌 Power Considerations

⚠️ **Use separate power supplies:**
- **Motors:** 12V battery (high current draw)
- **ESP32 & Sensors:** 5V regulator or USB

This prevents reset spikes from motor current draws.

---

## 🎯 Learning Outcomes

By completing this project, you'll learn:
- ✅ Embedded C programming on microcontrollers
- ✅ FreeRTOS real-time operating system
- ✅ PWM motor control and H-bridge drivers
- ✅ Sensor integration (ultrasonic, servo)
- ✅ WiFi connectivity on ESP32
- ✅ WebSocket real-time communication
- ✅ Autonomous navigation algorithms
- ✅ PlatformIO development workflow

---

## 📖 Resources

- [ESP32 Official Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/)
- [PlatformIO IDE](https://platformio.org/)
- [FreeRTOS Documentation](https://www.freertos.org/RTOS.html)
- [Arduino Reference](https://www.arduino.cc/reference/en/)

---

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add: feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 📞 Support

- **Author**: Khiem Nguyen
- **Email**: nguyenthanhkhiemvn@gmail.com
- **GitHub**: [@khiem-nguyen-ict](https://github.com/khiem-nguyen-ict)

---

## ⭐ Show Your Support

- ⭐ Star if you found this useful
- 🍴 Fork to create your own robot
- 📢 Share with the maker community
- 💬 Provide feedback and suggestions

---

**Let's build amazing robots! 🚀**
