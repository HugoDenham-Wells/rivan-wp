import socket

HOST = '127.0.0.1'  # localhost
PORT = 17726        # port CMD server is listening on

def main():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((HOST, PORT))
            print(f"Connected to {HOST}:{PORT}. Type messages and press Enter to send.")
            
            while True:
                msg = input("> ")  # Read from terminal
                if msg.lower() in ("exit", "quit"):
                    print("Exiting.")
                    break
                sock.sendall(msg.encode())  # Send as bytes
    except ConnectionRefusedError:
        print(f"Could not connect to {HOST}:{PORT}. Is the server running?")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    main()