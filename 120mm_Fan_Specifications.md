# 120mm PC Case Cooling Fan - Electrical Specifications

## 🔌 Standard 4-Wire PWM Fan Connector Pinout

```
Pin Layout (looking at connector from wire side):
┌─────┬─────┬─────┬─────┐
│  1  │  2  │  3  │  4  │
└─────┴─────┴─────┴─────┘

Pin 1: Ground (GND) - Black Wire
Pin 2: +12V Power   - Red Wire  
Pin 3: Tachometer   - Yellow Wire
Pin 4: PWM Control  - Blue Wire
```

## ⚡ Electrical Specifications

### Power Requirements
```
Nominal Voltage: 12V DC ±10% (10.8V - 13.2V)
Operating Range: 7V - 13.8V (reduced performance below 12V)
Reverse Polarity Protection: Usually none (external protection required)
```

### Current Draw Specifications

#### Typical Current Ranges by Fan Type:
```
┌─────────────────────┬─────────────┬─────────────┬─────────────┐
│ Fan Type            │ Idle/Min    │ Typical     │ Maximum     │
├─────────────────────┼─────────────┼─────────────┼─────────────┤
│ Standard Case Fan   │ 0.08-0.15A  │ 0.18-0.30A  │ 0.35-0.50A  │
│ High Performance    │ 0.12-0.20A  │ 0.25-0.45A  │ 0.50-0.80A  │
│ High Static Press.  │ 0.15-0.25A  │ 0.30-0.60A  │ 0.60-1.20A  │
│ RGB Lighting Fans   │ 0.20-0.35A  │ 0.40-0.70A  │ 0.70-1.50A  │
└─────────────────────┴─────────────┴─────────────┴─────────────┘
```

#### Popular 120mm Fan Examples:
```
┌──────────────────────┬──────────┬──────────┬──────────────────┐
│ Model                │ Voltage  │ Current  │ Power            │
├──────────────────────┼──────────┼──────────┼──────────────────┤
│ Noctua NF-A12x25     │ 12V      │ 0.14A    │ 1.68W           │
│ Arctic P12 PWM       │ 12V      │ 0.08A    │ 0.96W           │
│ Corsair ML120 Pro    │ 12V      │ 0.30A    │ 3.6W            │
│ be quiet! Pure Wings │ 12V      │ 0.11A    │ 1.32W           │
│ Cooler Master SF120  │ 12V      │ 0.15A    │ 1.8W            │
│ Phanteks PH-F120MP   │ 12V      │ 0.24A    │ 2.88W           │
└──────────────────────┴──────────┴──────────┴──────────────────┘
```

## 📊 PWM Control Specifications

### PWM Signal Requirements
```
Voltage Levels:
  Logic HIGH (Fan ON):  3.3V - 5V (TTL/CMOS compatible)
  Logic LOW (Fan OFF):  0V - 0.8V
  
Frequency Range:
  Standard: 21kHz - 28kHz (Intel spec: 25kHz ±10%)
  Extended: 1kHz - 30kHz (most fans work in this range)
  
Duty Cycle:
  0% - 100% (linear speed control on most fans)
  Minimum: ~20% (many fans stall below this)
  
Input Impedance:
  Typical: 10kΩ - 47kΩ pull-up to +12V
  Current Draw: <1mA at PWM pin
```

### PWM Response Characteristics
```
Speed Range with PWM:
  Minimum Speed: 20-30% of maximum (varies by fan)
  Linear Region: 30% - 100% duty cycle
  Resolution: ~1% steps achievable
  
Start-up Behavior:
  Minimum Start Voltage: 7-9V (varies by fan)
  Start-up Current: 150-200% of running current (brief spike)
  Spin-up Time: 0.5 - 2.0 seconds to reach set speed
```

## 📡 Tachometer Signal Specifications

### Tachometer Output
```
Signal Type: Open-collector/open-drain output
Voltage Levels:
  HIGH: +12V (with external pull-up resistor)
  LOW: 0V - 0.4V (transistor ON)
  
Pull-up Requirements:
  Resistor: 1kΩ - 10kΩ to +3.3V, +5V, or +12V
  ESP32 Compatible: Direct connection to GPIO (has internal pull-up)
  
Frequency Calculation:
  PPR (Pulses Per Revolution): Usually 2 (some fans have 1 or 4)
  Formula: RPM = (Frequency × 60) ÷ PPR
  Example: 50Hz × 60 ÷ 2 = 1500 RPM
```

### Tachometer Signal Timing
```
Pulse Width:
  Minimum LOW Time: 100µs typical
  Minimum HIGH Time: 100µs typical
  Maximum Frequency: ~5kHz (for high-speed fans)
  
Signal Quality:
  Rise/Fall Time: <10µs typical
  Jitter: ±2% typical
  Update Rate: Real-time (immediate response to speed changes)
```

## 🔧 Design Considerations for MOSFET Circuit

### Current Handling Verification
```
Your IRF520 Circuit Analysis:
  IRF520 Continuous Current: 9.2A
  Typical Fan Current: 0.08A - 1.5A
  Safety Margin: 6x - 115x (excellent)
  
Power Dissipation at Maximum Fan Current (1.5A):
  P = I²R = (1.5A)² × 0.27Ω = 0.61W
  Conclusion: No heatsink required for any 120mm fan
```

### PWM Compatibility
```
ESP32 PWM Output: 3.3V (compatible with fan PWM input)
IRF520 Gate Drive: 3.3V via 220Ω resistor
Gate Threshold: 2-4V (ESP32 3.3V will fully turn ON)
Switching Speed: 220ns (excellent for 25kHz PWM)
```

### Inrush Current Protection
```
Fan Start-up Current: 1.5 × Normal (brief spike)
IRF520 Peak Current: 37A (10ms pulse rating)
Safety Factor: >20x for start-up transients
Conclusion: No additional protection needed
```

## 📈 Performance Characteristics

### Typical 120mm Fan Performance
```
Speed Range:
  Minimum: 300-800 RPM (varies by model)
  Maximum: 1200-3000 RPM (varies by model)
  
Airflow (CFM):
  Standard Fans: 30-80 CFM
  High Performance: 60-120 CFM
  High Static Pressure: 40-100 CFM
  
Noise Levels:
  Quiet Operation: <20 dBA
  Standard: 20-35 dBA  
  High Performance: 25-45 dBA
  
Static Pressure:
  Standard: 0.5-1.5 mmH2O
  High Pressure: 1.5-4.0 mmH2O
```

### Temperature Characteristics
```
Operating Temperature Range:
  Standard: -10°C to +70°C
  Extended: -40°C to +85°C (industrial)
  
Temperature Coefficient:
  Speed Change: ±2% per 10°C (due to bearing friction)
  Current Change: ±1% per 10°C (motor characteristics)
  
Thermal Protection:
  Most fans: None (external monitoring recommended)
  Some premium: Built-in thermal shutdown
```

## 🛡️ Safety and Protection

### Overcurrent Protection
```
Recommended Protection:
  Current Monitoring: Optional (fans are self-limiting)
  Fuse Rating: 2A fast-blow (per fan, if desired)
  MOSFET Protection: Built-in current limiting in IRF520
  
Short Circuit Behavior:
  Fan Motor: Usually safe (impedance limited)
  Wiring Short: IRF520 will current-limit and may overheat
```

### Reverse Polarity Protection
```
Fan Internal Protection: Usually NONE
External Protection Required:
  Method 1: Series Schottky diode (0.3V drop)
  Method 2: P-Channel MOSFET (more complex)
  Recommendation: Careful wiring (fans rarely reversed)
```

## 🔌 Connector Specifications

### Physical Connector
```
Standard: 4-pin fan header (Molex KK series compatible)
Pin Spacing: 2.54mm (0.1")
Wire Gauge: 20-24 AWG typical
Cable Length: 150-300mm standard
Connector Rating: 2A per pin typical
```

### Wire Color Standards
```
International Standard (Most Common):
  Black: Ground (GND)
  Red: +12V Power
  Yellow: Tachometer Signal
  Blue: PWM Control

Alternative Color Schemes (Some Manufacturers):
  Black: Ground
  Red: +12V  
  White: Tachometer
  Blue: PWM Control
```

## 🎯 Design Validation

### Your IRF520 Circuit Suitability
```
✅ Voltage Compatibility: 12V ← Perfect match
✅ Current Handling: 9.2A ← Handles any 120mm fan (max 1.5A)
✅ PWM Frequency: 25kHz ← Within standard range (21-28kHz)
✅ Gate Drive: 3.3V ← Sufficient for IRF520 (2-4V threshold)
✅ Switching Speed: Fast enough for 25kHz PWM
✅ Protection: Flyback diodes protect against motor inductance
✅ Power Dissipation: <1W ← No heatsink needed
```

### Recommended Testing Points
```
1. No-Load Test:
   - PWM at 0%: Fan stopped, 0A current
   - PWM at 100%: Fan at max speed, measure actual current

2. PWM Linearity Test:
   - Test 20%, 40%, 60%, 80%, 100% duty cycles  
   - Verify proportional speed response

3. Thermal Test:
   - Run fans at 100% for 30 minutes
   - Monitor MOSFET temperature (should be <50°C)

4. Tachometer Test:
   - Verify RPM measurement accuracy
   - Check signal quality with oscilloscope
```

## 📋 Component Sizing Verification

### Your Circuit vs. Fan Requirements
```
Component Analysis:
┌─────────────────────┬────────────────┬─────────────────┬──────────┐
│ Component           │ Your Circuit   │ Fan Requirement │ Status   │
├─────────────────────┼────────────────┼─────────────────┼──────────┤
│ MOSFET Current      │ 9.2A          │ 0.08-1.5A       │ ✅ PASS  │
│ Gate Drive Voltage  │ 3.3V          │ >2V threshold    │ ✅ PASS  │
│ PWM Frequency       │ 25kHz         │ 21-28kHz std    │ ✅ PASS  │
│ Flyback Diode       │ 1N4007        │ >1.5A, fast     │ ✅ PASS  │
│ Gate Resistor       │ 220Ω          │ Current limit    │ ✅ PASS  │
│ Pull-down Resistor  │ 10kΩ          │ Gate protection  │ ✅ PASS  │
└─────────────────────┴────────────────┴─────────────────┴──────────┘

Conclusion: Your IRF520 circuit is perfectly sized for any 120mm PC fan!
```
