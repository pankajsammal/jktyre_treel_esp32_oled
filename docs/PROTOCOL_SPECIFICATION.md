# Treel TPMS BLE Protocol Specification

> **Target Devices:** JK Tyre TREEL SmartTyre BLE Pressure & Temperature Sensors  
> **Scope:** Technical specification of BLE advertisement payload formats, payload layouts, and telemetry decoding.

---

## 1. Overview & Operational Modes

TREEL TPMS sensors continuously broadcast tire telemetry over **Bluetooth Low Energy (BLE) advertisements**. Sensors do **not** require pairing or direct GATT connection; data is parsed purely from passive advertisement packet sniffing.

The system uses **two distinct BLE advertisement modes**:

| Feature / Mode | Mode 1: Apple iBeacon Format | Mode 2: SmartTyre Encrypted GATT Format |
| :--- | :--- | :--- |
| **Detection Filter** | UUID `FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFE0` | Service UUID `0000FFE0-0000-1000-8000-00805F9B34FB` |
| **Data Encryption** | None (Plaintext iBeacon fields) | **AES-128-ECB NoPadding** |
| **Security Key** | N/A | `#@Trl2018-lespl$` (16 bytes ASCII) |
| **Fields Transmitted** | Pressure (PSI), Temperature (°C) | Pressure (PSI), Temperature (°C), Battery %, Tamper & Vibration |
| **Battery Field** | Retains last known value (not transmitted) | Transmitted in decrypted Byte [5] |

---

## 2. Telemetry Format Specifications

### Mode 1: iBeacon Format

The iBeacon advertisement contains standard Apple iBeacon fields (`0x02 0x15` identifier followed by a 16-byte UUID):

```text
Raw Advertisement Byte Layout:
Offset 0..1  : Apple Manufacturer Header (0x4C 0x00)
Offset 2     : iBeacon Indicator (0x02)
Offset 3     : Length (0x15 = 21 bytes)
Offset 4..19 : UUID = FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFE0
Offset 20..21: Major (2 Bytes, Big-Endian) -> Sensor ID
Offset 22..23: Minor (2 Bytes, Big-Endian) -> Pressure in low-byte (Minor & 0xFF)
Offset 24    : TxPower (1 Byte, Unsigned) -> Temperature Formula
```

#### iBeacon Field Parsing Formulas:

1. **Pressure (PSI)**:
   ```text
   Pressure (PSI) = Minor & 0xFF
   ```

2. **Temperature (°C)**:
   ```text
   If TxPower > 65:
       Temperature (°C) = TxPower - 110
   Else:
       Temperature = 0 (Invalid / No Data)
   ```

---

### Mode 2: SmartTyre Encrypted GATT Format

The encrypted advertisement contains a 16-byte payload inside the scan record (starting at offset 15).

```text
Encryption Algorithm : AES/ECB/NoPadding
Cipher Key           : #@Trl2018-lespl$   (Hex: 23 40 54 72 6C 32 30 31 38 2D 6C 65 73 70 6C 24)
Input Buffer         : 16 encrypted bytes
Output Buffer        : 16 decrypted bytes
```

#### Decrypted 16-Byte Payload Layout:

```text
┌────┬──────────┬──────────┬────┬────┬────┬────┬────┬──────────┬──────────┬──────────────┐
│ 0  │  1   2   │  3   4   │ 5  │ 6  │ 7  │ 8  │ 9  │  10  11  │  12  13  │  14     15   │
├────┼──────────┼──────────┼────┼────┼────┼────┼────┼──────────┼──────────┼──────────────┤
│Tag │ Temp(LE) │ Press(LE)│Bat │Flag│TirT│TagT│Imp │ VibX(LE) │ VibZ(LE) │ Int Temp/FW  │
│Type│  /100°C  │ /100 PSI │ %  │    │Cnt │Cnt │Cnt │  /1000g  │  /1000g  │              │
└────┴──────────┴──────────┴────┴────┴────┴────┴────┴──────────┴──────────┴──────────────┘
```

#### Encrypted Field Parsing Formulas:

1. **Data Type Header** (`Byte [0]`):
   Must evaluate to `0x16` (22 decimal) for valid Treel TPMS telemetry.

2. **Surface Temperature (°C)** (`Bytes [1..2]`, Little-Endian):
   ```text
   Raw = Byte[1] | (Byte[2] << 8)
   
   If Raw == 0xFFFF (65535):
       Invalid / No Data
   Else If Raw <= 32768:
       Temperature (°C) = Raw / 100.0
   Else:
       Temperature (°C) = -((Raw - 32768) / 100.0)
   ```

3. **Tire Pressure (PSI)** (`Bytes [3..4]`, Little-Endian):
   ```text
   Raw = Byte[3] | (Byte[4] << 8)

   If Raw == 0xFFFF (65535):
       Invalid / No Data
   Else:
       Pressure (PSI) = Raw / 100.0
   ```

4. **Battery Level (%)** (`Byte [5]`):
   ```text
   Battery (%) = Min(100, Byte[5] & 0xFF)
   ```

5. **Accelerometers X & Z (g-force)** (`Bytes [10..11]` & `[12..13]`, Little-Endian):
   ```text
   Accel (g) = (Byte[low] | (Byte[high] << 8)) / 1000.0
   ```

---

## 3. Dual-Endian MAC Matching & Short Sensor IDs

Some Android BLE chipsets reverse MAC address byte order in advertisement reports. To guarantee robust matching across all hardware:

- **Forward MAC Format**: `D2:58:6D:8F:16:10`
- **Reversed MAC Format**: `10:16:8F:6D:58:D2`
- **Short 6-Char ID**: `8F1610` (extracted from last 6 characters of Forward MAC)

Matching algorithms should check for both Forward MAC, Reversed MAC, and Short 6-character IDs against sensor whitelists.
