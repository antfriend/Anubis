#include <Arduino.h>
#line 1 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
#include <RAK14014_FT6336U.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <ctype.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>

#include "anubis_rgb565.h"
#include "menu_icons_rgb565.h"

  #define EEPROM_SIZE 512
  #define MIX_STORAGE_VERSION 2
  #define ESPNOW_BIND_STORAGE_VERSION 1
  #define FAILSAFE_STORAGE_VERSION 1
  #define RATE_STORAGE_VERSION 1
  #define EXPO_STORAGE_VERSION 1
  #define ENDPOINT_STORAGE_VERSION 1
  #define EEPROM_MIX_VERSION_ADDR (EEPROM_SIZE - 1)
  #define EEPROM_BIND_VERSION_ADDR (EEPROM_SIZE - 2)
  #define EEPROM_FAILSAFE_VERSION_ADDR (EEPROM_SIZE - 3)
  #define EEPROM_RATE_VERSION_ADDR (EEPROM_SIZE - 4)
  #define EEPROM_EXPO_VERSION_ADDR (EEPROM_SIZE - 5)
  #define EEPROM_ENDPOINT_VERSION_ADDR (EEPROM_SIZE - 6)
  #define MIX_CHANNEL_MASK 0x0F
  #define MIX_REVERSE_SEPARATE_FLAG 0x80

  TFT_eSPI tft = TFT_eSPI();
  TFT_eSprite stickBaseSprite = TFT_eSprite(&tft);
  FT6336U touchPanel;

  enum Screen {
  SCREEN_SPLASH,
  SCREEN_MAIN,
  SCREEN_MENU,
  SCREEN_CONTROLLER_SETTINGS,
  SCREEN_EXPO,
  SCREEN_RATES,
  SCREEN_PROTOCOL,
  SCREEN_OTA_SETTINGS,
  SCREEN_ELRS,
  SCREEN_MODEL_SETTINGS,
  SCREEN_REVERSE,
  SCREEN_TRIM,
  SCREEN_ENDPOINTS,
  SCREEN_FAILSAFE,
  SCREEN_MODEL_NAME,
  SCREEN_DRIVE_TYPE,
  SCREEN_TANK_MODE,
  SCREEN_MIXING,
  SCREEN_DISPLAY_SETTINGS,
  SCREEN_SPACE_GAME
  };

  enum ButtonID {
  BTN_NONE,
  BTN_MENU,
  BTN_BACK,
  BTN_CTRL,
  BTN_MODEL,
  BTN_REVERSE,
  BTN_TRIM,
  BTN_ENDPOINTS,
  BTN_FAILSAFE,
  BTN_EXPO,
  BTN_RATES,
  BTN_PROTOCOL,
  BTN_PAGE_NAV,
  BTN_PROTOCOL_ELRS,
  BTN_PROTOCOL_ESPNOW,
  BTN_PROTOCOL_BIND,
  BTN_PROTOCOL_OTA,
  BTN_PROTOCOL_OTA_CFG,
  BTN_ELRS_BIND,
  BTN_DRIVE_TYPE,
  BTN_DRIVE_TANK,
  BTN_DRIVE_CAR,
  BTN_DRIVE_OMNI,
  BTN_DRIVE_X_DRONE,
  BTN_TANK_SINGLE,
  BTN_TANK_DUAL,
  BTN_MODEL_NAME,
  BTN_MIXING,
  BTN_DISPLAY_SETTINGS,
  BTN_DISPLAY_BRIGHTNESS_DEC,
  BTN_DISPLAY_BRIGHTNESS_INC,
  BTN_DISPLAY_TIMEOUT_DEC,
  BTN_DISPLAY_TIMEOUT_INC,
  BTN_DISPLAY_OFF_TIMEOUT_DEC,
  BTN_DISPLAY_OFF_TIMEOUT_INC,
  BTN_DISPLAY_SLEEP_DEC,
  BTN_DISPLAY_SLEEP_INC,
  BTN_DISPLAY_SLEEP_NOW,
  BTN_DISPLAY_THEME_TOGGLE,
  BTN_GAME,
  BTN_OPTION_2,
  BTN_OPTION_3,
  BTN_OPTION_4,
  BTN_OPTION_5
  };

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

  enum ProtocolType {
  PROTOCOL_ELRS,
  PROTOCOL_ESPNOW
  };

  #define MAX_MODELS 4
  #define MIX_COUNT 4
  #define CHANNEL_COUNT 4
  #define PROTOCOL_MASK 0x60
  #define PROTOCOL_SHIFT 5
  #define EEPROM_BIND_DATA_ADDR (EEPROM_RATE_VERSION_ADDR - (MAX_MODELS * ESP_NOW_ETH_ALEN))
  #define EEPROM_FAILSAFE_DATA_ADDR (EEPROM_BIND_DATA_ADDR - (MAX_MODELS * CHANNEL_COUNT))
  #define EEPROM_RATE_DATA_ADDR (EEPROM_FAILSAFE_DATA_ADDR - (MAX_MODELS * CHANNEL_COUNT))
  #define EEPROM_EXPO_DATA_ADDR (EEPROM_RATE_DATA_ADDR - (MAX_MODELS * CHANNEL_COUNT))
  #define EEPROM_ENDPOINT_LOW_DATA_ADDR (EEPROM_EXPO_DATA_ADDR - (MAX_MODELS * CHANNEL_COUNT))
  #define EEPROM_ENDPOINT_HIGH_DATA_ADDR (EEPROM_ENDPOINT_LOW_DATA_ADDR - (MAX_MODELS * CHANNEL_COUNT))
  #define OTA_SETTINGS_STORAGE_VERSION 1
  #define OTA_STA_SSID_STORAGE_BYTES 32
  #define OTA_STA_PASSWORD_STORAGE_BYTES 64
  #define OTA_AP_SSID_STORAGE_BYTES 32
  #define EEPROM_OTA_VERSION_ADDR (EEPROM_ENDPOINT_HIGH_DATA_ADDR - 1)
  #define EEPROM_OTA_STA_SSID_ADDR (EEPROM_OTA_VERSION_ADDR - OTA_STA_SSID_STORAGE_BYTES)
  #define EEPROM_OTA_STA_PASSWORD_ADDR (EEPROM_OTA_STA_SSID_ADDR - OTA_STA_PASSWORD_STORAGE_BYTES)
  #define EEPROM_OTA_AP_SSID_ADDR (EEPROM_OTA_STA_PASSWORD_ADDR - OTA_AP_SSID_STORAGE_BYTES)
  #define DISPLAY_SETTINGS_STORAGE_VERSION 2
  #define EEPROM_DISPLAY_VERSION_ADDR (EEPROM_OTA_AP_SSID_ADDR - 1)
  #define EEPROM_DISPLAY_BRIGHTNESS_ADDR (EEPROM_DISPLAY_VERSION_ADDR - 1)
  #define EEPROM_DISPLAY_SLEEP_BRIGHTNESS_ADDR (EEPROM_DISPLAY_BRIGHTNESS_ADDR - 1)
  #define EEPROM_DISPLAY_TIMEOUT_INDEX_ADDR (EEPROM_DISPLAY_SLEEP_BRIGHTNESS_ADDR - 1)
  #define EEPROM_DISPLAY_OFF_TIMEOUT_INDEX_ADDR (EEPROM_DISPLAY_TIMEOUT_INDEX_ADDR - 1)
  #define EEPROM_DISPLAY_THEME_ADDR (EEPROM_DISPLAY_OFF_TIMEOUT_INDEX_ADDR - 1)
  #define ESPNOW_LINK_MAGIC 0xA614
  #define ESPNOW_LINK_VERSION 1
  #define ESPNOW_WIFI_CHANNEL 6
  #define ESPNOW_SEND_INTERVAL_MS 20
  #define ESPNOW_PING_INTERVAL_MS 100
  #define ESPNOW_TELEMETRY_INTERVAL_MS 100
  #define ESPNOW_TELEMETRY_TIMEOUT_MS 1000
  #define ESPNOW_HEADER_SIGNAL_TIMEOUT_MS 2000
  #define ESPNOW_BIND_TIMEOUT_MS 15000
  #define ESPNOW_BIND_COMMIT_RETRY_MS 300
  // External ELRS modules normally use a single S.Port/Data/Signal line.
  // For the Ranger Nano direct test, use GPIO43 on a non-inverted half-duplex bus.
  #define ELRS_HALF_DUPLEX_MODE true
  #define ELRS_UART_DATA_PIN 43
#if ELRS_HALF_DUPLEX_MODE
  #define ELRS_UART_TX_PIN_DEFAULT ELRS_UART_DATA_PIN
  #define ELRS_UART_RX_PIN_DEFAULT ELRS_UART_DATA_PIN
#else
  #define ELRS_UART_TX_PIN_DEFAULT 44
  #define ELRS_UART_RX_PIN_DEFAULT 43
#endif
  #define ELRS_CRSF_TX_INTERVAL_MS 10
  #define ELRS_DEVICE_PING_INTERVAL_MS 500
  #define ELRS_MODULE_TIMEOUT_MS 2000
  #define ELRS_LINK_TIMEOUT_MS 1200
  #define ELRS_BAUD_RETRY_MS 1500
  #define ELRS_TX_ONLY_ACTIVE_WINDOW_MS 500
  #define ELRS_PARAM_READ_INTERVAL_MS 120
  #define ELRS_PARAM_SCAN_FALLBACK_MAX 64
  #define ELRS_TX_SYNC_PRIMARY 0xC8
  // -1 = auto-try both pin pairs, 0/1 = force a single pin-pair mapping.
  #define ELRS_FORCE_PIN_PAIR_INDEX 0
  // Most external-module hosts use an inverted S.Port/Data line.
  // -1 = keep existing, otherwise 0..2 maps to elrsInvertModes entry.
  #define ELRS_FORCE_INVERT_MODE 2
  #define RADIO_PROTOCOL_SERIAL_DEBUG false
  #define ELRS_VERBOSE_SERIAL_DEBUG false
  // The inverter is out of circuit now, so go back to normal ELRS traffic.
  #define ELRS_PROBE_UNTIL_MODULE_FRAMES false
  #define ELRS_RX_ONLY_DIAGNOSTIC false
  #define ELRS_RX_READ_BUDGET 256

  enum EspNowMessageType : uint8_t {
  ESPNOW_MSG_CONTROL = 0x43,
  ESPNOW_MSG_TELEMETRY = 0x54,
  ESPNOW_MSG_PING = 0x50,
  ESPNOW_MSG_PING_ACK = 0x51,
  ESPNOW_MSG_BIND_BEACON = 0x42,
  ESPNOW_MSG_BIND_COMMIT = 0x62,
  ESPNOW_MSG_BIND_ACK = 0x41
  };

enum NumpadTarget {
  NUMPAD_TARGET_MIX_RATE,
  NUMPAD_TARGET_MIX_OFFSET,
  NUMPAD_TARGET_RATE_VALUE,
  NUMPAD_TARGET_EXPO_VALUE,
  NUMPAD_TARGET_ENDPOINT_VALUE
  };

  enum KeyboardTarget {
  KEYBOARD_TARGET_MODEL_NAME,
  KEYBOARD_TARGET_OTA_STA_SSID,
  KEYBOARD_TARGET_OTA_STA_PASSWORD,
  KEYBOARD_TARGET_OTA_AP_SSID
  };

  enum EspNowLinkFlags : uint8_t {
  ESPNOW_FLAG_BATTERY_PRESENT = 0x01,
  ESPNOW_FLAG_BATTERY_CHARGING = 0x02
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

  struct __attribute__((packed)) EspNowBindPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t messageType;
  uint32_t token;
  uint8_t flags;
  };

  struct MixData {
  bool enabled;
  uint8_t source;
  uint8_t destination;
  int8_t rate;
  int8_t offset;
  };

  struct ModelData {
  char name[20];

  bool reverse[5];
  bool failsafe[5];

  int trimX[2];
  int trimY[2];
  MixData mixes[MIX_COUNT];
  uint8_t driveType;
  };

  struct BattleEnemy {
  float x;
  float y;
  bool alive;
  uint8_t type;
  unsigned long nextShotAt;
  };

  struct BattleObstacle {
  float x;
  float y;
  float radius;
  bool pyramid;
  };

  ModelData models[MAX_MODELS];
  int activeModel = 0;

  String currentModelName = "No Model";
  WiFiServer otaHttpServer(80);

  volatile int espNowLatency = 0;
  volatile float telemetryVoltage = 0.0;
  float transmitterBatteryVoltage = 0.0;
  bool espNowReady = false;
  bool espNowProtocolActive = false;
  bool espNowBindingMode = false;
  unsigned long espNowProtocolStartTime = 0;
  unsigned long lastEspNowSendTime = 0;
  unsigned long lastEspNowPingTime = 0;
  volatile unsigned long lastEspNowTelemetryTime = 0;
  volatile unsigned long lastEspNowAliveTime = 0;
  bool otaModeActive = false;
  bool otaServiceReady = false;
  bool otaUsingSoftAP = false;
  bool otaApReady = false;
  bool otaStaAutoConnectEnabled = false;
  bool otaStaConnectPending = false;
  bool otaUpdateInProgress = false;
  bool otaRestartPending = false;
  unsigned long otaStaAutoConnectAfterMs = 0;
  unsigned long otaStaConnectStartTime = 0;
  unsigned long espNowBindingStartTime = 0;
  unsigned long espNowBindSuccessTime = 0;
  unsigned long espNowLastBindCommitTime = 0;
  uint32_t espNowSequence = 0;
  uint32_t espNowPingSequence = 0;
  uint32_t espNowBindingToken = 0;
  esp_now_send_status_t lastEspNowSendStatus = ESP_NOW_SEND_FAIL;
  uint8_t boundReceiverMacs[MAX_MODELS][ESP_NOW_ETH_ALEN] = {};
  uint8_t espNowPendingBindMac[ESP_NOW_ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
  const uint8_t espNowBroadcastAddress[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  HardwareSerial &elrsSerial = Serial1;
  bool elrsInitialized = false;
  bool elrsProtocolActive = false;
  volatile bool elrsModulePresent = false;
  volatile bool elrsLinkActive = false;
  volatile uint8_t elrsUplinkLq = 0;
  volatile int8_t elrsUplinkSnr = 0;
  volatile uint8_t elrsUplinkRssi1 = 0;
  volatile uint8_t elrsUplinkRssi2 = 0;
  volatile uint32_t elrsRxByteCount = 0;
  volatile uint32_t elrsRxFrameCount = 0;
  volatile uint32_t elrsDeviceInfoCount = 0;
  volatile uint32_t elrsRxEdgeCount = 0;
  volatile uint32_t elrsType16Count = 0;
  volatile uint32_t elrsType14Count = 0;
  volatile uint32_t elrsType1CCount = 0;
  volatile uint32_t elrsType1DCount = 0;
  volatile uint32_t elrsType29Count = 0;
  volatile uint32_t elrsTypeOtherCount = 0;
  volatile uint32_t elrsSyncC8Count = 0;
  volatile uint32_t elrsSyncEECount = 0;
  volatile uint32_t elrsSyncEACount = 0;
  volatile uint32_t elrsSyncECCount = 0;
  volatile uint32_t elrsPrintableByteCount = 0;
  volatile uint8_t elrsRecentBytes[16] = {};
  volatile uint8_t elrsRecentByteIndex = 0;
  unsigned long lastElrsTxTime = 0;
  unsigned long lastElrsDevicePingTime = 0;
  unsigned long elrsBindCommandSentTime = 0;
  unsigned long elrsBindSuccessTime = 0;
  unsigned long elrsBindAwaitUntil = 0;
  unsigned long elrsBindBurstUntil = 0;
  unsigned long lastElrsBindBurstSendTime = 0;
  bool elrsBindAwaitingResult = false;
  uint8_t elrsBindFieldIndex = 0;
  bool elrsBindFieldKnown = false;
  uint8_t elrsParameterCount = 0;
  uint8_t elrsParameterVersion = 0;
  uint8_t elrsParamScanIndex = 0;
  unsigned long lastElrsParamReadTime = 0;
  unsigned long lastElrsBaudRetryTime = 0;
  volatile unsigned long lastElrsSerialRxTime = 0;
  volatile unsigned long lastElrsLinkStatsTime = 0;
  volatile unsigned long lastElrsDeviceInfoTime = 0;
  char elrsDeviceName[16] = "";
  const bool elrsInvertModes[][2] = {
    {false, false}, // RX normal, TX normal
    {true,  false}, // RX inverted, TX normal
    {true,  true}   // RX inverted, TX inverted
  };
  const int elrsInvertModeCount = (int)(sizeof(elrsInvertModes) / sizeof(elrsInvertModes[0]));
  int elrsInvertModeIndex = 0;
  bool elrsRxInvert = false;
  bool elrsTxInvert = false;
  const uint32_t elrsBaudCandidates[] = {400000, 420000, 115200};
  const int elrsBaudCandidateCount = (int)(sizeof(elrsBaudCandidates) / sizeof(elrsBaudCandidates[0]));
  int elrsBaudIndex = 0;
  uint32_t elrsActiveBaud = 0;
  const uint8_t elrsPinPairs[][2] = {
    {ELRS_UART_TX_PIN_DEFAULT, ELRS_UART_RX_PIN_DEFAULT}, // expected wiring
    {ELRS_UART_RX_PIN_DEFAULT, ELRS_UART_TX_PIN_DEFAULT}  // fallback swapped wiring
  };
  const int elrsPinPairCount = (int)(sizeof(elrsPinPairs) / sizeof(elrsPinPairs[0]));
  int elrsPinPairIndex = 0;
  uint8_t elrsActiveTxPin = ELRS_UART_TX_PIN_DEFAULT;
  uint8_t elrsActiveRxPin = ELRS_UART_RX_PIN_DEFAULT;
  bool elrsRxInterruptAttached = false;

  int focusIndex = 0;
  unsigned long lastDpadTime = 0;
  int controllerSettingsPage = 0;
  int modelSettingsPage = 0;

  DriveType currentDrive = DRIVE_TANK;
  Screen currentScreen = SCREEN_SPLASH;
  ButtonID pressedButton = BTN_NONE;
  ButtonID selectedButton = BTN_NONE;
  bool dpadFocusVisible = false;

  bool uiNeedsRedraw = true;
  bool topBarNeedsRedraw = true;
  bool touchActive = false;
  bool waitingForRelease = false;
  bool screenChangePending = false;
  bool screenAwake = true;
  bool displayDimmed = false;
  bool suppressWakeUntilRelease = false;
  bool fullRedraw = true;
  Screen nextScreen;
  Screen lastScreen = SCREEN_SPLASH;
  Screen reverseReturnScreen = SCREEN_CONTROLLER_SETTINGS;

  unsigned long lastActivityTime = 0;
  unsigned long splashStartTime = 0;
  unsigned long fpsLastTime = 0;
  int frameCount = 0;
  bool splashDone = false;
  int8_t modelFailsafeValues[MAX_MODELS][CHANNEL_COUNT] = {};
  uint8_t modelRateValues[MAX_MODELS][CHANNEL_COUNT] = {};
  uint8_t modelExpoValues[MAX_MODELS][CHANNEL_COUNT] = {};
  uint8_t modelEndpointLowValues[MAX_MODELS][CHANNEL_COUNT] = {};
  uint8_t modelEndpointHighValues[MAX_MODELS][CHANNEL_COUNT] = {};

  #define DISPLAY_BL_PIN 45
  #define DISPLAY_BL_PWM_FREQ 1000
  #define DISPLAY_BL_PWM_BITS 8
  #define BRIGHTNESS_ON 255
  #define BRIGHTNESS_OFF 0
  #define DISPLAY_BACKLIGHT_SERIAL_DEBUG true

  const uint8_t displayBrightnessOptions[] = {48, 80, 112, 144, 176, 208, 232, 255};
  const uint8_t displaySleepBrightnessOptions[] = {0, 12, 24, 40, 56, 72, 96, 128};
  const uint16_t displayTimeoutOptionsSec[] = {10, 15, 30, 60, 120, 300, 0};
  const uint16_t displayOffTimeoutOptionsSec[] = {5, 10, 15, 30, 60, 120, 0};
  const uint8_t displayBrightnessOptionCount = (uint8_t)(sizeof(displayBrightnessOptions) / sizeof(displayBrightnessOptions[0]));
  const uint8_t displaySleepBrightnessOptionCount = (uint8_t)(sizeof(displaySleepBrightnessOptions) / sizeof(displaySleepBrightnessOptions[0]));
  const uint8_t displayTimeoutOptionCount = (uint8_t)(sizeof(displayTimeoutOptionsSec) / sizeof(displayTimeoutOptionsSec[0]));
  const uint8_t displayOffTimeoutOptionCount = (uint8_t)(sizeof(displayOffTimeoutOptionsSec) / sizeof(displayOffTimeoutOptionsSec[0]));
  const uint8_t DEFAULT_DISPLAY_BRIGHTNESS = BRIGHTNESS_ON;
  const uint8_t DEFAULT_DISPLAY_SLEEP_BRIGHTNESS = BRIGHTNESS_OFF;
  const uint8_t DEFAULT_DISPLAY_TIMEOUT_INDEX = 2;
  const uint8_t DEFAULT_DISPLAY_OFF_TIMEOUT_INDEX = 2;
  const uint8_t THEME_DARK = 0;
  const uint8_t THEME_LIGHT = 1;
  uint8_t displayBrightness = DEFAULT_DISPLAY_BRIGHTNESS;
  uint8_t displaySleepBrightness = DEFAULT_DISPLAY_SLEEP_BRIGHTNESS;
  uint8_t displayTimeoutIndex = DEFAULT_DISPLAY_TIMEOUT_INDEX;
  uint8_t displayOffTimeoutIndex = DEFAULT_DISPLAY_OFF_TIMEOUT_INDEX;
  uint8_t themeMode = THEME_DARK;
  uint8_t lastAppliedBacklightBrightness = 255;
  bool panelDisplayEnabled = true;
  bool stickBaseSpriteNeedsRebuild = true;

  #define TARGET_FPS 24
  #define MAIN_FRAME_INTERVAL_MS (1000UL / TARGET_FPS)
  #define TOUCH_POLL_INTERVAL_MS 16
  #define TOPBAR_UPDATE_INTERVAL_MS 250
  #define MODEL_NAME_HOLD_MS 700
  #define BATTERY_ADC_PIN 9
  #define ADS1115_I2C_ADDR 0x48
  #define ADS1115_REG_CONVERSION 0x00
  #define ADS1115_REG_CONFIG 0x01
  #define ADS1115_GAIN_4_096V 0x0200
  #define ADS1115_MODE_SINGLE 0x0100
  #define ADS1115_DATA_RATE_860 0x00E0
  #define ADS1115_COMP_DISABLE 0x0003
  #define ADS1115_JOYSTICK_MAX_COUNT 26400
  #define STICK_SAMPLE_INTERVAL_MS 20
  #define ADS1115_RECONNECT_INTERVAL_MS 1000
  #define ADS1115_MAX_CONSECUTIVE_READ_FAILS 4
  #define STICK_DEADZONE 0.07f
  #define STICK_FILTER_ALPHA 0.35f
  #define BATTERY_DIVIDER_RATIO 2.0f
  #define BATTERY_PRESENT_MIN_V 2.50f
  #define BATTERY_EMPTY_V 3.30f
  #define BATTERY_FULL_V 4.20f
  #define BATTERY_SAMPLE_INTERVAL_MS 500
  #define BATTERY_CHARGE_WINDOW_MS 8000
  #define BATTERY_CHARGING_DELTA_V 0.018f
  #define BATTERY_CHARGING_HOLD_MS 15000

  // ==== BOTTOM NAV BAR ====
  #define FOOTER_H 36
  #define FOOTER_Y (320 - FOOTER_H)
  #define NAV_BTN_HEIGHT 30
  #define NAV_BTN_Y_OFFSET 3

  // ===== UI LAYOUT =====
  #define BTN_HEIGHT 50
  #define BTN_RADIUS 12

  // Menu button
  #define MENU_BTN_X 60
  #define MENU_BTN_Y (FOOTER_Y + NAV_BTN_Y_OFFSET)
  #define MENU_BTN_W 120
  #define MENU_BTN_H NAV_BTN_HEIGHT

  // Back button
  #define BACK_BTN_X 10
  #define BACK_BTN_Y (FOOTER_Y + NAV_BTN_Y_OFFSET)
  #define BACK_BTN_W 100
  #define BACK_BTN_H NAV_BTN_HEIGHT

  // Menu screen buttons
  #define MENU_BTN_HEIGHT 70

  #define CTRL_BTN_X 20
  #define CTRL_BTN_Y 100
  #define CTRL_BTN_W 200
  #define CTRL_BTN_H MENU_BTN_HEIGHT

  #define MODEL_BTN_X 20
  #define MODEL_BTN_Y 170
  #define MODEL_BTN_W 200
  #define MODEL_BTN_H MENU_BTN_HEIGHT

  #define DISPLAY_MENU_BTN_X 20
  #define DISPLAY_MENU_BTN_Y 228
  #define DISPLAY_MENU_BTN_W 48
  #define DISPLAY_MENU_BTN_H 48

  #define GAME_BTN_X 172
  #define GAME_BTN_Y 228
  #define GAME_BTN_W 48
  #define GAME_BTN_H 48

  #define DISPLAY_SETTINGS_ROW_X 12
  #define DISPLAY_SETTINGS_ROW_W 216
  #define DISPLAY_SETTINGS_ROW_H 42
  #define DISPLAY_SETTINGS_BRIGHTNESS_Y 64
  #define DISPLAY_SETTINGS_TIMEOUT_Y 114
  #define DISPLAY_SETTINGS_OFF_TIMEOUT_Y 164
  #define DISPLAY_SETTINGS_SLEEP_Y 214
  #define DISPLAY_SETTINGS_ADJUST_BTN_W 44
  #define DISPLAY_SETTINGS_ADJUST_BTN_H 28
  #define DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET 10
  #define DISPLAY_SETTINGS_MINUS_X 18
  #define DISPLAY_SETTINGS_PLUS_X 178
  #define DISPLAY_SETTINGS_SLEEP_NOW_X 138
  #define DISPLAY_SETTINGS_SLEEP_NOW_Y 258
  #define DISPLAY_SETTINGS_SLEEP_NOW_W 92
  #define DISPLAY_SETTINGS_SLEEP_NOW_H 26
  #define DISPLAY_SETTINGS_THEME_X 10
  #define DISPLAY_SETTINGS_THEME_Y 258
  #define DISPLAY_SETTINGS_THEME_W 120
  #define DISPLAY_SETTINGS_THEME_H 26

  #define SPACE_STATUS_H 28
  #define SPACE_CONTROL_Y 278
  #define SPACE_CONTROL_H 32
  #define SPACE_LEFT_BTN_X 10
  #define SPACE_FIRE_BTN_X 86
  #define SPACE_RIGHT_BTN_X 162
  #define SPACE_CTRL_BTN_W 68
  #define SPACE_CTRL_BTN_H 32
  #define SPACE_EXIT_X 8
  #define SPACE_EXIT_Y 6
  #define SPACE_EXIT_W 56
  #define SPACE_EXIT_H 20
  #define SPACE_FRAME_INTERVAL_MS 33
  #define BZ_VIEW_TOP (SPACE_STATUS_H + 2)
  #define BZ_VIEW_BOTTOM (SPACE_CONTROL_Y - 8)
  #define BZ_HORIZON_Y ((BZ_VIEW_TOP + BZ_VIEW_BOTTOM) / 2)
  #define BZ_FOV_SCALE 110.0f
  #define BZ_PLAYER_SPEED 0.055f
  #define BZ_TURN_SPEED 0.07f
  #define BZ_MAX_ENEMIES 5
  #define BZ_MAX_OBSTACLES 8
  #define BZ_SHOT_SPEED 2.6f
  #define BZ_SHOT_RANGE 80.0f
  #define BZ_FIRE_COOLDOWN_MS 220
  #define BZ_RESPAWN_DELAY_MS 1200
  #define BZ_ENEMY_TANK 0
  #define BZ_ENEMY_SUPER 1
  #define BZ_ENEMY_UFO 2
  #define BZ_MISSILE_SPEED 1.35f
  #define BZ_MISSILE_TURN 0.06f

  #define BACK_TEXT_OFFSET 50

  #define TRIM_CENTER_X 120
  #define TRIM_CENTER_Y 152
  #define TRIM_SIZE     60
  #define TRIM_BTN_SIZE 28
  #define TRIM_BTN_GAP  8
  #define TRIM_VALUE_Y  38
  #define TRIM_RESET_BTN_W 76
  #define TRIM_RESET_BTN_H 30
  #define TRIM_RESET_BTN_X (240 - TRIM_RESET_BTN_W - 10)
  #define TRIM_RESET_BTN_Y (FOOTER_Y - TRIM_RESET_BTN_H - 6)

  #define DRIVE_BTN_W 94
  #define DRIVE_BTN_H 82
  #define DRIVE_BTN_X1 20
  #define DRIVE_BTN_X2 126
  #define DRIVE_BTN_Y1 58
  #define DRIVE_BTN_Y2 148
  #define DRIVE_TOUCH_PAD 8

  #define MIX_TAB_Y        42
  #define MIX_TAB_W        46
  #define MIX_TAB_H        28
  #define MIX_TAB_GAP      8
  #define MIX_TAB_X(i)     (16 + ((i) * (MIX_TAB_W + MIX_TAB_GAP)))

  #define MIX_FIELD_X      120
  #define MIX_FIELD_W      92
  #define MIX_FIELD_H      28
  #define MIX_MINUS_X      120
  #define MIX_PLUS_X       186
  #define MIX_ADJUST_W     26
  #define MIX_VALUE_X      150
  #define MIX_VALUE_W      32
  #define MIX_ROW_ENABLE_Y 84
  #define MIX_ROW_SOURCE_Y 118
  #define MIX_ROW_DEST_Y   146
  #define MIX_ROW_LINK_Y   174
  #define MIX_ROW_RATE_Y   210
  #define MIX_ROW_OFFS_Y   242

  #define MIX_NUMPAD_X     10
  #define MIX_NUMPAD_Y     118
  #define MIX_NUMPAD_W     220
  #define MIX_NUMPAD_H     137
  #define MIX_NUMPAD_BOX_X 22
  #define MIX_NUMPAD_BOX_Y 126
  #define MIX_NUMPAD_BOX_W 132
  #define MIX_NUMPAD_BOX_H 24
  #define MIX_NUMPAD_OK_X  162
  #define MIX_NUMPAD_OK_Y  126
  #define MIX_NUMPAD_OK_W  56
  #define MIX_NUMPAD_OK_H  24
  #define MIX_NUMPAD_KEY_W 64
  #define MIX_NUMPAD_KEY_H 22
  #define MIX_NUMPAD_KEY_GAP 6
  #define MIX_NUMPAD_GRID_X 17
  #define MIX_NUMPAD_GRID_Y 157
  #define PROTOCOL_BIND_BTN_W 72
  #define PROTOCOL_BIND_BTN_H 28
  #define PROTOCOL_BIND_BTN_X (240 - PROTOCOL_BIND_BTN_W - 10)
  #define PROTOCOL_BIND_BTN_Y (FOOTER_Y - PROTOCOL_BIND_BTN_H - 10)
  #define PROTOCOL_MODE_BTN_X 20
  #define PROTOCOL_MODE_BTN_W 200
  #define PROTOCOL_MODE_BTN_H BTN_HEIGHT
  #define PROTOCOL_ELRS_BTN_Y 56
  #define PROTOCOL_ESPNOW_BTN_Y 122
  #define PROTOCOL_OTA_BTN_W 88
  #define PROTOCOL_OTA_BTN_H 28
  #define PROTOCOL_OTA_BTN_X 10
  #define PROTOCOL_OTA_BTN_Y PROTOCOL_BIND_BTN_Y
  #define PROTOCOL_OTA_CFG_BTN_W 52
  #define PROTOCOL_OTA_CFG_BTN_H 28
  #define PROTOCOL_OTA_CFG_BTN_X (PROTOCOL_OTA_BTN_X + PROTOCOL_OTA_BTN_W + 6)
  #define PROTOCOL_OTA_CFG_BTN_Y PROTOCOL_BIND_BTN_Y
  #define PROTOCOL_STATUS_X 14
  #define PROTOCOL_STATUS_Y (PROTOCOL_BIND_BTN_Y - 24)
  #define PROTOCOL_STATUS_W 212
  #define PROTOCOL_STATUS_H 20
  #define ELRS_BIND_BTN_X 20
  #define ELRS_BIND_BTN_Y 132
  #define ELRS_BIND_BTN_W 200
  #define ELRS_BIND_BTN_H 72
  #define OTA_HOSTNAME "anubis-tx"
  #define OTA_STA_SSID ""
  #define OTA_STA_PASSWORD ""
  #define OTA_AP_SSID "ANUBIS-OTA"
  #define OTA_AP_PASSWORD "anubisota"
  #define RATE_LOW_VALUE 40
  #define RATE_NORMAL_VALUE 65
  #define RATE_HIGH_VALUE 80
  #define EXPO_LOW_VALUE 20
  #define EXPO_NORMAL_VALUE 40
  #define EXPO_HIGH_VALUE 60
  #define RATE_TAB_Y 58
  #define RATE_TAB_W 46
  #define RATE_TAB_H 28
  #define RATE_TAB_GAP 8
  #define RATE_TAB_X(i) (16 + ((i) * (RATE_TAB_W + RATE_TAB_GAP)))
  #define RATE_TOGGLE_Y 112
  #define RATE_TOGGLE_W 60
  #define RATE_TOGGLE_H 28
  #define RATE_TOGGLE_GAP 10
  #define RATE_TOGGLE_X(i) (15 + ((i) * (RATE_TOGGLE_W + RATE_TOGGLE_GAP)))
  #define RATE_SLIDER_X 24
  #define RATE_SLIDER_Y 172
  #define RATE_SLIDER_W 192
  #define RATE_SLIDER_H 22
  #define RATE_VALUE_BOX_X 82
  #define RATE_VALUE_BOX_Y 218
  #define RATE_VALUE_BOX_W 76
  #define RATE_VALUE_BOX_H 34
  #define EXPO_GRAPH_X 28
  #define EXPO_GRAPH_Y 108
  #define EXPO_GRAPH_W 184
  #define EXPO_GRAPH_H 100
  #define EXPO_VALUE_LABEL_Y 216
  #define EXPO_VALUE_BOX_Y 232

  // ==== SPLASH SCREEN ====
  #define ANUBIS_WIDTH 160
  #define ANUBIS_HEIGHT 160

  #define MAX_MODELS 4

  int modelPanelX = 0;
  int modelPanelY = 0;
  int modelPanelW = 0;
  int modelPanelH = 0;

  uint16_t COLOR_BG = 0;
  uint16_t COLOR_PANEL = 0;
  uint16_t COLOR_TEXT = 0;
  uint16_t COLOR_ACCENT = 0;
  uint16_t COLOR_ACCENT_HI = 0;
  uint16_t COLOR_SIG = 0;
  uint16_t COLOR_STICK_PANEL = 0;

  int currentTrimPage = 0; // 0 = left gimbal, 1 = right gimbal

  bool trimNeedsRedraw = true;
  bool trimDirty = true;

  static int lastSignal = -1;
  static int lastBattery = -1;

  bool reverseNeedsRedraw = true;
  bool reverseChannelDirty[CHANNEL_COUNT] = { true, true, true, true };

  bool failsafeNeedsRedraw = true;
  bool failsafeDirty[CHANNEL_COUNT] = { true, true, true, true };
  bool mixingNeedsRedraw = true;
  bool batteryPresent = false;
  bool batteryCharging = false;
  bool ads1115Ready = false;
  bool stickFilterInitialized = false;
  uint8_t adsConsecutiveReadFails = 0;
  unsigned long lastAdsReconnectAttemptMs = 0;
  unsigned long lastBatterySampleTime = 0;
  unsigned long lastStickSampleTime = 0;
  float batteryFilteredVoltage = 0.0f;
  bool batteryFilterInitialized = false;
  unsigned long batteryChargeWindowStart = 0;
  float batteryChargeWindowVoltage = 0.0f;
  unsigned long batteryChargingHoldUntil = 0;
  int16_t adsStickCenter[4] = { 0, 0, 0, 0 };
  float filteredStickAxis[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  BattleEnemy spaceEnemies[BZ_MAX_ENEMIES];
  BattleObstacle spaceObstacles[BZ_MAX_OBSTACLES];
  float spacePlayerX = 0.0f;
  float spacePlayerY = 0.0f;
  float spacePlayerHeading = 0.0f;
  bool spacePlayerShotActive = false;
  float spacePlayerShotX = 0.0f;
  float spacePlayerShotY = 0.0f;
  float spacePlayerShotVX = 0.0f;
  float spacePlayerShotVY = 0.0f;
  float spacePlayerShotDistance = 0.0f;
  bool spaceEnemyShotActive = false;
  float spaceEnemyShotX = 0.0f;
  float spaceEnemyShotY = 0.0f;
  float spaceEnemyShotVX = 0.0f;
  float spaceEnemyShotVY = 0.0f;
  bool spaceMissileActive = false;
  float spaceMissileX = 0.0f;
  float spaceMissileY = 0.0f;
  float spaceMissileVX = 0.0f;
  float spaceMissileVY = 0.0f;
  unsigned long spaceNextMissileAt = 0;
  unsigned long spaceMuzzleFlashUntil = 0;
  int spaceScore = 0;
  int spaceLives = 3;
  int spaceWave = 1;
  unsigned long lastSpaceFrameTime = 0;
  unsigned long lastSpaceShotTime = 0;
  unsigned long lastSpaceRespawnAt = 0;
  bool spaceGameOver = false;
  bool spaceWaveCleared = false;
  bool spaceGameStarted = false;
  int selectedMixIndex = 0;
  int selectedRateChannel = 0;
  int selectedExpoChannel = 0;
  int endpointNumpadChannel = -1;
  bool endpointNumpadHighSide = true;
  bool mixNumpadActive = false;
  bool mixNumpadNeedsRedraw = false;
  bool ratesNeedsRedraw = true;
  bool expoNeedsRedraw = true;
  bool endpointNeedsRedraw = true;
  bool ratesValueDirty = false;
  bool expoValueDirty = false;
  uint8_t endpointSideLatch[CHANNEL_COUNT] = { 1, 1, 1, 1 };
  bool mixNumpadEditingRate = true;
  NumpadTarget mixNumpadTarget = NUMPAD_TARGET_MIX_RATE;
  String mixNumpadBuffer = "";
  int mixNumpadCursorRow = 0;
  int mixNumpadCursorCol = 0;
  extern char otaStaSsid[OTA_STA_SSID_STORAGE_BYTES + 1];
  extern char otaStaPassword[OTA_STA_PASSWORD_STORAGE_BYTES + 1];
  extern char otaApSsid[OTA_AP_SSID_STORAGE_BYTES + 1];

  // placeholder values for now (you’ll replace with real captured values later)
  bool isInside(int x, int y, int bx, int by, int bw, int bh);
  int mapTouch(int val, int in_min, int in_max, int out_min, int out_max);
  void setScreen(Screen screen);
  void queueScreenButton(ButtonID button, Screen screen);
  void saveKeyboardBufferToModelSlot();
  void sanitizeDisplaySettings();
  void resetDisplaySettingsToDefaults();
  bool loadDisplaySettings();
  void saveDisplaySettings();
  void applyThemePalette();
  void applyDisplayBacklight();
  unsigned long getDisplayDimTimeoutMs();
  unsigned long getDisplayOffTimeoutMs();
  int findDisplayOptionIndex(const uint8_t *options, int optionCount, uint8_t value);
  void stepDisplayOption(uint8_t &value, const uint8_t *options, int optionCount, int delta);
  const char* formatDisplayTimeoutValue(uint8_t optionIndex, const uint16_t *options, uint8_t optionCount,
                                        char *buffer, size_t bufferSize);
  const char* getThemeToggleLabel();
  void loadOtaSettings();
  void saveOtaSettings();
  void applyKeyboardBufferToTarget();
  int getKeyboardMaxLength();
  void beginOtaFieldEdit(KeyboardTarget target);
  void drawOtaSettingsStatic();
  void drawOtaSettingsDynamic();
  void drawDisplaySettingsScreen();
  void drawDisplaySettingsRow(int y, const char* label, const char* valueText,
                              bool minusPressed, bool minusSelected,
                              bool plusPressed, bool plusSelected,
                              uint16_t previewColor, int previewWidth);
  void drawGearIcon(int x, int y, int size, uint16_t iconColor);
  const char* getKeyboardKey(int row, int col);
  void processKeyboardKey(const char* key);
  bool modelSlotUninitialized(int i);
  bool modelHasBoundReceiver(int modelIndex);
  void clearBoundReceiver(int modelIndex);
  void setBoundReceiver(int modelIndex, const uint8_t *macAddress);
  const uint8_t* getBoundReceiverMac(int modelIndex);
  int getModelFailsafeValue(int modelIndex, int channel);
  void setModelFailsafeValue(int modelIndex, int channel, int value);
  void clearModelFailsafeValues(int modelIndex);
  int getModelRateValue(int modelIndex, int channel);
  void setModelRateValue(int modelIndex, int channel, int value);
  void clearModelRateValues(int modelIndex);
  int getModelExpoValue(int modelIndex, int channel);
  void setModelExpoValue(int modelIndex, int channel, int value);
  void clearModelExpoValues(int modelIndex);
  int getModelEndpointLowValue(int modelIndex, int channel);
  int getModelEndpointHighValue(int modelIndex, int channel);
  void setModelEndpointLowValue(int modelIndex, int channel, int value);
  void setModelEndpointHighValue(int modelIndex, int channel, int value);
  void clearModelEndpointValues(int modelIndex);
  void loadReceiverBindings();
  void saveReceiverBindings();
  void loadFailsafeValues();
  void saveFailsafeValues();
  void loadRateValues();
  void saveRateValues();
  void loadExpoValues();
  void saveExpoValues();
  void loadEndpointValues();
  void saveEndpointValues();
  void initDefaultMixSlot(int modelIndex, int mixIndex);
  bool sanitizeModelMixes(int modelIndex);
  bool sanitizeModelDriveType(int modelIndex);
  bool sanitizeModelProtocol(int modelIndex);
  uint8_t getMixSource(const MixData &mix);
  void setMixSource(MixData &mix, uint8_t source);
  uint8_t getMixDestination(const MixData &mix);
  void setMixDestination(MixData &mix, uint8_t destination);
  bool isMixReverseLinked(const MixData &mix);
  void setMixReverseLinked(MixData &mix, bool linked);
  void openMixNumpad(NumpadTarget target);
  void closeMixNumpad(bool commitValue);
  bool handleMixNumpadTouch(int x, int y);
  void drawMixNumpad();
  void drawDriveTypeScreen();
  void drawTankModeScreen();
  void drawSpaceGameStatic();
  void drawSpaceGameDynamic();
  void drawCarBars(int x, int y, int w, int h, float steering, float throttle);
  void drawOmniBars(int x, int y, int w, int h, float ch4, float ch1, float ch2);
  void drawDriveTypeOption(int x, int y, int w, int h, const char* label, DriveType drive, ButtonID button);
  void drawDriveTypeOptionIcon(int x, int y, int w, int h, DriveType drive, uint16_t iconColor);
  void drawLegacyTankIcon(int x, int y, int w, int h, uint16_t iconColor);
  void drawLegacyQuadXIcon(int x, int y, int w, int h, uint16_t iconColor);
  void drawLegacyCarIcon(int x, int y, int s, uint16_t iconColor);
  uint16_t tintRgb565Pixel(uint16_t srcColor, uint16_t tintColor);
  void drawRgb565IconTinted(int x, int y, int w, int h, const uint16_t* iconData, int iconW, int iconH, uint16_t tintColor);
  void drawBackIcon(int x, int y, int s, uint16_t iconColor);
  void drawOmniIcon(int x, int y, int w, int h, uint16_t iconColor);
  void drawEndpointIcon(int x, int y, int s, uint16_t iconColor);
  void drawExpoIcon(int x, int y, int s, uint16_t iconColor);
  void drawRatesIcon(int x, int y, int s, uint16_t iconColor);
  void drawProtocolIcon(int x, int y, int s, uint16_t iconColor);
  void getLinkedReverseChannels(int modelIndex, int channel, bool linked[CHANNEL_COUNT]);
  bool hasLinkedReversePeers(int modelIndex, int channel);
  bool shouldForceTankSingleReversePair(int modelIndex);
  void drawTrimGraphBase();
  void drawTrimButtons();
  void drawMixingStatic();
  void drawMixingDynamic();
  void updateBatteryState();
  void drawSplash();
  void drawGameMenuButton();
  bool initAds1115();
  bool readAds1115SingleEnded(uint8_t channel, int16_t &value);
  void updateStickInputs(unsigned long now);
  float normalizeStickAxis(int16_t raw, int16_t center);
  void updateChannelOutputs();
  float applyExpoCurve(float value, int expoPercent);
  bool updateEndpointSideLatch(int channel);
  bool isEndpointHighSideSelected(int channel);
  float applyEndpointScaling(float value, int channel);
  float getChannelTrimOffset(int channel);
  const char* getChannelAxisName(uint8_t channel);
  float getCarSteeringOutput();
  float getCarThrottleOutput();
  DriveType getModelDriveType(int modelIndex);
  void setModelDriveType(int modelIndex, DriveType driveType);
  TankControlMode getModelTankMode(int modelIndex);
  void setModelTankMode(int modelIndex, TankControlMode tankMode);
  ProtocolType getModelProtocol(int modelIndex);
  void setModelProtocol(int modelIndex, ProtocolType protocolType);
  void drawExpoStatic();
  void drawExpoDynamic();
  void drawRatesStatic();
  void drawRatesDynamic();
  void drawEndpointStatic();
  void drawEndpointDynamic();
  void composeProtocolStatusText(char *bindStatus, size_t bindStatusSize, uint16_t *statusColor);
  void composeElrsStatusText(char *statusText, size_t statusTextSize, uint16_t *statusColor);
  void drawProtocolScreen();
  void drawProtocolDynamic();
  void drawElrsScreen();
  void drawElrsDynamic();
  void sanitizeWifiText(char *value, size_t maxLen, bool allowEmpty);
  void sanitizeOtaSettings();
  void logOtaNetworkState(const char *label);
  void logPartitionInfo(const char *label, const esp_partition_t *partition);
  bool isOtaKeyboardTarget(KeyboardTarget target);
  const char* getKeyboardTargetLabel(KeyboardTarget target);
  void drawKeyboardPreview();
  void startOtaMode();
  void stopOtaMode();
  void updateOtaService();
  void drawControllerPlaceholderScreen(const char* title, const char* line1, const char* line2);
  bool initEspNowLink();
  bool setEspNowWifiChannel(uint8_t channel);
  bool ensureEspNowPeer(const uint8_t *peerAddress);
  void resetEspNowTelemetry();
  void updateEspNowLink(unsigned long now);
  void initElrsUart();
  void updateElrsLink(unsigned long now);
  void sendElrsChannelFrame();
  void elrsSendFrame(const uint8_t *frame, size_t len);
  void sendElrsDevicePing();
  void sendElrsBindCommandTo(uint8_t destination);
  void sendElrsLuaBindWrite(uint8_t fieldIndex);
  void sendElrsBindCommand();
  void sendElrsParameterRead(uint8_t fieldIndex, uint8_t chunkIndex);
  void updateElrsParameterDiscovery(unsigned long now);
  void parseElrsDeviceInfoFrame(const uint8_t *frame, uint8_t expected);
  void parseElrsParameterEntryFrame(const uint8_t *frame, uint8_t expected);
  void readElrsSerial(unsigned long now);
  void restartElrsUart(uint32_t baud);
  void advanceElrsSerialMode();
  void applyElrsUartPinPair(int pinPairIndex);
  bool hasElrsModuleFrames();
  bool hasElrsTxOnlyActivity(unsigned long now);
  void IRAM_ATTR onElrsRxEdge();
  bool containsIgnoreCase(const char *haystack, const char *needle);
  uint8_t crsfCrc8Poly(const uint8_t *data, size_t length, uint8_t poly);
  uint8_t crsfCrc8(const uint8_t *data, size_t length);
  int channelToCrsf(float value);
  int elrsLqToBars(uint8_t linkQuality);
  bool hasEspNowSignal(unsigned long now);
  bool hasEspNowHeaderSignal(unsigned long now);
  uint16_t getLatencyIndicatorColor(int latencyMs);
  void sendEspNowControlPacket(unsigned long now);
  void sendEspNowPing(unsigned long now);
  void sendEspNowBindCommit(const uint8_t *receiverMac);
  bool hasEspNowPendingBindMac();
  void beginEspNowBinding();
  void cancelEspNowBinding(bool clearPendingMac);
  void handleEspNowBindPacket(const esp_now_recv_info_t *info, const uint8_t *data, int len);
  void formatReceiverMacShort(const uint8_t *macAddress, char *buffer, size_t bufferSize);
  void handleEspNowTelemetryPacket(const uint8_t *data, int len);
  void handleEspNowPingAckPacket(const uint8_t *data, int len);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
  void onEspNowSend(const esp_now_send_info_t *tx_info, esp_now_send_status_t status);
#else
  void onEspNowSend(const uint8_t *macAddr, esp_now_send_status_t status);
#endif
  void onEspNowReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len);
  void resetSpaceGame();
  void updateSpaceGame(unsigned long now, bool leftPressed, bool rightPressed, bool firePressed, bool exitPressed);
  void handleSpaceGameTouch(int x, int y);
  void moveSpacePlayer(int delta);
  void fireSpacePlayerBullet();
  void fireSpaceEnemyBullet();
  bool spaceEnemiesRemaining();

#line 1759 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void initModelDefaults(int i);
#line 1965 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void selectModelSlot(int modelIndex);
#line 1991 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void beginModelNameEdit(int modelIndex);
#line 3370 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void setup();
#line 3521 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void loop();
#line 5528 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawThickLine(int x1, int y1, int x2, int y2, uint16_t c);
#line 5533 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawThickCircle(int x, int y, int r, uint16_t c);
#line 5538 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawThickRoundRect(int x, int y, int w, int h, int r, uint16_t c);
#line 5543 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawTopLeftBevelHighlight(int x, int y, int w, int h, int radius, int inset, uint16_t highlightColor);
#line 5598 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawGradientControl(int x, int y, int w, int h, int radius, uint16_t baseColor, uint16_t outlineColor);
#line 5831 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawMenuScreen();
#line 6044 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
static float wrapBattleAngle(float angle);
#line 6051 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
static int countAliveBattleEnemies();
#line 6059 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
static bool battleWorldToScreen(float wx, float wy, float &sx, float &scale, float &forward);
#line 6076 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
static bool battlePositionBlocked(float x, float y);
#line 6087 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
static void spawnBattleWave(unsigned long now);
#line 6600 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawControllerSettings();
#line 7258 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawModelSettings();
#line 7467 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void closeKeyboard();
#line 7718 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void saveModels();
#line 7730 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void loadModels();
#line 7740 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void handleTouch(int x, int y);
#line 8810 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
bool handleKeyboardTouch(int x, int y);
#line 8998 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawMainScreenStatic();
#line 9033 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawModelPanelSemiStatic();
#line 9073 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawMainScreenDynamic();
#line 9314 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawTankIcon(int x, int y, int w, int h, uint16_t iconColor);
#line 9318 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawQuadXIcon(int x, int y, int w, int h, uint16_t iconColor);
#line 9322 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawTankBars(int x, int y, int w, int h, float left, float right);
#line 9453 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawRightPanel(int x, int y, int w, int h);
#line 9506 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawCenteredBar(int x, int y, int w, int h, float value);
#line 9535 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawQuadBars(int x, int y, int w, int h, float m1, float m2, float m3, float m4);
#line 9547 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawBottomBar(int x, int y, int w, int h, float value);
#line 9558 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawSignalBars(int x, int y, int strength);
#line 9577 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawBattery(int x, int y, int level, bool present, bool charging);
#line 9623 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawTopBarStatic();
#line 9649 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawTopBarDynamic();
#line 9764 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
uint16_t fadeColor(uint16_t color, float factor);
#line 9778 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
uint16_t tankThrottleColor(float t);
#line 9792 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
uint16_t throttleColor(float t);
#line 9818 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawStickBaseDirect(int x, int y, int size);
#line 9864 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawStickBase(int x, int y, int size);
#line 9937 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawStickKnob(int x, int y, int size, int posX, int posY);
#line 9964 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawReverseStatic();
#line 10018 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawReverseDynamic();
#line 10140 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawTrimStatic();
#line 10178 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawTrimDynamic();
#line 10232 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawFailsafeStatic();
#line 10293 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawFailsafeDynamic();
#line 10348 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawKeyboardStatic();
#line 10405 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawKeyboardDynamic();
#line 10454 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void handleKeyboardSelect();
#line 10464 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawModelNameStatic();
#line 10495 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawModelNameDynamic();
#line 10828 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawControllerIcon(int x, int y, int s, uint16_t iconColor);
#line 10840 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawCarIcon(int x, int y, int s, uint16_t iconColor);
#line 10846 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawHomeIcon(int x, int y, int s, uint16_t iconColor);
#line 10851 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawReverseIcon(int x, int y, int s, uint16_t iconColor);
#line 10857 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawTrimIcon(int x, int y, int s, uint16_t iconColor);
#line 10863 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawFailsafeIcon(int x, int y, int s, uint16_t iconColor);
#line 10908 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawModelNameIcon(int x, int y, int s, uint16_t iconColor);
#line 10914 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawDriveTypeIcon(int x, int y, int s, uint16_t iconColor);
#line 10922 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
void drawMixingIcon(int x, int y, int s, uint16_t iconColor);
#line 1026 "C:\\Users\\Avala\\OneDrive\\Desktop\\sketch_may7a\\Hosyond_apr24b\\Hosyond_apr24b.ino"
  uint8_t getMixSource(const MixData &mix) {
    return mix.source & MIX_CHANNEL_MASK;
  }

  void setMixSource(MixData &mix, uint8_t source) {
    mix.source = (mix.source & MIX_REVERSE_SEPARATE_FLAG) | (source & MIX_CHANNEL_MASK);
  }

  uint8_t getMixDestination(const MixData &mix) {
    return mix.destination & MIX_CHANNEL_MASK;
  }

  void setMixDestination(MixData &mix, uint8_t destination) {
    mix.destination = destination & MIX_CHANNEL_MASK;
  }

  bool isMixReverseLinked(const MixData &mix) {
    return (mix.source & MIX_REVERSE_SEPARATE_FLAG) == 0;
  }

  void setMixReverseLinked(MixData &mix, bool linked) {
    if (linked) {
      mix.source &= ~MIX_REVERSE_SEPARATE_FLAG;
    } else {
      mix.source |= MIX_REVERSE_SEPARATE_FLAG;
    }
  }

  bool modelHasBoundReceiver(int modelIndex) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return false;

    for (int i = 0; i < ESP_NOW_ETH_ALEN; i++) {
      if (boundReceiverMacs[modelIndex][i] != 0x00) return true;
    }

    return false;
  }

  void clearBoundReceiver(int modelIndex) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
    memset(boundReceiverMacs[modelIndex], 0, ESP_NOW_ETH_ALEN);
  }

  void setBoundReceiver(int modelIndex, const uint8_t *macAddress) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS || macAddress == nullptr) return;
    memcpy(boundReceiverMacs[modelIndex], macAddress, ESP_NOW_ETH_ALEN);
  }

  const uint8_t* getBoundReceiverMac(int modelIndex) {
    if (!modelHasBoundReceiver(modelIndex)) return nullptr;
    return boundReceiverMacs[modelIndex];
  }

  int getModelFailsafeValue(int modelIndex, int channel) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return 0;
    if (channel < 0 || channel >= CHANNEL_COUNT) return 0;
    return modelFailsafeValues[modelIndex][channel];
  }

  void setModelFailsafeValue(int modelIndex, int channel, int value) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
    if (channel < 0 || channel >= CHANNEL_COUNT) return;
    modelFailsafeValues[modelIndex][channel] = (int8_t)constrain(value, -100, 100);
  }

  void clearModelFailsafeValues(int modelIndex) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
      modelFailsafeValues[modelIndex][channel] = 0;
    }
  }

  int getModelRateValue(int modelIndex, int channel) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return RATE_NORMAL_VALUE;
    if (channel < 0 || channel >= CHANNEL_COUNT) return RATE_NORMAL_VALUE;
    return modelRateValues[modelIndex][channel];
  }

  void setModelRateValue(int modelIndex, int channel, int value) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
    if (channel < 0 || channel >= CHANNEL_COUNT) return;
    modelRateValues[modelIndex][channel] = (uint8_t)constrain(value, 1, 100);
  }

  void clearModelRateValues(int modelIndex) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
      modelRateValues[modelIndex][channel] = RATE_NORMAL_VALUE;
    }
  }

  int getModelExpoValue(int modelIndex, int channel) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return EXPO_NORMAL_VALUE;
    if (channel < 0 || channel >= CHANNEL_COUNT) return EXPO_NORMAL_VALUE;
    return modelExpoValues[modelIndex][channel];
  }

  void setModelExpoValue(int modelIndex, int channel, int value) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
    if (channel < 0 || channel >= CHANNEL_COUNT) return;
    modelExpoValues[modelIndex][channel] = (uint8_t)constrain(value, 0, 100);
  }

  void clearModelExpoValues(int modelIndex) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
      modelExpoValues[modelIndex][channel] = EXPO_NORMAL_VALUE;
    }
  }

  int getModelEndpointLowValue(int modelIndex, int channel) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return 100;
    if (channel < 0 || channel >= CHANNEL_COUNT) return 100;
    return modelEndpointLowValues[modelIndex][channel];
  }

  int getModelEndpointHighValue(int modelIndex, int channel) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return 100;
    if (channel < 0 || channel >= CHANNEL_COUNT) return 100;
    return modelEndpointHighValues[modelIndex][channel];
  }

void setModelEndpointLowValue(int modelIndex, int channel, int value) {
  if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
  if (channel < 0 || channel >= CHANNEL_COUNT) return;
  modelEndpointLowValues[modelIndex][channel] = (uint8_t)constrain(value, 1, 120);
}

void setModelEndpointHighValue(int modelIndex, int channel, int value) {
  if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
  if (channel < 0 || channel >= CHANNEL_COUNT) return;
  modelEndpointHighValues[modelIndex][channel] = (uint8_t)constrain(value, 1, 120);
}

  void clearModelEndpointValues(int modelIndex) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return;
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
      modelEndpointLowValues[modelIndex][channel] = 100;
      modelEndpointHighValues[modelIndex][channel] = 100;
    }
  }

  void loadReceiverBindings() {
    if (EEPROM.read(EEPROM_BIND_VERSION_ADDR) != ESPNOW_BIND_STORAGE_VERSION) {
      for (int i = 0; i < MAX_MODELS; i++) {
        clearBoundReceiver(i);
      }
      return;
    }

    int addr = EEPROM_BIND_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int b = 0; b < ESP_NOW_ETH_ALEN; b++) {
        boundReceiverMacs[i][b] = EEPROM.read(addr++);
      }
    }
  }

  void saveReceiverBindings() {
    int addr = EEPROM_BIND_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int b = 0; b < ESP_NOW_ETH_ALEN; b++) {
        EEPROM.write(addr++, boundReceiverMacs[i][b]);
      }
    }

    EEPROM.write(EEPROM_BIND_VERSION_ADDR, ESPNOW_BIND_STORAGE_VERSION);
    EEPROM.commit();
  }

  void loadFailsafeValues() {
    if (EEPROM.read(EEPROM_FAILSAFE_VERSION_ADDR) != FAILSAFE_STORAGE_VERSION) {
      for (int i = 0; i < MAX_MODELS; i++) {
        clearModelFailsafeValues(i);
      }
      return;
    }

    int addr = EEPROM_FAILSAFE_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        modelFailsafeValues[i][channel] = (int8_t)EEPROM.read(addr++);
      }
    }
  }

  void saveFailsafeValues() {
    int addr = EEPROM_FAILSAFE_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        EEPROM.write(addr++, (uint8_t)modelFailsafeValues[i][channel]);
      }
    }

    EEPROM.write(EEPROM_FAILSAFE_VERSION_ADDR, FAILSAFE_STORAGE_VERSION);
    EEPROM.commit();
  }

  void loadRateValues() {
    if (EEPROM.read(EEPROM_RATE_VERSION_ADDR) != RATE_STORAGE_VERSION) {
      for (int i = 0; i < MAX_MODELS; i++) {
        clearModelRateValues(i);
      }
      return;
    }

    int addr = EEPROM_RATE_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        uint8_t storedValue = EEPROM.read(addr++);
        modelRateValues[i][channel] = constrain(storedValue, 1, 100);
      }
    }
  }

  void saveRateValues() {
    int addr = EEPROM_RATE_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        EEPROM.write(addr++, modelRateValues[i][channel]);
      }
    }

    EEPROM.write(EEPROM_RATE_VERSION_ADDR, RATE_STORAGE_VERSION);
    EEPROM.commit();
  }

  void loadExpoValues() {
    if (EEPROM.read(EEPROM_EXPO_VERSION_ADDR) != EXPO_STORAGE_VERSION) {
      for (int i = 0; i < MAX_MODELS; i++) {
        clearModelExpoValues(i);
      }
      return;
    }

    int addr = EEPROM_EXPO_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        uint8_t storedValue = EEPROM.read(addr++);
        modelExpoValues[i][channel] = constrain(storedValue, 0, 100);
      }
    }
  }

  void saveExpoValues() {
    int addr = EEPROM_EXPO_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        EEPROM.write(addr++, modelExpoValues[i][channel]);
      }
    }

    EEPROM.write(EEPROM_EXPO_VERSION_ADDR, EXPO_STORAGE_VERSION);
    EEPROM.commit();
  }

  void loadEndpointValues() {
    if (EEPROM.read(EEPROM_ENDPOINT_VERSION_ADDR) != ENDPOINT_STORAGE_VERSION) {
      for (int i = 0; i < MAX_MODELS; i++) {
        clearModelEndpointValues(i);
      }
      return;
    }

    int addrLow = EEPROM_ENDPOINT_LOW_DATA_ADDR;
    int addrHigh = EEPROM_ENDPOINT_HIGH_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        uint8_t lowStored = EEPROM.read(addrLow++);
        uint8_t highStored = EEPROM.read(addrHigh++);
        modelEndpointLowValues[i][channel] = constrain(lowStored, 1, 120);
        modelEndpointHighValues[i][channel] = constrain(highStored, 1, 120);
      }
    }
  }

  void saveEndpointValues() {
    int addrLow = EEPROM_ENDPOINT_LOW_DATA_ADDR;
    int addrHigh = EEPROM_ENDPOINT_HIGH_DATA_ADDR;
    for (int i = 0; i < MAX_MODELS; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        EEPROM.write(addrLow++, modelEndpointLowValues[i][channel]);
        EEPROM.write(addrHigh++, modelEndpointHighValues[i][channel]);
      }
    }

    EEPROM.write(EEPROM_ENDPOINT_VERSION_ADDR, ENDPOINT_STORAGE_VERSION);
    EEPROM.commit();
  }

  void sanitizeDisplaySettings() {
    displayBrightness = constrain(displayBrightness, (uint8_t)1, (uint8_t)255);
    displaySleepBrightness = constrain(displaySleepBrightness, (uint8_t)0, displayBrightness);
    if (displayTimeoutIndex >= displayTimeoutOptionCount) {
      displayTimeoutIndex = DEFAULT_DISPLAY_TIMEOUT_INDEX;
    }
    if (displayOffTimeoutIndex >= displayOffTimeoutOptionCount) {
      displayOffTimeoutIndex = DEFAULT_DISPLAY_OFF_TIMEOUT_INDEX;
    }
    if (themeMode > THEME_LIGHT) {
      themeMode = THEME_DARK;
    }
  }

  void resetDisplaySettingsToDefaults() {
    displayBrightness = DEFAULT_DISPLAY_BRIGHTNESS;
    displaySleepBrightness = DEFAULT_DISPLAY_SLEEP_BRIGHTNESS;
    displayTimeoutIndex = DEFAULT_DISPLAY_TIMEOUT_INDEX;
    displayOffTimeoutIndex = DEFAULT_DISPLAY_OFF_TIMEOUT_INDEX;
    themeMode = THEME_DARK;
    sanitizeDisplaySettings();
  }

  bool loadDisplaySettings() {
    if (EEPROM.read(EEPROM_DISPLAY_VERSION_ADDR) != DISPLAY_SETTINGS_STORAGE_VERSION) {
      resetDisplaySettingsToDefaults();
      return true;
    }

    uint8_t loadedBrightness = EEPROM.read(EEPROM_DISPLAY_BRIGHTNESS_ADDR);
    uint8_t loadedSleepBrightness = EEPROM.read(EEPROM_DISPLAY_SLEEP_BRIGHTNESS_ADDR);
    uint8_t loadedTimeoutIndex = EEPROM.read(EEPROM_DISPLAY_TIMEOUT_INDEX_ADDR);
    uint8_t loadedOffTimeoutIndex = EEPROM.read(EEPROM_DISPLAY_OFF_TIMEOUT_INDEX_ADDR);
    uint8_t loadedThemeMode = EEPROM.read(EEPROM_DISPLAY_THEME_ADDR);

    displayBrightness = loadedBrightness;
    displaySleepBrightness = loadedSleepBrightness;
    displayTimeoutIndex = loadedTimeoutIndex;
    displayOffTimeoutIndex = loadedOffTimeoutIndex;
    themeMode = loadedThemeMode;
    sanitizeDisplaySettings();

    return (displayBrightness != loadedBrightness) ||
           (displaySleepBrightness != loadedSleepBrightness) ||
           (displayTimeoutIndex != loadedTimeoutIndex) ||
           (displayOffTimeoutIndex != loadedOffTimeoutIndex) ||
           (themeMode != loadedThemeMode);
  }

  void saveDisplaySettings() {
    sanitizeDisplaySettings();
    EEPROM.write(EEPROM_DISPLAY_BRIGHTNESS_ADDR, displayBrightness);
    EEPROM.write(EEPROM_DISPLAY_SLEEP_BRIGHTNESS_ADDR, displaySleepBrightness);
    EEPROM.write(EEPROM_DISPLAY_TIMEOUT_INDEX_ADDR, displayTimeoutIndex);
    EEPROM.write(EEPROM_DISPLAY_OFF_TIMEOUT_INDEX_ADDR, displayOffTimeoutIndex);
    EEPROM.write(EEPROM_DISPLAY_THEME_ADDR, themeMode);
    EEPROM.write(EEPROM_DISPLAY_VERSION_ADDR, DISPLAY_SETTINGS_STORAGE_VERSION);
    EEPROM.commit();
  }

  void applyThemePalette() {
    if (themeMode == THEME_LIGHT) {
      COLOR_BG = tft.color565(244, 236, 214);
      COLOR_PANEL = tft.color565(230, 219, 193);
      COLOR_TEXT = tft.color565(50, 36, 16);
      COLOR_ACCENT = tft.color565(180, 140, 40);
      COLOR_ACCENT_HI = tft.color565(226, 184, 84);
      COLOR_SIG = tft.color565(56, 132, 64);
      COLOR_STICK_PANEL = tft.color565(216, 204, 178);
    } else {
      COLOR_BG = tft.color565(0, 0, 0);
      COLOR_PANEL = tft.color565(30, 30, 30);
      COLOR_TEXT = tft.color565(255, 255, 255);
      COLOR_ACCENT = tft.color565(180, 140, 40);
      COLOR_ACCENT_HI = tft.color565(255, 210, 80);
      COLOR_SIG = tft.color565(49, 146, 49);
      COLOR_STICK_PANEL = tft.color565(20, 20, 20);
    }

    stickBaseSpriteNeedsRebuild = true;
  }

  int findDisplayOptionIndex(const uint8_t *options, int optionCount, uint8_t value) {
    int bestIndex = 0;
    int bestDistance = 1000;

    for (int i = 0; i < optionCount; i++) {
      int distance = abs((int)options[i] - (int)value);
      if (distance < bestDistance) {
        bestDistance = distance;
        bestIndex = i;
      }
    }

    return bestIndex;
  }

  void stepDisplayOption(uint8_t &value, const uint8_t *options, int optionCount, int delta) {
    int index = findDisplayOptionIndex(options, optionCount, value);
    index = constrain(index + delta, 0, optionCount - 1);
    value = options[index];
  }

  unsigned long getDisplayDimTimeoutMs() {
    if (displayTimeoutIndex >= displayTimeoutOptionCount) {
      return (unsigned long)displayTimeoutOptionsSec[DEFAULT_DISPLAY_TIMEOUT_INDEX] * 1000UL;
    }

    uint16_t seconds = displayTimeoutOptionsSec[displayTimeoutIndex];
    if (seconds == 0) return 0;
    return (unsigned long)seconds * 1000UL;
  }

  unsigned long getDisplayOffTimeoutMs() {
    if (displayOffTimeoutIndex >= displayOffTimeoutOptionCount) {
      return (unsigned long)displayOffTimeoutOptionsSec[DEFAULT_DISPLAY_OFF_TIMEOUT_INDEX] * 1000UL;
    }

    uint16_t seconds = displayOffTimeoutOptionsSec[displayOffTimeoutIndex];
    if (seconds == 0) return 0;
    return (unsigned long)seconds * 1000UL;
  }

  const char* formatDisplayTimeoutValue(uint8_t optionIndex, const uint16_t *options, uint8_t optionCount,
                                        char *buffer, size_t bufferSize) {
    if (optionIndex >= optionCount) {
      snprintf(buffer, bufferSize, "30s");
      return buffer;
    }

    uint16_t seconds = options[optionIndex];
    if (seconds == 0) {
      snprintf(buffer, bufferSize, "Never");
    }
    else if (seconds < 60) {
      snprintf(buffer, bufferSize, "%us", (unsigned int)seconds);
    }
    else {
      snprintf(buffer, bufferSize, "%um", (unsigned int)(seconds / 60));
    }
    return buffer;
  }

  const char* getThemeToggleLabel() {
    return (themeMode == THEME_DARK) ? "Light Mode" : "Dark Mode";
  }

  void applyDisplayBacklight() {
    uint8_t targetBrightness = BRIGHTNESS_OFF;
    if (screenAwake) {
      targetBrightness = displayDimmed ? displaySleepBrightness : displayBrightness;
    }
    targetBrightness = min(targetBrightness, displayBrightness);

    bool shouldEnablePanel = (screenAwake && targetBrightness > 0);
    if (panelDisplayEnabled != shouldEnablePanel) {
      tft.writecommand(shouldEnablePanel ? ILI9341_DISPON : ILI9341_DISPOFF);
      panelDisplayEnabled = shouldEnablePanel;
    }

    if (lastAppliedBacklightBrightness != targetBrightness) {
      analogWrite(DISPLAY_BL_PIN, targetBrightness);
      if (targetBrightness == BRIGHTNESS_OFF) {
        digitalWrite(DISPLAY_BL_PIN, !TFT_BACKLIGHT_ON);
      }
      lastAppliedBacklightBrightness = targetBrightness;

#if DISPLAY_BACKLIGHT_SERIAL_DEBUG
      Serial.printf("DISPLAY: backlight target=%u awake=%u dimmed=%u duty=%lu freq=%lu\n",
                    targetBrightness,
                    screenAwake ? 1 : 0,
                    displayDimmed ? 1 : 0,
                    (unsigned long)ledcRead(DISPLAY_BL_PIN),
                    (unsigned long)ledcReadFreq(DISPLAY_BL_PIN));
#endif
    }
  }

  void sanitizeWifiText(char *value, size_t maxLen, bool allowEmpty) {
    if (value == nullptr || maxLen == 0) return;

    size_t writeIndex = 0;
    for (size_t readIndex = 0; readIndex < maxLen; readIndex++) {
      unsigned char ch = (unsigned char)value[readIndex];
      if (ch == '\0') {
        break;
      }

      if (isprint(ch)) {
        value[writeIndex++] = (char)ch;
      }
    }

    value[writeIndex] = '\0';

    if (!allowEmpty && writeIndex == 0) {
      strncpy(value, OTA_AP_SSID, maxLen);
      value[maxLen] = '\0';
    }
  }

  void sanitizeOtaSettings() {
    sanitizeWifiText(otaStaSsid, OTA_STA_SSID_STORAGE_BYTES, true);
    sanitizeWifiText(otaStaPassword, OTA_STA_PASSWORD_STORAGE_BYTES, true);
    sanitizeWifiText(otaApSsid, OTA_AP_SSID_STORAGE_BYTES, false);
  }

  void loadOtaSettings() {
    if (EEPROM.read(EEPROM_OTA_VERSION_ADDR) != OTA_SETTINGS_STORAGE_VERSION) {
      strncpy(otaStaSsid, OTA_STA_SSID, OTA_STA_SSID_STORAGE_BYTES);
      otaStaSsid[OTA_STA_SSID_STORAGE_BYTES] = '\0';
      strncpy(otaStaPassword, OTA_STA_PASSWORD, OTA_STA_PASSWORD_STORAGE_BYTES);
      otaStaPassword[OTA_STA_PASSWORD_STORAGE_BYTES] = '\0';
      strncpy(otaApSsid, OTA_AP_SSID, OTA_AP_SSID_STORAGE_BYTES);
      otaApSsid[OTA_AP_SSID_STORAGE_BYTES] = '\0';
      sanitizeOtaSettings();
      return;
    }

    int addr = EEPROM_OTA_STA_SSID_ADDR;
    for (int i = 0; i < OTA_STA_SSID_STORAGE_BYTES; i++) {
      otaStaSsid[i] = (char)EEPROM.read(addr++);
    }
    otaStaSsid[OTA_STA_SSID_STORAGE_BYTES] = '\0';

    addr = EEPROM_OTA_STA_PASSWORD_ADDR;
    for (int i = 0; i < OTA_STA_PASSWORD_STORAGE_BYTES; i++) {
      otaStaPassword[i] = (char)EEPROM.read(addr++);
    }
    otaStaPassword[OTA_STA_PASSWORD_STORAGE_BYTES] = '\0';

    addr = EEPROM_OTA_AP_SSID_ADDR;
    for (int i = 0; i < OTA_AP_SSID_STORAGE_BYTES; i++) {
      otaApSsid[i] = (char)EEPROM.read(addr++);
    }
    otaApSsid[OTA_AP_SSID_STORAGE_BYTES] = '\0';

    sanitizeOtaSettings();
  }

  void saveOtaSettings() {
    sanitizeOtaSettings();

    int addr = EEPROM_OTA_STA_SSID_ADDR;
    for (int i = 0; i < OTA_STA_SSID_STORAGE_BYTES; i++) {
      char ch = (i < (int)strlen(otaStaSsid)) ? otaStaSsid[i] : '\0';
      EEPROM.write(addr++, (uint8_t)ch);
    }

    addr = EEPROM_OTA_STA_PASSWORD_ADDR;
    for (int i = 0; i < OTA_STA_PASSWORD_STORAGE_BYTES; i++) {
      char ch = (i < (int)strlen(otaStaPassword)) ? otaStaPassword[i] : '\0';
      EEPROM.write(addr++, (uint8_t)ch);
    }

    addr = EEPROM_OTA_AP_SSID_ADDR;
    for (int i = 0; i < OTA_AP_SSID_STORAGE_BYTES; i++) {
      char ch = (i < (int)strlen(otaApSsid)) ? otaApSsid[i] : '\0';
      EEPROM.write(addr++, (uint8_t)ch);
    }

    EEPROM.write(EEPROM_OTA_VERSION_ADDR, OTA_SETTINGS_STORAGE_VERSION);
    EEPROM.commit();
  }

  void initDefaultMixSlot(int modelIndex, int mixIndex) {
    models[modelIndex].mixes[mixIndex].enabled = false;
    models[modelIndex].mixes[mixIndex].source = 0;
    models[modelIndex].mixes[mixIndex].destination = 0;
    setMixSource(models[modelIndex].mixes[mixIndex], mixIndex % CHANNEL_COUNT);
    setMixDestination(models[modelIndex].mixes[mixIndex], (mixIndex + 1) % CHANNEL_COUNT);
    setMixReverseLinked(models[modelIndex].mixes[mixIndex], true);
    models[modelIndex].mixes[mixIndex].rate = 0;
    models[modelIndex].mixes[mixIndex].offset = 0;
  }

  DriveType getModelDriveType(int modelIndex) {
    uint8_t drive = models[modelIndex].driveType & 0x0F;
    if (drive > DRIVE_X_DRONE) drive = DRIVE_TANK;
    return (DriveType)drive;
  }

  void setModelDriveType(int modelIndex, DriveType driveType) {
    models[modelIndex].driveType =
      (models[modelIndex].driveType & 0xF0) | ((uint8_t)driveType & 0x0F);
  }

  TankControlMode getModelTankMode(int modelIndex) {
    return (models[modelIndex].driveType & 0x10)
      ? TANK_MODE_RIGHT_STICK
      : TANK_MODE_DUAL_STICK;
  }

  void setModelTankMode(int modelIndex, TankControlMode tankMode) {
    if (tankMode == TANK_MODE_RIGHT_STICK) {
      models[modelIndex].driveType |= 0x10;
    } else {
      models[modelIndex].driveType &= ~0x10;
    }
  }

  ProtocolType getModelProtocol(int modelIndex) {
    uint8_t protocol = (models[modelIndex].driveType & PROTOCOL_MASK) >> PROTOCOL_SHIFT;
    if (protocol > PROTOCOL_ESPNOW) protocol = PROTOCOL_ELRS;
    return (ProtocolType)protocol;
  }

  void setModelProtocol(int modelIndex, ProtocolType protocolType) {
    models[modelIndex].driveType =
      (models[modelIndex].driveType & ~PROTOCOL_MASK) |
      ((((uint8_t)protocolType) << PROTOCOL_SHIFT) & PROTOCOL_MASK);
  }

  bool shouldForceTankSingleReversePair(int modelIndex) {
    if (modelIndex < 0 || modelIndex >= MAX_MODELS) return false;
    if (getModelDriveType(modelIndex) != DRIVE_TANK) return false;
    if (getModelTankMode(modelIndex) != TANK_MODE_RIGHT_STICK) return false;

    // In 1-stick tank mode, CH1 and CH2 are tied by default.
    // But if the user created a direct mix between them and explicitly
    // separated reverse linking on that mix, that explicit choice wins.
    for (int mixIndex = 0; mixIndex < MIX_COUNT; mixIndex++) {
      MixData &mix = models[modelIndex].mixes[mixIndex];
      if (!mix.enabled) continue;

      uint8_t source = getMixSource(mix);
      uint8_t destination = getMixDestination(mix);
      bool isTankPairMix =
        ((source == 0 && destination == 1) || (source == 1 && destination == 0));

      if (!isTankPairMix) continue;
      if (!isMixReverseLinked(mix)) return false;
    }

    return true;
  }

  void getLinkedReverseChannels(int modelIndex, int channel, bool linked[CHANNEL_COUNT]) {
    for (int i = 0; i < CHANNEL_COUNT; i++) linked[i] = false;
    if (channel < 0 || channel >= CHANNEL_COUNT) return;

    linked[channel] = true;

    bool changed = true;
    while (changed) {
      changed = false;

      for (int mixIndex = 0; mixIndex < MIX_COUNT; mixIndex++) {
        MixData &mix = models[modelIndex].mixes[mixIndex];
        if (!mix.enabled) continue;
        if (!isMixReverseLinked(mix)) continue;

        uint8_t source = getMixSource(mix);
        uint8_t destination = getMixDestination(mix);
        if (source >= CHANNEL_COUNT || destination >= CHANNEL_COUNT) continue;

        if (linked[source] && !linked[destination]) {
          linked[destination] = true;
          changed = true;
        }

        if (linked[destination] && !linked[source]) {
          linked[source] = true;
          changed = true;
        }
      }
    }

    if (shouldForceTankSingleReversePair(modelIndex) && (channel == 0 || channel == 1)) {
      linked[0] = true;
      linked[1] = true;
    }
  }

  bool hasLinkedReversePeers(int modelIndex, int channel) {
    bool linked[CHANNEL_COUNT];
    getLinkedReverseChannels(modelIndex, channel, linked);

    for (int i = 0; i < CHANNEL_COUNT; i++) {
      if (i != channel && linked[i]) return true;
    }

    return false;
  }

  bool sanitizeModelMixes(int modelIndex) {
    bool changed = false;

    for (int mixIndex = 0; mixIndex < MIX_COUNT; mixIndex++) {
      MixData &mix = models[modelIndex].mixes[mixIndex];
      uint8_t source = getMixSource(mix);
      uint8_t destination = getMixDestination(mix);

      bool invalid =
        ((mix.source & ~(MIX_CHANNEL_MASK | MIX_REVERSE_SEPARATE_FLAG)) != 0) ||
        ((mix.destination & ~MIX_CHANNEL_MASK) != 0) ||
        (source >= CHANNEL_COUNT) ||
        (destination >= CHANNEL_COUNT) ||
        (mix.rate < -100) || (mix.rate > 100) ||
        (mix.offset < -100) || (mix.offset > 100);

      if (invalid) {
        initDefaultMixSlot(modelIndex, mixIndex);
        changed = true;
        continue;
      }

      bool normalizedEnabled = mix.enabled ? true : false;
      if (mix.enabled != normalizedEnabled) {
        mix.enabled = normalizedEnabled;
        changed = true;
      }

      if (destination == source) {
        setMixDestination(mix, (destination + 1) % CHANNEL_COUNT);
        changed = true;
      }
    }

    return changed;
  }

  bool sanitizeModelDriveType(int modelIndex) {
    uint8_t drive = models[modelIndex].driveType & 0x0F;
    if (drive > DRIVE_X_DRONE) {
      models[modelIndex].driveType &= 0xF0;
      setModelDriveType(modelIndex, DRIVE_TANK);
      return true;
    }

    return false;
  }

  bool sanitizeModelProtocol(int modelIndex) {
    uint8_t protocol = (models[modelIndex].driveType & PROTOCOL_MASK) >> PROTOCOL_SHIFT;
    if (protocol > PROTOCOL_ESPNOW) {
      setModelProtocol(modelIndex, PROTOCOL_ELRS);
      return true;
    }

    return false;
  }

  void initModelDefaults(int i) {
    for (int ch = 0; ch < 5; ch++) {
      models[i].reverse[ch] = false;
      models[i].failsafe[ch] = false;
    }

    clearModelFailsafeValues(i);
    clearModelRateValues(i);
    clearModelExpoValues(i);
    clearModelEndpointValues(i);

    for (int g = 0; g < 2; g++) {
      models[i].trimX[g] = 0;
      models[i].trimY[g] = 0;
    }

    for (int mix = 0; mix < MIX_COUNT; mix++) {
      initDefaultMixSlot(i, mix);
    }

    models[i].driveType = 0;
    setModelDriveType(i, DRIVE_TANK);
    setModelTankMode(i, TANK_MODE_DUAL_STICK);

    strcpy(models[i].name, "");
  }

  float leftThrottle  = 0.0;
  float rightThrottle = 0.0;
  float leftY  = 0.0;
  float rightY = 0.0;
  float inputChannels[CHANNEL_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f };
  float outputChannels[CHANNEL_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f };

  float m1 = 0.0;
  float m2 = 0.0;
  float m3 = 0.0;
  float m4 = 0.0;

  float trimRenderX = 0;
  float trimRenderY = 0;

  int signalStrength = 3;
  int batteryLevel   = 75;

  int trimBtnLeftX, trimBtnRightX;
  int trimBtnTopY, trimBtnBottomY;

  // ===== KEYBOARD STATE =====
  bool keyboardActive = false;
  bool keyboardNeedsRedraw = true;
  bool keyboardLowercase = false;

  String keyboardBuffer = "";
  KeyboardTarget keyboardTarget = KEYBOARD_TARGET_MODEL_NAME;

  #define KB_ROWS 5
  #define KB_COLS 10

  int kbX = 0;
  int kbW = 240;
  int kbH = 160;
  int kbY = FOOTER_Y - kbH - 5;   // bottom half of screen
  int keyW = 22;
  int keyH = 30;
  int keySpacing = 2;

  // track pressed key for visual feedback
  int kbPressedRow = -1;
  int kbPressedCol = -1;
  int kbCursorRow = 1;   // start at "Q"
  int kbCursorCol = 0;

  const char* keyboardLayoutUpper[KB_ROWS][KB_COLS] = {
  {"1","2","3","4","5","6","7","8","9","0"},
  {"Q","W","E","R","T","Y","U","I","O","P"},
  {"A","S","D","F","G","H","J","K","L","<"},
  {"Aa","Z","X","C","V","B","N","M","OK",""},
  {" ","","","","","","","","",""}
  };

  const char* keyboardLayoutLower[KB_ROWS][KB_COLS] = {
  {"1","2","3","4","5","6","7","8","9","0"},
  {"q","w","e","r","t","y","u","i","o","p"},
  {"a","s","d","f","g","h","j","k","l","<"},
  {"Aa","z","x","c","v","b","n","m","OK",""},
  {" ","","","","","","","","",""}
  };

  // ===== MODEL NAME =====
  String modelNames[4] = {"", "", "", ""};
  int selectedModelIndex = -1;
  bool modelNameNeedsRedraw = true;
  bool modelNameDirty = true;
  bool inputBoxSelected = false;
  int pendingModelHoldIndex = -1;
  unsigned long pendingModelHoldStart = 0;
  bool pendingModelHoldTriggered = false;
  char otaStaSsid[OTA_STA_SSID_STORAGE_BYTES + 1] = OTA_STA_SSID;
  char otaStaPassword[OTA_STA_PASSWORD_STORAGE_BYTES + 1] = OTA_STA_PASSWORD;
  char otaApSsid[OTA_AP_SSID_STORAGE_BYTES + 1] = OTA_AP_SSID;
  bool otaSettingsNeedsRedraw = true;

const char* getKeyboardKey(int row, int col) {
  if (row < 0 || row >= KB_ROWS || col < 0 || col >= KB_COLS) return "";
  return keyboardLowercase ? keyboardLayoutLower[row][col] : keyboardLayoutUpper[row][col];
}

int getKeyboardMaxLength() {
  if (keyboardTarget == KEYBOARD_TARGET_OTA_STA_SSID) {
    return OTA_STA_SSID_STORAGE_BYTES;
  }
  if (keyboardTarget == KEYBOARD_TARGET_OTA_STA_PASSWORD) {
    return OTA_STA_PASSWORD_STORAGE_BYTES;
  }
  if (keyboardTarget == KEYBOARD_TARGET_OTA_AP_SSID) {
    return OTA_AP_SSID_STORAGE_BYTES;
  }
  return 18;
}

void applyKeyboardBufferToTarget() {
  if (keyboardTarget == KEYBOARD_TARGET_MODEL_NAME) {
    if (keyboardBuffer.length() > 0) {
      saveKeyboardBufferToModelSlot();
    }
    return;
  }

  if (keyboardTarget == KEYBOARD_TARGET_OTA_STA_SSID) {
    strncpy(otaStaSsid, keyboardBuffer.c_str(), OTA_STA_SSID_STORAGE_BYTES);
    otaStaSsid[OTA_STA_SSID_STORAGE_BYTES] = '\0';
  }
  else if (keyboardTarget == KEYBOARD_TARGET_OTA_STA_PASSWORD) {
    strncpy(otaStaPassword, keyboardBuffer.c_str(), OTA_STA_PASSWORD_STORAGE_BYTES);
    otaStaPassword[OTA_STA_PASSWORD_STORAGE_BYTES] = '\0';
  }
  else if (keyboardTarget == KEYBOARD_TARGET_OTA_AP_SSID) {
    strncpy(otaApSsid, keyboardBuffer.c_str(), OTA_AP_SSID_STORAGE_BYTES);
    otaApSsid[OTA_AP_SSID_STORAGE_BYTES] = '\0';
    if (strlen(otaApSsid) == 0) {
      strncpy(otaApSsid, OTA_AP_SSID, OTA_AP_SSID_STORAGE_BYTES);
      otaApSsid[OTA_AP_SSID_STORAGE_BYTES] = '\0';
    }
  }

  saveOtaSettings();
  otaSettingsNeedsRedraw = true;
}

void beginOtaFieldEdit(KeyboardTarget target) {
  keyboardTarget = target;
  if (target == KEYBOARD_TARGET_OTA_STA_SSID) {
    keyboardBuffer = String(otaStaSsid);
  } else if (target == KEYBOARD_TARGET_OTA_STA_PASSWORD) {
    keyboardBuffer = String(otaStaPassword);
  } else if (target == KEYBOARD_TARGET_OTA_AP_SSID) {
    keyboardBuffer = String(otaApSsid);
  } else {
    keyboardBuffer = "";
  }

  keyboardActive = true;
  keyboardLowercase = false;
  keyboardNeedsRedraw = true;
  kbCursorRow = 1;
  kbCursorCol = 0;
  uiNeedsRedraw = true;
  otaSettingsNeedsRedraw = true;
}

void processKeyboardKey(const char* key) {
  if (strlen(key) == 0) return;

  if (strcmp(key, "<") == 0) {
    if (keyboardBuffer.length() > 0) {
      keyboardBuffer.remove(keyboardBuffer.length() - 1);
      modelNameDirty = true;
    }
  }
  else if (strcmp(key, "Aa") == 0) {
    keyboardLowercase = !keyboardLowercase;
    keyboardNeedsRedraw = true;
  }
  else if (strcmp(key, "OK") == 0) {
    applyKeyboardBufferToTarget();
    keyboardBuffer = "";
    keyboardTarget = KEYBOARD_TARGET_MODEL_NAME;
    modelNameDirty = true;
    modelNameNeedsRedraw = true;
    otaSettingsNeedsRedraw = true;
    uiNeedsRedraw = true;
    closeKeyboard();
  }
  else {
    if (keyboardBuffer.length() < getKeyboardMaxLength()) {
      keyboardBuffer += key;
      modelNameDirty = true;
      otaSettingsNeedsRedraw = true;
    }
  }

  keyboardNeedsRedraw = true;
  uiNeedsRedraw = true;
}

void selectModelSlot(int modelIndex) {
  activeModel = modelIndex;
  selectedModelIndex = modelIndex;
  currentModelName = String(models[modelIndex].name);
  currentDrive = getModelDriveType(modelIndex);

  trimRenderX = models[modelIndex].trimX[currentTrimPage];
  trimRenderY = models[modelIndex].trimY[currentTrimPage];
  selectedMixIndex = 0;
  mixingNeedsRedraw = true;

  trimNeedsRedraw = true;
  reverseNeedsRedraw = true;
  failsafeNeedsRedraw = true;
  trimDirty = true;
  modelNameDirty = true;
  modelNameNeedsRedraw = true;

  for (int c = 0; c < CHANNEL_COUNT; c++) {
    reverseChannelDirty[c] = true;
    failsafeDirty[c] = true;
  }

  uiNeedsRedraw = true;
}

void beginModelNameEdit(int modelIndex) {
  selectedModelIndex = modelIndex;
  keyboardBuffer = modelNames[modelIndex];
  keyboardTarget = KEYBOARD_TARGET_MODEL_NAME;
  keyboardActive = true;
  keyboardLowercase = false;
  keyboardNeedsRedraw = true;
  inputBoxSelected = true;
  modelNameDirty = true;
  modelNameNeedsRedraw = true;
  uiNeedsRedraw = true;
}

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

  if (RADIO_PROTOCOL_SERIAL_DEBUG) {
    Serial.printf("ESP-NOW channel set failed (set=%d on=%d off=%d)\n",
                  (int)setChan, (int)promiscOn, (int)promiscOff);
  }
  return false;
}

bool ensureEspNowPeer(const uint8_t *peerAddress) {
  if (!espNowReady || peerAddress == nullptr) return false;
  if (esp_now_is_peer_exist(peerAddress)) return true;

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, ESP_NOW_ETH_ALEN);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);
  if (result != ESP_OK) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.printf("ESP-NOW add peer failed: %d\n", (int)result);
    }
    return false;
  }

  return true;
}

bool initEspNowLink() {
  if (otaModeActive) {
    return false;
  }

  if (espNowReady) {
    return ensureEspNowPeer(espNowBroadcastAddress);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  unsigned long wifiStart = millis();
  while (!WiFi.STA.started() && millis() - wifiStart < 1000) {
    delay(10);
  }

  if (!WiFi.STA.started()) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.println("ESP-NOW STA start timed out");
    }
    return false;
  }

  WiFi.disconnect();
  if (!setEspNowWifiChannel(ESPNOW_WIFI_CHANNEL)) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.println("ESP-NOW channel set failed");
    }
    return false;
  }

  esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.printf("ESP-NOW init failed: %d\n", (int)initResult);
    }
    return false;
  }

  if (esp_now_register_recv_cb(onEspNowReceive) != ESP_OK) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.println("ESP-NOW recv callback registration failed");
    }
    esp_now_deinit();
    return false;
  }

  if (esp_now_register_send_cb(onEspNowSend) != ESP_OK) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.println("ESP-NOW send callback registration failed");
    }
    esp_now_deinit();
    return false;
  }

  espNowReady = true;

  if (RADIO_PROTOCOL_SERIAL_DEBUG) {
    Serial.print("ESP-NOW transmitter ready on channel ");
    Serial.println(ESPNOW_WIFI_CHANNEL);
  }

  return ensureEspNowPeer(espNowBroadcastAddress);
}

void logOtaNetworkState(const char *label) {
  wifi_mode_t wifiMode = WIFI_MODE_NULL;
  esp_wifi_get_mode(&wifiMode);

  IPAddress apIp = WiFi.softAPIP();
  IPAddress staIp = WiFi.localIP();

  Serial.printf(
    "%s mode=%d apIP=%u.%u.%u.%u staIP=%u.%u.%u.%u staStatus=%d channel=%u apSSID='%s'\n",
    (label != nullptr) ? label : "OTA",
    (int)wifiMode,
    apIp[0], apIp[1], apIp[2], apIp[3],
    staIp[0], staIp[1], staIp[2], staIp[3],
    (int)WiFi.status(),
    WiFi.channel(),
    otaApSsid);
}

void logPartitionInfo(const char *label, const esp_partition_t *partition) {
  if (partition == nullptr) {
    Serial.printf("%s <null>\n", (label != nullptr) ? label : "partition");
    return;
  }

  Serial.printf("%s label=%s type=%u subtype=%u addr=0x%08lX size=0x%08lX\n",
                (label != nullptr) ? label : "partition",
                partition->label,
                (unsigned int)partition->type,
                (unsigned int)partition->subtype,
                (unsigned long)partition->address,
                (unsigned long)partition->size);
}

bool isOtaKeyboardTarget(KeyboardTarget target) {
  return target == KEYBOARD_TARGET_OTA_STA_SSID ||
         target == KEYBOARD_TARGET_OTA_STA_PASSWORD ||
         target == KEYBOARD_TARGET_OTA_AP_SSID;
}

const char* getKeyboardTargetLabel(KeyboardTarget target) {
  if (target == KEYBOARD_TARGET_OTA_STA_SSID) return "WiFi SSID";
  if (target == KEYBOARD_TARGET_OTA_STA_PASSWORD) return "Home Pass";
  if (target == KEYBOARD_TARGET_OTA_AP_SSID) return "AP Name";
  return "Text";
}

void startOtaMode() {
  if (otaModeActive || otaUpdateInProgress) return;

  if (espNowBindingMode) {
    cancelEspNowBinding(true);
  }
  if (espNowReady) {
    esp_now_deinit();
    espNowReady = false;
  }
  resetEspNowTelemetry();
  espNowProtocolActive = false;
  espNowProtocolStartTime = 0;

  sanitizeOtaSettings();
  Serial.println("OTA: starting WiFi service");

  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true, false);
  delay(100);
  WiFi.mode(WIFI_MODE_NULL);
  delay(100);

  if (!WiFi.mode(WIFI_AP_STA)) {
    Serial.println("OTA: failed to enter WIFI_AP_STA mode");
    otaModeActive = false;
    otaServiceReady = false;
    otaUsingSoftAP = false;
    otaApReady = false;
    otaStaAutoConnectEnabled = false;
    otaStaAutoConnectAfterMs = 0;
    otaStaConnectPending = false;
    fullRedraw = true;
    uiNeedsRedraw = true;
    return;
  }
  WiFi.setSleep(false);
  WiFi.setHostname(OTA_HOSTNAME);

  otaApReady = WiFi.softAP(otaApSsid, OTA_AP_PASSWORD, 1, false, 1);
  if (!otaApReady) {
    Serial.printf("OTA: softAP start failed for SSID '%s'\n", otaApSsid);
  } else {
    delay(100);
    logOtaNetworkState("OTA: AP started");
  }

  bool staConfigured = false;
  otaStaAutoConnectEnabled = false;
  otaStaAutoConnectAfterMs = 0;
  otaStaConnectPending = false;
  otaStaConnectStartTime = 0;
  if (strlen(otaStaSsid) > 0) {
    size_t passwordLen = strlen(otaStaPassword);
    bool passwordUsable = (passwordLen == 0 || passwordLen >= 8);
    if (!passwordUsable) {
      Serial.printf("OTA: STA password for '%s' is too short (%u), skipping STA connect\n",
                    otaStaSsid, (unsigned int)passwordLen);
    } else {
      staConfigured = true;
      otaStaAutoConnectEnabled = true;
      otaStaAutoConnectAfterMs = millis() + 15000UL;
      Serial.printf("OTA: AP ready, deferring STA connect to '%s' while AP is available\n",
                    otaStaSsid);
    }
  }

  if (!otaApReady && !staConfigured) {
    otaModeActive = false;
    otaServiceReady = false;
    otaUsingSoftAP = false;
    otaApReady = false;
    otaStaAutoConnectEnabled = false;
    otaStaAutoConnectAfterMs = 0;
    otaStaConnectPending = false;
    fullRedraw = true;
    uiNeedsRedraw = true;
    return;
  }

  otaUsingSoftAP = otaApReady;
  otaUpdateInProgress = false;
  otaRestartPending = false;
  otaHttpServer.begin();
  logOtaNetworkState("OTA: HTTP server ready");

  otaModeActive = true;
  otaServiceReady = true;
  fullRedraw = true;
  uiNeedsRedraw = true;
}

void stopOtaMode() {
  if (!otaModeActive) return;
  if (otaUpdateInProgress) return;

  otaHttpServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  otaModeActive = false;
  otaServiceReady = false;
  otaUsingSoftAP = false;
  otaApReady = false;
  otaStaAutoConnectEnabled = false;
  otaStaAutoConnectAfterMs = 0;
  otaStaConnectPending = false;
  otaStaConnectStartTime = 0;
  otaRestartPending = false;

  if (getModelProtocol(activeModel) == PROTOCOL_ESPNOW) {
    initEspNowLink();
  }

  fullRedraw = true;
  uiNeedsRedraw = true;
}

void updateOtaService() {
  if (!otaModeActive || !otaServiceReady) return;

  if (otaStaAutoConnectEnabled &&
      !otaStaConnectPending &&
      WiFi.status() != WL_CONNECTED) {
    bool apHasClients = otaApReady && WiFi.softAPgetStationNum() > 0;
    if (!apHasClients && millis() >= otaStaAutoConnectAfterMs) {
      Serial.printf("OTA: starting deferred STA connect to '%s'\n", otaStaSsid);
      WiFi.begin(otaStaSsid, otaStaPassword);
      otaStaConnectPending = true;
      otaStaConnectStartTime = millis();
      uiNeedsRedraw = true;
      fullRedraw = true;
    }
  }

  if (otaStaConnectPending) {
    wl_status_t staStatus = WiFi.status();
    if (staStatus == WL_CONNECTED) {
      otaStaConnectPending = false;
      otaUsingSoftAP = false;
      logOtaNetworkState("OTA: STA connected");
      uiNeedsRedraw = true;
      fullRedraw = true;
    } else if (millis() - otaStaConnectStartTime >= 8000) {
      otaStaConnectPending = false;
      if (!otaApReady) {
        otaHttpServer.stop();
        otaModeActive = false;
        otaServiceReady = false;
      }
      Serial.printf("OTA: STA connect timed out for '%s' (status=%d)\n",
                    otaStaSsid, (int)staStatus);
      uiNeedsRedraw = true;
      fullRedraw = true;
    }
  }

  WiFiClient client = otaHttpServer.available();
  if (client) {
    unsigned long requestStart = millis();
    while (client.connected() && !client.available() && millis() - requestStart < 1000) {
      delay(1);
    }

    if (client.available()) {
      String requestLine = client.readStringUntil('\r');
      client.readStringUntil('\n');
      requestLine.trim();

      int contentLength = -1;
      while (client.connected()) {
        String header = client.readStringUntil('\n');
        header.trim();
        if (header.length() == 0) break;

        if (header.startsWith("Content-Length:")) {
          String lengthValue = header.substring(strlen("Content-Length:"));
          lengthValue.trim();
          contentLength = lengthValue.toInt();
        }
      }

      if (requestLine.startsWith("GET / ")) {
        const char* page =
          "<!doctype html><html><head><meta charset='utf-8'><title>Anubis OTA</title></head>"
          "<body style='font-family:sans-serif;background:#101418;color:#e8edf2;padding:24px;'>"
          "<h2>Anubis Transmitter OTA</h2>"
          "<p>Select a firmware <code>.bin</code> file and send it as raw binary:</p>"
          "<input type='file' id='fw' accept='.bin'>"
          "<button onclick='up()'>Upload</button>"
          "<pre id='s'></pre>"
          "<script>"
          "async function up(){const f=document.getElementById('fw').files[0];"
          "if(!f){return;}document.getElementById('s').textContent='Uploading...';"
          "const r=await fetch('/update',{method:'POST',headers:{'Content-Type':'application/octet-stream'},body:f});"
          "document.getElementById('s').textContent=await r.text();}</script>"
          "</body></html>";
        client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
        client.print(page);
      } else if (requestLine.startsWith("POST /update")) {
        otaUpdateInProgress = true;
        fullRedraw = true;
        uiNeedsRedraw = true;

        bool updateOk = false;
        if (contentLength > 0) {
          const esp_partition_t* runningPartition = esp_ota_get_running_partition();
          const esp_partition_t* bootPartition = esp_ota_get_boot_partition();
          const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(nullptr);
          Serial.printf("OTA upload len=%d slots=%u\n",
                        contentLength,
                        (unsigned int)esp_ota_get_app_partition_count());
          logPartitionInfo("OTA running", runningPartition);
          logPartitionInfo("OTA boot", bootPartition);
          logPartitionInfo("OTA target", updatePartition);

          esp_ota_handle_t otaHandle = 0;
          esp_err_t beginErr = esp_ota_begin(updatePartition, (size_t)contentLength, &otaHandle);
          if (beginErr != ESP_OK) {
            Serial.printf("OTA begin failed: %d (%s)\n",
                          (int)beginErr,
                          esp_err_to_name(beginErr));
          } else {
          size_t remaining = (size_t)contentLength;
          uint8_t buffer[1024];
          unsigned long lastData = millis();

          while (remaining > 0 && millis() - lastData < 5000) {
            int availableBytes = client.available();
            if (availableBytes <= 0) {
              delay(1);
              continue;
            }

            size_t toRead = (size_t)availableBytes;
            if (toRead > sizeof(buffer)) toRead = sizeof(buffer);
            if (toRead > remaining) toRead = remaining;

            size_t readBytes = client.read(buffer, toRead);
            if (readBytes > 0) {
              lastData = millis();
              remaining -= readBytes;
              esp_err_t writeErr = esp_ota_write(otaHandle, (const void*)buffer, readBytes);
              if (writeErr != ESP_OK) {
                Serial.printf("OTA write failed: %d\n", (int)writeErr);
                break;
              }
            }
          }

          if (remaining == 0) {
            esp_err_t endErr = esp_ota_end(otaHandle);
            if (endErr == ESP_OK) {
              esp_err_t bootErr = esp_ota_set_boot_partition(updatePartition);
              if (bootErr == ESP_OK) {
                updateOk = true;
              } else {
                Serial.printf("OTA set boot partition failed: %d\n", (int)bootErr);
              }
            } else {
              Serial.printf("OTA end failed: %d\n", (int)endErr);
            }
          } else {
            esp_ota_abort(otaHandle);
          }
          }
        } else {
          Serial.println("OTA update rejected: missing content length");
        }

        otaUpdateInProgress = false;
        fullRedraw = true;
        uiNeedsRedraw = true;

        if (updateOk) {
          otaRestartPending = true;
          client.print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nUpdate complete. Rebooting...");
        } else {
          client.print("HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nUpdate failed.");
        }
      } else {
        client.print("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
      }
    }

    delay(1);
    client.stop();
  }

  if (otaRestartPending && !otaUpdateInProgress) {
    delay(250);
    ESP.restart();
  }
}

void resetEspNowTelemetry() {
  espNowLatency = 0;
  telemetryVoltage = 0.0f;
  lastEspNowTelemetryTime = 0;
  lastEspNowAliveTime = 0;
  lastEspNowPingTime = 0;
  espNowPingSequence = 0;
}

void sendEspNowControlPacket(unsigned long now) {
  if (!initEspNowLink()) return;
  if (espNowBindingMode) return;

  EspNowControlPacket packet = {};
  packet.magic = ESPNOW_LINK_MAGIC;
  packet.version = ESPNOW_LINK_VERSION;
  packet.messageType = ESPNOW_MSG_CONTROL;
  packet.sequence = ++espNowSequence;
  packet.txMillis = now;
  packet.modelIndex = (uint8_t)activeModel;
  packet.driveType = (uint8_t)getModelDriveType(activeModel);
  packet.tankMode = (uint8_t)getModelTankMode(activeModel);

  if (batteryPresent) packet.flags |= ESPNOW_FLAG_BATTERY_PRESENT;
  if (batteryCharging) packet.flags |= ESPNOW_FLAG_BATTERY_CHARGING;

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    float constrainedValue = constrain(outputChannels[i], -1.0f, 1.0f);
    packet.channels[i] = (int16_t)roundf(constrainedValue * 1000.0f);
  }

  if (transmitterBatteryVoltage > 0.05f) {
    packet.txBatteryMillivolts =
      (uint16_t)constrain((int)roundf(transmitterBatteryVoltage * 1000.0f), 0, 65535);
  }

  const uint8_t *destination = getBoundReceiverMac(activeModel);
  if (destination == nullptr) {
    destination = espNowBroadcastAddress;
  }

  if (!ensureEspNowPeer(destination)) return;

  esp_err_t sendResult = esp_now_send(destination, (const uint8_t *)&packet, sizeof(packet));
  if (sendResult != ESP_OK) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.printf("ESP-NOW send failed: %d\n", (int)sendResult);
    }
  }
}

void sendEspNowPing(unsigned long now) {
  if (!initEspNowLink()) return;
  if (espNowBindingMode) return;

  const uint8_t *destination = getBoundReceiverMac(activeModel);
  if (destination == nullptr) return;
  if (!ensureEspNowPeer(destination)) return;

  EspNowPingPacket packet = {};
  packet.magic = ESPNOW_LINK_MAGIC;
  packet.version = ESPNOW_LINK_VERSION;
  packet.messageType = ESPNOW_MSG_PING;
  packet.sequence = ++espNowPingSequence;
  packet.txMillis = now;

  esp_err_t sendResult = esp_now_send(destination, (const uint8_t *)&packet, sizeof(packet));
  if (sendResult != ESP_OK) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.printf("ESP-NOW ping send failed: %d\n", (int)sendResult);
    }
  }
}

void sendEspNowBindCommit(const uint8_t *receiverMac) {
  if (!espNowBindingMode || receiverMac == nullptr) return;
  if (!initEspNowLink()) return;
  if (!ensureEspNowPeer(receiverMac)) return;

  EspNowBindPacket packet = {};
  packet.magic = ESPNOW_LINK_MAGIC;
  packet.version = ESPNOW_LINK_VERSION;
  packet.messageType = ESPNOW_MSG_BIND_COMMIT;
  packet.token = espNowBindingToken;

  esp_err_t sendResult = esp_now_send(receiverMac, (const uint8_t *)&packet, sizeof(packet));
  if (sendResult != ESP_OK) {
    if (RADIO_PROTOCOL_SERIAL_DEBUG) {
      Serial.printf("ESP-NOW bind commit failed: %d\n", (int)sendResult);
    }
    return;
  }

  espNowLastBindCommitTime = millis();
}

bool hasEspNowPendingBindMac() {
  for (int i = 0; i < ESP_NOW_ETH_ALEN; i++) {
    if (espNowPendingBindMac[i] != 0) {
      return true;
    }
  }
  return false;
}

void beginEspNowBinding() {
  setModelProtocol(activeModel, PROTOCOL_ESPNOW);
  saveModels();
  initEspNowLink();

  espNowBindingMode = true;
  espNowBindingStartTime = millis();
  espNowBindingToken = ++espNowSequence;
  espNowLastBindCommitTime = 0;
  memset(espNowPendingBindMac, 0, ESP_NOW_ETH_ALEN);
  uiNeedsRedraw = true;
  fullRedraw = true;
}

void cancelEspNowBinding(bool clearPendingMac) {
  espNowBindingMode = false;
  espNowBindingStartTime = 0;
  espNowLastBindCommitTime = 0;
  if (clearPendingMac) {
    memset(espNowPendingBindMac, 0, ESP_NOW_ETH_ALEN);
  }
  uiNeedsRedraw = true;
}

void handleEspNowBindPacket(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (info == nullptr || data == nullptr || len != (int)sizeof(EspNowBindPacket)) return;

  EspNowBindPacket packet = {};
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != ESPNOW_LINK_MAGIC || packet.version != ESPNOW_LINK_VERSION) {
    return;
  }

  if (packet.messageType == ESPNOW_MSG_BIND_BEACON) {
    if (!espNowBindingMode) return;

    memcpy(espNowPendingBindMac, info->src_addr, ESP_NOW_ETH_ALEN);
    sendEspNowBindCommit(info->src_addr);
    return;
  }

  if (packet.messageType == ESPNOW_MSG_BIND_ACK) {
    if (!espNowBindingMode || packet.token != espNowBindingToken) return;

    setBoundReceiver(activeModel, info->src_addr);
    saveReceiverBindings();
    espNowBindSuccessTime = millis();
    cancelEspNowBinding(false);
    fullRedraw = true;
    uiNeedsRedraw = true;
  }
}

void formatReceiverMacShort(const uint8_t *macAddress, char *buffer, size_t bufferSize) {
  if (buffer == nullptr || bufferSize == 0) return;

  if (macAddress == nullptr) {
    snprintf(buffer, bufferSize, "No receiver bound");
    return;
  }

  snprintf(buffer, bufferSize, "Bound %02X:%02X:%02X",
           macAddress[3], macAddress[4], macAddress[5]);
}

void handleEspNowTelemetryPacket(const uint8_t *data, int len) {
  if (len != (int)sizeof(EspNowTelemetryPacket)) return;

  EspNowTelemetryPacket packet = {};
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != ESPNOW_LINK_MAGIC ||
      packet.version != ESPNOW_LINK_VERSION ||
      packet.messageType != ESPNOW_MSG_TELEMETRY) {
    return;
  }

  unsigned long now = millis();
  lastEspNowTelemetryTime = now;
  lastEspNowAliveTime = now;

  if (packet.receiverBatteryMillivolts > 0) {
    telemetryVoltage = packet.receiverBatteryMillivolts / 1000.0f;
  } else {
    telemetryVoltage = 0.0f;
  }
}

void handleEspNowPingAckPacket(const uint8_t *data, int len) {
  if (len != (int)sizeof(EspNowPingAckPacket)) return;

  EspNowPingAckPacket packet = {};
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != ESPNOW_LINK_MAGIC ||
      packet.version != ESPNOW_LINK_VERSION ||
      packet.messageType != ESPNOW_MSG_PING_ACK) {
    return;
  }

  unsigned long now = millis();
  lastEspNowAliveTime = now;

  if (packet.echoedTxMillis > 0) {
    int pingLatency = (int)(now - packet.echoedTxMillis);
    if (pingLatency >= 0 && pingLatency <= 60000) {
      espNowLatency = pingLatency;
    }
  }
}

void updateEspNowLink(unsigned long now) {
  if (otaModeActive) {
    if (espNowProtocolActive) {
      resetEspNowTelemetry();
      espNowProtocolActive = false;
      espNowProtocolStartTime = 0;
    }
    if (espNowBindingMode) {
      cancelEspNowBinding(true);
    }
    return;
  }

  if (getModelProtocol(activeModel) != PROTOCOL_ESPNOW) {
    if (espNowProtocolActive) {
      resetEspNowTelemetry();
      espNowProtocolActive = false;
      espNowProtocolStartTime = 0;
    }
    if (espNowBindingMode) {
      cancelEspNowBinding(true);
    }
    return;
  }

  if (!espNowProtocolActive) {
    espNowProtocolStartTime = now;
  }
  espNowProtocolActive = true;

  if (espNowBindingMode) {
    if (now - espNowBindingStartTime > ESPNOW_BIND_TIMEOUT_MS) {
      cancelEspNowBinding(true);
      fullRedraw = true;
      return;
    }

    if (hasEspNowPendingBindMac() &&
        (espNowLastBindCommitTime == 0 ||
         now - espNowLastBindCommitTime >= ESPNOW_BIND_COMMIT_RETRY_MS)) {
      sendEspNowBindCommit(espNowPendingBindMac);
    }
    return;
  }

  if (now - lastEspNowSendTime >= ESPNOW_SEND_INTERVAL_MS) {
    lastEspNowSendTime = now;
    sendEspNowControlPacket(now);
  }

  if (now - lastEspNowPingTime >= ESPNOW_PING_INTERVAL_MS) {
    lastEspNowPingTime = now;
    sendEspNowPing(now);
  }

}

uint8_t crsfCrc8Poly(const uint8_t *data, size_t length, uint8_t poly) {
  uint8_t crc = 0;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ poly);
      else crc <<= 1;
    }
  }
  return crc;
}

uint8_t crsfCrc8(const uint8_t *data, size_t length) {
  return crsfCrc8Poly(data, length, 0xD5);
}

int channelToCrsf(float value) {
  float constrained = constrain(value, -1.0f, 1.0f);
  int crsfValue = (int)roundf(992.0f + constrained * 819.0f);
  return constrain(crsfValue, 172, 1811);
}

int elrsLqToBars(uint8_t linkQuality) {
  if (linkQuality < 10) return 0;
  if (linkQuality < 30) return 1;
  if (linkQuality < 55) return 2;
  if (linkQuality < 80) return 3;
  return 4;
}

bool hasElrsTxOnlyActivity(unsigned long now) {
  return elrsProtocolActive &&
    lastElrsTxTime > 0 &&
    (now - lastElrsTxTime) <= ELRS_TX_ONLY_ACTIVE_WINDOW_MS;
}

bool hasElrsModuleFrames() {
  return
    (elrsDeviceInfoCount > 0) ||
    (elrsType14Count > 0) ||
    (elrsType1DCount > 0) ||
    (elrsType29Count > 0);
}

bool containsIgnoreCase(const char *haystack, const char *needle) {
  if (haystack == nullptr || needle == nullptr) return false;
  size_t needleLen = strlen(needle);
  if (needleLen == 0) return true;
  size_t haystackLen = strlen(haystack);
  if (haystackLen < needleLen) return false;

  for (size_t i = 0; i <= haystackLen - needleLen; i++) {
    bool match = true;
    for (size_t j = 0; j < needleLen; j++) {
      char a = (char)tolower((unsigned char)haystack[i + j]);
      char b = (char)tolower((unsigned char)needle[j]);
      if (a != b) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

void IRAM_ATTR onElrsRxEdge() {
  elrsRxEdgeCount++;
}

void elrsSendFrame(const uint8_t *frame, size_t len) {
  if (frame == nullptr || len == 0) return;
#if ELRS_HALF_DUPLEX_MODE
  pinMode(elrsActiveTxPin, OUTPUT);
#endif
  elrsSerial.write(frame, len);
#if ELRS_HALF_DUPLEX_MODE
  // Ensure the outgoing frame has fully shifted out, then release the line
  // so the module can reply in the remaining slot.
  elrsSerial.flush();
  pinMode(elrsActiveTxPin, INPUT_PULLUP);
#endif
}

void sendElrsChannelFrame() {
  uint16_t channels[16];
  for (int i = 0; i < 16; i++) channels[i] = 992;

  channels[0] = channelToCrsf(outputChannels[0]);  // CH1 RX
  channels[1] = channelToCrsf(outputChannels[1]);  // CH2 RY
  channels[2] = channelToCrsf(outputChannels[2]);  // CH3 LY
  channels[3] = channelToCrsf(outputChannels[3]);  // CH4 LX

  uint8_t payload[22] = {0};
  uint16_t bitPos = 0;
  for (int ch = 0; ch < 16; ch++) {
    uint16_t value = channels[ch] & 0x07FF;
    for (uint8_t bit = 0; bit < 11; bit++) {
      if (value & (1U << bit)) {
        uint16_t pos = bitPos + bit;
        payload[pos >> 3] |= (1U << (pos & 0x07));
      }
    }
    bitPos += 11;
  }

  // CRSF packet format: [dest/sync][len][type][payload...][crc]
  uint8_t frame[26];
  // CRSF serial sync/address byte.
  frame[0] = ELRS_TX_SYNC_PRIMARY;
  frame[1] = 0x18;  // len = type(1) + payload(22) + crc(1)
  frame[2] = 0x16;  // RC_CHANNELS_PACKED
  memcpy(&frame[3], payload, sizeof(payload));
  frame[25] = crsfCrc8(&frame[2], 1 + sizeof(payload));

  elrsSendFrame(frame, sizeof(frame));
  if (ELRS_TX_SYNC_PRIMARY != 0xC8) {
    frame[0] = 0xC8;
    elrsSendFrame(frame, sizeof(frame));
  }
}

void sendElrsDevicePing() {
  // Broadcast and direct destination, with both common handset/Lua source IDs.
  const uint8_t destinations[] = {0xEE, 0x00};
  const uint8_t sources[] = {0xEA, 0xEF};
  for (size_t d = 0; d < sizeof(destinations); d++) {
    for (size_t s = 0; s < sizeof(sources); s++) {
      uint8_t frame[6];
      frame[0] = ELRS_TX_SYNC_PRIMARY;
      frame[1] = 0x04;
      frame[2] = 0x28;
      frame[3] = destinations[d];
      frame[4] = sources[s];
      frame[5] = crsfCrc8(&frame[2], 3);
      elrsSendFrame(frame, sizeof(frame));
      if (ELRS_TX_SYNC_PRIMARY != 0xC8) {
        frame[0] = 0xC8;
        elrsSendFrame(frame, sizeof(frame));
      }
    }
  }
}

void sendElrsBindCommand() {
  // Start a short bind-command burst to improve compatibility with modules
  // that expect repeated bind pulses (similar to Lua bind behavior).
  sendElrsBindCommandTo(0xEE); // TX module destination
  sendElrsBindCommandTo(0xEC); // RX destination (forwarded over RF by module)
  sendElrsLuaBindWrite(elrsBindFieldKnown ? elrsBindFieldIndex : 0x11);
  elrsBindCommandSentTime = millis();
  elrsBindAwaitingResult = true;
  elrsBindAwaitUntil = elrsBindCommandSentTime + 10000UL;
  elrsBindBurstUntil = elrsBindCommandSentTime + 3000UL;
  lastElrsBindBurstSendTime = elrsBindCommandSentTime;
}

void sendElrsBindCommandTo(uint8_t destination) {
  // Send both common command encodings for compatibility:
  // 1) with command CRC8-BA (newer spec-compliant parsers)
  // 2) without command CRC8-BA (legacy/transitional parsers)
  const uint8_t sourceCandidates[] = {0xEA, 0xEF};
  for (size_t s = 0; s < sizeof(sourceCandidates); s++) {
    uint8_t src = sourceCandidates[s];

    // Variant A: includes command CRC8-BA.
    uint8_t frameWithCmdCrc[9];
    frameWithCmdCrc[0] = ELRS_TX_SYNC_PRIMARY;
    frameWithCmdCrc[1] = 0x07;
    frameWithCmdCrc[2] = 0x32;
    frameWithCmdCrc[3] = destination;
    frameWithCmdCrc[4] = src;
    frameWithCmdCrc[5] = 0x10;
    frameWithCmdCrc[6] = 0x01;
    frameWithCmdCrc[7] = crsfCrc8Poly(&frameWithCmdCrc[2], 5, 0xBA);
    frameWithCmdCrc[8] = crsfCrc8(&frameWithCmdCrc[2], 6);
    elrsSendFrame(frameWithCmdCrc, sizeof(frameWithCmdCrc));
    if (ELRS_TX_SYNC_PRIMARY != 0xC8) {
      frameWithCmdCrc[0] = 0xC8;
      elrsSendFrame(frameWithCmdCrc, sizeof(frameWithCmdCrc));
    }

    // Variant B: no command CRC8-BA.
    uint8_t frameNoCmdCrc[8];
    frameNoCmdCrc[0] = ELRS_TX_SYNC_PRIMARY;
    frameNoCmdCrc[1] = 0x06;
    frameNoCmdCrc[2] = 0x32;
    frameNoCmdCrc[3] = destination;
    frameNoCmdCrc[4] = src;
    frameNoCmdCrc[5] = 0x10;
    frameNoCmdCrc[6] = 0x01;
    frameNoCmdCrc[7] = crsfCrc8(&frameNoCmdCrc[2], 5);
    elrsSendFrame(frameNoCmdCrc, sizeof(frameNoCmdCrc));
    if (ELRS_TX_SYNC_PRIMARY != 0xC8) {
      frameNoCmdCrc[0] = 0xC8;
      elrsSendFrame(frameNoCmdCrc, sizeof(frameNoCmdCrc));
    }
  }
}

void sendElrsLuaBindWrite(uint8_t fieldIndex) {
  // Mirrors the common ELRS Lua bind write:
  // C8 06 2D EE EF [field] 01 CRC
  uint8_t frame[8];
  frame[0] = ELRS_TX_SYNC_PRIMARY;
  frame[1] = 0x06;
  frame[2] = 0x2D;
  frame[3] = 0xEE;
  frame[4] = 0xEF;
  frame[5] = fieldIndex;
  frame[6] = 0x01;
  frame[7] = crsfCrc8(&frame[2], 5);
  elrsSendFrame(frame, sizeof(frame));
  if (ELRS_TX_SYNC_PRIMARY != 0xC8) {
    frame[0] = 0xC8;
    elrsSendFrame(frame, sizeof(frame));
  }
}

void sendElrsParameterRead(uint8_t fieldIndex, uint8_t chunkIndex) {
  uint8_t frame[8];
  frame[0] = ELRS_TX_SYNC_PRIMARY;
  frame[1] = 0x06;
  frame[2] = 0x2C;
  frame[3] = 0xEE;
  frame[4] = 0xEF;
  frame[5] = fieldIndex;
  frame[6] = chunkIndex;
  frame[7] = crsfCrc8(&frame[2], 5);
  elrsSendFrame(frame, sizeof(frame));
  if (ELRS_TX_SYNC_PRIMARY != 0xC8) {
    frame[0] = 0xC8;
    elrsSendFrame(frame, sizeof(frame));
  }
}

void updateElrsParameterDiscovery(unsigned long now) {
  if (elrsBindFieldKnown) return;
  if (now - lastElrsParamReadTime < ELRS_PARAM_READ_INTERVAL_MS) return;
  lastElrsParamReadTime = now;

  uint8_t maxIndex = (elrsParameterCount > 0) ? elrsParameterCount : ELRS_PARAM_SCAN_FALLBACK_MAX;
  if (maxIndex == 0) return;

  if (elrsParamScanIndex == 0 || elrsParamScanIndex > maxIndex) {
    elrsParamScanIndex = 1;
  }

  sendElrsParameterRead(elrsParamScanIndex, 0);
  elrsParamScanIndex++;
}

void parseElrsDeviceInfoFrame(const uint8_t *frame, uint8_t expected) {
  if (frame == nullptr || expected < 12) return;

  int payloadStart = 5;
  int payloadEnd = expected - 1; // before CRC
  int i = payloadStart;
  int out = 0;
  while (i < payloadEnd && out < (int)sizeof(elrsDeviceName) - 1) {
    char c = (char)frame[i++];
    if (c == '\0') break;
    if ((uint8_t)c < 0x20 || (uint8_t)c > 0x7E) {
      out = 0;
      break;
    }
    elrsDeviceName[out++] = c;
  }
  elrsDeviceName[out] = '\0';
  elrsDeviceInfoCount++;
  lastElrsDeviceInfoTime = millis();

  // Device info trailing fields:
  // serial(4), hw(4), sw(4), param_count(1), param_version(1)
  if (i + 14 <= payloadEnd) {
    i += 12; // skip serial/hw/sw
    elrsParameterCount = frame[i++];
    elrsParameterVersion = frame[i++];
    if (elrsParameterCount > 0 && elrsParamScanIndex == 0) {
      elrsParamScanIndex = 1;
    }
  }
}

void parseElrsParameterEntryFrame(const uint8_t *frame, uint8_t expected) {
  if (frame == nullptr || expected < 12) return;
  if (frame[3] != 0xEA || frame[4] != 0xEE) return; // handset <- tx module

  uint8_t fieldIndex = frame[5];
  uint8_t typeHidden = frame[8];
  uint8_t fieldType = (uint8_t)(typeHidden & 0x7F);
  if (fieldType != 0x0D) return; // command-type parameters only

  int labelStart = 9;
  int payloadEnd = expected - 1; // before CRC
  if (labelStart >= payloadEnd) return;

  char label[32];
  int out = 0;
  for (int i = labelStart; i < payloadEnd && out < (int)sizeof(label) - 1; i++) {
    char c = (char)frame[i];
    if (c == '\0') break;
    if ((uint8_t)c < 0x20 || (uint8_t)c > 0x7E) break;
    label[out++] = c;
  }
  label[out] = '\0';
  if (out == 0) return;

  if (containsIgnoreCase(label, "bind")) {
    elrsBindFieldIndex = fieldIndex;
    elrsBindFieldKnown = true;
  }
}

void readElrsSerial(unsigned long now) {
  static uint8_t frame[64];
  static uint8_t index = 0;
  static uint8_t expected = 0;

  uint16_t bytesProcessed = 0;
  while (elrsSerial.available() > 0 && bytesProcessed < ELRS_RX_READ_BUDGET) {
    uint8_t byteIn = (uint8_t)elrsSerial.read();
    bytesProcessed++;
    elrsRxByteCount++;
    lastElrsSerialRxTime = now;
    elrsRecentBytes[elrsRecentByteIndex] = byteIn;
    elrsRecentByteIndex = (uint8_t)((elrsRecentByteIndex + 1U) & 0x0F);
    if (byteIn == 0xC8) elrsSyncC8Count++;
    else if (byteIn == 0xEE) elrsSyncEECount++;
    else if (byteIn == 0xEA) elrsSyncEACount++;
    else if (byteIn == 0xEC) elrsSyncECCount++;
    if (byteIn >= 0x20 && byteIn <= 0x7E) elrsPrintableByteCount++;

    if (index == 0) {
      // Common CRSF device/sync bytes for robust framing.
      if (byteIn != 0xC8 && byteIn != 0xEE && byteIn != 0xEA && byteIn != 0xEC) {
        continue;
      }
      frame[index++] = byteIn;
      continue;
    }

    if (index == 1) {
      if (byteIn < 2 || byteIn > 62) {
        index = 0;
        expected = 0;
        continue;
      }
      frame[index++] = byteIn;
      expected = byteIn + 2;
      continue;
    }

    frame[index++] = byteIn;
    if (expected == 0 || index < expected) continue;

    uint8_t len = frame[1];
    uint8_t type = frame[2];
    uint8_t crc = frame[expected - 1];
    uint8_t calculated = crsfCrc8(&frame[2], len - 1);
    if (crc == calculated) {
      elrsRxFrameCount++;
      if (type == 0x16) {
        elrsType16Count++;
      } else if (type == 0x14) {
        elrsType14Count++;
      } else if (type == 0x1C) {
        elrsType1CCount++;
      } else if (type == 0x1D) {
        elrsType1DCount++;
      } else if (type == 0x29) {
        elrsType29Count++;
      } else if (type == 0x2B) {
        // Parameter settings entry (response to 0x2C reads)
      } else {
        elrsTypeOtherCount++;
      }

      if ((type == 0x14 || type == 0x1D) && len >= 12) {
        // Link statistics frame.
        elrsUplinkRssi1 = frame[3];
        elrsUplinkRssi2 = frame[4];
        elrsUplinkLq = frame[5];
        elrsUplinkSnr = (int8_t)frame[6];
        lastElrsLinkStatsTime = now;
      } else if (type == 0x29) {
        parseElrsDeviceInfoFrame(frame, expected);
      } else if (type == 0x2B) {
        parseElrsParameterEntryFrame(frame, expected);
      }
    }

    index = 0;
    expected = 0;
  }
}

void initElrsUart() {
  #if ELRS_FORCE_PIN_PAIR_INDEX >= 0
  elrsPinPairIndex = ELRS_FORCE_PIN_PAIR_INDEX % elrsPinPairCount;
  #endif
  #if ELRS_FORCE_INVERT_MODE >= 0
  elrsInvertModeIndex = ELRS_FORCE_INVERT_MODE % elrsInvertModeCount;
  #endif
  applyElrsUartPinPair(elrsPinPairIndex);
  elrsBindFieldKnown = false;
  elrsBindFieldIndex = 0;
  elrsParameterCount = 0;
  elrsParameterVersion = 0;
  elrsParamScanIndex = 1;
  lastElrsParamReadTime = 0;
  elrsRxInvert = elrsInvertModes[elrsInvertModeIndex][0];
  elrsTxInvert = elrsInvertModes[elrsInvertModeIndex][1];
  restartElrsUart(elrsBaudCandidates[elrsBaudIndex]);
  elrsInitialized = true;
  if (RADIO_PROTOCOL_SERIAL_DEBUG) {
    Serial.printf("ELRS UART mode: %s tx=%d rx=%d\n",
                  ELRS_HALF_DUPLEX_MODE ? "half-duplex" : "full-duplex",
                  (int)elrsActiveTxPin,
                  (int)elrsActiveRxPin);
  }
}

void applyElrsUartPinPair(int pinPairIndex) {
  int wrapped = pinPairIndex % elrsPinPairCount;
  if (wrapped < 0) wrapped += elrsPinPairCount;
  uint8_t newTx = elrsPinPairs[wrapped][0];
  uint8_t newRx = elrsPinPairs[wrapped][1];

  if (elrsRxInterruptAttached) {
    detachInterrupt(digitalPinToInterrupt(elrsActiveRxPin));
    elrsRxInterruptAttached = false;
  }

  elrsActiveTxPin = newTx;
  elrsActiveRxPin = newRx;

  pinMode(elrsActiveRxPin, INPUT_PULLUP);
  if (ELRS_VERBOSE_SERIAL_DEBUG) {
    attachInterrupt(digitalPinToInterrupt(elrsActiveRxPin), onElrsRxEdge, CHANGE);
    elrsRxInterruptAttached = true;
  }

}

void restartElrsUart(uint32_t baud) {
  elrsSerial.end();
  elrsSerial.begin(baud, SERIAL_8N1, elrsActiveRxPin, elrsActiveTxPin, false);
  elrsSerial.setMode(UART_MODE_UART);
  elrsSerial.setRxInvert(elrsRxInvert);
  elrsSerial.setTxInvert(elrsTxInvert);
  elrsActiveBaud = baud;
}

void advanceElrsSerialMode() {
  elrsInvertModeIndex++;
  if (elrsInvertModeIndex >= elrsInvertModeCount) {
    elrsInvertModeIndex = 0;
    elrsBaudIndex++;
    if (elrsBaudIndex >= elrsBaudCandidateCount) {
      elrsBaudIndex = 0;
      #if ELRS_FORCE_PIN_PAIR_INDEX < 0
      elrsPinPairIndex = (elrsPinPairIndex + 1) % elrsPinPairCount;
      applyElrsUartPinPair(elrsPinPairIndex);
      #endif
    }
  }
  elrsRxInvert = elrsInvertModes[elrsInvertModeIndex][0];
  elrsTxInvert = elrsInvertModes[elrsInvertModeIndex][1];
  restartElrsUart(elrsBaudCandidates[elrsBaudIndex]);
}

void updateElrsLink(unsigned long now) {
  if (!elrsInitialized) return;

  readElrsSerial(now);

  elrsModulePresent = (lastElrsSerialRxTime > 0) &&
    ((now - lastElrsSerialRxTime) <= ELRS_MODULE_TIMEOUT_MS);
  elrsLinkActive = (lastElrsLinkStatsTime > 0) &&
    ((now - lastElrsLinkStatsTime) <= ELRS_LINK_TIMEOUT_MS);
  if (elrsBindAwaitingResult) {
    if (elrsLinkActive) {
      elrsBindSuccessTime = now;
      elrsBindAwaitingResult = false;
    } else if (now > elrsBindAwaitUntil) {
      elrsBindAwaitingResult = false;
    }
  }
  if (elrsBindBurstUntil > now && (now - lastElrsBindBurstSendTime >= 200UL)) {
    lastElrsBindBurstSendTime = now;
    sendElrsBindCommandTo(0xEE);
    sendElrsBindCommandTo(0xEC);
    sendElrsLuaBindWrite(elrsBindFieldKnown ? elrsBindFieldIndex : 0x11);
  }

  if (getModelProtocol(activeModel) != PROTOCOL_ELRS) {
    elrsProtocolActive = false;
    return;
  }

  elrsProtocolActive = true;

  bool haveModuleFrames = hasElrsModuleFrames();
  bool allowSerialModeRetry = true;
#if ELRS_HALF_DUPLEX_MODE
  // Single-wire mode is sensitive to UART restarts; hopping baud/invert states
  // here can wedge the link, so keep it on the known-stable mode.
  allowSerialModeRetry = false;
#endif
  if (allowSerialModeRetry &&
      !haveModuleFrames &&
      (now - lastElrsBaudRetryTime >= ELRS_BAUD_RETRY_MS)) {
    lastElrsBaudRetryTime = now;
    advanceElrsSerialMode();
  }

  bool onElrsControlScreen = (currentScreen == SCREEN_PROTOCOL || currentScreen == SCREEN_ELRS);
  unsigned long txIntervalMs = onElrsControlScreen ? 200UL : ELRS_CRSF_TX_INTERVAL_MS;
  bool probeListenOnly = ELRS_PROBE_UNTIL_MODULE_FRAMES && !haveModuleFrames && !elrsLinkActive;
#if ELRS_RX_ONLY_DIAGNOSTIC
  probeListenOnly = true;
#endif
  if (now - lastElrsTxTime >= txIntervalMs) {
    lastElrsTxTime = now;
    // Keep channel traffic minimal while on ELRS control screens so module
    // can respond to ping/parameter/bind frames on a busy single-wire bus.
    if (!probeListenOnly && (!onElrsControlScreen || elrsLinkActive)) {
      sendElrsChannelFrame();
    }
  }
  if (now - lastElrsDevicePingTime >= ELRS_DEVICE_PING_INTERVAL_MS) {
    lastElrsDevicePingTime = now;
    if (!probeListenOnly) {
      sendElrsDevicePing();
    }
  }
  if (!probeListenOnly) {
    updateElrsParameterDiscovery(now);
  }

  if (elrsLinkActive) {
    signalStrength = elrsLqToBars(elrsUplinkLq);
  } else if (hasElrsTxOnlyActivity(now)) {
    signalStrength = 1;
  } else {
    signalStrength = 0;
  }

  static unsigned long lastElrsDebugPrintTime = 0;
  if (RADIO_PROTOCOL_SERIAL_DEBUG && now - lastElrsDebugPrintTime >= 1000) {
    lastElrsDebugPrintTime = now;
    Serial.printf("ELRS baud=%lu pins(tx=%d,rx=%d,pair=%d) inv(rx=%d,tx=%d) tx=%s rxBytes=%lu rxFrames=%lu devInfo=%lu t14=%lu t29=%lu bindIdx=%u(%d) pCnt=%u name=%s lq=%u\n",
                  (unsigned long)elrsActiveBaud,
                  (int)elrsActiveTxPin,
                  (int)elrsActiveRxPin,
                  elrsPinPairIndex,
                  (int)elrsRxInvert,
                  (int)elrsTxInvert,
                  hasElrsTxOnlyActivity(now) ? "on" : "off",
                  (unsigned long)elrsRxByteCount,
                  (unsigned long)elrsRxFrameCount,
                  (unsigned long)elrsDeviceInfoCount,
                  (unsigned long)elrsType14Count,
                  (unsigned long)elrsType29Count,
                  (unsigned int)elrsBindFieldIndex,
                  (int)elrsBindFieldKnown,
                  (unsigned int)elrsParameterCount,
                  (elrsDeviceName[0] != '\0') ? elrsDeviceName : "-",
                  (unsigned int)elrsUplinkLq);
    if (ELRS_VERBOSE_SERIAL_DEBUG) {
      Serial.printf("ELRS pins tx=%d rx=%d modeSlot=%d\n",
                    (int)elrsActiveTxPin,
                    (int)elrsActiveRxPin,
                    elrsPinPairIndex);
      if (elrsRxFrameCount == 0) {
        Serial.printf("ELRS raw sig C8=%lu EE=%lu EA=%lu EC=%lu printable=%lu/%lu sample=",
                      (unsigned long)elrsSyncC8Count,
                      (unsigned long)elrsSyncEECount,
                      (unsigned long)elrsSyncEACount,
                      (unsigned long)elrsSyncECCount,
                      (unsigned long)elrsPrintableByteCount,
                      (unsigned long)elrsRxByteCount);
        uint8_t start = elrsRecentByteIndex;
        for (int i = 0; i < 16; i++) {
          uint8_t b = elrsRecentBytes[(start + i) & 0x0F];
          Serial.printf("%02X", b);
          if (i != 15) Serial.print(' ');
        }
        Serial.println();
      }
      Serial.printf("ELRS types 16=%lu 14=%lu 1C=%lu 1D=%lu 29=%lu other=%lu\n",
                    (unsigned long)elrsType16Count,
                    (unsigned long)elrsType14Count,
                    (unsigned long)elrsType1CCount,
                    (unsigned long)elrsType1DCount,
                    (unsigned long)elrsType29Count,
                    (unsigned long)elrsTypeOtherCount);
      Serial.printf("ELRS rxPin=%d edges=%lu\n",
                    elrsActiveRxPin,
                    (unsigned long)elrsRxEdgeCount);
    }
  }
}

bool hasEspNowSignal(unsigned long now) {
  return lastEspNowTelemetryTime > 0 &&
    (now - lastEspNowTelemetryTime) <= ESPNOW_TELEMETRY_TIMEOUT_MS;
}

bool hasEspNowHeaderSignal(unsigned long now) {
  if (lastEspNowAliveTime > 0) {
    return (now - lastEspNowAliveTime) <= ESPNOW_HEADER_SIGNAL_TIMEOUT_MS;
  }

  if (espNowProtocolStartTime == 0) return false;
  return (now - espNowProtocolStartTime) <= ESPNOW_HEADER_SIGNAL_TIMEOUT_MS;
}

uint16_t getLatencyIndicatorColor(int latencyMs) {
  if (latencyMs <= 40) return COLOR_SIG;
  if (latencyMs <= 100) return COLOR_ACCENT_HI;
  return TFT_RED;
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
void onEspNowSend(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
  (void)tx_info;
#else
void onEspNowSend(const uint8_t *macAddr, esp_now_send_status_t status) {
  (void)macAddr;
#endif
  lastEspNowSendStatus = status;
}

void onEspNowReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (info == nullptr || data == nullptr || len <= 0) return;
  handleEspNowBindPacket(info, data, len);
  handleEspNowTelemetryPacket(data, len);
  handleEspNowPingAckPacket(data, len);
}

void setup() {
  Serial.begin(115200);

  SPI.begin(12, 13, 11, 10);

  pinMode(45, OUTPUT);
  pinMode(DISPLAY_BL_PIN, OUTPUT);
  analogWriteResolution(DISPLAY_BL_PIN, DISPLAY_BL_PWM_BITS);
  analogWriteFrequency(DISPLAY_BL_PIN, DISPLAY_BL_PWM_FREQ);
  analogWrite(DISPLAY_BL_PIN, DEFAULT_DISPLAY_BRIGHTNESS);
  digitalWrite(DISPLAY_BL_PIN, TFT_BACKLIGHT_ON);
  pinMode(BATTERY_ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);

  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(true);
    
  Wire.begin(16, 15);
  touchPanel.begin();
  initAds1115();
  lastActivityTime = millis();
  splashStartTime = millis();

  EEPROM.begin(EEPROM_SIZE);

  bool displaySettingsRepaired = loadDisplaySettings();
  applyThemePalette();
  loadModels();
  loadFailsafeValues();
  loadRateValues();
  loadExpoValues();
  loadEndpointValues();
  loadOtaSettings();
  loadReceiverBindings();
  applyDisplayBacklight();
  if (displaySettingsRepaired) {
    saveDisplaySettings();
  }

  // FIRST BOOT CHECK
  bool initializedAnyModel = false;
  bool repairedAnyMix = false;
  bool resetMixSchema = (EEPROM.read(EEPROM_MIX_VERSION_ADDR) != MIX_STORAGE_VERSION);
  bool resetFailsafeSchema = (EEPROM.read(EEPROM_FAILSAFE_VERSION_ADDR) != FAILSAFE_STORAGE_VERSION);
  bool resetRateSchema = (EEPROM.read(EEPROM_RATE_VERSION_ADDR) != RATE_STORAGE_VERSION);
  bool resetExpoSchema = (EEPROM.read(EEPROM_EXPO_VERSION_ADDR) != EXPO_STORAGE_VERSION);
  bool resetEndpointSchema = (EEPROM.read(EEPROM_ENDPOINT_VERSION_ADDR) != ENDPOINT_STORAGE_VERSION);

  if (resetFailsafeSchema) {
    for (int i = 0; i < MAX_MODELS; i++) {
      clearModelFailsafeValues(i);
    }
  }
  if (resetRateSchema) {
    for (int i = 0; i < MAX_MODELS; i++) {
      clearModelRateValues(i);
    }
  }
  if (resetExpoSchema) {
    for (int i = 0; i < MAX_MODELS; i++) {
      clearModelExpoValues(i);
    }
  }
  if (resetEndpointSchema) {
    for (int i = 0; i < MAX_MODELS; i++) {
      clearModelEndpointValues(i);
    }
  }

  for (int i = 0; i < MAX_MODELS; i++) {
    if (modelSlotUninitialized(i)) {
      initModelDefaults(i);
      initializedAnyModel = true;
    }
    else {
      models[i].name[19] = '\0';
    }

    if (resetMixSchema) {
      for (int mix = 0; mix < MIX_COUNT; mix++) {
        initDefaultMixSlot(i, mix);
      }
      repairedAnyMix = true;
      continue;
    }

    if (sanitizeModelMixes(i)) {
      repairedAnyMix = true;
    }

    if (sanitizeModelDriveType(i)) {
      repairedAnyMix = true;
    }

    if (sanitizeModelProtocol(i)) {
      repairedAnyMix = true;
    }
  }

  if (strlen(models[0].name) == 0) {
    strncpy(models[0].name, "Anubis", sizeof(models[0].name) - 1);
    models[0].name[sizeof(models[0].name) - 1] = '\0';
    initializedAnyModel = true;
  }

  if (initializedAnyModel || repairedAnyMix) {
    saveModels();
  }
  if (initializedAnyModel || resetFailsafeSchema) {
    saveFailsafeValues();
  }
  if (initializedAnyModel || resetRateSchema) {
    saveRateValues();
  }
  if (initializedAnyModel || resetExpoSchema) {
    saveExpoValues();
  }
  if (initializedAnyModel || resetEndpointSchema) {
    saveEndpointValues();
  }
  if (EEPROM.read(EEPROM_OTA_VERSION_ADDR) != OTA_SETTINGS_STORAGE_VERSION) {
    saveOtaSettings();
  }
  if (EEPROM.read(EEPROM_DISPLAY_VERSION_ADDR) != DISPLAY_SETTINGS_STORAGE_VERSION) {
    saveDisplaySettings();
  }

  // sync names
  for (int i = 0; i < 4; i++) {
    modelNames[i] = String(models[i].name);
  }

  // SET ACTIVE MODEL
  activeModel = 0;
  currentModelName = String(models[0].name);
  currentDrive = getModelDriveType(0);

  // optional polish
  trimRenderX = models[activeModel].trimX[currentTrimPage];
  trimRenderY = models[activeModel].trimY[currentTrimPage];
  initElrsUart();
  initEspNowLink();
  }

void loop() {
  unsigned long now = millis();
    
  if (currentScreen != lastScreen) {

    fullRedraw = true;
    uiNeedsRedraw = true;

    lastScreen = currentScreen;
  }

  static int rawX = 0;
  static int rawY = 0;
  static uint8_t touchCount = 0;
  static unsigned long lastTouchPollTime = 0;

  if (now - lastTouchPollTime >= TOUCH_POLL_INTERVAL_MS) {
    touchCount = touchPanel.read_touch_number();

    if (touchCount > 0 && touchCount <= 2) {
      rawX = touchPanel.read_touch1_x();
      rawY = touchPanel.read_touch1_y();
    }
    else {
      touchCount = 0;
    }

    lastTouchPollTime = now;
  }

  bool select = digitalRead(2) == LOW;
  bool down  = digitalRead(3) == LOW;
  bool left  = digitalRead(14) == LOW;
  bool right = digitalRead(21) == LOW;

  bool userActive = false;

  if (currentScreen == SCREEN_SPACE_GAME) {
    updateSpaceGame(now, left, right, select, down);
    if (left || right || select || down) {
      userActive = true;
    }
  }

  updateStickInputs(now);
  updateBatteryState();
  updateOtaService();
  updateEspNowLink(now);
  updateElrsLink(now);

  static int lastProtocolBindSeconds = -1;
  static bool lastProtocolBindingMode = false;
  static bool lastProtocolShowSuccess = false;
  static unsigned long lastProtocolAutoRefreshTime = 0;

  if (currentScreen == SCREEN_PROTOCOL) {
    ProtocolType protocol = getModelProtocol(activeModel);
    int bindSeconds = -1;
    if (espNowBindingMode) {
      unsigned long remainingMs = 0;
      if (now - espNowBindingStartTime < ESPNOW_BIND_TIMEOUT_MS) {
        remainingMs = ESPNOW_BIND_TIMEOUT_MS - (now - espNowBindingStartTime);
      }
      bindSeconds = (int)((remainingMs + 999) / 1000);
    }

    bool showSuccess = (!espNowBindingMode && (now - espNowBindSuccessTime < 2500));

    if (bindSeconds != lastProtocolBindSeconds ||
        espNowBindingMode != lastProtocolBindingMode ||
        showSuccess != lastProtocolShowSuccess) {
      uiNeedsRedraw = true;
      lastProtocolBindSeconds = bindSeconds;
      lastProtocolBindingMode = espNowBindingMode;
      lastProtocolShowSuccess = showSuccess;
    }

    // Keep protocol status text (ELRS UART/link state, bind countdown, etc.) live
    // even when there is no touch/D-pad activity.
    if (now - lastProtocolAutoRefreshTime >= 250) {
      lastProtocolAutoRefreshTime = now;
      if (protocol == PROTOCOL_ELRS || espNowBindingMode || showSuccess || otaModeActive) {
        uiNeedsRedraw = true;
      }
    }
  } else {
    lastProtocolBindSeconds = -1;
    lastProtocolBindingMode = false;
    lastProtocolShowSuccess = false;
    lastProtocolAutoRefreshTime = 0;
  }

  static unsigned long lastElrsScreenAutoRefreshTime = 0;
  if (currentScreen == SCREEN_ELRS) {
    if (now - lastElrsScreenAutoRefreshTime >= 250) {
      lastElrsScreenAutoRefreshTime = now;
      uiNeedsRedraw = true;
    }
  } else {
    lastElrsScreenAutoRefreshTime = 0;
  }

  m1 = (leftThrottle + 1.0f) * 0.5f;
  m2 = (rightThrottle + 1.0f) * 0.5f;
  m3 = (leftY + 1.0f) * 0.5f;
  m4 = (rightY + 1.0f) * 0.5f;

  static unsigned long lastDpadTime = 0;
  static unsigned long lastMainFrameTime = 0;
  static unsigned long lastTopBarFrameTime = 0;

  if (keyboardActive) {

  if (millis() - lastDpadTime > 150) {

    bool didInput = false;

    if (down) {
      kbCursorRow = (kbCursorRow + 1) % KB_ROWS;
      keyboardNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      kbCursorCol--;
      if (kbCursorCol < 0) kbCursorCol = KB_COLS - 1;
      keyboardNeedsRedraw = true;
      didInput = true;
    }

    if (right) {
      kbCursorCol++;
      if (kbCursorCol >= KB_COLS) kbCursorCol = 0;
      keyboardNeedsRedraw = true;
      didInput = true;
    }

    if (select) {
      handleKeyboardSelect();
      didInput = true;
    }

    if (didInput) {
      lastDpadTime = millis();
    }

    userActive = true;
  }
}

  if (currentScreen == SCREEN_MAIN &&
      now - lastMainFrameTime >= MAIN_FRAME_INTERVAL_MS) {
    uiNeedsRedraw = true;
  }

  if (currentScreen != SCREEN_SPLASH &&
      currentScreen != SCREEN_SPACE_GAME &&
      now - lastTopBarFrameTime >= TOPBAR_UPDATE_INTERVAL_MS) {
    topBarNeedsRedraw = true;
    lastTopBarFrameTime = now;
  }

  if (currentScreen == SCREEN_SPLASH) {
    drawSplash();
    fullRedraw = true;
    return;
  }

  if (currentScreen != SCREEN_SPACE_GAME && millis() - lastDpadTime > 150) {

    bool didInput = false;

    if (mixNumpadActive) {
      dpadFocusVisible = true;
      const char* mixPadLayout[4][3] = {
        {"1", "2", "3"},
        {"4", "5", "6"},
        {"7", "8", "9"},
        {"<", "0", "-"}
      };

      if (down) {
        mixNumpadCursorRow = (mixNumpadCursorRow + 1) % 4;
        if (mixNumpadCursorRow > 0 && mixNumpadCursorCol > 2) {
          mixNumpadCursorCol = 2;
        }
        mixNumpadNeedsRedraw = true;
        didInput = true;
      }

      if (left) {
        mixNumpadCursorCol--;
        if (mixNumpadCursorRow == 0) {
          if (mixNumpadCursorCol < 0) mixNumpadCursorCol = 3;
        } else {
          if (mixNumpadCursorCol < 0) mixNumpadCursorCol = 2;
        }
        mixNumpadNeedsRedraw = true;
        didInput = true;
      }

      if (right) {
        int maxCol = (mixNumpadCursorRow == 0) ? 4 : 3;
        mixNumpadCursorCol = (mixNumpadCursorCol + 1) % maxCol;
        mixNumpadNeedsRedraw = true;
        didInput = true;
      }

      if (select) {
        if (mixNumpadCursorRow == 0 && mixNumpadCursorCol == 3) {
          closeMixNumpad(true);
          mixNumpadNeedsRedraw = true;
          uiNeedsRedraw = true;
          didInput = true;
        }
        else {
          const char* key = mixPadLayout[mixNumpadCursorRow][mixNumpadCursorCol];
          bool positiveOnlyTarget = (mixNumpadTarget == NUMPAD_TARGET_RATE_VALUE ||
                                     mixNumpadTarget == NUMPAD_TARGET_EXPO_VALUE ||
                                     mixNumpadTarget == NUMPAD_TARGET_ENDPOINT_VALUE);

          if (strcmp(key, "<") == 0) {
            if (mixNumpadBuffer.length() > 0) {
              mixNumpadBuffer.remove(mixNumpadBuffer.length() - 1);
              if (mixNumpadBuffer == "-") mixNumpadBuffer = "";
            }
          }
          else if (strcmp(key, "-") == 0) {
            if (!positiveOnlyTarget) {
              if (mixNumpadBuffer.startsWith("-")) mixNumpadBuffer.remove(0, 1);
              else mixNumpadBuffer = "-" + mixNumpadBuffer;
              if (mixNumpadBuffer.length() == 0) mixNumpadBuffer = "-";
            }
          }
          else {
            String candidate = mixNumpadBuffer;
            if (candidate == "0") candidate = "";
            if (candidate == "-0") candidate = "-";
            candidate += key;

            int digitCount = 0;
            for (int i = 0; i < candidate.length(); i++) {
              if (candidate[i] >= '0' && candidate[i] <= '9') digitCount++;
            }

            int parsedValue = candidate.toInt();
            bool validCandidate = false;
            if (positiveOnlyTarget) {
              int maxValue = (mixNumpadTarget == NUMPAD_TARGET_ENDPOINT_VALUE) ? 120 : 100;
              validCandidate = (digitCount <= 3 && parsedValue >= 0 && parsedValue <= maxValue);
            } else {
              validCandidate = (digitCount <= 3 && (candidate == "-" || (parsedValue >= -100 && parsedValue <= 100)));
            }

            if (validCandidate) {
              mixNumpadBuffer = candidate;
            }
          }

          mixNumpadNeedsRedraw = true;
          uiNeedsRedraw = true;
          didInput = true;
        }
      }
    }
    else if (currentScreen == SCREEN_MAIN) {
      if (down || left || right || select) dpadFocusVisible = true;

      if (down) {
        selectedButton = BTN_MENU;
        uiNeedsRedraw = true;
        didInput = true;
      }

      if (select && selectedButton == BTN_MENU) {
        setScreen(SCREEN_MENU);
        selectedButton = BTN_NONE;
        didInput = true;
      }
    }

  else if (currentScreen == SCREEN_MENU) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {

      if (selectedButton == BTN_NONE) selectedButton = BTN_CTRL;
        else if (selectedButton == BTN_CTRL) selectedButton = BTN_MODEL;
        else if (selectedButton == BTN_MODEL) selectedButton = BTN_DISPLAY_SETTINGS;
        else if (selectedButton == BTN_DISPLAY_SETTINGS) selectedButton = BTN_GAME;
        else if (selectedButton == BTN_GAME) selectedButton = BTN_BACK;
        else if (selectedButton == BTN_BACK) selectedButton = BTN_CTRL;
          fullRedraw = true;
          uiNeedsRedraw = true;
          didInput = true;
          userActive = true;
      }

    if (left) {
      if (selectedButton == BTN_GAME) {
        selectedButton = BTN_DISPLAY_SETTINGS;
        fullRedraw = true;
        uiNeedsRedraw = true;
      } else if (selectedButton == BTN_DISPLAY_SETTINGS) {
        selectedButton = BTN_MODEL;
        fullRedraw = true;
        uiNeedsRedraw = true;
      } else {
        setScreen(SCREEN_MAIN);
        selectedButton = BTN_NONE;
      }
      didInput = true;
      userActive = true;
    }

    if (right && selectedButton == BTN_MODEL) {
      selectedButton = BTN_DISPLAY_SETTINGS;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
      userActive = true;
    }
    else if (right && selectedButton == BTN_DISPLAY_SETTINGS) {
      selectedButton = BTN_GAME;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
      userActive = true;
    }

    if (select) {

      if (selectedButton == BTN_BACK) setScreen(SCREEN_MAIN);
      else if (selectedButton == BTN_CTRL) setScreen(SCREEN_CONTROLLER_SETTINGS);
      else if (selectedButton == BTN_MODEL) setScreen(SCREEN_MODEL_SETTINGS);
      else if (selectedButton == BTN_DISPLAY_SETTINGS) setScreen(SCREEN_DISPLAY_SETTINGS);
      else if (selectedButton == BTN_GAME) setScreen(SCREEN_SPACE_GAME);

      selectedButton = BTN_NONE;
      didInput = true;
      userActive = true;
    }
  }

  else if (currentScreen == SCREEN_DISPLAY_SETTINGS) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (selectedButton == BTN_NONE) selectedButton = BTN_DISPLAY_BRIGHTNESS_DEC;
      else if (selectedButton == BTN_DISPLAY_BRIGHTNESS_DEC) selectedButton = BTN_DISPLAY_BRIGHTNESS_INC;
      else if (selectedButton == BTN_DISPLAY_BRIGHTNESS_INC) selectedButton = BTN_DISPLAY_TIMEOUT_DEC;
      else if (selectedButton == BTN_DISPLAY_TIMEOUT_DEC) selectedButton = BTN_DISPLAY_TIMEOUT_INC;
      else if (selectedButton == BTN_DISPLAY_TIMEOUT_INC) selectedButton = BTN_DISPLAY_OFF_TIMEOUT_DEC;
      else if (selectedButton == BTN_DISPLAY_OFF_TIMEOUT_DEC) selectedButton = BTN_DISPLAY_OFF_TIMEOUT_INC;
      else if (selectedButton == BTN_DISPLAY_OFF_TIMEOUT_INC) selectedButton = BTN_DISPLAY_SLEEP_DEC;
      else if (selectedButton == BTN_DISPLAY_SLEEP_DEC) selectedButton = BTN_DISPLAY_SLEEP_INC;
      else if (selectedButton == BTN_DISPLAY_SLEEP_INC) selectedButton = BTN_DISPLAY_THEME_TOGGLE;
      else if (selectedButton == BTN_DISPLAY_THEME_TOGGLE) selectedButton = BTN_DISPLAY_SLEEP_NOW;
      else if (selectedButton == BTN_DISPLAY_SLEEP_NOW) selectedButton = BTN_BACK;
      else selectedButton = BTN_DISPLAY_BRIGHTNESS_DEC;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
      userActive = true;
    }

    if (left) {
      if (selectedButton == BTN_BACK) {
        setScreen(SCREEN_MENU);
      }
      else if (selectedButton == BTN_DISPLAY_BRIGHTNESS_INC) {
        selectedButton = BTN_DISPLAY_BRIGHTNESS_DEC;
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_TIMEOUT_INC) {
        selectedButton = BTN_DISPLAY_TIMEOUT_DEC;
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_OFF_TIMEOUT_INC) {
        selectedButton = BTN_DISPLAY_OFF_TIMEOUT_DEC;
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_SLEEP_INC) {
        selectedButton = BTN_DISPLAY_SLEEP_DEC;
        fullRedraw = true;
        uiNeedsRedraw = true;
      } else if (selectedButton == BTN_DISPLAY_THEME_TOGGLE) {
        selectedButton = BTN_DISPLAY_SLEEP_INC;
        fullRedraw = true;
        uiNeedsRedraw = true;
      } else if (selectedButton == BTN_DISPLAY_SLEEP_NOW) {
        selectedButton = BTN_DISPLAY_THEME_TOGGLE;
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else {
        setScreen(SCREEN_MENU);
      }
      didInput = true;
      userActive = true;
    }

    if (right) {
      if (selectedButton == BTN_DISPLAY_BRIGHTNESS_DEC) {
        selectedButton = BTN_DISPLAY_BRIGHTNESS_INC;
      }
      else if (selectedButton == BTN_DISPLAY_TIMEOUT_DEC) {
        selectedButton = BTN_DISPLAY_TIMEOUT_INC;
      }
      else if (selectedButton == BTN_DISPLAY_OFF_TIMEOUT_DEC) {
        selectedButton = BTN_DISPLAY_OFF_TIMEOUT_INC;
      }
      else if (selectedButton == BTN_DISPLAY_SLEEP_DEC) {
        selectedButton = BTN_DISPLAY_SLEEP_INC;
      }
      else if (selectedButton == BTN_DISPLAY_SLEEP_INC) {
        selectedButton = BTN_DISPLAY_THEME_TOGGLE;
      }
      else if (selectedButton == BTN_DISPLAY_THEME_TOGGLE) {
        selectedButton = BTN_DISPLAY_SLEEP_NOW;
      }
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
      userActive = true;
    }

    if (select) {
      if (selectedButton == BTN_BACK) {
        setScreen(SCREEN_MENU);
      }
      else if (selectedButton == BTN_DISPLAY_BRIGHTNESS_DEC) {
        stepDisplayOption(displayBrightness, displayBrightnessOptions, displayBrightnessOptionCount, -1);
        displaySleepBrightness = min(displaySleepBrightness, displayBrightness);
        saveDisplaySettings();
        applyDisplayBacklight();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_BRIGHTNESS_INC) {
        stepDisplayOption(displayBrightness, displayBrightnessOptions, displayBrightnessOptionCount, 1);
        saveDisplaySettings();
        applyDisplayBacklight();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_TIMEOUT_DEC) {
        displayTimeoutIndex = constrain((int)displayTimeoutIndex - 1, 0, displayTimeoutOptionCount - 1);
        saveDisplaySettings();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_TIMEOUT_INC) {
        displayTimeoutIndex = constrain((int)displayTimeoutIndex + 1, 0, displayTimeoutOptionCount - 1);
        saveDisplaySettings();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_OFF_TIMEOUT_DEC) {
        displayOffTimeoutIndex = constrain((int)displayOffTimeoutIndex - 1, 0, displayOffTimeoutOptionCount - 1);
        saveDisplaySettings();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_OFF_TIMEOUT_INC) {
        displayOffTimeoutIndex = constrain((int)displayOffTimeoutIndex + 1, 0, displayOffTimeoutOptionCount - 1);
        saveDisplaySettings();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_SLEEP_DEC) {
        stepDisplayOption(displaySleepBrightness, displaySleepBrightnessOptions, displaySleepBrightnessOptionCount, -1);
        saveDisplaySettings();
        applyDisplayBacklight();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_SLEEP_INC) {
        stepDisplayOption(displaySleepBrightness, displaySleepBrightnessOptions, displaySleepBrightnessOptionCount, 1);
        saveDisplaySettings();
        applyDisplayBacklight();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_THEME_TOGGLE) {
        themeMode = (themeMode == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
        applyThemePalette();
        saveDisplaySettings();
        fullRedraw = true;
        uiNeedsRedraw = true;
        topBarNeedsRedraw = true;
      }
      else if (selectedButton == BTN_DISPLAY_SLEEP_NOW) {
        displayDimmed = false;
        screenAwake = false;
        suppressWakeUntilRelease = true;
        applyDisplayBacklight();
      }

      didInput = true;
      userActive = true;
    }
  }
  else if (currentScreen == SCREEN_CONTROLLER_SETTINGS) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (controllerSettingsPage == 0) {
        if (selectedButton == BTN_NONE) selectedButton = BTN_TRIM;
        else if (selectedButton == BTN_TRIM) selectedButton = BTN_FAILSAFE;
        else if (selectedButton == BTN_FAILSAFE) selectedButton = BTN_ENDPOINTS;
        else if (selectedButton == BTN_ENDPOINTS) selectedButton = BTN_PAGE_NAV;
        else if (selectedButton == BTN_PAGE_NAV) selectedButton = BTN_BACK;
        else selectedButton = BTN_TRIM;
      } else {
        if (selectedButton == BTN_NONE) selectedButton = BTN_EXPO;
        else if (selectedButton == BTN_EXPO) selectedButton = BTN_RATES;
        else if (selectedButton == BTN_RATES) selectedButton = BTN_PROTOCOL;
        else if (selectedButton == BTN_PROTOCOL) selectedButton = BTN_PAGE_NAV;
        else if (selectedButton == BTN_PAGE_NAV) selectedButton = BTN_BACK;
        else selectedButton = BTN_EXPO;
      }
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      setScreen(SCREEN_MENU);
      didInput = true;
    }

    if (select) {
      if (selectedButton == BTN_BACK) setScreen(SCREEN_MENU);
      else if (selectedButton == BTN_PAGE_NAV) {
        controllerSettingsPage = 1 - controllerSettingsPage;
        selectedButton = (controllerSettingsPage == 0) ? BTN_TRIM : BTN_EXPO;
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_TRIM) setScreen(SCREEN_TRIM);
      else if (selectedButton == BTN_FAILSAFE) setScreen(SCREEN_FAILSAFE);
      else if (selectedButton == BTN_ENDPOINTS) setScreen(SCREEN_ENDPOINTS);
      else if (selectedButton == BTN_EXPO) setScreen(SCREEN_EXPO);
      else if (selectedButton == BTN_RATES) setScreen(SCREEN_RATES);
      else if (selectedButton == BTN_PROTOCOL) setScreen(SCREEN_PROTOCOL);
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_ENDPOINTS) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % (CHANNEL_COUNT + 1);
      endpointNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left && focusIndex == CHANNEL_COUNT) {
      setScreen(SCREEN_CONTROLLER_SETTINGS);
      didInput = true;
    }

    if (select) {
      if (focusIndex == CHANNEL_COUNT) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
      } else {
        endpointNumpadChannel = focusIndex;
        endpointNumpadHighSide = isEndpointHighSideSelected(focusIndex);
        openMixNumpad(NUMPAD_TARGET_ENDPOINT_VALUE);
      }
      endpointNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_EXPO) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % 6;
      expoNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      if (focusIndex == 0) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
        didInput = true;
      }
      else if (focusIndex >= 1 && focusIndex <= 4) {
        selectedExpoChannel = (selectedExpoChannel + CHANNEL_COUNT - 1) % CHANNEL_COUNT;
        focusIndex = selectedExpoChannel + 1;
        expoNeedsRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
      else if (focusIndex == 5) {
        setModelExpoValue(activeModel, selectedExpoChannel,
                          getModelExpoValue(activeModel, selectedExpoChannel) - 1);
        saveExpoValues();
        expoNeedsRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
    }

    if (right) {
      if (focusIndex >= 1 && focusIndex <= 4) {
        selectedExpoChannel = (selectedExpoChannel + 1) % CHANNEL_COUNT;
        focusIndex = selectedExpoChannel + 1;
        expoNeedsRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
      else if (focusIndex == 5) {
        setModelExpoValue(activeModel, selectedExpoChannel,
                          getModelExpoValue(activeModel, selectedExpoChannel) + 1);
        saveExpoValues();
        expoNeedsRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
    }

    if (select) {
      if (focusIndex == 0) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
      }
      else if (focusIndex >= 1 && focusIndex <= 4) {
        selectedExpoChannel = focusIndex - 1;
        expoNeedsRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (focusIndex == 5) {
        openMixNumpad(NUMPAD_TARGET_EXPO_VALUE);
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_RATES) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % 9;
      ratesNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      if (focusIndex == 0) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
        didInput = true;
      }
      else if (focusIndex >= 1 && focusIndex <= 4) {
        selectedRateChannel = (selectedRateChannel + CHANNEL_COUNT - 1) % CHANNEL_COUNT;
        focusIndex = selectedRateChannel + 1;
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
      else if (focusIndex == 8) {
        setModelRateValue(activeModel, selectedRateChannel,
                          getModelRateValue(activeModel, selectedRateChannel) - 1);
        saveRateValues();
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
    }

    if (right) {
      if (focusIndex >= 1 && focusIndex <= 4) {
        selectedRateChannel = (selectedRateChannel + 1) % CHANNEL_COUNT;
        focusIndex = selectedRateChannel + 1;
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
      else if (focusIndex == 8) {
        setModelRateValue(activeModel, selectedRateChannel,
                          getModelRateValue(activeModel, selectedRateChannel) + 1);
        saveRateValues();
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
    }

    if (select) {
      if (focusIndex == 0) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
      }
      else if (focusIndex >= 1 && focusIndex <= 4) {
        selectedRateChannel = focusIndex - 1;
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (focusIndex == 5) {
        setModelRateValue(activeModel, selectedRateChannel, RATE_LOW_VALUE);
        saveRateValues();
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (focusIndex == 6) {
        setModelRateValue(activeModel, selectedRateChannel, RATE_NORMAL_VALUE);
        saveRateValues();
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (focusIndex == 7) {
        setModelRateValue(activeModel, selectedRateChannel, RATE_HIGH_VALUE);
        saveRateValues();
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (focusIndex == 8) {
        openMixNumpad(NUMPAD_TARGET_RATE_VALUE);
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_PROTOCOL) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (selectedButton == BTN_NONE) selectedButton = BTN_PROTOCOL_ELRS;
      else if (selectedButton == BTN_PROTOCOL_ELRS) selectedButton = BTN_PROTOCOL_ESPNOW;
      else if (selectedButton == BTN_PROTOCOL_ESPNOW) selectedButton = BTN_PROTOCOL_BIND;
      else if (selectedButton == BTN_PROTOCOL_BIND) selectedButton = BTN_PROTOCOL_OTA;
      else if (selectedButton == BTN_PROTOCOL_OTA) selectedButton = BTN_PROTOCOL_OTA_CFG;
      else if (selectedButton == BTN_PROTOCOL_OTA_CFG) selectedButton = BTN_BACK;
      else selectedButton = BTN_PROTOCOL_ELRS;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      if (!otaUpdateInProgress) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
        didInput = true;
      }
    }

  if (select) {
      if (selectedButton == BTN_BACK) {
        if (!otaUpdateInProgress) {
          setScreen(SCREEN_CONTROLLER_SETTINGS);
        }
      } else if (selectedButton == BTN_PROTOCOL_ELRS) {
        if (!otaModeActive) {
          cancelEspNowBinding(true);
          setModelProtocol(activeModel, PROTOCOL_ELRS);
          saveModels();
          fullRedraw = true;
          uiNeedsRedraw = true;
        }
      } else if (selectedButton == BTN_PROTOCOL_ESPNOW) {
        if (!otaModeActive) {
          setModelProtocol(activeModel, PROTOCOL_ESPNOW);
          saveModels();
          fullRedraw = true;
          uiNeedsRedraw = true;
        }
      } else if (selectedButton == BTN_PROTOCOL_BIND) {
        if (!otaModeActive) {
          if (getModelProtocol(activeModel) == PROTOCOL_ELRS) {
            setScreen(SCREEN_ELRS);
          } else {
            if (espNowBindingMode) cancelEspNowBinding(true);
            else beginEspNowBinding();
          }
          fullRedraw = true;
          uiNeedsRedraw = true;
        }
      } else if (selectedButton == BTN_PROTOCOL_OTA) {
        if (otaModeActive) stopOtaMode();
        else startOtaMode();
        fullRedraw = true;
        uiNeedsRedraw = true;
      } else if (selectedButton == BTN_PROTOCOL_OTA_CFG) {
        if (!otaUpdateInProgress) {
          setScreen(SCREEN_OTA_SETTINGS);
        }
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_OTA_SETTINGS) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % 4;
      otaSettingsNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      setScreen(SCREEN_PROTOCOL);
      didInput = true;
    }

    if (select) {
      if (focusIndex == 0) {
        setScreen(SCREEN_PROTOCOL);
      } else if (focusIndex == 1) {
        beginOtaFieldEdit(KEYBOARD_TARGET_OTA_STA_SSID);
      } else if (focusIndex == 2) {
        beginOtaFieldEdit(KEYBOARD_TARGET_OTA_STA_PASSWORD);
      } else if (focusIndex == 3) {
        beginOtaFieldEdit(KEYBOARD_TARGET_OTA_AP_SSID);
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_ELRS) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (selectedButton == BTN_NONE) selectedButton = BTN_ELRS_BIND;
      else if (selectedButton == BTN_ELRS_BIND) selectedButton = BTN_BACK;
      else selectedButton = BTN_ELRS_BIND;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      setScreen(SCREEN_PROTOCOL);
      didInput = true;
    }

    if (select) {
      if (selectedButton == BTN_BACK) {
        setScreen(SCREEN_PROTOCOL);
      } else if (selectedButton == BTN_ELRS_BIND) {
        sendElrsBindCommand();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_MODEL_SETTINGS) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (modelSettingsPage == 0) {
        if (selectedButton == BTN_NONE) selectedButton = BTN_MODEL_NAME;
        else if (selectedButton == BTN_MODEL_NAME) selectedButton = BTN_DRIVE_TYPE;
        else if (selectedButton == BTN_DRIVE_TYPE) selectedButton = BTN_MIXING;
        else if (selectedButton == BTN_MIXING) selectedButton = BTN_PAGE_NAV;
        else if (selectedButton == BTN_PAGE_NAV) selectedButton = BTN_BACK;
        else selectedButton = BTN_MODEL_NAME;
      }
      else {
        if (selectedButton == BTN_NONE) selectedButton = BTN_REVERSE;
        else if (selectedButton == BTN_REVERSE) selectedButton = BTN_PAGE_NAV;
        else if (selectedButton == BTN_PAGE_NAV) selectedButton = BTN_BACK;
        else selectedButton = BTN_REVERSE;
      }
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      setScreen(SCREEN_MENU);
      didInput = true;
    }

    if (select) {
      if (selectedButton == BTN_BACK) setScreen(SCREEN_MENU);
      else if (selectedButton == BTN_PAGE_NAV) {
        modelSettingsPage = 1 - modelSettingsPage;
        selectedButton = (modelSettingsPage == 0) ? BTN_MODEL_NAME : BTN_REVERSE;
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      else if (selectedButton == BTN_MODEL_NAME) setScreen(SCREEN_MODEL_NAME);
      else if (selectedButton == BTN_DRIVE_TYPE) setScreen(SCREEN_DRIVE_TYPE);
      else if (selectedButton == BTN_MIXING) setScreen(SCREEN_MIXING);
      else if (selectedButton == BTN_REVERSE) setScreen(SCREEN_REVERSE);
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_DRIVE_TYPE) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (selectedButton == BTN_NONE || selectedButton == BTN_DRIVE_TANK) selectedButton = BTN_DRIVE_CAR;
      else if (selectedButton == BTN_DRIVE_CAR) selectedButton = BTN_DRIVE_OMNI;
      else if (selectedButton == BTN_DRIVE_OMNI) selectedButton = BTN_DRIVE_X_DRONE;
      else if (selectedButton == BTN_DRIVE_X_DRONE) selectedButton = BTN_BACK;
      else selectedButton = BTN_DRIVE_TANK;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      setScreen(SCREEN_MODEL_SETTINGS);
      didInput = true;
    }

    if (select) {
      if (selectedButton == BTN_BACK) {
        setScreen(SCREEN_MODEL_SETTINGS);
      } else {
        if (selectedButton == BTN_DRIVE_TANK) {
          currentDrive = DRIVE_TANK;
          setModelDriveType(activeModel, DRIVE_TANK);
          saveModels();
          setScreen(SCREEN_TANK_MODE);
        }
        else {
          if (selectedButton == BTN_DRIVE_CAR) {
            currentDrive = DRIVE_CAR;
            setModelDriveType(activeModel, DRIVE_CAR);
            saveModels();
            setScreen(SCREEN_TANK_MODE);
          }
          else if (selectedButton == BTN_DRIVE_OMNI) currentDrive = DRIVE_OMNI;
          else if (selectedButton == BTN_DRIVE_X_DRONE) currentDrive = DRIVE_X_DRONE;

          if (selectedButton != BTN_DRIVE_CAR) {
            setModelDriveType(activeModel, currentDrive);
            saveModels();
            fullRedraw = true;
            uiNeedsRedraw = true;
          }
        }
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_TANK_MODE) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (selectedButton == BTN_NONE || selectedButton == BTN_TANK_DUAL) selectedButton = BTN_TANK_SINGLE;
      else if (selectedButton == BTN_TANK_SINGLE) selectedButton = BTN_BACK;
      else selectedButton = BTN_TANK_DUAL;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      setScreen(SCREEN_DRIVE_TYPE);
      didInput = true;
    }

    if (select) {
      if (selectedButton == BTN_BACK) {
        setScreen(SCREEN_DRIVE_TYPE);
      } else if (selectedButton == BTN_TANK_DUAL) {
        setModelTankMode(activeModel, TANK_MODE_DUAL_STICK);
        saveModels();
        fullRedraw = true;
        uiNeedsRedraw = true;
      } else if (selectedButton == BTN_TANK_SINGLE) {
        setModelTankMode(activeModel, TANK_MODE_RIGHT_STICK);
        saveModels();
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_REVERSE) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % (CHANNEL_COUNT + 1);
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left && focusIndex == CHANNEL_COUNT) {
      setScreen(reverseReturnScreen);
      didInput = true;
    }

    if (select || right) {
      if (focusIndex == CHANNEL_COUNT) {
        setScreen(reverseReturnScreen);
      } else {
        bool linked[CHANNEL_COUNT] = {false, false, false, false};
        getLinkedReverseChannels(activeModel, focusIndex, linked);
        bool newState = !models[activeModel].reverse[focusIndex];

        for (int ch = 0; ch < CHANNEL_COUNT; ch++) {
          if (!linked[ch]) continue;
          models[activeModel].reverse[ch] = newState;
          reverseChannelDirty[ch] = true;
        }

        saveModels();
        reverseNeedsRedraw = true;
        uiNeedsRedraw = true;
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_TRIM) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % 7;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if ((select || right || left) && focusIndex == 0) {
      setScreen(SCREEN_CONTROLLER_SETTINGS);
      didInput = true;
    }
    else if ((select || right || left) && focusIndex == 1) {
      currentTrimPage = !currentTrimPage;
      trimRenderX = models[activeModel].trimX[currentTrimPage];
      trimRenderY = models[activeModel].trimY[currentTrimPage];
      trimNeedsRedraw = true;
      trimDirty = true;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if ((select || right || left) && focusIndex >= 2 && focusIndex <= 5) {
      if (focusIndex == 2) {
        models[activeModel].trimX[currentTrimPage] =
          constrain(models[activeModel].trimX[currentTrimPage] - 1, -50, 50);
      }
      else if (focusIndex == 3) {
        models[activeModel].trimX[currentTrimPage] =
          constrain(models[activeModel].trimX[currentTrimPage] + 1, -50, 50);
      }
      else if (focusIndex == 4) {
        models[activeModel].trimY[currentTrimPage] =
          constrain(models[activeModel].trimY[currentTrimPage] + 1, -50, 50);
      }
      else if (focusIndex == 5) {
        models[activeModel].trimY[currentTrimPage] =
          constrain(models[activeModel].trimY[currentTrimPage] - 1, -50, 50);
      }

      saveModels();
      trimRenderX = models[activeModel].trimX[currentTrimPage];
      trimRenderY = models[activeModel].trimY[currentTrimPage];
      trimDirty = true;
      trimNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if ((select || right || left) && focusIndex == 6) {
      models[activeModel].trimX[currentTrimPage] = 0;
      models[activeModel].trimY[currentTrimPage] = 0;
      saveModels();

      trimRenderX = 0;
      trimRenderY = 0;
      trimDirty = true;
      trimNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_FAILSAFE) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % (CHANNEL_COUNT + 1);
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left && focusIndex == CHANNEL_COUNT) {
      setScreen(SCREEN_CONTROLLER_SETTINGS);
      didInput = true;
    }

    if (select || right) {
      if (focusIndex == CHANNEL_COUNT) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
      } else {
        bool newState = !models[activeModel].failsafe[focusIndex];
        models[activeModel].failsafe[focusIndex] = newState;
        if (newState) {
          setModelFailsafeValue(activeModel, focusIndex,
                                (int)roundf(outputChannels[focusIndex] * 100.0f));
          saveFailsafeValues();
        }
        saveModels();
        failsafeDirty[focusIndex] = true;
        failsafeNeedsRedraw = true;
        uiNeedsRedraw = true;
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_MODEL_NAME) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % 10;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left) {
      if (focusIndex == 0) setScreen(SCREEN_MODEL_SETTINGS);
      else {
        focusIndex = 0;
        fullRedraw = true;
        uiNeedsRedraw = true;
      }
      didInput = true;
    }

    if (select) {
      if (focusIndex == 0) {
        setScreen(SCREEN_MODEL_SETTINGS);
      }
      else if (focusIndex == 1) {
        int targetModel = (selectedModelIndex >= 0) ? selectedModelIndex : activeModel;
        keyboardBuffer = modelNames[targetModel];
        selectedModelIndex = targetModel;
        keyboardTarget = KEYBOARD_TARGET_MODEL_NAME;
        keyboardActive = true;
        keyboardLowercase = false;
        keyboardNeedsRedraw = true;
        inputBoxSelected = true;
      }
      else if (focusIndex >= 2 && focusIndex <= 5) {
        int i = focusIndex - 2;
        selectedModelIndex = i;
        if (modelNames[i].length() == 0) {
          keyboardBuffer = "";
          keyboardTarget = KEYBOARD_TARGET_MODEL_NAME;
          keyboardActive = true;
          keyboardLowercase = false;
          keyboardNeedsRedraw = true;
          inputBoxSelected = true;
          modelNameDirty = true;
          modelNameNeedsRedraw = true;
          uiNeedsRedraw = true;
        } else {
          selectModelSlot(i);
        }
      }
      else if (focusIndex >= 6 && focusIndex <= 9) {
        int i = focusIndex - 6;
        for (int j = i; j < 3; j++) {
          modelNames[j] = modelNames[j + 1];
          models[j] = models[j + 1];
          memcpy(boundReceiverMacs[j], boundReceiverMacs[j + 1], ESP_NOW_ETH_ALEN);
          for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
            modelFailsafeValues[j][channel] = modelFailsafeValues[j + 1][channel];
            modelRateValues[j][channel] = modelRateValues[j + 1][channel];
            modelExpoValues[j][channel] = modelExpoValues[j + 1][channel];
            modelEndpointLowValues[j][channel] = modelEndpointLowValues[j + 1][channel];
            modelEndpointHighValues[j][channel] = modelEndpointHighValues[j + 1][channel];
          }
        }
        modelNames[3] = "";
        initModelDefaults(3);
        clearBoundReceiver(3);
        clearModelFailsafeValues(3);
        clearModelRateValues(3);
        clearModelExpoValues(3);
        clearModelEndpointValues(3);
        saveModels();
        saveReceiverBindings();
        saveFailsafeValues();
        saveRateValues();
        saveExpoValues();
        saveEndpointValues();
        modelNameDirty = true;
        modelNameNeedsRedraw = true;
        uiNeedsRedraw = true;
      }
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_MIXING) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % 15;
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    MixData &mix = models[activeModel].mixes[selectedMixIndex];
    uint8_t source = getMixSource(mix);
    uint8_t destination = getMixDestination(mix);

    if (left && focusIndex == 0) {
      setScreen(SCREEN_MODEL_SETTINGS);
      didInput = true;
    }
    else if ((select || left || right) && focusIndex >= 1 && focusIndex <= 4) {
      if (left && focusIndex > 1) focusIndex--;
      else if (right && focusIndex < 4) focusIndex++;
      selectedMixIndex = focusIndex - 1;
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 5 && (select || left || right)) {
      mix.enabled = !mix.enabled;
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 6 && (select || left || right)) {
      if (left) source = (source + CHANNEL_COUNT - 1) % CHANNEL_COUNT;
      else source = (source + 1) % CHANNEL_COUNT;
      setMixSource(mix, source);
      if (source == destination) {
        destination = (destination + 1) % CHANNEL_COUNT;
        setMixDestination(mix, destination);
      }
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 7 && (select || left || right)) {
      if (left) destination = (destination + CHANNEL_COUNT - 1) % CHANNEL_COUNT;
      else destination = (destination + 1) % CHANNEL_COUNT;
      setMixDestination(mix, destination);
      if (destination == source) {
        destination = (destination + 1) % CHANNEL_COUNT;
        setMixDestination(mix, destination);
      }
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 8 && (select || left || right)) {
      setMixReverseLinked(mix, !isMixReverseLinked(mix));
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 9 && (select || left || right)) {
      mix.rate = constrain(mix.rate - 1, -100, 100);
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 10 && select) {
      openMixNumpad(NUMPAD_TARGET_MIX_RATE);
      didInput = true;
    }
    else if (focusIndex == 11 && (select || left || right)) {
      mix.rate = constrain(mix.rate + 1, -100, 100);
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 12 && (select || left || right)) {
      mix.offset = constrain(mix.offset - 1, -100, 100);
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 13 && select) {
      openMixNumpad(NUMPAD_TARGET_MIX_OFFSET);
      didInput = true;
    }
    else if (focusIndex == 14 && (select || left || right)) {
      mix.offset = constrain(mix.offset + 1, -100, 100);
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
  }

    if (didInput) {
      lastDpadTime = millis();
      userActive = true;
    }
  }

  // ===== TOUCH DETECTION =====
  bool isTouching = (touchCount > 0);

  int x = mapTouch(rawX, 0, 240, 0, 240);
  int y = mapTouch(rawY, 320, 0, 320, 0);

  x = constrain(x, 0, 240);
  y = constrain(y, 0, 320);

  if (isTouching &&
      currentScreen == SCREEN_MODEL_NAME &&
      pendingModelHoldIndex >= 0 &&
      !pendingModelHoldTriggered &&
      !keyboardActive &&
      (millis() - pendingModelHoldStart >= MODEL_NAME_HOLD_MS)) {
    pendingModelHoldTriggered = true;
    beginModelNameEdit(pendingModelHoldIndex);
    uiNeedsRedraw = true;
  }

    // ===== DPAD =====
  if (select || down || left || right) {

    if (!screenAwake) {
      if (suppressWakeUntilRelease) {
        touchActive = true;
        return;
      }
      lastActivityTime = millis();
      screenAwake = true;
      displayDimmed = false;
      suppressWakeUntilRelease = false;
      applyDisplayBacklight();
      fullRedraw = true;
      uiNeedsRedraw = true;

      delay(100);
      return;
    }
    if (displayDimmed) {
      displayDimmed = false;
      applyDisplayBacklight();
      fullRedraw = true;
      uiNeedsRedraw = true;
    }
  userActive = true;
}

if (!isTouching && ratesValueDirty) {
  saveRateValues();
  ratesValueDirty = false;
}
if (!isTouching && expoValueDirty) {
  saveExpoValues();
  expoValueDirty = false;
}

// ===== RELEASE → ACTION =====
if (!isTouching && waitingForRelease) {
  if (currentScreen == SCREEN_MODEL_NAME && pendingModelHoldIndex >= 0) {
    if (!pendingModelHoldTriggered) {
      selectModelSlot(pendingModelHoldIndex);
    }

    pendingModelHoldIndex = -1;
    pendingModelHoldStart = 0;
    pendingModelHoldTriggered = false;
  }

  waitingForRelease = false;
  pressedButton = BTN_NONE;

  if (currentScreen == SCREEN_DRIVE_TYPE) {
    fullRedraw = true;
  }

  if (screenChangePending) {
    if (nextScreen == SCREEN_REVERSE) {
      reverseNeedsRedraw = true;
      for (int i = 0; i < CHANNEL_COUNT; i++) reverseChannelDirty[i] = true;
    }

    if (nextScreen == SCREEN_TRIM) {
      trimNeedsRedraw = true;
      trimDirty = true;
    }

    if (nextScreen == SCREEN_FAILSAFE) {
      failsafeNeedsRedraw = true;
      for (int i = 0; i < CHANNEL_COUNT; i++) failsafeDirty[i] = true;
    }
    if (nextScreen == SCREEN_ENDPOINTS) {
      endpointNeedsRedraw = true;
    }

    if (nextScreen == SCREEN_MIXING) {
      mixingNeedsRedraw = true;
    }

    if (nextScreen != currentScreen) {
      setScreen(nextScreen);
    }

    screenChangePending = false;
  }

  uiNeedsRedraw = true;
}

  // ===== DRAW =====
  if (uiNeedsRedraw) {
    frameCount++;

    if (millis() - fpsLastTime >= 2000) {
      frameCount = 0;
      fpsLastTime = millis();
    }

    tft.startWrite();

    if (currentScreen == SCREEN_MAIN) {

      if (fullRedraw) {
        tft.fillScreen(COLOR_BG);
        drawMainScreenStatic();
        drawModelPanelSemiStatic();
        drawTopBarStatic();
        topBarNeedsRedraw = true;
        fullRedraw = false;
      }

    drawMainScreenDynamic();
    lastMainFrameTime = millis();
    }
    else if (currentScreen == SCREEN_SPACE_GAME) {
      if (fullRedraw) {
        drawSpaceGameStatic();
        fullRedraw = false;
      }
      drawSpaceGameDynamic();
    }
    else {

      if (fullRedraw) {

        switch (currentScreen) {
          case SCREEN_MENU:
          drawMenuScreen();
          break;

          case SCREEN_CONTROLLER_SETTINGS:
          drawControllerSettings();
          break;

          case SCREEN_EXPO:
          drawExpoStatic();
          expoNeedsRedraw = true;
          break;

          case SCREEN_RATES:
          drawRatesStatic();
          ratesNeedsRedraw = true;
          break;

          case SCREEN_PROTOCOL:
          drawProtocolScreen();
          break;

          case SCREEN_OTA_SETTINGS:
          drawOtaSettingsStatic();
          otaSettingsNeedsRedraw = true;
          break;

          case SCREEN_ELRS:
          drawElrsScreen();
          break;

          case SCREEN_MODEL_SETTINGS:
          drawModelSettings();
          break;

          case SCREEN_REVERSE:
          drawReverseStatic();
          reverseNeedsRedraw = true;
          break;

          case SCREEN_TRIM:
          drawTrimStatic();
          fullRedraw = false;
          trimNeedsRedraw = true;
          break;

          case SCREEN_ENDPOINTS:
          drawEndpointStatic();
          endpointNeedsRedraw = true;
          break;

          case SCREEN_FAILSAFE:
          drawFailsafeStatic();
          failsafeNeedsRedraw = true;
          break;

          case SCREEN_MODEL_NAME:
          drawModelNameStatic();
          modelNameNeedsRedraw = true;
          break;

          case SCREEN_DRIVE_TYPE:
          drawDriveTypeScreen();
          break;

          case SCREEN_TANK_MODE:
          drawTankModeScreen();
          break;

          case SCREEN_MIXING:
          drawMixingStatic();
          mixingNeedsRedraw = true;
          break;

          case SCREEN_DISPLAY_SETTINGS:
          drawDisplaySettingsScreen();
          break;
        }

        drawTopBarStatic();
        topBarNeedsRedraw = true;
        fullRedraw = false;
        }

      if (currentScreen == SCREEN_REVERSE) {
        drawReverseDynamic();
      }
      if (currentScreen == SCREEN_RATES) {
        drawRatesDynamic();
      }
      if (currentScreen == SCREEN_EXPO) {
        drawExpoDynamic();
      }
      if (currentScreen == SCREEN_PROTOCOL) {
        drawProtocolDynamic();
      }
      if (currentScreen == SCREEN_OTA_SETTINGS) {
        drawOtaSettingsDynamic();
      }
      if (currentScreen == SCREEN_ELRS) {
        drawElrsDynamic();
      }
      if (currentScreen == SCREEN_TRIM) {
        drawTrimDynamic();
      }
      if (currentScreen == SCREEN_FAILSAFE) {
      drawFailsafeDynamic();
      }
      if (currentScreen == SCREEN_ENDPOINTS) {
      drawEndpointDynamic();
      }
      if (currentScreen == SCREEN_MODEL_NAME) {
      drawModelNameDynamic();
      }
      if (currentScreen == SCREEN_MIXING) {
      drawMixingDynamic();
      }
    }

    tft.endWrite();

    uiNeedsRedraw = false;
  }

  if (keyboardActive) {
    if (keyboardNeedsRedraw) {
      drawKeyboardStatic();
      keyboardNeedsRedraw = false;
    }
  drawKeyboardDynamic();
  }

  if (mixNumpadActive && mixNumpadNeedsRedraw) {
    drawMixNumpad();
    mixNumpadNeedsRedraw = false;
  }

  if (topBarNeedsRedraw &&
      currentScreen != SCREEN_SPLASH &&
      currentScreen != SCREEN_SPACE_GAME) {
    tft.startWrite();
    drawTopBarDynamic();
    tft.endWrite();
    topBarNeedsRedraw = false;
  }

  switch (currentScreen) {
  }
      
  // ===== TOUCH =====
  if (isTouching) {
    if (dpadFocusVisible) {
      dpadFocusVisible = false;
      uiNeedsRedraw = true;
    }

    // ===== WAKE ONLY =====
    if (!screenAwake) {
      lastActivityTime = millis();
      screenAwake = true;
      displayDimmed = false;
      applyDisplayBacklight();

      fullRedraw = true;
      uiNeedsRedraw = true;

      waitingForRelease = false;   // ✅ reset here
      touchActive = false;         // ✅ reset here

      delay(100);  // optional, but OK
      return;
    }
    if (displayDimmed) {
      displayDimmed = false;
      applyDisplayBacklight();
      fullRedraw = true;
      uiNeedsRedraw = true;
    }

  handleTouch(x, y);
  touchActive = true;
  userActive = true;
  }
  else {
    if (touchActive) delay(30);
      touchActive = false;

    if (suppressWakeUntilRelease) {
      suppressWakeUntilRelease = false;
      waitingForRelease = false;
      pressedButton = BTN_NONE;
    }

    if (!waitingForRelease) {
      pendingModelHoldIndex = -1;
      pendingModelHoldStart = 0;
      pendingModelHoldTriggered = false;
    }
  }

  // ===== UPDATE TIMER =====
  if (userActive) {
    lastActivityTime = millis();
  }

  unsigned long idleTime = millis() - lastActivityTime;
  unsigned long dimTimeoutMs = getDisplayDimTimeoutMs();
  unsigned long offTimeoutMs = getDisplayOffTimeoutMs();
  bool shouldDim = (dimTimeoutMs > 0 && idleTime > dimTimeoutMs);
  bool shouldTurnOff = (offTimeoutMs > 0 && dimTimeoutMs > 0 && idleTime > (dimTimeoutMs + offTimeoutMs));

  if (shouldTurnOff) {
    if (screenAwake) {
      screenAwake = false;
      displayDimmed = false;
      applyDisplayBacklight();
    }
  }
  else {
    if (!screenAwake) {
      screenAwake = true;
      displayDimmed = false;
      fullRedraw = true;
      uiNeedsRedraw = true;
    }

    bool nextDimmed = shouldDim && displaySleepBrightness < displayBrightness;
    if (displayDimmed != nextDimmed) {
      displayDimmed = nextDimmed;
      applyDisplayBacklight();
    } else {
      applyDisplayBacklight();
    }
  }
}

bool initAds1115() {
  Wire.beginTransmission(ADS1115_I2C_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println("ADS1115 not detected, using neutral stick inputs.");
    ads1115Ready = false;
    return false;
  }

  ads1115Ready = true;
  stickFilterInitialized = false;
  adsConsecutiveReadFails = 0;

  const int calibrationSamples = 8;
  int32_t sums[4] = { 0, 0, 0, 0 };

  for (int sample = 0; sample < calibrationSamples; sample++) {
    for (uint8_t channel = 0; channel < 4; channel++) {
      int16_t rawValue = 0;
      if (!readAds1115SingleEnded(channel, rawValue)) {
        Serial.println("ADS1115 calibration read failed, using neutral stick inputs.");
        ads1115Ready = false;
        return false;
      }
      sums[channel] += rawValue;
    }
  }

  for (uint8_t channel = 0; channel < 4; channel++) {
    adsStickCenter[channel] = (int16_t)(sums[channel] / calibrationSamples);
    filteredStickAxis[channel] = 0.0f;
  }

  Serial.printf(
    "ADS1115 ready. Centers A0=%d A1=%d A2=%d A3=%d\n",
    adsStickCenter[0], adsStickCenter[1], adsStickCenter[2], adsStickCenter[3]
  );
  return true;
}

bool readAds1115SingleEnded(uint8_t channel, int16_t &value) {
  if (channel > 3) return false;

  static const uint16_t muxByChannel[4] = {
    0x4000, // AIN0 vs GND
    0x5000, // AIN1 vs GND
    0x6000, // AIN2 vs GND
    0x7000  // AIN3 vs GND
  };

  uint16_t config =
    0x8000 |
    muxByChannel[channel] |
    ADS1115_GAIN_4_096V |
    ADS1115_MODE_SINGLE |
    ADS1115_DATA_RATE_860 |
    ADS1115_COMP_DISABLE;

  Wire.beginTransmission(ADS1115_I2C_ADDR);
  Wire.write(ADS1115_REG_CONFIG);
  Wire.write((uint8_t)(config >> 8));
  Wire.write((uint8_t)(config & 0xFF));
  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(2);

  Wire.beginTransmission(ADS1115_I2C_ADDR);
  Wire.write(ADS1115_REG_CONVERSION);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((int)ADS1115_I2C_ADDR, 2) != 2) {
    return false;
  }

  value = (int16_t)((Wire.read() << 8) | Wire.read());
  return true;
}

float normalizeStickAxis(int16_t raw, int16_t center) {
  int32_t delta = (int32_t)raw - (int32_t)center;
  int32_t estimatedTop =
    constrain((int32_t)center * 2, (int32_t)center + 1, (int32_t)ADS1115_JOYSTICK_MAX_COUNT);
  int32_t positiveSpan = max<int32_t>(1, estimatedTop - center);
  int32_t negativeSpan = max<int32_t>(1, center);
  int32_t span = (delta >= 0) ? positiveSpan : negativeSpan;

  if (span < 1) span = 1;

  float normalized = (float)delta / (float)span;
  normalized = constrain(normalized, -1.0f, 1.0f);

  float magnitude = fabs(normalized);
  if (magnitude <= STICK_DEADZONE) {
    return 0.0f;
  }

  float scaled = (magnitude - STICK_DEADZONE) / (1.0f - STICK_DEADZONE);
  scaled = constrain(scaled, 0.0f, 1.0f);
  return (normalized < 0.0f) ? -scaled : scaled;
}

void updateStickInputs(unsigned long now) {
  if (!ads1115Ready) {
    if (now - lastAdsReconnectAttemptMs >= ADS1115_RECONNECT_INTERVAL_MS) {
      lastAdsReconnectAttemptMs = now;
      if (initAds1115()) {
        Serial.println("ADS1115 recovered");
      }
    }

    for (int i = 0; i < CHANNEL_COUNT; i++) {
      inputChannels[i] = 0.0f;
      outputChannels[i] = 0.0f;
    }
    leftThrottle = 0.0f;
    rightThrottle = 0.0f;
    leftY = 0.0f;
    rightY = 0.0f;
    return;
  }

  if (now - lastStickSampleTime < STICK_SAMPLE_INTERVAL_MS) {
    inputChannels[0] = filteredStickAxis[0];
    inputChannels[1] = filteredStickAxis[1];
    inputChannels[2] = filteredStickAxis[3];
    inputChannels[3] = filteredStickAxis[2];
    updateChannelOutputs();
    return;
  }

  int16_t rawValues[4] = { 0, 0, 0, 0 };
  for (uint8_t channel = 0; channel < 4; channel++) {
    if (!readAds1115SingleEnded(channel, rawValues[channel])) {
      adsConsecutiveReadFails++;
      Serial.printf("ADS1115 read failed on A%d (streak %u)\n",
                    channel, (unsigned int)adsConsecutiveReadFails);
      if (adsConsecutiveReadFails >= ADS1115_MAX_CONSECUTIVE_READ_FAILS) {
        Serial.println("ADS1115 marked offline; using neutral inputs until reconnect");
        ads1115Ready = false;
      }
      return;
    }
  }
  adsConsecutiveReadFails = 0;

  for (uint8_t channel = 0; channel < 4; channel++) {
    float normalized = normalizeStickAxis(rawValues[channel], adsStickCenter[channel]);

    if (!stickFilterInitialized) {
      filteredStickAxis[channel] = normalized;
    } else {
      filteredStickAxis[channel] =
        (filteredStickAxis[channel] * (1.0f - STICK_FILTER_ALPHA)) +
        (normalized * STICK_FILTER_ALPHA);
    }
  }

  stickFilterInitialized = true;
  lastStickSampleTime = now;

  // Channel map from the external ADS1115 wiring:
  // CH1 = right X (A0), CH2 = right Y (A1), CH3 = left Y (A3), CH4 = left X (A2).
  inputChannels[0] = filteredStickAxis[0];
  inputChannels[1] = filteredStickAxis[1];
  inputChannels[2] = filteredStickAxis[3];
  inputChannels[3] = filteredStickAxis[2];
  updateChannelOutputs();
}

float getChannelTrimOffset(int channel) {
  switch (channel) {
    case 0:
      return models[activeModel].trimX[1] / 100.0f;
    case 1:
      return models[activeModel].trimY[1] / 100.0f;
    case 2:
      return models[activeModel].trimY[0] / 100.0f;
    case 3:
      return models[activeModel].trimX[0] / 100.0f;
    default:
      return 0.0f;
  }
}

const char* getChannelAxisName(uint8_t channel) {
  switch (channel) {
    case 0: return "RX";
    case 1: return "RY";
    case 2: return "LY";
    case 3: return "LX";
    default: return "--";
  }
}

float applyExpoCurve(float value, int expoPercent) {
  float x = constrain(value, -1.0f, 1.0f);
  float expo = constrain(expoPercent, 0, 100) / 100.0f;
  float curved = (x * (1.0f - expo)) + (x * x * x * expo);
  return constrain(curved, -1.0f, 1.0f);
}

bool updateEndpointSideLatch(int channel) {
  if (channel < 0 || channel >= CHANNEL_COUNT) return false;
  uint8_t previous = endpointSideLatch[channel];
  float value = inputChannels[channel];
  if (value > 0.12f) endpointSideLatch[channel] = 1;
  else if (value < -0.12f) endpointSideLatch[channel] = 0;
  return previous != endpointSideLatch[channel];
}

bool isEndpointHighSideSelected(int channel) {
  updateEndpointSideLatch(channel);
  if (channel < 0 || channel >= CHANNEL_COUNT) return true;
  return endpointSideLatch[channel] != 0;
}

float applyEndpointScaling(float value, int channel) {
  if (channel < 0 || channel >= CHANNEL_COUNT) return value;
  float normalized = constrain(value, -1.0f, 1.0f);
  float lowScale = getModelEndpointLowValue(activeModel, channel) / 100.0f;
  float highScale = getModelEndpointHighValue(activeModel, channel) / 100.0f;
  if (normalized >= 0.0f) {
    return constrain(normalized * highScale, -1.0f, 1.0f);
  }
  return constrain(normalized * lowScale, -1.0f, 1.0f);
}

void updateChannelOutputs() {
  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    float value = inputChannels[channel] + getChannelTrimOffset(channel);
    value = constrain(value, -1.0f, 1.0f);
    value = applyExpoCurve(value, getModelExpoValue(activeModel, channel));
    value = applyEndpointScaling(value, channel);

    if (models[activeModel].reverse[channel]) {
      value = -value;
    }

    outputChannels[channel] = value;
  }

  for (int mixIndex = 0; mixIndex < MIX_COUNT; mixIndex++) {
    MixData &mix = models[activeModel].mixes[mixIndex];
    if (!mix.enabled) continue;

    uint8_t source = getMixSource(mix);
    uint8_t destination = getMixDestination(mix);
    if (source >= CHANNEL_COUNT || destination >= CHANNEL_COUNT) continue;

    float mixAmount = outputChannels[source] * (mix.rate / 100.0f);
    mixAmount += mix.offset / 100.0f;
    outputChannels[destination] = constrain(outputChannels[destination] + mixAmount, -1.0f, 1.0f);
  }

  rightThrottle = outputChannels[0];
  rightY = outputChannels[1];
  leftY = outputChannels[2];
  leftThrottle = outputChannels[3];
}

void updateBatteryState() {
  unsigned long now = millis();
  if (now - lastBatterySampleTime < BATTERY_SAMPLE_INTERVAL_MS) return;
  lastBatterySampleTime = now;

  uint32_t totalMv = 0;
  const int sampleCount = 8;
  for (int i = 0; i < sampleCount; i++) {
    totalMv += analogReadMilliVolts(BATTERY_ADC_PIN);
  }

  float adcVoltage = ((float)totalMv / (float)sampleCount) / 1000.0f;
  float measuredBatteryVoltage = adcVoltage * BATTERY_DIVIDER_RATIO;

  if (!batteryFilterInitialized) {
    batteryFilteredVoltage = measuredBatteryVoltage;
    batteryChargeWindowVoltage = measuredBatteryVoltage;
    batteryChargeWindowStart = now;
    batteryFilterInitialized = true;
  } else {
    batteryFilteredVoltage = (batteryFilteredVoltage * 0.82f) + (measuredBatteryVoltage * 0.18f);
  }

  transmitterBatteryVoltage = batteryFilteredVoltage;
  batteryPresent = (transmitterBatteryVoltage >= BATTERY_PRESENT_MIN_V);

  if (!batteryPresent) {
    batteryCharging = false;
    batteryChargingHoldUntil = 0;
    batteryLevel = 0;
    batteryChargeWindowVoltage = transmitterBatteryVoltage;
    batteryChargeWindowStart = now;
    return;
  }

  batteryLevel = constrain((int)roundf(
    ((transmitterBatteryVoltage - BATTERY_EMPTY_V) / (BATTERY_FULL_V - BATTERY_EMPTY_V)) * 100.0f), 0, 100);

  if (batteryChargeWindowStart == 0) {
    batteryChargeWindowStart = now;
    batteryChargeWindowVoltage = transmitterBatteryVoltage;
  }

  if (now - batteryChargeWindowStart >= BATTERY_CHARGE_WINDOW_MS) {
    float deltaV = transmitterBatteryVoltage - batteryChargeWindowVoltage;
    if (deltaV >= BATTERY_CHARGING_DELTA_V) {
      batteryChargingHoldUntil = now + BATTERY_CHARGING_HOLD_MS;
    }
    batteryChargeWindowStart = now;
    batteryChargeWindowVoltage = transmitterBatteryVoltage;
  }

  batteryCharging = (batteryChargingHoldUntil > now);
}

// !!!---- DRAWING ----!!!

// ===== THICK DRAW HELPERS =====
void drawThickLine(int x1, int y1, int x2, int y2, uint16_t c) {
  tft.drawLine(x1, y1, x2, y2, c);
  tft.drawLine(x1+1, y1, x2+1, y2, c);
}

void drawThickCircle(int x, int y, int r, uint16_t c) {
  tft.drawCircle(x, y, r, c);
  tft.drawCircle(x, y, r-1, c);
}

void drawThickRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
  tft.drawRoundRect(x, y, w, h, r, c);
  tft.drawRoundRect(x+1, y, w-2, h, r-1, c);
}

void drawTopLeftBevelHighlight(int x, int y, int w, int h, int radius,
                               int inset, uint16_t highlightColor) {
  int topRun = w - (2 * radius);
  int sideRun = h - (2 * radius);
  const int highlightLayers = 3;

  for (int layer = 0; layer < highlightLayers; layer++) {
    int localInset = inset + layer;
    int cornerRadius = radius - localInset;
    if (cornerRadius <= 0) {
      break;
    }

    float layerFade = 1.0f - (layer * 0.24f);
    int cornerSteps = max(8, cornerRadius * 5);
    int lastPx = -32768;
    int lastPy = -32768;

    for (int step = 1; step < cornerSteps; step++) {
      float t = (float)step / (float)cornerSteps;
      float angle = PI + (t * (PI * 0.5f));
      int px = x + radius + (int)roundf(cosf(angle) * cornerRadius);
      int py = y + radius + (int)roundf(sinf(angle) * cornerRadius);

      if (px != lastPx || py != lastPy) {
        tft.drawPixel(px, py, fadeColor(highlightColor, layerFade));
        lastPx = px;
        lastPy = py;
      }
    }

    if (topRun > 0) {
      for (int i = 0; i < topRun; i++) {
        float fade = 1.0f - ((float)i / (float)topRun);
        tft.drawPixel(
          x + radius + i,
          y + localInset,
          fadeColor(highlightColor, layerFade * fade)
        );
      }
    }

    if (sideRun > 0) {
      for (int i = 0; i < sideRun; i++) {
        float fade = 1.0f - ((float)i / (float)sideRun);
        tft.drawPixel(
          x + localInset,
          y + radius + i,
          fadeColor(highlightColor, layerFade * fade)
        );
      }
    }
  }
}

void drawGradientControl(int x, int y, int w, int h, int radius,
                         uint16_t baseColor, uint16_t outlineColor) {
  for (int i = 0; i < h; i++) {
    int shade = map(i, 0, h, 18, -18);

    uint8_t r = (baseColor >> 11) & 0x1F;
    uint8_t g = (baseColor >> 5)  & 0x3F;
    uint8_t b = baseColor & 0x1F;

    int nr = constrain(r + shade / 2, 0, 31);
    int ng = constrain(g + shade,     0, 63);
    int nb = constrain(b + shade / 2, 0, 31);

    uint16_t lineColor = (nr << 11) | (ng << 5) | nb;

    int xOffset = 0;
    if (radius > 0) {
      if (i < radius) {
        int dy = radius - i;
        xOffset = radius - sqrt(radius * radius - dy * dy);
      }
      else if (i >= h - radius) {
        int dy = i - (h - radius);
        xOffset = radius - sqrt(radius * radius - dy * dy);
      }
    }

    tft.drawFastHLine(x + xOffset, y + i, w - (2 * xOffset), lineColor);
  }

  tft.drawRoundRect(x, y, w, h, radius, outlineColor);

  int inset = (radius >= 8) ? 5 : 3;
  drawTopLeftBevelHighlight(x, y, w, h, radius, inset, COLOR_ACCENT_HI);

  tft.drawFastHLine(x + radius, y + h - 2, w - (2 * radius), TFT_DARKGREY);
  tft.drawFastVLine(x + w - 2, y + radius, h - (2 * radius), TFT_DARKGREY);
}

// STATIC 3D EFFECT
void drawButtonBubble(int x, int y, int w, int h,
                      const char* label,
                      bool pressed,
                      bool selected,
                      int textOffset = 40)
{
  // Keep button label sizing consistent regardless of prior screen/font state.
  tft.setTextFont(2);

  int drawY = y + (pressed ? 2 : 0);

  bool isBackNavIcon = (strcmp(label, "BACK") == 0);
  bool isCompactMenuIcon =
    (strcmp(label, "Reverse") == 0) ||
    (strcmp(label, "Trim") == 0) ||
    (strcmp(label, "Failsafe") == 0) ||
    (strcmp(label, "End Points") == 0) ||
    (strcmp(label, "Expo") == 0) ||
    (strcmp(label, "Rates") == 0) ||
    (strcmp(label, "Protocol") == 0) ||
    (strcmp(label, "Model Name") == 0) ||
    (strcmp(label, "Drive Type") == 0) ||
    (strcmp(label, "Mixing") == 0);

  int iconScale = 2;   // default
  int iconUnit = 24;

  // ===== SCALE OVERRIDE =====
  if (isBackNavIcon) {
    iconScale = 1;
    iconUnit = 18;
  }
  else if (isCompactMenuIcon) {
    iconScale = 1;
    iconUnit = 24;
  }

  int radius = BTN_RADIUS;
  int bevelInset = 5;
  int iconSafeLeftInset = max(14, radius + 3);
  int iconSafeTopInset = bevelInset + 7;
  int iconSafeBottomInset = 8;
  int iconVerticalNudge = 0;
  int iconSize = iconUnit * iconScale;
  int defaultIconX = isBackNavIcon ? (x + 10) : (x + 20);
  int iconX = max(defaultIconX, x + iconSafeLeftInset);

  if (isBackNavIcon) {
    iconSafeBottomInset = 12;
    iconVerticalNudge = -4;
  }
  else if (isCompactMenuIcon) {
    iconSafeBottomInset = 11;
    iconVerticalNudge = -3;
  }

  int iconMinY = drawY + iconSafeTopInset;
  int iconMaxY = drawY + h - iconSafeBottomInset - iconSize;
  int iconY = drawY + (h / 2) - (iconSize / 2) + iconVerticalNudge;
  if (iconMaxY >= iconMinY) {
    iconY = constrain(iconY, iconMinY, iconMaxY);
  } else {
    iconY = iconMinY;
  }

  uint16_t baseColor;
  uint16_t textColor;
  uint16_t outlineColor;

  if (pressed) {
    baseColor = COLOR_PANEL;
    textColor = COLOR_TEXT;
    outlineColor = COLOR_ACCENT;
  } 
  else if (selected) {
    baseColor = COLOR_ACCENT;
    textColor = COLOR_BG;
    outlineColor = COLOR_ACCENT_HI;
  } 
  else {
    baseColor = COLOR_PANEL;
    textColor = COLOR_TEXT;
    outlineColor = COLOR_ACCENT;
  }

  // ===== GRADIENT BASE =====
  for (int i = 0; i < h; i++) {

    int shade = map(i, 0, h, 18, -18);

    uint8_t r = (baseColor >> 11) & 0x1F;
    uint8_t g = (baseColor >> 5)  & 0x3F;
    uint8_t b = baseColor & 0x1F;

    int nr = constrain(r + shade / 2, 0, 31);
    int ng = constrain(g + shade,     0, 63);
    int nb = constrain(b + shade / 2, 0, 31);

    uint16_t lineColor = (nr << 11) | (ng << 5) | nb;

    int xOffset = 0;

    if (i < radius) {
      int dy = radius - i;
      xOffset = radius - sqrt(radius * radius - dy * dy);
    }
    else if (i >= h - radius) {
      int dy = i - (h - radius);
      xOffset = radius - sqrt(radius * radius - dy * dy);
    }

    tft.drawFastHLine(
      x + xOffset,
      drawY + i,
      w - (2 * xOffset),
      lineColor
    );
  }

  // ===== OUTLINE =====
  tft.drawRoundRect(x, drawY, w, h, radius, outlineColor);

  int inset = 5;
  drawTopLeftBevelHighlight(x, drawY, w, h, radius, inset, COLOR_ACCENT_HI);

  // ===== SHADOW =====
  tft.drawFastHLine(x + radius, drawY + h - 2, w - (2 * radius), TFT_DARKGREY);
  tft.drawFastVLine(x + w - 2, drawY + radius, h - (2 * radius), TFT_DARKGREY);

  uint16_t iconColor = selected ? COLOR_BG : COLOR_ACCENT;

  // ===== ICONS =====
  if (strcmp(label, "Controller") == 0) {
    drawControllerIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Model") == 0) {
    drawCarIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "BACK") == 0) {
    if (currentScreen == SCREEN_MENU) {
      drawHomeIcon(iconX, iconY, iconScale, iconColor);
    }
    else {
      drawBackIcon(iconX, iconY, iconScale, iconColor);
    }
  }
  else if (strcmp(label, "Reverse") == 0) {
    drawReverseIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Trim") == 0) {
    drawTrimIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Failsafe") == 0) {
    drawFailsafeIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "End Points") == 0) {
    drawEndpointIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Expo") == 0) {
    drawExpoIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Rates") == 0) {
    drawRatesIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Protocol") == 0) {
    drawProtocolIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Model Name") == 0) {
    drawModelNameIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Drive Type") == 0) {
    drawDriveTypeIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Mixing") == 0) {
    drawMixingIcon(iconX, iconY, iconScale, iconColor);
  }

  // ===== TEXT =====
  int16_t tx = x + textOffset;
  int16_t ty = drawY + (h / 2) - 7;

  if (textOffset < 0) {
    tx = x + ((w - tft.textWidth(label)) / 2);
  }

  tft.setTextColor(textColor);

  tft.setCursor(tx, ty);
  tft.print(label);
  tft.setCursor(tx + 1, ty);
  tft.print(label);
}

void drawMenuScreen() {
  tft.fillScreen(COLOR_BG);

  // ===== BACK BUTTON (BOTTOM LEFT) =====
  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y,
    BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  // ===== MAIN BUTTONS =====
  drawButtonBubble(
    CTRL_BTN_X, 60,
    CTRL_BTN_W, CTRL_BTN_H,
    "Controller",
    pressedButton == BTN_CTRL,
    dpadFocusVisible && selectedButton == BTN_CTRL,
    100);

  drawButtonBubble(
    MODEL_BTN_X, 150,
    MODEL_BTN_W, MODEL_BTN_H,
    "Model",
    pressedButton == BTN_MODEL,
    dpadFocusVisible && selectedButton == BTN_MODEL,
    100);

  drawButtonBubble(
    DISPLAY_MENU_BTN_X, DISPLAY_MENU_BTN_Y,
    DISPLAY_MENU_BTN_W, DISPLAY_MENU_BTN_H,
    "",
    pressedButton == BTN_DISPLAY_SETTINGS,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_SETTINGS,
    -1);
  int gearSize = 22;
  int gearDrawY = DISPLAY_MENU_BTN_Y + (pressedButton == BTN_DISPLAY_SETTINGS ? 2 : 0);
  drawGearIcon(
    DISPLAY_MENU_BTN_X + max(14, BTN_RADIUS + 3),
    gearDrawY + max(12, 5 + 7),
    gearSize,
    (dpadFocusVisible && selectedButton == BTN_DISPLAY_SETTINGS) ? COLOR_BG : COLOR_ACCENT);

  drawGameMenuButton();
}

void drawGameMenuButton() {
  drawButtonBubble(
    GAME_BTN_X, GAME_BTN_Y,
    GAME_BTN_W, GAME_BTN_H,
    "PLAY",
    pressedButton == BTN_GAME,
    dpadFocusVisible && selectedButton == BTN_GAME,
    -1);
}

void drawDisplaySettingsRow(int y, const char* label, const char* valueText,
                            bool minusPressed, bool minusSelected,
                            bool plusPressed, bool plusSelected,
                            uint16_t previewColor, int previewWidth) {
  int panelY = y;
  int panelH = DISPLAY_SETTINGS_ROW_H;
  int contentCenterX = DISPLAY_SETTINGS_ROW_X + (DISPLAY_SETTINGS_ROW_W / 2);

  tft.fillRoundRect(DISPLAY_SETTINGS_ROW_X, panelY, DISPLAY_SETTINGS_ROW_W, panelH, 12, COLOR_PANEL);
  tft.drawRoundRect(DISPLAY_SETTINGS_ROW_X, panelY, DISPLAY_SETTINGS_ROW_W, panelH, 12, COLOR_ACCENT);

  tft.setTextColor(COLOR_TEXT);
  tft.setTextFont(2);
  tft.drawCentreString(label, contentCenterX, panelY + 3, 2);
  tft.drawCentreString(valueText, contentCenterX, panelY + 15, 2);

  int previewX = 72;
  int previewY = panelY + 31;
  int previewH = 5;
  int previewTrackW = 96;
  tft.drawRoundRect(previewX, previewY, previewTrackW, previewH, 3, COLOR_ACCENT_HI);
  if (previewWidth > 0) {
    tft.fillRoundRect(previewX + 1, previewY + 1, min(previewWidth, previewTrackW - 2), previewH - 2, 3, previewColor);
  }

  drawButtonBubble(
    DISPLAY_SETTINGS_MINUS_X, panelY + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
    DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H,
    "-",
    minusPressed,
    minusSelected,
    -1);

  drawButtonBubble(
    DISPLAY_SETTINGS_PLUS_X, panelY + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
    DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H,
    "+",
    plusPressed,
    plusSelected,
    -1);
}

void drawDisplaySettingsScreen() {
  char dimTimeoutText[16];
  char offTimeoutText[16];
  char brightnessText[16];
  char sleepText[16];
  int brightnessPercent = (displayBrightness * 100) / 255;
  int sleepPercent = (displaySleepBrightness * 100) / 255;
  snprintf(brightnessText, sizeof(brightnessText), "%d%%", brightnessPercent);
  if (displaySleepBrightness == 0) {
    snprintf(sleepText, sizeof(sleepText), "Off");
  } else {
    snprintf(sleepText, sizeof(sleepText), "%d%%", sleepPercent);
  }

  tft.fillScreen(COLOR_BG);
  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y,
    BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

  tft.drawCentreString("Display Settings", 120, 38, 2);
  tft.setTextColor(fadeColor(COLOR_TEXT, 0.65f));
  tft.drawCentreString("Changes save automatically", 120, 50, 2);

  drawDisplaySettingsRow(
    DISPLAY_SETTINGS_BRIGHTNESS_Y,
    "Brightness",
    brightnessText,
    pressedButton == BTN_DISPLAY_BRIGHTNESS_DEC,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_BRIGHTNESS_DEC,
    pressedButton == BTN_DISPLAY_BRIGHTNESS_INC,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_BRIGHTNESS_INC,
    COLOR_ACCENT,
    map(displayBrightness, 0, 255, 0, 94));

  drawDisplaySettingsRow(
    DISPLAY_SETTINGS_TIMEOUT_Y,
    "Dim Timeout",
    formatDisplayTimeoutValue(displayTimeoutIndex, displayTimeoutOptionsSec, displayTimeoutOptionCount,
                              dimTimeoutText, sizeof(dimTimeoutText)),
    pressedButton == BTN_DISPLAY_TIMEOUT_DEC,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_TIMEOUT_DEC,
    pressedButton == BTN_DISPLAY_TIMEOUT_INC,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_TIMEOUT_INC,
    COLOR_ACCENT_HI,
    map(min((int)displayTimeoutIndex, (int)displayTimeoutOptionCount - 1), 0, displayTimeoutOptionCount - 1, 12, 94));

  drawDisplaySettingsRow(
    DISPLAY_SETTINGS_OFF_TIMEOUT_Y,
    "Off Delay",
    formatDisplayTimeoutValue(displayOffTimeoutIndex, displayOffTimeoutOptionsSec, displayOffTimeoutOptionCount,
                              offTimeoutText, sizeof(offTimeoutText)),
    pressedButton == BTN_DISPLAY_OFF_TIMEOUT_DEC,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_OFF_TIMEOUT_DEC,
    pressedButton == BTN_DISPLAY_OFF_TIMEOUT_INC,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_OFF_TIMEOUT_INC,
    fadeColor(COLOR_ACCENT_HI, 0.70f),
    map(min((int)displayOffTimeoutIndex, (int)displayOffTimeoutOptionCount - 1), 0, displayOffTimeoutOptionCount - 1, 12, 94));

  drawDisplaySettingsRow(
    DISPLAY_SETTINGS_SLEEP_Y,
    "Dim Level",
    sleepText,
    pressedButton == BTN_DISPLAY_SLEEP_DEC,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_SLEEP_DEC,
    pressedButton == BTN_DISPLAY_SLEEP_INC,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_SLEEP_INC,
    fadeColor(COLOR_ACCENT, 0.70f),
    map(displaySleepBrightness, 0, 255, 0, 94));

  drawButtonBubble(
    DISPLAY_SETTINGS_THEME_X, DISPLAY_SETTINGS_THEME_Y,
    DISPLAY_SETTINGS_THEME_W, DISPLAY_SETTINGS_THEME_H,
    getThemeToggleLabel(),
    pressedButton == BTN_DISPLAY_THEME_TOGGLE,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_THEME_TOGGLE,
    -1);

  drawButtonBubble(
    DISPLAY_SETTINGS_SLEEP_NOW_X, DISPLAY_SETTINGS_SLEEP_NOW_Y,
    DISPLAY_SETTINGS_SLEEP_NOW_W, DISPLAY_SETTINGS_SLEEP_NOW_H,
    "Sleep Now",
    pressedButton == BTN_DISPLAY_SLEEP_NOW,
    dpadFocusVisible && selectedButton == BTN_DISPLAY_SLEEP_NOW,
    -1);
}

void drawGearIcon(int x, int y, int size, uint16_t iconColor) {
  int cx = x + (size / 2);
  int cy = y + (size / 2);
  int outerR = max(6, size / 2 - 3);
  int innerR = max(2, size / 4);
  int toothLen = max(2, size / 6);

  for (int i = 0; i < 8; i++) {
    float angle = (float)i * 0.7853982f;
    int sx = cx + (int)(cosf(angle) * (innerR + 2));
    int sy = cy + (int)(sinf(angle) * (innerR + 2));
    int ex = cx + (int)(cosf(angle) * (outerR + toothLen));
    int ey = cy + (int)(sinf(angle) * (outerR + toothLen));
    tft.drawLine(sx, sy, ex, ey, iconColor);
  }

  tft.drawCircle(cx, cy, outerR, iconColor);
  tft.drawCircle(cx, cy, outerR - 1, iconColor);
  tft.fillCircle(cx, cy, innerR, iconColor);
  tft.fillCircle(cx, cy, max(1, innerR - 3), COLOR_PANEL);
}

static float wrapBattleAngle(float angle) {
  const float twoPi = 6.2831853f;
  while (angle < 0.0f) angle += twoPi;
  while (angle >= twoPi) angle -= twoPi;
  return angle;
}

static int countAliveBattleEnemies() {
  int count = 0;
  for (int i = 0; i < BZ_MAX_ENEMIES; i++) {
    if (spaceEnemies[i].alive) count++;
  }
  return count;
}

static bool battleWorldToScreen(float wx, float wy, float &sx, float &scale, float &forward) {
  float dx = wx - spacePlayerX;
  float dy = wy - spacePlayerY;
  float c = cosf(spacePlayerHeading);
  float s = sinf(spacePlayerHeading);

  forward = (c * dx) + (s * dy);
  if (forward <= 1.0f) return false;

  float side = (-s * dx) + (c * dy);
  sx = 120.0f + ((side / forward) * BZ_FOV_SCALE);
  if (sx < -26.0f || sx > 266.0f) return false;

  scale = constrain(220.0f / forward, 4.0f, 92.0f);
  return true;
}

static bool battlePositionBlocked(float x, float y) {
  for (int i = 0; i < BZ_MAX_OBSTACLES; i++) {
    float dx = x - spaceObstacles[i].x;
    float dy = y - spaceObstacles[i].y;
    if ((dx * dx) + (dy * dy) < (spaceObstacles[i].radius * spaceObstacles[i].radius)) {
      return true;
    }
  }
  return false;
}

static void spawnBattleWave(unsigned long now) {
  int targetEnemies = min(2 + (spaceWave / 2), BZ_MAX_ENEMIES);
  for (int i = 0; i < BZ_MAX_ENEMIES; i++) {
    spaceEnemies[i].alive = false;
  }

  for (int i = 0; i < targetEnemies; i++) {
    float angle = random(0, 628) / 100.0f;
    float dist = 28.0f + (random(0, 380) / 10.0f);
    spaceEnemies[i].x = spacePlayerX + (cosf(angle) * dist);
    spaceEnemies[i].y = spacePlayerY + (sinf(angle) * dist);
    spaceEnemies[i].alive = true;
    spaceEnemies[i].type = BZ_ENEMY_TANK;
    if (spaceWave >= 3 && (i == 0) && ((spaceWave % 2) == 1)) {
      spaceEnemies[i].type = BZ_ENEMY_SUPER;
    } else if (spaceWave >= 4 && i == (targetEnemies - 1) && (random(0, 100) < 35)) {
      spaceEnemies[i].type = BZ_ENEMY_UFO;
    }
    spaceEnemies[i].nextShotAt = now + random(1100, 2400);
  }

  spaceNextMissileAt = now + random(6000, 11000);
}

void resetSpaceGame() {
  spacePlayerX = 0.0f;
  spacePlayerY = 0.0f;
  spacePlayerHeading = 0.0f;
  spaceScore = 0;
  spaceLives = 3;
  spaceWave = 1;
  spaceGameOver = false;
  spaceWaveCleared = false;
  spaceGameStarted = false;
  spacePlayerShotActive = false;
  spaceEnemyShotActive = false;
  spaceMissileActive = false;
  spacePlayerShotDistance = 0.0f;
  lastSpaceFrameTime = millis();
  lastSpaceShotTime = 0;
  lastSpaceRespawnAt = 0;
  spaceMuzzleFlashUntil = 0;

  for (int i = 0; i < BZ_MAX_OBSTACLES; i++) {
    spaceObstacles[i].pyramid = (i % 2 == 0);
    spaceObstacles[i].radius = spaceObstacles[i].pyramid ? 3.8f : 3.2f;
    float angle = random(0, 628) / 100.0f;
    float dist = 22.0f + (random(0, 820) / 10.0f);
    spaceObstacles[i].x = cosf(angle) * dist;
    spaceObstacles[i].y = sinf(angle) * dist;
  }

  spawnBattleWave(lastSpaceFrameTime);
}

bool spaceEnemiesRemaining() {
  for (int i = 0; i < BZ_MAX_ENEMIES; i++) {
    if (spaceEnemies[i].alive) return true;
  }
  return false;
}

void moveSpacePlayer(int delta) {
  float direction = (delta < 0) ? -1.0f : 1.0f;
  spacePlayerHeading = wrapBattleAngle(spacePlayerHeading + (direction * BZ_TURN_SPEED));
}

void fireSpacePlayerBullet() {
  if (spaceGameOver) return;
  if (millis() - lastSpaceShotTime < BZ_FIRE_COOLDOWN_MS) return;
  if (spacePlayerShotActive) return;

  spacePlayerShotActive = true;
  spacePlayerShotDistance = 0.0f;
  spacePlayerShotX = spacePlayerX;
  spacePlayerShotY = spacePlayerY;
  spacePlayerShotVX = cosf(spacePlayerHeading) * BZ_SHOT_SPEED;
  spacePlayerShotVY = sinf(spacePlayerHeading) * BZ_SHOT_SPEED;
  lastSpaceShotTime = millis();
  spaceMuzzleFlashUntil = millis() + 90;
}

void fireSpaceEnemyBullet() {
  if (spaceGameOver || !spaceEnemiesRemaining() || spaceEnemyShotActive) return;

  int bestIndex = -1;
  float bestDist2 = 1e9f;
  for (int i = 0; i < BZ_MAX_ENEMIES; i++) {
    if (!spaceEnemies[i].alive) continue;
    if (spaceEnemies[i].type == BZ_ENEMY_UFO) continue;
    float dx = spacePlayerX - spaceEnemies[i].x;
    float dy = spacePlayerY - spaceEnemies[i].y;
    float c = cosf(spacePlayerHeading);
    float s = sinf(spacePlayerHeading);
    float forward = (c * dx) + (s * dy);
    if (forward < 6.0f) continue;
    float d2 = (dx * dx) + (dy * dy);
    if (d2 < bestDist2) {
      bestDist2 = d2;
      bestIndex = i;
    }
  }
  if (bestIndex < 0) return;

  float dx = spacePlayerX - spaceEnemies[bestIndex].x;
  float dy = spacePlayerY - spaceEnemies[bestIndex].y;
  float mag = sqrtf((dx * dx) + (dy * dy));
  if (mag < 0.001f) return;

  spaceEnemyShotActive = true;
  spaceEnemyShotX = spaceEnemies[bestIndex].x;
  spaceEnemyShotY = spaceEnemies[bestIndex].y;
  float shotSpeed = (spaceEnemies[bestIndex].type == BZ_ENEMY_SUPER) ? 1.2f : 0.82f;
  spaceEnemyShotVX = (dx / mag) * shotSpeed;
  spaceEnemyShotVY = (dy / mag) * shotSpeed;
}

void updateSpaceGame(unsigned long now, bool leftPressed, bool rightPressed, bool firePressed, bool exitPressed) {
  if (currentScreen != SCREEN_SPACE_GAME) return;
  if (exitPressed) {
    setScreen(SCREEN_MENU);
    return;
  }

  if (now - lastSpaceFrameTime < SPACE_FRAME_INTERVAL_MS) return;
  lastSpaceFrameTime = now;

  if (spaceGameOver) {
    if (firePressed) {
      resetSpaceGame();
      uiNeedsRedraw = true;
    }
    return;
  }

  if (leftPressed || rightPressed || firePressed) {
    spaceGameStarted = true;
  }

  // Battlezone mode uses the same "single-stick tank" feel:
  // RX steers, RY moves forward/reverse.
  float turnInput = outputChannels[0];
  float throttleInput = outputChannels[1];

  if (leftPressed && !rightPressed) turnInput -= 0.9f;
  if (rightPressed && !leftPressed) turnInput += 0.9f;
  turnInput = constrain(turnInput, -1.0f, 1.0f);
  throttleInput = constrain(throttleInput, -1.0f, 1.0f);

  spacePlayerHeading = wrapBattleAngle(spacePlayerHeading + (turnInput * BZ_TURN_SPEED));

  float step = throttleInput * BZ_PLAYER_SPEED;
  float nextX = spacePlayerX + (cosf(spacePlayerHeading) * step);
  float nextY = spacePlayerY + (sinf(spacePlayerHeading) * step);
  if (!battlePositionBlocked(nextX, nextY)) {
    spacePlayerX = nextX;
    spacePlayerY = nextY;
  }

  if (firePressed) fireSpacePlayerBullet();

  if (spacePlayerShotActive) {
    spacePlayerShotX += spacePlayerShotVX;
    spacePlayerShotY += spacePlayerShotVY;
    spacePlayerShotDistance += BZ_SHOT_SPEED;

    if (spaceMissileActive) {
      float mdx = spacePlayerShotX - spaceMissileX;
      float mdy = spacePlayerShotY - spaceMissileY;
      if ((mdx * mdx) + (mdy * mdy) <= 2.6f) {
        spaceMissileActive = false;
        spacePlayerShotActive = false;
        spaceScore += 1000;
      }
    }

    for (int i = 0; i < BZ_MAX_ENEMIES; i++) {
      if (!spacePlayerShotActive) break;
      if (!spaceEnemies[i].alive) continue;
      float dx = spacePlayerShotX - spaceEnemies[i].x;
      float dy = spacePlayerShotY - spaceEnemies[i].y;
      if ((dx * dx) + (dy * dy) <= 3.2f) {
        spaceEnemies[i].alive = false;
        if (spaceEnemies[i].type == BZ_ENEMY_SUPER) spaceScore += 3000;
        else if (spaceEnemies[i].type == BZ_ENEMY_UFO) spaceScore += 5000;
        else spaceScore += 1000;
        spacePlayerShotActive = false;
        break;
      }
    }

    if (spacePlayerShotDistance >= BZ_SHOT_RANGE) {
      spacePlayerShotActive = false;
    }
  }

  for (int i = 0; i < BZ_MAX_ENEMIES; i++) {
    if (!spaceEnemies[i].alive) continue;

    float toPlayerX = spacePlayerX - spaceEnemies[i].x;
    float toPlayerY = spacePlayerY - spaceEnemies[i].y;
    float distSq = (toPlayerX * toPlayerX) + (toPlayerY * toPlayerY);
    float dist = sqrtf(distSq);

    if (spaceEnemies[i].type == BZ_ENEMY_UFO) {
      float orbit = now * 0.0012f + i;
      spaceEnemies[i].x += cosf(orbit) * 0.05f;
      spaceEnemies[i].y += sinf(orbit) * 0.03f;
      if (dist > 55.0f) {
        spaceEnemies[i].x += (toPlayerX / max(dist, 0.001f)) * 0.05f;
        spaceEnemies[i].y += (toPlayerY / max(dist, 0.001f)) * 0.05f;
      }
      continue;
    }

    if (dist > 7.0f) {
      float stepEnemy = (spaceEnemies[i].type == BZ_ENEMY_SUPER)
                          ? (0.028f + (0.004f * min(spaceWave, 6)))
                          : (0.015f + (0.003f * min(spaceWave, 6)));
      float nx = spaceEnemies[i].x + (toPlayerX / max(dist, 0.001f)) * stepEnemy;
      float ny = spaceEnemies[i].y + (toPlayerY / max(dist, 0.001f)) * stepEnemy;
      if (!battlePositionBlocked(nx, ny)) {
        spaceEnemies[i].x = nx;
        spaceEnemies[i].y = ny;
      }
    }

    if (now >= spaceEnemies[i].nextShotAt && dist < 65.0f) {
      fireSpaceEnemyBullet();
      spaceEnemies[i].nextShotAt = now + ((spaceEnemies[i].type == BZ_ENEMY_SUPER)
                                          ? random(700, 1500)
                                          : random(1100, 2400));
    }
  }

  if (spaceEnemyShotActive) {
    spaceEnemyShotX += spaceEnemyShotVX;
    spaceEnemyShotY += spaceEnemyShotVY;
    float hitDx = spaceEnemyShotX - spacePlayerX;
    float hitDy = spaceEnemyShotY - spacePlayerY;
    if ((hitDx * hitDx) + (hitDy * hitDy) <= 2.2f) {
      spaceEnemyShotActive = false;
      if (spaceLives > 0) spaceLives--;
      if (spaceLives <= 0) {
        spaceGameOver = true;
      } else {
        spacePlayerX = 0.0f;
        spacePlayerY = 0.0f;
        spacePlayerHeading = 0.0f;
      }
    } else if ((hitDx * hitDx) + (hitDy * hitDy) > 6400.0f) {
      spaceEnemyShotActive = false;
    }
  }

  if (!spaceMissileActive && now >= spaceNextMissileAt) {
    float angle = random(0, 628) / 100.0f;
    float dist = 85.0f;
    spaceMissileX = spacePlayerX + cosf(angle) * dist;
    spaceMissileY = spacePlayerY + sinf(angle) * dist;
    float dx = spacePlayerX - spaceMissileX;
    float dy = spacePlayerY - spaceMissileY;
    float mag = sqrtf((dx * dx) + (dy * dy));
    if (mag > 0.001f) {
      spaceMissileVX = (dx / mag) * BZ_MISSILE_SPEED;
      spaceMissileVY = (dy / mag) * BZ_MISSILE_SPEED;
      spaceMissileActive = true;
    }
    spaceNextMissileAt = now + random(9000, 15000);
  }

  if (spaceMissileActive) {
    float targetVX = spacePlayerX - spaceMissileX;
    float targetVY = spacePlayerY - spaceMissileY;
    float targetMag = sqrtf((targetVX * targetVX) + (targetVY * targetVY));
    if (targetMag > 0.001f) {
      targetVX = (targetVX / targetMag) * BZ_MISSILE_SPEED;
      targetVY = (targetVY / targetMag) * BZ_MISSILE_SPEED;
      spaceMissileVX += (targetVX - spaceMissileVX) * BZ_MISSILE_TURN;
      spaceMissileVY += (targetVY - spaceMissileVY) * BZ_MISSILE_TURN;
    }

    spaceMissileX += spaceMissileVX;
    spaceMissileY += spaceMissileVY;

    float mdx = spaceMissileX - spacePlayerX;
    float mdy = spaceMissileY - spacePlayerY;
    if ((mdx * mdx) + (mdy * mdy) <= 2.6f) {
      spaceMissileActive = false;
      if (spaceLives > 0) spaceLives--;
      if (spaceLives <= 0) {
        spaceGameOver = true;
      } else {
        spacePlayerX = 0.0f;
        spacePlayerY = 0.0f;
        spacePlayerHeading = 0.0f;
      }
    } else if ((mdx * mdx) + (mdy * mdy) > 18000.0f) {
      spaceMissileActive = false;
    }
  }

  if (!spaceGameOver && countAliveBattleEnemies() == 0 && !spaceWaveCleared) {
    spaceWaveCleared = true;
    lastSpaceRespawnAt = now + BZ_RESPAWN_DELAY_MS;
  }

  if (!spaceGameOver && spaceWaveCleared && now >= lastSpaceRespawnAt) {
    spaceWave++;
    spawnBattleWave(now);
    spaceWaveCleared = false;
    spaceEnemyShotActive = false;
    spacePlayerShotActive = false;
    spaceMissileActive = false;
  }

  uiNeedsRedraw = true;
}

void handleSpaceGameTouch(int x, int y) {
  if (isInside(x, y, SPACE_EXIT_X, SPACE_EXIT_Y, SPACE_EXIT_W, SPACE_EXIT_H)) {
    queueScreenButton(BTN_BACK, SCREEN_MENU);
    return;
  }

  if (spaceGameOver) {
    resetSpaceGame();
    waitingForRelease = true;
    uiNeedsRedraw = true;
    return;
  }

  if (isInside(x, y, SPACE_LEFT_BTN_X, SPACE_CONTROL_Y, SPACE_CTRL_BTN_W, SPACE_CTRL_BTN_H)) {
    spaceGameStarted = true;
    moveSpacePlayer(-1);
    waitingForRelease = true;
  }
  else if (isInside(x, y, SPACE_RIGHT_BTN_X, SPACE_CONTROL_Y, SPACE_CTRL_BTN_W, SPACE_CTRL_BTN_H)) {
    spaceGameStarted = true;
    moveSpacePlayer(1);
    waitingForRelease = true;
  }
  else {
    spaceGameStarted = true;
    fireSpacePlayerBullet();
    waitingForRelease = true;
  }

  uiNeedsRedraw = true;
}

void drawSpaceGameStatic() {
  tft.fillScreen(COLOR_BG);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);

  drawButtonBubble(SPACE_EXIT_X, SPACE_EXIT_Y, SPACE_EXIT_W, SPACE_EXIT_H, "EXIT", false, false, -1);

  tft.drawFastHLine(0, SPACE_STATUS_H, 240, COLOR_ACCENT);
  tft.drawString("BATTLEZONE", 72, 8, 2);

  drawButtonBubble(SPACE_LEFT_BTN_X, SPACE_CONTROL_Y, SPACE_CTRL_BTN_W, SPACE_CTRL_BTN_H, "TURN-", false, false, -1);
  drawButtonBubble(SPACE_FIRE_BTN_X, SPACE_CONTROL_Y, SPACE_CTRL_BTN_W, SPACE_CTRL_BTN_H, "FIRE", false, false, -1);
  drawButtonBubble(SPACE_RIGHT_BTN_X, SPACE_CONTROL_Y, SPACE_CTRL_BTN_W, SPACE_CTRL_BTN_H, "TURN+", false, false, -1);

  tft.drawFastHLine(0, SPACE_CONTROL_Y - 6, 240, COLOR_ACCENT);
}

void drawSpaceGameDynamic() {
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);

  tft.fillRect(96, 6, 136, 16, COLOR_BG);
  tft.drawString(String(spaceScore), 98, 8, 2);
  tft.drawString("L:" + String(spaceLives), 150, 8, 2);
  tft.drawString("W:" + String(spaceWave), 188, 8, 2);

  tft.fillRect(0, BZ_VIEW_TOP, 240, BZ_VIEW_BOTTOM - BZ_VIEW_TOP, COLOR_BG);
  tft.drawFastHLine(0, BZ_HORIZON_Y, 240, TFT_DARKGREY);

  for (int x = 0; x <= 240; x += 30) {
    int peak = (int)(BZ_HORIZON_Y - 6 - 4 * sinf((x + (spaceWave * 7)) * 0.06f));
    tft.drawLine(x, BZ_HORIZON_Y, x + 14, peak, COLOR_PANEL);
    tft.drawLine(x + 14, peak, x + 30, BZ_HORIZON_Y, COLOR_PANEL);
  }

  auto drawWireTarget = [&](float sx, float scale, uint8_t enemyType, bool pyramid) {
    int cx = (int)roundf(sx);
    int h = max(6, (int)roundf(scale));
    int w = max(8, (int)roundf(scale * (pyramid ? 0.9f : 1.2f)));
    int baseY = BZ_HORIZON_Y + (int)roundf(70.0f / max(scale, 1.0f));
    baseY = constrain(baseY, BZ_HORIZON_Y + 8, BZ_VIEW_BOTTOM - 16);
    bool enemyColor = (enemyType == BZ_ENEMY_TANK || enemyType == BZ_ENEMY_SUPER || enemyType == BZ_ENEMY_UFO);
    uint16_t c = enemyColor ? COLOR_ACCENT_HI : COLOR_TEXT;

    if (pyramid) {
      tft.drawLine(cx - w / 2, baseY, cx, baseY - h, c);
      tft.drawLine(cx + w / 2, baseY, cx, baseY - h, c);
      tft.drawLine(cx - w / 2, baseY, cx + w / 2, baseY, c);
      tft.drawLine(cx - w / 3, baseY - h / 2, cx + w / 3, baseY - h / 2, c);
    } else if (enemyType == BZ_ENEMY_UFO) {
      int rw = max(10, (int)roundf(w * 1.1f));
      tft.drawFastHLine(cx - rw / 2, baseY - h / 2, rw, c);
      tft.drawFastHLine(cx - rw / 3, baseY - h / 2 - 4, (rw * 2) / 3, c);
      tft.drawLine(cx - rw / 2, baseY - h / 2, cx - rw / 3, baseY - h / 2 - 4, c);
      tft.drawLine(cx + rw / 2, baseY - h / 2, cx + rw / 3, baseY - h / 2 - 4, c);
      tft.drawLine(cx - rw / 3, baseY - h / 2 + 1, cx + rw / 3, baseY - h / 2 + 1, c);
    } else if (enemyType == BZ_ENEMY_SUPER) {
      tft.drawRect(cx - w / 2, baseY - h, w, h, c);
      tft.drawRect(cx - w / 2 - 2, baseY - h - 2, w + 4, h + 4, c);
      tft.drawLine(cx - w / 2, baseY, cx - w / 3, baseY - h - h / 3, c);
      tft.drawLine(cx + w / 2, baseY, cx + w / 3, baseY - h - h / 3, c);
      tft.drawLine(cx - w / 3, baseY - h - h / 3, cx + w / 3, baseY - h - h / 3, c);
    } else {
      tft.drawRect(cx - w / 2, baseY - h, w, h, c);
      tft.drawLine(cx - w / 2, baseY, cx - w / 3, baseY - h - h / 3, c);
      tft.drawLine(cx + w / 2, baseY, cx + w / 3, baseY - h - h / 3, c);
      tft.drawLine(cx - w / 3, baseY - h - h / 3, cx + w / 3, baseY - h - h / 3, c);
    }
  };

  for (int i = 0; i < BZ_MAX_OBSTACLES; i++) {
    float sx, scale, forward;
    if (!battleWorldToScreen(spaceObstacles[i].x, spaceObstacles[i].y, sx, scale, forward)) continue;
    drawWireTarget(sx, scale * 0.8f, 255, spaceObstacles[i].pyramid);
  }

  for (int i = 0; i < BZ_MAX_ENEMIES; i++) {
    if (!spaceEnemies[i].alive) continue;
    float sx, scale, forward;
    if (!battleWorldToScreen(spaceEnemies[i].x, spaceEnemies[i].y, sx, scale, forward)) continue;
    drawWireTarget(sx, scale, spaceEnemies[i].type, false);
  }

  if (spacePlayerShotActive) {
    float sx, scale, forward;
    if (battleWorldToScreen(spacePlayerShotX, spacePlayerShotY, sx, scale, forward)) {
      int y = BZ_HORIZON_Y + (int)roundf(68.0f / max(scale, 1.0f));
      int ix = (int)sx;
      tft.drawFastVLine(ix, y - 6, 12, COLOR_TEXT);
      tft.drawFastVLine(ix - 1, y - 4, 8, COLOR_ACCENT_HI);
      tft.drawFastVLine(ix + 1, y - 4, 8, COLOR_ACCENT_HI);
    }
  }

  if (spaceEnemyShotActive) {
    float sx, scale, forward;
    if (battleWorldToScreen(spaceEnemyShotX, spaceEnemyShotY, sx, scale, forward)) {
      int y = BZ_HORIZON_Y + (int)roundf(68.0f / max(scale, 1.0f));
      tft.drawFastVLine((int)sx, y - 4, 8, COLOR_ACCENT);
      tft.drawPixel((int)sx - 1, y - 1, COLOR_ACCENT_HI);
      tft.drawPixel((int)sx + 1, y + 1, COLOR_ACCENT_HI);
    }
  }

  if (spaceMissileActive) {
    float sx, scale, forward;
    if (battleWorldToScreen(spaceMissileX, spaceMissileY, sx, scale, forward)) {
      int y = BZ_HORIZON_Y + (int)roundf(68.0f / max(scale, 1.0f));
      int mx = (int)sx;
      tft.drawLine(mx, y, mx - 4, y + 2, COLOR_ACCENT_HI);
      tft.drawLine(mx, y, mx - 4, y - 2, COLOR_ACCENT_HI);
      tft.drawLine(mx, y, mx + 3, y, COLOR_ACCENT_HI);
      tft.drawPixel(mx - 5, y, COLOR_ACCENT);
    }
  }

  tft.drawFastHLine(112, BZ_HORIZON_Y + 2, 16, COLOR_SIG);
  tft.drawFastVLine(120, BZ_HORIZON_Y - 6, 14, COLOR_SIG);
  if (millis() < spaceMuzzleFlashUntil) {
    tft.drawCircle(120, BZ_HORIZON_Y + 1, 4, COLOR_ACCENT_HI);
    tft.drawCircle(120, BZ_HORIZON_Y + 1, 7, COLOR_ACCENT);
  }

  int radarX = 12;
  int radarY = 6;
  int radarW = 52;
  int radarH = 16;
  tft.drawRect(radarX, radarY, radarW, radarH, COLOR_ACCENT);
  tft.fillCircle(radarX + radarW / 2, radarY + radarH / 2, 1, COLOR_SIG);
  for (int i = 0; i < BZ_MAX_ENEMIES; i++) {
    if (!spaceEnemies[i].alive) continue;
    if (spaceEnemies[i].type == BZ_ENEMY_UFO) continue;
    float dx = spaceEnemies[i].x - spacePlayerX;
    float dy = spaceEnemies[i].y - spacePlayerY;
    float rx = constrain(dx * 0.22f, -24.0f, 24.0f);
    float ry = constrain(dy * 0.22f, -7.0f, 7.0f);
    tft.drawPixel(radarX + radarW / 2 + (int)rx, radarY + radarH / 2 + (int)ry, COLOR_ACCENT_HI);
  }
  if (spaceMissileActive) {
    float dx = spaceMissileX - spacePlayerX;
    float dy = spaceMissileY - spacePlayerY;
    float rx = constrain(dx * 0.22f, -24.0f, 24.0f);
    float ry = constrain(dy * 0.22f, -7.0f, 7.0f);
    tft.drawPixel(radarX + radarW / 2 + (int)rx, radarY + radarH / 2 + (int)ry, COLOR_ACCENT);
  }

  if (spaceGameOver) {
    tft.setTextColor(COLOR_ACCENT_HI, COLOR_BG);
    tft.drawCentreString("GAME OVER", 120, 132, 4);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawCentreString("SELECT to restart", 120, 174, 2);
    tft.drawCentreString("DOWN exits to Menu", 120, 192, 2);
  } else if (spaceWaveCleared) {
    tft.setTextColor(COLOR_SIG, COLOR_BG);
    tft.drawCentreString("WAVE CLEAR", 120, 132, 4);
  } else if (!spaceGameStarted) {
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.drawCentreString("Right stick tank mode", 120, 248, 2);
    tft.drawCentreString("SELECT fires", 120, 266, 2);
  }
}

  void drawControllerSettings() {
  tft.fillScreen(COLOR_BG);

  const char* navLabel = (controllerSettingsPage == 0) ? "NEXT" : "PREV";

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

    tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  drawButtonBubble(130, BACK_BTN_Y, 100, BACK_BTN_H, navLabel,
                   pressedButton == BTN_PAGE_NAV, dpadFocusVisible && selectedButton == BTN_PAGE_NAV, 40);

  if (controllerSettingsPage == 0) {
    drawButtonBubble(20, 50, 200, BTN_HEIGHT, "Trim",
                     pressedButton == BTN_TRIM, dpadFocusVisible && selectedButton == BTN_TRIM, 100);
    drawButtonBubble(20, 120, 200, BTN_HEIGHT, "Failsafe",
                     pressedButton == BTN_FAILSAFE, dpadFocusVisible && selectedButton == BTN_FAILSAFE, 100);
    drawButtonBubble(20, 190, 200, BTN_HEIGHT, "End Points",
                     pressedButton == BTN_ENDPOINTS, dpadFocusVisible && selectedButton == BTN_ENDPOINTS, -1);
  } else {
    drawButtonBubble(20, 50, 200, BTN_HEIGHT, "Expo",
                     pressedButton == BTN_EXPO, dpadFocusVisible && selectedButton == BTN_EXPO, 100);
    drawButtonBubble(20, 120, 200, BTN_HEIGHT, "Rates",
                     pressedButton == BTN_RATES, dpadFocusVisible && selectedButton == BTN_RATES, 100);
    drawButtonBubble(20, 190, 200, BTN_HEIGHT, "Protocol",
                     pressedButton == BTN_PROTOCOL, dpadFocusVisible && selectedButton == BTN_PROTOCOL, 100);
  }
  }

void drawEndpointStatic() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    false,
    (dpadFocusVisible && focusIndex == CHANNEL_COUNT),
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString("End Points", 120, 38, 2);
  tft.drawCentreString("Move stick -/+ to pick side", 120, 58, 2);

  int startY = 88;
  int spacing = 42;
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    int y = startY + (i * spacing);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(16, y + 10);
    tft.print("CH");
    tft.print(i + 1);
    tft.setCursor(52, y + 10);
    tft.print(getChannelAxisName(i));
    tft.drawRoundRect(88, y, 136, 30, 10, COLOR_ACCENT);
  }
}

void drawEndpointDynamic() {
  bool sideChanged = false;
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    if (updateEndpointSideLatch(i)) sideChanged = true;
  }
  if (sideChanged) endpointNeedsRedraw = true;
  if (!endpointNeedsRedraw) return;

  int startY = 88;
  int spacing = 42;

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    int y = startY + (i * spacing);
    int low = getModelEndpointLowValue(activeModel, i);
    int high = getModelEndpointHighValue(activeModel, i);
    bool rowFocused = (dpadFocusVisible && focusIndex == i);
    bool highSide = isEndpointHighSideSelected(i);

    tft.fillRect(90, y + 2, 132, 26, COLOR_BG);
    tft.fillRoundRect(90, y + 2, 132, 26, 8, COLOR_PANEL);
    tft.drawRoundRect(88, y, 136, 30, 10, COLOR_ACCENT);

    if (rowFocused) {
      tft.drawRoundRect(86, y - 2, 140, 34, 10, COLOR_ACCENT_HI);
    }

    char lowText[8];
    char highText[8];
    snprintf(lowText, sizeof(lowText), "%3d", low);
    snprintf(highText, sizeof(highText), "%3d", high);

    int lowBoxX = 94;
    int lowBoxW = 46;
    int valueBoxY = y + 4;
    int valueBoxH = 22;
    int highBoxX = 170;
    int highBoxW = 46;

    tft.fillRoundRect(lowBoxX, valueBoxY, lowBoxW, valueBoxH, 6, COLOR_BG);
    tft.drawRoundRect(lowBoxX, valueBoxY, lowBoxW, valueBoxH, 6,
                      (!highSide) ? COLOR_ACCENT_HI : COLOR_ACCENT);
    tft.fillRoundRect(highBoxX, valueBoxY, highBoxW, valueBoxH, 6, COLOR_BG);
    tft.drawRoundRect(highBoxX, valueBoxY, highBoxW, valueBoxH, 6,
                      highSide ? COLOR_ACCENT_HI : COLOR_ACCENT);

    tft.setTextColor((rowFocused && !highSide) ? COLOR_ACCENT_HI : COLOR_TEXT);
    tft.drawCentreString(lowText, lowBoxX + (lowBoxW / 2), y + 9, 2);

    tft.setTextColor(COLOR_TEXT);
    tft.drawString(">", 153, y + 9, 2);

    tft.setTextColor((rowFocused && highSide) ? COLOR_ACCENT_HI : COLOR_TEXT);
    tft.drawCentreString(highText, highBoxX + (highBoxW / 2), y + 9, 2);
  }

  endpointNeedsRedraw = false;
}

  void drawControllerPlaceholderScreen(const char* title, const char* line1, const char* line2) {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString(title, 120, 52, 2);
  tft.drawCentreString(line1, 120, 112, 2);
  tft.drawCentreString(line2, 120, 136, 2);
  }

  void drawExpoStatic() {
  tft.fillScreen(COLOR_BG);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString("Expo", 120, 38, 2);
  tft.drawCentreString("Value", 120, EXPO_VALUE_LABEL_Y, 2);
  }

  void drawExpoDynamic() {
  if (!expoNeedsRedraw) return;

  int currentValue = getModelExpoValue(activeModel, selectedExpoChannel);
  char valueText[12];

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    false,
    (dpadFocusVisible && focusIndex == 0),
    BACK_TEXT_OFFSET);

  tft.fillRect(0, RATE_TAB_Y - 4, 240, RATE_TAB_H + 10, COLOR_BG);
  tft.fillRect(EXPO_GRAPH_X - 4, EXPO_GRAPH_Y - 4, EXPO_GRAPH_W + 8, EXPO_GRAPH_H + 8, COLOR_BG);
  tft.fillRect(RATE_VALUE_BOX_X - 4, EXPO_VALUE_BOX_Y - 4, RATE_VALUE_BOX_W + 8, RATE_VALUE_BOX_H + 8, COLOR_BG);

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    bool isSelected = (i == selectedExpoChannel);
    int tabX = RATE_TAB_X(i);

    drawGradientControl(
      tabX, RATE_TAB_Y, RATE_TAB_W, RATE_TAB_H, 8,
      isSelected ? COLOR_ACCENT : COLOR_PANEL, COLOR_ACCENT);
    if (dpadFocusVisible && focusIndex == i + 1) {
      tft.drawRoundRect(tabX - 2, RATE_TAB_Y - 2, RATE_TAB_W + 4, RATE_TAB_H + 4, 8, COLOR_ACCENT_HI);
    }
    tft.setTextColor(isSelected ? COLOR_BG : COLOR_TEXT);
    snprintf(valueText, sizeof(valueText), "CH%d", i + 1);
    tft.drawCentreString(valueText, tabX + (RATE_TAB_W / 2), RATE_TAB_Y + 7, 2);
  }

  drawGradientControl(EXPO_GRAPH_X, EXPO_GRAPH_Y, EXPO_GRAPH_W, EXPO_GRAPH_H, 10, COLOR_PANEL, COLOR_ACCENT);

  int graphPad = 12;
  int gx = EXPO_GRAPH_X + graphPad;
  int gy = EXPO_GRAPH_Y + graphPad;
  int gw = EXPO_GRAPH_W - (graphPad * 2);
  int gh = EXPO_GRAPH_H - (graphPad * 2);
  int cx = gx + (gw / 2);
  int cy = gy + (gh / 2);

  tft.drawRect(gx, gy, gw, gh, TFT_DARKGREY);
  tft.drawFastVLine(cx, gy, gh, TFT_DARKGREY);
  tft.drawFastHLine(gx, cy, gw, TFT_DARKGREY);

  tft.drawLine(gx, gy + gh - 1, gx + gw - 1, gy, tft.color565(100, 100, 100));

  int prevX = gx;
  int prevY = gy + gh - 1;
  for (int i = 1; i < gw; i++) {
    float input = ((float)i / (float)(gw - 1)) * 2.0f - 1.0f;
    float output = applyExpoCurve(input, currentValue);
    int py = gy + (gh - 1) - (int)roundf(((output + 1.0f) * 0.5f) * (gh - 1));
    int px = gx + i;
    tft.drawLine(prevX, prevY, px, py, COLOR_ACCENT_HI);
    prevX = px;
    prevY = py;
  }

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(EXPO_GRAPH_X + 12, EXPO_GRAPH_Y + 8);
  tft.print("Input");
  tft.setCursor(EXPO_GRAPH_X + EXPO_GRAPH_W - 56, EXPO_GRAPH_Y + 8);
  tft.print("Output");

  drawGradientControl(
    RATE_VALUE_BOX_X, EXPO_VALUE_BOX_Y, RATE_VALUE_BOX_W, RATE_VALUE_BOX_H, 10,
    COLOR_PANEL, COLOR_ACCENT);
  if (dpadFocusVisible && focusIndex == 5) {
    tft.drawRoundRect(RATE_VALUE_BOX_X - 2, EXPO_VALUE_BOX_Y - 2,
                      RATE_VALUE_BOX_W + 4, RATE_VALUE_BOX_H + 4, 10, COLOR_ACCENT_HI);
  }
  if (mixNumpadActive && mixNumpadTarget == NUMPAD_TARGET_EXPO_VALUE) {
    tft.drawRoundRect(RATE_VALUE_BOX_X - 2, EXPO_VALUE_BOX_Y - 2,
                      RATE_VALUE_BOX_W + 4, RATE_VALUE_BOX_H + 4, 10, COLOR_ACCENT_HI);
  }
  snprintf(valueText, sizeof(valueText), "%d", currentValue);
  tft.setTextColor(COLOR_ACCENT_HI);
  tft.drawCentreString(valueText, RATE_VALUE_BOX_X + (RATE_VALUE_BOX_W / 2), EXPO_VALUE_BOX_Y + 10, 2);

  expoNeedsRedraw = false;
  }

  void drawRatesStatic() {
  tft.fillScreen(COLOR_BG);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString("Rates", 120, 38, 2);
  tft.drawCentreString("Rate Amount", 120, 146, 2);
  tft.drawCentreString("Value", 120, RATE_VALUE_BOX_Y - 20, 2);

  tft.fillRoundRect(RATE_SLIDER_X, RATE_SLIDER_Y + 7, RATE_SLIDER_W, 8, 4, COLOR_PANEL);
  tft.drawRoundRect(RATE_SLIDER_X - 1, RATE_SLIDER_Y + 6, RATE_SLIDER_W + 2, 10, 4, COLOR_ACCENT);

  const int notchValues[3] = {RATE_LOW_VALUE, RATE_NORMAL_VALUE, RATE_HIGH_VALUE};
  for (int i = 0; i < 3; i++) {
    int notchX = RATE_SLIDER_X + ((notchValues[i] - 1) * RATE_SLIDER_W) / 99;
    tft.drawFastVLine(notchX, RATE_SLIDER_Y + 2, 18, COLOR_ACCENT_HI);
  }
  }

  void drawRatesDynamic() {
  if (!ratesNeedsRedraw) return;

  int currentValue = getModelRateValue(activeModel, selectedRateChannel);
  int sliderKnobX = RATE_SLIDER_X + ((currentValue - 1) * RATE_SLIDER_W) / 99;
  char valueText[12];

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    false,
    (dpadFocusVisible && focusIndex == 0),
    BACK_TEXT_OFFSET);

  tft.fillRect(0, RATE_TAB_Y - 4, 240, RATE_TAB_H + 10, COLOR_BG);
  tft.fillRect(0, RATE_TOGGLE_Y - 4, 240, RATE_TOGGLE_H + 10, COLOR_BG);
  tft.fillRect(RATE_SLIDER_X - 6, RATE_SLIDER_Y - 4, RATE_SLIDER_W + 12, RATE_SLIDER_H + 12, COLOR_BG);
  tft.fillRoundRect(RATE_SLIDER_X, RATE_SLIDER_Y + 7, RATE_SLIDER_W, 8, 4, COLOR_PANEL);
  tft.drawRoundRect(RATE_SLIDER_X - 1, RATE_SLIDER_Y + 6, RATE_SLIDER_W + 2, 10, 4, COLOR_ACCENT);
  const int notchValues[3] = {RATE_LOW_VALUE, RATE_NORMAL_VALUE, RATE_HIGH_VALUE};
  for (int i = 0; i < 3; i++) {
    int notchX = RATE_SLIDER_X + ((notchValues[i] - 1) * RATE_SLIDER_W) / 99;
    tft.drawFastVLine(notchX, RATE_SLIDER_Y + 2, 18, COLOR_ACCENT_HI);
  }
  tft.fillRect(RATE_VALUE_BOX_X - 4, RATE_VALUE_BOX_Y - 4, RATE_VALUE_BOX_W + 8, RATE_VALUE_BOX_H + 8, COLOR_BG);

  for (int i = 0; i < CHANNEL_COUNT; i++) {
    bool isSelected = (i == selectedRateChannel);
    int tabX = RATE_TAB_X(i);

    drawGradientControl(
      tabX, RATE_TAB_Y, RATE_TAB_W, RATE_TAB_H, 8,
      isSelected ? COLOR_ACCENT : COLOR_PANEL, COLOR_ACCENT);
    if (dpadFocusVisible && focusIndex == i + 1) {
      tft.drawRoundRect(tabX - 2, RATE_TAB_Y - 2, RATE_TAB_W + 4, RATE_TAB_H + 4, 8, COLOR_ACCENT_HI);
    }
    tft.setTextColor(isSelected ? COLOR_BG : COLOR_TEXT);
    snprintf(valueText, sizeof(valueText), "CH%d", i + 1);
    tft.drawCentreString(valueText, tabX + (RATE_TAB_W / 2), RATE_TAB_Y + 7, 2);
  }

  const char* toggleLabels[3] = {"LOW", "NORMAL", "HIGH"};
  const int toggleValues[3] = {RATE_LOW_VALUE, RATE_NORMAL_VALUE, RATE_HIGH_VALUE};
  for (int i = 0; i < 3; i++) {
    bool isSelected = (currentValue == toggleValues[i]);
    int toggleX = RATE_TOGGLE_X(i);
    int toggleW = (i == 1) ? 70 : RATE_TOGGLE_W;
    if (i == 1) toggleX -= 5;

    drawGradientControl(
      toggleX, RATE_TOGGLE_Y, toggleW, RATE_TOGGLE_H, 8,
      isSelected ? COLOR_ACCENT : COLOR_PANEL, COLOR_ACCENT);
    if (dpadFocusVisible && focusIndex == i + 5) {
      tft.drawRoundRect(toggleX - 2, RATE_TOGGLE_Y - 2, toggleW + 4, RATE_TOGGLE_H + 4, 8, COLOR_ACCENT_HI);
    }
    tft.setTextColor(isSelected ? COLOR_BG : COLOR_TEXT);
    tft.drawCentreString(toggleLabels[i], toggleX + (toggleW / 2), RATE_TOGGLE_Y + 7, 2);
  }

  tft.setTextColor(COLOR_TEXT);
  if (dpadFocusVisible && focusIndex == 8) {
    tft.drawRoundRect(RATE_SLIDER_X - 4, RATE_SLIDER_Y - 2, RATE_SLIDER_W + 8, RATE_SLIDER_H + 8, 8, COLOR_ACCENT_HI);
  }

  tft.fillCircle(sliderKnobX, RATE_SLIDER_Y + 11, 11, COLOR_ACCENT);
  tft.drawCircle(sliderKnobX, RATE_SLIDER_Y + 11, 11, COLOR_ACCENT_HI);
  tft.fillCircle(sliderKnobX, RATE_SLIDER_Y + 11, 4, COLOR_BG);

  drawGradientControl(
    RATE_VALUE_BOX_X, RATE_VALUE_BOX_Y, RATE_VALUE_BOX_W, RATE_VALUE_BOX_H, 10,
    COLOR_PANEL, COLOR_ACCENT);
  if (mixNumpadActive && mixNumpadTarget == NUMPAD_TARGET_RATE_VALUE) {
    tft.drawRoundRect(RATE_VALUE_BOX_X - 2, RATE_VALUE_BOX_Y - 2,
                      RATE_VALUE_BOX_W + 4, RATE_VALUE_BOX_H + 4, 10, COLOR_ACCENT_HI);
  }
  snprintf(valueText, sizeof(valueText), "%d", currentValue);
  tft.setTextColor(COLOR_ACCENT_HI);
  tft.drawCentreString(valueText, RATE_VALUE_BOX_X + (RATE_VALUE_BOX_W / 2), RATE_VALUE_BOX_Y + 10, 2);

  ratesNeedsRedraw = false;
  }

void composeProtocolStatusText(char *bindStatus, size_t bindStatusSize, uint16_t *statusColor) {
  if (bindStatus == nullptr || bindStatusSize == 0) return;

  if (otaModeActive) {
    uint16_t resolvedColor = COLOR_ACCENT_HI;
    if (otaUpdateInProgress) {
      snprintf(bindStatus, bindStatusSize, "OTA update in progress");
    } else if (otaStaConnectPending && otaUsingSoftAP) {
      IPAddress ip = WiFi.softAPIP();
      snprintf(bindStatus, bindStatusSize, "OTA AP %u.%u.%u.%u + STA...",
               ip[0], ip[1], ip[2], ip[3]);
    } else if (otaStaConnectPending) {
      snprintf(bindStatus, bindStatusSize, "OTA connecting STA...");
    } else if (otaServiceReady) {
      IPAddress ip = otaUsingSoftAP ? WiFi.softAPIP() : WiFi.localIP();
      if ((uint32_t)ip == 0) {
        ip = WiFi.softAPIP();
      }
      snprintf(bindStatus, bindStatusSize, "OTA %u.%u.%u.%u",
               ip[0], ip[1], ip[2], ip[3]);
    } else {
      snprintf(bindStatus, bindStatusSize, "OTA unavailable");
      resolvedColor = TFT_RED;
    }
    if (statusColor != nullptr) *statusColor = resolvedColor;
    return;
  }

  bool elrsActive = (getModelProtocol(activeModel) == PROTOCOL_ELRS);
  bool espNowActive = !elrsActive;

  uint16_t resolvedColor = COLOR_TEXT;
  if (elrsActive) {
    snprintf(bindStatus, bindStatusSize, "ELRS selected");
    resolvedColor = COLOR_ACCENT_HI;
  } else if (espNowBindingMode) {
    unsigned long remainingMs = 0;
    unsigned long now = millis();
    if (now - espNowBindingStartTime < ESPNOW_BIND_TIMEOUT_MS) {
      remainingMs = ESPNOW_BIND_TIMEOUT_MS - (now - espNowBindingStartTime);
    }
    snprintf(bindStatus, bindStatusSize, "Listening %lus", (remainingMs + 999) / 1000);
    if (espNowActive) resolvedColor = COLOR_ACCENT_HI;
  } else if (millis() - espNowBindSuccessTime < 2500) {
    snprintf(bindStatus, bindStatusSize, "Bind complete");
  } else {
    formatReceiverMacShort(getBoundReceiverMac(activeModel), bindStatus, bindStatusSize);
  }

  if (statusColor != nullptr) *statusColor = resolvedColor;
}

void composeElrsStatusText(char *statusText, size_t statusTextSize, uint16_t *statusColor) {
  if (statusText == nullptr || statusTextSize == 0) return;

  unsigned long now = millis();
  uint16_t resolvedColor = COLOR_TEXT;
  bool haveRawRx = (elrsRxByteCount > 0);
  bool haveModuleFrames = hasElrsModuleFrames();
  if (now - elrsBindSuccessTime < 3000) {
    snprintf(statusText, statusTextSize, "ELRS bind successful");
    resolvedColor = COLOR_SIG;
  } else if (elrsBindAwaitingResult) {
    snprintf(statusText, statusTextSize, "ELRS binding...");
    resolvedColor = COLOR_ACCENT_HI;
  } else if (now - elrsBindCommandSentTime < 2500) {
    snprintf(statusText, statusTextSize, "ELRS bind command sent");
    resolvedColor = COLOR_ACCENT_HI;
  } else if (!elrsInitialized) {
    snprintf(statusText, statusTextSize, "ELRS uart offline");
  } else if (elrsLinkActive) {
    snprintf(statusText, statusTextSize, "ELRS link active LQ %u", (unsigned int)elrsUplinkLq);
    resolvedColor = COLOR_SIG;
  } else if (haveModuleFrames || elrsModulePresent) {
    snprintf(statusText, statusTextSize, "ELRS module rx baud %lu",
             (unsigned long)elrsActiveBaud);
    resolvedColor = COLOR_ACCENT_HI;
  } else if (haveRawRx) {
    snprintf(statusText, statusTextSize, "ELRS raw uart rx only");
    resolvedColor = TFT_ORANGE;
  } else {
    snprintf(statusText, statusTextSize, "ELRS no uart rx");
  }

  if (statusColor != nullptr) *statusColor = resolvedColor;
}

void drawProtocolScreen() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString("Protocol", 120, 38, 2);

  bool elrsActive = (getModelProtocol(activeModel) == PROTOCOL_ELRS);
  bool espNowActive = !elrsActive;
  char bindStatus[56];
  uint16_t statusColor = COLOR_TEXT;
  composeProtocolStatusText(bindStatus, sizeof(bindStatus), &statusColor);

  drawButtonBubble(PROTOCOL_MODE_BTN_X, PROTOCOL_ELRS_BTN_Y, PROTOCOL_MODE_BTN_W, PROTOCOL_MODE_BTN_H, "ELRS",
                   pressedButton == BTN_PROTOCOL_ELRS,
                   elrsActive || (dpadFocusVisible && selectedButton == BTN_PROTOCOL_ELRS),
                   -1);
  drawButtonBubble(PROTOCOL_MODE_BTN_X, PROTOCOL_ESPNOW_BTN_Y, PROTOCOL_MODE_BTN_W, PROTOCOL_MODE_BTN_H, "ESP-NOW",
                   pressedButton == BTN_PROTOCOL_ESPNOW,
                   espNowActive || (dpadFocusVisible && selectedButton == BTN_PROTOCOL_ESPNOW),
                   -1);

  // Clear status area so protocol switches never leave stale text behind.
  tft.fillRect(PROTOCOL_STATUS_X, PROTOCOL_STATUS_Y, PROTOCOL_STATUS_W, PROTOCOL_STATUS_H, COLOR_BG);
  tft.setTextFont(2);
  tft.setTextColor(statusColor);
  tft.drawString(bindStatus, PROTOCOL_STATUS_X + 2, PROTOCOL_STATUS_Y + 2, 2);

  drawButtonBubble(PROTOCOL_OTA_BTN_X, PROTOCOL_OTA_BTN_Y,
                   PROTOCOL_OTA_BTN_W, PROTOCOL_OTA_BTN_H,
                   otaModeActive ? "OTA ON" : "OTA OFF",
                   pressedButton == BTN_PROTOCOL_OTA,
                   dpadFocusVisible && selectedButton == BTN_PROTOCOL_OTA,
                   -1);
  drawButtonBubble(PROTOCOL_OTA_CFG_BTN_X, PROTOCOL_OTA_CFG_BTN_Y,
                   PROTOCOL_OTA_CFG_BTN_W, PROTOCOL_OTA_CFG_BTN_H,
                   "CFG",
                   pressedButton == BTN_PROTOCOL_OTA_CFG,
                   dpadFocusVisible && selectedButton == BTN_PROTOCOL_OTA_CFG,
                   -1);

  drawButtonBubble(PROTOCOL_BIND_BTN_X, PROTOCOL_BIND_BTN_Y,
                   PROTOCOL_BIND_BTN_W, PROTOCOL_BIND_BTN_H,
                   otaModeActive ? "LOCK" : (elrsActive ? "ELRS" : (espNowBindingMode ? "CANCEL" : "BIND")),
                   pressedButton == BTN_PROTOCOL_BIND,
                   dpadFocusVisible && selectedButton == BTN_PROTOCOL_BIND,
                   -1);
}

void drawProtocolDynamic() {
  static char lastStatus[56] = "";
  static uint16_t lastStatusColor = 0;
  static bool lastBindingMode = false;
  static bool lastElrsActive = false;
  static bool lastOtaMode = false;

  char bindStatus[56];
  uint16_t statusColor = COLOR_TEXT;
  composeProtocolStatusText(bindStatus, sizeof(bindStatus), &statusColor);
  bool elrsActive = (getModelProtocol(activeModel) == PROTOCOL_ELRS);

  if (strcmp(bindStatus, lastStatus) != 0 || statusColor != lastStatusColor) {
    tft.fillRect(PROTOCOL_STATUS_X, PROTOCOL_STATUS_Y, PROTOCOL_STATUS_W, PROTOCOL_STATUS_H, COLOR_BG);
    tft.setTextFont(2);
    tft.setTextColor(statusColor, COLOR_BG);
    tft.drawString(bindStatus, PROTOCOL_STATUS_X + 2, PROTOCOL_STATUS_Y + 2, 2);
    strncpy(lastStatus, bindStatus, sizeof(lastStatus) - 1);
    lastStatus[sizeof(lastStatus) - 1] = '\0';
    lastStatusColor = statusColor;
  }

  if (lastBindingMode != espNowBindingMode || lastElrsActive != elrsActive || lastOtaMode != otaModeActive) {
    drawButtonBubble(PROTOCOL_OTA_BTN_X, PROTOCOL_OTA_BTN_Y,
                     PROTOCOL_OTA_BTN_W, PROTOCOL_OTA_BTN_H,
                     otaModeActive ? "OTA ON" : "OTA OFF",
                     pressedButton == BTN_PROTOCOL_OTA,
                     dpadFocusVisible && selectedButton == BTN_PROTOCOL_OTA,
                     -1);
    drawButtonBubble(PROTOCOL_OTA_CFG_BTN_X, PROTOCOL_OTA_CFG_BTN_Y,
                     PROTOCOL_OTA_CFG_BTN_W, PROTOCOL_OTA_CFG_BTN_H,
                     "CFG",
                     pressedButton == BTN_PROTOCOL_OTA_CFG,
                     dpadFocusVisible && selectedButton == BTN_PROTOCOL_OTA_CFG,
                     -1);

    drawButtonBubble(PROTOCOL_BIND_BTN_X, PROTOCOL_BIND_BTN_Y,
                     PROTOCOL_BIND_BTN_W, PROTOCOL_BIND_BTN_H,
                     otaModeActive ? "LOCK" : (elrsActive ? "ELRS" : (espNowBindingMode ? "CANCEL" : "BIND")),
                     pressedButton == BTN_PROTOCOL_BIND,
                     dpadFocusVisible && selectedButton == BTN_PROTOCOL_BIND,
                     -1);
    lastBindingMode = espNowBindingMode;
    lastElrsActive = elrsActive;
    lastOtaMode = otaModeActive;
  }
}

void drawOtaSettingsStatic() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    (dpadFocusVisible && focusIndex == 0),
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString("OTA Settings", 120, 38, 2);
  tft.drawCentreString("Select field, then type", 120, 58, 2);
  tft.drawCentreString("AP password: anubisota", 120, 72, 2);

  const char* labels[3] = {"Home SSID", "Home Pass", "AP Name"};
  int rowY = 96;
  for (int i = 0; i < 3; i++) {
    tft.setTextColor(COLOR_TEXT);
    tft.drawString(labels[i], 12, rowY + 10, 2);
    tft.drawRoundRect(98, rowY, 130, 30, 8, COLOR_ACCENT);
    rowY += 48;
  }
}

void drawOtaSettingsDynamic() {
  if (!otaSettingsNeedsRedraw) return;

  char ssidText[OTA_STA_SSID_STORAGE_BYTES + 1];
  char passText[OTA_STA_PASSWORD_STORAGE_BYTES + 1];
  char apText[OTA_AP_SSID_STORAGE_BYTES + 1];

  strncpy(ssidText, otaStaSsid, OTA_STA_SSID_STORAGE_BYTES);
  ssidText[OTA_STA_SSID_STORAGE_BYTES] = '\0';
  strncpy(apText, otaApSsid, OTA_AP_SSID_STORAGE_BYTES);
  apText[OTA_AP_SSID_STORAGE_BYTES] = '\0';

  if (keyboardActive) {
    if (keyboardTarget == KEYBOARD_TARGET_OTA_STA_SSID) {
      strncpy(ssidText, keyboardBuffer.c_str(), OTA_STA_SSID_STORAGE_BYTES);
      ssidText[OTA_STA_SSID_STORAGE_BYTES] = '\0';
    } else if (keyboardTarget == KEYBOARD_TARGET_OTA_AP_SSID) {
      strncpy(apText, keyboardBuffer.c_str(), OTA_AP_SSID_STORAGE_BYTES);
      apText[OTA_AP_SSID_STORAGE_BYTES] = '\0';
    }
  }

  const char* passwordSource = otaStaPassword;
  if (keyboardActive && keyboardTarget == KEYBOARD_TARGET_OTA_STA_PASSWORD) {
    passwordSource = keyboardBuffer.c_str();
  }

  int passLen = (int)strlen(passwordSource);
  if (passLen > OTA_STA_PASSWORD_STORAGE_BYTES) passLen = OTA_STA_PASSWORD_STORAGE_BYTES;
  bool showPassword = (keyboardActive && keyboardTarget == KEYBOARD_TARGET_OTA_STA_PASSWORD);
  for (int i = 0; i < passLen; i++) passText[i] = showPassword ? passwordSource[i] : '*';
  passText[passLen] = '\0';

  const char* values[3] = {
    (strlen(ssidText) > 0) ? ssidText : "<empty>",
    (passLen > 0) ? passText : "<empty>",
    (strlen(apText) > 0) ? apText : "<empty>"
  };

  int rowY = 96;
  for (int i = 0; i < 3; i++) {
    tft.fillRoundRect(100, rowY + 2, 126, 26, 6, COLOR_PANEL);
    tft.drawRoundRect(98, rowY, 130, 30, 8, COLOR_ACCENT);
    if (dpadFocusVisible && focusIndex == i + 1) {
      tft.drawRoundRect(96, rowY - 2, 134, 34, 8, COLOR_ACCENT_HI);
    }

    tft.setTextColor(COLOR_ACCENT_HI);
    tft.drawString(values[i], 104, rowY + 9, 2);
    rowY += 48;
  }

  otaSettingsNeedsRedraw = false;
}

void drawElrsScreen() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString("ELRS", 120, 38, 2);

  char statusText[56];
  uint16_t statusColor = COLOR_TEXT;
  composeElrsStatusText(statusText, sizeof(statusText), &statusColor);
  tft.fillRect(14, 82, 212, 20, COLOR_BG);
  tft.setTextColor(statusColor, COLOR_BG);
  tft.drawString(statusText, 16, 84, 2);

  drawButtonBubble(ELRS_BIND_BTN_X, ELRS_BIND_BTN_Y,
                   ELRS_BIND_BTN_W, ELRS_BIND_BTN_H,
                   "BIND",
                   pressedButton == BTN_ELRS_BIND,
                   dpadFocusVisible && selectedButton == BTN_ELRS_BIND,
                   -1);
}

void drawElrsDynamic() {
  static char lastStatus[56] = "";
  static uint16_t lastStatusColor = 0;

  char statusText[56];
  uint16_t statusColor = COLOR_TEXT;
  composeElrsStatusText(statusText, sizeof(statusText), &statusColor);

  if (strcmp(lastStatus, statusText) != 0 || lastStatusColor != statusColor) {
    tft.fillRect(14, 82, 212, 20, COLOR_BG);
    tft.setTextColor(statusColor, COLOR_BG);
    tft.drawString(statusText, 16, 84, 2);
    strncpy(lastStatus, statusText, sizeof(lastStatus) - 1);
    lastStatus[sizeof(lastStatus) - 1] = '\0';
    lastStatusColor = statusColor;
  }
}

  void drawModelSettings() {
  tft.fillScreen(COLOR_BG);

  const char* navLabel = (modelSettingsPage == 0) ? "NEXT" : "PREV";

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

    tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  drawButtonBubble(130, BACK_BTN_Y, 100, BACK_BTN_H, navLabel,
                   pressedButton == BTN_PAGE_NAV, dpadFocusVisible && selectedButton == BTN_PAGE_NAV, 40);

  if (modelSettingsPage == 0) {
    drawButtonBubble(20, 50, 200, BTN_HEIGHT, "Model Name",
                     pressedButton == BTN_MODEL_NAME, dpadFocusVisible && selectedButton == BTN_MODEL_NAME, 100);
    drawButtonBubble(20, 120, 200, BTN_HEIGHT, "Drive Type",
                     pressedButton == BTN_DRIVE_TYPE, dpadFocusVisible && selectedButton == BTN_DRIVE_TYPE, 100);
    drawButtonBubble(20, 190, 200, BTN_HEIGHT, "Mixing",
                     pressedButton == BTN_MIXING, dpadFocusVisible && selectedButton == BTN_MIXING, 100);
  }
  else {
    drawButtonBubble(20, 120, 200, BTN_HEIGHT, "Reverse",
                     pressedButton == BTN_REVERSE, dpadFocusVisible && selectedButton == BTN_REVERSE, 100);
  }
}

void drawDriveTypeScreen() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString("Drive Type", 120, 38, 2);

  drawDriveTypeOption(DRIVE_BTN_X1, DRIVE_BTN_Y1, DRIVE_BTN_W, DRIVE_BTN_H,
                      "Tank", DRIVE_TANK, BTN_DRIVE_TANK);
  drawDriveTypeOption(DRIVE_BTN_X2, DRIVE_BTN_Y1, DRIVE_BTN_W, DRIVE_BTN_H,
                      "Car", DRIVE_CAR, BTN_DRIVE_CAR);
  drawDriveTypeOption(DRIVE_BTN_X1, DRIVE_BTN_Y2, DRIVE_BTN_W, DRIVE_BTN_H,
                      "Omni", DRIVE_OMNI, BTN_DRIVE_OMNI);
  drawDriveTypeOption(DRIVE_BTN_X2, DRIVE_BTN_Y2, DRIVE_BTN_W, DRIVE_BTN_H,
                      "X-Drone", DRIVE_X_DRONE, BTN_DRIVE_X_DRONE);
}

void drawTankModeScreen() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString(currentDrive == DRIVE_CAR ? "Car Control" : "Tank Control", 120, 38, 2);

  bool dualActive = (getModelTankMode(activeModel) == TANK_MODE_DUAL_STICK);
  bool singleActive = !dualActive;

  drawButtonBubble(20, 70, 200, 72, "2 Stick",
                   pressedButton == BTN_TANK_DUAL,
                   dualActive || (dpadFocusVisible && selectedButton == BTN_TANK_DUAL),
                   102);
  drawButtonBubble(20, 160, 200, 72, "1 Stick",
                   pressedButton == BTN_TANK_SINGLE,
                   singleActive || (dpadFocusVisible && selectedButton == BTN_TANK_SINGLE),
                   102);

  uint16_t dualColor = dualActive ? COLOR_BG : COLOR_ACCENT;
  uint16_t singleColor = singleActive ? COLOR_BG : COLOR_ACCENT;

  // simple 2-stick icon
  tft.drawRoundRect(34, 89, 34, 26, 6, dualColor);
  tft.drawRoundRect(76, 89, 34, 26, 6, dualColor);
  tft.drawCircle(51, 102, 5, dualColor);
  tft.drawCircle(93, 102, 5, dualColor);

  // simple 1-stick icon
  tft.drawRoundRect(55, 179, 34, 26, 6, singleColor);
  tft.drawCircle(72, 192, 5, singleColor);
  tft.drawLine(72, 171, 72, 179, singleColor);
  tft.drawLine(72, 213, 72, 205, singleColor);
  tft.drawLine(47, 192, 55, 192, singleColor);
  tft.drawLine(89, 192, 97, 192, singleColor);
}

void drawDriveTypeOption(int x, int y, int w, int h, const char* label,
                         DriveType drive, ButtonID button) {

  bool active = (currentDrive == drive);
  bool pressed = (pressedButton == button);
  bool focused = (dpadFocusVisible && selectedButton == button);

  uint16_t baseColor = active ? COLOR_ACCENT : COLOR_PANEL;
  uint16_t textColor = active ? COLOR_BG : COLOR_TEXT;
  uint16_t iconColor = active ? COLOR_BG : COLOR_ACCENT;
  uint16_t outlineColor = active ? COLOR_ACCENT_HI : (focused ? COLOR_ACCENT_HI : COLOR_ACCENT);

  int drawY = pressed ? y + 2 : y;
  int radius = 10;

  for (int i = 0; i < h; i++) {
    int shade = map(i, 0, h, 18, -18);

    uint8_t r = (baseColor >> 11) & 0x1F;
    uint8_t g = (baseColor >> 5)  & 0x3F;
    uint8_t b = baseColor & 0x1F;

    int nr = constrain(r + shade / 2, 0, 31);
    int ng = constrain(g + shade,     0, 63);
    int nb = constrain(b + shade / 2, 0, 31);

    uint16_t lineColor = (nr << 11) | (ng << 5) | nb;

    int xOffset = 0;

    if (i < radius) {
      int dy = radius - i;
      xOffset = radius - sqrt(radius * radius - dy * dy);
    }
    else if (i >= h - radius) {
      int dy = i - (h - radius);
      xOffset = radius - sqrt(radius * radius - dy * dy);
    }

    tft.drawFastHLine(x + xOffset, drawY + i, w - (2 * xOffset), lineColor);
  }

  tft.drawRoundRect(x, drawY, w, h, radius, outlineColor);
  tft.drawFastHLine(x + radius, drawY + h - 2, w - (2 * radius), TFT_DARKGREY);
  tft.drawFastVLine(x + w - 2, drawY + radius, h - (2 * radius), TFT_DARKGREY);

  for (int i = 0; i < 5; i++) {
    uint16_t c = fadeColor(COLOR_ACCENT_HI, 1.0 - (i * 0.16));
    tft.drawFastHLine(x + radius + i, drawY + 5 + i, w - (2 * radius) - (2 * i), c);
  }

  if (active) {
    tft.drawRoundRect(x + 2, drawY + 2, w - 4, h - 4, radius - 2, COLOR_ACCENT_HI);
  }
  else if (focused) {
    tft.drawRoundRect(x + 2, drawY + 2, w - 4, h - 4, radius - 2, COLOR_TEXT);
  }

  tft.setTextFont(2);
  tft.setTextColor(textColor);
  tft.drawCentreString(label, x + w / 2, drawY + 8, 2);

  drawDriveTypeOptionIcon(x + 12, drawY + 30, w - 24, h - 38, drive, iconColor);
}

void drawDriveTypeOptionIcon(int x, int y, int w, int h, DriveType drive, uint16_t iconColor) {
  if (drive == DRIVE_TANK) {
    drawTankIcon(x, y, w, h, iconColor);
  }
  else if (drive == DRIVE_X_DRONE) {
    drawQuadXIcon(x, y, w, h, iconColor);
  }
  else if (drive == DRIVE_CAR) {
    int iconX = x + (w / 2) - 24;
    int iconY = y + (h / 2) - 24;
    drawCarIcon(iconX, iconY, 2, iconColor);
  }
  else if (drive == DRIVE_OMNI) {
    drawOmniIcon(x, y, w, h, iconColor);
  }
}

void drawOmniIcon(int x, int y, int w, int h, uint16_t iconColor) {
  int cx = x + w / 2;
  int cy = y + h / 2;
  int r = min(w, h) / 4;

  tft.drawCircle(cx, cy, r, iconColor);
  tft.drawCircle(cx, cy, r + 4, iconColor);

  tft.drawLine(cx, y + 2, cx, cy - r - 5, iconColor);
  tft.drawLine(cx, y + 2, cx - 4, y + 8, iconColor);
  tft.drawLine(cx, y + 2, cx + 4, y + 8, iconColor);

  tft.drawLine(cx, y + h - 2, cx, cy + r + 5, iconColor);
  tft.drawLine(cx, y + h - 2, cx - 4, y + h - 8, iconColor);
  tft.drawLine(cx, y + h - 2, cx + 4, y + h - 8, iconColor);

  tft.drawLine(x + 2, cy, cx - r - 5, cy, iconColor);
  tft.drawLine(x + 2, cy, x + 8, cy - 4, iconColor);
  tft.drawLine(x + 2, cy, x + 8, cy + 4, iconColor);

  tft.drawLine(x + w - 2, cy, cx + r + 5, cy, iconColor);
  tft.drawLine(x + w - 2, cy, x + w - 8, cy - 4, iconColor);
  tft.drawLine(x + w - 2, cy, x + w - 8, cy + 4, iconColor);
}

void closeKeyboard() {
  if (!keyboardActive) return;

  keyboardActive = false;
  keyboardTarget = KEYBOARD_TARGET_MODEL_NAME;
  keyboardNeedsRedraw = true;

  // force full redraw of underlying screen
  fullRedraw = true;
  uiNeedsRedraw = true;
}

void openMixNumpad(NumpadTarget target) {
  mixNumpadTarget = target;
  mixNumpadEditingRate = (target == NUMPAD_TARGET_MIX_RATE);

  if (target == NUMPAD_TARGET_MIX_RATE || target == NUMPAD_TARGET_MIX_OFFSET) {
    MixData &mix = models[activeModel].mixes[selectedMixIndex];
    mixNumpadBuffer = String((target == NUMPAD_TARGET_MIX_RATE) ? mix.rate : mix.offset);
    mixingNeedsRedraw = true;
  }
  else if (target == NUMPAD_TARGET_RATE_VALUE) {
    mixNumpadBuffer = String(getModelRateValue(activeModel, selectedRateChannel));
    ratesNeedsRedraw = true;
  }
  else if (target == NUMPAD_TARGET_ENDPOINT_VALUE) {
    int channel = constrain(endpointNumpadChannel, 0, CHANNEL_COUNT - 1);
    mixNumpadBuffer = String(endpointNumpadHighSide
                               ? getModelEndpointHighValue(activeModel, channel)
                               : getModelEndpointLowValue(activeModel, channel));
    endpointNeedsRedraw = true;
  }
  else {
    mixNumpadBuffer = String(getModelExpoValue(activeModel, selectedExpoChannel));
    expoNeedsRedraw = true;
  }

  mixNumpadActive = true;
  mixNumpadNeedsRedraw = true;
  mixNumpadCursorRow = 0;
  mixNumpadCursorCol = 0;
  uiNeedsRedraw = true;
}

void closeMixNumpad(bool commitValue) {
  if (!mixNumpadActive) return;

  if (commitValue) {
    int parsedValue = 0;

    if (mixNumpadBuffer.length() > 0 && mixNumpadBuffer != "-") {
      parsedValue = mixNumpadBuffer.toInt();
    }

    if (mixNumpadTarget == NUMPAD_TARGET_MIX_RATE || mixNumpadTarget == NUMPAD_TARGET_MIX_OFFSET) {
      parsedValue = constrain(parsedValue, -100, 100);
      MixData &mix = models[activeModel].mixes[selectedMixIndex];
      if (mixNumpadTarget == NUMPAD_TARGET_MIX_RATE) {
        mix.rate = parsedValue;
      } else {
        mix.offset = parsedValue;
      }

      saveModels();
      mixingNeedsRedraw = true;
    }
    else if (mixNumpadTarget == NUMPAD_TARGET_RATE_VALUE) {
      parsedValue = constrain(parsedValue, 1, 100);
      setModelRateValue(activeModel, selectedRateChannel, parsedValue);
      saveRateValues();
      ratesNeedsRedraw = true;
    }
    else if (mixNumpadTarget == NUMPAD_TARGET_ENDPOINT_VALUE) {
      parsedValue = constrain(parsedValue, 1, 120);
      int channel = constrain(endpointNumpadChannel, 0, CHANNEL_COUNT - 1);
      if (endpointNumpadHighSide) {
        setModelEndpointHighValue(activeModel, channel, parsedValue);
      } else {
        setModelEndpointLowValue(activeModel, channel, parsedValue);
      }
      saveEndpointValues();
      endpointNeedsRedraw = true;
    }
    else {
      parsedValue = constrain(parsedValue, 0, 100);
      setModelExpoValue(activeModel, selectedExpoChannel, parsedValue);
      saveExpoValues();
      expoNeedsRedraw = true;
    }
  }

  mixNumpadActive = false;
  endpointNumpadChannel = -1;
  mixNumpadNeedsRedraw = false;
  fullRedraw = true;
  uiNeedsRedraw = true;
}

// ===== FUNCTIONS =====
int mapTouch(int val, int in_min, int in_max, int out_min, int out_max) {
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
bool isInside(int x, int y, int bx, int by, int bw, int bh) {
  return (x > bx && x < bx + bw && y > by && y < by + bh);
}

void setScreen(Screen screen) {
  Screen previousScreen = currentScreen;
  if (previousScreen == screen) return;

  if (previousScreen == SCREEN_PROTOCOL &&
      screen != SCREEN_PROTOCOL &&
      screen != SCREEN_OTA_SETTINGS &&
      otaModeActive &&
      !otaUpdateInProgress) {
    stopOtaMode();
  }
  if (previousScreen == SCREEN_OTA_SETTINGS &&
      screen != SCREEN_PROTOCOL &&
      screen != SCREEN_OTA_SETTINGS &&
      otaModeActive &&
      !otaUpdateInProgress) {
    stopOtaMode();
  }

  currentScreen = screen;
  lastScreen = screen;
  dpadFocusVisible = false;
  selectedButton = BTN_NONE;
  focusIndex = 0;

  if (screen == SCREEN_MENU) {
    selectedButton = BTN_CTRL;
  }
  else if (screen == SCREEN_DISPLAY_SETTINGS) {
    selectedButton = BTN_DISPLAY_BRIGHTNESS_DEC;
  }
  else if (screen == SCREEN_CONTROLLER_SETTINGS) {
    selectedButton = (controllerSettingsPage == 0) ? BTN_TRIM : BTN_EXPO;
  }
  else if (screen == SCREEN_ENDPOINTS) {
    focusIndex = 0;
    endpointNeedsRedraw = true;
  }
  else if (screen == SCREEN_EXPO) {
    focusIndex = 0;
    expoNeedsRedraw = true;
  }
  else if (screen == SCREEN_RATES) {
    focusIndex = 0;
    ratesNeedsRedraw = true;
  }
  else if (screen == SCREEN_PROTOCOL) {
    if (otaModeActive) selectedButton = BTN_PROTOCOL_OTA;
    else if (espNowBindingMode) selectedButton = BTN_PROTOCOL_BIND;
    else {
      selectedButton = (getModelProtocol(activeModel) == PROTOCOL_ESPNOW)
        ? BTN_PROTOCOL_ESPNOW
        : BTN_PROTOCOL_ELRS;
    }
  }
  else if (screen == SCREEN_OTA_SETTINGS) {
    focusIndex = 1;
    otaSettingsNeedsRedraw = true;
  }
  else if (screen == SCREEN_ELRS) {
    selectedButton = BTN_ELRS_BIND;
  }
  else if (screen == SCREEN_MODEL_SETTINGS) {
    selectedButton = (modelSettingsPage == 0) ? BTN_MODEL_NAME : BTN_REVERSE;
  }
  else if (screen == SCREEN_REVERSE) {
    reverseReturnScreen = (previousScreen == SCREEN_MODEL_SETTINGS)
      ? SCREEN_MODEL_SETTINGS
      : SCREEN_CONTROLLER_SETTINGS;
  }
  else if (screen == SCREEN_DRIVE_TYPE) {
    if (currentDrive == DRIVE_TANK) selectedButton = BTN_DRIVE_TANK;
    else if (currentDrive == DRIVE_CAR) selectedButton = BTN_DRIVE_CAR;
    else if (currentDrive == DRIVE_OMNI) selectedButton = BTN_DRIVE_OMNI;
    else selectedButton = BTN_DRIVE_X_DRONE;
  }
  else if (screen == SCREEN_TANK_MODE) {
    selectedButton = (getModelTankMode(activeModel) == TANK_MODE_RIGHT_STICK)
      ? BTN_TANK_SINGLE
      : BTN_TANK_DUAL;
  }
  else if (screen == SCREEN_SPACE_GAME) {
    resetSpaceGame();
  }

  fullRedraw = true;
  uiNeedsRedraw = true;
  topBarNeedsRedraw = true;
  screenChangePending = false;
}

void queueScreenButton(ButtonID button, Screen screen) {
  if (waitingForRelease) return;

  if (screen == SCREEN_REVERSE) {
    if (currentScreen == SCREEN_MODEL_SETTINGS) {
      reverseReturnScreen = SCREEN_MODEL_SETTINGS;
    } else if (currentScreen == SCREEN_CONTROLLER_SETTINGS) {
      reverseReturnScreen = SCREEN_CONTROLLER_SETTINGS;
    }
  }

  pressedButton = button;
  nextScreen = screen;
  waitingForRelease = true;
  screenChangePending = true;
  uiNeedsRedraw = true;
}

void saveKeyboardBufferToModelSlot() {
  if (keyboardBuffer.length() == 0) return;

  int targetModel = selectedModelIndex;
  if (targetModel < 0 || targetModel >= MAX_MODELS) {
    targetModel = activeModel;
  }

  if (modelNames[targetModel].length() == 0) {
    initModelDefaults(targetModel);
  }

  strncpy(models[targetModel].name, keyboardBuffer.c_str(), 19);
  models[targetModel].name[19] = '\0';
  modelNames[targetModel] = keyboardBuffer;

  activeModel = targetModel;
  selectedModelIndex = targetModel;
  currentModelName = String(models[targetModel].name);

  trimRenderX = models[targetModel].trimX[currentTrimPage];
  trimRenderY = models[targetModel].trimY[currentTrimPage];
  selectedMixIndex = 0;
  mixingNeedsRedraw = true;

  saveModels();
}

bool modelSlotUninitialized(int i) {
  for (int c = 0; c < 20; c++) {
    if ((uint8_t)models[i].name[c] != 0xFF) return false;
  }

  return true;
}

void saveModels() {
  int addr = 0;

  for (int i = 0; i < MAX_MODELS; i++) {
    EEPROM.put(addr, models[i]);
    addr += sizeof(ModelData);
  }

  EEPROM.write(EEPROM_MIX_VERSION_ADDR, MIX_STORAGE_VERSION);
  EEPROM.commit();
}

void loadModels() {
  int addr = 0;

  for (int i = 0; i < MAX_MODELS; i++) {
    EEPROM.get(addr, models[i]);
    addr += sizeof(ModelData);
  }
}

// !!!!==== TOUCH HANDELING ====!!!!
void handleTouch(int x, int y) {

  if (keyboardActive) {
    handleKeyboardTouch(x, y);
    return;
  }

  if (mixNumpadActive) {
    handleMixNumpadTouch(x, y);
    return;
  }

  // ===== MAIN =====
  if (currentScreen == SCREEN_MAIN) {
    if (isInside(x, y, MENU_BTN_X, MENU_BTN_Y, MENU_BTN_W, MENU_BTN_H)) {
      queueScreenButton(BTN_MENU, SCREEN_MENU);
      return;
    }

    if (isInside(x, y, modelPanelX, modelPanelY, modelPanelW, modelPanelH)) {
      if (!waitingForRelease) selectedButton = BTN_DRIVE_TYPE;
      queueScreenButton(BTN_DRIVE_TYPE, SCREEN_MODEL_SETTINGS);
      return;
    }
  }

  // ===== MENU =====
  else if (currentScreen == SCREEN_MENU) {

    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_MAIN);
      return;
    }

    else if (isInside(x, y, CTRL_BTN_X, 60, CTRL_BTN_W, CTRL_BTN_H)) {
      queueScreenButton(BTN_CTRL, SCREEN_CONTROLLER_SETTINGS);
      return;
    }

    else if (isInside(x, y, MODEL_BTN_X, 150, MODEL_BTN_W, MODEL_BTN_H)) {
      queueScreenButton(BTN_MODEL, SCREEN_MODEL_SETTINGS);
      return;
    }

    else if (isInside(x, y, DISPLAY_MENU_BTN_X, DISPLAY_MENU_BTN_Y, DISPLAY_MENU_BTN_W, DISPLAY_MENU_BTN_H)) {
      queueScreenButton(BTN_DISPLAY_SETTINGS, SCREEN_DISPLAY_SETTINGS);
      return;
    }

    else if (isInside(x, y, GAME_BTN_X, GAME_BTN_Y, GAME_BTN_W, GAME_BTN_H)) {
      queueScreenButton(BTN_GAME, SCREEN_SPACE_GAME);
      return;
    }
  }

  else if (currentScreen == SCREEN_DISPLAY_SETTINGS) {
    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_MENU);
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_MINUS_X, DISPLAY_SETTINGS_BRIGHTNESS_Y + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
                 DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H)) {
      pressedButton = BTN_DISPLAY_BRIGHTNESS_DEC;
      selectedButton = BTN_DISPLAY_BRIGHTNESS_DEC;
      stepDisplayOption(displayBrightness, displayBrightnessOptions, displayBrightnessOptionCount, -1);
      displaySleepBrightness = min(displaySleepBrightness, displayBrightness);
      saveDisplaySettings();
      applyDisplayBacklight();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_PLUS_X, DISPLAY_SETTINGS_BRIGHTNESS_Y + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
                 DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H)) {
      pressedButton = BTN_DISPLAY_BRIGHTNESS_INC;
      selectedButton = BTN_DISPLAY_BRIGHTNESS_INC;
      stepDisplayOption(displayBrightness, displayBrightnessOptions, displayBrightnessOptionCount, 1);
      saveDisplaySettings();
      applyDisplayBacklight();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_MINUS_X, DISPLAY_SETTINGS_TIMEOUT_Y + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
                 DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H)) {
      pressedButton = BTN_DISPLAY_TIMEOUT_DEC;
      selectedButton = BTN_DISPLAY_TIMEOUT_DEC;
      displayTimeoutIndex = constrain((int)displayTimeoutIndex - 1, 0, displayTimeoutOptionCount - 1);
      saveDisplaySettings();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_PLUS_X, DISPLAY_SETTINGS_TIMEOUT_Y + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
                 DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H)) {
      pressedButton = BTN_DISPLAY_TIMEOUT_INC;
      selectedButton = BTN_DISPLAY_TIMEOUT_INC;
      displayTimeoutIndex = constrain((int)displayTimeoutIndex + 1, 0, displayTimeoutOptionCount - 1);
      saveDisplaySettings();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_MINUS_X, DISPLAY_SETTINGS_OFF_TIMEOUT_Y + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
                 DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H)) {
      pressedButton = BTN_DISPLAY_OFF_TIMEOUT_DEC;
      selectedButton = BTN_DISPLAY_OFF_TIMEOUT_DEC;
      displayOffTimeoutIndex = constrain((int)displayOffTimeoutIndex - 1, 0, displayOffTimeoutOptionCount - 1);
      saveDisplaySettings();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_PLUS_X, DISPLAY_SETTINGS_OFF_TIMEOUT_Y + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
                 DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H)) {
      pressedButton = BTN_DISPLAY_OFF_TIMEOUT_INC;
      selectedButton = BTN_DISPLAY_OFF_TIMEOUT_INC;
      displayOffTimeoutIndex = constrain((int)displayOffTimeoutIndex + 1, 0, displayOffTimeoutOptionCount - 1);
      saveDisplaySettings();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_MINUS_X, DISPLAY_SETTINGS_SLEEP_Y + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
                 DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H)) {
      pressedButton = BTN_DISPLAY_SLEEP_DEC;
      selectedButton = BTN_DISPLAY_SLEEP_DEC;
      stepDisplayOption(displaySleepBrightness, displaySleepBrightnessOptions, displaySleepBrightnessOptionCount, -1);
      saveDisplaySettings();
      applyDisplayBacklight();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_PLUS_X, DISPLAY_SETTINGS_SLEEP_Y + DISPLAY_SETTINGS_ADJUST_BTN_Y_OFFSET,
                 DISPLAY_SETTINGS_ADJUST_BTN_W, DISPLAY_SETTINGS_ADJUST_BTN_H)) {
      pressedButton = BTN_DISPLAY_SLEEP_INC;
      selectedButton = BTN_DISPLAY_SLEEP_INC;
      stepDisplayOption(displaySleepBrightness, displaySleepBrightnessOptions, displaySleepBrightnessOptionCount, 1);
      saveDisplaySettings();
      applyDisplayBacklight();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_SLEEP_NOW_X, DISPLAY_SETTINGS_SLEEP_NOW_Y,
                 DISPLAY_SETTINGS_SLEEP_NOW_W, DISPLAY_SETTINGS_SLEEP_NOW_H)) {
      pressedButton = BTN_DISPLAY_SLEEP_NOW;
      selectedButton = BTN_DISPLAY_SLEEP_NOW;
      displayDimmed = false;
      screenAwake = false;
      suppressWakeUntilRelease = true;
      applyDisplayBacklight();
      waitingForRelease = true;
      return;
    }

    if (isInside(x, y, DISPLAY_SETTINGS_THEME_X, DISPLAY_SETTINGS_THEME_Y,
                 DISPLAY_SETTINGS_THEME_W, DISPLAY_SETTINGS_THEME_H)) {
      pressedButton = BTN_DISPLAY_THEME_TOGGLE;
      selectedButton = BTN_DISPLAY_THEME_TOGGLE;
      themeMode = (themeMode == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
      applyThemePalette();
      saveDisplaySettings();
      fullRedraw = true;
      uiNeedsRedraw = true;
      topBarNeedsRedraw = true;
      waitingForRelease = true;
      return;
    }
  }

  // ===== SPACE GAME =====
  else if (currentScreen == SCREEN_SPACE_GAME) {
    handleSpaceGameTouch(x, y);
    return;
  }

  // ===== CONTROLLER SETTINGS =====
  else if (currentScreen == SCREEN_CONTROLLER_SETTINGS) {

    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_MENU);
      return;
    }

    else if (isInside(x, y, 130, BACK_BTN_Y, 100, BACK_BTN_H)) {
      if (!waitingForRelease) {
        controllerSettingsPage = 1 - controllerSettingsPage;
        selectedButton = (controllerSettingsPage == 0) ? BTN_TRIM : BTN_EXPO;
        pressedButton = BTN_PAGE_NAV;
        fullRedraw = true;
        uiNeedsRedraw = true;
        waitingForRelease = true;
      }
      return;
    }

    else if (controllerSettingsPage == 0) {
      if (isInside(x, y, 20, 50, 200, BTN_HEIGHT)) {
        queueScreenButton(BTN_TRIM, SCREEN_TRIM);
        return;
      }

      else if (isInside(x, y, 20, 120, 200, BTN_HEIGHT)) {
        queueScreenButton(BTN_FAILSAFE, SCREEN_FAILSAFE);
        return;
      }

      else if (isInside(x, y, 20, 190, 200, BTN_HEIGHT)) {
        queueScreenButton(BTN_ENDPOINTS, SCREEN_ENDPOINTS);
        return;
      }
    }
    else {
      if (isInside(x, y, 20, 50, 200, BTN_HEIGHT)) {
        queueScreenButton(BTN_EXPO, SCREEN_EXPO);
        return;
      }

      else if (isInside(x, y, 20, 120, 200, BTN_HEIGHT)) {
        queueScreenButton(BTN_RATES, SCREEN_RATES);
        return;
      }

      else if (isInside(x, y, 20, 190, 200, BTN_HEIGHT)) {
        queueScreenButton(BTN_PROTOCOL, SCREEN_PROTOCOL);
        return;
      }
    }
  }

  else if (currentScreen == SCREEN_EXPO || currentScreen == SCREEN_RATES) {
    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_CONTROLLER_SETTINGS);
      return;
    }

    if (currentScreen == SCREEN_EXPO) {
      if (!waitingForRelease) {
        for (int i = 0; i < CHANNEL_COUNT; i++) {
          if (isInside(x, y, RATE_TAB_X(i), RATE_TAB_Y, RATE_TAB_W, RATE_TAB_H)) {
            selectedExpoChannel = i;
            focusIndex = i + 1;
            expoNeedsRedraw = true;
            uiNeedsRedraw = true;
            waitingForRelease = true;
            return;
          }
        }
      }

      if (isInside(x, y, RATE_VALUE_BOX_X, EXPO_VALUE_BOX_Y, RATE_VALUE_BOX_W, RATE_VALUE_BOX_H)) {
        openMixNumpad(NUMPAD_TARGET_EXPO_VALUE);
        focusIndex = 5;
        waitingForRelease = true;
        return;
      }
    }
    else if (currentScreen == SCREEN_RATES) {
      if (!waitingForRelease) {
        for (int i = 0; i < CHANNEL_COUNT; i++) {
          if (isInside(x, y, RATE_TAB_X(i), RATE_TAB_Y, RATE_TAB_W, RATE_TAB_H)) {
            selectedRateChannel = i;
            focusIndex = i + 1;
            ratesNeedsRedraw = true;
            uiNeedsRedraw = true;
            waitingForRelease = true;
            return;
          }
        }

        if (isInside(x, y, RATE_TOGGLE_X(0), RATE_TOGGLE_Y, RATE_TOGGLE_W, RATE_TOGGLE_H)) {
          setModelRateValue(activeModel, selectedRateChannel, RATE_LOW_VALUE);
          ratesValueDirty = true;
          ratesNeedsRedraw = true;
          uiNeedsRedraw = true;
          focusIndex = 5;
          waitingForRelease = true;
          return;
        }

        if (isInside(x, y, RATE_TOGGLE_X(1), RATE_TOGGLE_Y, RATE_TOGGLE_W, RATE_TOGGLE_H)) {
          setModelRateValue(activeModel, selectedRateChannel, RATE_NORMAL_VALUE);
          ratesValueDirty = true;
          ratesNeedsRedraw = true;
          uiNeedsRedraw = true;
          focusIndex = 6;
          waitingForRelease = true;
          return;
        }

        if (isInside(x, y, RATE_TOGGLE_X(2), RATE_TOGGLE_Y, RATE_TOGGLE_W, RATE_TOGGLE_H)) {
          setModelRateValue(activeModel, selectedRateChannel, RATE_HIGH_VALUE);
          ratesValueDirty = true;
          ratesNeedsRedraw = true;
          uiNeedsRedraw = true;
          focusIndex = 7;
          waitingForRelease = true;
          return;
        }
      }

      if (isInside(x, y, RATE_SLIDER_X, RATE_SLIDER_Y - 8, RATE_SLIDER_W, RATE_SLIDER_H + 16)) {
        int sliderValue = mapTouch(x, RATE_SLIDER_X, RATE_SLIDER_X + RATE_SLIDER_W, 1, 100);
        setModelRateValue(activeModel, selectedRateChannel, sliderValue);
        ratesValueDirty = true;
        ratesNeedsRedraw = true;
        uiNeedsRedraw = true;
        focusIndex = 8;
        return;
      }

      if (isInside(x, y, RATE_VALUE_BOX_X, RATE_VALUE_BOX_Y, RATE_VALUE_BOX_W, RATE_VALUE_BOX_H)) {
        openMixNumpad(NUMPAD_TARGET_RATE_VALUE);
        focusIndex = 8;
        waitingForRelease = true;
        return;
      }
    }
  }

  else if (currentScreen == SCREEN_ENDPOINTS) {
    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_CONTROLLER_SETTINGS);
      return;
    }

    int startY = 88;
    int spacing = 42;
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      int rowY = startY + (i * spacing);
      if (isInside(x, y, 88, rowY, 136, 30)) {
        focusIndex = i;
        endpointNeedsRedraw = true;
        uiNeedsRedraw = true;

        if (!waitingForRelease) {
          bool highSide = isEndpointHighSideSelected(i);
          bool tappedLow = isInside(x, y, 94, rowY + 4, 46, 22);
          bool tappedHigh = isInside(x, y, 170, rowY + 4, 46, 22);

          if ((tappedLow && !highSide) || (tappedHigh && highSide)) {
            endpointNumpadChannel = i;
            endpointNumpadHighSide = highSide;
            openMixNumpad(NUMPAD_TARGET_ENDPOINT_VALUE);
          }
        }

        waitingForRelease = true;
        return;
      }
    }
  }

  else if (currentScreen == SCREEN_PROTOCOL) {
    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      if (!otaUpdateInProgress) {
        queueScreenButton(BTN_BACK, SCREEN_CONTROLLER_SETTINGS);
      }
      return;
    }

    if (!waitingForRelease) {
      if (isInside(x, y, PROTOCOL_MODE_BTN_X, PROTOCOL_ELRS_BTN_Y, PROTOCOL_MODE_BTN_W, PROTOCOL_MODE_BTN_H)) {
        if (!otaModeActive) {
          cancelEspNowBinding(true);
          setModelProtocol(activeModel, PROTOCOL_ELRS);
          pressedButton = BTN_PROTOCOL_ELRS;
          selectedButton = BTN_PROTOCOL_ELRS;
          saveModels();
          fullRedraw = true;
          uiNeedsRedraw = true;
          waitingForRelease = true;
        }
        return;
      }
      else if (isInside(x, y, PROTOCOL_MODE_BTN_X, PROTOCOL_ESPNOW_BTN_Y, PROTOCOL_MODE_BTN_W, PROTOCOL_MODE_BTN_H)) {
        if (!otaModeActive) {
          setModelProtocol(activeModel, PROTOCOL_ESPNOW);
          pressedButton = BTN_PROTOCOL_ESPNOW;
          selectedButton = BTN_PROTOCOL_ESPNOW;
          saveModels();
          fullRedraw = true;
          uiNeedsRedraw = true;
          waitingForRelease = true;
        }
        return;
      }
      else if (isInside(x, y, PROTOCOL_BIND_BTN_X, PROTOCOL_BIND_BTN_Y,
                        PROTOCOL_BIND_BTN_W, PROTOCOL_BIND_BTN_H)) {
        if (otaModeActive) {
          return;
        }

        pressedButton = BTN_PROTOCOL_BIND;
        selectedButton = BTN_PROTOCOL_BIND;
        if (getModelProtocol(activeModel) == PROTOCOL_ELRS) {
          setScreen(SCREEN_ELRS);
        } else {
          if (espNowBindingMode) cancelEspNowBinding(true);
          else beginEspNowBinding();
        }

        fullRedraw = true;
        uiNeedsRedraw = true;
        waitingForRelease = true;
        return;
      }
      else if (isInside(x, y, PROTOCOL_OTA_BTN_X, PROTOCOL_OTA_BTN_Y,
                        PROTOCOL_OTA_BTN_W, PROTOCOL_OTA_BTN_H)) {
        pressedButton = BTN_PROTOCOL_OTA;
        selectedButton = BTN_PROTOCOL_OTA;

        if (otaModeActive) stopOtaMode();
        else startOtaMode();

        fullRedraw = true;
        uiNeedsRedraw = true;
        waitingForRelease = true;
        return;
      }
      else if (isInside(x, y, PROTOCOL_OTA_CFG_BTN_X, PROTOCOL_OTA_CFG_BTN_Y,
                        PROTOCOL_OTA_CFG_BTN_W, PROTOCOL_OTA_CFG_BTN_H)) {
        pressedButton = BTN_PROTOCOL_OTA_CFG;
        selectedButton = BTN_PROTOCOL_OTA_CFG;

        if (!otaUpdateInProgress) {
          setScreen(SCREEN_OTA_SETTINGS);
          fullRedraw = true;
          uiNeedsRedraw = true;
          waitingForRelease = true;
        }
        return;
      }
      else {
        return;
      }
    }

    return;
  }
  else if (currentScreen == SCREEN_OTA_SETTINGS) {
    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_PROTOCOL);
      return;
    }

    if (!waitingForRelease) {
      if (isInside(x, y, 98, 88, 130, 30)) {
        focusIndex = 1;
        beginOtaFieldEdit(KEYBOARD_TARGET_OTA_STA_SSID);
        waitingForRelease = true;
        return;
      }
      if (isInside(x, y, 98, 136, 130, 30)) {
        focusIndex = 2;
        beginOtaFieldEdit(KEYBOARD_TARGET_OTA_STA_PASSWORD);
        waitingForRelease = true;
        return;
      }
      if (isInside(x, y, 98, 184, 130, 30)) {
        focusIndex = 3;
        beginOtaFieldEdit(KEYBOARD_TARGET_OTA_AP_SSID);
        waitingForRelease = true;
        return;
      }
    }
  }
  else if (currentScreen == SCREEN_ELRS) {
    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_PROTOCOL);
      return;
    }

    if (!waitingForRelease) {
      if (isInside(x, y, ELRS_BIND_BTN_X, ELRS_BIND_BTN_Y, ELRS_BIND_BTN_W, ELRS_BIND_BTN_H)) {
        pressedButton = BTN_ELRS_BIND;
        selectedButton = BTN_ELRS_BIND;
        sendElrsBindCommand();
        fullRedraw = true;
        uiNeedsRedraw = true;
        waitingForRelease = true;
        return;
      }
    }
    return;
  }

  // ===== MODEL SETTINGS =====
  else if (currentScreen == SCREEN_MODEL_SETTINGS) {

  // ===== BACK =====
  if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
    queueScreenButton(BTN_BACK, SCREEN_MENU);
    return;
  }

  // ===== PAGE NAV =====
  else if (isInside(x, y, 130, BACK_BTN_Y, 100, BACK_BTN_H)) {
    if (!waitingForRelease) {
      modelSettingsPage = 1 - modelSettingsPage;
      selectedButton = (modelSettingsPage == 0) ? BTN_MODEL_NAME : BTN_REVERSE;
      pressedButton = BTN_PAGE_NAV;
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
    }
    return;
  }

  if (modelSettingsPage == 0) {
    // ===== MODEL NAME =====
    if (isInside(x, y, 20, 50, 200, BTN_HEIGHT)) {
      queueScreenButton(BTN_MODEL_NAME, SCREEN_MODEL_NAME);
      return;
    }

    // ===== DRIVE TYPE =====
    else if (isInside(x, y, 20, 120, 200, BTN_HEIGHT)) {
      queueScreenButton(BTN_DRIVE_TYPE, SCREEN_DRIVE_TYPE);
      return;
    }

    // ===== MIXING =====
    else if (isInside(x, y, 20, 190, 200, BTN_HEIGHT)) {
      queueScreenButton(BTN_MIXING, SCREEN_MIXING);
      return;
    }
  }
  else {
    // ===== REVERSE =====
    if (isInside(x, y, 20, 120, 200, BTN_HEIGHT)) {
      queueScreenButton(BTN_REVERSE, SCREEN_REVERSE);
      return;
    }
  }
}

  // ===== MIXING =====
  else if (currentScreen == SCREEN_MIXING) {

    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_MODEL_SETTINGS);
      return;
    }

    if (waitingForRelease) return;

    for (int i = 0; i < MIX_COUNT; i++) {
      if (isInside(x, y, MIX_TAB_X(i), MIX_TAB_Y, MIX_TAB_W, MIX_TAB_H)) {
        selectedMixIndex = i;
        mixingNeedsRedraw = true;
        uiNeedsRedraw = true;
        waitingForRelease = true;
        return;
      }
    }

    MixData &mix = models[activeModel].mixes[selectedMixIndex];
    uint8_t source = getMixSource(mix);
    uint8_t destination = getMixDestination(mix);

    if (isInside(x, y, MIX_FIELD_X, MIX_ROW_ENABLE_Y, MIX_FIELD_W, MIX_FIELD_H)) {
      mix.enabled = !mix.enabled;
    }
    else if (isInside(x, y, MIX_FIELD_X, MIX_ROW_SOURCE_Y, MIX_FIELD_W, MIX_FIELD_H)) {
      source = (source + 1) % CHANNEL_COUNT;
      setMixSource(mix, source);
      if (source == destination) {
        destination = (destination + 1) % CHANNEL_COUNT;
        setMixDestination(mix, destination);
      }
    }
    else if (isInside(x, y, MIX_FIELD_X, MIX_ROW_DEST_Y, MIX_FIELD_W, MIX_FIELD_H)) {
      destination = (destination + 1) % CHANNEL_COUNT;
      setMixDestination(mix, destination);
      if (destination == source) {
        destination = (destination + 1) % CHANNEL_COUNT;
        setMixDestination(mix, destination);
      }
    }
    else if (isInside(x, y, MIX_FIELD_X, MIX_ROW_LINK_Y, MIX_FIELD_W, MIX_FIELD_H)) {
      setMixReverseLinked(mix, !isMixReverseLinked(mix));
    }
    else if (isInside(x, y, MIX_VALUE_X, MIX_ROW_RATE_Y, MIX_VALUE_W, MIX_FIELD_H)) {
      openMixNumpad(NUMPAD_TARGET_MIX_RATE);
      waitingForRelease = true;
      return;
    }
    else if (isInside(x, y, MIX_VALUE_X, MIX_ROW_OFFS_Y, MIX_VALUE_W, MIX_FIELD_H)) {
      openMixNumpad(NUMPAD_TARGET_MIX_OFFSET);
      waitingForRelease = true;
      return;
    }
    else if (isInside(x, y, MIX_MINUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H)) {
      mix.rate = constrain(mix.rate - 1, -100, 100);
    }
    else if (isInside(x, y, MIX_PLUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H)) {
      mix.rate = constrain(mix.rate + 1, -100, 100);
    }
    else if (isInside(x, y, MIX_MINUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H)) {
      mix.offset = constrain(mix.offset - 1, -100, 100);
    }
    else if (isInside(x, y, MIX_PLUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H)) {
      mix.offset = constrain(mix.offset + 1, -100, 100);
    }
    else {
      return;
    }

    saveModels();
    mixingNeedsRedraw = true;
    uiNeedsRedraw = true;
    waitingForRelease = true;
    return;
  }

  // ===== DRIVE TYPE =====
  else if (currentScreen == SCREEN_DRIVE_TYPE) {

    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_MODEL_SETTINGS);
      return;
    }

    if (!waitingForRelease) {

      if (isInside(x, y,
                   DRIVE_BTN_X1 - DRIVE_TOUCH_PAD,
                   DRIVE_BTN_Y1 - DRIVE_TOUCH_PAD,
                   DRIVE_BTN_W + (DRIVE_TOUCH_PAD * 2),
                   DRIVE_BTN_H + (DRIVE_TOUCH_PAD * 2))) {
        currentDrive = DRIVE_TANK;
        setModelDriveType(activeModel, DRIVE_TANK);
        pressedButton = BTN_DRIVE_TANK;
        selectedButton = BTN_DRIVE_TANK;
        nextScreen = SCREEN_TANK_MODE;
        screenChangePending = true;
      }
      else if (isInside(x, y,
                        DRIVE_BTN_X2 - DRIVE_TOUCH_PAD,
                        DRIVE_BTN_Y1 - DRIVE_TOUCH_PAD,
                        DRIVE_BTN_W + (DRIVE_TOUCH_PAD * 2),
                        DRIVE_BTN_H + (DRIVE_TOUCH_PAD * 2))) {
        currentDrive = DRIVE_CAR;
        setModelDriveType(activeModel, DRIVE_CAR);
        pressedButton = BTN_DRIVE_CAR;
        selectedButton = BTN_DRIVE_CAR;
        nextScreen = SCREEN_TANK_MODE;
        screenChangePending = true;
      }
      else if (isInside(x, y,
                        DRIVE_BTN_X1 - DRIVE_TOUCH_PAD,
                        DRIVE_BTN_Y2 - DRIVE_TOUCH_PAD,
                        DRIVE_BTN_W + (DRIVE_TOUCH_PAD * 2),
                        DRIVE_BTN_H + (DRIVE_TOUCH_PAD * 2))) {
        currentDrive = DRIVE_OMNI;
        setModelDriveType(activeModel, DRIVE_OMNI);
        pressedButton = BTN_DRIVE_OMNI;
        selectedButton = BTN_DRIVE_OMNI;
        screenChangePending = false;
      }
      else if (isInside(x, y,
                        DRIVE_BTN_X2 - DRIVE_TOUCH_PAD,
                        DRIVE_BTN_Y2 - DRIVE_TOUCH_PAD,
                        DRIVE_BTN_W + (DRIVE_TOUCH_PAD * 2),
                        DRIVE_BTN_H + (DRIVE_TOUCH_PAD * 2))) {
        currentDrive = DRIVE_X_DRONE;
        setModelDriveType(activeModel, DRIVE_X_DRONE);
        pressedButton = BTN_DRIVE_X_DRONE;
        selectedButton = BTN_DRIVE_X_DRONE;
        screenChangePending = false;
      }
      else {
        return;
      }

      fullRedraw = true;
      uiNeedsRedraw = true;
      saveModels();
      waitingForRelease = true;
    }

    return;
  }

  // ===== TANK MODE =====
  else if (currentScreen == SCREEN_TANK_MODE) {
    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_DRIVE_TYPE);
      return;
    }

    if (!waitingForRelease) {
      if (isInside(x, y, 20, 70, 200, 72)) {
        setModelTankMode(activeModel, TANK_MODE_DUAL_STICK);
        pressedButton = BTN_TANK_DUAL;
        selectedButton = BTN_TANK_DUAL;
      }
      else if (isInside(x, y, 20, 160, 200, 72)) {
        setModelTankMode(activeModel, TANK_MODE_RIGHT_STICK);
        pressedButton = BTN_TANK_SINGLE;
        selectedButton = BTN_TANK_SINGLE;
      }
      else {
        return;
      }

      saveModels();
      fullRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
    }

    return;
  }

  // ===== REVERSE =====
  else if (currentScreen == SCREEN_REVERSE) {

    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, reverseReturnScreen);
      return;
    }

    int startY = 40;
    int spacing = 45;

    for (int i = 0; i < CHANNEL_COUNT; i++) {

      int ty = startY + (i * spacing);
      int tx = 120;
      int tw = 80;
      int th = 30;

      if (isInside(x, y, tx, ty, tw, th)) {

        if (!waitingForRelease) {
          bool linked[CHANNEL_COUNT] = {false, false, false, false};
          getLinkedReverseChannels(activeModel, i, linked);
          bool newState = !models[activeModel].reverse[i];

          for (int ch = 0; ch < CHANNEL_COUNT; ch++) {
            if (!linked[ch]) continue;
            models[activeModel].reverse[ch] = newState;
            reverseChannelDirty[ch] = true;
          }

          saveModels();
          reverseNeedsRedraw = true;
          uiNeedsRedraw = true;
          waitingForRelease = true;
        }

        return;
      }
    }
  }

  // ===== TRIM =====
  else if (currentScreen == SCREEN_TRIM) {

  int cx = TRIM_CENTER_X;
  int cy = TRIM_CENTER_Y;
  int s  = TRIM_SIZE;
  int sideBtnY = cy - (TRIM_BTN_SIZE / 2);
  int centerBtnX = cx - (TRIM_BTN_SIZE / 2);

  int graphLeft   = cx - s;
  int graphRight  = cx + s;
  int graphTop    = cy - s;
  int graphBottom = cy + s;

  bool didPress = false;

  // ===== BACK =====
  if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {

    if (!waitingForRelease) {

      queueScreenButton(BTN_BACK, SCREEN_CONTROLLER_SETTINGS);
    }

    return;
  }

  // ===== NEXT =====
  if (isInside(x, y, 130, BACK_BTN_Y, 100, BACK_BTN_H)) {

    if (!waitingForRelease) {

      currentTrimPage = !currentTrimPage;

      // snap render to correct position for new gimbal
      trimRenderX = models[activeModel].trimX[currentTrimPage];
      trimRenderY = models[activeModel].trimY[currentTrimPage];

      trimNeedsRedraw = true;
      trimDirty = true;
      uiNeedsRedraw = true;
      fullRedraw = true;

      waitingForRelease = true;
    }

    return;
  }

  // ===== RESET =====
  if (isInside(x, y, TRIM_RESET_BTN_X, TRIM_RESET_BTN_Y, TRIM_RESET_BTN_W, TRIM_RESET_BTN_H)) {
    if (!waitingForRelease) {
      models[activeModel].trimX[currentTrimPage] = 0;
      models[activeModel].trimY[currentTrimPage] = 0;
      saveModels();

      trimRenderX = 0;
      trimRenderY = 0;
      trimDirty = true;
      trimNeedsRedraw = true;
      uiNeedsRedraw = true;
      waitingForRelease = true;
    }
    return;
  }

  // ===== GRAPH TOUCH =====
  if (isInside(x, y, graphLeft, graphTop,
             graphRight - graphLeft,
             graphBottom - graphTop)) {

  int newX = map(x, graphLeft, graphRight, -50, 50);
  int newY = map(y, graphBottom, graphTop, -50, 50);

  newX = constrain(newX, -50, 50);
  newY = constrain(newY, -50, 50);

  models[activeModel].trimX[currentTrimPage] = newX;
  models[activeModel].trimY[currentTrimPage] = newY;
  saveModels();

  trimRenderX = newX;
  trimRenderY = newY;

  trimDirty = true;
  uiNeedsRedraw = true;

  waitingForRelease = true;
  return;
  }

  // ===== TRIM BUTTONS =====
  if (!waitingForRelease) {

    // X-
    if (isInside(x, y, trimBtnLeftX, sideBtnY, TRIM_BTN_SIZE, TRIM_BTN_SIZE)) {
      models[activeModel].trimX[currentTrimPage] =
      constrain(models[activeModel].trimX[currentTrimPage] - 1, -50, 50);
      saveModels();
      didPress = true;
    }

    // X+
    else if (isInside(x, y, trimBtnRightX, sideBtnY, TRIM_BTN_SIZE, TRIM_BTN_SIZE)) {
      models[activeModel].trimX[currentTrimPage] =
      constrain(models[activeModel].trimX[currentTrimPage] + 1, -50, 50);
      saveModels();
      didPress = true;
    }

    // Y+
    else if (isInside(x, y, centerBtnX, trimBtnTopY, TRIM_BTN_SIZE, TRIM_BTN_SIZE)) {
      models[activeModel].trimY[currentTrimPage] =
      constrain(models[activeModel].trimY[currentTrimPage] + 1, -50, 50);
      saveModels();
      didPress = true;
    }

    // Y-
    else if (isInside(x, y, centerBtnX, trimBtnBottomY, TRIM_BTN_SIZE, TRIM_BTN_SIZE)) {
      models[activeModel].trimY[currentTrimPage] =
      constrain(models[activeModel].trimY[currentTrimPage] - 1, -50, 50);
      saveModels();
      didPress = true;
    }

      if (didPress) {
        trimDirty = true;
        uiNeedsRedraw = true;
        waitingForRelease = true;
      }
    }
  }
  // ==== FAILSAFE ====
  else if (currentScreen == SCREEN_FAILSAFE) {

  if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {

    queueScreenButton(BTN_BACK, SCREEN_CONTROLLER_SETTINGS);
    return;
  }

  int startY = 85;
  int spacing = 34;

  for (int i = 0; i < CHANNEL_COUNT; i++) {

    int ty = startY + (i * spacing);
    int tx = 120;
    int tw = 80;
    int th = 30;

    if (isInside(x, y, tx, ty, tw, th)) {

      if (!waitingForRelease) {

        bool newState = !models[activeModel].failsafe[i];
        models[activeModel].failsafe[i] = newState;
        if (newState) {
          setModelFailsafeValue(activeModel, i,
                                (int)roundf(outputChannels[i] * 100.0f));
          saveFailsafeValues();
        }
        saveModels();
        failsafeDirty[i] = true;
        uiNeedsRedraw = true;

        waitingForRelease = true;
      }

      return;
    }
  }
}
  else if (currentScreen == SCREEN_MODEL_NAME) {

    // ===== BACK =====
    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_MODEL_SETTINGS);
      return;
    }

    // ===== INPUT BOX =====
    if (isInside(x, y, 10, 65, 220, 30)) {
      if (!waitingForRelease) {
        keyboardTarget = KEYBOARD_TARGET_MODEL_NAME;
        keyboardActive = true;
        keyboardLowercase = false;
        keyboardNeedsRedraw = true;
        inputBoxSelected = true;
        waitingForRelease = true;
      }
      return;
    }

    // ===== SELECT MODEL =====
    int listY = 130;

    for (int i = 0; i < 4; i++) {

      int bx = 30;
      int by = listY - 2;
      int bw = 150;
      int bh = 20;

      if (isInside(x, y, bx, by, bw, bh)) {

        if (!waitingForRelease) {

          selectedModelIndex = i;

          if (modelNames[i].length() == 0) {
            keyboardBuffer = "";
            keyboardTarget = KEYBOARD_TARGET_MODEL_NAME;
            keyboardActive = true;
            keyboardLowercase = false;
            keyboardNeedsRedraw = true;
            inputBoxSelected = true;
            modelNameDirty = true;
            modelNameNeedsRedraw = true;
            uiNeedsRedraw = true;
            waitingForRelease = true;
            return;
          }

          pendingModelHoldIndex = i;
          pendingModelHoldStart = millis();
          pendingModelHoldTriggered = false;
          waitingForRelease = true;
        }

        return;
      }

      listY += 25;
    }

    // ===== DELETE BUTTONS =====
    int rowY = 130;

    for (int i = 0; i < 4; i++) {

    int bx = 200;
    int by = rowY - 2;
    int bw = 25;
    int bh = 18;

      if (isInside(x, y, bx, by, bw, bh)) {
        if (!waitingForRelease) {

          // ===== SHIFT BOTH NAME + DATA =====
          for (int j = i; j < 3; j++) {
            modelNames[j] = modelNames[j + 1];
            models[j] = models[j + 1];
            memcpy(boundReceiverMacs[j], boundReceiverMacs[j + 1], ESP_NOW_ETH_ALEN);
            for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
              modelFailsafeValues[j][channel] = modelFailsafeValues[j + 1][channel];
              modelRateValues[j][channel] = modelRateValues[j + 1][channel];
              modelExpoValues[j][channel] = modelExpoValues[j + 1][channel];
              modelEndpointLowValues[j][channel] = modelEndpointLowValues[j + 1][channel];
              modelEndpointHighValues[j][channel] = modelEndpointHighValues[j + 1][channel];
            }
          }

          // clear last slot
          modelNames[3] = "";
          initModelDefaults(3);
          clearBoundReceiver(3);
          clearModelFailsafeValues(3);
          clearModelRateValues(3);
          clearModelExpoValues(3);
          clearModelEndpointValues(3);

          saveModels();
          saveReceiverBindings();
          saveFailsafeValues();
          saveRateValues();
          saveExpoValues();
          saveEndpointValues();

        modelNameDirty = true;
        modelNameNeedsRedraw = true;
        uiNeedsRedraw = true;
        waitingForRelease = true;
        }
      return;
      }
      rowY += 25;
    }

  }
}

bool handleKeyboardTouch(int x, int y) {

  if (!keyboardActive) return false;

  if (y < kbY) {
    if (!waitingForRelease) {
      closeKeyboard();
      inputBoxSelected = false;
      waitingForRelease = true;
    }

    return true;
  }

  for (int r = 0; r < KB_ROWS; r++) {
    for (int c = 0; c < KB_COLS; c++) {

      const char* key = getKeyboardKey(r, c);
      if (strlen(key) == 0) continue;

      int kx = kbX + c * (keyW + keySpacing) + 5;
      int ky = kbY + r * (keyH + keySpacing) + 5;

      int kw = keyW;

      // ===== SPECIAL KEYS =====
      if (strcmp(key, " ") == 0) {
        kx = kbX + 10;
        kw = kbW - 20;
      }
      else if (strcmp(key, "OK") == 0) {
        kw = keyW * 2;
      }

      if (isInside(x, y, kx, ky, kw, keyH)) {

        if (!waitingForRelease) {

          kbPressedRow = r;
          kbPressedCol = c;

          processKeyboardKey(key);
          waitingForRelease = true;
        }

        return true;
      }
    }
  }

  return true;
}

bool handleMixNumpadTouch(int x, int y) {
  if (!mixNumpadActive) return false;

  if (y < MIX_NUMPAD_Y) {
    if (!waitingForRelease) {
      closeMixNumpad(false);
      waitingForRelease = true;
    }

    return true;
  }

  const char* mixPadLayout[4][3] = {
    {"1", "2", "3"},
    {"4", "5", "6"},
    {"7", "8", "9"},
    {"<", "0", "-"}
  };

  if (isInside(x, y, MIX_NUMPAD_OK_X, MIX_NUMPAD_OK_Y, MIX_NUMPAD_OK_W, MIX_NUMPAD_OK_H)) {
    if (!waitingForRelease) {
      closeMixNumpad(true);
      waitingForRelease = true;
    }

    return true;
  }

  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 3; c++) {
      int keyX = MIX_NUMPAD_GRID_X + (c * (MIX_NUMPAD_KEY_W + MIX_NUMPAD_KEY_GAP));
      int keyY = MIX_NUMPAD_GRID_Y + (r * (MIX_NUMPAD_KEY_H + MIX_NUMPAD_KEY_GAP));

      if (!isInside(x, y, keyX, keyY, MIX_NUMPAD_KEY_W, MIX_NUMPAD_KEY_H)) continue;

      if (waitingForRelease) return true;

      const char* key = mixPadLayout[r][c];
      bool positiveOnlyTarget = (mixNumpadTarget == NUMPAD_TARGET_RATE_VALUE ||
                                 mixNumpadTarget == NUMPAD_TARGET_EXPO_VALUE ||
                                 mixNumpadTarget == NUMPAD_TARGET_ENDPOINT_VALUE);

      if (strcmp(key, "<") == 0) {
        if (mixNumpadBuffer.length() > 0) {
          mixNumpadBuffer.remove(mixNumpadBuffer.length() - 1);

          if (mixNumpadBuffer == "-") {
            mixNumpadBuffer = "";
          }

          mixNumpadNeedsRedraw = true;
          uiNeedsRedraw = true;
        }
      }
      else if (strcmp(key, "-") == 0) {
        if (!positiveOnlyTarget) {
          if (mixNumpadBuffer.startsWith("-")) {
            mixNumpadBuffer.remove(0, 1);
          }
          else {
            mixNumpadBuffer = "-" + mixNumpadBuffer;
          }

          if (mixNumpadBuffer.length() == 0) {
            mixNumpadBuffer = "-";
          }

          mixNumpadNeedsRedraw = true;
          uiNeedsRedraw = true;
        }
      }
      else {
        String candidate = mixNumpadBuffer;

        if (candidate == "0") candidate = "";
        if (candidate == "-0") candidate = "-";

        candidate += key;

        bool validLength = true;
        int digitCount = 0;
        for (int i = 0; i < candidate.length(); i++) {
          if (candidate[i] >= '0' && candidate[i] <= '9') digitCount++;
        }

        if (digitCount > 3) validLength = false;

        if (validLength) {
          int parsedValue = candidate.toInt();
          bool validCandidate = false;
          if (positiveOnlyTarget) {
            int maxValue = (mixNumpadTarget == NUMPAD_TARGET_ENDPOINT_VALUE) ? 120 : 100;
            validCandidate = (parsedValue >= 0 && parsedValue <= maxValue);
          } else {
            validCandidate = ((candidate == "-") || (parsedValue >= -100 && parsedValue <= 100));
          }

          if (validCandidate) {
            mixNumpadBuffer = candidate;
            mixNumpadNeedsRedraw = true;
            uiNeedsRedraw = true;
          }
        }
      }

      waitingForRelease = true;
      return true;
    }
  }

  return true;
}

// ==== SPLASH SCREEN ====
  void drawSplash() {

  static bool drawn = false;

  // draw ONCE only
  if (!drawn) {
    tft.fillScreen(COLOR_BG);

    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 320, anubisLogo);

    drawn = true;
  }

  // wait, then switch screens
  if (millis() - splashStartTime > 3000) {
    setScreen(SCREEN_MAIN);
  }
}

// ==== STATIC MAIN SCREEN ====
void drawMainScreenStatic() {

  int stickY = 50;
  int stickSize = 80;
  int padding = 10;

  int panelX = 10;
  int panelW = 220;
  int panelY = stickY - padding;
  int panelH = stickSize + (padding * 2);

  int modelY = panelY + panelH + 10;
  int modelBottom = MENU_BTN_Y - 10;
  int modelH = modelBottom - modelY;

  // panel background + border
  tft.fillRoundRect(panelX, modelY, panelW, modelH, 10, COLOR_STICK_PANEL);
  tft.drawRoundRect(panelX, modelY, panelW, modelH, 10, COLOR_ACCENT);
  tft.drawRoundRect(panelX+2, modelY+2, panelW-4, modelH-4, 10, TFT_DARKGREY);

  // divider
  tft.drawFastVLine(panelX + panelW / 2, modelY + 10, modelH - 20, COLOR_ACCENT);

  drawStickBase(20, stickY, stickSize);
  drawStickBase(140, stickY, stickSize);

  drawButtonBubble(
    MENU_BTN_X, MENU_BTN_Y, MENU_BTN_W, MENU_BTN_H,
    "MENU",
    pressedButton == BTN_MENU,
    selectedButton == BTN_MENU,
    -1
  );
}

void drawModelPanelSemiStatic() {
  
  int stickY = 50;
  int stickSize = 80;
  int padding = 10;

  int panelX = 10;
  int panelW = 220;
  int panelY = stickY - padding;
  int panelH = stickSize + (padding * 2);

  int modelY = panelY + panelH + 10;
  int modelBottom = MENU_BTN_Y - 10;
  int modelH = modelBottom - modelY;

  int iconX = panelX + 10;
  int iconY = modelY + 10;
  int iconW = (panelW - 20) / 2;
  int iconH = modelH - 20;

  // clear ONLY icon area
  tft.fillRect(iconX - 2, iconY, iconW + 2, iconH, COLOR_STICK_PANEL);

  if (currentDrive == DRIVE_TANK) {
    drawLegacyTankIcon(iconX, iconY, iconW, iconH, COLOR_ACCENT);
  } 
  else if (currentDrive == DRIVE_X_DRONE) {
    drawQuadXIcon(iconX, iconY, iconW, iconH, COLOR_ACCENT);
  }
  else if (currentDrive == DRIVE_CAR) {
    drawCarIcon(iconX + (iconW / 2) - 36, iconY + (iconH / 2) - 36, 3, COLOR_ACCENT);
  }
  else if (currentDrive == DRIVE_OMNI) {
    drawOmniIcon(iconX + 10, iconY + 8, iconW - 20, iconH - 16, COLOR_ACCENT);
  }

  drawRightPanel(panelX + panelW / 2, modelY, panelW / 2, modelH);
}

// ==== DYNAMIC MAIN SCREEN ====
void drawMainScreenDynamic() {

int stickY = 50;
int stickSize = 80;
int padding = 10;

int panelX = 10;
int panelW = 220;
int panelY = stickY - padding;
int panelH = stickSize + (padding * 2);

int modelY = panelY + panelH + 10;
int modelBottom = MENU_BTN_Y - 10;
int modelH = modelBottom - modelY;

int range = 25;  // max stick travel (tweak this)

// Show physical stick position on the gimbals, not reversed/mixed action output.
int lx = inputChannels[3] * range;
int ly = -inputChannels[2] * range;

int rx = inputChannels[0] * range;
int ry = -inputChannels[1] * range;

static int lastLX = 0, lastLY = 0;
static int lastRX = 0, lastRY = 0;
static int lastLatency = -1;
static float lastTelemetryVoltage = -1;
static String lastModel = "";
static unsigned long lastRightPanelRefreshTime = 0;

if (espNowLatency != lastLatency ||
    abs(telemetryVoltage - lastTelemetryVoltage) > 0.01 ||
    currentModelName != lastModel ||
    (millis() - lastRightPanelRefreshTime >= 200)) {

  int rightX = panelX + panelW / 2;
  int rightW = panelW / 2;

  drawRightPanel(rightX, modelY, rightW, modelH);

  lastLatency = espNowLatency;
  lastTelemetryVoltage = telemetryVoltage;
  lastModel = currentModelName;
  lastRightPanelRefreshTime = millis();
}

  // ===== ERASE OLD KNOBS =====
  if (lx != lastLX || ly != lastLY) {
  drawStickBase(20, stickY, stickSize);
}

if (rx != lastRX || ry != lastRY) {
  drawStickBase(140, stickY, stickSize);
}

  drawStickKnob(20, stickY, stickSize, lx, ly);
  drawStickKnob(140, stickY, stickSize, rx, ry);

lastLX = lx;
lastLY = ly;
lastRX = rx;
lastRY = ry;

// ===== REDRAW ICON =====
int iconX = panelX + 10;
int iconY = modelY + 10;
int iconW = (panelW - 20) / 2;
int iconH = modelH - 20;

if (currentDrive == DRIVE_TANK) {

static float lastLeft = 0;
static float lastRight = 0;
float tankLeftOutput = leftThrottle;
float tankRightOutput = rightThrottle;

if (getModelTankMode(activeModel) == TANK_MODE_DUAL_STICK) {
  tankLeftOutput = leftY;
  tankRightOutput = rightY;
}
else {
  float throttle = rightY;
  float turn = rightThrottle;
  tankLeftOutput = constrain(throttle + turn, -1.0, 1.0);
  tankRightOutput = constrain(throttle - turn, -1.0, 1.0);
}

if (abs(tankLeftOutput - lastLeft) > 0.01 ||
    abs(tankRightOutput - lastRight) > 0.01) {

  int barW = 6;

  int cx = iconX + iconW / 2;
  int bodyW = iconW / 2;
  int trackOffset = bodyW / 2 + 8;

  int leftX  = cx - trackOffset;
  int rightX = cx + trackOffset - barW;

  int top = iconY + 10;
  int height = iconH - 20;

  // ✅ CLEAR FIRST
  tft.fillRect(leftX - 1,  top, barW + 2, height, COLOR_STICK_PANEL);
  tft.fillRect(rightX - 1, top, barW + 2, height, COLOR_STICK_PANEL);

  // ✅ THEN redraw icon (restores tracks)
  drawLegacyTankIcon(iconX, iconY, iconW, iconH, COLOR_ACCENT);

  // ✅ THEN bars on top
  drawTankBars(iconX, iconY, iconW, iconH, tankLeftOutput, tankRightOutput);

  lastLeft = tankLeftOutput;
  lastRight = tankRightOutput;
}
}
else if (currentDrive == DRIVE_CAR) {

static float lastSteering = 0.0f;
static float lastThrottle = 0.0f;
float carSteeringOutput = getCarSteeringOutput();
float carThrottleOutput = getCarThrottleOutput();

if (abs(carSteeringOutput - lastSteering) > 0.01 ||
    abs(carThrottleOutput - lastThrottle) > 0.01) {

  tft.fillRect(iconX - 2, iconY, iconW + 2, iconH, COLOR_STICK_PANEL);
  drawCarIcon(iconX + (iconW / 2) - 36, iconY + (iconH / 2) - 36, 3, COLOR_ACCENT);
  drawCarBars(iconX, iconY, iconW, iconH, carSteeringOutput, carThrottleOutput);

  lastSteering = carSteeringOutput;
  lastThrottle = carThrottleOutput;
}
}
else if (currentDrive == DRIVE_OMNI) {

static float lastOmniArc = 0.0f;
static float lastOmniX = 0.0f;
static float lastOmniY = 0.0f;
float omniArcOutput = outputChannels[3];
float omniXOutput = outputChannels[0];
float omniYOutput = outputChannels[1];

if (abs(omniArcOutput - lastOmniArc) > 0.01 ||
    abs(omniXOutput - lastOmniX) > 0.01 ||
    abs(omniYOutput - lastOmniY) > 0.01) {

  tft.fillRect(iconX - 2, iconY, iconW + 2, iconH, COLOR_STICK_PANEL);
  drawOmniIcon(iconX + 10, iconY + 8, iconW - 20, iconH - 16, COLOR_ACCENT);
  drawOmniBars(iconX + 10, iconY + 8, iconW - 20, iconH - 16, omniArcOutput, omniXOutput, omniYOutput);

  lastOmniArc = omniArcOutput;
  lastOmniX = omniXOutput;
  lastOmniY = omniYOutput;
}
}

  // ===== MENU BUTTON =====
  static ButtonID lastMenuPressed = BTN_NONE;
  static ButtonID lastMenuSelected = BTN_NONE;

  if (pressedButton != lastMenuPressed || selectedButton != lastMenuSelected) {
    drawButtonBubble(
      MENU_BTN_X, MENU_BTN_Y, MENU_BTN_W, MENU_BTN_H,
      "MENU",
      pressedButton == BTN_MENU,
      dpadFocusVisible && selectedButton == BTN_MENU,
      -1
    );

    lastMenuPressed = pressedButton;
    lastMenuSelected = selectedButton;
  }
}

void drawLegacyTankIcon(int x, int y, int w, int h, uint16_t iconColor) {

  int cx = x + w / 2;

  int bodyW = w / 2;
  int bodyH = bodyW + 14;

  int bodyX = cx - bodyW / 2;
  int bodyY = y + (h - bodyH) / 2;

  tft.drawRect(bodyX, bodyY, bodyW, bodyH, iconColor);

  int ty = bodyY + bodyH / 2;
  tft.drawCircle(cx, ty, 6, iconColor);

  tft.drawLine(cx, ty - 6, cx, bodyY - 6, iconColor);

  int trackOffset = bodyW / 2 + 8;

  for (int i = 0; i < bodyH; i += 8) {
    tft.drawRect(cx - trackOffset, bodyY + i, 6, 4, iconColor);
    tft.drawRect(cx + trackOffset - 6, bodyY + i, 6, 4, iconColor);
  }
}

void drawLegacyQuadXIcon(int x, int y, int w, int h, uint16_t iconColor) {

  int cx = x + w / 2;
  int cy = y + h / 2;

  int armX = w / 2 - 10;
  int armY = h / 3;

  tft.drawLine(cx, cy, cx - armX, cy - armY, iconColor);
  tft.drawLine(cx, cy, cx + armX, cy - armY, iconColor);
  tft.drawLine(cx, cy, cx - armX, cy + armY, iconColor);
  tft.drawLine(cx, cy, cx + armX, cy + armY, iconColor);

  tft.drawCircle(cx - armX, cy - armY, 5, iconColor);
  tft.drawCircle(cx + armX, cy - armY, 5, iconColor);
  tft.drawCircle(cx - armX, cy + armY, 5, iconColor);
  tft.drawCircle(cx + armX, cy + armY, 5, iconColor);

  tft.drawCircle(cx, cy, 4, iconColor);
}

void drawLegacyCarIcon(int x, int y, int s, uint16_t iconColor) {

  int ax = x + 4*s;
  int baseY = y + 8*s;
  int tipY  = baseY - 6*s;

  drawThickRoundRect(x + 2*s, y + 8*s, 20*s, 10*s, 3*s, iconColor);

  drawThickLine(x + 6*s, y + 8*s, x + 10*s, y + 4*s, iconColor);
  drawThickLine(x + 10*s, y + 4*s, x + 16*s, y + 4*s, iconColor);
  drawThickLine(x + 16*s, y + 4*s, x + 20*s, y + 8*s, iconColor);

  drawThickCircle(x + 6*s, y + 20*s, 3*s, iconColor);
  drawThickCircle(x + 18*s, y + 20*s, 3*s, iconColor);

  drawThickLine(ax, baseY, ax, tipY, iconColor);
  drawThickCircle(ax, tipY, 2*s, iconColor);
}

void drawTankIcon(int x, int y, int w, int h, uint16_t iconColor) {
  drawRgb565IconTinted(x, y, w, h, icon_TANK, ICON_TANK_W, ICON_TANK_H, iconColor);
}

void drawQuadXIcon(int x, int y, int w, int h, uint16_t iconColor) {
  drawRgb565IconTinted(x, y, w, h, icon_X_DRONE, ICON_X_DRONE_W, ICON_X_DRONE_H, iconColor);
}

void drawTankBars(int x, int y, int w, int h, float left, float right) {

  int barW = 6;

  // ===== LEFT HALF OF PANEL =====
  int cx = x + w / 2;

  int bodyW = w / 2;
  int trackOffset = bodyW / 2 + 8;

  int leftX  = cx - trackOffset;
  int rightX = cx + trackOffset - barW;

  // ===== MATCH ICON VERTICAL AREA =====
  int top = y + 10;
  int height = h - 20;

  drawCenteredBar(leftX,  top, barW, height, left);
  drawCenteredBar(rightX, top, barW, height, right);
}

float getCarSteeringOutput() {
  if (getModelTankMode(activeModel) == TANK_MODE_DUAL_STICK) {
    return outputChannels[3];  // CH4 = left X
  }

  return outputChannels[0];    // CH1 = right X
}

float getCarThrottleOutput() {
  return outputChannels[1];    // CH2 = right Y
}

void drawCarBars(int x, int y, int w, int h, float steering, float throttle) {
  int carX = x + (w / 2) - 36;
  int carY = y + (h / 2) - 36;
  int carW = 72;
  int carH = 72;

  int vertBarX = carX - 12;
  int vertBarY = carY + 8;
  int vertBarW = 6;
  int vertBarH = carH - 16;

  int horizBarX = carX + 10;
  int horizBarY = carY + carH + 2;
  int horizBarW = carW - 20;
  int horizBarH = 6;

  int vertCenterY = vertBarY + (vertBarH / 2);
  int vertMaxLen = max(1, (vertBarH / 2) - 2);
  int vertLen = throttle * vertMaxLen;
  if (vertLen > 0) {
    for (int i = 0; i < vertLen; i++) {
      int drawY = vertCenterY - i;
      if (drawY >= vertBarY + 2) {
        uint16_t c = tankThrottleColor((float)i / vertMaxLen * throttle);
        tft.drawFastHLine(vertBarX, drawY, vertBarW, c);
      }
    }
  } else {
    for (int i = 0; i < -vertLen; i++) {
      int drawY = vertCenterY + i;
      if (drawY <= vertBarY + vertBarH - 2) {
        uint16_t c = tankThrottleColor((float)i / vertMaxLen * throttle);
        tft.drawFastHLine(vertBarX, drawY, vertBarW, c);
      }
    }
  }

  int centerX = horizBarX + (horizBarW / 2);
  int maxLen = horizBarW / 2;
  int len = steering * maxLen;
  if (len > 0) {
    for (int i = 0; i < len; i++) {
      int drawX = centerX + i;
      if (drawX <= horizBarX + horizBarW) {
        uint16_t c = tankThrottleColor((float)i / maxLen * steering);
        tft.drawFastVLine(drawX, horizBarY, horizBarH, c);
      }
    }
  } else {
    for (int i = 0; i < -len; i++) {
      int drawX = centerX - i;
      if (drawX >= horizBarX) {
        uint16_t c = tankThrottleColor((float)i / maxLen * steering);
        tft.drawFastVLine(drawX, horizBarY, horizBarH, c);
      }
    }
  }
}

void drawOmniBars(int x, int y, int w, int h, float ch4, float ch1, float ch2) {
  int cx = x + w / 2;
  int cy = y + h / 2;
  int innerR = min(w, h) / 4;
  int outerR = innerR + 4;

  int arcHalfSpanDeg = 110;
  int arcCenterDeg = -90;
  int arcOffsetDeg = (int)roundf(ch4 * arcHalfSpanDeg);
  int arcStart = min(arcCenterDeg, arcCenterDeg + arcOffsetDeg);
  int arcEnd = max(arcCenterDeg, arcCenterDeg + arcOffsetDeg);

  if (arcOffsetDeg != 0) {
    for (int angle = arcStart; angle <= arcEnd; angle += 2) {
      float radians = angle * 0.0174532925f;
      float intensity = (arcHalfSpanDeg == 0) ? 0.0f : fabs((float)(angle - arcCenterDeg) / arcHalfSpanDeg);
      uint16_t c = tankThrottleColor((ch4 >= 0.0f ? 1.0f : -1.0f) * intensity);

      for (int radius = innerR + 1; radius < outerR; radius++) {
        int px = cx + (int)roundf(cosf(radians) * radius);
        int py = cy + (int)roundf(sinf(radians) * radius);
        tft.drawPixel(px, py, c);
      }
    }
  }

  int crossRange = max(6, innerR - 6);
  int markerX = cx + (int)roundf(ch1 * crossRange);
  int markerY = cy - (int)roundf(ch2 * crossRange);

  tft.drawFastHLine(cx - crossRange, cy, crossRange * 2, fadeColor(COLOR_ACCENT, 0.45f));
  tft.drawFastVLine(cx, cy - crossRange, crossRange * 2, fadeColor(COLOR_ACCENT, 0.45f));

  tft.drawFastHLine(markerX - 5, markerY, 11, COLOR_ACCENT_HI);
  tft.drawFastVLine(markerX, markerY - 5, 11, COLOR_ACCENT_HI);
  tft.drawPixel(markerX, markerY, COLOR_TEXT);
}

  // ==== RIGHT PANEL INFO ====
void drawRightPanel(int x, int y, int w, int h) {

  // ===== CLEAR ONLY RIGHT SIDE =====
  tft.fillRect(x + 2, y + 2, w - 4, h - 4, COLOR_STICK_PANEL);

  // ===== TEXT STYLING =====
  tft.setTextColor(COLOR_TEXT);
  tft.setTextFont(2);

  int lineY = y + 5;

  // ===== MODEL NAME =====
  tft.setCursor(x + 10, lineY);
  tft.print("Model:");

  tft.setCursor(x + 14, lineY + 15);
  tft.setTextColor(COLOR_ACCENT);
  tft.print(currentModelName);

  // ===== LATENCY =====
  lineY += 30;

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(x + 10, lineY);
  tft.print("Latency:");

  int latencySnapshot = espNowLatency;
  tft.setCursor(x + 14, lineY + 15);
  tft.setTextColor(COLOR_ACCENT);
  if (hasEspNowHeaderSignal(millis())) {
    tft.print(max(0, latencySnapshot));
    tft.print(" ms");
  } else {
    tft.print("-- ms");
  }

  // ===== VOLTAGE =====
  lineY += 30;

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(x + 10, lineY);
  tft.print("Voltage:");

  tft.setCursor(x + 14, lineY + 15);
  tft.setTextColor(COLOR_ACCENT);
  if (telemetryVoltage > 0.05f) {
    tft.print(telemetryVoltage, 2);
    tft.print(" V");
  } else {
    tft.print("--.-- V");
  }
}

void drawCenteredBar(int x, int y, int w, int h, float value) {

  int top = y + 10;          // match tank body top
  int bottom = y + h - 10;   // match tank body bottom

  int centerY = (top + bottom) / 2;
  int maxLen = (bottom - top) / 2;

  int len = value * maxLen;

  if (len > 0) {
    for (int i = 0; i < len; i++) {
      int drawY = centerY - i;
      if (drawY >= top) {
        uint16_t c = tankThrottleColor((float)i / maxLen * value);
        tft.drawFastHLine(x, drawY, w, c);
      }
    }
  } else {
    for (int i = 0; i < -len; i++) {
      int drawY = centerY + i;
      if (drawY <= bottom) {
        uint16_t c = tankThrottleColor((float)i / maxLen * value);
        tft.drawFastHLine(x, drawY, w, c);
      }
    }
  }
}

void drawQuadBars(int x, int y, int w, int h,
                  float m1, float m2, float m3, float m4) {

  int barW = 5;

  drawBottomBar(x, y, barW, h, m1);
  drawBottomBar(x + w - barW, y, barW, h, m2);

  drawBottomBar(x, y + h/2, barW, h/2, m3);
  drawBottomBar(x + w - barW, y + h/2, barW, h/2, m4);
}

void drawBottomBar(int x, int y, int w, int h, float value) {
  value = constrain(value, 0.0, 1.0);

  int len = value * h;

  for (int i = 0; i < len; i++) {
    float t = (float)i / h;
    uint16_t c = throttleColor(t * 2 - 1); // reuse gradient
    tft.drawFastHLine(x, y + h - i, w, c);
  }
}
    void drawSignalBars(int x, int y, int strength) {
    // strength = 0–4

    int barWidth = 6;
    int spacing  = 3;

    int heights[4] = {6, 10, 14, 18};

    for (int i = 0; i < 4; i++) {

      int bx = x + i * (barWidth + spacing);
      int by = y + (13 - heights[i]);  // align bottoms

      uint16_t color = (i < strength) ? COLOR_SIG : COLOR_PANEL;

      tft.fillRect(bx, by, barWidth, heights[i], color);
    }
  }

  void drawBattery(int x, int y, int level, bool present, bool charging) {
  // level = 0–100

  int w = 24;
  int h = 12;
  int padding = 2;

  // ===== OUTLINE =====
  tft.drawRoundRect(x, y, w, h, 2, COLOR_TEXT);

  // terminal
  tft.fillRect(x + w, y + (h / 3), 3, h / 3, COLOR_TEXT);

  if (!present) {
    tft.drawLine(x + 4, y + 3, x + w - 4, y + h - 3, COLOR_ACCENT_HI);
    tft.drawLine(x + w - 4, y + 3, x + 4, y + h - 3, COLOR_ACCENT_HI);
    return;
  }

  // ===== FILL WIDTH =====
  int fillWidth = map(level, 0, 100, 0, w - (padding * 2));

  uint16_t color;

  if (level > 60) color = COLOR_SIG;
  else if (level > 30) color = COLOR_ACCENT;
  else color = TFT_RED;

  // ===== FILL =====
  tft.fillRoundRect(
    x + padding,
    y + padding,
    fillWidth,
    h - (padding * 2),
    2,
    color
  );

  if (charging) {
    int boltX = x + 9;
    int boltY = y + 1;
    tft.fillTriangle(boltX + 3, boltY, boltX, boltY + 5, boltX + 4, boltY + 5, COLOR_BG);
    tft.fillTriangle(boltX + 3, boltY + 10, boltX + 2, boltY + 5, boltX + 6, boltY + 5, COLOR_BG);
  }
}

void drawTopBarStatic() {
  ProtocolType protocol = getModelProtocol(activeModel);

  // ===== CLEAR AREA =====
  tft.fillRect(0, 0, 240, 35, COLOR_BG);

  // ===== TITLE =====
  tft.setTextFont(4);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 5);
  tft.print("Anubis");

  // ===== STATUS LABEL =====
  tft.setTextFont(2);
  tft.setTextColor(COLOR_TEXT);

  if (protocol == PROTOCOL_ESPNOW) {
    tft.setCursor(104, 10);
    tft.print("Latency");
  }
  else {
    tft.setCursor(110, 10);
    tft.print("RSSI");
  }
}

void drawTopBarDynamic() {

  static unsigned long lastBlinkTime = 0;
  static bool blinkState = true;
  static bool lastBlinkState = true;
  static bool lastBatteryPresent = true;
  static bool lastBatteryCharging = false;
  static bool lastNoSignalFlashState = true;
  static bool lastNoSignalEligible = false;
  static bool lastEspNowSignalState = false;
  static ProtocolType lastProtocol = PROTOCOL_ELRS;
  static int lastHeaderLatency = -1;

  unsigned long now = millis();
  ProtocolType protocol = getModelProtocol(activeModel);
  bool protocolChanged = (protocol != lastProtocol);
  int latencySnapshot = espNowLatency;

  if (protocolChanged) {
    drawTopBarStatic();
    lastSignal = -1;
    lastHeaderLatency = -1;
    lastEspNowSignalState = false;
    lastNoSignalFlashState = true;
  }

  if (batteryPresent && !batteryCharging && batteryLevel < 10) {
    if (now - lastBlinkTime >= 1000) {
      blinkState = !blinkState;
      lastBlinkTime = now;
    }
  } 
    else {
      blinkState = true; // always visible if not low
    }

  bool espNowSignalPresent = hasEspNowHeaderSignal(now);
  bool noSignalEligible = (lastEspNowAliveTime > 0) ||
    (espNowProtocolStartTime > 0 &&
     (now - espNowProtocolStartTime) > ESPNOW_HEADER_SIGNAL_TIMEOUT_MS);
  bool noSignalFlashState = ((now / 400) % 2) == 0;

  bool headerStateChanged = protocolChanged ||
    batteryLevel != lastBattery ||
    blinkState != lastBlinkState ||
    batteryPresent != lastBatteryPresent ||
    batteryCharging != lastBatteryCharging;

  if (protocol == PROTOCOL_ESPNOW) {
    headerStateChanged = headerStateChanged ||
      espNowSignalPresent != lastEspNowSignalState ||
      noSignalEligible != lastNoSignalEligible ||
      latencySnapshot != lastHeaderLatency ||
      noSignalFlashState != lastNoSignalFlashState;
  }
  else {
    headerStateChanged = headerStateChanged ||
      signalStrength != lastSignal;
  }

  if (headerStateChanged) {
    tft.fillRect(104, 5, 94, 25, COLOR_BG);

    if (protocol == PROTOCOL_ESPNOW) {
      if (espNowSignalPresent) {
        char latencyText[16];
        int displayLatency = max(0, latencySnapshot);
        snprintf(latencyText, sizeof(latencyText), "%d ms", displayLatency);
        tft.setTextFont(2);
        tft.setTextColor(getLatencyIndicatorColor(displayLatency), COLOR_BG);
        tft.drawRightString(latencyText, 194, 10, 2);
      }
      else {
        tft.setTextFont(2);
        if (noSignalEligible) {
          if (noSignalFlashState) {
            tft.setTextColor(TFT_RED, COLOR_BG);
            tft.drawString("No Signal", 118, 10, 2);
          }
        } else {
          tft.setTextColor(COLOR_TEXT, COLOR_BG);
          tft.drawRightString("-- ms", 194, 10, 2);
        }
      }
    }
    else {
      drawSignalBars(150, 10, signalStrength);
    }

    tft.fillRect(200, 10, 30, 20, COLOR_BG);
    if (blinkState) {
      drawBattery(200, 10, batteryLevel, batteryPresent, batteryCharging);
    }
    else {
      // clear battery area when "off"
      tft.fillRect(200, 10, 30, 20, COLOR_BG);
    }

    lastSignal = signalStrength;
    lastHeaderLatency = latencySnapshot;
    lastBattery = batteryLevel;
    lastBlinkState = blinkState;
    lastBatteryPresent = batteryPresent;
    lastBatteryCharging = batteryCharging;
    lastEspNowSignalState = espNowSignalPresent;
    lastNoSignalFlashState = noSignalFlashState;
    lastNoSignalEligible = noSignalEligible;
    lastProtocol = protocol;
  }

  if (protocol == PROTOCOL_ELRS && !elrsLinkActive) {
    signalStrength = 0;
  }
}

uint16_t fadeColor(uint16_t color, float factor) {
  // factor: 1.0 = full color, 0.0 = black

  uint8_t r = (color >> 11) & 0x1F;
  uint8_t g = (color >> 5)  & 0x3F;
  uint8_t b = color & 0x1F;

  r = r * factor;
  g = g * factor;
  b = b * factor;

  return (r << 11) | (g << 5) | b;
}

uint16_t tankThrottleColor(float t) {
  // t = -1.0 → 1.0
  t = constrain(t, -1.0, 1.0);

  float v = abs(t);  // distance from center (0 → 1)

  // green → red
  uint8_t r = 255 * v;
  uint8_t g = 255 * (1.0 - v);
  uint8_t b = 0;

  return tft.color565(r, g, b);
}

uint16_t throttleColor(float t) {
  // t = -1.0 → 1.0 (tank)
  // t =  0.0 → 1.0 (quad)

  t = constrain(t, -1.0, 1.0);

  // map to 0–1
  float v = (t + 1.0) * 0.5;

  uint8_t r, g, b = 0;

  if (v < 0.5) {
    // green → yellow
    float f = v * 2;
    r = 255 * f;
    g = 255;
  } else {
    // yellow → red
    float f = (v - 0.5) * 2;
    r = 255;
    g = 255 * (1 - f);
  }

  return tft.color565(r, g, b);
}

void drawStickBaseDirect(int x, int y, int size) {

  int radius = 10;

  // ===== GRADIENT BACKGROUND =====
  for (int i = 0; i < size; i++) {

    int shade = map(i, 0, size, 10, -15);

    uint8_t r = (COLOR_PANEL >> 11) & 0x1F;
    uint8_t g = (COLOR_PANEL >> 5)  & 0x3F;
    uint8_t b = COLOR_PANEL & 0x1F;

    int nr = constrain(r + shade / 2, 0, 31);
    int ng = constrain(g + shade,     0, 63);
    int nb = constrain(b + shade / 2, 0, 31);

    uint16_t c = (nr << 11) | (ng << 5) | nb;

    int xOffset = 0;

    if (i < radius) {
      int dy = radius - i;
      xOffset = radius - sqrt(radius * radius - dy * dy);
    }
    else if (i >= size - radius) {
      int dy = i - (size - radius);
      xOffset = radius - sqrt(radius * radius - dy * dy);
    }

    tft.drawFastHLine(x + xOffset, y + i, size - (2 * xOffset), c);
  }

  // border + crosshair
  tft.drawRoundRect(x, y, size, size, radius, COLOR_ACCENT);
  tft.drawRoundRect(x+2, y+2, size-4, size-4, radius, TFT_DARKGREY);

  int cx = x + size / 2;
  int cy = y + size / 2;

  tft.drawFastHLine(x + 10, cy, size - 20, COLOR_ACCENT);
  tft.drawFastVLine(cx, y + 10, size - 20, COLOR_ACCENT);

  tft.drawCircle(cx, cy, 15, COLOR_PANEL);
}

void drawStickBase(int x, int y, int size) {

  static bool spriteReady = false;
  static int spriteSize = 0;

  if (stickBaseSpriteNeedsRebuild && spriteReady) {
    stickBaseSprite.deleteSprite();
    spriteReady = false;
  }

  if (!spriteReady || spriteSize != size) {
    if (spriteReady) {
      stickBaseSprite.deleteSprite();
      spriteReady = false;
    }

    stickBaseSprite.setColorDepth(16);

    if (stickBaseSprite.createSprite(size, size) == nullptr) {
      drawStickBaseDirect(x, y, size);
      return;
    }

    stickBaseSprite.fillSprite(COLOR_BG);

    int radius = 10;

    for (int i = 0; i < size; i++) {

      int shade = map(i, 0, size, 10, -15);

      uint8_t r = (COLOR_PANEL >> 11) & 0x1F;
      uint8_t g = (COLOR_PANEL >> 5)  & 0x3F;
      uint8_t b = COLOR_PANEL & 0x1F;

      int nr = constrain(r + shade / 2, 0, 31);
      int ng = constrain(g + shade,     0, 63);
      int nb = constrain(b + shade / 2, 0, 31);

      uint16_t c = (nr << 11) | (ng << 5) | nb;

      int xOffset = 0;

      if (i < radius) {
        int dy = radius - i;
        xOffset = radius - sqrt(radius * radius - dy * dy);
      }
      else if (i >= size - radius) {
        int dy = i - (size - radius);
        xOffset = radius - sqrt(radius * radius - dy * dy);
      }

      stickBaseSprite.drawFastHLine(xOffset, i, size - (2 * xOffset), c);
    }

    stickBaseSprite.drawRoundRect(0, 0, size, size, radius, COLOR_ACCENT);
    stickBaseSprite.drawRoundRect(2, 2, size - 4, size - 4, radius, TFT_DARKGREY);

    int cx = size / 2;
    int cy = size / 2;

    stickBaseSprite.drawFastHLine(10, cy, size - 20, COLOR_ACCENT);
    stickBaseSprite.drawFastVLine(cx, 10, size - 20, COLOR_ACCENT);
    stickBaseSprite.drawCircle(cx, cy, 15, COLOR_PANEL);

    spriteReady = true;
    spriteSize = size;
    stickBaseSpriteNeedsRebuild = false;
  }

  stickBaseSprite.pushSprite(x, y);
}

void drawStickKnob(int x, int y, int size, int posX, int posY) {

  float maxRadius = size / 2 - 10;

  float dist = sqrt(posX * posX + posY * posY);

  if (dist > maxRadius) {
    posX = posX * maxRadius / dist;
    posY = posY * maxRadius / dist;
  }

  int radius = 10;

  int cx = x + size / 2;
  int cy = y + size / 2;

  int px = cx + posX;
  int py = cy + posY;

  // ===== DRAW KNOB =====
  tft.drawCircle(px, py, 8, COLOR_ACCENT);
  tft.drawCircle(px, py, 10, COLOR_ACCENT_HI);
  tft.fillCircle(px, py, 6, COLOR_ACCENT_HI);
  tft.fillCircle(px - 2, py - 2, 2, COLOR_TEXT);
}

// ==== STATIC REVERSE ====
void drawReverseStatic() {
  tft.fillScreen(COLOR_BG);

  // BACK button
  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    false,
    (dpadFocusVisible && focusIndex == CHANNEL_COUNT),
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  int startY = 40;
  int spacing = 45;

  for (int i = 0; i < CHANNEL_COUNT; i++) {

    int y = startY + (i * spacing);
    bool linked = hasLinkedReversePeers(activeModel, i);

    // Labels
    tft.setTextColor(linked ? COLOR_ACCENT_HI : COLOR_TEXT);
    tft.setCursor(20, y + 10);
    tft.print("CH");
    tft.print(i + 1);
    tft.setCursor(54, y + 10);
    tft.print(getChannelAxisName(i));

    if (linked) {
      tft.setCursor(82, y + 10);
      tft.print("*");
    }

    // Toggle outline
    tft.drawRoundRect(120, y, 80, 30, 10, COLOR_ACCENT);

    int tx = 120;
    int ty = y;
    int tw = 80;
    int th = 30;

    // LEFT label (OFF)
    tft.setTextColor(linked ? COLOR_ACCENT_HI : COLOR_TEXT);
    tft.setCursor(tx - 35, ty + (th / 2) - 6);
    tft.print("OFF");

    // RIGHT label (ON)
    tft.setCursor(tx + tw + 5, ty + (th / 2) - 6);
    tft.print("ON");
  }
}

// ==== DYNAMIC REVERSE ====
void drawReverseDynamic() {

  int startY = 40;
  int spacing = 45;

  for (int i = 0; i < CHANNEL_COUNT; i++) {

    if (!reverseChannelDirty[i] && !reverseNeedsRedraw) continue;

    int y = startY + (i * spacing);
    bool linked = hasLinkedReversePeers(activeModel, i);

    int tx = 120;
    int ty = y;
    int tw = 80;
    int th = 30;


    // CLEAR FULL TOGGLE AREA
    tft.fillRect(tx, ty - 2, tw, th + 4, COLOR_BG);

    // redraw track background
    tft.fillRoundRect(tx + 2, ty + 2, tw - 4, th - 4, 10, COLOR_PANEL);

    // redraw outline
    tft.drawRoundRect(tx, ty, tw, th, 10, COLOR_ACCENT);

    if (linked) {
      tft.drawRoundRect(tx - 2, ty - 2, tw + 4, th + 4, 10, COLOR_ACCENT_HI);
    }

    if (currentScreen == SCREEN_REVERSE && dpadFocusVisible && focusIndex == i) {
      tft.drawRoundRect(tx - 2, ty - 2, tw + 4, th + 4, 10, COLOR_ACCENT_HI);
    }

    // knob radius
    int r = 10;

    int knobX = models[activeModel].reverse[i]
    ? (tx + tw - r - 2)
    : (tx + r + 2);

    uint16_t knobColor = models[activeModel].reverse[i]
    ? COLOR_ACCENT
    : COLOR_TEXT;

    tft.fillCircle(knobX, ty + th / 2, 10, knobColor);

    reverseChannelDirty[i] = false;
  }

  reverseNeedsRedraw = false;
}

// ==== STATIC TRIM ====
void drawTrimGraphBase() {
  int cx = TRIM_CENTER_X;
  int cy = TRIM_CENTER_Y;
  int s  = TRIM_SIZE;
  int step = 20;

  tft.fillRect(cx - s, cy - s, (s * 2) + 1, (s * 2) + 1, COLOR_BG);

  for (int i = -s; i <= s; i += step) {
    tft.drawFastVLine(cx + i, cy - s, s * 2, TFT_DARKGREY);
  }

  for (int i = -s; i <= s; i += step) {
    tft.drawFastHLine(cx - s, cy + i, s * 2, TFT_DARKGREY);
  }

  tft.drawLine(cx - s, cy, cx + s, cy, COLOR_ACCENT);
  tft.drawLine(cx - s, cy + 1, cx + s, cy + 1, COLOR_ACCENT);
  tft.drawLine(cx, cy - s, cx, cy + s, COLOR_ACCENT);
  tft.drawLine(cx + 1, cy - s, cx + 1, cy + s, COLOR_ACCENT);
}

void drawTrimButtons() {
  int cx = TRIM_CENTER_X;
  int cy = TRIM_CENTER_Y;
  int s  = TRIM_SIZE;
  int sideBtnY = cy - (TRIM_BTN_SIZE / 2);
  int centerBtnX = cx - (TRIM_BTN_SIZE / 2);

  trimBtnLeftX  = cx - s - TRIM_BTN_SIZE - TRIM_BTN_GAP;
  trimBtnRightX = cx + s + TRIM_BTN_GAP;
  trimBtnTopY = cy - s - TRIM_BTN_SIZE - TRIM_BTN_GAP;
  trimBtnBottomY = cy + s + TRIM_BTN_GAP;

  tft.fillRoundRect(trimBtnLeftX, sideBtnY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(trimBtnLeftX, sideBtnY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);
  if (currentScreen == SCREEN_TRIM && dpadFocusVisible && focusIndex == 2) {
    tft.drawRoundRect(trimBtnLeftX - 2, sideBtnY - 2, TRIM_BTN_SIZE + 4, TRIM_BTN_SIZE + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawString("-", trimBtnLeftX + 10, sideBtnY + 4);

  tft.fillRoundRect(trimBtnRightX, sideBtnY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(trimBtnRightX, sideBtnY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);
  if (currentScreen == SCREEN_TRIM && dpadFocusVisible && focusIndex == 3) {
    tft.drawRoundRect(trimBtnRightX - 2, sideBtnY - 2, TRIM_BTN_SIZE + 4, TRIM_BTN_SIZE + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawString("+", trimBtnRightX + 9, sideBtnY + 4);

  tft.fillRoundRect(centerBtnX, trimBtnTopY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(centerBtnX, trimBtnTopY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);
  if (currentScreen == SCREEN_TRIM && dpadFocusVisible && focusIndex == 4) {
    tft.drawRoundRect(centerBtnX - 2, trimBtnTopY - 2, TRIM_BTN_SIZE + 4, TRIM_BTN_SIZE + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawString("+", centerBtnX + 10, trimBtnTopY + 4);

  tft.fillRoundRect(centerBtnX, trimBtnBottomY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(centerBtnX, trimBtnBottomY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);
  if (currentScreen == SCREEN_TRIM && dpadFocusVisible && focusIndex == 5) {
    tft.drawRoundRect(centerBtnX - 2, trimBtnBottomY - 2, TRIM_BTN_SIZE + 4, TRIM_BTN_SIZE + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawString("-", centerBtnX + 10, trimBtnBottomY + 6);
}

void drawTrimStatic() {
  tft.fillScreen(COLOR_BG);

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(90, 20);

  // BACK
  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK", false, (dpadFocusVisible && focusIndex == 0), BACK_TEXT_OFFSET);

  // NEXT button (right side)
  const char* navLabel = (currentTrimPage == 0) ? "NEXT" : "PREV";
  drawButtonBubble(130, BACK_BTN_Y, 100, BACK_BTN_H, navLabel, false, (dpadFocusVisible && focusIndex == 1), 40);
  drawButtonBubble(
    TRIM_RESET_BTN_X, TRIM_RESET_BTN_Y, TRIM_RESET_BTN_W, TRIM_RESET_BTN_H,
    "RESET", false, (dpadFocusVisible && focusIndex == 6), 24);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  // ===== GIMBAL LABEL =====
  tft.setTextColor(COLOR_TEXT);

  // center-ish alignment
  int labelY = FOOTER_Y - 22;

  if (currentTrimPage == 0) {
    tft.drawCentreString("Left Stick", 120, labelY, 2);
  }
  else {
    tft.drawCentreString("Right Stick", 120, labelY, 2);
  }

  drawTrimGraphBase();
  drawTrimButtons();
}

// ==== DYNAMIC TRIM ====
void drawTrimDynamic() {

  static int lastPx = TRIM_CENTER_X;
  static int lastPy = TRIM_CENTER_Y;

  if (!trimDirty && !trimNeedsRedraw) return;

  int cx = TRIM_CENTER_X;
  int cy = TRIM_CENTER_Y;

  // ===== TARGET VALUES FROM ACTIVE MODEL =====
  int targetX = models[activeModel].trimX[currentTrimPage];
  int targetY = models[activeModel].trimY[currentTrimPage];

  // ===== SMOOTH ANIMATION =====
  trimRenderX += (targetX - trimRenderX) * 0.2;
  trimRenderY += (targetY - trimRenderY) * 0.2;

  int px = cx + trimRenderX;
  int py = cy - trimRenderY;

  drawTrimGraphBase();
  drawTrimButtons();

  // ===== DRAW NEW DOT =====
  tft.fillCircle(px, py, 5, COLOR_ACCENT_HI);

  // store for next frame
  lastPx = px;
  lastPy = py;

  trimDirty = false;
  trimNeedsRedraw = false;

  // Keep the live values in a dedicated top band above the upper trim button.
  tft.fillRect(0, TRIM_VALUE_Y, 240, 18, COLOR_BG);

  char xText[12];
  char yText[12];
  snprintf(xText, sizeof(xText), "X:%+d", targetX);
  snprintf(yText, sizeof(yText), "Y:%+d", targetY);

  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString(xText, 56, TRIM_VALUE_Y, 2);
  tft.drawCentreString(yText, 184, TRIM_VALUE_Y, 2);

  // ===== KEEP ANIMATING UNTIL SETTLED =====
  if (abs(trimRenderX - targetX) > 0.1 ||
      abs(trimRenderY - targetY) > 0.1) {
    uiNeedsRedraw = true;
  }
}

// ==== FAILSAFE STATIC ====
void drawFailsafeStatic() {
  tft.fillScreen(COLOR_BG);

  // ===== HEADER =====
  tft.setTextColor(COLOR_TEXT);
  tft.setTextFont(2);

  tft.drawCentreString(
    "Position input to desired value",
    120, 40, 2);

  tft.drawCentreString(
    "before setting failsafe",
    120, 60, 2);

  // ===== BACK BUTTON =====
  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    false,
    (dpadFocusVisible && focusIndex == CHANNEL_COUNT),
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  // ===== CHANNEL ROWS =====
  int startY = 85;
  int spacing = 34;

  for (int i = 0; i < CHANNEL_COUNT; i++) {

    int y = startY + (i * spacing);

    // CH label
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(20, y + 10);
    tft.print("CH");
    tft.print(i + 1);

    int tx = 120;
    int ty = y;
    int tw = 80;
    int th = 30;

    // ===== SWITCH OUTLINE =====
    tft.drawRoundRect(tx, ty, tw, th, 10, COLOR_ACCENT);

    // ===== LABELS =====
    tft.setTextColor(COLOR_TEXT);

    // OFF (left)
    tft.setCursor(tx - 35, ty + (th / 2) - 6);
    tft.print("OFF");

    // ON (right)
    tft.setCursor(tx + tw + 5, ty + (th / 2) - 6);
    tft.print("ON");
  }
}

// ==== FAILSAFE DYNAMIC ====
void drawFailsafeDynamic() {

  int startY = 85;
  int spacing = 34;
  char valueText[16];

  for (int i = 0; i < CHANNEL_COUNT; i++) {

    if (!failsafeDirty[i] && !failsafeNeedsRedraw) continue;

    int y = startY + (i * spacing);

    int tx = 120;
    int ty = y;
    int tw = 80;
    int th = 30;

    tft.fillRect(50, y, 64, 24, COLOR_BG);
    snprintf(valueText, sizeof(valueText), "%+d%%", getModelFailsafeValue(activeModel, i));
    tft.setTextColor(models[activeModel].failsafe[i] ? COLOR_ACCENT_HI : COLOR_TEXT);
    tft.drawString(valueText, 54, y + 10, 2);

    // ===== CLEAR FULL TOGGLE AREA =====
    tft.fillRect(tx, ty - 2, tw, th + 4, COLOR_BG);

    // ===== TRACK =====
    tft.fillRoundRect(tx + 2, ty + 2, tw - 4, th - 4, 10, COLOR_PANEL);

    // ===== OUTLINE =====
    tft.drawRoundRect(tx, ty, tw, th, 10, COLOR_ACCENT);

    if (currentScreen == SCREEN_FAILSAFE && dpadFocusVisible && focusIndex == i) {
      tft.drawRoundRect(tx - 2, ty - 2, tw + 4, th + 4, 10, COLOR_ACCENT_HI);
    }

    // ===== KNOB (IDENTICAL TO REVERSE) =====
    int r = 10;

    int knobX = models[activeModel].failsafe[i]
      ? (tx + tw - r - 2)
      : (tx + r + 2);

    uint16_t knobColor = models[activeModel].failsafe[i]
      ? COLOR_ACCENT
      : COLOR_TEXT;

    tft.fillCircle(knobX, ty + th / 2, 10, knobColor);

    failsafeDirty[i] = false;
  }

  failsafeNeedsRedraw = false;
}

// ==== KEYBOARD STATIC ====
void drawKeyboardStatic() {

  tft.fillRect(kbX, kbY, kbW, kbH, COLOR_BG);
  tft.drawFastHLine(0, kbY - 2, 240, COLOR_ACCENT);

  for (int r = 0; r < KB_ROWS; r++) {
    for (int c = 0; c < KB_COLS; c++) {

      const char* key = getKeyboardKey(r, c);
      if (strlen(key) == 0) continue;

      int x = kbX + c * (keyW + keySpacing) + 5;
      int y = kbY + r * (keyH + keySpacing) + 5;

      int w = keyW;

      // ===== SPECIAL KEYS =====
      if (strcmp(key, " ") == 0) {
        x = kbX + 10;              // ← FORCE LEFT ALIGN
        w = kbW - 20;              // ← FULL WIDTH
      }
      else if (strcmp(key, "OK") == 0) {
        w = keyW * 2;
      }

      tft.drawRoundRect(x, y, w, keyH, 4, COLOR_ACCENT);

      tft.setTextColor(COLOR_TEXT);
      tft.drawCentreString(key, x + w/2, y + 8, 2);
    }
  }
}

void drawKeyboardPreview() {
  if (!isOtaKeyboardTarget(keyboardTarget)) return;

  char preview[33];
  const char *source = keyboardBuffer.c_str();
  size_t sourceLen = strlen(source);
  size_t copyLen = sourceLen;
  if (copyLen > sizeof(preview) - 1) {
    copyLen = sizeof(preview) - 1;
  }

  memset(preview, 0, sizeof(preview));
  memcpy(preview, source, copyLen);
  preview[copyLen] = '\0';

  tft.fillRoundRect(8, kbY - 34, 224, 28, 6, COLOR_PANEL);
  tft.drawRoundRect(8, kbY - 34, 224, 28, 6, COLOR_ACCENT);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  tft.drawString(getKeyboardTargetLabel(keyboardTarget), 14, kbY - 30, 2);
  tft.setTextColor(COLOR_ACCENT_HI, COLOR_PANEL);
  tft.drawRightString((copyLen > 0) ? preview : "<empty>", 224, kbY - 30, 2);
}

// ==== KEYBOARD DYNAMIC ====
void drawKeyboardDynamic() {
  drawKeyboardPreview();

  for (int r = 0; r < KB_ROWS; r++) {
    for (int c = 0; c < KB_COLS; c++) {

      const char* key = getKeyboardKey(r, c);
      if (strlen(key) == 0) continue;

      int kx = kbX + c * (keyW + keySpacing) + 5;
      int ky = kbY + r * (keyH + keySpacing) + 5;

      int kw = keyW;

      // ===== SPECIAL KEYS =====
      if (strcmp(key, " ") == 0) {
        kx = kbX + 10;
        kw = kbW - 20;
      }
      else if (strcmp(key, "OK") == 0) {
        kw = keyW * 2;
      }

      // ===== KEY BASE =====
      tft.fillRoundRect(kx, ky, kw, keyH, 4, COLOR_PANEL);

      // ===== CURSOR HIGHLIGHT =====
      if (r == kbCursorRow && c == kbCursorCol) {
        tft.drawRoundRect(kx - 2, ky - 2, kw + 4, keyH + 4, 4, COLOR_ACCENT);
      }

      // ===== PRESSED EFFECT =====
      if (r == kbPressedRow && c == kbPressedCol) {
        tft.fillRoundRect(kx, ky, kw, keyH, 4, COLOR_ACCENT);
        tft.setTextColor(COLOR_BG);
      } else {
        tft.setTextColor(COLOR_TEXT);
      }

      // ===== TEXT =====
      tft.drawCentreString(key, kx + kw / 2, ky + 8, 2);
    }
  }

  // reset pressed state AFTER drawing
  kbPressedRow = -1;
  kbPressedCol = -1;
}

void handleKeyboardSelect() {

  kbPressedRow = kbCursorRow;
  kbPressedCol = kbCursorCol;

  const char* key = getKeyboardKey(kbCursorRow, kbCursorCol);
  processKeyboardKey(key);
}

// ==== MODEL NAME STATIC ====
void drawModelNameStatic() {

  tft.fillScreen(COLOR_BG);

  // ===== BACK =====
  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK", false, (dpadFocusVisible && focusIndex == 0), BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  // ===== LABEL =====
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 45);
  tft.print("Model Name:");

  // ===== INPUT BOX =====
  tft.drawRoundRect(10, 65, 220, 30, 6, COLOR_ACCENT);
  if (dpadFocusVisible && focusIndex == 1) {
    tft.drawRoundRect(8, 63, 224, 34, 6, COLOR_ACCENT_HI);
  }

  // ===== DIVIDER =====
  tft.drawFastHLine(10, 105, 220, COLOR_ACCENT);

  // ===== LIST LABEL =====
  tft.setCursor(10, 115);
  tft.print("Saved:");
}

// ==== MODEL NAME DYNAMIC ====
void drawModelNameDynamic() {

  if (!modelNameDirty && !modelNameNeedsRedraw) return;

  // ===== INPUT TEXT =====
  tft.fillRect(12, 67, 216, 26, COLOR_BG);

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(15, 72);
  tft.print(keyboardBuffer);

  // ===== MODEL LIST =====
  for (int i = 0; i < 4; i++) {

    int y = 130 + (i * 25);

    int numberX = 5;
    int nameX   = 35;
    int boxX    = 30;

    // ===== CLEAR ROW =====
    tft.fillRect(0, y - 2, 240, 22, COLOR_BG);

    bool isActive = (i == activeModel);

    // ===== HIGHLIGHT BOX =====
    if (isActive) {
      tft.fillRoundRect(boxX, y - 2, 150, 20, 5, COLOR_ACCENT);
    }
    if (dpadFocusVisible && focusIndex == i + 2) {
      tft.drawRoundRect(boxX - 2, y - 4, 154, 24, 5, COLOR_ACCENT_HI);
    }

    // ===== SELECTOR ARROW =====
    if (isActive) {
      tft.setTextColor(COLOR_ACCENT);
      tft.setCursor(0, y);
      tft.print(">");
    }

    // ===== INDEX NUMBER =====
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(numberX, y);
    tft.print("[");
    tft.print(i + 1);
    tft.print("]");

    // ===== MODEL NAME =====
    tft.setTextColor(isActive ? COLOR_BG : COLOR_TEXT);
    tft.setCursor(nameX, y);
    tft.print(modelNames[i]);

    // ===== DELETE BUTTON =====
    tft.fillRoundRect(200, y - 2, 25, 18, 4, COLOR_PANEL);
    tft.drawRoundRect(200, y - 2, 25, 18, 4, COLOR_ACCENT);
    if (dpadFocusVisible && focusIndex == i + 6) {
      tft.drawRoundRect(198, y - 4, 29, 22, 4, COLOR_ACCENT_HI);
    }

    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(208, y);
    tft.print("-");
  }

  modelNameDirty = false;
  modelNameNeedsRedraw = false;
}

void drawMixNumpad() {
  const char* mixPadLayout[4][3] = {
    {"1", "2", "3"},
    {"4", "5", "6"},
    {"7", "8", "9"},
    {"<", "0", "-"}
  };

  tft.fillRoundRect(MIX_NUMPAD_X, MIX_NUMPAD_Y, MIX_NUMPAD_W, MIX_NUMPAD_H, 10, COLOR_BG);
  tft.drawRoundRect(MIX_NUMPAD_X, MIX_NUMPAD_Y, MIX_NUMPAD_W, MIX_NUMPAD_H, 10, COLOR_ACCENT);
  tft.drawFastHLine(MIX_NUMPAD_X + 8, MIX_NUMPAD_Y + 30, MIX_NUMPAD_W - 16, TFT_DARKGREY);

  tft.setTextColor(COLOR_TEXT);
  const char* title = "Value";
  if (mixNumpadTarget == NUMPAD_TARGET_MIX_RATE) title = "Rate";
  else if (mixNumpadTarget == NUMPAD_TARGET_MIX_OFFSET) title = "Offset";
  else if (mixNumpadTarget == NUMPAD_TARGET_EXPO_VALUE) title = "Expo";
  else if (mixNumpadTarget == NUMPAD_TARGET_ENDPOINT_VALUE) title = "End Point";
  tft.drawString(title, MIX_NUMPAD_X + 10, MIX_NUMPAD_Y + 8, 2);

  tft.fillRoundRect(MIX_NUMPAD_BOX_X, MIX_NUMPAD_BOX_Y, MIX_NUMPAD_BOX_W, MIX_NUMPAD_BOX_H, 6, COLOR_PANEL);
  tft.drawRoundRect(MIX_NUMPAD_BOX_X, MIX_NUMPAD_BOX_Y, MIX_NUMPAD_BOX_W, MIX_NUMPAD_BOX_H, 6, COLOR_ACCENT);
  tft.setTextColor(COLOR_ACCENT_HI);
  tft.drawRightString(mixNumpadBuffer, MIX_NUMPAD_BOX_X + MIX_NUMPAD_BOX_W - 8, MIX_NUMPAD_BOX_Y + 5, 2);

  tft.fillRoundRect(MIX_NUMPAD_OK_X, MIX_NUMPAD_OK_Y, MIX_NUMPAD_OK_W, MIX_NUMPAD_OK_H, 6, COLOR_ACCENT);
  tft.drawRoundRect(MIX_NUMPAD_OK_X, MIX_NUMPAD_OK_Y, MIX_NUMPAD_OK_W, MIX_NUMPAD_OK_H, 6, COLOR_ACCENT_HI);
  if (mixNumpadCursorRow == 0 && mixNumpadCursorCol == 3) {
    tft.drawRoundRect(MIX_NUMPAD_OK_X - 2, MIX_NUMPAD_OK_Y - 2, MIX_NUMPAD_OK_W + 4, MIX_NUMPAD_OK_H + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawCentreString("OK", MIX_NUMPAD_OK_X + (MIX_NUMPAD_OK_W / 2), MIX_NUMPAD_OK_Y + 5, 2);

  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 3; c++) {
      int keyX = MIX_NUMPAD_GRID_X + (c * (MIX_NUMPAD_KEY_W + MIX_NUMPAD_KEY_GAP));
      int keyY = MIX_NUMPAD_GRID_Y + (r * (MIX_NUMPAD_KEY_H + MIX_NUMPAD_KEY_GAP));

      tft.fillRoundRect(keyX, keyY, MIX_NUMPAD_KEY_W, MIX_NUMPAD_KEY_H, 6, COLOR_PANEL);
      tft.drawRoundRect(keyX, keyY, MIX_NUMPAD_KEY_W, MIX_NUMPAD_KEY_H, 6, COLOR_ACCENT);
      if (r == mixNumpadCursorRow && c == mixNumpadCursorCol) {
        tft.drawRoundRect(keyX - 2, keyY - 2, MIX_NUMPAD_KEY_W + 4, MIX_NUMPAD_KEY_H + 4, 6, COLOR_ACCENT_HI);
      }
      tft.setTextColor(COLOR_TEXT);
      tft.drawCentreString(mixPadLayout[r][c], keyX + (MIX_NUMPAD_KEY_W / 2), keyY + 5, 2);
    }
  }
}

// ==== MIXING STATIC ====
void drawMixingStatic() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK", false, (dpadFocusVisible && focusIndex == 0), BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);
}

// ==== MIXING DYNAMIC ====
void drawMixingDynamic() {
  if (!mixingNeedsRedraw) return;

  char valueText[32];
  MixData &mix = models[activeModel].mixes[selectedMixIndex];
  uint8_t source = getMixSource(mix);
  uint8_t destination = getMixDestination(mix);
  bool reverseLinked = isMixReverseLinked(mix);

  tft.fillRect(0, 38, 240, FOOTER_Y - 44, COLOR_BG);

  for (int i = 0; i < MIX_COUNT; i++) {
    bool isSelected = (i == selectedMixIndex);
    int tabX = MIX_TAB_X(i);

    tft.fillRoundRect(
      tabX, MIX_TAB_Y, MIX_TAB_W, MIX_TAB_H, 8,
      isSelected ? COLOR_ACCENT : COLOR_PANEL);
    tft.drawRoundRect(tabX, MIX_TAB_Y, MIX_TAB_W, MIX_TAB_H, 8, COLOR_ACCENT);
    if (dpadFocusVisible && focusIndex == i + 1) {
      tft.drawRoundRect(tabX - 2, MIX_TAB_Y - 2, MIX_TAB_W + 4, MIX_TAB_H + 4, 8, COLOR_ACCENT_HI);
    }

    tft.setTextColor(isSelected ? COLOR_BG : COLOR_TEXT);
    snprintf(valueText, sizeof(valueText), "M%d", i + 1);
    tft.drawCentreString(valueText, tabX + (MIX_TAB_W / 2), MIX_TAB_Y + 7, 2);
  }

  tft.setTextColor(COLOR_ACCENT_HI);
  if (mix.enabled) {
    snprintf(valueText, sizeof(valueText), "CH%d %s > CH%d %s",
             source + 1, getChannelAxisName(source),
             destination + 1, getChannelAxisName(destination));
  } else {
    snprintf(valueText, sizeof(valueText), "Mix %d Disabled", selectedMixIndex + 1);
  }
  tft.drawCentreString(valueText, 120, 72, 2);

  const char* labels[6] = {"Enable", "Source", "Dest", "Rev Link", "Rate", "Offset"};
  const int rowY[6] = {
    MIX_ROW_ENABLE_Y, MIX_ROW_SOURCE_Y, MIX_ROW_DEST_Y, MIX_ROW_LINK_Y, MIX_ROW_RATE_Y, MIX_ROW_OFFS_Y
  };

  tft.setTextColor(COLOR_TEXT);
  for (int i = 0; i < 6; i++) {
    tft.setCursor(20, rowY[i] + 8);
    tft.print(labels[i]);
  }

  // Enable toggle
  tft.fillRoundRect(MIX_FIELD_X + 2, MIX_ROW_ENABLE_Y + 2, MIX_FIELD_W - 4, MIX_FIELD_H - 4, 10, COLOR_PANEL);
  tft.drawRoundRect(MIX_FIELD_X, MIX_ROW_ENABLE_Y, MIX_FIELD_W, MIX_FIELD_H, 10, COLOR_ACCENT);
  if (dpadFocusVisible && focusIndex == 5) {
    tft.drawRoundRect(MIX_FIELD_X - 2, MIX_ROW_ENABLE_Y - 2, MIX_FIELD_W + 4, MIX_FIELD_H + 4, 10, COLOR_ACCENT_HI);
  }
  tft.setTextColor(COLOR_TEXT);
  tft.drawString("OFF", MIX_FIELD_X - 36, MIX_ROW_ENABLE_Y + 8, 2);
  tft.drawString("ON", MIX_FIELD_X + MIX_FIELD_W + 6, MIX_ROW_ENABLE_Y + 8, 2);

  int knobRadius = 9;
  int knobX = mix.enabled
    ? (MIX_FIELD_X + MIX_FIELD_W - knobRadius - 3)
    : (MIX_FIELD_X + knobRadius + 3);
  tft.fillCircle(knobX, MIX_ROW_ENABLE_Y + (MIX_FIELD_H / 2), knobRadius,
                 mix.enabled ? COLOR_ACCENT : COLOR_TEXT);

  snprintf(valueText, sizeof(valueText), "CH%d %s", source + 1, getChannelAxisName(source));
  tft.fillRoundRect(MIX_FIELD_X, MIX_ROW_SOURCE_Y, MIX_FIELD_W, MIX_FIELD_H, 8, COLOR_PANEL);
  tft.drawRoundRect(MIX_FIELD_X, MIX_ROW_SOURCE_Y, MIX_FIELD_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  if (dpadFocusVisible && focusIndex == 6) {
    tft.drawRoundRect(MIX_FIELD_X - 2, MIX_ROW_SOURCE_Y - 2, MIX_FIELD_W + 4, MIX_FIELD_H + 4, 8, COLOR_ACCENT_HI);
  }
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString(valueText, MIX_FIELD_X + (MIX_FIELD_W / 2), MIX_ROW_SOURCE_Y + 8, 2);

  snprintf(valueText, sizeof(valueText), "CH%d %s", destination + 1, getChannelAxisName(destination));
  tft.fillRoundRect(MIX_FIELD_X, MIX_ROW_DEST_Y, MIX_FIELD_W, MIX_FIELD_H, 8, COLOR_PANEL);
  tft.drawRoundRect(MIX_FIELD_X, MIX_ROW_DEST_Y, MIX_FIELD_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  if (dpadFocusVisible && focusIndex == 7) {
    tft.drawRoundRect(MIX_FIELD_X - 2, MIX_ROW_DEST_Y - 2, MIX_FIELD_W + 4, MIX_FIELD_H + 4, 8, COLOR_ACCENT_HI);
  }
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString(valueText, MIX_FIELD_X + (MIX_FIELD_W / 2), MIX_ROW_DEST_Y + 8, 2);

  tft.fillRoundRect(MIX_FIELD_X + 2, MIX_ROW_LINK_Y + 2, MIX_FIELD_W - 4, MIX_FIELD_H - 4, 10, COLOR_PANEL);
  tft.drawRoundRect(MIX_FIELD_X, MIX_ROW_LINK_Y, MIX_FIELD_W, MIX_FIELD_H, 10, COLOR_ACCENT);
  if (dpadFocusVisible && focusIndex == 8) {
    tft.drawRoundRect(MIX_FIELD_X - 2, MIX_ROW_LINK_Y - 2, MIX_FIELD_W + 4, MIX_FIELD_H + 4, 10, COLOR_ACCENT_HI);
  }
  tft.setTextColor(COLOR_TEXT);
  tft.drawString("SEP", MIX_FIELD_X - 36, MIX_ROW_LINK_Y + 8, 2);
  tft.drawString("LINK", MIX_FIELD_X + MIX_FIELD_W + 4, MIX_ROW_LINK_Y + 8, 2);

  knobX = reverseLinked
    ? (MIX_FIELD_X + MIX_FIELD_W - knobRadius - 3)
    : (MIX_FIELD_X + knobRadius + 3);
  tft.fillCircle(knobX, MIX_ROW_LINK_Y + (MIX_FIELD_H / 2), knobRadius,
                 reverseLinked ? COLOR_ACCENT : COLOR_TEXT);

  tft.fillRoundRect(MIX_MINUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.drawRoundRect(MIX_MINUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT_HI);
  if (dpadFocusVisible && focusIndex == 9) {
    tft.drawRoundRect(MIX_MINUS_X - 2, MIX_ROW_RATE_Y - 2, MIX_ADJUST_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawCentreString("-", MIX_MINUS_X + (MIX_ADJUST_W / 2), MIX_ROW_RATE_Y + 8, 2);

  tft.fillRoundRect(MIX_VALUE_X, MIX_ROW_RATE_Y, MIX_VALUE_W, MIX_FIELD_H, 8, COLOR_PANEL);
  tft.drawRoundRect(MIX_VALUE_X, MIX_ROW_RATE_Y, MIX_VALUE_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.setTextColor(COLOR_TEXT);
  snprintf(valueText, sizeof(valueText), "%d", mix.rate);
  tft.drawCentreString(valueText, MIX_VALUE_X + (MIX_VALUE_W / 2), MIX_ROW_RATE_Y + 8, 2);
  if (mixNumpadActive && mixNumpadEditingRate) {
    tft.drawRoundRect(MIX_VALUE_X - 2, MIX_ROW_RATE_Y - 2, MIX_VALUE_W + 4, MIX_FIELD_H + 4, 8, COLOR_ACCENT_HI);
  }
  else if (dpadFocusVisible && focusIndex == 10) {
    tft.drawRoundRect(MIX_VALUE_X - 2, MIX_ROW_RATE_Y - 2, MIX_VALUE_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }

  tft.fillRoundRect(MIX_PLUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.drawRoundRect(MIX_PLUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT_HI);
  if (dpadFocusVisible && focusIndex == 11) {
    tft.drawRoundRect(MIX_PLUS_X - 2, MIX_ROW_RATE_Y - 2, MIX_ADJUST_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawCentreString("+", MIX_PLUS_X + (MIX_ADJUST_W / 2), MIX_ROW_RATE_Y + 8, 2);

  tft.fillRoundRect(MIX_MINUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.drawRoundRect(MIX_MINUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT_HI);
  if (dpadFocusVisible && focusIndex == 12) {
    tft.drawRoundRect(MIX_MINUS_X - 2, MIX_ROW_OFFS_Y - 2, MIX_ADJUST_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawCentreString("-", MIX_MINUS_X + (MIX_ADJUST_W / 2), MIX_ROW_OFFS_Y + 8, 2);

  tft.fillRoundRect(MIX_VALUE_X, MIX_ROW_OFFS_Y, MIX_VALUE_W, MIX_FIELD_H, 8, COLOR_PANEL);
  tft.drawRoundRect(MIX_VALUE_X, MIX_ROW_OFFS_Y, MIX_VALUE_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.setTextColor(COLOR_TEXT);
  snprintf(valueText, sizeof(valueText), "%d", mix.offset);
  tft.drawCentreString(valueText, MIX_VALUE_X + (MIX_VALUE_W / 2), MIX_ROW_OFFS_Y + 8, 2);
  if (mixNumpadActive && !mixNumpadEditingRate) {
    tft.drawRoundRect(MIX_VALUE_X - 2, MIX_ROW_OFFS_Y - 2, MIX_VALUE_W + 4, MIX_FIELD_H + 4, 8, COLOR_ACCENT_HI);
  }
  else if (dpadFocusVisible && focusIndex == 13) {
    tft.drawRoundRect(MIX_VALUE_X - 2, MIX_ROW_OFFS_Y - 2, MIX_VALUE_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }

  tft.fillRoundRect(MIX_PLUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.drawRoundRect(MIX_PLUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT_HI);
  if (dpadFocusVisible && focusIndex == 14) {
    tft.drawRoundRect(MIX_PLUS_X - 2, MIX_ROW_OFFS_Y - 2, MIX_ADJUST_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawCentreString("+", MIX_PLUS_X + (MIX_ADJUST_W / 2), MIX_ROW_OFFS_Y + 8, 2);

  mixingNeedsRedraw = false;
}

uint16_t tintRgb565Pixel(uint16_t srcColor, uint16_t tintColor) {
  uint8_t srcR = (uint8_t)((srcColor >> 11) & 0x1F);
  uint8_t srcG = (uint8_t)((srcColor >> 5) & 0x3F);
  uint8_t srcB = (uint8_t)(srcColor & 0x1F);

  uint16_t srcR8 = (uint16_t)((srcR * 255U) / 31U);
  uint16_t srcG8 = (uint16_t)((srcG * 255U) / 63U);
  uint16_t srcB8 = (uint16_t)((srcB * 255U) / 31U);

  uint16_t luminance = (uint16_t)((srcR8 * 2126U + srcG8 * 7152U + srcB8 * 722U) / 10000U);

  uint8_t tintR = (uint8_t)((tintColor >> 11) & 0x1F);
  uint8_t tintG = (uint8_t)((tintColor >> 5) & 0x3F);
  uint8_t tintB = (uint8_t)(tintColor & 0x1F);

  uint8_t outR = (uint8_t)((tintR * luminance) / 255U);
  uint8_t outG = (uint8_t)((tintG * luminance) / 255U);
  uint8_t outB = (uint8_t)((tintB * luminance) / 255U);

  return (uint16_t)((outR << 11) | (outG << 5) | outB);
}

void drawRgb565IconTinted(int x, int y, int w, int h, const uint16_t* iconData, int iconW, int iconH, uint16_t tintColor) {
  if (iconData == nullptr || iconW <= 0 || iconH <= 0 || w <= 0 || h <= 0) {
    return;
  }

  for (int dy = 0; dy < h; ++dy) {
    int sy = (dy * iconH) / h;
    int srcRow = sy * iconW;
    int py = y + dy;

    for (int dx = 0; dx < w; ++dx) {
      int sx = (dx * iconW) / w;
      uint16_t srcColor = iconData[srcRow + sx];

      if (srcColor == ICON_TRANSPARENT_RGB565) {
        continue;
      }

      tft.drawPixel(x + dx, py, tintRgb565Pixel(srcColor, tintColor));
    }
  }
}

// ==== CONTROLLER ICON ====
void drawControllerIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 24 * s;
  drawRgb565IconTinted(x, y, size, size, icon_CONTROLLER, ICON_CONTROLLER_W, ICON_CONTROLLER_H, iconColor);
}

// ==== BACK ICON ====
void drawBackIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 18 * s;
  drawRgb565IconTinted(x, y, size, size, icon_BACK, ICON_BACK_W, ICON_BACK_H, iconColor);
}

// ==== MODEL ICON ====
void drawCarIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 24 * s;
  drawRgb565IconTinted(x, y, size, size, icon_CAR, ICON_CAR_W, ICON_CAR_H, iconColor);
}

// ==== HOME ICON ====
void drawHomeIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 18 * s;
  drawRgb565IconTinted(x, y, size, size, icon_HOME, ICON_HOME_W, ICON_HOME_H, iconColor);
}
// ==== REVERSE ICON ====
void drawReverseIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 24 * s;
  drawRgb565IconTinted(x, y, size, size, icon_REVERSE, ICON_REVERSE_W, ICON_REVERSE_H, iconColor);
}

// ==== TRIM ICON ====
void drawTrimIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 24 * s;
  drawRgb565IconTinted(x, y, size, size, icon_TRIM, ICON_TRIM_W, ICON_TRIM_H, iconColor);
}

// ==== FAILSAFE ICON ====
void drawFailsafeIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 24 * s;
  drawRgb565IconTinted(x, y, size, size, icon_FAILSAFE, ICON_FAILSAFE_W, ICON_FAILSAFE_H, iconColor);
}

// ==== ENDPOINT ICON ====
void drawEndpointIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 24 * s;
  drawRgb565IconTinted(x, y, size, size, icon_ENDPOINT, ICON_ENDPOINT_W, ICON_ENDPOINT_H, iconColor);
}

void drawExpoIcon(int x, int y, int s, uint16_t iconColor) {
  int left = x + 2*s;
  int right = x + 22*s;
  int top = y + 2*s;
  int bottom = y + 22*s;
  int midX = (left + right) / 2;
  int midY = (top + bottom) / 2;

  drawThickLine(left, bottom, right, bottom, iconColor);
  drawThickLine(left, top, left, bottom, iconColor);

  tft.drawLine(midX, top + 2*s, midX, bottom - 1, iconColor);
  tft.drawLine(left + 2*s, midY, right - 1, midY, iconColor);

  drawThickLine(left + 3*s, bottom - 3*s, left + 8*s, bottom - 8*s, iconColor);
  drawThickLine(left + 8*s, bottom - 8*s, left + 13*s, bottom - 11*s, iconColor);
  drawThickLine(left + 13*s, bottom - 11*s, left + 18*s, top + 5*s, iconColor);
}

void drawRatesIcon(int x, int y, int s, uint16_t iconColor) {
  int baseSize = 24 * s;
  int size = (baseSize * 9) / 10;
  int offset = (baseSize - size) / 2;
  drawRgb565IconTinted(x + offset, y + offset, size, size, icon_RATES, ICON_RATES_W, ICON_RATES_H, iconColor);
}

void drawProtocolIcon(int x, int y, int s, uint16_t iconColor) {
  int baseSize = 24 * s;
  int size = (baseSize * 9) / 10;
  int offset = (baseSize - size) / 2;
  drawRgb565IconTinted(x + offset, y + offset, size, size, icon_PROTOCOL, ICON_PROTOCOL_W, ICON_PROTOCOL_H, iconColor);
}

// ==== MODEL NAME ICON ====
void drawModelNameIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 24 * s;
  drawRgb565IconTinted(x, y, size, size, icon_MODEL_NAME, ICON_MODEL_NAME_W, ICON_MODEL_NAME_H, iconColor);
}

// ==== DRIVE TYPE ICON ====
void drawDriveTypeIcon(int x, int y, int s, uint16_t iconColor) {

  int size = 24 * s;

  drawTankIcon(x, y, size, size, iconColor);
}

// ==== MIXING ICON ====
void drawMixingIcon(int x, int y, int s, uint16_t iconColor) {
  int size = 24 * s;
  drawRgb565IconTinted(x, y, size, size, icon_MIXING, ICON_MIXING_W, ICON_MIXING_H, iconColor);
}

