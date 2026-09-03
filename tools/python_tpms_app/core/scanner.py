"""
BLE Scanner Engine for Treel TPMS Application.
Supports Windows WinRT active scanning via Bleak.
"""

import asyncio
import logging
import random
from typing import Callable, Dict, List, Optional

from bleak import BleakScanner
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData

from .decoder import TreelDecoder
from .models import TireReading

logger = logging.getLogger("TreelScanner")


class TreelBLEScanner:
    """Manages BLE hardware scanning and packet callback dispatch."""

    # IMPORTANT: Replace these sample MAC addresses and short IDs with your actual sensor MACs!
    # Find your MAC addresses in the official JK Tyre SmartTyre app under Settings -> Sensor Debug.
    WHITELISTED_SENSORS: Dict[str, Dict[str, str]] = {
        "D2:58:6D:8F:16:10": {"pos": "FL", "name": "Front Left", "short_id": "8F1610"},
        "CA:E8:6C:2D:92:15": {"pos": "FR", "name": "Front Right", "short_id": "2D9215"},
        "F7:FC:85:AD:35:E2": {"pos": "RL", "name": "Rear Left", "short_id": "AD35E2"},
        "CD:8D:E6:9E:FB:E6": {"pos": "RR", "name": "Rear Right", "short_id": "9EFBE6"},
    }

    def __init__(self, callback: Optional[Callable[[TireReading], None]] = None, raw_callback: Optional[Callable[[dict], None]] = None):
        self.decoder = TreelDecoder()
        self.callback = callback
        self.raw_callback = raw_callback
        self.scanner: Optional[BleakScanner] = None
        self.is_scanning = False
        self.recent_readings: Dict[str, TireReading] = {}
        self.cached_battery: Dict[str, int] = {}

    @classmethod
    def resolve_sensor(cls, mac_address: str) -> Optional[Dict[str, str]]:
        """
        Resolve sensor info matching whitelist.
        Returns None if the MAC is not one of the whitelisted sensors.
        """
        if not mac_address:
            return None
        clean_mac = mac_address.replace(":", "").replace("-", "").replace(" ", "").upper()

        for mac_str, info in cls.WHITELISTED_SENSORS.items():
            clean_target = mac_str.replace(":", "").replace("-", "").replace(" ", "").upper()
            if clean_mac == clean_target or (len(clean_mac) >= 6 and clean_target.endswith(clean_mac[-6:])):
                return {
                    "mac": mac_str,
                    "pos": info["pos"],
                    "name": info["name"],
                    "short_id": info["short_id"]
                }
        return None

    def _process_advertisement(self, device: BLEDevice, adv: AdvertisementData):
        """
        Process incoming BLE advertisement from TPMS sensors.
        """
        mac = device.address or ""
        sensor_info = self.resolve_sensor(mac)
        
        pos = sensor_info["pos"] if sensor_info else "UNKNOWN"
        canonical_mac = sensor_info["mac"] if sensor_info else mac
        rssi = adv.rssi or -100

        mfg_hex_list = []
        payloads_to_test: List[bytes] = []

        # 1. Gather all raw byte buffers in advertisement
        if adv.manufacturer_data:
            for company_id, mfg_bytes in adv.manufacturer_data.items():
                mfg_hex_list.append(f"0x{company_id:04X}:{mfg_bytes.hex().upper()}")
                payloads_to_test.append(mfg_bytes)
                if company_id == 0x004C:
                    payloads_to_test.append(bytes([len(mfg_bytes) + 1, 0xFF, 0x4C, 0x00]) + mfg_bytes)
                else:
                    comp_bytes = company_id.to_bytes(2, byteorder='little')
                    payloads_to_test.append(bytes([len(mfg_bytes) + 3, 0xFF]) + comp_bytes + mfg_bytes)

        svc_hex_list = []
        if adv.service_data:
            for s_uuid, s_bytes in adv.service_data.items():
                svc_hex_list.append(f"{s_uuid}:{s_bytes.hex().upper()}")
                payloads_to_test.append(s_bytes)

        if self.raw_callback:
            self.raw_callback({
                "mac": canonical_mac,
                "name": device.name or adv.local_name or f"Treel TPMS ({pos})",
                "rssi": rssi,
                "position": pos,
                "manufacturer_data": mfg_hex_list,
                "service_data": svc_hex_list,
                "service_uuids": adv.service_uuids or []
            })

        reading: Optional[TireReading] = None
        for p in payloads_to_test:
            reading = self.decoder.decode_raw_payload(p, mac_address=canonical_mac, rssi=rssi)
            if reading:
                break

        # 2. Check if reading was decoded successfully
        if reading:
            reading.tire_position = pos
            reading.mac_address = canonical_mac

            # Retain battery level if previously known
            if reading.battery_percent >= 0:
                self.cached_battery[pos] = reading.battery_percent
            elif pos in self.cached_battery:
                reading.battery_percent = self.cached_battery[pos]

            reading.alert_level = reading.evaluate_alert()
            self.recent_readings[pos] = reading

            if self.callback:
                self.callback(reading)

    async def start(self):
        """Start Windows BLE scanning using Bleak."""
        if self.is_scanning:
            return
        logger.info("Starting Bleak BLE hardware scanner...")
        try:
            self.scanner = BleakScanner(detection_callback=self._process_advertisement)
            await self.scanner.start()
            self.is_scanning = True
            logger.info("Bleak BLE hardware scanner started successfully.")
        except Exception as e:
            self.is_scanning = False
            logger.warning(f"Could not start BLE scanner: {e}")
            raise e

    async def stop(self):
        """Stop BLE scanning."""
        if not self.is_scanning or not self.scanner:
            return
        logger.info("Stopping BLE scanner...")
        try:
            await self.scanner.stop()
        except Exception:
            pass
        self.is_scanning = False


class SimulatorScanner:
    """Mock BLE scanner generating simulated Treel TPMS packets for testing."""

    def __init__(self, callback: Optional[Callable[[TireReading], None]] = None):
        self.decoder = TreelDecoder()
        self.callback = callback
        self.is_running = False
        self._task: Optional[asyncio.Task] = None

        self.simulated_sensors = [
            {"mac": "D2:58:6D:8F:16:10", "sensor_id": "8F1610", "pos": "FL", "base_psi": 32.0, "base_temp": 30.0, "batt": 100},
            {"mac": "CA:E8:6C:2D:92:15", "sensor_id": "2D92-1521", "pos": "FR", "base_psi": 34.0, "base_temp": 32.0, "batt": 92},
            {"mac": "F7:FC:85:AD:35:E2", "sensor_id": "35E2", "pos": "RL", "base_psi": 36.0, "base_temp": 37.0, "batt": 90},
            {"mac": "CD:8D:E6:9E:FB:E6", "sensor_id": "FBE6", "pos": "RR", "base_psi": 34.0, "base_temp": 36.0, "batt": 88},
        ]

    async def start(self):
        if self.is_running:
            return
        self.is_running = True
        self._task = asyncio.create_task(self._run_simulation())
        logger.info("TPMS Simulator started.")

    async def stop(self):
        if not self.is_running:
            return
        self.is_running = False
        if self._task:
            self._task.cancel()
        logger.info("TPMS Simulator stopped.")

    async def _run_simulation(self):
        while self.is_running:
            try:
                sensor_info = random.choice(self.simulated_sensors)

                psi = max(15.0, round(sensor_info["base_psi"] + random.uniform(-0.5, 0.5), 1))
                temp = round(sensor_info["base_temp"] + random.uniform(-0.3, 0.4), 1)
                rssi = random.randint(-82, -55)

                reading = TireReading(
                    mac_address=sensor_info["mac"],
                    sensor_id=sensor_info["sensor_id"],
                    pressure_psi=psi,
                    temperature_celsius=temp,
                    battery_percent=sensor_info["batt"],
                    mode="GATT / Encrypted",
                    rssi=rssi,
                    raw_hex="1858D2006D8F161045A444BC94807935DE77B49CFEA565F1",
                    status_flags="00000000",
                    accel_x_g=round(random.uniform(0.01, 0.08), 3),
                    accel_y_g=round(random.uniform(0.01, 0.08), 3),
                    is_valid=True
                )

                if reading:
                    reading.tire_position = sensor_info["pos"]
                    reading.alert_level = reading.evaluate_alert()
                    if self.callback:
                        self.callback(reading)

                await asyncio.sleep(random.uniform(1.5, 3.0))
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.error(f"Simulator error: {e}")
                await asyncio.sleep(2)
