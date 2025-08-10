#!/usr/bin/env python3
"""
Display Diagnostics Script for HT1632 4-Panel Display
Monitors display health and provides troubleshooting tools for the 4th panel going blank issue.
"""

import time
import subprocess
import json
import sys
import argparse
from datetime import datetime, timedelta

def run_mqtt_command(command_json, topic="display/state", host="localhost", port=1883):
    """Send MQTT command to display"""
    cmd = [
        "mosquitto_pub", 
        "-h", host, 
        "-p", str(port),
        "-t", topic, 
        "-m", json.dumps(command_json)
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
        if result.returncode != 0:
            print(f"MQTT command failed: {result.stderr}")
            return False
        return True
    except subprocess.TimeoutExpired:
        print("MQTT command timed out")
        return False
    except FileNotFoundError:
        print("mosquitto_pub not found. Please install mosquitto-clients")
        return False

def test_panel_sequence():
    """Test each panel individually to identify which one fails"""
    print("Testing individual panels...")
    
    # Test patterns for each panel (32 pixels wide each)
    test_patterns = [
        "Panel 1: ████████████████████████████████",  # Panel 1 (leftmost)
        "Panel 2: ████████████████████████████████",  # Panel 2
        "Panel 3: ████████████████████████████████",  # Panel 3  
        "Panel 4: ████████████████████████████████",  # Panel 4 (rightmost)
    ]
    
    for i, pattern in enumerate(test_patterns, 1):
        print(f"\nTesting Panel {i}...")
        success = run_mqtt_command({
            "text": pattern,
            "scroll": "disabled",
            "brightness": 10
        })
        
        if success:
            print(f"✓ Panel {i} test command sent")
            input(f"Press Enter after verifying Panel {i} display...")
        else:
            print(f"✗ Panel {i} test command failed")
    
    # Clear display
    run_mqtt_command({"clear": True})

def stress_test(duration_minutes=10):
    """Perform stress test with frequent updates to trigger the issue"""
    print(f"Starting {duration_minutes}-minute stress test...")
    print("This will rapidly update the display to try to trigger the 4th panel issue")
    
    start_time = datetime.now()
    end_time = start_time + timedelta(minutes=duration_minutes)
    counter = 0
    
    try:
        while datetime.now() < end_time:
            # Alternate between different patterns
            patterns = [
                f"Stress Test #{counter:06d} - Scrolling Text",
                f"COUNT: {counter:08d} - ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                f"Time: {datetime.now().strftime('%H:%M:%S')} - Counter: {counter}",
            ]
            
            pattern = patterns[counter % len(patterns)]
            
            success = run_mqtt_command({
                "text": pattern,
                "scroll": "enabled",
                "brightness": 8 + (counter % 8),  # Vary brightness
                "show_time": counter % 3 == 0  # Sometimes show time
            })
            
            if not success:
                print(f"\n✗ Command failed at counter {counter}")
                break
                
            counter += 1
            
            # Status update every 100 iterations
            if counter % 100 == 0:
                elapsed = datetime.now() - start_time
                print(f"Stress test: {counter} updates in {elapsed.total_seconds():.1f}s")
                print("Check if 4th panel is still working...")
            
            time.sleep(0.1)  # 10Hz update rate
            
    except KeyboardInterrupt:
        print(f"\nStress test interrupted after {counter} updates")
    
    print(f"\nStress test completed: {counter} updates")
    print("Final check - displaying test pattern...")
    
    # Final test pattern
    run_mqtt_command({
        "text": "STRESS TEST COMPLETE - CHECK ALL 4 PANELS",
        "scroll": "disabled",
        "brightness": 15
    })

def monitor_mode(check_interval=30):
    """Monitor mode - periodically check display health"""
    print(f"Starting monitor mode (checking every {check_interval} seconds)")
    print("Press Ctrl+C to stop")
    
    test_counter = 0
    
    try:
        while True:
            test_counter += 1
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            
            # Send test pattern
            test_text = f"Health Check #{test_counter:04d} - {timestamp}"
            success = run_mqtt_command({
                "text": test_text,
                "scroll": "enabled" if len(test_text) > 32 else "disabled",
                "brightness": 10,
                "show_time": True
            })
            
            if success:
                print(f"[{timestamp}] ✓ Health check #{test_counter} sent")
            else:
                print(f"[{timestamp}] ✗ Health check #{test_counter} failed")
            
            time.sleep(check_interval)
            
    except KeyboardInterrupt:
        print(f"\nMonitoring stopped after {test_counter} checks")

def diagnostic_info():
    """Display diagnostic information"""
    print("HT1632 Display Diagnostic Information")
    print("=" * 50)
    print()
    
    print("Hardware Configuration:")
    print("- 4 panels, 32x8 pixels each (128x8 total)")
    print("- HT1632 controllers")
    print("- SPI communication at 200kHz")
    print("- CS pins: 8, 9, 15, 16 (wiringPi numbering)")
    print()
    
    print("Common Issues:")
    print("1. 4th Panel Going Blank:")
    print("   - Most likely: Signal integrity issues")
    print("   - Power supply voltage drops")
    print("   - Controller state corruption")
    print("   - Timing violations in SPI communication")
    print()
    
    print("Recent Fixes Applied:")
    print("- Increased CS pin setup delays (2μs → 5μs)")
    print("- Added delays before CS deselection")
    print("- Extended delay for 4th panel (10μs vs 5μs)")
    print("- Inter-panel delays for signal integrity")
    print("- Time-based gentle reinitialization (default: 60 minutes)")
    print("- No SYS_DIS used - prevents flickering working displays")
    print("- Brightness restoration after reinit")
    print()
    
    print("Troubleshooting Steps:")
    print("1. Run panel sequence test: --test-panels")
    print("2. Run stress test: --stress-test 30")
    print("3. Monitor continuously: --monitor")
    print("4. Check power supply voltage (should be stable 5V)")
    print("5. Verify SPI wiring integrity")
    print("6. Consider shorter/better quality cables")

def main():
    parser = argparse.ArgumentParser(description="HT1632 Display Diagnostics")
    parser.add_argument("--test-panels", action="store_true", 
                       help="Test each panel individually")
    parser.add_argument("--stress-test", type=int, metavar="MINUTES",
                       help="Run stress test for specified minutes")
    parser.add_argument("--monitor", action="store_true",
                       help="Monitor display health continuously")
    parser.add_argument("--monitor-interval", type=int, default=30,
                       help="Monitor check interval in seconds (default: 30)")
    parser.add_argument("--mqtt-host", default="localhost",
                       help="MQTT broker host (default: localhost)")
    parser.add_argument("--mqtt-port", type=int, default=1883,
                       help="MQTT broker port (default: 1883)")
    
    args = parser.parse_args()
    
    if args.test_panels:
        test_panel_sequence()
    elif args.stress_test:
        stress_test(args.stress_test)
    elif args.monitor:
        monitor_mode(args.monitor_interval)
    else:
        diagnostic_info()

if __name__ == "__main__":
    main()
