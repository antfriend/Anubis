#define ILI9341_DRIVER
#define USE_HSPI_PORT
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   46
#define TFT_RST  -1

#define TFT_BL   45
#define TFT_BACKLIGHT_ON HIGH

#define SPI_FREQUENCY  20000000
#define TFT_RGB_ORDER TFT_BGR

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
