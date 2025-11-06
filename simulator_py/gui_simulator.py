import socket
import json
import threading
import time
import tkinter as tk

HOST = '127.0.0.1'
PORT = 5555

class SimulatorServer:
    def __init__(self):
        self.flow = 2.0
        self.humidity = 40.0
        self.flowing = True
        self.leak = False
        self.high_flow = False
        self.running = False
        self._server_thread = None
        self._client = None
        self._lock = threading.Lock()

    def start(self):
        if self._server_thread and self._server_thread.is_alive():
            return
        self.running = True
        self._server_thread = threading.Thread(target=self._serve, daemon=True)
        self._server_thread.start()

    def stop(self):
        self.running = False
        with self._lock:
            if self._client:
                try: self._client.shutdown(socket.SHUT_RDWR)
                except: pass
                try: self._client.close()
                except: pass
                self._client = None

    def _serve(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((HOST, PORT))
            s.listen(1)
            print(f"[SIM] Listening on {HOST}:{PORT}")
            while self.running:
                try:
                    s.settimeout(1.0)
                    c, addr = s.accept()
                except TimeoutError:
                    continue
                except Exception:
                    continue
                with c:
                    with self._lock:
                        self._client = c
                    print(f"[SIM] Client connected: {addr}")
                    c.settimeout(1.0)
                    try:
                        while self.running:
                            with self._lock:
                                msg = {
                                    "flow_lpm": float(self.flow),
                                    "humidity_pct": float(self.humidity),
                                    "flowing": bool(self.flowing),
                                    "leak": bool(self.leak),
                                    "high_flow": bool(self.high_flow),
                                }
                            line = (str(msg).replace("'", '"') + "\n").encode('utf-8')
                            c.sendall(line)
                            time.sleep(0.5)
                    except Exception:
                        pass
                    finally:
                        print("[SIM] Client disconnected")
                        with self._lock:
                            self._client = None

server = SimulatorServer()

def run_gui():
    root = tk.Tk()
    root.title("AquaGuard Simulator")
    root.geometry("360x360")

    def start():
        server.start()
        status.configure(text="Server: RUNNING", fg="#58d68d")
    def stop():
        server.stop()
        status.configure(text="Server: STOPPED", fg="#ec7063")
    def set_flow(v):
        with server._lock: server.flow = float(v)
    def set_hum(v):
        with server._lock: server.humidity = float(v)
    def set_flowing():
        with server._lock: server.flowing = bool(flowing_var.get())
    def set_leak():
        with server._lock: server.leak = bool(leak_var.get())
    def set_high():
        with server._lock: server.high_flow = bool(high_var.get())

    tk.Label(root, text="AquaGuard TCP Simulator", font=("Helvetica", 14, "bold")).pack(pady=6)
    status = tk.Label(root, text="Server: STOPPED", fg="#ec7063")
    status.pack()

    frm = tk.Frame(root); frm.pack(pady=8, fill=tk.X, padx=10)

    tk.Label(frm, text="Flow (L/min)").pack(anchor='w')
    flow = tk.Scale(frm, from_=0, to=50, orient='horizontal', resolution=0.1, command=set_flow)
    flow.set(server.flow); flow.pack(fill=tk.X)

    tk.Label(frm, text="Humidity (%)").pack(anchor='w', pady=(8,0))
    hum = tk.Scale(frm, from_=10, to=90, orient='horizontal', resolution=0.1, command=set_hum)
    hum.set(server.humidity); hum.pack(fill=tk.X)

    flowing_var = tk.IntVar(value=1 if server.flowing else 0)
    leak_var = tk.IntVar(value=1 if server.leak else 0)
    high_var = tk.IntVar(value=1 if server.high_flow else 0)

    tk.Checkbutton(root, text="Flowing", variable=flowing_var, command=set_flowing).pack(anchor='w', padx=10)
    tk.Checkbutton(root, text="Leak", variable=leak_var, command=set_leak).pack(anchor='w', padx=10)
    tk.Checkbutton(root, text="High flow", variable=high_var, command=set_high).pack(anchor='w', padx=10)

    btns = tk.Frame(root); btns.pack(pady=10)
    tk.Button(btns, text="Start Server", command=start, width=14).grid(row=0, column=0, padx=6)
    tk.Button(btns, text="Stop Server", command=stop, width=14).grid(row=0, column=1, padx=6)

    tk.Label(root, text=f"Host: {HOST}   Port: {PORT}", fg="#95a5a6").pack(pady=(4,0))

    root.mainloop()

if __name__ == '__main__':
    run_gui()
