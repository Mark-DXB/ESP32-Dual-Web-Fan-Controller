# 🌪️ ESP32 Dual Web Fan Controller

A professional dual fan controller for ESP32 Board #1 with independent PWM control, real-time RPM monitoring, and a beautiful web interface.

## 🎯 Features

- **🔥 Dual Fan Control** - Independent control of intake and exhaust fans
- **🌐 Professional Web UI** - Beautiful, responsive interface with real-time updates
- **⚡ Precision Control** - 0-100% speed control in 10% increments (11 buttons per fan)
- **📊 Live RPM Monitoring** - Real-time tachometer feedback for both fans
- **🎨 Smart UI** - Independent button highlighting, debug logging, status updates
- **📡 WiFi Connectivity** - Auto-connect with AP mode fallback
- **📱 Mobile Friendly** - Responsive design works on all devices

## 🔌 Hardware Configuration

### Board #1 Specifications
- **Chip**: ESP32-D0WD-V3 (Original)
- **MAC Address**: 44:1d:64:f5:b4:84
- **Flash**: 4MB
- **USB Chip**: CP2102
- **Crystal**: 40MHz

### GPIO Pin Assignment (Optimized for Reliability)
```
🔧 PWM Outputs (25kHz, 8-bit resolution):
GPIO 5  - Fan 1 (Intake) PWM Output
GPIO 21 - Fan 2 (Exhaust) PWM Output

📊 Tachometer Inputs (with pull-ups):
GPIO 18 - Fan 1 (Intake) Tacho Input  
GPIO 19 - Fan 2 (Exhaust) Tacho Input
```

### Dual Fan Connection
```
Fan 1 (Intake) - 4-pin Connector:
Pin 1: Ground (Black)
Pin 2: +12V Power (Red) 
Pin 3: Tacho Signal (Yellow) → GPIO 18
Pin 4: PWM Control (Blue) → GPIO 5

Fan 2 (Exhaust) - 4-pin Connector:
Pin 1: Ground (Black)
Pin 2: +12V Power (Red) 
Pin 3: Tacho Signal (Yellow) → GPIO 19
Pin 4: PWM Control (Blue) → GPIO 21
```

### ⚡ 12V Power Circuit with MOSFET Drivers

The ESP32 outputs 3.3V PWM signals, but most PC fans require 12V PWM. Use IRF520 MOSFETs to level-shift:

#### 📋 Circuit Documentation
- **[Complete Circuit Diagram](MOSFET_Circuit_Diagram.md)** - Detailed technical schematic with component values
- **[Simple Wiring Guide](Simple_Wiring_Diagram.txt)** - Easy-to-follow breadboard layout

#### 🔧 Required Components (per fan)
- **1x IRF520 N-Channel MOSFET** - Main switching element (TO-220 package)
- **1x 220Ω Resistor** - Gate current limiting (ESP32 protection)  
- **1x 10kΩ Resistor** - Gate pull-down (ensures OFF state)
- **1x 1N4007 Diode** - Flyback protection (prevents voltage spikes)

#### 🔌 Connection Summary
```
ESP32 GPIO 5/21 → 220Ω → IRF520 Gate → 10kΩ to GND
12V Supply (+) → Fan Red Wire
12V Supply (-) → Circuit Ground  
IRF520 Drain → Fan Black Wire (PWM switched ground)
IRF520 Source → Circuit Ground
Fan Yellow → ESP32 GPIO 18/19 (Tacho - direct connection)
```

#### ⚡ Circuit Performance
- **PWM Frequency**: 25kHz (silent operation)
- **Resolution**: 8-bit (0-255, mapped to 0-100%)
- **Current Handling**: Up to 9A per channel (typical fans: 0.1-2A)
- **Power Dissipation**: ~0.6W per MOSFET at full load

## 📡 Network Configuration

- **SSID**: HareNet
- **Password**: 01505336189
- **Web Interface**: http://[ESP32_IP_ADDRESS]

## 🚀 Getting Started

### 1. Hardware Setup
1. Connect Board #1 to your computer via USB
2. Wire the fan to GPIO 2 (PWM) and GPIO 18 (Tacho)
3. Ensure fan has proper 12V power supply

### 2. Software Upload
```bash
# Using PlatformIO
pio run --target upload

# Monitor serial output
pio device monitor
```

### 3. Web Interface Access
1. Open serial monitor to find ESP32's IP address
2. Navigate to `http://[IP_ADDRESS]` in your browser
3. Use the web interface to control fan speed

## 🎛️ Web Interface

The professional web interface provides:

- **🌟 Dual Fan Control** - Independent control panels for each fan
- **🎯 Precision Buttons** - 11 speed buttons per fan (0%, 10%, 20%, 30%, 40%, 50%, 60%, 70%, 80%, 90%, 100%)
- **💡 Smart Highlighting** - Green button shows current speed, independent for each fan
- **📊 Live RPM Display** - Real-time tachometer readings for both fans
- **🔍 Debug Section** - Live JavaScript activity logging (red bar at top)
- **📡 System Status** - WiFi network, IP address, signal strength, uptime
- **📱 Responsive Design** - Perfect on desktop, tablet, and mobile
- **🎨 Professional Styling** - Modern blue gradient design with smooth animations

## 📊 API Endpoints

### Get System Status
```http
GET /status
```
Returns JSON with comprehensive system information:
```json
{
  "fan1_speed": 40,
  "fan1_rpm": 1200,
  "fan2_speed": 70,
  "fan2_rpm": 2100,
  "uptime": 3600,
  "board_mac": "44:1D:64:F5:B4:84",
  "wifi_mode": "Station",
  "wifi_network": "HareNet",
  "wifi_signal": -65,
  "ip_address": "10.0.1.146"
}
```

### Control Individual Fans
```http
POST /set_fan1
Content-Type: application/json
{
  "speed": 60
}

POST /set_fan2
Content-Type: application/json
{
  "speed": 80
}
```

### Legacy Support
```http
POST /set_speed
Content-Type: application/json
{
  "speed": 50,
  "fan1_speed": 40,
  "fan2_speed": 70
}
```

## 🔧 Technical Details

### Dual PWM Configuration  
- **Frequency**: 25kHz (silent operation for both fans)
- **Resolution**: 8-bit (256 duty cycle steps = 0.39% precision)
- **Channels**: LEDC Channel 0 (Fan 1), LEDC Channel 1 (Fan 2)
- **GPIO Selection**: Optimized pins (GPIO 5 & 21) avoid boot restrictions
- **Range**: 0-100% in 10% increments via web UI
- **Update Rate**: Instant response to web commands

### Dual RPM Measurement
- **Method**: Independent pulse counting via interrupts for each fan
- **Update Rate**: 1-second measurement windows for both fans
- **Calculation**: (pulses × 60) ÷ 2 pulses/revolution
- **Accuracy**: ±30 RPM typical for standard PC fans
- **Range**: 0-6000 RPM supported
- **Pull-ups**: Hardware pull-up resistors on tacho inputs

### Advanced WiFi Features
- **Primary Mode**: Auto-connect to HareNet network
- **Fallback Mode**: Creates "ESP32-FanController" AP if STA fails
- **Connection Monitoring**: Real-time signal strength display
- **Retry Logic**: Automatic reconnection with exponential backoff
- **Status Reporting**: Live network information in web UI

## 🛠️ Development

### Project Structure
```
ESP32_Web_Fan_Controller/
├── src/
│   └── main.cpp                    # Main dual fan controller application
├── platformio.ini                  # PlatformIO configuration
├── README.md                       # Project documentation
├── BOARD_1_CONFIG.md               # Board-specific configuration notes
├── .gitignore                      # Git ignore rules
├── dual_fan_interface.html         # Web UI development version
├── comprehensive_esp32_finder.py   # Network discovery utility
├── monitor_board1.py               # Serial monitoring utility
└── debug tools/                    # Various HTML and monitoring scripts
```

### Dependencies
- **ESP Async WebServer** @ 3.0.6 - High-performance async web server
- **ArduinoJson** @ 7.4.2 - JSON parsing and generation
- **WiFi** @ 2.0.0 - ESP32 WiFi management (built-in)

### Build Configuration
- **Platform**: ESP32 (Espressif 32)
- **Framework**: Arduino
- **Board**: esp32dev (Generic ESP32)
- **Upload Speed**: 921600 baud
- **Monitor Speed**: 115200 baud
- **Debug Level**: INFO (configurable)

## 🎯 Usage Examples

### Web Interface Control
1. **Access**: Navigate to `http://10.0.1.146` (or your ESP32's IP)
2. **Fan 1 Control**: Click any button in the "Fan 1 (GPIO 5)" section
3. **Fan 2 Control**: Click any button in the "Fan 2 (GPIO 21)" section  
4. **Monitor**: Watch real-time RPM updates and debug messages
5. **Visual Feedback**: Active button glows green, others remain grey

### API Control Examples
```bash
# Set Fan 1 (Intake) to 60%
curl -X POST http://10.0.1.146/set_fan1 \
  -H "Content-Type: application/json" \
  -d '{"speed":60}'

# Set Fan 2 (Exhaust) to 80%
curl -X POST http://10.0.1.146/set_fan2 \
  -H "Content-Type: application/json" \
  -d '{"speed":80}'

# Get complete system status
curl -s http://10.0.1.146/status | jq

# Set both fans simultaneously (legacy endpoint)
curl -X POST http://10.0.1.146/set_speed \
  -H "Content-Type: application/json" \
  -d '{"fan1_speed":40, "fan2_speed":90}'
```

## 📈 Monitoring & Status

The system provides comprehensive real-time monitoring:

### Fan Performance
- **Dual RPM Display** - Independent readings for both fans
- **PWM Percentage** - Current drive level (0-100%) for each fan
- **Speed Accuracy** - Live comparison between commanded and actual speeds

### System Health  
- **WiFi Signal Strength** - Real-time dBm readings
- **Network Status** - Station mode vs AP mode indication
- **IP Address** - Current network assignment
- **System Uptime** - Runtime in hours/minutes/seconds
- **Board Identification** - MAC address verification

### Debug Information
- **JavaScript Activity** - Live operation logging in red debug bar
- **API Response Times** - Network latency monitoring
- **Connection Status** - Real-time connectivity feedback

## 🔒 Safety & Reliability Features

- ✅ **Input Validation** - Speed settings bounded to 0-100% range
- ✅ **Hardware Protection** - GPIO pull-up resistors prevent floating inputs
- ✅ **Non-blocking Architecture** - Async operations prevent system lockup
- ✅ **Connection Recovery** - Auto-reconnect with AP mode fallback
- ✅ **Error Handling** - Graceful degradation with user feedback
- ✅ **Interrupt-driven RPM** - Hardware interrupts ensure accurate measurements

## 🏆 Project Achievements

This ESP32 dual fan controller represents a **production-quality embedded system** with:

### ⚡ **Hardware Excellence**
- **Optimized GPIO Selection** - Avoided boot-restricted pins (GPIO 2/4)
- **25kHz PWM Output** - Silent operation on both channels
- **Interrupt-driven Tachometers** - Precise RPM measurement
- **Robust Power Design** - Proper pull-ups and signal conditioning

### 🌐 **Software Excellence** 
- **Professional Web UI** - 11 buttons per fan with smart highlighting
- **Real-time Updates** - 2-second refresh with live debug logging
- **Responsive Design** - Perfect on desktop, tablet, and mobile
- **RESTful API** - Clean JSON endpoints for automation

### 🔧 **Engineering Excellence**
- **Dual Independent Control** - Separate intake/exhaust fan management
- **Precision Control** - 0.39% PWM resolution (8-bit)
- **Network Resilience** - STA mode + AP fallback + auto-recovery
- **User Experience** - Intuitive interface with visual feedback

### 📱 **Modern Features**
- **Mobile Optimized** - Touch-friendly responsive interface
- **Debug Visibility** - Real-time JavaScript activity logging  
- **API Integration** - Ready for home automation systems
- **Professional Styling** - Modern blue gradient with smooth animations

## 📝 Board Specifications

**Target Hardware**: Board #1 from ESP32 collection
- ✅ **Verified Compatible** with standard 4-pin PC fans
- ✅ **Tested GPIO Performance** with oscilloscope verification  
- ✅ **Confirmed Network Connectivity** on HareNet WiFi
- ✅ **Validated Web Interface** on multiple browsers/devices

## 🚀 Ready for Production

This controller is **immediately deployable** for:
- **PC Case Fan Management** - Intake/exhaust coordination
- **Home Automation Integration** - RESTful API ready  
- **Cooling System Control** - Real-time feedback and adjustment
- **Educational Projects** - Well-documented, clean codebase
