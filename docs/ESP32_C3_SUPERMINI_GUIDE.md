# ESP32-C3 SuperMini Setup Guide — Treel TPMS BLE Receiver

This step-by-step guide explains how to configure, wire, and flash the **Treel TPMS BLE Receiver & Web Server** on the ultra-compact **ESP32-C3 SuperMini** board.

---

## 1. Board Overview & Highlights

The **ESP32-C3 SuperMini** is an ideal microcontroller for discreet in-car installation:
- **Ultra-Small Form Factor** (~22mm x 18mm)
- **160 MHz 32-bit RISC-V Processor** with integrated Wi-Fi and Bluetooth 5.0 LE
- **Hardware AES Engine**: Hardware-accelerated decryption of Treel TPMS packets with low power consumption
- **Headless Mode Support**: Can run standalone without any screen, serving live dashboard data over Wi-Fi

---

## 2. Arduino IDE Board Configuration

Before flashing the sketch, set the correct board parameters in Arduino IDE:

1. Go to **Tools -> Board -> esp32 -> ESP32C3 Dev Module**.
2. Set the following parameters under **Tools**:

| Setting | Selection | Critical Note |
| :--- | :--- | :--- |
| **Board** | **ESP32C3 Dev Module** | Core board selection |
| **USB CDC On Boot** | **Enabled** | **CRITICAL:** Must be Enabled for Serial Monitor output over USB-C! |
| **Flash Frequency** | **80MHz** | Standard flash speed |
| **Flash Mode** | **QIO** (or DIO) | Flash memory mode |
| **Flash Size** | **4MB (32Mb)** | Standard SuperMini flash size |
| **Upload Speed** | **921600** (or 115200) | Baud rate for uploading |
| **Port** | Select COM Port | Your board's USB-C serial port |

---

## 3. Flashing Mode Instructions (If USB Port is Not Detected)

If your ESP32-C3 SuperMini is not recognized by your computer:
1. Press and hold the **BOOT** button on the board.
2. Connect the USB-C cable to your computer while holding **BOOT**.
3. Release the **BOOT** button.
4. Select the newly detected COM port in Arduino IDE and click **Upload**.

---

## 4. Hardware Wiring Options

### Board Pinout Diagram

```
                 [ ESP32-C3 SuperMini ]
                         USB-C
  (Pin 1) GPIO 5  [ ]               [ ] 5V    (Pin 1)
  (Pin 2) GPIO 6  [ ]               [ ] GND   (Pin 2 - 'G')   ---> OLED GND
  (Pin 3) GPIO 7  [ ]               [ ] 3.3V  (Pin 3 - '3.3') ---> OLED VCC
  (Pin 4) GPIO 8  [ ] [BOOT]  [RST] [ ] GPIO 4(Pin 4)         ---> OLED SDA (Option B)
          (SDA)    |                 |
  (Pin 5) GPIO 9  [ ]    [ESP32]    [ ] GPIO 3(Pin 5)         ---> OLED SCL (Option B)
          (SCL)   |                 |
  (Pin 6) GPIO 10 [ ]    [ C3  ]    [ ] GPIO 2(Pin 6)
  (Pin 7) GPIO 20 [ ]               [ ] GPIO 1(Pin 7)
  (Pin 8) GPIO 21 [ ]               [ ] GPIO 0(Pin 8)
```

---

### Option A: Standard Hardware I2C (Default)

Use standard hardware I2C pins (GPIO 8 & GPIO 9) on the left header:

| OLED Display Pin | ESP32-C3 Pin | Location on Board |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Right Header, Pin 3 (`3.3`) |
| **GND** | **GND** | Right Header, Pin 2 (`G`) |
| **SDA** | **GPIO 8** | Left Header, Pin 4 |
| **SCL** | **GPIO 9** | Left Header, Pin 5 |

---

### Option B: 100% Single-Side Wiring (Right Header Consecutive Pins)

Wire all 4 display pins cleanly to consecutive pins on the right header:

| OLED Display Pin | ESP32-C3 Pin | Location on Board |
| :--- | :--- | :--- |
| **GND** | **GND** | Right Header, Pin 2 (`G`) |
| **VCC** | **3.3V** | Right Header, Pin 3 (`3.3`) |
| **SDA** | **GPIO 4** | Right Header, Pin 4 |
| **SCL** | **GPIO 3** | Right Header, Pin 5 |

*If using Option B, update pin definitions in `esp32_tpms_webserver.ino`:*
```cpp
#define OLED_SDA_PIN 4
#define OLED_SCL_PIN 3
```

---

## 5. Running Headless (No Display Mode)

If you are placing the board inside a car dashboard without a screen:
In `esp32_tpms_webserver.ino`:
```cpp
#define ENABLE_OLED false
```
- Completely disables I2C bus initialization.
- Saves RAM and CPU cycles.
- View live tire telemetry from your phone or tablet browser at `http://192.168.4.1` or your Wi-Fi router IP.
