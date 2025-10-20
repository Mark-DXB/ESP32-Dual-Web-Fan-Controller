#!/usr/bin/env python3
"""
Comprehensive ESP32 Board #1 Finder
Try multiple network ranges and methods to locate the ESP32
"""

import requests
import subprocess
import threading
from concurrent.futures import ThreadPoolExecutor
import time

def test_esp32_endpoint(ip):
    """Test if an IP responds to ESP32 web interface"""
    try:
        response = requests.get(f"http://{ip}/status", timeout=1.5)
        if response.status_code == 200:
            data = response.json()
            if 'board_mac' in data and ('fan_rpm' in data or 'fan_speed' in data):
                return {
                    'ip': ip,
                    'mac': data.get('board_mac', 'Unknown'),
                    'fan_rpm': data.get('fan_rpm', 'N/A'),
                    'fan_speed': data.get('fan_speed', 'N/A'),
                    'wifi_mode': data.get('wifi_mode', 'Unknown'),
                    'network': data.get('wifi_network', 'Unknown'),
                    'uptime': data.get('uptime', 0)
                }
    except:
        pass
    return None

def scan_ip_range(start_ip, count, description):
    """Scan a range of IP addresses"""
    print(f"🔍 Scanning {description}...")
    
    # Parse the base IP
    ip_parts = start_ip.split('.')
    base = int(ip_parts[3])
    ip_base = '.'.join(ip_parts[:3])
    
    # Generate IP list
    ips = [f"{ip_base}.{base + i}" for i in range(count)]
    
    found_devices = []
    
    # Parallel scan
    with ThreadPoolExecutor(max_workers=20) as executor:
        results = list(executor.map(test_esp32_endpoint, ips))
    
    # Filter results
    found_devices = [r for r in results if r is not None]
    return found_devices

def check_wifi_networks():
    """Check for ESP32 AP networks"""
    try:
        print("📡 Scanning for ESP32 Access Point networks...")
        
        # Use system_profiler to check WiFi networks
        result = subprocess.run(['system_profiler', 'SPAirPortDataType'], 
                              capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            output = result.stdout.lower()
            
            # Look for ESP32 related networks
            esp32_indicators = ['esp32', 'fancontroller', 'fan-controller']
            
            for indicator in esp32_indicators:
                if indicator in output:
                    print(f"🎯 Possible ESP32 network containing '{indicator}' detected!")
                    return True
                    
    except Exception as e:
        print(f"⚠️ WiFi scan error: {e}")
    
    return False

def main():
    print("🎯 Comprehensive ESP32 Board #1 Finder")
    print("Target MAC: 44:1d:64:f5:b4:84")
    print("=" * 60)
    
    all_found_devices = []
    
    # 1. Check standard AP mode IP
    print("1️⃣ Checking standard ESP32 AP mode...")
    ap_result = test_esp32_endpoint("192.168.4.1")
    if ap_result:
        all_found_devices.append(ap_result)
        print(f"   ✅ Found ESP32 at 192.168.4.1 (AP Mode)")
    else:
        print(f"   ❌ No ESP32 at 192.168.4.1")
    
    # 2. Scan current network (10.0.1.x)
    current_network_devices = scan_ip_range("10.0.1.1", 254, "current network 10.0.1.0/24")
    all_found_devices.extend(current_network_devices)
    
    # 3. Common home router ranges
    common_ranges = [
        ("192.168.1.1", 254, "192.168.1.0/24 (common home network)"),
        ("192.168.0.1", 254, "192.168.0.0/24 (common home network)"),
        ("172.16.1.1", 100, "172.16.1.0/24 (private network)"),
        ("10.0.0.1", 254, "10.0.0.0/24 (private network)")
    ]
    
    for start_ip, count, description in common_ranges:
        devices = scan_ip_range(start_ip, count, description)
        all_found_devices.extend(devices)
        
        # Stop if we found something
        if devices:
            break
    
    # 4. Check for WiFi networks
    check_wifi_networks()
    
    print("\n" + "=" * 60)
    print("📊 SEARCH RESULTS")
    print("=" * 60)
    
    if all_found_devices:
        print(f"🎉 Found {len(all_found_devices)} ESP32 device(s):")
        print()
        
        target_mac = "44:1d:64:f5:b4:84"
        board_1_found = False
        
        for i, device in enumerate(all_found_devices, 1):
            print(f"Device #{i}:")
            print(f"   🔗 IP: {device['ip']}")
            print(f"   🏷️ MAC: {device['mac']}")
            print(f"   📡 WiFi: {device['wifi_mode']}")
            print(f"   🌐 Network: {device['network']}")
            print(f"   🌪️ Fan RPM: {device['fan_rpm']}")
            print(f"   ⚡ Fan Speed: {device['fan_speed']}%")
            print(f"   ⏱️ Uptime: {device['uptime']} seconds")
            print(f"   🌍 Web Interface: http://{device['ip']}")
            
            # Check if this is Board #1
            if device['mac'].lower().replace(':', '') == target_mac.lower().replace(':', ''):
                print(f"   ✅ CONFIRMED: This is Board #1!")
                board_1_found = True
            print()
        
        if board_1_found:
            print("🎯 SUCCESS: Board #1 found and accessible!")
        else:
            print("⚠️ ESP32 devices found, but none match Board #1's MAC address")
            
    else:
        print("❌ No ESP32 devices found on any scanned networks")
        print()
        print("🔧 TROUBLESHOOTING STEPS:")
        print("1. Check if ESP32 is powered on (serial shows 'Fan RPM: 0')")
        print("2. Verify HareNet WiFi credentials are correct")
        print("3. Check if router has MAC filtering enabled")
        print("4. Try manually connecting to 'ESP32-FanController' WiFi")
        print("5. Check router admin panel for connected devices")
    
    print("=" * 60)

if __name__ == "__main__":
    main()
