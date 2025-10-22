#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// ====== CONFIGURATION FOR BOARD #1 ======
// Board #1 from catalog: ESP32 with MAC 44:1d:64:f5:b4:84

// WiFi Configuration
const char* ssid = "HareNet";
const char* password = "01505336189";

// AP Mode Fallback Configuration
const char* ap_ssid = "ESP32-FanController";
const char* ap_password = "fancontrol123";
bool ap_mode_active = false;

// Dual Fan Control GPIO Pins (Updated for reliable PWM)
#define FAN1_PWM_GPIO           5   // Fan 1 (Intake) PWM output pin (GPIO 5 - safe for PWM)
#define FAN1_TACHO_GPIO         18  // Fan 1 (Intake) Tacho input pin
#define FAN2_PWM_GPIO           21  // Fan 2 (Exhaust) PWM output pin (GPIO 21 - safe for PWM)
#define FAN2_TACHO_GPIO         19  // Fan 2 (Exhaust) Tacho input pin

// PWM Configuration
#define PWM_FREQUENCY           25000  // 25kHz PWM frequency
#define PWM_RESOLUTION          8      // 8-bit resolution (0-255)
#define FAN1_PWM_CHANNEL        0      // LEDC channel for Fan 1
#define FAN2_PWM_CHANNEL        1      // LEDC channel for Fan 2

// Tachometer Configuration (Measured Values)
// Standard assumption: 2 pulses per revolution
// Actual measurement: Fan max 2300 RPM produces 70 Hz tachometer signal
// Calculated PPR: 70 Hz / (2300 RPM / 60) = 1.83 pulses per revolution
#define PULSES_PER_REVOLUTION   1.83   // Measured value for accurate RPM calculation

// Interrupt Debouncing Configuration
// Minimum time between valid interrupts: 500µs (prevents signal bouncing)
// Maximum theoretical frequency: 2kHz (well above typical fan speeds)
#define DEBOUNCE_MICROS         500    // Microseconds between valid interrupts

// ARGB LED Configuration (Confirmed 3-wire ARGB fans)
#define FAN1_ARGB_GPIO          16     // Fan 1 ARGB data pin
#define FAN2_ARGB_GPIO          17     // Fan 2 ARGB data pin  
#define LEDS_PER_FAN            12     // Estimated LED count per fan (adjust based on actual count)
#define ARGB_BRIGHTNESS         50     // Default brightness (0-255)
#define ARGB_CHIPSET            WS2812B // Standard ARGB chipset
#define ARGB_COLOR_ORDER        GRB     // Standard color order for WS2812B

// Global variables
AsyncWebServer server(80);

// Fan 1 (Intake) variables
volatile int fan1_pulse_count = 0;
volatile int fan1_current_rpm = 0;
volatile int fan1_current_pwm_percent = 0;
volatile unsigned long fan1_last_rpm_measurement = 0;

// Fan 2 (Exhaust) variables  
volatile int fan2_pulse_count = 0;
volatile int fan2_current_rpm = 0;
volatile int fan2_current_pwm_percent = 0;
volatile unsigned long fan2_last_rpm_measurement = 0;

// Debugging variables for interrupt analysis
volatile unsigned long fan1_last_interrupt_time = 0;
volatile unsigned long fan2_last_interrupt_time = 0;
volatile int fan1_bounce_count = 0;
volatile int fan2_bounce_count = 0;

// ARGB LED Arrays and Control Variables
CRGB fan1_leds[LEDS_PER_FAN];
CRGB fan2_leds[LEDS_PER_FAN];

// ARGB Control Variables  
uint8_t fan1_red = 0, fan1_green = 100, fan1_blue = 255;     // Default: Light blue
uint8_t fan2_red = 255, fan2_green = 100, fan2_blue = 0;     // Default: Orange
uint8_t fan1_brightness = ARGB_BRIGHTNESS;
uint8_t fan2_brightness = ARGB_BRIGHTNESS;
uint8_t argb_effect = 0;  // 0=solid, 1=breathing, 2=rainbow, etc.

// Forward declarations for ARGB functions
void set_fan1_color(uint8_t red, uint8_t green, uint8_t blue);
void set_fan2_color(uint8_t red, uint8_t green, uint8_t blue);
void set_fan1_brightness(uint8_t brightness);
void set_fan2_brightness(uint8_t brightness);
void update_argb_leds();

// Interrupt handlers for tacho pulses with debouncing
void IRAM_ATTR fan1_tacho_isr() {
    unsigned long current_time = micros();
    
    // Debounce: Ignore interrupts within DEBOUNCE_MICROS (prevents bouncing)
    if (current_time - fan1_last_interrupt_time > DEBOUNCE_MICROS) {
        fan1_pulse_count++;
        fan1_last_interrupt_time = current_time;
    } else {
        fan1_bounce_count++;  // Count bounced/ignored pulses
    }
}

void IRAM_ATTR fan2_tacho_isr() {
    unsigned long current_time = micros();
    
    // Debounce: Ignore interrupts within DEBOUNCE_MICROS (prevents bouncing)
    if (current_time - fan2_last_interrupt_time > DEBOUNCE_MICROS) {
        fan2_pulse_count++;
        fan2_last_interrupt_time = current_time;
    } else {
        fan2_bounce_count++;  // Count bounced/ignored pulses
    }
}

// ====== DUAL FAN PWM CONTROL ======
void init_fan_pwm() {
    Serial.println("Initializing Dual Fan PWM Control...");
    
    // Fan 1 (Intake) PWM setup
    Serial.printf("Fan 1 (Intake) PWM: GPIO %d, Channel %d\n", FAN1_PWM_GPIO, FAN1_PWM_CHANNEL);
    ledcSetup(FAN1_PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(FAN1_PWM_GPIO, FAN1_PWM_CHANNEL);
    ledcWrite(FAN1_PWM_CHANNEL, 0);  // Start at 0%
    
    // Fan 2 (Exhaust) PWM setup  
    Serial.printf("Fan 2 (Exhaust) PWM: GPIO %d, Channel %d\n", FAN2_PWM_GPIO, FAN2_PWM_CHANNEL);
    ledcSetup(FAN2_PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(FAN2_PWM_GPIO, FAN2_PWM_CHANNEL);
    ledcWrite(FAN2_PWM_CHANNEL, 0);  // Start at 0%
    
    Serial.println("Dual Fan PWM initialized (25kHz, 8-bit)");
}

void set_fan1_speed(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    int duty_cycle = (percent * 255) / 100;
    
    Serial.printf("🔧 Setting Fan 1 PWM: %d%% -> duty=%d on GPIO %d, Channel %d\n", 
                  percent, duty_cycle, FAN1_PWM_GPIO, FAN1_PWM_CHANNEL);
    
    ledcWrite(FAN1_PWM_CHANNEL, duty_cycle);
    
    fan1_current_pwm_percent = percent;
    Serial.printf("✅ Fan 1 PWM set successfully\n");
}

void set_fan2_speed(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    int duty_cycle = (percent * 255) / 100;
    
    Serial.printf("🔧 Setting Fan 2 PWM: %d%% -> duty=%d on GPIO %d, Channel %d\n", 
                  percent, duty_cycle, FAN2_PWM_GPIO, FAN2_PWM_CHANNEL);
    
    ledcWrite(FAN2_PWM_CHANNEL, duty_cycle);
    
    fan2_current_pwm_percent = percent;
    Serial.printf("✅ Fan 2 PWM set successfully\n");
}

// ====== DUAL TACHO RPM MEASUREMENT ======
void init_tacho_input() {
    Serial.println("Initializing Dual Tacho inputs...");
    
    // Fan 1 (Intake) Tacho setup
    Serial.printf("Fan 1 (Intake) Tacho: GPIO %d\n", FAN1_TACHO_GPIO);
    pinMode(FAN1_TACHO_GPIO, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FAN1_TACHO_GPIO), fan1_tacho_isr, RISING);
    
    // Fan 2 (Exhaust) Tacho setup
    Serial.printf("Fan 2 (Exhaust) Tacho: GPIO %d\n", FAN2_TACHO_GPIO);
    pinMode(FAN2_TACHO_GPIO, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FAN2_TACHO_GPIO), fan2_tacho_isr, RISING);
    
    Serial.printf("Dual Tacho measurement ready (%.2f pulses per revolution - measured)\n", PULSES_PER_REVOLUTION);
    Serial.printf("Debouncing enabled: %dµs minimum pulse width (%.1f kHz max frequency)\n", 
                  DEBOUNCE_MICROS, 1000.0 / DEBOUNCE_MICROS);
}

// ====== ARGB LED CONTROL ======
void init_argb() {
    Serial.println("Initializing ARGB LED control...");
    
    // Initialize FastLED for both fans
    FastLED.addLeds<ARGB_CHIPSET, FAN1_ARGB_GPIO, ARGB_COLOR_ORDER>(fan1_leds, LEDS_PER_FAN);
    FastLED.addLeds<ARGB_CHIPSET, FAN2_ARGB_GPIO, ARGB_COLOR_ORDER>(fan2_leds, LEDS_PER_FAN);
    
    // Set global brightness
    FastLED.setBrightness(ARGB_BRIGHTNESS);
    
    // Set initial colors (different for each fan for easy identification)
    set_fan1_color(fan1_red, fan1_green, fan1_blue);
    set_fan2_color(fan2_red, fan2_green, fan2_blue);
    
    Serial.printf("ARGB initialized: GPIO %d (Fan1), GPIO %d (Fan2), %d LEDs per fan\n", 
                  FAN1_ARGB_GPIO, FAN2_ARGB_GPIO, LEDS_PER_FAN);
}

void set_fan1_color(uint8_t red, uint8_t green, uint8_t blue) {
    fan1_red = red;
    fan1_green = green; 
    fan1_blue = blue;
    
    for (int i = 0; i < LEDS_PER_FAN; i++) {
        fan1_leds[i] = CRGB(red, green, blue);
    }
    
    Serial.printf("Fan 1 ARGB: RGB(%d,%d,%d)\n", red, green, blue);
}

void set_fan2_color(uint8_t red, uint8_t green, uint8_t blue) {
    fan2_red = red;
    fan2_green = green;
    fan2_blue = blue;
    
    for (int i = 0; i < LEDS_PER_FAN; i++) {
        fan2_leds[i] = CRGB(red, green, blue);
    }
    
    Serial.printf("Fan 2 ARGB: RGB(%d,%d,%d)\n", red, green, blue);
}

void set_fan1_brightness(uint8_t brightness) {
    fan1_brightness = brightness;
    // Apply brightness by scaling colors
    for (int i = 0; i < LEDS_PER_FAN; i++) {
        fan1_leds[i] = CRGB((fan1_red * brightness) / 255, 
                           (fan1_green * brightness) / 255, 
                           (fan1_blue * brightness) / 255);
    }
    Serial.printf("Fan 1 brightness: %d%%\n", (brightness * 100) / 255);
}

void set_fan2_brightness(uint8_t brightness) {
    fan2_brightness = brightness;
    // Apply brightness by scaling colors
    for (int i = 0; i < LEDS_PER_FAN; i++) {
        fan2_leds[i] = CRGB((fan2_red * brightness) / 255, 
                           (fan2_green * brightness) / 255, 
                           (fan2_blue * brightness) / 255);
    }
    Serial.printf("Fan 2 brightness: %d%%\n", (brightness * 100) / 255);
}

void update_argb_leds() {
    // Update LED effects based on current mode
    if (argb_effect == 1) {
        // Breathing effect
        static uint8_t breathing_value = 0;
        static int8_t breathing_direction = 1;
        
        breathing_value += breathing_direction * 2;
        if (breathing_value >= 255 || breathing_value <= 50) {
            breathing_direction *= -1;
        }
        
        set_fan1_brightness(breathing_value);
        set_fan2_brightness(breathing_value);
        
    } else if (argb_effect == 2) {
        // Rainbow effect
        static uint8_t rainbow_hue = 0;
        rainbow_hue += 2;
        
        for (int i = 0; i < LEDS_PER_FAN; i++) {
            fan1_leds[i] = CHSV(rainbow_hue + (i * 20), 255, fan1_brightness);
            fan2_leds[i] = CHSV(rainbow_hue + (i * 20) + 128, 255, fan2_brightness); // Offset for variety
        }
    }
    // Effect 0 (solid) is handled by set_fanX_color functions
    
    FastLED.show();
}

void measure_rpm() {
    unsigned long current_time = millis();
    
    // Measure Fan 1 RPM every second
    if (current_time - fan1_last_rpm_measurement >= 1000) {
        noInterrupts();
        int fan1_pulses = fan1_pulse_count;
        fan1_pulse_count = 0;
        interrupts();
        
        fan1_current_rpm = (fan1_pulses * 60) / PULSES_PER_REVOLUTION;  // Measured PPR for accurate calculation
        fan1_last_rpm_measurement = current_time;
        
        // Debug output with bounce detection
        noInterrupts();
        int fan1_bounces = fan1_bounce_count;
        fan1_bounce_count = 0;  // Reset bounce counter
        interrupts();
        
        Serial.printf("Fan 1 (GPIO 18) RPM: %d | Pulses: %d | Bounces: %d | Freq: %.1f Hz\n", 
                      fan1_current_rpm, fan1_pulses, fan1_bounces, fan1_pulses / 1.0);
    }
    
    // Measure Fan 2 RPM every second  
    if (current_time - fan2_last_rpm_measurement >= 1000) {
        noInterrupts();
        int fan2_pulses = fan2_pulse_count;
        fan2_pulse_count = 0;
        interrupts();
        
        fan2_current_rpm = (fan2_pulses * 60) / PULSES_PER_REVOLUTION;  // Measured PPR for accurate calculation
        fan2_last_rpm_measurement = current_time;
        
        // Debug output with bounce detection
        noInterrupts();
        int fan2_bounces = fan2_bounce_count;
        fan2_bounce_count = 0;  // Reset bounce counter
        interrupts();
        
        Serial.printf("Fan 2 (GPIO 19) RPM: %d | Pulses: %d | Bounces: %d | Freq: %.1f Hz\n", 
                      fan2_current_rpm, fan2_pulses, fan2_bounces, fan2_pulses / 1.0);
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
    
    // Dual Fan Status
    doc["fan1_speed"] = fan1_current_pwm_percent;
    doc["fan1_rpm"] = fan1_current_rpm;
    doc["fan2_speed"] = fan2_current_pwm_percent; 
    doc["fan2_rpm"] = fan2_current_rpm;
    
    // System Info
    doc["uptime"] = millis() / 1000;
    doc["board_mac"] = WiFi.macAddress();
    
    // ARGB LED Status
    doc["fan1_red"] = fan1_red;
    doc["fan1_green"] = fan1_green;
    doc["fan1_blue"] = fan1_blue;
    doc["fan1_brightness"] = (fan1_brightness * 100) / 255;  // Convert to percentage
    doc["fan2_red"] = fan2_red;
    doc["fan2_green"] = fan2_green;
    doc["fan2_blue"] = fan2_blue;
    doc["fan2_brightness"] = (fan2_brightness * 100) / 255;  // Convert to percentage
    doc["argb_effect"] = argb_effect;
    
    // Legacy single fan fields (for backward compatibility)
    doc["fan_speed"] = fan1_current_pwm_percent;  // Default to Fan 1
    doc["fan_rpm"] = fan1_current_rpm;           // Default to Fan 1
    
    // WiFi mode information
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
        String html = "<!DOCTYPE html> <html> <head> <title>ESP32 Fan Controller</title> <style>body{font-family:Arial,sans-serif;background:#1e3c72;color:white;padding:20px}.container{max-width:700px;margin:0 auto;background:rgba(255,255,255,0.1);border-radius:10px;padding:20px}h1{text-align:center}#debug{background:rgba(255,0,0,0.3);padding:10px;margin:10px 0;border-radius:5px;font-size:12px}#status{background:rgba(0,0,0,0.3);padding:15px;margin:10px 0;border-radius:5px}.fan{background:rgba(255,255,255,0.1);margin:10px 0;padding:15px;border-radius:8px}.fan h3{margin:0 0 10px 0}.info{display:flex;justify-content:space-between;margin:5px 0}.buttons{display:grid;grid-template-columns:repeat(6,1fr);gap:8px;margin-top:10px}.btn{background:rgba(255,255,255,0.2);border:none;color:white;padding:8px 4px;border-radius:5px;cursor:pointer;font-weight:bold;font-size:11px}.btn:hover{background:rgba(255,255,255,0.3)}.btn.active{background:rgba(0,255,0,0.6);border:1px solid #00ff00}</style> </head> <body> <div class=\"container\"> <h1>ESP32 Fan Controller</h1> <div id=\"debug\">Debug: Starting...</div> <div id=\"status\"> <div class=\"info\"><span>WiFi:</span><span id=\"wifi\">Loading...</span></div> <div class=\"info\"><span>IP:</span><span id=\"ip\">Loading...</span></div> <div class=\"info\"><span>Uptime:</span><span id=\"uptime\">Loading...</span></div> </div> <div class=\"fan\" id=\"fan1-section\"> <h3>Fan 1 (GPIO 5)</h3> <div class=\"info\"><span>Speed:</span><span id=\"fan1-speed\">0%</span></div> <div class=\"info\"><span>RPM:</span><span id=\"fan1-rpm\">0</span></div> <div class=\"buttons\"> <button class=\"btn fan1-btn\" onclick=\"setFan1(0)\">0%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(10)\">10%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(20)\">20%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(30)\">30%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(40)\">40%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(50)\">50%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(60)\">60%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(70)\">70%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(80)\">80%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(90)\">90%</button> <button class=\"btn fan1-btn\" onclick=\"setFan1(100)\">100%</button> </div> </div> <div class=\"fan\" id=\"fan2-section\"> <h3>Fan 2 (GPIO 21)</h3> <div class=\"info\"><span>Speed:</span><span id=\"fan2-speed\">0%</span></div> <div class=\"info\"><span>RPM:</span><span id=\"fan2-rpm\">0</span></div> <div class=\"buttons\"> <button class=\"btn fan2-btn\" onclick=\"setFan2(0)\">0%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(10)\">10%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(20)\">20%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(30)\">30%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(40)\">40%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(50)\">50%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(60)\">60%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(70)\">70%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(80)\">80%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(90)\">90%</button> <button class=\"btn fan2-btn\" onclick=\"setFan2(100)\">100%</button> </div> </div> </div> <script> function log(msg) { document.getElementById('debug').textContent = new Date().toLocaleTimeString() + ': ' + msg; console.log(msg); } function setFan1(speed) { log('Setting Fan 1 to ' + speed + '%'); fetch('/set_fan1', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({speed: speed}) }).then(r => r.json()).then(d => { if (d.success) { log('Fan 1 set to ' + speed + '%'); updateFan1Buttons(speed); updateStatus(); } }).catch(e => log('Fan 1 Error: ' + e)); } function setFan2(speed) { log('Setting Fan 2 to ' + speed + '%'); fetch('/set_fan2', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({speed: speed}) }).then(r => r.json()).then(d => { if (d.success) { log('Fan 2 set to ' + speed + '%'); updateFan2Buttons(speed); updateStatus(); } }).catch(e => log('Fan 2 Error: ' + e)); } function updateFan1Buttons(activeSpeed) { document.querySelectorAll('.fan1-btn').forEach(btn => { btn.classList.remove('active'); if (parseInt(btn.textContent) === activeSpeed) { btn.classList.add('active'); } }); } function updateFan2Buttons(activeSpeed) { document.querySelectorAll('.fan2-btn').forEach(btn => { btn.classList.remove('active'); if (parseInt(btn.textContent) === activeSpeed) { btn.classList.add('active'); } }); } function updateStatus() { log('Updating status...'); fetch('/status').then(r => r.json()).then(d => { log('Status OK - Fan1: ' + d.fan1_speed + '%, Fan2: ' + d.fan2_speed + '%'); document.getElementById('fan1-speed').textContent = d.fan1_speed + '%'; document.getElementById('fan1-rpm').textContent = d.fan1_rpm; document.getElementById('fan2-speed').textContent = d.fan2_speed + '%'; document.getElementById('fan2-rpm').textContent = d.fan2_rpm; document.getElementById('wifi').textContent = d.wifi_network; document.getElementById('ip').textContent = d.ip_address; document.getElementById('uptime').textContent = Math.floor(d.uptime/60) + 'm'; updateFan1Buttons(d.fan1_speed || 0); updateFan2Buttons(d.fan2_speed || 0); }).catch(e => log('Status Error: ' + e)); } log('JavaScript loaded'); updateStatus(); setInterval(updateStatus, 2000); </script> </body> </html> ";
        
        request->send(200, "text/html", html);
    });
    
    // API endpoint to get current status
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", get_status_json());
    });
    
    // API endpoint to set fan speeds (dual fan support)
    server.on("/set_speed", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            JsonDocument response;
            response["success"] = true;
            
            // Handle legacy single fan control (defaults to Fan 1)
            if (doc.containsKey("speed")) {
                int speed = doc["speed"];
                set_fan1_speed(speed);
                response["speed"] = speed;
                response["fan"] = "fan1";
            }
            
            // Handle individual fan control
            if (doc.containsKey("fan1_speed")) {
                int speed = doc["fan1_speed"];
                set_fan1_speed(speed);
                response["fan1_speed"] = speed;
            }
            
            if (doc.containsKey("fan2_speed")) {
                int speed = doc["fan2_speed"];
                set_fan2_speed(speed);
                response["fan2_speed"] = speed;
            }
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });
    
    // API endpoint for individual fan control
    server.on("/set_fan1", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            int speed = doc["speed"];
            set_fan1_speed(speed);
            
            JsonDocument response;
            response["success"] = true;
            response["fan"] = "fan1";
            response["speed"] = speed;
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });
        
    server.on("/set_fan2", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            int speed = doc["speed"];
            set_fan2_speed(speed);
            
            JsonDocument response;
            response["success"] = true;
            response["fan"] = "fan2";
            response["speed"] = speed;
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });

    // ====== ARGB CONTROL ENDPOINTS ======
    
    // Fan 1 ARGB Color Control
    server.on("/set_fan1_color", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            uint8_t red = doc["red"];
            uint8_t green = doc["green"];
            uint8_t blue = doc["blue"];
            
            set_fan1_color(red, green, blue);
            
            JsonDocument response;
            response["success"] = true;
            response["fan"] = "fan1";
            response["red"] = red;
            response["green"] = green; 
            response["blue"] = blue;
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });
    
    // Fan 2 ARGB Color Control  
    server.on("/set_fan2_color", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            uint8_t red = doc["red"];
            uint8_t green = doc["green"];
            uint8_t blue = doc["blue"];
            
            set_fan2_color(red, green, blue);
            
            JsonDocument response;
            response["success"] = true;
            response["fan"] = "fan2";
            response["red"] = red;
            response["green"] = green;
            response["blue"] = blue;
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });

    // Fan 1 Brightness Control
    server.on("/set_fan1_brightness", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            int brightness_percent = doc["brightness"];
            uint8_t brightness = (brightness_percent * 255) / 100;  // Convert percentage to 0-255
            
            set_fan1_brightness(brightness);
            
            JsonDocument response;
            response["success"] = true;
            response["fan"] = "fan1";
            response["brightness"] = brightness_percent;
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });

    // Fan 2 Brightness Control
    server.on("/set_fan2_brightness", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            int brightness_percent = doc["brightness"];
            uint8_t brightness = (brightness_percent * 255) / 100;  // Convert percentage to 0-255
            
            set_fan2_brightness(brightness);
            
            JsonDocument response;
            response["success"] = true;
            response["fan"] = "fan2";  
            response["brightness"] = brightness_percent;
            
            String response_str;
            serializeJson(response, response_str);
            request->send(200, "application/json", response_str);
        });

    // ARGB Effect Control
    server.on("/set_argb_effect", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            deserializeJson(doc, data);
            
            int effect = doc["effect"];
            argb_effect = effect;
            
            Serial.printf("ARGB effect changed to: %d\n", effect);
            
            JsonDocument response;
            response["success"] = true;
            response["effect"] = effect;
            
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
    Serial.println("🔧 GPIO PIN ASSIGNMENTS (Updated for reliable PWM):");
    Serial.printf("Fan 1 PWM: GPIO %d (was GPIO 2) | Fan 2 PWM: GPIO %d (was GPIO 4)\n", FAN1_PWM_GPIO, FAN2_PWM_GPIO);
    Serial.printf("Fan 1 Tacho: GPIO %d | Fan 2 Tacho: GPIO %d\n", FAN1_TACHO_GPIO, FAN2_TACHO_GPIO);
    Serial.println("Note: GPIO 2 & 4 have restrictions - using GPIO 5 & 21 for reliable PWM");
    Serial.println();
    
    // Initialize hardware
    init_fan_pwm();
    init_tacho_input();
    init_argb();  // Initialize ARGB LED control
    
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
    
    // Update ARGB LED effects
    update_argb_leds();
    
    // Small delay to prevent overwhelming the system
    delay(100);
}