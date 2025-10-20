#!/usr/bin/env python3
import requests
import threading
from concurrent.futures import ThreadPoolExecutor

def test_esp32(ip):
    """Test if IP has ESP32 web interface"""
    try:
        response = requests.get(f"http://{ip}/status", timeout=2)
        if response.status_code == 200:
            data = response.json()
            if 'board_mac' in data and 'fan_rpm' in data:
                return {
                    'ip': ip,
                    'mac': data.get('board_mac', ''),
                    'fan_rpm': data.get('fan_rpm', 0),
                    'fan_speed': data.get('fan_speed', 0),
                    'wifi_mode': data.get('wifi_mode', 'Unknown')
                }
    except:
        pass
    return None

def main():
    print("🎯 Scanning 10.0.1.0/24 network for ESP32 Board #1")
    print("Target MAC: 44:1d:64:f5:b4:84")
    print("=" * 50)
    
    # Generate all IPs in 10.0.1.0/24 range
    ips = [f"10.0.1.{i}" for i in range(1, 255)]
    
    print(f"📡 Testing {len(ips)} IP addresses...")
    
    found_devices = []
    
    # Scan in parallel for speed
    with ThreadPoolExecutor(max_workers=30) as executor:
        results = list(executor.map(test_esp32, ips))
    
    # Filter results
    found_devices = [r for r in results if r is not None]
    
    print("\n" + "=" * 50)
    
    if found_devices:
        for device in found_devices:
            print(f"🎉 ESP32 Found!")
            print(f"   IP: {device['ip']}")
            print(f"   MAC: {device['mac']}")
            print(f"   Mode: {device['wifi_mode']}")
            print(f"   Fan RPM: {device['fan_rpm']}")
            print(f"   Fan Speed: {device['fan_speed']}%")
            print(f"   🌐 Web Interface: http://{device['ip']}")
            
            # Check if this is Board #1
            target_mac = "44:1d:64:f5:b4:84"
            if device['mac'].lower() == target_mac.lower():
                print(f"   ✅ CONFIRMED: This is Board #1!")
            print()
    else:
        print("❌ No ESP32 devices found on 10.0.1.0/24 network")
        print("\n💡 Let me check if ESP32 is in AP mode...")
        
        # Test AP mode
        ap_result = test_esp32("192.168.4.1")
        if ap_result:
            print(f"🎉 Found ESP32 in AP Mode!")
            print(f"   IP: 192.168.4.1")
            print(f"   MAC: {ap_result['mac']}")
            print(f"   Connect to WiFi: 'ESP32-FanController'")
            print(f"   Password: 'fancontrol123'")
            print(f"   Web Interface: http://192.168.4.1")
        else:
            print("❌ ESP32 not found in AP mode either")

if __name__ == "__main__":
    main()
