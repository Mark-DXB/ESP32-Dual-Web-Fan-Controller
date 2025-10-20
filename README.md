# 🌪️ ESP32 Web Fan Controller

A web-based PC fan controller for ESP32 Board #1 with real-time RPM monitoring and PWM speed control.

## 🎯 Features

- **Web-based Control Interface** - Beautiful, responsive web UI
- **Real-time RPM Monitoring** - Live tacho signal reading
- **PWM Speed Control** - 0-100% in 10% increments
- **WiFi Connectivity** - Connect to home network
- **Board Identification** - Designed for Board #1 (MAC: 44:1d:64:f5:b4:84)

## 🔌 Hardware Configuration

### Board #1 Specifications
- **Chip**: ESP32 (Original)
- **MAC Address**: 44:1d:64:f5:b4:84
- **Flash**: 4MB
- **USB Chip**: CP2102

### GPIO Pin Assignment
```
GPIO 2  - Fan PWM Output (25kHz)
GPIO 18 - Fan Tacho Input (with pull-up)
```

### Fan Connection
```
Fan Connector (4-pin):
Pin 1: Ground (Black)
Pin 2: +12V Power (Red) 
Pin 3: Tacho Signal (Yellow) → GPIO 18
Pin 4: PWM Control (Blue) → GPIO 2
```

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

The web interface provides:

- **Real-time RPM Display** - Current fan speed
- **PWM Control Buttons** - 0%, 10%, 20%, ... 100%
- **System Information** - Board MAC, WiFi signal, uptime
- **Responsive Design** - Works on desktop and mobile

## 📊 API Endpoints

### Get Status
```http
GET /status
```
Returns JSON with current fan status, RPM, and system info.

### Set Fan Speed
```http
POST /set_speed
Content-Type: application/json

{
  "speed": 50
}
```
Sets fan speed (0-100%).

## 🔧 Technical Details

### PWM Configuration
- **Frequency**: 25kHz (silent operation)
- **Resolution**: 8-bit (256 steps)
- **Channel**: LEDC Channel 0

### RPM Measurement
- **Method**: Pulse counting on tacho signal
- **Update Rate**: Every 1 second
- **Calculation**: (pulses × 60) ÷ 2 pulses/revolution

### WiFi Features
- **Auto-connect** to HareNet network
- **Connection retry** with timeout
- **Signal strength monitoring**

## 🛠️ Development

### Project Structure
```
ESP32_Web_Fan_Controller/
├── src/
│   └── main.cpp          # Main application code
├── platformio.ini        # PlatformIO configuration
└── README.md            # This file
```

### Dependencies
- ESP Async WebServer
- ESPAsyncTCP  
- ArduinoJson

### Build Configuration
- **Platform**: ESP32 (Arduino framework)
- **Board**: esp32dev
- **Monitor Speed**: 115200 baud

## 🎯 Usage Examples

### Basic Fan Control
1. Access web interface at ESP32's IP address
2. Click speed buttons (0% to 100%)
3. Monitor real-time RPM feedback

### API Control
```bash
# Set fan to 75%
curl -X POST http://[ESP32_IP]/set_speed \
  -H "Content-Type: application/json" \
  -d '{"speed":75}'

# Get current status  
curl http://[ESP32_IP]/status
```

## 📈 Monitoring

The system provides real-time monitoring of:
- **Fan RPM** - Actual measured speed
- **PWM Setting** - Current drive percentage  
- **WiFi Signal** - Connection quality
- **System Uptime** - Runtime statistics

## 🔒 Safety Features

- **Input validation** on speed settings
- **Connection monitoring** with retry logic
- **GPIO pull-up resistors** for reliable tacho reading
- **Non-blocking operations** to prevent system lockup

## 📝 Notes

- Designed specifically for Board #1 from your ESP32 collection
- Optimized for standard 4-pin PC fans
- Web interface auto-updates every 2 seconds
- Compatible with most modern web browsers

## 🚀 Future Enhancements

- Multiple fan support
- Temperature-based automatic control
- Historical data logging
- Mobile app integration
- MQTT connectivity
