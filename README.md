# Health Monitor — AI-Powered Wearable Health Assistant
 
A full-stack embedded systems and AI project that combines a custom wearable biosensor device with an intelligent mobile application. The device reads real-time physiological data from the body and streams it wirelessly to an iOS app, where an AI agent analyzes the readings and provides personalized health feedback.
 
---
 
## System Architecture
 
```
MAX30102 (Heart Rate + SpO2)  ─┐
MLX90614 (Temperature)         ├── I2C Bus ──► ESP32 ──► BLE ──► React Native App ──► Groq AI Agent
MPU6050 (Motion)               ┘                │
                                                 └──► OLED Display
```
 
The system is organized into five layers:
 
1. **Sensor hardware** — biometric sensors communicating over I2C
2. **Firmware** — ESP32 microcontroller reading, processing, and transmitting sensor data
3. **Wireless** — Bluetooth Low Energy (BLE) data stream from device to phone
4. **Mobile app** — React Native app displaying live vitals and agent feedback
5. **AI agent** — Llama 3 powered agent with session memory analyzing health trends
---
 
## Hardware
 
### Components
| Component | Purpose | Protocol |
|---|---|---|
| ESP32-WROOM-32 | Microcontroller + BLE radio | — |
| MAX30102 | Heart rate + SpO2 sensor | I2C |
| MLX90614 | Non-contact infrared temperature | I2C |
| MPU6050 | Accelerometer + gyroscope (motion) | I2C |
| SSD1306 OLED (0.96") | Live vitals display on wrist unit | I2C |
 
### Form Factor
Two-piece wearable design:
- **Fingertip sleeve** — houses MAX30102 for clean optical signal
- **Wrist unit** — houses ESP32, MLX90614, MPU6050, OLED display, and battery
The two pieces connect via a thin 6-wire cable (SDA, SCL, 3.3V, GND + 2 analog lines).
 
---
 
## Firmware
 
Built with **ESP-IDF v6.0** (Espressif's official framework), not Arduino — keeping the code close to the metal for professional-grade embedded development.
 
### Key firmware responsibilities
- I2C bus management across multiple sensors
- MAX30102 signal processing to extract BPM and SpO2 from raw PPG data
- FreeRTOS task architecture — separate tasks for sensing, display, and BLE transmission
- BLE GATT server broadcasting sensor data as structured JSON
- OLED display driver rendering live vitals at 128x64 resolution
### Tech stack
- Language: C
- Framework: ESP-IDF v6.0
- RTOS: FreeRTOS (built into ESP-IDF)
- Build system: CMake + Ninja
---
 
## Mobile App
 
Built with **React Native** and **Expo**, targeting iOS.
 
### Screens
- **Dashboard** — live vitals display with heart rate, SpO2, temperature, and motion level updated in real time over BLE
- **History** — session trend charts for all vitals using react-native-chart-kit
- **AI Agent** — conversational health feedback interface powered by Llama 3 with session memory
- **Settings** — user profile (age, weight, height, fitness level) and BLE device pairing
### Tech stack
- Framework: React Native + Expo
- Navigation: Expo Router (file-based)
- BLE: react-native-ble-plx
- Charts: react-native-chart-kit
- AI: Groq SDK (Llama 3.3 70B)
- Language: TypeScript
---
 
## AI Agent
 
The agent is built on top of Groq's Llama 3.3 70B model with custom orchestration logic:
 
- **Session memory** — maintains a history of all readings and responses within a session, passing context into every API call so the agent can detect trends over time
- **Classification system** — model self-tags responses as `[normal]`, `[warning]`, or `[info]`, which the app uses to apply appropriate visual treatment
- **Personalization** — user profile from settings is injected into the system prompt so the agent understands the user's baseline
- **Auto-analysis** — configurable interval-based analysis that runs without user input during active sessions
---
 
## Project Structure
 
```
health-monitor/
├── embedded/          # ESP32 firmware (ESP-IDF)
│   ├── main/
│   │   └── main.c     # Firmware entry point
│   └── CMakeLists.txt
└── app/               # React Native mobile app (Expo)
    ├── app/
    │   ├── (tabs)/
    │   │   ├── index.tsx      # Dashboard screen
    │   │   ├── history.tsx    # History screen
    │   │   ├── agent.tsx      # AI Agent screen
    │   │   └── settings.tsx   # Settings screen
    │   └── _layout.tsx
    ├── components/
    └── hooks/
        └── useBLE.ts          # BLE connection and data management
```
 
---
 
## Getting Started
 
### Firmware
 
1. Install [VS Code](https://code.visualstudio.com/) and the [ESP-IDF Extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-vscode-extension)
2. Run **ESP-IDF: Configure ESP-IDF Extension** and install ESP-IDF v6.0
3. Open the `embedded/` folder in VS Code
4. Build: `idf.py build`
5. Flash: `idf.py -p PORT flash monitor`
### Mobile App
 
1. Install [Node.js](https://nodejs.org/) and [Expo Go](https://expo.dev/go) on your iPhone
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
   npx expo start
   ```
5. Scan the QR code with your iPhone camera
---
 
## Roadmap
 
- [x] React Native app scaffold with four screens
- [x] Dashboard UI with live vitals display
- [x] Session history with trend charts
- [x] AI agent with Groq Llama 3 integration
- [x] Session memory for trend-aware agent responses
- [x] Settings screen with user profile
- [ ] ESP32 firmware — I2C sensor reading
- [ ] ESP32 firmware — BLE GATT server
- [ ] ESP32 firmware — OLED display driver
- [ ] BLE integration in React Native app
- [ ] Live data streaming end to end
- [ ] Auto-analysis with configurable intervals
- [ ] Data persistence across sessions
---
 
## Skills Demonstrated
 
- Embedded C firmware development (ESP-IDF, FreeRTOS)
- I2C communication protocol
- Bluetooth Low Energy (BLE) GATT architecture
- Signal processing (PPG → heart rate extraction)
- React Native mobile development
- AI agent design and prompt engineering
- Full-stack system integration across hardware and software layers
