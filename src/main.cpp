#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// ====== CONFIGURATION FOR BOARD #1 ======
// Board #1 from catalog: ESP32 with MAC 44:1d:64:f5:b4:84

// WiFi Configuration
const char* ssid = "HareNet";
const char* password = "01505336189";

// AP Mode Fallback Configuration
const char* ap_ssid = "ESP32-FanController";
const char* ap_password = "fancontrol123";
bool ap_mode_active = false;

// Fan Control GPIO Pins (from original project)
#define FAN_PWM_GPIO            2   // Fan PWM output pin
#define FAN_TACHO_GPIO          18  // Fan Tacho input pin

// PWM Configuration
#define PWM_FREQUENCY           25000  // 25kHz PWM frequency
#define PWM_RESOLUTION          8      // 8-bit resolution (0-255)
#define PWM_CHANNEL             0      // LEDC channel

// Global variables
AsyncWebServer server(80);
volatile int pulse_count = 0;
volatile int current_rpm = 0;
volatile int current_pwm_percent = 0;
volatile unsigned long last_rpm_measurement = 0;

// Interrupt handler for tacho pulses
void IRAM_ATTR tacho_pulse_isr() {
    pulse_count++;
}

// ====== FAN PWM CONTROL ======
void init_fan_pwm() {
    Serial.printf("Fan PWM initializing (GPIO %d)\n", FAN_PWM_GPIO);
    
    // Configure LED Control for PWM
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(FAN_PWM_GPIO, PWM_CHANNEL);
    
    // Start at 0% speed
    ledcWrite(PWM_CHANNEL, 0);
    
    Serial.println("Fan PWM initialized (25kHz, 8-bit)");
}

void set_fan_speed(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    int duty_cycle = (percent * 255) / 100;  // Convert percentage to 8-bit value
    ledcWrite(PWM_CHANNEL, duty_cycle);
    
    current_pwm_percent = percent;
    Serial.printf("Fan speed set to %d%% (duty=%d)\n", percent, duty_cycle);
}

// ====== TACHO RPM MEASUREMENT ======
void init_tacho_input() {
    Serial.printf("Tacho input initializing (GPIO %d)\n", FAN_TACHO_GPIO);
    
    // Configure GPIO as input with pull-up
    pinMode(FAN_TACHO_GPIO, INPUT_PULLUP);
    
    // Attach interrupt for rising edge (tacho pulse)
    attachInterrupt(digitalPinToInterrupt(FAN_TACHO_GPIO), tacho_pulse_isr, RISING);
    
    Serial.println("Tacho measurement ready (2 pulses per revolution)");
}

void measure_rpm() {
    static unsigned long last_measurement_time = 0;
    unsigned long current_time = millis();
    
    // Measure RPM every second
    if (current_time - last_measurement_time >= 1000) {
        // Get pulse count and reset counter
        noInterrupts();
        int pulses = pulse_count;
        pulse_count = 0;
        interrupts();
        
        // Convert pulse count to RPM (2 pulses per revolution, measured over 1 second)
        current_rpm = (pulses * 60) / 2;
        
        last_measurement_time = current_time;
        last_rpm_measurement = current_time;
        
        Serial.printf("Fan RPM: %d\n", current_rpm);
    }
}

// ====== WIFI CONNECTION WITH AP FALLBACK ======
void init_wifi() {
    Serial.printf("Attempting to connect to WiFi: %s\n", ssid);
    
    // Try to connect to home network first
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int connection_attempts = 0;
    while (WiFi.status() != WL_CONNECTED && connection_attempts < 15) {
        delay(1000);
        connection_attempts++;
        Serial.printf("Connecting to %s... attempt %d/15\n", ssid, connection_attempts);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        // Successfully connected to home network
        ap_mode_active = false;
        Serial.println("=== WiFi Station Mode Connected ===");
        Serial.printf("Network: %s\n", ssid);
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    } else {
        // Failed to connect - switch to AP mode
        Serial.println("=== WiFi Connection Failed - Starting AP Mode ===");
        
        WiFi.mode(WIFI_AP);
        bool ap_success = WiFi.softAP(ap_ssid, ap_password);
        
        if (ap_success) {
            ap_mode_active = true;
            IPAddress ap_ip = WiFi.softAPIP();
            Serial.println("Access Point Mode Active!");
            Serial.printf("AP Network: %s\n", ap_ssid);
            Serial.printf("AP Password: %s\n", ap_password);
            Serial.printf("AP IP Address: %s\n", ap_ip.toString().c_str());
            Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
            Serial.println("Connect your device to this network to access the fan controller");
        } else {
            Serial.println("Failed to start Access Point mode!");
        }
    }
}

// ====== WEB SERVER & API ======
String get_status_json() {
    JsonDocument doc;
    
    doc["fan_speed"] = current_pwm_percent;
    doc["fan_rpm"] = current_rpm;
    doc["uptime"] = millis() / 1000;
    doc["board_mac"] = WiFi.macAddress();
    
    // Add WiFi mode information
    if (ap_mode_active) {
        doc["wifi_mode"] = "Access Point";
        doc["wifi_network"] = ap_ssid;
        doc["wifi_signal"] = "N/A";
        doc["ip_address"] = WiFi.softAPIP().toString();
        doc["connected_clients"] = WiFi.softAPgetStationNum();
    } else {
        doc["wifi_mode"] = "Station";
        doc["wifi_network"] = ssid;
        doc["wifi_signal"] = WiFi.RSSI();
        doc["ip_address"] = WiFi.localIP().toString();
        doc["connected_clients"] = "N/A";
    }
    
    String json_string;
    serializeJson(doc, json_string);
    return json_string;
}

void init_web_server() {
    Serial.println("Initializing Web Server...");
    
    // Serve main control page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>ESP32 Fan Controller - Board #1</title><style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1e3c72 0%,#2a5298 100%);margin:0;padding:20px;color:white}.container{max-width:600px;margin:0 auto;background:rgba(255,255,255,0.1);border-radius:15px;padding:30px;box-shadow:0 8px 32px rgba(0,0,0,0.3)}h1{text-align:center;margin-bottom:30px;font-size:2.2em}.wifi-status{text-align:center;margin-bottom:20px;padding:10px;border-radius:8px;font-weight:bold}.wifi-station{background:rgba(0,255,0,0.3)}.wifi-ap{background:rgba(255,165,0,0.3)}.status-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin-bottom:30px}.status-card{background:rgba(255,255,255,0.15);border-radius:10px;padding:20px;text-align:center}.status-value{font-size:2.5em;font-weight:bold;margin:10px 0}.status-label{font-size:0.9em;opacity:0.8}.control-section{background:rgba(255,255,255,0.15);border-radius:10px;padding:25px}.control-title{font-size:1.3em;margin-bottom:20px;text-align:center}.speed-buttons{display:grid;grid-template-columns:repeat(5,1fr);gap:10px;margin-bottom:20px}.speed-btn{background:rgba(255,255,255,0.2);border:2px solid rgba(255,255,255,0.3);color:white;padding:15px 10px;border-radius:8px;cursor:pointer;font-size:1em;font-weight:bold}.speed-btn:hover{background:rgba(255,255,255,0.3)}.speed-btn.active{background:rgba(46,204,113,0.8);border-color:#2ecc71}.info-section{margin-top:20px;padding:20px;background:rgba(0,0,0,0.2);border-radius:10px;font-size:0.9em}.info-row{display:flex;justify-content:space-between;margin:5px 0}</style></head><body><div class=\"container\"><h1>ESP32 Fan Controller</h1><div id=\"wifi-status\" class=\"wifi-status\">Loading WiFi Status...</div><div class=\"status-grid\"><div class=\"status-card\"><div class=\"status-value\" id=\"rpm-display\">0</div><div class=\"status-label\">Fan RPM</div></div><div class=\"status-card\"><div class=\"status-value\" id=\"speed-display\">0%</div><div class=\"status-label\">PWM Speed</div></div></div><div class=\"control-section\"><div class=\"control-title\">Fan Speed Control</div><div class=\"speed-buttons\"><button class=\"speed-btn\" onclick=\"setFanSpeed(0)\">0%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(10)\">10%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(20)\">20%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(30)\">30%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(40)\">40%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(50)\">50%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(60)\">60%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(70)\">70%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(80)\">80%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(90)\">90%</button><button class=\"speed-btn\" onclick=\"setFanSpeed(100)\">100%</button></div></div><div class=\"info-section\"><div class=\"info-row\"><span>Board MAC:</span><span id=\"board-mac\">Loading...</span></div><div class=\"info-row\"><span>WiFi Mode:</span><span id=\"wifi-mode\">Loading...</span></div><div class=\"info-row\"><span>Network:</span><span id=\"wifi-network\">Loading...</span></div><div class=\"info-row\"><span>IP Address:</span><span id=\"ip-address\">Loading...</span></div><div class=\"info-row\"><span>Signal/Clients:</span><span id=\"wifi-signal\">Loading...</span></div><div class=\"info-row\"><span>Uptime:</span><span id=\"uptime\">Loading...</span></div></div></div><script>let currentSpeed=0;function setFanSpeed(speed){fetch('/set_speed',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({speed:speed})}).then(response=>response.json()).then(data=>{if(data.success){updateSpeedButtons(speed);updateStatus()}}).catch(error=>console.error('Error:',error))}function updateSpeedButtons(activeSpeed){currentSpeed=activeSpeed;document.querySelectorAll('.speed-btn').forEach(btn=>{btn.classList.remove('active')});const speeds=[0,10,20,30,40,50,60,70,80,90,100];const index=speeds.indexOf(activeSpeed);if(index>=0){document.querySelectorAll('.speed-btn')[index].classList.add('active')}}function updateStatus(){fetch('/status').then(response=>response.json()).then(data=>{document.getElementById('rpm-display').textContent=data.fan_rpm;document.getElementById('speed-display').textContent=data.fan_speed+'%';document.getElementById('board-mac').textContent=data.board_mac;document.getElementById('wifi-mode').textContent=data.wifi_mode;document.getElementById('wifi-network').textContent=data.wifi_network;document.getElementById('ip-address').textContent=data.ip_address;document.getElementById('uptime').textContent=formatUptime(data.uptime);updateSpeedButtons(data.fan_speed);const wifiStatus=document.getElementById('wifi-status');if(data.wifi_mode==='Access Point'){wifiStatus.textContent='AP Mode: Connect to \"'+data.wifi_network+'\" (Password: fancontrol123)';wifiStatus.className='wifi-status wifi-ap';document.getElementById('wifi-signal').textContent=data.connected_clients+' clients'}else{wifiStatus.textContent='Connected to: '+data.wifi_network;wifiStatus.className='wifi-status wifi-station';document.getElementById('wifi-signal').textContent=data.wifi_signal+' dBm'}}).catch(error=>console.error('Error:',error))}function formatUptime(seconds){const hours=Math.floor(seconds/3600);const minutes=Math.floor((seconds%3600)/60);const secs=seconds%60;return hours+'h '+minutes+'m '+secs+'s'}setInterval(updateStatus,2000);updateStatus()</script></body></html>";
        
        request->send(200, "text/html", html);
    });
    
    // API endpoint to get current status
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", get_status_json());
    });
    
    // API endpoint to set fan speed
    server.on("/set_speed", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            int speed = doc["speed"];
            set_fan_speed(speed);
            
            JsonDocument response;
            response["success"] = true;
            response["speed"] = speed;
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });
    
    server.begin();
    Serial.printf("Web Server started on http://%s\n", WiFi.localIP().toString().c_str());
}

// ====== MAIN SETUP AND LOOP ======
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 Web Fan Controller Starting...");
    Serial.println("Board #1 - MAC: Expected 44:1d:64:f5:b4:84");
    Serial.printf("Fan PWM: GPIO %d\n", FAN_PWM_GPIO);
    Serial.printf("Fan Tacho: GPIO %d\n", FAN_TACHO_GPIO);
    Serial.println();
    
    // Initialize hardware
    init_fan_pwm();
    init_tacho_input();
    
    // Connect to WiFi
    init_wifi();
    
    // Start web server (works in both Station and AP modes)
    if (WiFi.status() == WL_CONNECTED || ap_mode_active) {
        init_web_server();
        Serial.println();
        Serial.println("=== System Ready! ===");
        
        if (ap_mode_active) {
            Serial.printf("Web Interface: http://%s\n", WiFi.softAPIP().toString().c_str());
            Serial.printf("Connect to WiFi: %s (Password: %s)\n", ap_ssid, ap_password);
        } else {
            Serial.printf("Web Interface: http://%s\n", WiFi.localIP().toString().c_str());
        }
        
        Serial.println("Fan Control: Use web interface for 0-100% control");
        Serial.println("RPM Monitoring: Real-time tacho feedback");
        Serial.println();
    } else {
        Serial.println("System Error: No WiFi connection available");
    }
}

void loop() {
    // Measure RPM continuously
    measure_rpm();
    
    // Small delay to prevent overwhelming the system
    delay(100);
}