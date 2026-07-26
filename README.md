# Health Monitor — AI-Powered Wearable Health Assistant

A full-stack embedded systems and AI project that combines a custom wearable biosensor device with an intelligent mobile application. The device reads real-time physiological data from the body and streams it wirelessly to an iOS app, where an AI agent analyzes the readings and provides personalized health feedback.

---

## System Architecture

```
MAX30102 (Heart Rate + SpO2)  ─┐
MLX90614 (Temperature)         ├── I2C Bus ──► ESP32 ──► BLE ──► React Native App ──► Groq AI Agent
MPU6050 (Motion)               ┘                │
                                                 └──► SSD1306 OLED Display
```

The system is organized into five layers:

1. **Sensor hardware** — biometric sensors communicating over I2C
2. **Firmware** — ESP32 microcontroller reading, processing, and transmitting sensor data
3. **Wireless** — Bluetooth Low Energy (BLE) GATT data stream from device to phone
4. **Mobile app** — React Native iOS app displaying live vitals and agent feedback
5. **AI agent** — Llama 3 powered agent with session memory analyzing health trends

---

## Hardware

### Components

| Component                | Purpose                                  | Protocol       |
| ------------------------ | ---------------------------------------- | -------------- |
| ESP32-WROOM-32 Dev Board | Microcontroller + BLE radio              | —              |
| MAX30102                 | Heart rate + SpO2 sensor                 | I2C (0x57)     |
| MLX90614                 | Non-contact infrared temperature         | I2C (0x5A)     |
| MPU6050                  | Accelerometer + gyroscope (motion)       | I2C (0x68)     |
| SSD1306 OLED (0.96")     | Live vitals display on wrist unit        | I2C (0x3C)     |
| Custom 2-layer PCB       | Carrier board integrating all components | KiCad / JLCPCB |

### PCB

Designed a custom 2-layer carrier PCB in KiCad that integrates the ESP32 dev board and all sensor connectors into a compact form factor. Fabricated through JLCPCB and hand-soldered for final assembly.

### Form Factor

Two-piece wearable design:

- **Wrist unit** — houses ESP32 dev board on custom PCB, MPU6050, MLX90614, and OLED display
- **Fingertip unit** — houses MAX30102 for optimal heart rate signal quality

The two pieces connect via a thin 4-wire cable (SDA, SCL, 3.3V, GND).

---

## Firmware

Built with **ESP-IDF v6.0** (Espressif's official framework) for professional-grade embedded development close to the metal.

### Key firmware features

- I2C bus management across 4 sensors simultaneously using ESP-IDF I2C driver
- MAX30102 PPG signal processing using a **2nd order Butterworth bandpass filter (0.5–5Hz)** to isolate the heartbeat signal from noise
- Heart rate calculated from peak detection on the filtered signal, averaged over 5 readings
- SpO2 calculated from RED/IR ratio using the R-curve approximation
- MLX90614 infrared temperature reading with Kelvin to Fahrenheit conversion
- MPU6050 accelerometer magnitude deviation for 3-level activity classification (Resting/Light/Active)
- FreeRTOS task for continuous MAX30102 sampling at 100Hz independent of main loop
- BLE GATT server (Bluedroid stack) broadcasting all vitals as structured JSON at 500ms intervals
- SSD1306 OLED driver rendering live vitals at 128x64 resolution

### Tech stack

- Language: C
- Framework: ESP-IDF v6.0
- RTOS: FreeRTOS (built into ESP-IDF)
- Build system: CMake + Ninja
- BLE: Bluedroid GATT stack
- Display: espressif/ssd1306 component

---

## Mobile App

Built with **React Native** and **Expo**, targeting iOS. Deployed via custom Expo dev client built through Xcode for full native BLE support.

### Screens

- **Dashboard** — live vitals display with heart rate, SpO2, temperature, and motion level updated in real time over BLE, with device connect/disconnect control
- **History** — session trend charts for all vitals using react-native-chart-kit, populated from real BLE data
- **AI Agent** — conversational health feedback interface powered by Llama 3 with session memory, using real BLE vitals as context
- **Settings** — user profile (age, weight, height, fitness level), BLE device pairing, and auto-analyze toggle

### Tech stack

- Framework: React Native + Expo
- Navigation: Expo Router (file-based)
- BLE: react-native-ble-plx
- Charts: react-native-chart-kit
- AI: Groq API (Llama 3.3 70B) via fetch
- State: React Context (BLEContext) shared across all screens
- Language: TypeScript

---

## AI Agent

The agent is built on top of Groq's Llama 3.3 70B model with custom orchestration logic:

- **Real vitals context** — receives live BLE sensor data as structured input for every analysis
- **Session memory** — maintains a history of all readings and responses within a session, passing context into every API call so the agent can detect trends over time
- **Classification system** — model self-tags responses as `[normal]`, `[warning]`, or `[info]`, which the app uses to apply color-coded visual treatment
- **User profile personalization** — age, weight, height, and fitness level injected into the system prompt for context-aware feedback
- **Direct fetch API** — uses native fetch instead of SDK for reliable React Native compatibility

---

## Project Structure

```
health-monitor/
├── embedded/                          # ESP32 firmware (ESP-IDF)
│   └── health_monitor_firmware/
│       ├── main/
│       │   └── health_monitor_firmware.c
│       └── CMakeLists.txt
└── app/                               # React Native mobile app (Expo)
    └── app/
        ├── (tabs)/
        │   ├── index.tsx              # Dashboard screen
        │   ├── history.tsx            # History screen
        │   ├── agent.tsx              # AI Agent screen
        │   └── settings.tsx           # Settings screen
        ├── context/
        │   └── BLEContext.tsx         # Shared BLE state + vitals history + user profile
        ├── hooks/
        │   └── useBLE.ts              # BLE connection, scanning, and data management
        └── _layout.tsx
```

---

## Getting Started

### Firmware

1. Install [VS Code](https://code.visualstudio.com/) and the [ESP-IDF Extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-vscode-extension)
2. Open the ESP-IDF Installation Manager and install ESP-IDF v6.0
3. Open the `embedded/health_monitor_firmware` folder in VS Code
4. Build: `idf.py build`
5. Flash: `idf.py -p PORT flash monitor`

### Mobile App

1. Install [Node.js](https://nodejs.org/)
2. Install dependencies:
   ```bash
   cd app
   npm install
   ```
3. Create a `.env` file in the `app/` directory:
   ```
   EXPO_PUBLIC_GROQ_API_KEY=your_groq_api_key
   ```
4. Start the development server:
   ```bash
   npx expo start --dev-client
   ```
5. Open the installed dev client on your iPhone and scan the QR code

> **Note:** BLE requires a custom Expo dev client built via Xcode. Expo Go cannot run native BLE modules.

---

## Roadmap

- [x] React Native app scaffold with four screens
- [x] Dashboard UI with live vitals display
- [x] Session history with real-time trend charts
- [x] AI agent with Groq Llama 3 integration
- [x] Session memory for trend-aware agent responses
- [x] Settings screen with user profile and device connection
- [x] ESP32 firmware — I2C sensor reading (all 4 sensors)
- [x] ESP32 firmware — Butterworth bandpass filter for heart rate
- [x] ESP32 firmware — BLE GATT server transmitting JSON vitals
- [x] ESP32 firmware — OLED display driver
- [x] BLE integration in React Native app — live data streaming end to end
- [x] Real vitals wired into AI agent, history charts, and settings
- [x] Custom PCB designed in KiCad and fabricated through JLCPCB
- [x] PCB assembled, soldered, and tested — fully functional
- [ ] Auto-analysis with configurable intervals
- [ ] Data persistence across sessions
- [ ] Final wearable enclosure (3D printed)

---

## Skills Demonstrated

- Embedded C firmware development (ESP-IDF v6.0, FreeRTOS)
- I2C communication protocol across multiple devices
- Digital signal processing (Butterworth bandpass filter, PPG peak detection)
- Bluetooth Low Energy (BLE) GATT server architecture (Bluedroid stack)
- PCB schematic design and layout (KiCad)
- Hardware fabrication — JLCPCB manufacturing and hand soldering
- React Native iOS mobile development
- AI agent design, prompt engineering, and session memory
- Full-stack system integration across hardware, firmware, wireless, mobile, and AI layers
