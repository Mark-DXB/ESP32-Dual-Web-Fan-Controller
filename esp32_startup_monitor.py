#!/usr/bin/env python3
import serial
import time
import re

def monitor_esp32_startup():
    try:
        print("🔄 Monitoring ESP32 Board #1 startup sequence...")
        print("🎯 Looking for WiFi connection details...")
        print("=" * 60)
        
        ser = serial.Serial('/dev/cu.usbserial-0001', 115200, timeout=2)
        time.sleep(1)
        
        # Reset the ESP32 to capture startup
        print("🔌 Resetting ESP32...")
        ser.dtr = False
        time.sleep(0.1)
        ser.dtr = True
        time.sleep(0.5)
        ser.dtr = False
        time.sleep(1)
        
        print("📡 Capturing startup logs...")
        print("-" * 60)
        
        wifi_info = {}
        startup_complete = False
        start_time = time.time()
        
        while (time.time() - start_time) < 30 and not startup_complete:
            if ser.in_waiting:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                        
                        # Look for WiFi information
                        if "IP Address:" in line:
                            ip_match = re.search(r'IP Address: (\d+\.\d+\.\d+\.\d+)', line)
                            if ip_match:
                                wifi_info['ip'] = ip_match.group(1)
                                
                        elif "AP IP Address:" in line:
                            ap_ip_match = re.search(r'AP IP Address: (\d+\.\d+\.\d+\.\d+)', line)
                            if ap_ip_match:
                                wifi_info['ap_ip'] = ap_ip_match.group(1)
                                wifi_info['mode'] = 'AP'
                                
                        elif "Network:" in line:
                            network_match = re.search(r'Network: (.+)', line)
                            if network_match:
                                wifi_info['network'] = network_match.group(1)
                                wifi_info['mode'] = 'Station'
                                
                        elif "AP Network:" in line:
                            ap_network_match = re.search(r'AP Network: (.+)', line)
                            if ap_network_match:
                                wifi_info['ap_network'] = ap_network_match.group(1)
                                
                        elif "Signal Strength:" in line:
                            signal_match = re.search(r'Signal Strength: (-?\d+) dBm', line)
                            if signal_match:
                                wifi_info['signal'] = signal_match.group(1)
                                
                        elif "Web Interface:" in line:
                            web_match = re.search(r'Web Interface: (http://[^\s]+)', line)
                            if web_match:
                                wifi_info['web_interface'] = web_match.group(1)
                                
                        elif "System Ready" in line:
                            startup_complete = True
                            
                except:
                    pass
            time.sleep(0.1)
        
        print("-" * 60)
        print("📊 WIFI CONNECTION SUMMARY:")
        print("=" * 60)
        
        if wifi_info:
            if wifi_info.get('mode') == 'Station':
                print(f"✅ WiFi Mode: Station (Connected to router)")
                print(f"🌐 Network: {wifi_info.get('network', 'Unknown')}")
                print(f"🔗 IP Address: {wifi_info.get('ip', 'Unknown')}")
                print(f"📶 Signal: {wifi_info.get('signal', 'Unknown')} dBm")
                if 'web_interface' in wifi_info:
                    print(f"🌍 Web Interface: {wifi_info['web_interface']}")
                    
            elif wifi_info.get('mode') == 'AP':
                print(f"🟡 WiFi Mode: Access Point (Fallback)")
                print(f"📡 AP Network: {wifi_info.get('ap_network', 'Unknown')}")
                print(f"🔗 AP IP: {wifi_info.get('ap_ip', 'Unknown')}")
                print(f"📋 Instructions: Connect to AP network, then access web interface")
                
            print("=" * 60)
        else:
            print("❌ No WiFi information captured")
            print("💡 ESP32 might still be booting or having WiFi issues")
        
        ser.close()
        return wifi_info
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return {}

if __name__ == "__main__":
    wifi_info = monitor_esp32_startup()
    
    if wifi_info and 'ip' in wifi_info:
        print(f"\n🎯 NEXT STEP: Try accessing http://{wifi_info['ip']}")
    elif wifi_info and 'ap_ip' in wifi_info:
        print(f"\n🎯 NEXT STEP: Connect to '{wifi_info.get('ap_network')}' and access http://{wifi_info['ap_ip']}")
    else:
        print("\n💭 Startup monitoring complete")
