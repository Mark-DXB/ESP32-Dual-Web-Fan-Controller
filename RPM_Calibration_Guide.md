# RPM Calibration Guide - Fixing Tachometer Readings

## 🔍 **Problem Identified**

The ESP32 fan controller was showing **incorrect RPM readings** compared to the actual fan speed:

### **Symptoms Observed:**
- **Official fan max speed**: 2300 RPM
- **Web UI displayed**: 3600 RPM (56% higher than actual)
- **At 50% PWM**: Web UI showed 2520 RPM instead of expected ~1150 RPM
- **Oscilloscope measurements**: 70 Hz at max speed, 35 Hz at 50% speed

## ⚙️ **Root Cause Analysis**

### **Standard Assumption vs Reality**
```
Standard PC Fan Specification: 2 pulses per revolution
Our Fan Actual Measurement: 1.83 pulses per revolution
```

### **Calculation Error**
The original code assumed **2 pulses per revolution (PPR)**, but our fan actually produces **1.83 PPR**.

```cpp
// INCORRECT (original code):
RPM = (pulse_count * 60) / 2  // Assumes 2 PPR

// CORRECT (after measurement):  
RPM = (pulse_count * 60) / 1.83  // Measured actual PPR
```

## 📐 **PPR Calculation Method**

### **Formula to Determine Actual PPR:**
```
PPR = Tachometer_Frequency_Hz / (Actual_RPM / 60)
```

### **Our Fan Measurements:**
```
Given:
- Actual fan speed: 2300 RPM (from fan specifications)
- Measured tachometer frequency: 70 Hz (oscilloscope)

Calculation:
PPR = 70 Hz / (2300 RPM / 60 seconds)
PPR = 70 / 38.33
PPR = 1.83 pulses per revolution
```

## 🔧 **Solution Implemented**

### **1. Updated Configuration:**
```cpp
// Added measured PPR constant
#define PULSES_PER_REVOLUTION   1.83   // Measured value for accurate RPM calculation
```

### **2. Updated RPM Calculation:**
```cpp
// Before (incorrect):
fan1_current_rpm = (fan1_pulses * 60) / 2;

// After (correct):
fan1_current_rpm = (fan1_pulses * 60) / PULSES_PER_REVOLUTION;
```

### **3. Added Documentation:**
```cpp
// Tachometer Configuration (Measured Values)
// Standard assumption: 2 pulses per revolution
// Actual measurement: Fan max 2300 RPM produces 70 Hz tachometer signal
// Calculated PPR: 70 Hz / (2300 RPM / 60) = 1.83 pulses per revolution
```

## ✅ **Verification Results**

### **Expected Corrected Readings:**
```
At 100% PWM (70 Hz tachometer):
RPM = (70 * 60) / 1.83 = 2295 RPM ≈ 2300 RPM ✓

At 50% PWM (35 Hz tachometer):  
RPM = (35 * 60) / 1.83 = 1148 RPM ≈ 1150 RPM (50% of 2300) ✓
```

## 🛠️ **How to Calibrate Your Own Fans**

### **Method 1: Using Fan Specifications (Recommended)**
```bash
Step 1: Find official fan max RPM (datasheet/label)
Step 2: Set fan to 100% PWM  
Step 3: Measure tachometer frequency with oscilloscope
Step 4: Calculate: PPR = Frequency_Hz / (Max_RPM / 60)
Step 5: Update PULSES_PER_REVOLUTION constant in code
```

### **Method 2: Using Reference RPM Meter**
```bash
Step 1: Use external laser/contact tachometer
Step 2: Measure actual RPM at known PWM setting
Step 3: Compare with ESP32 displayed RPM
Step 4: Calculate correction factor
Step 5: Adjust PPR value accordingly
```

### **Method 3: Using Oscilloscope + Trial**
```bash
Step 1: Measure tachometer frequency at several PWM levels
Step 2: Try different PPR values (1.5, 1.8, 2.0, 2.2, etc.)
Step 3: Check which gives most consistent results across PWM range
Step 4: Use the PPR value that gives best linearity
```

## 📊 **Common PPR Values by Fan Type**

```
┌─────────────────────┬─────────────┬─────────────────────┐
│ Fan Type/Brand      │ Typical PPR │ Notes               │
├─────────────────────┼─────────────┼─────────────────────┤
│ Standard PC Fans    │ 2.0         │ Most common         │
│ Server Fans         │ 1.5 - 2.5   │ Varies by model     │
│ Noctua Fans         │ 2.0         │ Usually standard    │
│ Cheap Generic Fans  │ 1.5 - 2.2   │ Often non-standard  │
│ High-Speed Fans     │ 1.8 - 2.2   │ May use different   │
│ PWM vs 3-pin        │ Varies      │ Check individually  │
└─────────────────────┴─────────────┴─────────────────────┘
```

## 🔍 **Troubleshooting RPM Issues**

### **Symptom: RPM Reading Too High**
```
Possible Causes:
1. PPR value too low → Increase PPR value
2. Interrupt triggering on both edges → Check interrupt setup (should be RISING only)
3. Electrical noise → Add filtering capacitor on tacho line

Solution: Measure actual PPR and update constant
```

### **Symptom: RPM Reading Too Low**  
```
Possible Causes:
1. PPR value too high → Decrease PPR value
2. Missing pulses due to poor connection → Check tachometer wiring
3. Fan tachometer signal too weak → Add pull-up resistor

Solution: Verify connections and measure actual PPR
```

### **Symptom: Erratic/Jumping RPM**
```
Possible Causes:
1. Loose tachometer connection → Check wire connections
2. Electrical interference from PWM → Add EMI filtering
3. Poor quality fan tachometer → Try different fan for testing

Solution: Check connections and add 100nF capacitor across tacho signal
```

## ⚡ **Advanced Calibration Features**

### **Runtime PPR Adjustment (Future Enhancement)**
```cpp
// Could be added to web interface:
void set_ppr_calibration(float new_ppr) {
    PULSES_PER_REVOLUTION = new_ppr;
    // Save to EEPROM for persistence
}
```

### **Automatic PPR Detection (Future Enhancement)**  
```cpp
// Could measure RPM at known PWM and auto-calculate PPR:
float auto_calibrate_ppr(int known_rpm, int pwm_percent) {
    // Set PWM to known value
    // Measure pulse frequency for 5 seconds
    // Calculate PPR based on expected RPM
    return calculated_ppr;
}
```

## 🎯 **Key Takeaways**

1. **Never assume standard PPR** - Always measure your specific fans
2. **Use oscilloscope** for accurate frequency measurement  
3. **Verify with fan specifications** when available
4. **Document your PPR value** for future reference
5. **Test across full PWM range** to verify linearity
6. **Consider manufacturing tolerances** - PPR may vary between identical fan models

## ✅ **Success Metrics**

After calibration, you should see:
- **Accurate max RPM** matching fan specifications  
- **Linear RPM response** to PWM changes
- **Consistent readings** across multiple measurements
- **Reasonable minimum RPM** (typically 20-30% of max)

**With proper PPR calibration, your ESP32 fan controller will display accurate, reliable RPM readings that match your fan's actual performance!** 🎯
