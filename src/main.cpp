#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <driver/ledc.h>
#include <driver/pulse_cnt.h>
#include <esp_log.h>

// ====== CONFIGURATION FOR BOARD #1 ======
// Board #1 from catalog: ESP32 with MAC 44:1d:64:f5:b4:84
static const char* TAG = "ESP32_WEB_FAN_CONTROLLER";

// WiFi Configuration
const char* ssid = "HareNet";
const char* password = "01505336189";

// Fan Control GPIO Pins (from original project)
#define FAN_PWM_GPIO            2   // Fan PWM output pin
#define FAN_TACHO_GPIO          18  // Fan Tacho input pin

// PWM Configuration
#define PWM_FREQUENCY           25000  // 25kHz PWM frequency
#define PWM_RESOLUTION          8      // 8-bit resolution (0-255)
#define PWM_CHANNEL             0      // LEDC channel

// Global variables
AsyncWebServer server(80);
pcnt_unit_handle_t pcnt_unit = NULL;
volatile int current_rpm = 0;
volatile int current_pwm_percent = 0;
volatile unsigned long last_rpm_measurement = 0;

// ====== FAN PWM CONTROL ======
void init_fan_pwm() {
    ESP_LOGI(TAG, "💨 Initializing Fan PWM (GPIO %d)", FAN_PWM_GPIO);
    
    // Configure LED Control for PWM
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(FAN_PWM_GPIO, PWM_CHANNEL);
    
    // Start at 0% speed
    ledcWrite(PWM_CHANNEL, 0);
    
    ESP_LOGI(TAG, "✅ Fan PWM initialized (25kHz, 8-bit)");
}

void set_fan_speed(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    int duty_cycle = (percent * 255) / 100;  // Convert percentage to 8-bit value
    ledcWrite(PWM_CHANNEL, duty_cycle);
    
    current_pwm_percent = percent;
    ESP_LOGI(TAG, "🌪️ Fan speed set to %d%% (duty=%d)", percent, duty_cycle);
}

// ====== TACHO RPM MEASUREMENT ======
void init_tacho_input() {
    ESP_LOGI(TAG, "📡 Initializing Tacho input (GPIO %d)", FAN_TACHO_GPIO);
    
    // Configure GPIO as input with pull-up
    gpio_config_t tacho_gpio_config = {
        .pin_bit_mask = (1ULL << FAN_TACHO_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&tacho_gpio_config);
    
    // Create pulse counter unit
    pcnt_unit_config_t unit_config = {
        .high_limit = 10000,
        .low_limit = -10000,
    };
    pcnt_new_unit(&unit_config, &pcnt_unit);
    
    // Configure pulse counter channel
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = FAN_TACHO_GPIO,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t pcnt_channel = NULL;
    pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_channel);
    
    // Count on positive edge (tacho pulse)
    pcnt_channel_set_edge_action(pcnt_channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD);
    pcnt_channel_set_level_action(pcnt_channel, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);
    
    // Enable and start pulse counter
    pcnt_unit_enable(pcnt_unit);
    pcnt_unit_start(pcnt_unit);
    
    ESP_LOGI(TAG, "✅ Tacho measurement ready (2 pulses per revolution)");
}

void measure_rpm() {
    if (!pcnt_unit) return;
    
    static unsigned long last_measurement_time = 0;
    unsigned long current_time = millis();
    
    // Measure RPM every second
    if (current_time - last_measurement_time >= 1000) {
        int pulse_count = 0;
        pcnt_unit_get_count(pcnt_unit, &pulse_count);
        pcnt_unit_clear_count(pcnt_unit);
        
        // Convert pulse count to RPM (2 pulses per revolution, measured over 1 second)
        current_rpm = (pulse_count * 60) / 2;
        
        last_measurement_time = current_time;
        last_rpm_measurement = current_time;
        
        ESP_LOGI(TAG, "📊 Fan RPM: %d", current_rpm);
    }
}

// ====== WIFI CONNECTION ======
void init_wifi() {
    ESP_LOGI(TAG, "📶 Connecting to WiFi: %s", ssid);
    
    WiFi.begin(ssid, password);
    
    int connection_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && connection_attempts < 20) {
        delay(1000);
        connection_attempts++;
        ESP_LOGI(TAG, "📶 Connecting... attempt %d/20", connection_attempts);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "✅ WiFi Connected!");
        ESP_LOGI(TAG, "🌐 IP Address: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI(TAG, "📡 MAC Address: %s", WiFi.macAddress().c_str());
    } else {
        ESP_LOGE(TAG, "❌ WiFi Connection Failed!");
    }
}

// ====== WEB SERVER & API ======
String get_status_json() {
    DynamicJsonDocument doc(1024);
    
    doc["fan_speed"] = current_pwm_percent;
    doc["fan_rpm"] = current_rpm;
    doc["wifi_signal"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    doc["board_mac"] = WiFi.macAddress();
    
    String json_string;
    serializeJson(doc, json_string);
    return json_string;
}

void init_web_server() {
    ESP_LOGI(TAG, "🌐 Initializing Web Server...");
    
    // Serve main control page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Fan Controller - Board #1</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            margin: 0;
            padding: 20px;
            color: white;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 30px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
            backdrop-filter: blur(10px);
        }
        h1 {
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.2em;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);
        }
        .status-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin-bottom: 30px;
        }
        .status-card {
            background: rgba(255, 255, 255, 0.15);
            border-radius: 10px;
            padding: 20px;
            text-align: center;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        .status-value {
            font-size: 2.5em;
            font-weight: bold;
            margin: 10px 0;
            text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.5);
        }
        .status-label {
            font-size: 0.9em;
            opacity: 0.8;
        }
        .control-section {
            background: rgba(255, 255, 255, 0.15);
            border-radius: 10px;
            padding: 25px;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        .control-title {
            font-size: 1.3em;
            margin-bottom: 20px;
            text-align: center;
        }
        .speed-buttons {
            display: grid;
            grid-template-columns: repeat(5, 1fr);
            gap: 10px;
            margin-bottom: 20px;
        }
        .speed-btn {
            background: rgba(255, 255, 255, 0.2);
            border: 2px solid rgba(255, 255, 255, 0.3);
            color: white;
            padding: 15px 10px;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1em;
            font-weight: bold;
            transition: all 0.3s ease;
        }
        .speed-btn:hover {
            background: rgba(255, 255, 255, 0.3);
            transform: translateY(-2px);
        }
        .speed-btn.active {
            background: rgba(46, 204, 113, 0.8);
            border-color: #2ecc71;
        }
        .info-section {
            margin-top: 20px;
            padding: 20px;
            background: rgba(0, 0, 0, 0.2);
            border-radius: 10px;
            font-size: 0.9em;
        }
        .info-row {
            display: flex;
            justify-content: space-between;
            margin: 5px 0;
        }
        @media (max-width: 600px) {
            .status-grid {
                grid-template-columns: 1fr;
            }
            .speed-buttons {
                grid-template-columns: repeat(3, 1fr);
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌪️ ESP32 Fan Controller</h1>
        <div class="status-grid">
            <div class="status-card">
                <div class="status-value" id="rpm-display">0</div>
                <div class="status-label">Fan RPM</div>
            </div>
            <div class="status-card">
                <div class="status-value" id="speed-display">0%</div>
                <div class="status-label">PWM Speed</div>
            </div>
        </div>
        
        <div class="control-section">
            <div class="control-title">🎛️ Fan Speed Control</div>
            <div class="speed-buttons">
                <button class="speed-btn" onclick="setFanSpeed(0)">0%</button>
                <button class="speed-btn" onclick="setFanSpeed(10)">10%</button>
                <button class="speed-btn" onclick="setFanSpeed(20)">20%</button>
                <button class="speed-btn" onclick="setFanSpeed(30)">30%</button>
                <button class="speed-btn" onclick="setFanSpeed(40)">40%</button>
                <button class="speed-btn" onclick="setFanSpeed(50)">50%</button>
                <button class="speed-btn" onclick="setFanSpeed(60)">60%</button>
                <button class="speed-btn" onclick="setFanSpeed(70)">70%</button>
                <button class="speed-btn" onclick="setFanSpeed(80)">80%</button>
                <button class="speed-btn" onclick="setFanSpeed(90)">90%</button>
                <button class="speed-btn" onclick="setFanSpeed(100)">100%</button>
            </div>
        </div>
        
        <div class="info-section">
            <div class="info-row">
                <span>📱 Board:</span>
                <span id="board-mac">Loading...</span>
            </div>
            <div class="info-row">
                <span>📶 WiFi Signal:</span>
                <span id="wifi-signal">Loading...</span>
            </div>
            <div class="info-row">
                <span>⏰ Uptime:</span>
                <span id="uptime">Loading...</span>
            </div>
        </div>
    </div>

    <script>
        let currentSpeed = 0;
        
        function setFanSpeed(speed) {
            fetch('/set_speed', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({speed: speed})
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    updateSpeedButtons(speed);
                    updateStatus();
                }
            })
            .catch(error => console.error('Error:', error));
        }
        
        function updateSpeedButtons(activeSpeed) {
            currentSpeed = activeSpeed;
            document.querySelectorAll('.speed-btn').forEach(btn => {
                btn.classList.remove('active');
            });
            
            const speeds = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100];
            const index = speeds.indexOf(activeSpeed);
            if (index >= 0) {
                document.querySelectorAll('.speed-btn')[index].classList.add('active');
            }
        }
        
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('rpm-display').textContent = data.fan_rpm;
                    document.getElementById('speed-display').textContent = data.fan_speed + '%';
                    document.getElementById('board-mac').textContent = data.board_mac;
                    document.getElementById('wifi-signal').textContent = data.wifi_signal + ' dBm';
                    document.getElementById('uptime').textContent = formatUptime(data.uptime);
                    
                    updateSpeedButtons(data.fan_speed);
                })
                .catch(error => console.error('Error:', error));
        }
        
        function formatUptime(seconds) {
            const hours = Math.floor(seconds / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = seconds % 60;
            return `${hours}h ${minutes}m ${secs}s`;
        }
        
        // Update status every 2 seconds
        setInterval(updateStatus, 2000);
        
        // Initial load
        updateStatus();
    </script>
</body>
</html>
        )";
        
        request->send(200, "text/html", html);
    });
    
    // API endpoint to get current status
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", get_status_json());
    });
    
    // API endpoint to set fan speed
    server.on("/set_speed", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, data);
            
            int speed = doc["speed"];
            set_fan_speed(speed);
            
            DynamicJsonDocument response(256);
            response["success"] = true;
            response["speed"] = speed;
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });
    
    server.begin();
    ESP_LOGI(TAG, "✅ Web Server started on http://%s", WiFi.localIP().toString().c_str());
}

// ====== MAIN SETUP AND LOOP ======
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    ESP_LOGI(TAG, "🎯 ESP32 Web Fan Controller Starting...");
    ESP_LOGI(TAG, "📱 Board #1 - MAC: Expected 44:1d:64:f5:b4:84");
    ESP_LOGI(TAG, "🌪️ Fan PWM: GPIO %d", FAN_PWM_GPIO);
    ESP_LOGI(TAG, "📡 Fan Tacho: GPIO %d", FAN_TACHO_GPIO);
    ESP_LOGI(TAG, "");
    
    // Initialize hardware
    init_fan_pwm();
    init_tacho_input();
    
    // Connect to WiFi
    init_wifi();
    
    // Start web server
    if (WiFi.status() == WL_CONNECTED) {
        init_web_server();
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "✅ System Ready!");
        ESP_LOGI(TAG, "🌐 Web Interface: http://%s", WiFi.localIP().toString().c_str());
        ESP_LOGI(TAG, "🎛️ Fan Control: Use web interface for 0-100%% control");
        ESP_LOGI(TAG, "📊 RPM Monitoring: Real-time tacho feedback");
        ESP_LOGI(TAG, "");
    } else {
        ESP_LOGE(TAG, "❌ System Error: No WiFi connection");
    }
}

void loop() {
    // Measure RPM continuously
    measure_rpm();
    
    // Small delay to prevent overwhelming the system
    delay(100);
}
