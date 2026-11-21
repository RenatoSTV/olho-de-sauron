#!/usr/bin/env python3
"""Janela ao vivo do que o ESP32-CAM (sketch etapa4_visual) esta vendo e detectando.

Mostra a imagem 160x120 da camera (16 tons de cinza) ampliada com suavizacao, os pontos que contaram
como "mudou" em vermelho e uma linha laranja no X calculado. Grava cada quadro em
<pasta>/quadros.bin e, ao fechar a janela, gera <pasta>/camera.mp4.
Se o cabo for desconectado, espera a camera voltar.

Uso: python3 ferramentas/ver_camera.py [pasta_saida] [--porta /dev/ttyUSB0] [--segundos N] [--sem-video]
"""
import os, sys, time, struct, termios, fcntl, select, subprocess, threading, queue
import tkinter as tk
import numpy as np
from PIL import Image, ImageTk

ESCALA = 5                      # 160x120 -> 800x600
MAGIA = b"OLH4"
CAB = 4 + 2 + 2 + 2 + 2 + 1     # magia, largura, altura, x, qtd, luz
BAUD = termios.B921600          # o CH340 do programador nao aguentou 2 Mbaud

def arg(nome, padrao):
    if nome in sys.argv:
        return sys.argv[sys.argv.index(nome) + 1]
    return padrao

porta = arg("--porta", "/dev/ttyUSB0")
duracao = float(arg("--segundos", "0"))
posicionais = [a for i, a in enumerate(sys.argv[1:], 1)
               if not a.startswith("--") and sys.argv[i - 1] not in ("--porta", "--segundos")]
saida = posicionais[0] if posicionais else "captura_camera"
os.makedirs(saida, exist_ok=True)
arquivo = os.path.join(saida, "quadros.bin")

def tamanho_pacote(w, h):
    return CAB + w * h // 2 + (w // 2) * (h // 2) // 8

def abrir_porta():
    fd = os.open(porta, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    a = termios.tcgetattr(fd)
    a[0] = 0; a[1] = 0; a[3] = 0
    a[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    a[4] = a[5] = BAUD
    termios.tcsetattr(fd, termios.TCSANOW, a)
    for bit in (termios.TIOCM_DTR, termios.TIOCM_RTS):   # solta DTR e RTS: placa roda
        fcntl.ioctl(fd, termios.TIOCMBIC, struct.pack("I", bit))
    return fd

fila = queue.Queue(maxsize=4)
parar = threading.Event()
estado = {"conectada": False}

def leitor():
    fd = None
    buf = b""
    with open(arquivo, "ab") as arq:
        while not parar.is_set():
            if fd is None:                      # espera a camera (re)aparecer
                try:
                    fd = abrir_porta()
                    estado["conectada"] = True
                    buf = b""
                except OSError:
                    estado["conectada"] = False
                    time.sleep(0.5)
                    continue
            try:
                r, _, _ = select.select([fd], [], [], 0.2)
                if r:
                    dados = os.read(fd, 65536)
                    if not dados:
                        raise OSError("porta fechou")
                    buf += dados
            except BlockingIOError:
                pass
            except OSError:                     # cabo desconectado
                try:
                    os.close(fd)
                except OSError:
                    pass
                fd = None
                estado["conectada"] = False
                continue
            while True:
                i = buf.find(MAGIA)
                if i < 0:
                    buf = buf[-3:]
                    break
                if len(buf) - i < CAB:
                    buf = buf[i:]
                    break
                w, h = struct.unpack_from("<HH", buf, i + 4)
                if not (0 < w <= 640 and 0 < h <= 480 and w % 16 == 0 and h % 2 == 0):
                    buf = buf[i + 4:]           # cabecalho falso, procura o proximo
                    continue
                tam = tamanho_pacote(w, h)
                if len(buf) - i < tam:
                    buf = buf[i:]
                    break
                pacote = buf[i:i + tam]
                buf = buf[i + tam:]
                t = time.time()
                arq.write(struct.pack("<dI", t, tam) + pacote)
                try:
                    fila.put_nowait((t, pacote))
                except queue.Full:
                    pass
    if fd is not None:
        os.close(fd)

def decodificar(pacote):
    w, h, x, qtd, luz = struct.unpack_from("<HHhHB", pacote, 4)
    b = np.frombuffer(pacote, np.uint8, w * h // 2, CAB)
    img = np.empty(w * h, np.uint8)
    img[0::2] = (b & 0x0F) * 17                 # 4 bits -> 0..255
    img[1::2] = (b >> 4) * 17
    img = img.reshape(h, w)
    gw, gh = w // 2, h // 2
    bits = np.frombuffer(pacote, np.uint8, gw * gh // 8, CAB + w * h // 2)
    mask = np.unpackbits(bits, bitorder="little").reshape(gh, gw).astype(bool)
    return w, h, x, qtd, luz, img, mask

def compor(w, h, x, img, mask):
    W, H = w * ESCALA, h * ESCALA
    base = Image.fromarray(img, "L").resize((W, H), Image.BICUBIC).convert("RGB")
    rgb = np.asarray(base).copy()
    m = Image.fromarray(mask.astype(np.uint8) * 255, "L").resize((W, H), Image.NEAREST)
    m = np.asarray(m) > 0
    rgb[m] = (rgb[m] * 0.35 + np.array([255, 30, 30]) * 0.65).astype(np.uint8)
    if x >= 0:
        cx = int((x + 0.5) * ESCALA)
        rgb[:, max(cx - 2, 0):cx + 3] = (255, 150, 0)
    return rgb

threading.Thread(target=leitor, daemon=True).start()

raiz = tk.Tk()
raiz.title("Olho de Sauron - o que a camera ve")
tela = tk.Label(raiz, bg="black", width=160 * ESCALA, height=120 * ESCALA)
tela.pack()
info = tk.Label(raiz, font=("monospace", 16), anchor="w", justify="left", text="esperando a camera...")
info.pack(fill="x")
tk.Label(raiz, font=("sans", 11), fg="#555",
         text="vermelho = pontos que mudaram   laranja = X calculado (posicao enviada ao olho)").pack(fill="x")
foto = None
inicio = time.time()
contagem = []

def atualizar():
    global foto
    ultimo = None
    while not fila.empty():
        ultimo = fila.get_nowait()
    if ultimo:
        t, pacote = ultimo
        contagem.append(t)
        while contagem and t - contagem[0] > 2:
            contagem.pop(0)
        w, h, x, qtd, luz, img, mask = decodificar(pacote)
        foto = ImageTk.PhotoImage(Image.fromarray(compor(w, h, x, img, mask)))
        tela.configure(image=foto, width=w * ESCALA, height=h * ESCALA)
        txt_x = f"X:{x:3d}" if x >= 0 else "X: --"
        info.configure(text=f"{time.time() - inicio:6.1f}s   {txt_x}   mudou={qtd:4d} (minimo 41)   luz={luz:3d}   {len(contagem) / 2:4.1f} quadros/s")
    elif not estado["conectada"]:
        info.configure(text=f"{time.time() - inicio:6.1f}s   camera desconectada, esperando ela voltar...")
    if duracao and time.time() - inicio > duracao:
        fechar()
        return
    raiz.after(30, atualizar)

def fechar():
    parar.set()
    raiz.destroy()

raiz.protocol("WM_DELETE_WINDOW", fechar)
raiz.after(30, atualizar)
raiz.mainloop()
time.sleep(0.3)

# Gera o video com tudo o que foi gravado
registros = []
with open(arquivo, "rb") as f:
    while True:
        cab = f.read(12)
        if len(cab) < 12:
            break
        t, tam = struct.unpack("<dI", cab)
        pacote = f.read(tam)
        if len(pacote) < tam or not pacote.startswith(MAGIA):
            break
        registros.append((t, pacote))
print(f"{len(registros)} quadros gravados em {arquivo}")
if len(registros) > 1 and "--sem-video" not in sys.argv:
    fps = max(1, round((len(registros) - 1) / (registros[-1][0] - registros[0][0])))
    w, h = struct.unpack_from("<HH", registros[0][1], 4)
    video = os.path.join(saida, "camera.mp4")
    ff = subprocess.Popen(["ffmpeg", "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24",
                           "-s", f"{w * ESCALA}x{h * ESCALA}", "-r", str(fps), "-i", "-",
                           "-pix_fmt", "yuv420p", video], stdin=subprocess.PIPE)
    for t, pacote in registros:
        w2, h2, x, qtd, luz, img, mask = decodificar(pacote)
        if (w2, h2) == (w, h):
            ff.stdin.write(compor(w, h, x, img, mask).tobytes())
    ff.stdin.close(); ff.wait()
    print(f"video: {video} ({fps} quadros/s)")
