# Windows Desktop Python CLI & Diagnostic Tool Guide

This guide explains how to install, set up, and run the Python command-line utility and diagnostic tool on Windows for sniffing and decoding **JK Tyre TREEL TPMS Sensors**.

---

## 1. Prerequisites

Before starting, ensure you have:
1. **Windows 10 / Windows 11** with built-in Bluetooth 4.0/5.0 adapter or USB BLE Dongle.
2. **Python 3.9 or higher** installed on your system.
   - Download from [python.org](https://www.python.org/downloads/) if not installed.
   - **Important:** Make sure to check **"Add Python to PATH"** during installation.

---

## 2. Easy Step-by-Step Installation

### Step 1: Open Terminal / Command Prompt
Press `Win + R`, type `cmd`, and press **Enter**.

### Step 2: Navigate to the Python Tool Directory
```cmd
cd tools/python_tpms_app
```

### Step 3: Create a Virtual Environment (Recommended)
Creating a virtual environment keeps project dependencies isolated and clean:
```cmd
python -m venv venv
```

Activate the virtual environment:
```cmd
venv\Scripts\activate
```
*(You will see `(venv)` appear at the start of your command prompt line).*

### Step 4: Install Dependencies
Install required packages (`bleak` for Windows BLE and `pycryptodome` for AES decryption):
```cmd
pip install -r requirements.txt
```

### Step 5: Configure Your Sensor MAC Addresses
> [!IMPORTANT]
> Before running the scanner, edit `core/scanner.py` (lines 24-29) and replace the sample MAC addresses with your actual sensor MACs.
>
> You can find your sensor MAC addresses in the **JK Tyre SmartTyre app** (under **Settings -> Sensor Debug**) or by running `python diagnose_ble.py`.

```python
WHITELISTED_SENSORS: Dict[str, Dict[str, str]] = {
    "YOUR_FL_MAC": {"pos": "FL", "name": "Front Left", "short_id": "FL_ID"},
    "YOUR_FR_MAC": {"pos": "FR", "name": "Front Right", "short_id": "FR_ID"},
    "YOUR_RL_MAC": {"pos": "RL", "name": "Rear Left", "short_id": "RL_ID"},
    "YOUR_RR_MAC": {"pos": "RR", "name": "Rear Right", "short_id": "RR_ID"},
}
```

---

## 3. Tool Usage Guide

### Tool 1: Live Hardware BLE Packet Scanner (`tpms_cli.py`)

Scans for nearby TREEL TPMS sensors using Windows native Bluetooth API (WinRT) and prints real-time colorized telemetry in Command Prompt.

Run the hardware scanner:
```cmd
python tpms_cli.py
```

#### What You Will See in the Console:
```
==================================================================
        JK TYRE TREEL TPMS — BLE PACKET SNIFFER (WINDOWS)        
==================================================================
[INFO] Starting Hardware BLE Scanner (WinRT)...
Press Ctrl+C to stop scanning.

[12:30:15] Sensor: 8F1610    | Pos: FL    | Mode: GATT / Encrypted | Press:  32.5 PSI (2.24 bar) | Temp: 30.0 °C | Batt: 100% | RSSI:  -65 dBm | Status: NORMAL
[12:30:18] Sensor: 2D9215    | Pos: FR    | Mode: Beacon          | Press:  33.0 PSI (2.27 bar) | Temp: 31.0 °C | Batt:  --% | RSSI:  -72 dBm | Status: NORMAL
```

All packets are automatically logged to `tpms_packets.csv` and raw captures to `tpms_raw_capture.csv` in the same directory.

---

### Tool 2: BLE Diagnostic & Discovery Tool (`diagnose_ble.py`)

If you want to discover all BLE devices around you or find raw manufacturer byte streams from your TPMS sensors:

Run the diagnostic scanner:
```cmd
python diagnose_ble.py
```

#### Sample Output:
```
==================================================================
        RAW BLE ADVERTISEMENT DIAGNOSTIC SCANNER (WINDOWS)       
==================================================================
Scanning for 15 seconds... Turn on your TPMS sensors or move nearby.

[BLE DEVICE DETECTED] MAC: D2:58:6D:8F:16:10 | Name: Treel TPMS | RSSI: -64 dBm
  Manufacturer Data:
    - Company 0x004C: 0215FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE08F1610212187
    *** MATCHED TREEL TPMS SENSOR ***
    Sensor ID: 8F1610 | Pos: FL
    Pressure: 33.0 PSI | Temp: 25.0 °C | Mode: Beacon
```

---

### Tool 3: Offline Single Hex String Decoder

You can decode any saved raw advertisement hex payload string without hardware BLE scanning:

Run:
```cmd
python tpms_cli.py --decode 0215FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE08F1610212187
```

#### Output:
```
=== Decoded Treel TPMS Reading ===
  mac_address           : UNKNOWN
  sensor_id             : 8F16-1021
  pressure_psi          : 33.0
  temperature_celsius   : 25.0
  battery_percent       : -1
  mode                  : Beacon
  rssi                  : -100
  is_valid              : True
```

---

### Tool 4: Simulator Mode (No Hardware Needed)

If you don't have BLE hardware connected or want to test script behavior:
```cmd
python tpms_cli.py --simulate
```

---

## 4. Troubleshooting Checklist

1. **Error: `No Bluetooth Adapters Found`**:
   - Ensure Bluetooth is switched ON in Windows Settings (**Settings -> Bluetooth & devices**).
   - If using a USB Bluetooth dongle, ensure drivers are installed properly.

2. **No Sensors Detected**:
   - TPMS sensors broadcast data periodically (every 1 to 10 seconds depending on movement).
   - Ensure the sensors have battery and are within range (~5 meters).

3. **Virtual Environment Reminder**:
   - Always remember to activate `venv\Scripts\activate` before running the commands.
