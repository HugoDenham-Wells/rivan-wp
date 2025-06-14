import socket

HOST = '127.0.0.1'  # Localhost
PORT = 17725        # Listening port

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.bind((HOST, PORT))
        server.listen(1)
        print(f"Waiting for connection on {HOST}:{PORT}...")
        
        conn, addr = server.accept()
        print(f"Connected by {addr}")

        with conn:
            while True:
                try:
                    msg = input("> ")  # Wait for user to type a message
                    if msg.lower() in ("exit", "quit"):
                        print("Closing connection.")
                        break
                    conn.sendall(msg.encode())  # Send as bytes
                except (ConnectionResetError, BrokenPipeError):
                    print("Client disconnected.")
                    break
                except KeyboardInterrupt:
                    print("\nInterrupted. Exiting.")
                    break

if __name__ == "__main__":
    main()