// Driver GC9B72 (360 x 360, SPI de 4 fios) para a biblioteca Arduino_GFX.
// A sequência de inicialização vem do componente ESP-IDF viewesmart/esp_lcd_gc9b72 1.0.0
// (Apache-2.0, (c) 2023 Espressif Systems, (c) 2026 VIEWE), adaptada para o formato
// batchOperation da Arduino_GFX. O restante segue o modelo do Arduino_GC9A01.
#pragma once

#include <Arduino_GFX_Library.h>

#define GC9B72_TFTWIDTH 360
#define GC9B72_TFTHEIGHT 360
#define GC9B72_RST_DELAY 120
#define GC9B72_SLPOUT_DELAY 120

#define GC9B72_SLPIN 0x10
#define GC9B72_SLPOUT 0x11
#define GC9B72_INVOFF 0x20
#define GC9B72_INVON 0x21
#define GC9B72_DISPON 0x29
#define GC9B72_CASET 0x2A
#define GC9B72_RASET 0x2B
#define GC9B72_RAMWR 0x2C
#define GC9B72_MADCTL 0x36
#define GC9B72_COLMOD 0x3A

#define GC9B72_MADCTL_MY 0x80
#define GC9B72_MADCTL_MX 0x40
#define GC9B72_MADCTL_MV 0x20
#define GC9B72_MADCTL_BGR 0x08

static const uint8_t gc9b72_init_operations[] = {
    BEGIN_WRITE,
    WRITE_C8_D8, GC9B72_COLMOD, 0x55, // RGB565
    WRITE_COMMAND_8, 0xFE,            // entra no modo de registradores do fabricante
    WRITE_COMMAND_8, 0xEF,
    WRITE_C8_D8, 0x80, 0x19,
    WRITE_C8_D8, 0x82, 0x09,
    WRITE_C8_D8, 0x83, 0x03,
    WRITE_C8_D8, 0x88, 0x00,
    WRITE_C8_D8, 0x89, 0x38,
    WRITE_C8_D8, 0x8A, 0x40,
    WRITE_C8_D8, 0x8B, 0x0A,
    WRITE_C8_D8, 0x8C, 0x00,
    WRITE_C8_D8, 0x81, 0xFF,
    WRITE_C8_D8, 0x84, 0xFF,
    WRITE_C8_D8, 0x85, 0xFF,
    WRITE_C8_D8, 0x86, 0xFF,
    WRITE_C8_D8, 0x87, 0xFF,
    WRITE_C8_D8, 0x8E, 0xFF,
    WRITE_C8_D8, 0x8F, 0xFF,
    WRITE_C8_D8, 0x98, 0x3E,
    WRITE_C8_D8, 0x99, 0x3E,
    WRITE_C8_D8, 0x7D, 0x72,
    WRITE_COMMAND_8, 0x70,
    WRITE_BYTES, 10, 0x02, 0x03, 0x03, 0x06, 0x03, 0x03, 0x09, 0x07, 0x09, 0x03,
    WRITE_COMMAND_8, 0x90,
    WRITE_BYTES, 4, 0x06, 0x06, 0x01, 0x01,
    WRITE_COMMAND_8, 0x93,
    WRITE_BYTES, 3, 0x02, 0xFF, 0x00,
    WRITE_C8_D8, 0xCB, 0x02,
    WRITE_C8_D16, 0xFB, 0x00, 0x00,
    WRITE_C8_D8, 0xF6, 0xC0,
    WRITE_COMMAND_8, 0x6C,
    WRITE_BYTES, 7, 0x00, 0x00, 0x22, 0x00, 0xCC, 0x04, 0x58,
    WRITE_C8_D16, 0xAA, 0x0B, 0x00,
    WRITE_C8_D8, 0xEC, 0x07,
    WRITE_C8_D8, 0xF9, 0x40,
    WRITE_C8_D16, 0xEB, 0x01, 0x67,
    WRITE_COMMAND_8, 0x74,
    WRITE_BYTES, 6, 0x01, 0x60, 0x00, 0x00, 0x00, 0x00,
    WRITE_COMMAND_8, 0xB5,
    WRITE_BYTES, 3, 0x14, 0x14, 0x14,
    WRITE_COMMAND_8, 0x6E,
    WRITE_BYTES, 32,
    0x0B, 0x0B, 0x09, 0x09, 0x13, 0x13, 0x11, 0x11,
    0x16, 0x15, 0x01, 0x04, 0x00, 0x0D, 0x1D, 0x00,
    0x00, 0x1D, 0x0D, 0x00, 0x04, 0x08, 0x15, 0x16,
    0x12, 0x12, 0x14, 0x14, 0x0A, 0x0A, 0x0C, 0x0C,
    WRITE_COMMAND_8, 0x60,
    WRITE_BYTES, 4, 0x38, 0x1C, 0x13, 0x56,
    WRITE_COMMAND_8, 0x61,
    WRITE_BYTES, 4, 0xF8, 0x0A, 0x13, 0x56,
    WRITE_COMMAND_8, 0x62,
    WRITE_BYTES, 4, 0xF8, 0x0B, 0x13, 0x56,
    WRITE_COMMAND_8, 0x63,
    WRITE_BYTES, 4, 0x38, 0x1C, 0x13, 0x56,
    WRITE_COMMAND_8, 0x64,
    WRITE_BYTES, 6, 0x38, 0x20, 0x72, 0xF8, 0x13, 0x56,
    WRITE_COMMAND_8, 0x65,
    WRITE_BYTES, 6, 0x78, 0x1A, 0x70, 0x0B, 0x56, 0x13,
    WRITE_COMMAND_8, 0x66,
    WRITE_BYTES, 6, 0x38, 0x24, 0x72, 0xFC, 0x13, 0x56,
    WRITE_COMMAND_8, 0x68,
    WRITE_BYTES, 7, 0xB3, 0x08, 0x0E, 0x08, 0x0E, 0x0A, 0x0A,
    WRITE_COMMAND_8, 0x69,
    WRITE_BYTES, 7, 0xB3, 0x08, 0x0E, 0x08, 0x0E, 0x0A, 0x0A,
    WRITE_C8_D16, 0x6A, 0x00, 0x00,
    WRITE_C8_D16, 0x7C, 0xB6, 0x29,
    WRITE_C8_D8, 0xAC, 0x40,
    WRITE_C8_D8, 0xC3, 0x1A,
    WRITE_C8_D8, 0xC4, 0x24,
    WRITE_C8_D8, 0xC9, 0x2F,
    WRITE_COMMAND_8, 0xF0,
    WRITE_BYTES, 6, 0x11, 0x17, 0x08, 0x06, 0x05, 0x38,
    WRITE_COMMAND_8, 0xF1,
    WRITE_BYTES, 6, 0x4D, 0x72, 0x72, 0x2D, 0x34, 0x8F,
    WRITE_COMMAND_8, 0xF2,
    WRITE_BYTES, 6, 0x11, 0x17, 0x08, 0x06, 0x05, 0x38,
    WRITE_COMMAND_8, 0xF3,
    WRITE_BYTES, 6, 0x4D, 0x72, 0x72, 0x2D, 0x34, 0x8F,
    WRITE_C8_D8, 0xB4, 0x0A,
    WRITE_C8_D8, 0x35, 0x00,          // sinal de TE ligado
    WRITE_COMMAND_8, 0xFE,            // sai do modo do fabricante
    WRITE_COMMAND_8, 0xEE,
    WRITE_COMMAND_8, GC9B72_SLPOUT,
    END_WRITE,
    DELAY, GC9B72_SLPOUT_DELAY,
    BEGIN_WRITE,
    WRITE_COMMAND_8, GC9B72_DISPON,
    END_WRITE,
    DELAY, 20};

class Arduino_GC9B72 : public Arduino_TFT
{
public:
  Arduino_GC9B72(Arduino_DataBus *bus, int8_t rst = GFX_NOT_DEFINED, uint8_t r = 0,
                 bool ips = false, int16_t w = GC9B72_TFTWIDTH, int16_t h = GC9B72_TFTHEIGHT)
      : Arduino_TFT(bus, rst, r, ips, w, h, 0, 0, 0, 0) {}

  bool begin(int32_t speed = GFX_NOT_DEFINED) override
  {
    _override_datamode = SPI_MODE0;
    return Arduino_TFT::begin(speed);
  }

  // true = a tela espera as cores na ordem BGR (vermelho e azul trocados)
  void setBGR(bool bgr)
  {
    _bgr = bgr;
    setRotation(_rotation);
  }
  bool bgr() const { return _bgr; }

  void writeAddrWindow(int16_t x, int16_t y, uint16_t w, uint16_t h) override
  {
    if ((x != _currentX) || (w != _currentW) || (y != _currentY) || (h != _currentH))
    {
      _bus->writeC8D16D16(GC9B72_CASET, x + _xStart, x + w - 1 + _xStart);
      _bus->writeC8D16D16(GC9B72_RASET, y + _yStart, y + h - 1 + _yStart);
      _currentX = x;
      _currentY = y;
      _currentW = w;
      _currentH = h;
    }
    _bus->writeCommand(GC9B72_RAMWR);
  }

  void setRotation(uint8_t r) override
  {
    Arduino_TFT::setRotation(r);
    uint8_t m;
    switch (_rotation)
    {
    case 1: m = GC9B72_MADCTL_MX | GC9B72_MADCTL_MV; break;
    case 2: m = GC9B72_MADCTL_MX | GC9B72_MADCTL_MY; break;
    case 3: m = GC9B72_MADCTL_MY | GC9B72_MADCTL_MV; break;
    default: m = 0; break;
    }
    if (_bgr) m |= GC9B72_MADCTL_BGR;
    _bus->beginWrite();
    _bus->writeC8D8(GC9B72_MADCTL, m);
    _bus->endWrite();
  }

  void invertDisplay(bool i) override
  {
    _bus->sendCommand(_ips ? (i ? GC9B72_INVOFF : GC9B72_INVON) : (i ? GC9B72_INVON : GC9B72_INVOFF));
  }

  void displayOn() override
  {
    _bus->sendCommand(GC9B72_SLPOUT);
    delay(GC9B72_SLPOUT_DELAY);
  }

  void displayOff() override
  {
    _bus->sendCommand(GC9B72_SLPIN);
    delay(GC9B72_SLPOUT_DELAY);
  }

protected:
  void tftInit() override
  {
    if (_rst != GFX_NOT_DEFINED)
    {
      pinMode(_rst, OUTPUT);
      digitalWrite(_rst, HIGH);
      delay(10);
      digitalWrite(_rst, LOW);
      delay(10);
      digitalWrite(_rst, HIGH);
      delay(GC9B72_RST_DELAY);
    }
    _bus->batchOperation(gc9b72_init_operations, sizeof(gc9b72_init_operations));
    invertDisplay(false);
  }

private:
  bool _bgr = false;
};
