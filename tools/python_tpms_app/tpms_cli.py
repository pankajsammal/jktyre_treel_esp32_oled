import os
import csv
import sys
import argparse
import asyncio
from datetime import datetime

from core.decoder import TreelDecoder
from core.scanner import TreelBLEScanner, SimulatorScanner
from core.models import TireReading

CSV_LOG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "tpms_packets.csv")
RAW_CSV_LOG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "tpms_raw_capture.csv")


def log_reading_to_csv(reading: TireReading):
    """Write decoded reading to CSV file."""
    file_exists = os.path.exists(CSV_LOG_PATH)
    try:
        with open(CSV_LOG_PATH, "a", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            if not file_exists:
                writer.writerow([
                    "timestamp", "sensor_id", "mac_address", "tire_position",
                    "mode", "pressure_psi", "pressure_bar", "temperature_celsius",
                    "temperature_fahrenheit", "battery_percent", "rssi", "alert_level", "raw_hex"
                ])
            writer.writerow([
                reading.timestamp, reading.sensor_id, reading.mac_address, reading.tire_position,
                reading.mode, reading.pressure_psi, reading.pressure_bar, reading.temperature_celsius,
                reading.temperature_fahrenheit, reading.battery_percent, reading.rssi, reading.alert_level, reading.raw_hex
            ])
            f.flush()
    except Exception as e:
        print(f"[ERROR] Failed to write to {CSV_LOG_PATH}: {e}")

    # Also log raw hex capture
    if reading.raw_hex:
        raw_exists = os.path.exists(RAW_CSV_LOG_PATH)
        try:
            with open(RAW_CSV_LOG_PATH, "a", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)
                if not raw_exists:
                    writer.writerow(["timestamp", "mac_address", "tire_position", "rssi", "raw_hex"])
                writer.writerow([reading.timestamp, reading.mac_address, reading.tire_position, reading.rssi, reading.raw_hex])
                f.flush()
        except Exception:
            pass


def print_reading(reading: TireReading):
    """Print colorized TPMS packet reading to CLI and log to CSV."""
    log_reading_to_csv(reading)

    timestamp = datetime.now().strftime("%H:%M:%S")
    mode_color = "\033[94m" if "Beacon" in reading.mode else "\033[95m"
    reset_color = "\033[0m"

    alert_color = "\033[92m"  # Green
    if reading.alert_level != "NORMAL":
        alert_color = "\033[91m"  # Red

    print(
        f"[{timestamp}] Sensor: \033[96m{reading.sensor_id:<9}\033[0m | "
        f"Pos: \033[93m{reading.tire_position:<5}\033[0m | "
        f"Mode: {mode_color}{reading.mode:<14}{reset_color} | "
        f"Press: \033[97m{reading.pressure_psi:>5.1f} PSI\033[0m ({reading.pressure_bar:>4.2f} bar) | "
        f"Temp: \033[97m{reading.temperature_celsius:>4.1f} °C\033[0m | "
        f"Batt: {reading.battery_percent if reading.battery_percent >= 0 else '--':>3}% | "
        f"RSSI: {reading.rssi:>4} dBm | "
        f"Status: {alert_color}{reading.alert_level}{reset_color}"
    )


async def main():
    parser = argparse.ArgumentParser(description="JK Tyre Treel TPMS BLE Packet Sniffer CLI for Windows")
    parser.add_argument("--hardware", action="store_true", default=True, help="Run active BLE hardware scan using Bleak (default)")
    parser.add_argument("--simulate", action="store_true", help="Run in simulator mode")
    parser.add_argument("--decode", type=str, help="Decode a single raw hex string payload")

    args = parser.parse_args()

    if args.decode:
        decoder = TreelDecoder()
        reading = decoder.decode_hex(args.decode)
        if reading:
            print("\n=== Decoded Treel TPMS Reading ===")
            for k, v in reading.to_dict().items():
                print(f"  {k:<22}: {v}")
        else:
            print("\033[91mFailed to decode hex payload!\033[0m")
        return

    print("==================================================================")
    print("        JK TYRE TREEL TPMS — BLE PACKET SNIFFER (WINDOWS)        ")
    print("==================================================================")
    print("Press Ctrl+C to stop scanning.\n")

    if args.simulate:
        print("[INFO] Starting Simulator Scanner Mode...")
        scanner = SimulatorScanner(callback=print_reading)
        try:
            await scanner.start()
            while True:
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            await scanner.stop()
            print("\nSimulator stopped.")
    else:
        print("[INFO] Starting Hardware BLE Scanner (WinRT)...")
        scanner = TreelBLEScanner(callback=print_reading)
        try:
            await scanner.start()
            while True:
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            await scanner.stop()
            print("\nScanner stopped.")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        sys.exit(0)
