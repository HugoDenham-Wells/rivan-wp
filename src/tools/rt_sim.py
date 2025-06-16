import socket
import select
import sys

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
                    # Wait for input from either stdin or the socket
                    rlist, _, _ = select.select([sys.stdin, conn], [], [])
                    for ready in rlist:
                        if ready == sys.stdin:
                            msg = sys.stdin.readline()
                            if not msg:
                                print("EOF on stdin. Exiting.")
                                return
                            msg = msg.rstrip('\n')
                            if msg.lower() in ("exit", "quit"):
                                print("Closing connection.")
                                return
                            conn.sendall(msg.encode())
                        elif ready == conn:
                            data = conn.recv(4096)
                            if not data:
                                print("Client disconnected.")
                                return
                            hexstr = ' '.join(f'{b:02X}' for b in data)
                            print(f"[client] {data.decode(errors='replace')}")
                            print(f"[client][hex] {hexstr}")
                except (ConnectionResetError, BrokenPipeError):
                    print("Client disconnected.")
                    break
                except KeyboardInterrupt:
                    print("\nInterrupted. Exiting.")
                    break

if __name__ == "__main__":
    main()