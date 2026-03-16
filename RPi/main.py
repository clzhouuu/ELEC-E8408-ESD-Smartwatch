import threading
import wserver
import receiver

def main():
    print("Starting Bluetooth receiver.")
    try:
        while True:
            hubbt.wait_for_connection()
            hubbt.synchronize(callback=process_sessions)
            
    except KeyboardInterrupt:
        print("CTRL+C Pressed. Shutting down the server...")

    except Exception as e:
        print(f"Unexpected shutdown...")
        print(f"ERROR: {e}")
        hubbt.sock.close()
        raise e

thread = threading.Thread(target=main, daemon=True)
thread.start()

wserver.app.run('0.0.0.0', debug=True)
