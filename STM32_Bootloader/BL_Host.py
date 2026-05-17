#!/usr/bin/env python3
import glob
import os
import sys
import time

import serial

# Bootloader command codes
CBL_GET_VER_CMD = 0x10
CBL_GET_CID_CMD = 0x12
CBL_GET_RDP_STATUS_CMD = 0x13
CBL_FLASH_ERASE_CMD = 0x15
CBL_MEM_WRITE_CMD = 0x16
CBL_GO_TO_MAIN_APP_CMD = 0x18

CBL_SEND_ACK = 0xCD
CBL_SEND_NACK = 0xAB

SUCCESSFUL_ERASE = 0x03
FLASH_PAYLOAD_WRITE_PASSED = 0x01
FLASH_PAYLOAD_WRITE_FAILED = 0x00

PAYLOAD_CHUNK_SIZE = 32
APPLICATION_START_ADDRESS = 0x08004000


def calculate_crc32(data: bytes) -> int:
    crc = 0xFFFFFFFF
    poly = 0x04C11DB7
    for byte in data:
        crc ^= byte & 0xFF
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    return crc & 0xFFFFFFFF


def open_serial_port(port_name: str) -> serial.Serial:
    port = serial.Serial(
        port=port_name,
        baudrate=230400,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=1,
        write_timeout=1,
    )
    time.sleep(2)
    port.reset_input_buffer()
    return port


def read_exact(port: serial.Serial, length: int, timeout_s: float) -> bytes | None:
    deadline = time.monotonic() + timeout_s
    data = b""
    while len(data) < length and time.monotonic() < deadline:
        chunk = port.read(length - len(data))
        if chunk:
            data += chunk
    return data if len(data) == length else None


def send_packet(port: serial.Serial, packet: bytes) -> None:
    port.reset_input_buffer()
    print("   " + "  ".join(f"0x{b:02x}" for b in packet))
    total = len(packet)
    sent = 0
    while sent < total:
        written = port.write(packet[sent:])
        if written is None or written == 0:
            raise IOError("Serial write failed or timed out")
        sent += written
    port.flush()
    time.sleep(0.005)


def read_ack(port: serial.Serial, command_code: int, timeout_s: float = 10.0) -> tuple[bool, bytes | None]:
    first = read_exact(port, 1, timeout_s)
    if first is None:
        print("\n   Timeout — no response from Bootloader")
        return False, None

    if first[0] == CBL_SEND_ACK:
        second = read_exact(port, 1, timeout_s)
        if second is None:
            print("\n   Timeout — missing ACK length byte")
            return False, None

        length_to_follow = second[0]
        print("\n   Received Acknowledgement from Bootloader")
        print(f"   Preparing to receive ({length_to_follow}) bytes from the bootloader")

        payload = read_exact(port, length_to_follow, timeout_s)
        if payload is None:
            print("\n   Timeout — incomplete response payload")
            return False, None

        return True, payload

    if first[0] == CBL_SEND_NACK:
        print("\n   Received NACK from Bootloader — command rejected")
        return False, None

    print(f"\n   Unknown response: {hex(first[0])}")
    return False, None


def build_simple_packet(command: int) -> bytes:
    packet = bytearray(6)
    packet[0] = 5
    packet[1] = command
    crc = calculate_crc32(packet[:2])
    packet[2:6] = crc.to_bytes(4, "little")
    return bytes(packet)


def build_write_packet(address: int, payload: bytes) -> bytes:
    packet_length = 11 + len(payload)
    packet = bytearray(packet_length)
    packet[0] = packet_length - 1
    packet[1] = CBL_MEM_WRITE_CMD
    packet[2:6] = address.to_bytes(4, "little")
    packet[6] = len(payload)
    packet[7 : 7 + len(payload)] = payload
    crc = calculate_crc32(packet[: packet_length - 4])
    packet[7 + len(payload) : 11 + len(payload)] = crc.to_bytes(4, "little")
    return bytes(packet)


def process_response(command: int, data: bytes | None) -> bool:
    if data is None:
        return False

    if command == CBL_GET_VER_CMD:
        if len(data) != 4:
            print("   Unexpected version response length")
            return False
        print(f"\n   Bootloader Vendor ID : {data[0]}")
        print(f"   Bootloader Version   : {data[1]}.{data[2]}.{data[3]}")
        return True

    if command == CBL_GET_CID_CMD:
        if len(data) != 2:
            print("   Unexpected chip ID response length")
            return False
        cid = int.from_bytes(data, "little")
        print(f"\n   Chip Identification Number : {hex(cid)}")
        return True

    if command == CBL_GET_RDP_STATUS_CMD:
        if len(data) != 1:
            print("   Unexpected RDP response length")
            return False
        value = data[0]
        if value == 0xAA:
            print("\n   FLASH Protection : LEVEL 0")
        elif value == 0x55:
            print("\n   FLASH Protection : LEVEL 1")
        elif value == 0xCC:
            print("\n   FLASH Protection : LEVEL 2")
        else:
            print(f"\n   Unknown RDP value: {hex(value)}")
        return True

    if command == CBL_FLASH_ERASE_CMD:
        if len(data) != 1:
            print("   Unexpected erase response length")
            return False
        if data[0] == SUCCESSFUL_ERASE:
            print("\n   Erase Status -> Successful Erase")
            return True
        print(f"\n   Erase Status -> Failed (code {hex(data[0])})")
        return False

    if command == CBL_MEM_WRITE_CMD:
        if len(data) != 1:
            print("   Unexpected write response length")
            return False
        if data[0] == FLASH_PAYLOAD_WRITE_PASSED:
            print("\n   Write Status -> Write Successful")
            return True
        print("\n   Write Status -> Write Failed or Invalid Address")
        return False

    if command == CBL_GO_TO_MAIN_APP_CMD:
        if len(data) != 1:
            print("   Unexpected jump response length")
            return False
        if data[0] == 1:
            print("\n   Jump Successful")
            return True
        print("\n   Jump Failed — no valid application found")
        return False

    print("   Unknown command response")
    return False


def prompt_command() -> int:
    print("\n>>>>>> STM32F401CCU6 Custom BootLoader <<<<<")
    print("    ===== Made by Eng. Abdelrahman Mohamed ======")
    print("=================================================")
    print("Which command you need to send to the bootLoader :")
    print("   CBL_GET_VER_CMD              --> 1")
    print("   CBL_GET_CID_CMD              --> 2")
    print("   CBL_GET_RDP_STATUS_CMD       --> 3")
    print("   CBL_APP_ERASE_CMD            --> 4   <- run before upload")
    print("   CBL_UPLOAD_APP_CMD           --> 5")
    print("   CBL_JUMP_TO_APP_CMD          --> 6")
    choice = input("\nEnter the command code : ").strip()
    return int(choice) if choice.isdigit() else -1


def execute_command(port: serial.Serial, command: int, bin_file: str | None) -> bool:
    if command == 1:
        print("Request the bootloader version")
        packet = build_simple_packet(CBL_GET_VER_CMD)
        send_packet(port, packet)
        ack, data = read_ack(port, CBL_GET_VER_CMD)
        return process_response(CBL_GET_VER_CMD, data) if ack else False

    if command == 2:
        print("Read the MCU chip identification number")
        packet = build_simple_packet(CBL_GET_CID_CMD)
        send_packet(port, packet)
        ack, data = read_ack(port, CBL_GET_CID_CMD)
        return process_response(CBL_GET_CID_CMD, data) if ack else False

    if command == 3:
        print("Read the FLASH Read Protection level")
        packet = build_simple_packet(CBL_GET_RDP_STATUS_CMD)
        send_packet(port, packet)
        ack, data = read_ack(port, CBL_GET_RDP_STATUS_CMD)
        return process_response(CBL_GET_RDP_STATUS_CMD, data) if ack else False

    if command == 4:
        print("Erasing Application Flash sectors")
        packet = build_simple_packet(CBL_FLASH_ERASE_CMD)
        send_packet(port, packet)
        ack, data = read_ack(port, CBL_FLASH_ERASE_CMD, timeout_s=15.0)
        return process_response(CBL_FLASH_ERASE_CMD, data) if ack else False

    if command == 5:
        if bin_file is None:
            candidates = glob.glob("*.bin")
            if not candidates:
                print("   No .bin file found in current directory.")
                return False
            if len(candidates) > 1:
                print("   Multiple .bin files found. Use a single .bin or pass the path explicitly.")
                return False
            bin_file = candidates[0]

        if not os.path.isfile(bin_file):
            print(f"   Binary file not found: {bin_file}")
            return False

        print("Write data into Flash memory")
        print(f"   Binary file : {bin_file}")
        total_size = os.path.getsize(bin_file)
        total_packets = (total_size + PAYLOAD_CHUNK_SIZE - 1) // PAYLOAD_CHUNK_SIZE
        print(f"   File size   : {total_size} bytes  ({total_packets} packets)")

        with open(bin_file, "rb") as f:
            address = APPLICATION_START_ADDRESS
            sent = 0
            packet_index = 0

            while sent < total_size:
                chunk = f.read(PAYLOAD_CHUNK_SIZE)
                if not chunk:
                    break

                packet_index += 1
                packet = build_write_packet(address, chunk)
                send_packet(port, packet)
                sent += len(chunk)
                print(f"\n   Progress: {sent} / {total_size} bytes")
                ack, data = read_ack(port, CBL_MEM_WRITE_CMD, timeout_s=15.0)
                if not ack or not process_response(CBL_MEM_WRITE_CMD, data):
                    print("\n   Upload aborted.")
                    return False

                address += len(chunk)
                time.sleep(0.05)

        print("\n   Payload Written Successfully")
        return True

    if command == 6:
        print("Jump to Main Application")
        packet = build_simple_packet(CBL_GO_TO_MAIN_APP_CMD)
        send_packet(port, packet)
        ack, data = read_ack(port, CBL_GO_TO_MAIN_APP_CMD)
        return process_response(CBL_GO_TO_MAIN_APP_CMD, data) if ack else False

    print("   Unknown command")
    return False


def main() -> None:
    if len(sys.argv) > 1:
        port_name = sys.argv[1]
    else:
        port_name = input("Enter Port Name (e.g. /dev/ttyUSB0 or COM3): ").strip()

    try:
        port = open_serial_port(port_name)
    except Exception as exc:
        print(f"Error opening serial port: {exc}")
        return

    bin_file = sys.argv[2] if len(sys.argv) > 2 else None

    while True:
        command = prompt_command()
        if command < 1 or command > 6:
            print("   Error: please enter a number from 1 to 6")
        else:
            execute_command(port, command, bin_file)

        input("\nPlease press any key to continue ...")
        port.reset_input_buffer()


if __name__ == "__main__":
    main()
