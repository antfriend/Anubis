#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <Wire.h>
#include <pgmspace.h>

// Receiver-side maximum accepted channel metadata. The receiver still only
// drives RC_OUTPUT_COUNT physical PWM outputs, but packets may carry fewer
// channels from older/smaller transmitters.
#define CHANNEL_COUNT 12
#define RC_OUTPUT_COUNT 4
#define ESPNOW_LINK_MAGIC 0xA614
#define ESPNOW_LINK_VERSION 2
#define ESPNOW_WIFI_CHANNEL 6
#define HELTEC_OLED_ADDRESS 0x3C
#define HELTEC_OLED_SDA 17
#define HELTEC_OLED_SCL 18
#define HELTEC_OLED_RST 21
#define HELTEC_VEXT_PIN 36
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES (OLED_HEIGHT / 8)
#define OLED_FLIP_HORIZONTAL true
#define OLED_FLIP_VERTICAL false
#define DISPLAY_REFRESH_INTERVAL_MS 150
#define FAILSAFE_TIMEOUT_MS 300
#define TELEMETRY_INTERVAL_MS 100
#define TELEMETRY_KEEPALIVE_INTERVAL_MS 1000
#define BIND_BEACON_INTERVAL_MS 500
#define REBIND_BUTTON_PIN 0
#define REBIND_HOLD_MS 3000
#define RC_PWM_FREQUENCY_HZ 50
// ESP32-S3 LEDC supports up to 14-bit resolution.
#define RC_PWM_RESOLUTION_BITS 14
#define RC_PWM_MIN_US 1000
#define RC_PWM_NEUTRAL_US 1500
#define RC_PWM_MAX_US 2000
#define ESC_ARM_HOLD_MS 3000
// Set true for typical one-direction brushless ESC arming behavior (idle at minimum throttle).
// Set false for bidirectional ESC behavior (idle at neutral/center).
#define ESC_IDLE_AT_MIN_THROTTLE false

enum DriveType {
  DRIVE_TANK,
  DRIVE_CAR,
  DRIVE_OMNI,
  DRIVE_X_DRONE,
  DRIVE_QUAD_X = DRIVE_X_DRONE
};

enum TankControlMode {
  TANK_MODE_DUAL_STICK,
  TANK_MODE_RIGHT_STICK
};

enum EspNowMessageType : uint8_t {
  ESPNOW_MSG_CONTROL = 0x43,
  ESPNOW_MSG_TELEMETRY = 0x54,
  ESPNOW_MSG_PING = 0x50,
  ESPNOW_MSG_PING_ACK = 0x51,
  ESPNOW_MSG_BIND_BEACON = 0x42,
  ESPNOW_MSG_BIND_COMMIT = 0x62,
  ESPNOW_MSG_BIND_ACK = 0x41
};

struct __attribute__((packed)) EspNowControlPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t messageType;
  uint32_t sequence;
  uint32_t txMillis;
  uint8_t modelIndex;
  uint8_t driveType;
  uint8_t tankMode;
  uint8_t flags;
  int16_t channels[CHANNEL_COUNT];
  uint16_t txBatteryMillivolts;
  uint16_t linkLatencyMs;
  char modelName[20];
};

const size_t ESPNOW_CONTROL_PREFIX_BYTES = offsetof(EspNowControlPacket, channels);
const size_t ESPNOW_CONTROL_TRAILER_BYTES =
  sizeof(((EspNowControlPacket *)0)->txBatteryMillivolts) +
  sizeof(((EspNowControlPacket *)0)->linkLatencyMs) +
  sizeof(((EspNowControlPacket *)0)->modelName);

struct __attribute__((packed)) EspNowTelemetryPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t messageType;
  uint32_t sequence;
  uint32_t echoedTxMillis;
  uint16_t receiverBatteryMillivolts;
  uint16_t receiverPacketAgeMs;
  uint8_t flags;
};

struct __attribute__((packed)) EspNowBindPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t messageType;
  uint32_t token;
  uint8_t flags;
};

struct __attribute__((packed)) EspNowPingPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t messageType;
  uint32_t sequence;
  uint32_t txMillis;
};

struct __attribute__((packed)) EspNowPingAckPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t messageType;
  uint32_t sequence;
  uint32_t echoedTxMillis;
};

static const uint8_t GLYPH_SPACE[5] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t GLYPH_DASH[5] PROGMEM = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t GLYPH_DOT[5] PROGMEM = {0x00, 0x00, 0x00, 0x18, 0x18};
static const uint8_t GLYPH_COLON[5] PROGMEM = {0x00, 0x18, 0x00, 0x18, 0x00};
static const uint8_t GLYPH_SLASH[5] PROGMEM = {0x02, 0x04, 0x08, 0x10, 0x20};
static const uint8_t GLYPH_EXCL[5] PROGMEM = {0x08, 0x08, 0x08, 0x00, 0x08};
static const uint8_t GLYPH_0[5] PROGMEM = {0x3E, 0x45, 0x49, 0x51, 0x3E};
static const uint8_t GLYPH_1[5] PROGMEM = {0x00, 0x21, 0x7F, 0x01, 0x00};
static const uint8_t GLYPH_2[5] PROGMEM = {0x23, 0x45, 0x49, 0x51, 0x21};
static const uint8_t GLYPH_3[5] PROGMEM = {0x22, 0x41, 0x49, 0x49, 0x36};
static const uint8_t GLYPH_4[5] PROGMEM = {0x0C, 0x14, 0x24, 0x7F, 0x04};
static const uint8_t GLYPH_5[5] PROGMEM = {0x72, 0x51, 0x51, 0x51, 0x4E};
static const uint8_t GLYPH_6[5] PROGMEM = {0x1E, 0x29, 0x49, 0x49, 0x06};
static const uint8_t GLYPH_7[5] PROGMEM = {0x40, 0x47, 0x48, 0x50, 0x60};
static const uint8_t GLYPH_8[5] PROGMEM = {0x36, 0x49, 0x49, 0x49, 0x36};
static const uint8_t GLYPH_9[5] PROGMEM = {0x30, 0x49, 0x49, 0x4A, 0x3C};
static const uint8_t GLYPH_A[5] PROGMEM = {0x1F, 0x24, 0x44, 0x24, 0x1F};
static const uint8_t GLYPH_B[5] PROGMEM = {0x7F, 0x49, 0x49, 0x49, 0x36};
static const uint8_t GLYPH_C[5] PROGMEM = {0x3E, 0x41, 0x41, 0x41, 0x22};
static const uint8_t GLYPH_D[5] PROGMEM = {0x7F, 0x41, 0x41, 0x22, 0x1C};
static const uint8_t GLYPH_E[5] PROGMEM = {0x7F, 0x49, 0x49, 0x49, 0x41};
static const uint8_t GLYPH_F[5] PROGMEM = {0x7F, 0x48, 0x48, 0x48, 0x40};
static const uint8_t GLYPH_G[5] PROGMEM = {0x3E, 0x41, 0x49, 0x49, 0x2E};
static const uint8_t GLYPH_H[5] PROGMEM = {0x7F, 0x08, 0x08, 0x08, 0x7F};
static const uint8_t GLYPH_I[5] PROGMEM = {0x00, 0x41, 0x7F, 0x41, 0x00};
static const uint8_t GLYPH_J[5] PROGMEM = {0x02, 0x01, 0x01, 0x7E, 0x00};
static const uint8_t GLYPH_K[5] PROGMEM = {0x7F, 0x08, 0x14, 0x22, 0x41};
static const uint8_t GLYPH_L[5] PROGMEM = {0x7F, 0x01, 0x01, 0x01, 0x01};
static const uint8_t GLYPH_M[5] PROGMEM = {0x7F, 0x20, 0x18, 0x20, 0x7F};
static const uint8_t GLYPH_N[5] PROGMEM = {0x7F, 0x10, 0x08, 0x04, 0x7F};
static const uint8_t GLYPH_O[5] PROGMEM = {0x3E, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t GLYPH_P[5] PROGMEM = {0x7F, 0x48, 0x48, 0x48, 0x30};
static const uint8_t GLYPH_Q[5] PROGMEM = {0x3E, 0x41, 0x45, 0x42, 0x3D};
static const uint8_t GLYPH_R[5] PROGMEM = {0x7F, 0x48, 0x4C, 0x4A, 0x31};
static const uint8_t GLYPH_S[5] PROGMEM = {0x31, 0x49, 0x49, 0x49, 0x46};
static const uint8_t GLYPH_T[5] PROGMEM = {0x40, 0x40, 0x7F, 0x40, 0x40};
static const uint8_t GLYPH_U[5] PROGMEM = {0x7E, 0x01, 0x01, 0x01, 0x7E};
static const uint8_t GLYPH_V[5] PROGMEM = {0x7C, 0x02, 0x01, 0x02, 0x7C};
static const uint8_t GLYPH_W[5] PROGMEM = {0x7E, 0x01, 0x0E, 0x01, 0x7E};
static const uint8_t GLYPH_X[5] PROGMEM = {0x63, 0x14, 0x08, 0x14, 0x63};
static const uint8_t GLYPH_Y[5] PROGMEM = {0x70, 0x08, 0x07, 0x08, 0x70};
static const uint8_t GLYPH_Z[5] PROGMEM = {0x43, 0x45, 0x49, 0x51, 0x61};

const uint8_t* glyphForChar(char c) {
  if (c >= 'a' && c <= 'z') c = (char)(c - 32);
  switch (c) {
    case 'A': return GLYPH_A; case 'B': return GLYPH_B; case 'C': return GLYPH_C;
    case 'D': return GLYPH_D; case 'E': return GLYPH_E; case 'F': return GLYPH_F;
    case 'G': return GLYPH_G; case 'H': return GLYPH_H; case 'I': return GLYPH_I;
    case 'J': return GLYPH_J; case 'K': return GLYPH_K; case 'L': return GLYPH_L;
    case 'M': return GLYPH_M; case 'N': return GLYPH_N; case 'O': return GLYPH_O;
    case 'P': return GLYPH_P; case 'Q': return GLYPH_Q; case 'R': return GLYPH_R;
    case 'S': return GLYPH_S; case 'T': return GLYPH_T; case 'U': return GLYPH_U;
    case 'V': return GLYPH_V; case 'W': return GLYPH_W; case 'X': return GLYPH_X;
    case 'Y': return GLYPH_Y; case 'Z': return GLYPH_Z;
    case '0': return GLYPH_0; case '1': return GLYPH_1; case '2': return GLYPH_2;
    case '3': return GLYPH_3; case '4': return GLYPH_4; case '5': return GLYPH_5;
    case '6': return GLYPH_6; case '7': return GLYPH_7; case '8': return GLYPH_8;
    case '9': return GLYPH_9; case '-': return GLYPH_DASH; case '.': return GLYPH_DOT;
    case ':': return GLYPH_COLON; case '/': return GLYPH_SLASH; case '!': return GLYPH_EXCL;
    default: return GLYPH_SPACE;
  }
}

struct TinyOLED {
  uint8_t buffer[OLED_WIDTH * OLED_PAGES];

  void command(uint8_t value) {
    Wire.beginTransmission(HELTEC_OLED_ADDRESS);
    Wire.write(0x00);
    Wire.write(value);
    Wire.endTransmission();
  }

  void begin() {
    pinMode(HELTEC_VEXT_PIN, OUTPUT);
    digitalWrite(HELTEC_VEXT_PIN, LOW);
    delay(20);

    pinMode(HELTEC_OLED_RST, OUTPUT);
    digitalWrite(HELTEC_OLED_RST, LOW);
    delay(20);
    digitalWrite(HELTEC_OLED_RST, HIGH);

    Wire.begin(HELTEC_OLED_SDA, HELTEC_OLED_SCL);
    Wire.setClock(400000);

    command(0xAE);
    command(0xD5); command(0x80);
    command(0xA8); command(0x3F);
    command(0xD3); command(0x00);
    command(0x40);
    command(0x8D); command(0x14);
    command(0x20); command(0x00);
    command(0xA0);
    command(0xC0);
    command(0xDA); command(0x12);
    command(0x81); command(0xCF);
    command(0xD9); command(0xF1);
    command(0xDB); command(0x40);
    command(0xA4);
    command(0xA6);
    command(0x2E);
    command(0xAF);
    clear();
    display();
  }

  void clear() {
    memset(buffer, 0, sizeof(buffer));
  }

  void setPixel(int x, int y, bool on = true) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    const int tx = OLED_FLIP_HORIZONTAL ? (OLED_WIDTH - 1 - x) : x;
    const int ty = OLED_FLIP_VERTICAL ? (OLED_HEIGHT - 1 - y) : y;
    const uint16_t index = tx + (ty / 8) * OLED_WIDTH;
    const uint8_t mask = 1 << (ty & 7);
    if (on) buffer[index] |= mask;
    else buffer[index] &= ~mask;
  }

  void drawChar(int x, int y, char c, uint8_t scale = 1) {
    const uint8_t* glyph = glyphForChar(c);
    for (uint8_t col = 0; col < 5; col++) {
      uint8_t bits = pgm_read_byte(glyph + col);
      for (uint8_t row = 0; row < 7; row++) {
        if (bits & (1 << row)) {
          for (uint8_t sx = 0; sx < scale; sx++) {
            for (uint8_t sy = 0; sy < scale; sy++) {
              setPixel(x + (col * scale) + sx, y + (row * scale) + sy);
            }
          }
        }
      }
    }
  }

  void drawText(int x, int y, const char* text, uint8_t scale = 1) {
    while (*text) {
      drawChar(x, y, *text, scale);
      x += 6 * scale;
      text++;
    }
  }

  int textWidth(const char* text, uint8_t scale = 1) {
    return (int)strlen(text) * 6 * scale;
  }

  void drawCenteredText(int y, const char* text, uint8_t scale = 1) {
    drawText((OLED_WIDTH - textWidth(text, scale)) / 2, y, text, scale);
  }

  void display() {
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
      command(0xB0 + page);
      command(0x00);
      command(0x10);
      for (uint8_t x = 0; x < OLED_WIDTH; x += 16) {
        Wire.beginTransmission(HELTEC_OLED_ADDRESS);
        Wire.write(0x40);
        for (uint8_t i = 0; i < 16; i++) {
          Wire.write(buffer[(page * OLED_WIDTH) + x + i]);
        }
        Wire.endTransmission();
      }
    }
  }
};

const uint8_t OUTPUT_PINS[RC_OUTPUT_COUNT] = {4, 5, 6, 7};
const uint8_t OUTPUT_LEDC_CHANNEL[RC_OUTPUT_COUNT] = {0, 1, 2, 3};
// Output-to-channel map. The transmitter sends final post-mix/post-reverse
// channel values, so the receiver should only route CH1..CH4 to PWM outputs.
const uint8_t OUTPUT_SOURCE_CHANNEL[RC_OUTPUT_COUNT] = {0, 1, 2, 3};
// Per-output mode:
// true  -> unidirectional ESC mapping (idle/min at 1000us)
// false -> bidirectional servo/ESC mapping (center at 1500us)
const bool OUTPUT_UNIDIRECTIONAL_ESC[RC_OUTPUT_COUNT] = {false, false, true, false};
// Per-output failsafe/boot idle level:
// true  -> hold at minimum pulse (1000us)
// false -> hold at neutral pulse (1500us)
const bool OUTPUT_IDLE_AT_MIN_THROTTLE[RC_OUTPUT_COUNT] = {false, false, true, false};
// Per-output direction for unidirectional mapping:
// true  -> positive control value increases throttle
// false -> negative control value increases throttle
const bool OUTPUT_ESC_POSITIVE_IS_FORWARD[RC_OUTPUT_COUNT] = {true, true, true, true};
// Per-output throttle interpretation for unidirectional ESC mode:
// true  -> use only the active half of the stick for throttle (idle at 1000us at center)
// false -> map full -1..+1 range to 1000..2000
const bool OUTPUT_ESC_USE_ACTIVE_HALF_ONLY[RC_OUTPUT_COUNT] = {false, false, true, false};
// Heltec WiFi LoRa 32 V3 battery sense: GPIO37 controls the divider feeding
// GPIO1 / ADC1_CH0. Different board revisions/examples disagree on polarity,
// so the code auto-detects the state that produces a plausible LiPo voltage.
const int RECEIVER_BATTERY_ADC_PIN = 1;
const int RECEIVER_BATTERY_ADC_CTRL_PIN = 37;
const float RECEIVER_BATTERY_DIVIDER_RATIO = 4.9f;
const float RECEIVER_BATTERY_EMPTY_V = 3.30f;
const float RECEIVER_BATTERY_FULL_V = 4.20f;
const uint16_t RECEIVER_BATTERY_PRESENT_MIN_MV = 2500;
const uint16_t RECEIVER_BATTERY_PRESENT_MAX_MV = 5200;
const uint8_t ESPNOW_BROADCAST_ADDRESS[ESP_NOW_ETH_ALEN] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

TinyOLED display;

Preferences receiverPrefs;
portMUX_TYPE controlPacketMux = portMUX_INITIALIZER_UNLOCKED;
EspNowControlPacket latestControlPacket = {};
uint8_t latestControlChannelCount = 0;
bool hasControlPacket = false;
bool telemetryPending = false;
bool senderPeerReady = false;
bool receiverBound = false;
bool rebindTriggered = false;
unsigned long lastBindBeaconTime = 0;
unsigned long lastPacketMillis = 0;
unsigned long lastTelemetrySendTime = 0;
unsigned long rebindButtonPressStart = 0;
unsigned long escArmHoldStart = 0;
bool escArmHoldActive = true;
uint8_t lastSenderMac[6] = { 0, 0, 0, 0, 0, 0 };
uint8_t boundTransmitterMac[6] = { 0, 0, 0, 0, 0, 0 };
uint16_t lastOutputPulseUs[RC_OUTPUT_COUNT] = { RC_PWM_NEUTRAL_US, RC_PWM_NEUTRAL_US, RC_PWM_NEUTRAL_US, RC_PWM_NEUTRAL_US };
bool outputPwmAttached[RC_OUTPUT_COUNT] = { false, false, false, false };
bool outputUseChannelWrite[RC_OUTPUT_COUNT] = { false, false, false, false };
uint32_t pwmWriteFailureCount = 0;
bool displayReady = false;
unsigned long lastDisplayRefreshTime = 0;

bool initEspNowReceiver();
bool setEspNowWifiChannel(uint8_t channel);
bool ensureSenderPeer(const uint8_t *macAddress);
bool ensureBroadcastPeer();
void loadReceiverBinding();
void saveReceiverBinding();
void clearReceiverBinding();
bool parseEspNowControlPacket(const uint8_t *data, int len, EspNowControlPacket &packet, uint8_t &channelCount);
void applyControlPacket(const EspNowControlPacket &packet);
void applyFailsafeOutputs();
void writeRcPulseUs(int channelIndex, uint16_t pulseUs);
uint16_t normalizedToPulseUs(float value);
uint16_t normalizedToPulseUsForOutput(float value, int outputIndex);
uint32_t pulseUsToDuty(uint16_t pulseUs);
float packetValueToNormalized(int16_t value);
uint16_t readReceiverBatteryMillivoltsWithCtrl(int ctrlLevel);
uint16_t readReceiverBatteryMillivolts();
int receiverBatteryPercentFromMillivolts(uint16_t millivolts);
void sendTelemetryPacket(unsigned long now);
void sendBindBeacon(unsigned long now);
void sendBindAck(const uint8_t *macAddress, uint32_t token);
void sendPingAck(const uint8_t *macAddress, uint32_t sequence, uint32_t echoedTxMillis);
void updateRebindButton(unsigned long now);
void onEspNowReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void initRcPwmOutputs();
void initHeltecDisplay();
void updateReceiverDisplay(unsigned long now, bool packetFresh);
const char* driveTypeLabel(uint8_t driveType);
void copyPacketModelName(const EspNowControlPacket &packet, char *buffer, size_t bufferSize);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
void onEspNowSend(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
  (void)tx_info;
  (void)status;
}
#else
void onEspNowSend(const uint8_t *macAddress, esp_now_send_status_t status) {
  (void)macAddress;
  (void)status;
}
#endif

bool setEspNowWifiChannel(uint8_t channel) {
  if (WiFi.setChannel(channel)) {
    return true;
  }

  esp_err_t promiscOn = esp_wifi_set_promiscuous(true);
  esp_err_t setChan = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_err_t promiscOff = esp_wifi_set_promiscuous(false);

  if (promiscOn == ESP_OK && setChan == ESP_OK && promiscOff == ESP_OK) {
    return true;
  }

  Serial.printf("Failed to set ESP-NOW channel (set=%d on=%d off=%d)\n",
                (int)setChan, (int)promiscOn, (int)promiscOff);
  return false;
}

bool initEspNowReceiver() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  unsigned long wifiStart = millis();
  while (!WiFi.STA.started() && millis() - wifiStart < 1000) {
    delay(10);
  }

  if (!WiFi.STA.started()) {
    Serial.println("WiFi STA start timed out");
    return false;
  }

  WiFi.disconnect();
  if (!setEspNowWifiChannel(ESPNOW_WIFI_CHANNEL)) {
    return false;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("esp_now_init failed");
    return false;
  }

  if (esp_now_register_recv_cb(onEspNowReceive) != ESP_OK) {
    Serial.println("ESP-NOW receive callback registration failed");
    return false;
  }

  if (esp_now_register_send_cb(onEspNowSend) != ESP_OK) {
    Serial.println("ESP-NOW send callback registration failed");
    return false;
  }

  return true;
}

bool ensureSenderPeer(const uint8_t *macAddress) {
  if (esp_now_is_peer_exist(macAddress)) {
    senderPeerReady = true;
    return true;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macAddress, ESP_NOW_ETH_ALEN);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);
  if (result != ESP_OK) {
    Serial.printf("Failed to add sender peer: %d\n", (int)result);
    senderPeerReady = false;
    return false;
  }

  senderPeerReady = true;
  return true;
}

bool ensureBroadcastPeer() {
  if (esp_now_is_peer_exist(ESPNOW_BROADCAST_ADDRESS)) return true;

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, ESPNOW_BROADCAST_ADDRESS, ESP_NOW_ETH_ALEN);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);
  if (result != ESP_OK) {
    Serial.printf("Failed to add broadcast peer: %d\n", (int)result);
    return false;
  }

  return true;
}

void loadReceiverBinding() {
  receiverPrefs.begin("espnow-rx", false);
  size_t loaded = receiverPrefs.getBytes("txmac", boundTransmitterMac, sizeof(boundTransmitterMac));
  receiverBound = (loaded == sizeof(boundTransmitterMac));
  if (!receiverBound) {
    memset(boundTransmitterMac, 0, sizeof(boundTransmitterMac));
  }
}

void saveReceiverBinding() {
  receiverPrefs.putBytes("txmac", boundTransmitterMac, sizeof(boundTransmitterMac));
  receiverBound = true;
}

void clearReceiverBinding() {
  receiverPrefs.remove("txmac");
  memset(boundTransmitterMac, 0, sizeof(boundTransmitterMac));
  memset(lastSenderMac, 0, sizeof(lastSenderMac));
  receiverBound = false;
  senderPeerReady = false;
  hasControlPacket = false;
  latestControlChannelCount = 0;
  telemetryPending = false;
  lastPacketMillis = 0;
  lastTelemetrySendTime = 0;
  lastBindBeaconTime = 0;
  applyFailsafeOutputs();
  Serial.println("Binding cleared; receiver is back in beacon mode");
}

bool parseEspNowControlPacket(const uint8_t *data, int len, EspNowControlPacket &packet, uint8_t &channelCount) {
  packet = {};
  channelCount = 0;
  if (data == nullptr || len < 0) return false;

  size_t packetLen = (size_t)len;
  size_t minimumLen = ESPNOW_CONTROL_PREFIX_BYTES + sizeof(int16_t) + ESPNOW_CONTROL_TRAILER_BYTES;
  if (packetLen < minimumLen) return false;

  size_t channelBytes = packetLen - ESPNOW_CONTROL_PREFIX_BYTES - ESPNOW_CONTROL_TRAILER_BYTES;
  if ((channelBytes % sizeof(int16_t)) != 0) return false;

  size_t encodedChannels = channelBytes / sizeof(int16_t);
  if (encodedChannels == 0) return false;

  memcpy(&packet, data, ESPNOW_CONTROL_PREFIX_BYTES);

  size_t copiedChannels = min(encodedChannels, (size_t)CHANNEL_COUNT);
  const uint8_t *channelData = data + ESPNOW_CONTROL_PREFIX_BYTES;
  for (size_t i = 0; i < copiedChannels; i++) {
    int16_t channelValue = 0;
    memcpy(&channelValue, channelData + (i * sizeof(channelValue)), sizeof(channelValue));
    packet.channels[i] = channelValue;
  }
  channelCount = (uint8_t)copiedChannels;

  const uint8_t *trailer = channelData + channelBytes;
  memcpy(&packet.txBatteryMillivolts, trailer, sizeof(packet.txBatteryMillivolts));
  trailer += sizeof(packet.txBatteryMillivolts);
  memcpy(&packet.linkLatencyMs, trailer, sizeof(packet.linkLatencyMs));
  trailer += sizeof(packet.linkLatencyMs);
  memcpy(packet.modelName, trailer, sizeof(packet.modelName));

  return true;
}

const char* driveTypeLabel(uint8_t driveType) {
  switch ((DriveType)driveType) {
    case DRIVE_TANK:
      return "Tank";
    case DRIVE_CAR:
      return "Car";
    case DRIVE_OMNI:
      return "Omni";
    case DRIVE_X_DRONE:
      return "X-Drone";
    default:
      return "Unknown";
  }
}

void copyPacketModelName(const EspNowControlPacket &packet, char *buffer, size_t bufferSize) {
  if (buffer == nullptr || bufferSize == 0) return;

  size_t copyLen = strnlen(packet.modelName, sizeof(packet.modelName));
  if (copyLen == 0) {
    snprintf(buffer, bufferSize, "Model %u", (unsigned int)packet.modelIndex + 1);
    return;
  }

  copyLen = min(copyLen, bufferSize - 1);
  memcpy(buffer, packet.modelName, copyLen);
  buffer[copyLen] = '\0';
}

void initHeltecDisplay() {
  display.begin();
  display.clear();
  display.drawCenteredText(8, "ESP-NOW RX", 2);
  display.drawCenteredText(32, "WAITING", 1);
  display.display();
  displayReady = true;
}

void updateReceiverDisplay(unsigned long now, bool packetFresh) {
  if (!displayReady) return;
  if (now - lastDisplayRefreshTime < DISPLAY_REFRESH_INTERVAL_MS) return;
  lastDisplayRefreshTime = now;

  EspNowControlPacket packetCopy = {};
  bool hasPacket = false;
  portENTER_CRITICAL(&controlPacketMux);
  if (hasControlPacket) {
    packetCopy = latestControlPacket;
    hasPacket = true;
  }
  portEXIT_CRITICAL(&controlPacketMux);

  char modelName[24] = "No model";
  if (hasPacket) {
    copyPacketModelName(packetCopy, modelName, sizeof(modelName));
  }

  display.clear();
  display.drawCenteredText(0, modelName, 1);
  display.drawText(0, 14, receiverBound ? (packetFresh ? "LINK:OK" : "LINK:FAILSAFE") : "LINK:BIND", 1);

  char latencyText[32];
  if (hasPacket && packetCopy.linkLatencyMs > 0) {
    snprintf(latencyText, sizeof(latencyText), "LAT:%uMS",
             (unsigned int)packetCopy.linkLatencyMs);
  } else {
    snprintf(latencyText, sizeof(latencyText), "LAT:--MS");
  }
  display.drawText(0, 28, latencyText, 1);

  char driveText[32];
  if (hasPacket) {
    snprintf(driveText, sizeof(driveText), "DRIVE:%s", driveTypeLabel(packetCopy.driveType));
  } else {
    snprintf(driveText, sizeof(driveText), "DRIVE:--");
  }
  display.drawText(0, 42, driveText, 1);

  uint16_t batteryMv = readReceiverBatteryMillivolts();
  int batteryPercent = receiverBatteryPercentFromMillivolts(batteryMv);
  char batteryText[20];
  if (batteryPercent >= 0) {
    snprintf(batteryText, sizeof(batteryText), "BAT:%d%%", batteryPercent);
  } else {
    snprintf(batteryText, sizeof(batteryText), "BAT:--%%");
  }
  display.drawText(0, 56, batteryText, 1);
  display.display();
}

float packetValueToNormalized(int16_t value) {
  return constrain((float)value / 1000.0f, -1.0f, 1.0f);
}

uint32_t pulseUsToDuty(uint16_t pulseUs) {
  const uint32_t maxDuty = (1UL << RC_PWM_RESOLUTION_BITS) - 1UL;
  const uint32_t periodUs = 1000000UL / RC_PWM_FREQUENCY_HZ;
  return (uint32_t)(((uint64_t)pulseUs * maxDuty) / periodUs);
}

uint16_t normalizedToPulseUs(float value) {
  float constrainedValue = constrain(value, -1.0f, 1.0f);
  float halfSpan = (RC_PWM_MAX_US - RC_PWM_MIN_US) * 0.5f;
  float pulse = RC_PWM_NEUTRAL_US + (constrainedValue * halfSpan);
  return (uint16_t)constrain((int)roundf(pulse), RC_PWM_MIN_US, RC_PWM_MAX_US);
}

uint16_t normalizedToPulseUsForOutput(float value, int outputIndex) {
  if (outputIndex < 0 || outputIndex >= RC_OUTPUT_COUNT) {
    return normalizedToPulseUs(value);
  }

  if (!OUTPUT_UNIDIRECTIONAL_ESC[outputIndex]) {
    return normalizedToPulseUs(value);
  }

  float signedValue = OUTPUT_ESC_POSITIVE_IS_FORWARD[outputIndex] ? value : -value;
  float throttle01 = 0.0f;
  if (OUTPUT_ESC_USE_ACTIVE_HALF_ONLY[outputIndex]) {
    // Treat center as idle and only use the selected active half-range as throttle.
    throttle01 = constrain(signedValue, 0.0f, 1.0f);
  } else {
    throttle01 = constrain((signedValue + 1.0f) * 0.5f, 0.0f, 1.0f);
  }
  float pulse = RC_PWM_MIN_US + throttle01 * (RC_PWM_MAX_US - RC_PWM_MIN_US);
  return (uint16_t)constrain((int)roundf(pulse), RC_PWM_MIN_US, RC_PWM_MAX_US);
}

void writeRcPulseUs(int channelIndex, uint16_t pulseUs) {
  if (channelIndex < 0 || channelIndex >= RC_OUTPUT_COUNT) return;
  if (!outputPwmAttached[channelIndex]) return;
  lastOutputPulseUs[channelIndex] = pulseUs;
  uint32_t duty = pulseUsToDuty(pulseUs);
  bool writeOk = false;
  if (outputUseChannelWrite[channelIndex]) {
    writeOk = ledcWriteChannel(OUTPUT_LEDC_CHANNEL[channelIndex], duty);
  } else {
    writeOk = ledcWrite(OUTPUT_PINS[channelIndex], duty);
  }
  if (!writeOk) {
    pwmWriteFailureCount++;
  }
}

void initRcPwmOutputs() {
  for (int i = 0; i < RC_OUTPUT_COUNT; i++) {
    outputPwmAttached[i] = ledcAttachChannel(
      OUTPUT_PINS[i],
      RC_PWM_FREQUENCY_HZ,
      RC_PWM_RESOLUTION_BITS,
      OUTPUT_LEDC_CHANNEL[i]
    );

    if (outputPwmAttached[i]) {
      outputUseChannelWrite[i] = true;
      Serial.printf("PWM attached on GPIO %u (LEDC ch%u, write=channel) @ %lu Hz\n",
                    OUTPUT_PINS[i], OUTPUT_LEDC_CHANNEL[i],
                    (unsigned long)ledcReadFreq(OUTPUT_PINS[i]));
    } else {
      // Fallback for boards/cores where explicit channel attach fails but pin attach works.
      outputPwmAttached[i] = ledcAttach(OUTPUT_PINS[i], RC_PWM_FREQUENCY_HZ, RC_PWM_RESOLUTION_BITS);
      outputUseChannelWrite[i] = false;
      if (outputPwmAttached[i]) {
        Serial.printf("PWM attached on GPIO %u (auto channel, write=pin) @ %lu Hz\n",
                      OUTPUT_PINS[i], (unsigned long)ledcReadFreq(OUTPUT_PINS[i]));
      } else {
        Serial.printf("Failed to attach PWM on GPIO %u (LEDC ch%u + auto fallback)\n",
                      OUTPUT_PINS[i], OUTPUT_LEDC_CHANNEL[i]);
      }
    }
  }
}

void applyFailsafeOutputs() {
  for (int i = 0; i < RC_OUTPUT_COUNT; i++) {
    bool idleAtMin = OUTPUT_IDLE_AT_MIN_THROTTLE[i] || ESC_IDLE_AT_MIN_THROTTLE;
    uint16_t failsafePulse = idleAtMin ? RC_PWM_MIN_US : RC_PWM_NEUTRAL_US;
    writeRcPulseUs(i, failsafePulse);
  }
}

void applyControlPacket(const EspNowControlPacket &packet) {
  float outputs[CHANNEL_COUNT];

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    outputs[i] = packetValueToNormalized(packet.channels[i]);
  }

  for (int i = 0; i < RC_OUTPUT_COUNT; i++) {
    int sourceChannel = constrain((int)OUTPUT_SOURCE_CHANNEL[i], 0, CHANNEL_COUNT - 1);
    writeRcPulseUs(i, normalizedToPulseUsForOutput(outputs[sourceChannel], i));
  }
}

uint16_t readReceiverBatteryMillivoltsWithCtrl(int ctrlLevel) {
  if (RECEIVER_BATTERY_ADC_PIN < 0) return 0;

  if (RECEIVER_BATTERY_ADC_CTRL_PIN >= 0) {
    digitalWrite(RECEIVER_BATTERY_ADC_CTRL_PIN, ctrlLevel);
    delayMicroseconds(500);
  }

  uint32_t totalMv = 0;
  const int sampleCount = 8;
  for (int i = 0; i < sampleCount; i++) {
    totalMv += analogReadMilliVolts(RECEIVER_BATTERY_ADC_PIN);
    delayMicroseconds(100);
  }

  float adcVoltage = ((float)totalMv / (float)sampleCount) / 1000.0f;
  float batteryVoltage = adcVoltage * RECEIVER_BATTERY_DIVIDER_RATIO;
  return (uint16_t)constrain((int)roundf(batteryVoltage * 1000.0f), 0, 65535);
}

uint16_t readReceiverBatteryMillivolts() {
  if (RECEIVER_BATTERY_ADC_PIN < 0) return 0;
  if (RECEIVER_BATTERY_ADC_CTRL_PIN < 0) {
    return readReceiverBatteryMillivoltsWithCtrl(LOW);
  }

  static int detectedCtrlLevel = -1;
  static bool detectionLogged = false;
  if (detectedCtrlLevel >= 0) {
    uint16_t mv = readReceiverBatteryMillivoltsWithCtrl(detectedCtrlLevel);
    if (mv < RECEIVER_BATTERY_PRESENT_MIN_MV || mv > RECEIVER_BATTERY_PRESENT_MAX_MV) {
      return 0;
    }
    return mv;
  }

  uint16_t lowMv = readReceiverBatteryMillivoltsWithCtrl(LOW);
  uint16_t highMv = readReceiverBatteryMillivoltsWithCtrl(HIGH);
  bool lowLooksValid =
    lowMv >= RECEIVER_BATTERY_PRESENT_MIN_MV && lowMv <= RECEIVER_BATTERY_PRESENT_MAX_MV;
  bool highLooksValid =
    highMv >= RECEIVER_BATTERY_PRESENT_MIN_MV && highMv <= RECEIVER_BATTERY_PRESENT_MAX_MV;

  if (lowLooksValid && !highLooksValid) {
    detectedCtrlLevel = LOW;
  } else if (highLooksValid && !lowLooksValid) {
    detectedCtrlLevel = HIGH;
  } else {
    detectedCtrlLevel = (highMv >= lowMv) ? HIGH : LOW;
  }

  uint16_t selectedMv = (detectedCtrlLevel == HIGH) ? highMv : lowMv;
  if (!detectionLogged) {
    Serial.print("Receiver battery sense: GPIO");
    Serial.print(RECEIVER_BATTERY_ADC_CTRL_PIN);
    Serial.print(" LOW=");
    Serial.print(lowMv);
    Serial.print("mV HIGH=");
    Serial.print(highMv);
    Serial.print("mV using ");
    Serial.println(detectedCtrlLevel == HIGH ? "HIGH" : "LOW");
    detectionLogged = true;
  }

  if (selectedMv < RECEIVER_BATTERY_PRESENT_MIN_MV ||
      selectedMv > RECEIVER_BATTERY_PRESENT_MAX_MV) {
    return 0;
  }
  return selectedMv;
}

int receiverBatteryPercentFromMillivolts(uint16_t millivolts) {
  if (millivolts < 1000) return -1;

  float voltage = (float)millivolts / 1000.0f;
  float percent =
    ((voltage - RECEIVER_BATTERY_EMPTY_V) /
     (RECEIVER_BATTERY_FULL_V - RECEIVER_BATTERY_EMPTY_V)) * 100.0f;
  return constrain((int)roundf(percent), 0, 100);
}

void sendTelemetryPacket(unsigned long now) {
  if (!receiverBound || !senderPeerReady) return;

  bool sendKeepalive = false;
  if (telemetryPending) {
    if (now - lastTelemetrySendTime < TELEMETRY_INTERVAL_MS) return;
  } else {
    if (now - lastTelemetrySendTime < TELEMETRY_KEEPALIVE_INTERVAL_MS) return;
    sendKeepalive = true;
  }

  EspNowControlPacket packetCopy = {};
  if (!sendKeepalive) {
    portENTER_CRITICAL(&controlPacketMux);
    packetCopy = latestControlPacket;
    telemetryPending = false;
    portEXIT_CRITICAL(&controlPacketMux);
  }

  EspNowTelemetryPacket telemetry = {};
  telemetry.magic = ESPNOW_LINK_MAGIC;
  telemetry.version = ESPNOW_LINK_VERSION;
  telemetry.messageType = ESPNOW_MSG_TELEMETRY;
  telemetry.sequence = packetCopy.sequence;
  telemetry.echoedTxMillis = packetCopy.txMillis;
  telemetry.receiverBatteryMillivolts = readReceiverBatteryMillivolts();
  telemetry.receiverPacketAgeMs = (uint16_t)constrain((int)(now - lastPacketMillis), 0, 65535);

  esp_err_t result = esp_now_send(lastSenderMac, (const uint8_t *)&telemetry, sizeof(telemetry));
  if (result != ESP_OK) {
    Serial.printf("Telemetry send failed: %d\n", (int)result);
    if (!sendKeepalive) telemetryPending = true;
    senderPeerReady = false;
  } else {
    lastTelemetrySendTime = now;
  }
}

void sendBindBeacon(unsigned long now) {
  if (receiverBound) return;
  if (now - lastBindBeaconTime < BIND_BEACON_INTERVAL_MS) return;
  if (!ensureBroadcastPeer()) return;

  EspNowBindPacket packet = {};
  packet.magic = ESPNOW_LINK_MAGIC;
  packet.version = ESPNOW_LINK_VERSION;
  packet.messageType = ESPNOW_MSG_BIND_BEACON;

  esp_err_t result = esp_now_send(ESPNOW_BROADCAST_ADDRESS, (const uint8_t *)&packet, sizeof(packet));
  if (result == ESP_OK) {
    lastBindBeaconTime = now;
  }
}

void sendBindAck(const uint8_t *macAddress, uint32_t token) {
  if (macAddress == nullptr) return;
  if (!ensureSenderPeer(macAddress)) return;

  EspNowBindPacket packet = {};
  packet.magic = ESPNOW_LINK_MAGIC;
  packet.version = ESPNOW_LINK_VERSION;
  packet.messageType = ESPNOW_MSG_BIND_ACK;
  packet.token = token;

  esp_now_send(macAddress, (const uint8_t *)&packet, sizeof(packet));
}

void sendPingAck(const uint8_t *macAddress, uint32_t sequence, uint32_t echoedTxMillis) {
  if (macAddress == nullptr) return;
  if (!ensureSenderPeer(macAddress)) return;

  EspNowPingAckPacket packet = {};
  packet.magic = ESPNOW_LINK_MAGIC;
  packet.version = ESPNOW_LINK_VERSION;
  packet.messageType = ESPNOW_MSG_PING_ACK;
  packet.sequence = sequence;
  packet.echoedTxMillis = echoedTxMillis;

  esp_now_send(macAddress, (const uint8_t *)&packet, sizeof(packet));
}

void updateRebindButton(unsigned long now) {
  bool pressed = (digitalRead(REBIND_BUTTON_PIN) == LOW);

  if (!pressed) {
    rebindButtonPressStart = 0;
    rebindTriggered = false;
    return;
  }

  if (rebindButtonPressStart == 0) {
    rebindButtonPressStart = now;
    return;
  }

  if (!rebindTriggered && (now - rebindButtonPressStart >= REBIND_HOLD_MS)) {
    clearReceiverBinding();
    rebindTriggered = true;
  }
}

void onEspNowReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (info == nullptr || data == nullptr || len <= 0) return;

  if (len == (int)sizeof(EspNowPingPacket)) {
    EspNowPingPacket pingPacket = {};
    memcpy(&pingPacket, data, sizeof(pingPacket));

    if (pingPacket.magic == ESPNOW_LINK_MAGIC &&
        pingPacket.version == ESPNOW_LINK_VERSION &&
        pingPacket.messageType == ESPNOW_MSG_PING &&
        receiverBound &&
        memcmp(info->src_addr, boundTransmitterMac, ESP_NOW_ETH_ALEN) == 0) {
      memcpy(lastSenderMac, info->src_addr, ESP_NOW_ETH_ALEN);
      sendPingAck(info->src_addr, pingPacket.sequence, pingPacket.txMillis);
    }
    return;
  }

  if (len == (int)sizeof(EspNowBindPacket)) {
    EspNowBindPacket bindPacket = {};
    memcpy(&bindPacket, data, sizeof(bindPacket));

    if (bindPacket.magic == ESPNOW_LINK_MAGIC &&
        bindPacket.version == ESPNOW_LINK_VERSION &&
        bindPacket.messageType == ESPNOW_MSG_BIND_COMMIT) {

      if (!receiverBound) {
        memcpy(boundTransmitterMac, info->src_addr, ESP_NOW_ETH_ALEN);
        saveReceiverBinding();
      } else if (memcmp(info->src_addr, boundTransmitterMac, ESP_NOW_ETH_ALEN) != 0) {
        return;
      }

      ensureSenderPeer(info->src_addr);
      memcpy(lastSenderMac, info->src_addr, ESP_NOW_ETH_ALEN);
      sendBindAck(info->src_addr, bindPacket.token);
    }
    return;
  }

  EspNowControlPacket packet = {};
  uint8_t packetChannelCount = 0;
  if (!parseEspNowControlPacket(data, len, packet, packetChannelCount)) return;

  if (packet.magic != ESPNOW_LINK_MAGIC ||
      packet.version != ESPNOW_LINK_VERSION ||
      packet.messageType != ESPNOW_MSG_CONTROL) {
    return;
  }

  if (!receiverBound) return;
  if (memcmp(info->src_addr, boundTransmitterMac, ESP_NOW_ETH_ALEN) != 0) return;

  memcpy(lastSenderMac, info->src_addr, ESP_NOW_ETH_ALEN);
  ensureSenderPeer(lastSenderMac);

  portENTER_CRITICAL(&controlPacketMux);
  latestControlPacket = packet;
  latestControlChannelCount = packetChannelCount;
  hasControlPacket = true;
  telemetryPending = true;
  portEXIT_CRITICAL(&controlPacketMux);

  lastPacketMillis = millis();
}

void setup() {
  Serial.begin(115200);
  initHeltecDisplay();

  pinMode(REBIND_BUTTON_PIN, INPUT_PULLUP);

  if (RECEIVER_BATTERY_ADC_PIN >= 0) {
    if (RECEIVER_BATTERY_ADC_CTRL_PIN >= 0) {
      pinMode(RECEIVER_BATTERY_ADC_CTRL_PIN, OUTPUT);
      digitalWrite(RECEIVER_BATTERY_ADC_CTRL_PIN, LOW);
    }
    pinMode(RECEIVER_BATTERY_ADC_PIN, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(RECEIVER_BATTERY_ADC_PIN, ADC_11db);
  }

  initRcPwmOutputs();

  applyFailsafeOutputs();
  escArmHoldStart = millis();
  escArmHoldActive = true;

  loadReceiverBinding();

  if (!initEspNowReceiver()) {
    Serial.println("Receiver ESP-NOW init failed; restarting in 5 seconds");
    delay(5000);
    ESP.restart();
  }

  ensureBroadcastPeer();
  if (receiverBound) {
    ensureSenderPeer(boundTransmitterMac);
    memcpy(lastSenderMac, boundTransmitterMac, ESP_NOW_ETH_ALEN);
  }

  Serial.print("Receiver ready on Wi-Fi channel ");
  Serial.println(ESPNOW_WIFI_CHANNEL);
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Receiver bind state: ");
  Serial.println(receiverBound ? "BOUND" : "UNBOUND");
}

void loop() {
  unsigned long now = millis();

  updateRebindButton(now);

  bool packetFresh = hasControlPacket && (now - lastPacketMillis <= FAILSAFE_TIMEOUT_MS);

  if (escArmHoldActive && (now - escArmHoldStart >= ESC_ARM_HOLD_MS)) {
    escArmHoldActive = false;
  }

  if (escArmHoldActive) {
    applyFailsafeOutputs();
  } else if (packetFresh) {
    EspNowControlPacket packetCopy = {};
    portENTER_CRITICAL(&controlPacketMux);
    packetCopy = latestControlPacket;
    portEXIT_CRITICAL(&controlPacketMux);
    applyControlPacket(packetCopy);
  } else {
    applyFailsafeOutputs();
  }

  sendBindBeacon(now);
  sendTelemetryPacket(now);
  updateReceiverDisplay(now, packetFresh);

  static unsigned long lastDebugTime = 0;
  if (now - lastDebugTime >= 1000) {
    lastDebugTime = now;
    Serial.print("Link ");
    Serial.print(packetFresh ? "OK" : "FAILSAFE");
    Serial.print(" | age ");
    Serial.print(packetFresh ? (now - lastPacketMillis) : FAILSAFE_TIMEOUT_MS);
    Serial.print(" ms | bind ");
    Serial.print(receiverBound ? "BOUND" : "WAITING");
    Serial.print(" | battery ");
    uint16_t batteryMv = readReceiverBatteryMillivolts();
    if (batteryMv > 0) {
      Serial.print(batteryMv / 1000.0f, 2);
      Serial.println(" V");
    } else {
      Serial.println("n/a");
    }

    Serial.print("PWM us [");
    for (int i = 0; i < RC_OUTPUT_COUNT; i++) {
      if (i > 0) Serial.print(", ");
      Serial.print("GPIO");
      Serial.print(OUTPUT_PINS[i]);
      Serial.print(":");
      Serial.print(lastOutputPulseUs[i]);
    }
    Serial.println("]");

    if (hasControlPacket) {
      EspNowControlPacket debugPacket = {};
      uint8_t debugChannelCount = 0;
      portENTER_CRITICAL(&controlPacketMux);
      debugPacket = latestControlPacket;
      debugChannelCount = latestControlChannelCount;
      portEXIT_CRITICAL(&controlPacketMux);

      Serial.print("RX ch raw [");
      for (int i = 0; i < debugChannelCount; i++) {
        if (i > 0) Serial.print(", ");
        Serial.print(debugPacket.channels[i]);
      }
      Serial.print("] count=");
      Serial.print(debugChannelCount);
      Serial.print(" drive=");
      Serial.print(driveTypeLabel(debugPacket.driveType));
      Serial.print(" tankMode=");
      Serial.print(debugPacket.tankMode);
      Serial.print(" model=");
      char debugModelName[24];
      copyPacketModelName(debugPacket, debugModelName, sizeof(debugModelName));
      Serial.print(debugModelName);
      Serial.print(" latency=");
      Serial.println(debugPacket.linkLatencyMs);
    }

    Serial.print("PWM attach [");
    for (int i = 0; i < RC_OUTPUT_COUNT; i++) {
      if (i > 0) Serial.print(", ");
      Serial.print("GPIO");
      Serial.print(OUTPUT_PINS[i]);
      Serial.print(":");
      Serial.print(outputPwmAttached[i] ? "OK" : "FAIL");
      Serial.print("/");
      Serial.print(outputUseChannelWrite[i] ? "ch" : "pin");
    }
    Serial.println("]");

    if (pwmWriteFailureCount > 0) {
      Serial.print("PWM write failures: ");
      Serial.println(pwmWriteFailureCount);
    }
  }

  delay(5);
}
