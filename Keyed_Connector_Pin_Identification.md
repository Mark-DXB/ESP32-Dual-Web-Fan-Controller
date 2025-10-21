# 4-Pin PWM Fan Connector - Physical Pin Identification (Keyed Connector)

## 🔑 **CRITICAL: Identifying Pins by Physical Connector Keying**

When all wires are black, use the **physical connector shape** to identify pins correctly!

## 📐 **Standard 4-Pin Fan Connector Physical Layout**

### **Connector View - Looking at the PLASTIC HOUSING**

```
     FAN CONNECTOR (Female - from wire side)
     ╭─────────────────────────────────╮
     │  ┌───┐  ┌───┐  ┌───┐  ┌───┐   │
     │  │ 1 │  │ 2 │  │ 3 │  │ 4 │   │  ← Pin numbers
     │  └───┘  └───┘  └───┘  └───┘   │
     │                               │
     │     ▼ KEYING NOTCH HERE ▼     │  ← Physical key prevents wrong insertion
     ╰─────────────────────────────────╯
```

### **Motherboard Header (Male - on PCB)**

```
     MOTHERBOARD FAN HEADER (Male - top view)
     ╭─────────────────────────────────╮
     │  ┌───┐  ┌───┐  ┌───┐  ┌───┐   │
     │  │ 4 │  │ 3 │  │ 2 │  │ 1 │   │  ← Pin numbers (mirror image)
     │  └───┘  └───┘  └───┘  └───┘   │
     │                               │
     │     ▲ KEYING TAB HERE ▲       │  ← Physical tab matches notch
     ╰─────────────────────────────────╯
```

## 🎯 **PIN IDENTIFICATION METHOD**

### **Step 1: Locate the Keying Feature**
- **Fan Connector**: Look for the **NOTCH** (indent) on one side
- **Motherboard Header**: Look for the **TAB** (protrusion) on one side
- These MUST align - connector only fits one way!

### **Step 2: Pin Numbering with Keying**

```
RULE: Pin 1 is ALWAYS on the side WITH the keying feature

Fan Connector (Female):
┌─ Keying Notch Side ─┐
│  1    2    3    4   │
└─────────────────────┘

Motherboard Header (Male):  
┌─ Keying Tab Side ─┐
│  1   2   3   4    │
└───────────────────┘
```

## ⚡ **DEFINITIVE PIN ASSIGNMENT**

### **When Connector is Properly Oriented:**

```
Pin Position:  │  1  │  2  │  3  │  4  │
Function:      │ GND │+12V │TACH │ PWM │
Description:   │Black│ Red │Yellow│Blue│ ← Standard colors (yours are all black)
Your Circuit:  │ GND │+12V │GPIO │GPIO │
```

### **Physical Identification Guide:**

```
Look at your fan connector:

    Side A              Side B
     │                   │
     ▼                   ▼
┌────────────────────────────┐
│  □    □    □    □         │  ← 4 pins visible
│                           │
│  ████ KEYING NOTCH        │  ← DEEP NOTCH on one side
└────────────────────────────┘
     ▲
  PIN 1 SIDE

If keying notch is on the LEFT: Pin 1 = Left-most pin
If keying notch is on the RIGHT: Pin 1 = Right-most pin
```

## 🔍 **MOTHERBOARD HEADER IDENTIFICATION**

Most motherboards label the fan headers:

```
Typical Motherboard Marking:
┌─────────────────┐
│  PWM FAN        │  ← Header label
│                 │
│  ┌─┬─┬─┬─┐      │
│  │1│2│3│4│      │  ← Pin numbers often printed
│  └─┴─┴─┴─┘      │
│    ██            │  ← Keying tab (plastic)
└─────────────────┘
```

## ⚠️ **CRITICAL SAFETY CHECKS**

### **Before Connecting ANYTHING:**

1. **Visual Inspection**: Ensure connector can only fit ONE way
2. **Gentle Test Fit**: Connector should slide on easily when oriented correctly
3. **Resistance Check**: If it doesn't fit easily, DON'T force it - try flipping
4. **Double Check**: Pin 1 (GND) should be on the keying side

### **Multimeter Verification (POWER OFF):**

```bash
Step 1: Identify Pin 1 (keying side)
Step 2: With fan disconnected, use multimeter continuity mode
Step 3: Check Pin 1 to fan motor case - should show continuity (GND)
Step 4: Check Pin 2 - should be isolated (this is +12V)
```

## 📏 **CONNECTOR DIMENSIONS & KEYING SPECS**

```
Standard 2.54mm (0.1") Header:
- Pin spacing: 2.54mm
- Keying notch depth: ~2mm  
- Keying notch width: ~3mm
- Housing width: ~10.5mm
- Housing length: ~13mm
```

## 🎯 **YOUR ESP32 CONNECTION MAP**

### **Based on Proper Connector Orientation:**

```
Fan Pin (Keying Side → Far Side):
┌─────┬─────┬─────┬─────┐
│  1  │  2  │  3  │  4  │  ← Pin numbers
│ GND │+12V │TACH │ PWM │  ← Functions  
└─────┴─────┴─────┴─────┘
   │     │     │     │
   │     │     │     └── ESP32 GPIO 5/21 (via MOSFET)
   │     │     └── ESP32 GPIO 18/19 (direct connection)
   │     └── +12V Supply 
   └── Circuit Ground (ESP32 GND)
```

## 🔧 **PRACTICAL VERIFICATION STEPS**

### **Method 1: Motherboard Reference**
1. Find a working PC motherboard with fan headers
2. Look for printed pin numbers (1,2,3,4)
3. Note keying orientation relative to pin 1
4. Apply same orientation to your circuit

### **Method 2: Multimeter Testing (SAFE)**
```bash
With fan DISCONNECTED and circuit UNPOWERED:
1. Set multimeter to continuity mode
2. Touch one probe to fan motor metal housing
3. Touch other probe to each pin in connector
4. Pin showing continuity = Pin 1 (GND)
5. Pin 1 should be on keying side
```

### **Method 3: Known Good Fan**
1. If you have a fan with colored wires, use it as reference
2. Note black wire position relative to keying
3. Apply same orientation to your all-black fans

## ⚡ **FINAL CONNECTION VERIFICATION**

```
BEFORE applying power:
✅ Pin 1 (GND) connects to circuit ground
✅ Pin 2 (+12V) connects to 12V supply  
✅ Pin 3 (TACH) connects to ESP32 GPIO 18/19
✅ Pin 4 (PWM) connects to MOSFET output (Drain)
✅ Connector keying prevents wrong insertion
✅ All connections double-checked with multimeter
```

## 🚨 **WHAT HAPPENS IF PINS ARE WRONG**

```
Wrong Connection Consequences:
- Pin 1/2 swapped: 12V on ground → Circuit damage
- Pin 3/4 swapped: PWM on tach input → No control but safe
- 180° rotation: +12V and GND swapped → IMMEDIATE DAMAGE

Prevention: Physical keying makes wrong connection impossible!
```

## 🎯 **BOTTOM LINE FOR ALL-BLACK WIRES:**

**The keying feature is your friend!** 
- Connector physically prevents wrong insertion
- Pin 1 is ALWAYS on the keying side  
- When properly seated: Pin 1 = GND, Pin 2 = +12V
- Use multimeter to verify GND continuity if unsure

**Trust the keying - it's designed to prevent exactly this problem!** 🔑
