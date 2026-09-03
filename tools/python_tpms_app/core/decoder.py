"""
Treel TPMS BLE Packet Decoder.
Decoder for Treel SmartTyre TPMS (Beacon & Encrypted GATT modes).
"""

import uuid
from typing import Optional
from Crypto.Cipher import AES
from .models import TireReading

# Protocol constants
TREEL_BEACON_UUID = "ffffffff-ffff-ffff-ffff-ffffffffffe0"
TREEL_GATT_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
AES_KEY = b"#@Trl2018-lespl$"


class TreelDecoder:
    """Decoder for Treel TPMS BLE advertisements."""

    def __init__(self, aes_key: bytes = AES_KEY):
        self.aes_key = aes_key

    def aes_decrypt(self, encrypted_bytes: bytes) -> Optional[bytes]:
        """Decrypt 16-byte payload using AES-128-ECB NoPadding."""
        if len(encrypted_bytes) != 16:
            return None
        try:
            cipher = AES.new(self.aes_key, AES.MODE_ECB)
            return cipher.decrypt(encrypted_bytes)
        except Exception:
            return None

    def decode_raw_payload(self, raw_bytes: bytes, mac_address: str = "UNKNOWN", rssi: int = -100) -> Optional[TireReading]:
        """
        Main entry point for decoding any raw BLE advertisement byte string or buffer.
        Prioritizes Treel iBeacon packets before attempting AES-128 decryption.
        """
        # 1. Try Beacon mode decoding first (prevents false AES decryption on beacon packets)
        reading = self.decode_ibeacon(raw_bytes, mac_address=mac_address, rssi=rssi)
        if reading is not None:
            return reading

        # 2. Try Encrypted GATT mode
        reading = self.decode_encrypted_gatt(raw_bytes, mac_address=mac_address, rssi=rssi)
        if reading is not None:
            return reading

        return None

    def decode_ibeacon(self, raw_bytes: bytes, mac_address: str = "UNKNOWN", rssi: int = -100) -> Optional[TireReading]:
        """
        Parse raw iBeacon advertisement record for Treel Beacon mode.
        Looks for 0x02 0x15 iBeacon marker and matching UUID.
        Required structure from marker: 0x02 0x15 [16B UUID] [2B Major] [2B Minor] [1B TX Power] = 23 bytes.
        """
        raw_len = len(raw_bytes)
        if raw_len < 23:
            return None

        # Search for 0x02 0x15 marker in payload
        for offset in range(0, raw_len - 22):
            if raw_bytes[offset] == 0x02 and raw_bytes[offset + 1] == 0x15:
                beacon_uuid_bytes = raw_bytes[offset + 2 : offset + 18]
                if len(beacon_uuid_bytes) < 16:
                    continue

                beacon_uuid = str(uuid.UUID(bytes=beacon_uuid_bytes)).lower()
                if beacon_uuid != TREEL_BEACON_UUID:
                    continue

                # Found Treel TPMS Beacon!
                major = (raw_bytes[offset + 18] << 8) | raw_bytes[offset + 19]
                minor = (raw_bytes[offset + 20] << 8) | raw_bytes[offset + 21]
                tx_power_temp = raw_bytes[offset + 22] & 0xFF

                # Pressure is in low byte of minor (minor & 0xFF)
                pressure_psi = float(minor & 0xFF)

                # Temperature calculation: if tx_power_temp > 65 -> temp = raw - 110
                if tx_power_temp > 65:
                    temp_celsius = float(tx_power_temp - 110)
                else:
                    temp_celsius = 0.0

                if not (0.0 <= pressure_psi <= 217.0 and -40.0 <= temp_celsius <= 125.0):
                    continue

                sensor_id = f"{major:04X}-{minor:04X}"
                raw_hex = raw_bytes.hex().upper()

                return TireReading(
                    mac_address=mac_address,
                    sensor_id=sensor_id,
                    pressure_psi=pressure_psi,
                    temperature_celsius=temp_celsius,
                    battery_percent=-1,  # Not available in beacon mode
                    mode="Beacon",
                    rssi=rssi,
                    raw_hex=raw_hex,
                    status_flags="00000000",
                    is_valid=True
                )

        return None

    def decode_encrypted_gatt(self, scan_record: bytes, mac_address: str = "UNKNOWN", rssi: int = -100) -> Optional[TireReading]:
        """
        Parse encrypted GATT advertisement record.
        Scans all 16-byte slices for valid AES-128 decrypted TPMS payload.
        """
        raw_len = len(scan_record)
        if raw_len < 16:
            return None

        # If payload contains the Treel iBeacon UUID, skip GATT decryption
        if bytes.fromhex(TREEL_BEACON_UUID.replace("-", "")) in scan_record:
            return None

        # Scan all possible 16-byte offsets in the payload
        for offset in range(0, raw_len - 15):
            encrypted_16 = scan_record[offset : offset + 16]
            decrypted = self.aes_decrypt(encrypted_16)
            if not decrypted or len(decrypted) < 16:
                continue

            data_type = decrypted[0] & 0xFF
            # Treel TPMS GATT telemetry packets strictly start with 0x16 (22 decimal)
            if data_type != 0x16:
                continue

            # Bytes 1-2: Surface temperature (Little-Endian)
            raw_temp = decrypted[1] | (decrypted[2] << 8)
            if raw_temp == 65535:
                continue
            elif raw_temp <= 32768:
                temp_c = round(raw_temp / 100.0, 1)
            else:
                temp_c = round(-((raw_temp - 32768) / 100.0), 1)

            # Bytes 3-4: Pressure in PSI (Little-Endian)
            raw_press = decrypted[3] | (decrypted[4] << 8)
            if raw_press == 65535:
                continue
            else:
                press_psi = round(raw_press / 100.0, 1)

            battery = decrypted[5] & 0xFF

            # Sanity validation check for GATT mode: valid temp & pressure & battery
            is_valid = (-40.0 <= temp_c <= 125.0) and (0.0 <= press_psi <= 217.0) and (battery <= 100)
            if not is_valid:
                continue

            status_flags = bin((decrypted[6] & 0xFF) + 256)[3:]

            raw_accel_x = decrypted[10] | (decrypted[11] << 8)
            accel_x = round(raw_accel_x / 1000.0, 3)

            raw_accel_y = decrypted[12] | (decrypted[13] << 8)
            accel_y = round(raw_accel_y / 1000.0, 3)

            sensor_id = mac_address.replace(":", "").replace("-", "")[-6:].upper() if mac_address != "UNKNOWN" else "TREEL-SENS"

            return TireReading(
                mac_address=mac_address,
                sensor_id=sensor_id,
                pressure_psi=press_psi,
                temperature_celsius=temp_c,
                battery_percent=battery,
                mode="GATT / Encrypted",
                rssi=rssi,
                raw_hex=scan_record.hex().upper(),
                status_flags=status_flags,
                accel_x_g=accel_x,
                accel_y_g=accel_y,
                is_valid=True
            )

        return None

    def decode_hex(self, hex_string: str, mac_address: str = "UNKNOWN", rssi: int = -100) -> Optional[TireReading]:
        """Clean hex string and parse."""
        cleaned = hex_string.replace(" ", "").replace("0x", "").replace("-", "").strip()
        try:
            raw_bytes = bytes.fromhex(cleaned)
            return self.decode_raw_payload(raw_bytes, mac_address=mac_address, rssi=rssi)
        except ValueError:
            return None
