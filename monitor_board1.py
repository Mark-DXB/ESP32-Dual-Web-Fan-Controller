#!/usr/bin/env python3
"""
Monitor ESP32 Web Fan Controller - Board #1
Shows WiFi connection status and access information
"""

import serial
import time
import sys

def monitor_esp32():
    try:
        # Connect to Board #1 serial port
        port = '/dev/cu.usbserial-0001'  # Board #1 port
        baudrate = 115200
        
        print("🔍 Connecting to ESP32 Web Fan Controller - Board #1")
        print(f"📡 Port: {port} @ {baudrate} baud")
        print("=" * 60)
        
        ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(2)  # Wait for connection
        
        print("📊 Monitoring serial output... (Press Ctrl+C to stop)")
        print("=" * 60)
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Highlight important WiFi status messages
                        if "WiFi" in line or "AP Mode" in line or "Connected" in line:
                            print(f"🌐 {line}")
                        elif "Web Interface" in line:
                            print(f"🔗 {line}")
                        elif "RPM" in line:
                            print(f"🌪️ {line}")
                        elif "System Ready" in line:
                            print(f"✅ {line}")
                        else:
                            print(f"   {line}")
                            
                except UnicodeDecodeError:
                    pass
            
            time.sleep(0.1)
            
    except serial.SerialException as e:
        print(f"❌ Serial connection error: {e}")
        print(f"💡 Make sure Board #1 is connected to {port}")
        return False
        
    except KeyboardInterrupt:
        print("\n🛑 Monitoring stopped by user")
        return True
        
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    monitor_esp32()
