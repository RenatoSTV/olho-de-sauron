#!/usr/bin/env python3
"""Le a serial do ESP32-CAM (programador ESP32-CAM-MB) sem deixar a placa presa em reset.

Uso: python3 ferramentas/ler_camera.py [segundos] [--sem-reset] [porta]
Por padrao da um pulso de reset (RTS -> EN) e depois solta DTR e RTS.
"""
import os, sys, termios, fcntl, struct, time, select

args = [a for a in sys.argv[1:] if not a.startswith("--")]
segundos = float(args[0]) if args else 10
porta = args[1] if len(args) > 1 else "/dev/ttyUSB0"
reset = "--sem-reset" not in sys.argv

fd = os.open(porta, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
a = termios.tcgetattr(fd)
a[0] = 0; a[1] = 0; a[3] = 0
a[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
a[4] = a[5] = termios.B115200
termios.tcsetattr(fd, termios.TCSANOW, a)

def linhas(dtr, rts):
    for bit, on in ((termios.TIOCM_DTR, dtr), (termios.TIOCM_RTS, rts)):
        fcntl.ioctl(fd, termios.TIOCMBIS if on else termios.TIOCMBIC, struct.pack("I", bit))

if reset:
    linhas(False, True); time.sleep(0.2)
linhas(False, False)

t0 = time.time(); resto = b""
while time.time() - t0 < segundos:
    r, _, _ = select.select([fd], [], [], 0.2)
    if not r:
        continue
    try:
        resto += os.read(fd, 4096)
    except BlockingIOError:
        continue
    *completas, resto = resto.split(b"\n")
    for l in completas:
        txt = l.decode("utf-8", "replace").strip().replace("\x00", "")
        if txt:
            print(f"{time.time() - t0:6.2f}s  {txt}", flush=True)
os.close(fd)
