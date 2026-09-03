"""
Diagnostic script to discover ALL raw BLE advertisement data on Windows using Bleak.
Prints raw company IDs, manufacturer bytes, service UUIDs, RSSI, and device names.
"""

import asyncio
from bleak import BleakScanner
from core.decoder import TreelDecoder

decoder = TreelDecoder()


def callback(device, adv):
    mac = device.address
    name = device.name or adv.local_name or "Unknown"
    rssi = adv.rssi

    print(f"\n[BLE DEVICE DETECTED] MAC: {mac} | Name: {name} | RSSI: {rssi} dBm")

    if adv.service_uuids:
        print(f"  Service UUIDs: {adv.service_uuids}")

    if adv.manufacturer_data:
        print(f"  Manufacturer Data:")
        for comp_id, data in adv.manufacturer_data.items():
            comp_hex = f"0x{comp_id:04X}"
            data_hex = data.hex().upper()
            print(f"    - Company {comp_hex}: {data_hex}")

            # Reconstruct iBeacon or raw payload and attempt decode
            if comp_id == 0x004C:
                full_payload = bytes([len(data) + 1, 0xFF, 0x4C, 0x00]) + data
            else:
                comp_bytes = comp_id.to_bytes(2, byteorder='little')
                full_payload = bytes([len(data) + 3, 0xFF]) + comp_bytes + data

            reading = decoder.decode_raw_payload(full_payload, mac_address=mac, rssi=rssi)
            if reading:
                print(f"    \033[92m*** MATCHED TREEL TPMS SENSOR ***\033[0m")
                print(f"    Sensor ID: {reading.sensor_id} | Pos: {reading.tire_position}")
                print(f"    Pressure: {reading.pressure_psi} PSI | Temp: {reading.temperature_celsius} °C | Mode: {reading.mode}")

    if adv.service_data:
        print(f"  Service Data:")
        for s_uuid, s_data in adv.service_data.items():
            print(f"    - UUID {s_uuid}: {s_data.hex().upper()}")


async def main():
    print("==================================================================")
    print("        RAW BLE ADVERTISEMENT DIAGNOSTIC SCANNER (WINDOWS)       ")
    print("==================================================================")
    print("Scanning for 15 seconds... Turn on your TPMS sensors or move nearby.\n")

    scanner = BleakScanner(detection_callback=callback)
    await scanner.start()
    await asyncio.sleep(15)
    await scanner.stop()
    print("\nScan completed.")


if __name__ == "__main__":
    asyncio.run(main())
