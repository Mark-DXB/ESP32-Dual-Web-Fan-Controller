# ESP32 to 12V Fan Control Circuit - IRF520 MOSFET Driver

## Circuit Diagram

```
                    +12V Supply
                         │
                         │
    ┌────────────────────┼────────────────────┐
    │                    │                    │
    │              ┌─────┴─────┐        ┌─────┴─────┐
    │              │    FAN1   │        │    FAN2   │
    │              │   (12V)   │        │   (12V)   │
    │              └─────┬─────┘        └─────┬─────┘
    │                    │                    │
    │              ┌─────┴─────┐        ┌─────┴─────┐
    │              │     D1    │        │     D2    │
    │              │  1N4007   │        │  1N4007   │
    │              │  Flyback  │        │  Flyback  │
    │              └─────┬─────┘        └─────┬─────┘
    │                    │                    │
    │                    │                    │
    │              ┌─────┴─────┐        ┌─────┴─────┐
    │              │  IRF520   │        │  IRF520   │
    │              │     D     │        │     D     │
    │              │     │     │        │     │     │
    │              │     S     │        │     S     │
    │              └─────┬─────┘        └─────┬─────┘
    │                    │                    │
    └────────────────────┼────────────────────┘
                         │
                       GND
                         │
    ┌────────────────────┼────────────────────┐
    │                    │                    │
    │              ┌─────┴─────┐        ┌─────┴─────┐
    │              │    10K    │        │    10K    │
    │              │    R2     │        │    R4     │
    │              └─────┬─────┘        └─────┬─────┘
    │                    │                    │
    │              ┌─────┴─────┐        ┌─────┴─────┐
    │              │           │        │           │
    │              │    GATE   │        │    GATE   │
    │              │           │        │           │
    │              └─────┬─────┘        └─────┬─────┘
    │                    │                    │
    │              ┌─────┴─────┐        ┌─────┴─────┐
    │              │   220Ω    │        │   220Ω    │
    │              │    R1     │        │    R3     │
    │              └─────┬─────┘        └─────┬─────┘
    │                    │                    │
    └────────────────────┼────────────────────┘
                         │
              ┌──────────┼──────────┐
              │          │          │
         GPIO 5      GPIO 21    ESP32 GND
        (Fan 1)     (Fan 2)        │
              │          │          │
              └──────────┼──────────┘
                         │
                     ESP32 Board
```

## Component List

### MOSFETs
- **Q1, Q2**: IRF520 N-Channel MOSFET
  - Drain-Source Voltage: 100V
  - Drain Current: 9.2A
  - Gate Threshold: 2-4V
  - RDS(on): 0.27Ω @ 10V gate drive

### Resistors
- **R1, R3**: 220Ω (Gate Current Limiting)
  - Limits gate current during switching
  - Reduces EMI and protects ESP32
- **R2, R4**: 10kΩ (Gate Pull-down)
  - Ensures MOSFET is OFF when ESP32 is not driving
  - Prevents floating gate conditions

### Diodes  
- **D1, D2**: 1N4007 Flyback Diodes
  - Protects MOSFETs from inductive kickback
  - Essential for motor/fan loads
  - Fast recovery time important for PWM

### Power Supply
- **12V Supply**: 2-5A depending on fan requirements
- **Common Ground**: ESP32 and 12V supply must share ground

## Detailed Connection Guide

### ESP32 Connections
```
ESP32 Pin    →    Circuit Connection
─────────────────────────────────────
GPIO 5       →    R1 (220Ω) → Q1 Gate
GPIO 21      →    R3 (220Ω) → Q2 Gate  
GND          →    Circuit Ground
```

### IRF520 Pinout (TO-220 Package)
```
Looking at the flat side with pins down:

  G   D   S
  │   │   │
  1   2   3

G = Gate   (connects to ESP32 via 220Ω resistor)
D = Drain  (connects to fan negative terminal)
S = Source (connects to circuit ground)
```

### Fan Connections
```
Fan Wire     →    Connection
─────────────────────────────
Red (+12V)   →    +12V Supply
Black (GND)  →    MOSFET Drain (through flyback diode)
Yellow (Tacho) →  ESP32 GPIO 18/19 (existing tacho inputs)
```

## PCB Layout Considerations

### Trace Width
- **12V Power Traces**: Minimum 1mm width for 2A
- **Gate Drive Traces**: 0.3mm sufficient  
- **Ground Plane**: Use pour/plane for low impedance

### Component Placement
- Keep MOSFETs close to fans (minimize high-current traces)
- Place flyback diodes physically close to fan connectors
- Gate resistors close to ESP32
- Pull-down resistors close to MOSFET gates

### Heat Management
- IRF520 may need heatsink for continuous high current
- Calculate: Power = I²R × Duty Cycle
- At 2A fan current: P = 4 × 0.27Ω × 0.5 = 0.54W
- TO-220 package can handle this without heatsink

## Circuit Analysis

### Gate Drive Calculation
```
ESP32 Output: 3.3V
Gate Resistor: 220Ω  
Gate Capacitance: ~1nF (IRF520)
Rise Time: τ = R×C = 220Ω × 1nF = 220ns

This is fast enough for 25kHz PWM (40μs period)
```

### Power Dissipation
```
At 25kHz PWM, 50% duty cycle, 2A fan:
Conduction Loss: I²×RDS(on)×D = 4×0.27×0.5 = 0.54W
Switching Loss: Negligible at 25kHz
Total: ~0.6W per MOSFET (no heatsink needed)
```

## Safety Features

### Overcurrent Protection
- MOSFETs have built-in current limiting
- Fans are self-limiting loads
- 12V supply should have current limiting

### Thermal Protection  
- Monitor MOSFET temperature if enclosed
- Fans provide airflow for cooling
- IRF520 has thermal shutdown at 175°C

### EMI Reduction
- Gate resistors reduce switching noise
- Flyback diodes suppress voltage spikes
- Keep gate traces short and controlled impedance

## Testing Procedure

### 1. Static Testing (No PWM)
```bash
# Set GPIO 5 LOW (Fan 1 OFF)
# Measure: Drain voltage = 12V, Gate voltage = 0V

# Set GPIO 5 HIGH (Fan 1 ON)  
# Measure: Drain voltage = ~0.5V, Gate voltage = 3.3V
```

### 2. PWM Testing
```bash
# Set 50% duty cycle on GPIO 5
# Measure with oscilloscope:
# - Gate: 0-3.3V square wave at 25kHz
# - Drain: 0-12V square wave at 25kHz  
# - Fan voltage: ~6V average (50% × 12V)
```

### 3. Current Measurement
```bash
# Insert current meter in 12V supply line
# Verify fan current matches specifications
# Typical PC fan: 0.1-0.5A at 12V
```

## Troubleshooting

### Fan Not Running
- Check 12V supply voltage
- Verify ESP32 GPIO output with multimeter
- Check MOSFET gate voltage (should be 3.3V when ON)
- Verify flyback diode polarity

### Overheating MOSFETs
- Check RDS(on) specification  
- Verify fan current within MOSFET ratings
- Add heatsink if continuous high current
- Check for short circuits

### PWM Noise/Interference
- Add 100nF ceramic capacitor across 12V supply near MOSFETs
- Use twisted pair wires for fan connections
- Ensure proper grounding between ESP32 and 12V supply

## Schematic Symbol Reference

```
IRF520 N-Channel MOSFET:
      G │
        │  ┌─── D
        └──┤
           │  ┌─── S  
           └──┘

1N4007 Diode:
    ┌─│◄─┐
    │    │
  Anode  Cathode
```
