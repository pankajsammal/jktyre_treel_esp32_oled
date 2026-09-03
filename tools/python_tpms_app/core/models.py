"""
Data Models for Treel TPMS BLE Packet Capture Application.
"""

from dataclasses import dataclass, field, asdict
from datetime import datetime
from typing import Optional, Dict, Any


@dataclass
class TireReading:
    mac_address: str
    sensor_id: str
    pressure_psi: float
    temperature_celsius: float
    battery_percent: int = -1  # -1 indicates unavailable in Beacon mode
    mode: str = "Beacon"       # "Beacon" or "GATT / Encrypted"
    rssi: int = -100
    raw_hex: str = ""
    status_flags: str = "00000000"
    accel_x_g: float = 0.0
    accel_y_g: float = 0.0
    timestamp: str = field(default_factory=lambda: datetime.now().isoformat())
    is_valid: bool = True
    tire_position: str = "UNKNOWN"  # FL, FR, RL, RR, UNKNOWN
    alert_level: str = "NORMAL"     # NORMAL, LOW_PRESSURE, HIGH_PRESSURE, HIGH_TEMP, LOW_BATTERY

    @property
    def pressure_bar(self) -> float:
        return round(self.pressure_psi * 0.0689476, 2)

    @property
    def pressure_kpa(self) -> float:
        return round(self.pressure_psi * 6.89476, 1)

    @property
    def temperature_fahrenheit(self) -> float:
        return round((self.temperature_celsius * 9 / 5) + 32, 1)

    def evaluate_alert(self, min_psi: float = 26.0, max_psi: float = 45.0, max_temp_c: float = 70.0) -> str:
        if not self.is_valid:
            return "INVALID"
        if self.pressure_psi < min_psi:
            return "LOW_PRESSURE"
        if self.pressure_psi > max_psi:
            return "HIGH_PRESSURE"
        if self.temperature_celsius > max_temp_c:
            return "HIGH_TEMP"
        if 0 <= self.battery_percent < 15:
            return "LOW_BATTERY"
        return "NORMAL"

    def to_dict(self) -> Dict[str, Any]:
        data = asdict(self)
        data["pressure_bar"] = self.pressure_bar
        data["pressure_kpa"] = self.pressure_kpa
        data["temperature_fahrenheit"] = self.temperature_fahrenheit
        return data
