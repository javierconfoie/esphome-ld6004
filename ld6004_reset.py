"""
HLK-LD6004 — Reset de configuración por serial (TinyFrame)
Envía: resetDetectionArea + clearInterferenceZones + clearDwellZones
       + sensibilidad HIGH + modo NORMAL + autoGenerateInterference

Uso:
    python ld6004_reset.py COM6
"""

import sys
import time
import serial

TF_SOF              = 0x01
TF_TYPE_CONTROL     = 0x0201
TF_TYPE_SET_AREA    = 0x0202

# Control subcmds
CMD_AUTO_GEN_INTERFERENCE = 0x01
CMD_REQUEST_ZONES         = 0x02
CMD_CLEAR_INTERFERENCE    = 0x03
CMD_RESET_DETECTION_AREA  = 0x04
CMD_SENSITIVITY_LOW       = 0x0A
CMD_SENSITIVITY_MED       = 0x0B
CMD_SENSITIVITY_HIGH      = 0x0C
CMD_MODE_NORMAL           = 0x17
CMD_CLEAR_DWELL           = 0x25

_send_id = 0

def checksum(data: bytes) -> int:
    result = 0
    for b in data:
        result ^= b
    return (~result) & 0xFF

def build_frame(type_: int, data: bytes) -> bytes:
    global _send_id
    _send_id = (_send_id + 1) & 0xFFFF
    length = len(data)

    hdr = bytes([
        TF_SOF,
        (_send_id >> 8) & 0xFF,
        _send_id & 0xFF,
        (length >> 8) & 0xFF,
        length & 0xFF,
        (type_ >> 8) & 0xFF,
        type_ & 0xFF,
    ])
    hdr_cksum = checksum(hdr)

    frame = hdr + bytes([hdr_cksum])
    if length > 0:
        frame += data + bytes([checksum(data)])
    else:
        frame += bytes([0xFF])  # empty data checksum

    return frame

def send_control(ser: serial.Serial, subcmd: int, label: str):
    data = subcmd.to_bytes(4, 'little')
    frame = build_frame(TF_TYPE_CONTROL, data)
    ser.write(frame)
    print(f"  TX {label} (0x{subcmd:02X}): {frame.hex(' ').upper()}")
    time.sleep(0.15)

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM6"
    print(f"Conectando a {port} @ 115200...")

    with serial.Serial(port, 115200, timeout=1) as ser:
        time.sleep(0.5)
        ser.reset_input_buffer()

        print("\n--- Reset de zonas ---")
        send_control(ser, CMD_RESET_DETECTION_AREA,  "resetDetectionArea")
        send_control(ser, CMD_CLEAR_INTERFERENCE,     "clearInterferenceZones")
        send_control(ser, CMD_CLEAR_DWELL,            "clearDwellZones")

        print("\n--- Restore sensibilidad y modo ---")
        send_control(ser, CMD_SENSITIVITY_HIGH, "sensitivity HIGH")
        send_control(ser, CMD_MODE_NORMAL,       "mode NORMAL")

        print("\n--- Regenerar zonas de interferencia ---")
        send_control(ser, CMD_AUTO_GEN_INTERFERENCE, "autoGenerateInterference")

        print("\n--- Consultar zonas para verificar ---")
        send_control(ser, CMD_REQUEST_ZONES, "requestZones")

        # Leer respuesta durante 3 segundos
        print("\n--- Respuesta del radar (3s) ---")
        deadline = time.time() + 3.0
        while time.time() < deadline:
            chunk = ser.read(ser.in_waiting or 1)
            if chunk:
                print(f"  RX: {chunk.hex(' ').upper()}")

    print("\nListo.")

if __name__ == "__main__":
    main()
