import bluetooth
import subprocess
import wave
import os
import time
import threading

CHUNK_SIZE = 512
INCOMING_WAV_FILE = "received.wav"

def convert_aac_to_wav(aac_path):
    """Convert AAC file to WAV using ffmpeg and return raw PCM bytes."""
    wav_path = "temp.wav"
    command = ["ffmpeg", "-y", "-i", aac_path, "-ar", "44100", "-ac", "1", wav_path]
    subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    with open(wav_path, "rb") as f:
        wav_data = f.read()

    os.remove(wav_path)
    return wav_data

def send_wav_bt(bt_socket, wav_data):
    """Send WAV data in 512-byte chunks over Bluetooth socket."""
    total = len(wav_data)
    sent = 0
    for i in range(0, total, CHUNK_SIZE):
        if i + CHUNK_SIZE > total:
            break  # ignore last chunk if smaller than 512 bytes
        chunk = wav_data[i:i + CHUNK_SIZE]
        bt_socket.send(chunk)
        sent += len(chunk)
        time.sleep(0.01)
        print(f"Sent {sent}/{total} bytes", end='\r')
    print("\n[INFO] Bluetooth transmission complete.")

def receive_wav_bt(bt_socket):
    """Receive raw bytes over the same Bluetooth socket and save as WAV."""
    frames = []

    print("[INFO] Listening on Bluetooth for incoming raw bytes...")
    try:
        while True:
            bt_socket.settimeout(0.1)
            try:
                data = bt_socket.recv(1024)
                if data:
                    frames.append(data)
            except bluetooth.BluetoothError:
                continue

    except KeyboardInterrupt:
        print("\n[INFO] Stopped listening, saving WAV...")
        if frames:
            with wave.open(INCOMING_WAV_FILE, 'wb') as wf:
                wf.setnchannels(1)
                wf.setsampwidth(2)  # assuming 16-bit PCM
                wf.setframerate(44100)
                wf.writeframes(b''.join(frames))
            print(f"[INFO] Saved received data to {INCOMING_WAV_FILE}")
        else:
            print("[INFO] No data received.")

def main():
    # Discover Bluetooth devices
    nearby_devices = bluetooth.discover_devices(duration=8, lookup_names=True)
    if not nearby_devices:
        print("[ERROR] No Bluetooth devices found.")
        return

    print("Available devices:")
    for idx, (addr, name) in enumerate(nearby_devices):
        print(f"{idx+1}: {name} ({addr})")

    choice = int(input(f"Choose a device [1-{len(nearby_devices)}]: ").strip()) - 1
    target_addr = nearby_devices[choice][0]

    bt_socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
    try:
        bt_socket.connect((target_addr, 1))
        print(f"[INFO] Connected to {nearby_devices[choice][1]} ({target_addr})")
    except bluetooth.BluetoothError as e:
        print(f"[ERROR] Bluetooth connection failed: {e}")
        return

    # Ask for AAC file path and convert to WAV
    aac_path = input("Enter AAC file path to send: ").strip()
    if not os.path.exists(aac_path):
        print("[ERROR] File not found!")
        bt_socket.close()
        return

    wav_data = convert_aac_to_wav(aac_path)

    # Start sending and receiving threads on the same Bluetooth socket
    bt_send_thread = threading.Thread(target=send_wav_bt, args=(bt_socket, wav_data))
    bt_recv_thread = threading.Thread(target=receive_wav_bt, args=(bt_socket,))
    bt_recv_thread.daemon = True

    bt_send_thread.start()
    bt_recv_thread.start()

    bt_send_thread.join()
    print("[INFO] Bluetooth send complete. Receiving will continue until Ctrl+C.")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("[INFO] Exiting...")
    bt_socket.close()

if __name__ == "__main__":
    main()