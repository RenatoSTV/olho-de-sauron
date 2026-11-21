#!/usr/bin/env python3
"""Gera a íris do Olho de Sauron (360 x 360) no mesmo estilo da prévia interativa.

Saídas:
  etapa2_iris/iris.h          array RGB565 da íris SEM a pupila (vai para a flash do XIAO)
  previa/iris_base.png        a íris sozinha
  previa/previa_olho.png      a tela redonda simulada, pupila contraída e dilatada lado a lado

A pupila é calculada pela mesma fórmula que o XIAO usa (pupila()), então a prévia
mostra o que a tela vai mostrar.

Uso: python3 ferramentas/gerar_iris.py [semente]
"""
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

S = 360
C = S / 2
R = S / 2
ESCALA = 2  # desenha em 720 x 720 e reduz, para suavizar os raios
RAIZ = Path(__file__).resolve().parent.parent

# Mesmas cores e posições do degradê da prévia interativa
PARADAS = [
    (0.00, (0xFF, 0xF6, 0xC8)),
    (0.18, (0xFF, 0xCF, 0x4A)),
    (0.42, (0xF2, 0x7A, 0x12)),
    (0.68, (0xA8, 0x26, 0x0A)),
    (0.88, (0x3A, 0x08, 0x03)),
    (1.00, (0x00, 0x00, 0x00)),
]
COR_BRILHO = np.array([255, 190, 70], dtype=np.float64)  # halo da pupila
SIGMA_BRILHO = 11.0  # shadowBlur 22 do canvas ~ gaussiana de sigma 11


def degrade(t):
    t = np.clip(t, 0, 1)
    out = np.zeros(t.shape + (3,))
    for (t0, c0), (t1, c1) in zip(PARADAS, PARADAS[1:]):
        m = (t >= t0) & (t <= t1)
        f = ((t - t0) / (t1 - t0))[m][:, None]
        out[m] = np.array(c0) * (1 - f) + np.array(c1) * f
    return out


def gerar_iris(semente=7):
    N = S * ESCALA
    y, x = np.mgrid[0:N, 0:N] / ESCALA + 0.5 / ESCALA
    r = np.hypot(x - C, y - C)

    # Degradê radial (raio interno 4 px, externo R)
    img = degrade((r - 4) / (R - 4))

    # Raios de fogo somados à luz ("lighter" no canvas)
    rng = np.random.default_rng(semente)
    camada = Image.new("RGB", (N, N))
    acum = np.zeros((N, N, 3))
    for i in range(110):
        a0 = (i / 110) * 2 * math.pi + rng.random() * 0.05
        comp = 0.45 + rng.random() * 0.4
        vel = 0.6 + rng.random() * 1.6
        fase = rng.random() * 6.28
        larg = 0.8 + rng.random() * 1.8
        fl = 0.65 + 0.35 * math.sin(fase)
        a = a0 + math.sin(fase) * 0.04
        r0, r1 = 26, R * comp * fl
        curva = math.sin(fase * 0.7) * 0.12
        pts = []
        p0 = (C + math.cos(a) * r0, C + math.sin(a) * r0)
        pc = (C + math.cos(a + curva) * r1 * 0.6, C + math.sin(a + curva) * r1 * 0.6)
        p1 = (C + math.cos(a) * r1, C + math.sin(a) * r1)
        for k in range(25):
            t = k / 24
            px = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * pc[0] + t * t * p1[0]
            py = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * pc[1] + t * t * p1[1]
            pts.append((px * ESCALA, py * ESCALA))
        mascara = Image.new("L", (N, N))
        ImageDraw.Draw(mascara).line(pts, fill=255, width=max(1, round(larg * ESCALA)), joint="curve")
        m = np.asarray(mascara, dtype=np.float64)[..., None] / 255
        alfa = 0.09 + fl * 0.12
        cor = np.array([255, 150 + round(fl * 80), 60])
        acum += m * cor * alfa
    img = np.minimum(img + acum, 255)

    # Escurece a borda: transparente em 0,7 R, 85 % de preto em R
    v = np.clip((r - 0.7 * R) / (0.3 * R), 0, 1) * 0.85
    img *= (1 - v)[..., None]
    img[r > R] = 0

    # Reduz para 360 x 360 com média por bloco
    img = img.reshape(S, ESCALA, S, ESCALA, 3).mean(axis=(1, 3))
    return img


def distancia_elipse(y0, y1, e0, e1, iteracoes=40):
    """Distância exata de pontos (y0, y1 >= 0, fora da elipse) até a elipse de semieixos e0 >= e1.
    Método de Eberly (bissecção), o mesmo que o XIAO usa."""
    z0, z1 = y0 / e0, y1 / e1
    r0 = (e0 / e1) ** 2
    n0 = r0 * z0
    s0 = z1 - 1
    s1 = np.hypot(n0, z1) - 1
    for _ in range(iteracoes):
        s = (s0 + s1) / 2
        g = (n0 / (s + r0)) ** 2 + (z1 / (s + 1)) ** 2 - 1
        s0 = np.where(g > 0, s, s0)
        s1 = np.where(g > 0, s1, s)
    s = (s0 + s1) / 2
    x0 = r0 * y0 / (s + r0)
    x1 = y1 / (s + 1)
    return np.hypot(x0 - y0, x1 - y1)


def pupila(base, cx, cy, pw, ph):
    """Aplica a pupila em fenda com halo laranja. Mesma conta do XIAO (etapa2_iris.ino)."""
    out = base.copy()
    y, x = np.mgrid[0:S, 0:S] + 0.5
    dx, dy = np.abs(x - cx), np.abs(y - cy)
    e = np.sqrt((dx / pw) ** 2 + (dy / ph) ** 2)
    fora = e > 1
    dist = np.zeros_like(e)
    dist[fora] = distancia_elipse(dy[fora], dx[fora], ph, pw)  # eixo maior é o vertical
    erfc = np.frompyfunc(math.erfc, 1, 1)
    a = 0.95 * 0.5 * erfc(dist / (SIGMA_BRILHO * math.sqrt(2))).astype(np.float64)
    # o brilho também é escurecido perto da borda, como na prévia interativa
    r = np.hypot(x - C, y - C)
    vinheta = 1 - np.clip((r - 0.7 * R) / (0.3 * R), 0, 1) * 0.85
    out = out * (1 - a[..., None]) + COR_BRILHO * (a * vinheta)[..., None]
    out[~fora] = 0
    return out


def para_rgb565(img):
    c = np.clip(np.rint(img), 0, 255).astype(np.uint16)
    return ((c[..., 0] >> 3) << 11) | ((c[..., 1] >> 2) << 5) | (c[..., 2] >> 3)


def de_rgb565(v):
    r = ((v >> 11) & 0x1F) * 255 / 31
    g = ((v >> 5) & 0x3F) * 255 / 63
    b = (v & 0x1F) * 255 / 31
    return np.stack([r, g, b], axis=-1)


def salvar_header(v565, caminho):
    linhas = []
    plano = v565.flatten()
    for i in range(0, len(plano), 16):
        linhas.append("  " + ", ".join(f"0x{x:04X}" for x in plano[i:i + 16]) + ",")
    texto = (
        "// Gerado por ferramentas/gerar_iris.py - não editar à mão.\n"
        "// Íris do Olho de Sauron, 360 x 360, RGB565, sem a pupila.\n"
        "#pragma once\n#include <stdint.h>\n\n"
        f"#define IRIS_LARG {S}\n#define IRIS_ALT {S}\n\n"
        f"const uint16_t iris[{S * S}] = {{\n" + "\n".join(linhas) + "\n};\n"
    )
    caminho.parent.mkdir(parents=True, exist_ok=True)
    caminho.write_text(texto)


def tela_redonda(img):
    """Simula a tela: o que fica fora do círculo não aparece."""
    im = Image.fromarray(np.clip(np.rint(img), 0, 255).astype(np.uint8))
    fundo = Image.new("RGB", (S + 40, S + 40), (24, 18, 16))
    mascara = Image.new("L", (S, S))
    ImageDraw.Draw(mascara).ellipse((0, 0, S - 1, S - 1), fill=255)
    fundo.paste(im, (20, 20), mascara)
    return fundo


PUPILA_CONTRAIDA = (7.0, 132.0)   # semilargura, semialtura (sala vazia)
PUPILA_DILATADA = (16.0, 142.0)   # alguém detectado
NIVEIS = 8                        # níveis de dilatação pré-calculados
Q_L, Q_A = 61, 180                # quadrante da máscara: 16 + 45 de halo, meia tela de altura
MARGEM = 45


def mascaras_pupila():
    """Para cada nível, um quadrante (Q_A x Q_L) com 255 = dentro da pupila
    ou o alfa do halo em 1/512 (0..243). O XIAO espelha o quadrante."""
    j, i = np.mgrid[0:Q_A, 0:Q_L] + 0.5   # distância ao centro da pupila
    erfc = np.frompyfunc(math.erfc, 1, 1)
    saida = np.zeros((NIVEIS, Q_A, Q_L), dtype=np.uint8)
    for n in range(NIVEIS):
        f = n / (NIVEIS - 1)
        pw = PUPILA_CONTRAIDA[0] + (PUPILA_DILATADA[0] - PUPILA_CONTRAIDA[0]) * f
        ph = PUPILA_CONTRAIDA[1] + (PUPILA_DILATADA[1] - PUPILA_CONTRAIDA[1]) * f
        e = np.sqrt((i / pw) ** 2 + (j / ph) ** 2)
        fora = e > 1
        dist = np.zeros_like(e)
        dist[fora] = distancia_elipse(j[fora], i[fora], ph, pw)
        a = 0.95 * 0.5 * erfc(dist / (SIGMA_BRILHO * math.sqrt(2))).astype(np.float64)
        a[dist > MARGEM] = 0
        v = np.clip(np.rint(a * 512), 0, 243).astype(np.uint8)
        v[~fora] = 255
        saida[n] = v
    return saida


def salvar_mascaras(m, caminho):
    blocos = []
    for n in range(m.shape[0]):
        linhas = ["    {" + ", ".join(str(x) for x in m[n, y]) + "}," for y in range(m.shape[1])]
        blocos.append("  {\n" + "\n".join(linhas) + "\n  },")
    texto = (
        "// Gerado por ferramentas/gerar_iris.py - não editar à mão.\n"
        "// Máscaras da pupila: um quadrante por nível de dilatação.\n"
        "// 255 = dentro da pupila (preto); 0..243 = alfa do halo laranja em 1/512.\n"
        "#pragma once\n#include <stdint.h>\n\n"
        f"#define PUPILA_NIVEIS {m.shape[0]}\n#define PUPILA_QA {m.shape[1]}\n#define PUPILA_QL {m.shape[2]}\n\n"
        f"const uint8_t pupilaMascara[{m.shape[0]}][{m.shape[1]}][{m.shape[2]}] = {{\n" + "\n".join(blocos) + "\n};\n"
    )
    caminho.write_text(texto)


def main():
    semente = int(sys.argv[1]) if len(sys.argv) > 1 else 7
    base = gerar_iris(semente)
    v565 = para_rgb565(base)
    for pasta_sketch in ("etapa2_iris", "etapa3_pupila"):
        salvar_header(v565, RAIZ / pasta_sketch / "iris.h")
    salvar_mascaras(mascaras_pupila(), RAIZ / "etapa3_pupila" / "pupila.h")

    # A prévia usa a íris já convertida para RGB565, como a tela vai receber
    base_tela = de_rgb565(v565)
    contraida = pupila(base_tela, C, C, 7, 132)   # sala vazia
    dilatada = pupila(base_tela, C, C, 16, 142)   # alguém detectado

    pasta = RAIZ / "previa"
    pasta.mkdir(exist_ok=True)
    tela_redonda(base_tela).save(pasta / "iris_base.png")
    lado = Image.new("RGB", ((S + 40) * 2, S + 40), (24, 18, 16))
    lado.paste(tela_redonda(contraida), (0, 0))
    lado.paste(tela_redonda(dilatada), (S + 40, 0))
    lado.save(pasta / "previa_olho.png")
    print(f"ok: {S}x{S}, {v565.nbytes} bytes de imagem, semente {semente}")


if __name__ == "__main__":
    main()
