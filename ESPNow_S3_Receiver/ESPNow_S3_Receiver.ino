#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>

#define CHANNEL_COUNT 4
#define RC_OUTPUT_COUNT 4
#define ESPNOW_LINK_MAGIC 0xA614
#define ESPNOW_LINK_VERSION 1
#define ESPNOW_WIFI_CHANNEL 6
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
};

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

const uint8_t OUTPUT_PINS[RC_OUTPUT_COUNT] = {8, 9, 10, 11};
const uint8_t OUTPUT_LEDC_CHANNEL[RC_OUTPUT_COUNT] = {0, 1, 2, 3};
// Output-to-channel map (CH1=RX, CH2=RY, CH3=LY, CH4=LX).
// GPIO10 (index 2) is mapped to CH3 / left stick Y for brushless ESC throttle.
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
const int RECEIVER_BATTERY_ADC_PIN = -1;
const float RECEIVER_BATTERY_DIVIDER_RATIO = 2.0f;
const uint8_t ESPNOW_BROADCAST_ADDRESS[ESP_NOW_ETH_ALEN] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

Preferences receiverPrefs;
portMUX_TYPE controlPacketMux = portMUX_INITIALIZER_UNLOCKED;
EspNowControlPacket latestControlPacket = {};
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

bool initEspNowReceiver();
bool setEspNowWifiChannel(uint8_t channel);
bool ensureSenderPeer(const uint8_t *macAddress);
bool ensureBroadcastPeer();
void loadReceiverBinding();
void saveReceiverBinding();
void clearReceiverBinding();
void applyControlPacket(const EspNowControlPacket &packet);
void applyFailsafeOutputs();
void writeRcPulseUs(int channelIndex, uint16_t pulseUs);
uint16_t normalizedToPulseUs(float value);
uint16_t normalizedToPulseUsForOutput(float value, int outputIndex);
uint32_t pulseUsToDuty(uint16_t pulseUs);
float packetValueToNormalized(int16_t value);
uint16_t readReceiverBatteryMillivolts();
void sendTelemetryPacket(unsigned long now);
void sendBindBeacon(unsigned long now);
void sendBindAck(const uint8_t *macAddress, uint32_t token);
void sendPingAck(const uint8_t *macAddress, uint32_t sequence, uint32_t echoedTxMillis);
void updateRebindButton(unsigned long now);
void onEspNowReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void initRcPwmOutputs();

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
  telemetryPending = false;
  lastPacketMillis = 0;
  lastTelemetrySendTime = 0;
  lastBindBeaconTime = 0;
  applyFailsafeOutputs();
  Serial.println("Binding cleared; receiver is back in beacon mode");
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
  float rawChannels[CHANNEL_COUNT];
  float outputs[CHANNEL_COUNT];

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    rawChannels[i] = packetValueToNormalized(packet.channels[i]);
    outputs[i] = rawChannels[i];
  }

  if (packet.driveType == DRIVE_TANK) {
    if (packet.tankMode == TANK_MODE_DUAL_STICK) {
      outputs[0] = rawChannels[2];
      outputs[1] = rawChannels[1];
    } else {
      float throttle = rawChannels[1];
      float turn = rawChannels[0];
      outputs[0] = constrain(throttle + turn, -1.0f, 1.0f);
      outputs[1] = constrain(throttle - turn, -1.0f, 1.0f);
    }
  } else if (packet.driveType == DRIVE_CAR) {
    outputs[0] = (packet.tankMode == TANK_MODE_DUAL_STICK)
      ? rawChannels[3]
      : rawChannels[0];
    outputs[1] = rawChannels[1];
  }

  for (int i = 0; i < RC_OUTPUT_COUNT; i++) {
    int sourceChannel = constrain((int)OUTPUT_SOURCE_CHANNEL[i], 0, CHANNEL_COUNT - 1);
    writeRcPulseUs(i, normalizedToPulseUsForOutput(outputs[sourceChannel], i));
  }
}

uint16_t readReceiverBatteryMillivolts() {
  if (RECEIVER_BATTERY_ADC_PIN < 0) return 0;

  uint32_t totalMv = 0;
  const int sampleCount = 8;
  for (int i = 0; i < sampleCount; i++) {
    totalMv += analogReadMilliVolts(RECEIVER_BATTERY_ADC_PIN);
  }

  float adcVoltage = ((float)totalMv / (float)sampleCount) / 1000.0f;
  float batteryVoltage = adcVoltage * RECEIVER_BATTERY_DIVIDER_RATIO;
  return (uint16_t)constrain((int)roundf(batteryVoltage * 1000.0f), 0, 65535);
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

  if (len != (int)sizeof(EspNowControlPacket)) return;

  EspNowControlPacket packet = {};
  memcpy(&packet, data, sizeof(packet));

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
  hasControlPacket = true;
  telemetryPending = true;
  portEXIT_CRITICAL(&controlPacketMux);

  lastPacketMillis = millis();
}

void setup() {
  Serial.begin(115200);

  pinMode(REBIND_BUTTON_PIN, INPUT_PULLUP);

  if (RECEIVER_BATTERY_ADC_PIN >= 0) {
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
      portENTER_CRITICAL(&controlPacketMux);
      debugPacket = latestControlPacket;
      portEXIT_CRITICAL(&controlPacketMux);

      Serial.print("RX ch raw [");
      for (int i = 0; i < CHANNEL_COUNT; i++) {
        if (i > 0) Serial.print(", ");
        Serial.print(debugPacket.channels[i]);
      }
      Serial.print("] drive=");
      Serial.print(debugPacket.driveType);
      Serial.print(" tankMode=");
      Serial.println(debugPacket.tankMode);
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
