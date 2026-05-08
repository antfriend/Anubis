#include <RAK14014_FT6336U.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <EEPROM.h>

#include "anubis_rgb565.h"

  #define EEPROM_SIZE 512
  #define MIX_STORAGE_VERSION 2
  #define EEPROM_MIX_VERSION_ADDR (EEPROM_SIZE - 1)

  TFT_eSPI tft = TFT_eSPI();
  TFT_eSprite stickBaseSprite = TFT_eSprite(&tft);
  FT6336U touchPanel;

  enum Screen {
  SCREEN_SPLASH,
  SCREEN_MAIN,
  SCREEN_MENU,
  SCREEN_CONTROLLER_SETTINGS,
  SCREEN_MODEL_SETTINGS,
  SCREEN_REVERSE,
  SCREEN_TRIM,
  SCREEN_FAILSAFE,
  SCREEN_MODEL_NAME,
  SCREEN_DRIVE_TYPE,
  SCREEN_TANK_MODE,
  SCREEN_MIXING
  };

  enum ButtonID {
  BTN_NONE,
  BTN_MENU,
  BTN_BACK,
  BTN_CTRL,
  BTN_MODEL,
  BTN_REVERSE,
  BTN_TRIM,
  BTN_FAILSAFE,
  BTN_DRIVE_TYPE,
  BTN_DRIVE_TANK,
  BTN_DRIVE_CAR,
  BTN_DRIVE_OMNI,
  BTN_DRIVE_X_DRONE,
  BTN_TANK_SINGLE,
  BTN_TANK_DUAL,
  BTN_MODEL_NAME,
  BTN_MIXING,
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

  #define MAX_MODELS 4
  #define MIX_COUNT 4

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

  ModelData models[MAX_MODELS];
  int activeModel = 0;

  String currentModelName = "No Model";

  int espNowLatency = 0;
  float batteryVoltage = 0.0;

  int focusIndex = 0;
  unsigned long lastDpadTime = 0;

  DriveType currentDrive = DRIVE_TANK;
  Screen currentScreen = SCREEN_SPLASH;
  ButtonID pressedButton = BTN_NONE;
  ButtonID selectedButton = BTN_NONE;
  bool dpadFocusVisible = false;

  bool uiNeedsRedraw = true;
  bool touchActive = false;
  bool waitingForRelease = false;
  bool screenChangePending = false;
  bool screenAwake = true;
  bool fullRedraw = true;
  Screen nextScreen;
  Screen lastScreen = SCREEN_SPLASH;

  unsigned long lastActivityTime = 0;
  unsigned long splashStartTime = 0;
  unsigned long fpsLastTime = 0;
  int frameCount = 0;
  bool splashDone = false;

  #define OFF_TIMEOUT   25000

  #define BRIGHTNESS_ON   255
  #define BRIGHTNESS_DIM   60
  #define BRIGHTNESS_OFF    0

  #define TARGET_FPS 24
  #define MAIN_FRAME_INTERVAL_MS (1000UL / TARGET_FPS)
  #define TOUCH_POLL_INTERVAL_MS 16
  #define TOPBAR_UPDATE_INTERVAL_MS 250
  #define MODEL_NAME_HOLD_MS 700

  // ===== UI LAYOUT =====
  #define BTN_HEIGHT 50
  #define BTN_RADIUS 12

  // Menu button
  #define MENU_BTN_X 50
  #define MENU_BTN_Y 260
  #define MENU_BTN_W 140
  #define MENU_BTN_H BTN_HEIGHT

  // Back button
  #define BACK_BTN_X 10
  #define BACK_BTN_Y (FOOTER_Y + 5)
  #define BACK_BTN_W 100
  #define BACK_BTN_H BTN_HEIGHT

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

  #define BACK_TEXT_OFFSET 50

  #define TRIM_CENTER_X 120
  #define TRIM_CENTER_Y 140
  #define TRIM_SIZE     80
  #define TRIM_BTN_SIZE 28

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

  #define MIX_FIELD_X      128
  #define MIX_FIELD_W      92
  #define MIX_FIELD_H      28
  #define MIX_MINUS_X      128
  #define MIX_PLUS_X       194
  #define MIX_ADJUST_W     26
  #define MIX_VALUE_X      158
  #define MIX_VALUE_W      32
  #define MIX_ROW_ENABLE_Y 96
  #define MIX_ROW_SOURCE_Y 128
  #define MIX_ROW_DEST_Y   160
  #define MIX_ROW_RATE_Y   192
  #define MIX_ROW_OFFS_Y   224

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

  // ==== SPLASH SCREEN ====
  #define ANUBIS_WIDTH 160
  #define ANUBIS_HEIGHT 160

  // ==== BOTTOM NAV BAR ====
  #define FOOTER_Y 260
  #define FOOTER_H 60

  #define MAX_MODELS 4

  int modelPanelX = 0;
  int modelPanelY = 0;
  int modelPanelW = 0;
  int modelPanelH = 0;

  const uint16_t COLOR_BG        = tft.color565(0, 0, 0);
  const uint16_t COLOR_PANEL     = tft.color565(30, 30, 30);
  const uint16_t COLOR_TEXT      = tft.color565(255, 255, 255);

  const uint16_t COLOR_ACCENT    = tft.color565(180, 140, 40);
  const uint16_t COLOR_ACCENT_HI = tft.color565(255, 210, 80);

  const uint16_t COLOR_SIG       = tft.color565(49, 146, 49);
  const uint16_t COLOR_STICK_PANEL = tft.color565(20, 20, 20);

  int currentTrimPage = 0; // 0 = left gimbal, 1 = right gimbal

  bool trimNeedsRedraw = true;
  bool trimDirty = true;

  static int lastSignal = -1;
  static int lastBattery = -1;

  bool reverseNeedsRedraw = true;
  bool reverseChannelDirty[5] = { true, true, true, true, true };

  bool failsafeNeedsRedraw = true;
  bool failsafeDirty[5] = { true, true, true, true, true };
  bool mixingNeedsRedraw = true;
  int selectedMixIndex = 0;
  bool mixNumpadActive = false;
  bool mixNumpadNeedsRedraw = false;
  bool mixNumpadEditingRate = true;
  String mixNumpadBuffer = "";
  int mixNumpadCursorRow = 0;
  int mixNumpadCursorCol = 0;

  // placeholder values for now (you’ll replace with real captured values later)
  int failsafeValue[5] = { 0, 0, 0, 0, 0 };

  bool isInside(int x, int y, int bx, int by, int bw, int bh);
  int mapTouch(int val, int in_min, int in_max, int out_min, int out_max);
  void setScreen(Screen screen);
  void queueScreenButton(ButtonID button, Screen screen);
  void saveKeyboardBufferToModelSlot();
  const char* getKeyboardKey(int row, int col);
  void processKeyboardKey(const char* key);
  bool modelSlotUninitialized(int i);
  void initDefaultMixSlot(int modelIndex, int mixIndex);
  bool sanitizeModelMixes(int modelIndex);
  bool sanitizeModelDriveType(int modelIndex);
  void openMixNumpad(bool editRate);
  void closeMixNumpad(bool commitValue);
  bool handleMixNumpadTouch(int x, int y);
  void drawMixNumpad();
  void drawDriveTypeScreen();
  void drawTankModeScreen();
  void drawDriveTypeOption(int x, int y, int w, int h, const char* label, DriveType drive, ButtonID button);
  void drawDriveTypeOptionIcon(int x, int y, int w, int h, DriveType drive, uint16_t iconColor);
  void drawOmniIcon(int x, int y, int w, int h, uint16_t iconColor);
  void getLinkedReverseChannels(int modelIndex, int channel, bool linked[5]);
  bool hasLinkedReversePeers(int modelIndex, int channel);
  void drawTrimGraphBase();
  void drawTrimButtons();
  void drawMixingStatic();
  void drawMixingDynamic();
  void drawSplash();
  DriveType getModelDriveType(int modelIndex);
  void setModelDriveType(int modelIndex, DriveType driveType);
  TankControlMode getModelTankMode(int modelIndex);
  void setModelTankMode(int modelIndex, TankControlMode tankMode);

  void initDefaultMixSlot(int modelIndex, int mixIndex) {
    models[modelIndex].mixes[mixIndex].enabled = false;
    models[modelIndex].mixes[mixIndex].source = mixIndex % 5;
    models[modelIndex].mixes[mixIndex].destination = (mixIndex + 1) % 5;
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

  void getLinkedReverseChannels(int modelIndex, int channel, bool linked[5]) {
    for (int i = 0; i < 5; i++) linked[i] = false;
    if (channel < 0 || channel >= 5) return;

    linked[channel] = true;

    bool changed = true;
    while (changed) {
      changed = false;

      for (int mixIndex = 0; mixIndex < MIX_COUNT; mixIndex++) {
        MixData &mix = models[modelIndex].mixes[mixIndex];
        if (!mix.enabled) continue;
        if (mix.source >= 5 || mix.destination >= 5) continue;

        if (linked[mix.source] && !linked[mix.destination]) {
          linked[mix.destination] = true;
          changed = true;
        }

        if (linked[mix.destination] && !linked[mix.source]) {
          linked[mix.source] = true;
          changed = true;
        }
      }
    }
  }

  bool hasLinkedReversePeers(int modelIndex, int channel) {
    bool linked[5];
    getLinkedReverseChannels(modelIndex, channel, linked);

    for (int i = 0; i < 5; i++) {
      if (i != channel && linked[i]) return true;
    }

    return false;
  }

  bool sanitizeModelMixes(int modelIndex) {
    bool changed = false;

    for (int mixIndex = 0; mixIndex < MIX_COUNT; mixIndex++) {
      MixData &mix = models[modelIndex].mixes[mixIndex];

      bool invalid =
        (mix.source > 4) ||
        (mix.destination > 4) ||
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

      if (mix.destination == mix.source) {
        mix.destination = (mix.destination + 1) % 5;
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

  void initModelDefaults(int i) {
    for (int ch = 0; ch < 5; ch++) {
      models[i].reverse[ch] = false;
      models[i].failsafe[ch] = false;
    }

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

const char* getKeyboardKey(int row, int col) {
  if (row < 0 || row >= KB_ROWS || col < 0 || col >= KB_COLS) return "";
  return keyboardLowercase ? keyboardLayoutLower[row][col] : keyboardLayoutUpper[row][col];
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
    if (keyboardBuffer.length() > 0) {
      saveKeyboardBufferToModelSlot();
    }

    keyboardBuffer = "";
    modelNameDirty = true;
    modelNameNeedsRedraw = true;
    uiNeedsRedraw = true;
    closeKeyboard();
  }
  else {
    if (keyboardBuffer.length() < 18) {
      keyboardBuffer += key;
      modelNameDirty = true;
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

  for (int c = 0; c < 5; c++) {
    reverseChannelDirty[c] = true;
    failsafeDirty[c] = true;
  }

  uiNeedsRedraw = true;
}

void beginModelNameEdit(int modelIndex) {
  selectedModelIndex = modelIndex;
  keyboardBuffer = modelNames[modelIndex];
  keyboardActive = true;
  keyboardLowercase = false;
  keyboardNeedsRedraw = true;
  inputBoxSelected = true;
  modelNameDirty = true;
  modelNameNeedsRedraw = true;
  uiNeedsRedraw = true;
}

void setup() {
  Serial.begin(115200);

  SPI.begin(12, 13, 11, 10);

  pinMode(45, OUTPUT);
  digitalWrite(45, HIGH);

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);

  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(true);
    
  Wire.begin(16, 15);
  touchPanel.begin();
  lastActivityTime = millis();
  splashStartTime = millis();

  EEPROM.begin(EEPROM_SIZE);

  loadModels();

  // FIRST BOOT CHECK
  bool initializedAnyModel = false;
  bool repairedAnyMix = false;
  bool resetMixSchema = (EEPROM.read(EEPROM_MIX_VERSION_ADDR) != MIX_STORAGE_VERSION);

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
  }

  if (strlen(models[0].name) == 0) {
    strncpy(models[0].name, "Anubis", sizeof(models[0].name) - 1);
    models[0].name[sizeof(models[0].name) - 1] = '\0';
    initializedAnyModel = true;
  }

  if (initializedAnyModel || repairedAnyMix) {
    saveModels();
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

  leftThrottle  = sin(now / 500.0);
  rightThrottle = cos(now / 500.0);
  leftY  = cos(now / 500.0);
  rightY = sin(now / 500.0);

  m1 = (sin(now / 400.0) + 1) / 2;
  m2 = (cos(now / 400.0) + 1) / 2;
  m3 = (sin(now / 600.0) + 1) / 2;
  m4 = (cos(now / 600.0) + 1) / 2;

  static float lastLX = 0;
  static float lastRX = 0;
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
      now - lastMainFrameTime >= MAIN_FRAME_INTERVAL_MS &&
      (abs(leftThrottle - lastLX) > 0.005 ||
       abs(rightThrottle - lastRX) > 0.005)) {

    uiNeedsRedraw = true;

    lastLX = leftThrottle;
    lastRX = rightThrottle;
  }

  if (currentScreen != SCREEN_SPLASH &&
      currentScreen != SCREEN_MAIN &&
      now - lastTopBarFrameTime >= TOPBAR_UPDATE_INTERVAL_MS) {
    uiNeedsRedraw = true;
    lastTopBarFrameTime = now;
  }

  if (currentScreen == SCREEN_SPLASH) {
    drawSplash();
    fullRedraw = true;
    return;
  }

  if (millis() - lastDpadTime > 150) {

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

          if (strcmp(key, "<") == 0) {
            if (mixNumpadBuffer.length() > 0) {
              mixNumpadBuffer.remove(mixNumpadBuffer.length() - 1);
              if (mixNumpadBuffer == "-") mixNumpadBuffer = "";
            }
          }
          else if (strcmp(key, "-") == 0) {
            if (mixNumpadBuffer.startsWith("-")) mixNumpadBuffer.remove(0, 1);
            else mixNumpadBuffer = "-" + mixNumpadBuffer;
            if (mixNumpadBuffer.length() == 0) mixNumpadBuffer = "-";
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
            if (digitCount <= 3 && (candidate == "-" || (parsedValue >= -100 && parsedValue <= 100))) {
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
        else if (selectedButton == BTN_MODEL) selectedButton = BTN_BACK;
        else if (selectedButton == BTN_BACK) selectedButton = BTN_CTRL;
          fullRedraw = true;
          uiNeedsRedraw = true;
          didInput = true;
          userActive = true;
      }

    if (left) {
      setScreen(SCREEN_MAIN);
      selectedButton = BTN_NONE;
      didInput = true;
      userActive = true;
    }

    if (select) {

      if (selectedButton == BTN_BACK) setScreen(SCREEN_MAIN);
      else if (selectedButton == BTN_CTRL) setScreen(SCREEN_CONTROLLER_SETTINGS);
      else if (selectedButton == BTN_MODEL) setScreen(SCREEN_MODEL_SETTINGS);

      selectedButton = BTN_NONE;
      didInput = true;
      userActive = true;
    }
  }
  else if (currentScreen == SCREEN_CONTROLLER_SETTINGS) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (selectedButton == BTN_NONE) selectedButton = BTN_REVERSE;
      else if (selectedButton == BTN_REVERSE) selectedButton = BTN_TRIM;
      else if (selectedButton == BTN_TRIM) selectedButton = BTN_FAILSAFE;
      else if (selectedButton == BTN_FAILSAFE) selectedButton = BTN_BACK;
      else selectedButton = BTN_REVERSE;
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
      else if (selectedButton == BTN_REVERSE) setScreen(SCREEN_REVERSE);
      else if (selectedButton == BTN_TRIM) setScreen(SCREEN_TRIM);
      else if (selectedButton == BTN_FAILSAFE) setScreen(SCREEN_FAILSAFE);
      didInput = true;
    }
  }
  else if (currentScreen == SCREEN_MODEL_SETTINGS) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      if (selectedButton == BTN_NONE) selectedButton = BTN_MODEL_NAME;
      else if (selectedButton == BTN_MODEL_NAME) selectedButton = BTN_DRIVE_TYPE;
      else if (selectedButton == BTN_DRIVE_TYPE) selectedButton = BTN_MIXING;
      else if (selectedButton == BTN_MIXING) selectedButton = BTN_BACK;
      else selectedButton = BTN_MODEL_NAME;
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
      else if (selectedButton == BTN_MODEL_NAME) setScreen(SCREEN_MODEL_NAME);
      else if (selectedButton == BTN_DRIVE_TYPE) setScreen(SCREEN_DRIVE_TYPE);
      else if (selectedButton == BTN_MIXING) setScreen(SCREEN_MIXING);
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
          if (selectedButton == BTN_DRIVE_CAR) currentDrive = DRIVE_CAR;
          else if (selectedButton == BTN_DRIVE_OMNI) currentDrive = DRIVE_OMNI;
          else if (selectedButton == BTN_DRIVE_X_DRONE) currentDrive = DRIVE_X_DRONE;

          setModelDriveType(activeModel, currentDrive);
          saveModels();
          fullRedraw = true;
          uiNeedsRedraw = true;
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
      focusIndex = (focusIndex + 1) % 6;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left && focusIndex == 5) {
      setScreen(SCREEN_CONTROLLER_SETTINGS);
      didInput = true;
    }

    if (select || right) {
      if (focusIndex == 5) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
      } else {
        bool linked[5];
        getLinkedReverseChannels(activeModel, focusIndex, linked);
        bool newState = !models[activeModel].reverse[focusIndex];

        for (int ch = 0; ch < 5; ch++) {
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
      focusIndex = (focusIndex + 1) % 6;
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
  }
  else if (currentScreen == SCREEN_FAILSAFE) {
    if (down || left || right || select) dpadFocusVisible = true;

    if (down) {
      focusIndex = (focusIndex + 1) % 6;
      uiNeedsRedraw = true;
      didInput = true;
    }

    if (left && focusIndex == 5) {
      setScreen(SCREEN_CONTROLLER_SETTINGS);
      didInput = true;
    }

    if (select || right) {
      if (focusIndex == 5) {
        setScreen(SCREEN_CONTROLLER_SETTINGS);
      } else {
        models[activeModel].failsafe[focusIndex] = !models[activeModel].failsafe[focusIndex];
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
        }
        modelNames[3] = "";
        initModelDefaults(3);
        saveModels();
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
      focusIndex = (focusIndex + 1) % 14;
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }

    MixData &mix = models[activeModel].mixes[selectedMixIndex];

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
      if (left) mix.source = (mix.source + 4) % 5;
      else mix.source = (mix.source + 1) % 5;
      if (mix.source == mix.destination) mix.destination = (mix.destination + 1) % 5;
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 7 && (select || left || right)) {
      if (left) mix.destination = (mix.destination + 4) % 5;
      else mix.destination = (mix.destination + 1) % 5;
      if (mix.destination == mix.source) mix.destination = (mix.destination + 1) % 5;
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 8 && (select || left || right)) {
      mix.rate = constrain(mix.rate - 1, -100, 100);
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 9 && select) {
      openMixNumpad(true);
      didInput = true;
    }
    else if (focusIndex == 10 && (select || left || right)) {
      mix.rate = constrain(mix.rate + 1, -100, 100);
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 11 && (select || left || right)) {
      mix.offset = constrain(mix.offset - 1, -100, 100);
      saveModels();
      mixingNeedsRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
    }
    else if (focusIndex == 12 && select) {
      openMixNumpad(false);
      didInput = true;
    }
    else if (focusIndex == 13 && (select || left || right)) {
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
      lastActivityTime = millis();

    digitalWrite(45, HIGH);

    screenAwake = true;
    fullRedraw = true;
    uiNeedsRedraw = true;

    delay(100);
    return;
    }
  userActive = true;
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
      for (int i = 0; i < 5; i++) reverseChannelDirty[i] = true;
    }

    if (nextScreen == SCREEN_TRIM) {
      trimNeedsRedraw = true;
      trimDirty = true;
    }

    if (nextScreen == SCREEN_FAILSAFE) {
      failsafeNeedsRedraw = true;
      for (int i = 0; i < 5; i++) failsafeDirty[i] = true;
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

      float fps = frameCount / 2.0;

      Serial.print("FPS: ");
      Serial.println(fps);
      Serial.print("Frame time: ");
      Serial.print(1000.0 / fps);
      Serial.println(" ms");

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
        fullRedraw = false;
      }

    drawMainScreenDynamic();
    drawTopBarDynamic();
    lastMainFrameTime = millis();
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
        }
          
        drawTopBarStatic();
        fullRedraw = false;
        }

      if (currentScreen == SCREEN_REVERSE) {
        drawReverseDynamic();
      }
      if (currentScreen == SCREEN_TRIM) {
        drawTrimDynamic();
      }
      if (currentScreen == SCREEN_FAILSAFE) {
      drawFailsafeDynamic();
      }
      if (currentScreen == SCREEN_MODEL_NAME) {
      drawModelNameDynamic();
      }
      if (currentScreen == SCREEN_MIXING) {
      drawMixingDynamic();
      }
    drawTopBarDynamic();
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
      digitalWrite(45, HIGH);

      screenAwake = true;

      fullRedraw = true;
      uiNeedsRedraw = true;

      waitingForRelease = false;   // ✅ reset here
      touchActive = false;         // ✅ reset here

      delay(100);  // optional, but OK
      return;
    }

  handleTouch(x, y);
  touchActive = true;
  userActive = true;
  }
  else {
    if (touchActive) delay(30);
      touchActive = false;

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

  if (idleTime > OFF_TIMEOUT) {

    if (screenAwake) {
      digitalWrite(45, LOW);
      screenAwake = false;
    }
  }
  else {
    digitalWrite(45, HIGH);
      if (!screenAwake) {
  screenAwake = true;
  fullRedraw = true;
  uiNeedsRedraw = true;
}
  }  
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

// STATIC 3D EFFECT
void drawButtonBubble(int x, int y, int w, int h,
                      const char* label,
                      bool pressed,
                      bool selected,
                      int textOffset = 40)
{
  int iconScale = 2;   // default

  // ===== SCALE OVERRIDE =====
  if (strcmp(label, "BACK") == 0) {
    iconScale = 1;
  }

  int iconSize = 24 * iconScale;
  int iconX = x + 20;

  // optional: better alignment for BACK
  if (strcmp(label, "BACK") == 0) {
    iconX = x + 10;
  }

  int iconY = y + (h / 2) - (iconSize / 2);

  if (pressed) y += 2;

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

  int radius = BTN_RADIUS;

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
      y + i,
      w - (2 * xOffset),
      lineColor
    );
  }

  // ===== OUTLINE =====
  tft.drawRoundRect(x, y, w, h, radius, outlineColor);

  int inset = 5;

  // ===== HIGHLIGHT =====
  for (int i = 0; i < 6; i++) {
    float fade = 1.0 - (i * 0.15);
    uint16_t c = fadeColor(COLOR_ACCENT_HI, fade);

    tft.drawCircleHelper(
      x + radius,
      y + radius,
      radius - inset - i,
      1,
      c
    );
  }

  for (int i = 0; i < w - (2 * radius); i++) {
    float fade = 1.0 - ((float)i / (w - (2 * radius)));
    uint16_t c = fadeColor(COLOR_ACCENT_HI, fade);

    tft.drawPixel(x + radius + i, y + inset, c);
  }

  for (int i = 0; i < h - (2 * radius); i++) {
    float fade = 1.0 - ((float)i / (h - (2 * radius)));
    uint16_t c = fadeColor(COLOR_ACCENT_HI, fade);

    tft.drawPixel(x + inset, y + radius + i, c);
  }

  // ===== SHADOW =====
  tft.drawFastHLine(x + radius, y + h - 2, w - (2 * radius), TFT_DARKGREY);
  tft.drawFastVLine(x + w - 2, y + radius, h - (2 * radius), TFT_DARKGREY);

  uint16_t iconColor = selected ? COLOR_BG : COLOR_ACCENT;

  // ===== ICONS =====
  if (strcmp(label, "Controller") == 0) {
    drawControllerIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "Model") == 0) {
    drawCarIcon(iconX, iconY, iconScale, iconColor);
  }
  else if (strcmp(label, "BACK") == 0) {
    drawHomeIcon(iconX, iconY, iconScale, iconColor);
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
  int16_t ty = y + (h / 2) - 7;

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
    10, FOOTER_Y + 5,
    100, BTN_HEIGHT,
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
}

  void drawControllerSettings() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

    tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  drawButtonBubble(20, 50, 200, BTN_HEIGHT, "Reverse",
                   pressedButton == BTN_REVERSE, dpadFocusVisible && selectedButton == BTN_REVERSE, 100);
  drawButtonBubble(20, 120, 200, BTN_HEIGHT, "Trim",
                   pressedButton == BTN_TRIM, dpadFocusVisible && selectedButton == BTN_TRIM, 100);
  drawButtonBubble(20, 190, 200, BTN_HEIGHT, "Failsafe",
                   pressedButton == BTN_FAILSAFE, dpadFocusVisible && selectedButton == BTN_FAILSAFE, 100);
  }

  void drawModelSettings() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    dpadFocusVisible && selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

    tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  drawButtonBubble(20, 50, 200, BTN_HEIGHT, "Model Name",
                   pressedButton == BTN_MODEL_NAME, dpadFocusVisible && selectedButton == BTN_MODEL_NAME, 100);
  drawButtonBubble(20, 120, 200, BTN_HEIGHT, "Drive Type",
                   pressedButton == BTN_DRIVE_TYPE, dpadFocusVisible && selectedButton == BTN_DRIVE_TYPE, 100);
  drawButtonBubble(20, 190, 200, BTN_HEIGHT, "Mixing",
                   pressedButton == BTN_MIXING, dpadFocusVisible && selectedButton == BTN_MIXING, 100);
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
  tft.drawCentreString("Tank Control", 120, 38, 2);

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
  keyboardNeedsRedraw = true;

  // force full redraw of underlying screen
  fullRedraw = true;
  uiNeedsRedraw = true;
}

void openMixNumpad(bool editRate) {
  MixData &mix = models[activeModel].mixes[selectedMixIndex];

  mixNumpadEditingRate = editRate;
  mixNumpadBuffer = String(editRate ? mix.rate : mix.offset);
  mixNumpadActive = true;
  mixNumpadNeedsRedraw = true;
  mixNumpadCursorRow = 0;
  mixNumpadCursorCol = 0;
  mixingNeedsRedraw = true;
  uiNeedsRedraw = true;
}

void closeMixNumpad(bool commitValue) {
  if (!mixNumpadActive) return;

  if (commitValue) {
    int parsedValue = 0;

    if (mixNumpadBuffer.length() > 0 && mixNumpadBuffer != "-") {
      parsedValue = constrain(mixNumpadBuffer.toInt(), -100, 100);
    }

    MixData &mix = models[activeModel].mixes[selectedMixIndex];
    if (mixNumpadEditingRate) {
      mix.rate = parsedValue;
    } else {
      mix.offset = parsedValue;
    }

    saveModels();
    mixingNeedsRedraw = true;
  }

  mixNumpadActive = false;
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
  if (currentScreen == screen) return;

  currentScreen = screen;
  lastScreen = screen;
  dpadFocusVisible = false;
  selectedButton = BTN_NONE;
  focusIndex = 0;

  if (screen == SCREEN_MENU) {
    selectedButton = BTN_CTRL;
  }
  else if (screen == SCREEN_CONTROLLER_SETTINGS) {
    selectedButton = BTN_REVERSE;
  }
  else if (screen == SCREEN_MODEL_SETTINGS) {
    selectedButton = BTN_MODEL_NAME;
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

  fullRedraw = true;
  uiNeedsRedraw = true;
  screenChangePending = false;
}

void queueScreenButton(ButtonID button, Screen screen) {
  if (waitingForRelease) return;

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

    if (isInside(x, y, 10, FOOTER_Y + 5, 100, BTN_HEIGHT)) {
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
  }

  // ===== CONTROLLER SETTINGS =====
  else if (currentScreen == SCREEN_CONTROLLER_SETTINGS) {

    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      queueScreenButton(BTN_BACK, SCREEN_MENU);
      return;
    }

    else if (isInside(x, y, 20, 50, 200, BTN_HEIGHT)) {
      queueScreenButton(BTN_REVERSE, SCREEN_REVERSE);
      return;
    }

    else if (isInside(x, y, 20, 120, 200, BTN_HEIGHT)) {
      queueScreenButton(BTN_TRIM, SCREEN_TRIM);
      return;
    }

    else if (isInside(x, y, 20, 190, 200, BTN_HEIGHT)) {
      queueScreenButton(BTN_FAILSAFE, SCREEN_FAILSAFE);
      return;
    }
  }

  // ===== MODEL SETTINGS =====
  else if (currentScreen == SCREEN_MODEL_SETTINGS) {

  // ===== BACK =====
  if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
    queueScreenButton(BTN_BACK, SCREEN_MENU);
    return;
  }

  // ===== MODEL NAME =====
  else if (isInside(x, y, 20, 50, 200, BTN_HEIGHT)) {
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

    if (isInside(x, y, MIX_FIELD_X, MIX_ROW_ENABLE_Y, MIX_FIELD_W, MIX_FIELD_H)) {
      mix.enabled = !mix.enabled;
    }
    else if (isInside(x, y, MIX_FIELD_X, MIX_ROW_SOURCE_Y, MIX_FIELD_W, MIX_FIELD_H)) {
      mix.source = (mix.source + 1) % 5;
      if (mix.source == mix.destination) {
        mix.destination = (mix.destination + 1) % 5;
      }
    }
    else if (isInside(x, y, MIX_FIELD_X, MIX_ROW_DEST_Y, MIX_FIELD_W, MIX_FIELD_H)) {
      mix.destination = (mix.destination + 1) % 5;
      if (mix.destination == mix.source) {
        mix.destination = (mix.destination + 1) % 5;
      }
    }
    else if (isInside(x, y, MIX_VALUE_X, MIX_ROW_RATE_Y, MIX_VALUE_W, MIX_FIELD_H)) {
      openMixNumpad(true);
      waitingForRelease = true;
      return;
    }
    else if (isInside(x, y, MIX_VALUE_X, MIX_ROW_OFFS_Y, MIX_VALUE_W, MIX_FIELD_H)) {
      openMixNumpad(false);
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
        screenChangePending = false;
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

  queueScreenButton(BTN_BACK, SCREEN_CONTROLLER_SETTINGS);


  return;
}

    int startY = 40;
    int spacing = 45;

    for (int i = 0; i < 5; i++) {

      int ty = startY + (i * spacing);
      int tx = 120;
      int tw = 80;
      int th = 30;

      if (isInside(x, y, tx, ty, tw, th)) {

        if (!waitingForRelease) {
          bool linked[5];
          getLinkedReverseChannels(activeModel, i, linked);
          bool newState = !models[activeModel].reverse[i];

          for (int ch = 0; ch < 5; ch++) {
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

  int edgeMargin = TRIM_BTN_SIZE + 6;  // space for buttons + padding

  int graphLeft   = cx - s + edgeMargin;
  int graphRight  = cx + s - edgeMargin;
  int graphTop    = cy - s + edgeMargin;
  int graphBottom = cy + s - edgeMargin;

  bool didPress = false;

  // ===== BACK =====
  if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {

    if (!waitingForRelease) {

      queueScreenButton(BTN_BACK, SCREEN_CONTROLLER_SETTINGS);
    }

    return;
  }

  // ===== NEXT =====
  if (isInside(x, y, 130, BACK_BTN_Y, 100, BTN_HEIGHT)) {

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
    if (isInside(x, y, trimBtnLeftX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE)) {
      models[activeModel].trimX[currentTrimPage] =
      constrain(models[activeModel].trimX[currentTrimPage] - 1, -50, 50);
      saveModels();
      didPress = true;
    }

    // X+
    else if (isInside(x, y, trimBtnRightX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE)) {
      models[activeModel].trimX[currentTrimPage] =
      constrain(models[activeModel].trimX[currentTrimPage] + 1, -50, 50);
      saveModels();
      didPress = true;
    }

    // Y+
    else if (isInside(x, y, cx - 14, trimBtnTopY, TRIM_BTN_SIZE, TRIM_BTN_SIZE)) {
      models[activeModel].trimY[currentTrimPage] =
      constrain(models[activeModel].trimY[currentTrimPage] + 1, -50, 50);
      saveModels();
      didPress = true;
    }

    // Y-
    else if (isInside(x, y, cx - 14, trimBtnBottomY, TRIM_BTN_SIZE, TRIM_BTN_SIZE)) {
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

  for (int i = 0; i < 5; i++) {

    int ty = startY + (i * spacing);
    int tx = 120;
    int tw = 80;
    int th = 30;

    if (isInside(x, y, tx, ty, tw, th)) {

      if (!waitingForRelease) {

        models[activeModel].failsafe[i] = !models[activeModel].failsafe[i];
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
          }

          // clear last slot
          modelNames[3] = "";
          initModelDefaults(3);

          saveModels();

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
          if ((candidate == "-") || (parsedValue >= -100 && parsedValue <= 100)) {
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

  drawButtonBubble(
    MENU_BTN_X, MENU_BTN_Y, MENU_BTN_W, MENU_BTN_H,
    "MENU",
    pressedButton == BTN_MENU,
    selectedButton == BTN_MENU,
    55
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
  tft.fillRect(iconX - 2, iconY, iconW + 4, iconH, COLOR_STICK_PANEL);

  if (currentDrive == DRIVE_TANK) {
    drawTankIcon(iconX, iconY, iconW, iconH, COLOR_ACCENT);
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

int lx = leftThrottle * range;
int ly = -leftY * range;

int rx = rightThrottle * range;
int ry = -rightY * range;

static int lastLX = 0, lastLY = 0;
static int lastRX = 0, lastRY = 0;
static int lastLatency = -1;
static float lastVoltage = -1;
static String lastModel = "";

if (espNowLatency != lastLatency ||
    abs(batteryVoltage - lastVoltage) > 0.01 ||
    currentModelName != lastModel) {

  int rightX = panelX + panelW / 2;
  int rightW = panelW / 2;

  drawRightPanel(rightX, modelY, rightW, modelH);

  lastLatency = espNowLatency;
  lastVoltage = batteryVoltage;
  lastModel = currentModelName;
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

// ===== REDRAW DIVIDER =====
tft.drawFastVLine(
  panelX + panelW / 2,
  modelY + 10,
  modelH - 20,
  COLOR_ACCENT
);

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
  drawTankIcon(iconX, iconY, iconW, iconH, COLOR_ACCENT);

  // ✅ THEN bars on top
  drawTankBars(iconX, iconY, iconW, iconH, tankLeftOutput, tankRightOutput);

  lastLeft = tankLeftOutput;
  lastRight = tankRightOutput;
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
      55
    );

    lastMenuPressed = pressedButton;
    lastMenuSelected = selectedButton;
  }
}

void drawTankIcon(int x, int y, int w, int h, uint16_t iconColor) {

  int cx = x + w / 2;

  // ===== MAIN BODY (wider) =====
  int bodyW = w / 2;   // was fixed 20 → now scales
  int bodyX = cx - bodyW / 2;

  tft.drawRect(bodyX, y + 10, bodyW, h - 20, iconColor);

  // ===== TURRET =====
  int ty = y + h / 2;
  tft.drawCircle(cx, ty, 6, iconColor);

  // ===== BARREL =====
  tft.drawLine(cx, ty - 6, cx, y + 2, iconColor);

  // ===== TRACKS (pushed outward) =====
  int trackOffset = bodyW / 2 + 8;

  for (int i = 0; i < h - 20; i += 8) {
    tft.drawRect(cx - trackOffset, y + 10 + i, 6, 4, iconColor);
    tft.drawRect(cx + trackOffset - 6, y + 10 + i, 6, 4, iconColor);
  }
}

void drawQuadXIcon(int x, int y, int w, int h, uint16_t iconColor) {

  int cx = x + w / 2;
  int cy = y + h / 2;

  // ===== MAKE IT WIDER THAN TALL =====
  int armX = w / 2 - 10;   // horizontal reach (BIG)
  int armY = h / 3;        // vertical reach (smaller)

  // arms (stretched X)
  tft.drawLine(cx, cy, cx - armX, cy - armY, iconColor);
  tft.drawLine(cx, cy, cx + armX, cy - armY, iconColor);
  tft.drawLine(cx, cy, cx - armX, cy + armY, iconColor);
  tft.drawLine(cx, cy, cx + armX, cy + armY, iconColor);

  // motors
  tft.drawCircle(cx - armX, cy - armY, 5, iconColor);
  tft.drawCircle(cx + armX, cy - armY, 5, iconColor);
  tft.drawCircle(cx - armX, cy + armY, 5, iconColor);
  tft.drawCircle(cx + armX, cy + armY, 5, iconColor);

  // body
  tft.drawCircle(cx, cy, 4, iconColor);
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

  tft.setCursor(x + 14, lineY + 15);
  tft.setTextColor(COLOR_ACCENT);
  tft.print(espNowLatency);
  tft.print(" ms");

  // ===== VOLTAGE =====
  lineY += 30;

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(x + 10, lineY);
  tft.print("Voltage:");

  tft.setCursor(x + 14, lineY + 15);
  tft.setTextColor(COLOR_ACCENT);
  tft.print(batteryVoltage, 2);
  tft.print(" V");
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

  void drawBattery(int x, int y, int level) {
  // level = 0–100

  int w = 24;
  int h = 12;
  int padding = 2;

  // ===== OUTLINE =====
  tft.drawRoundRect(x, y, w, h, 2, COLOR_TEXT);

  // terminal
  tft.fillRect(x + w, y + (h / 3), 3, h / 3, COLOR_TEXT);

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
}

void drawTopBarStatic() {

  // ===== CLEAR AREA =====
  tft.fillRect(0, 0, 240, 35, COLOR_BG);

  // ===== TITLE =====
  tft.setTextFont(4);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 5);
  tft.print("Anubis");

  // ===== RSSI LABEL =====
  tft.setTextFont(2);
  tft.setCursor(110, 10);
  tft.print("RSSI");
}

void drawTopBarDynamic() {

  static unsigned long lastBlinkTime = 0;
  static bool blinkState = true;
  static bool lastBlinkState = true;

  if (batteryLevel < 10) {
    if (millis() - lastBlinkTime >= 1000) {
      blinkState = !blinkState;
      lastBlinkTime = millis();
    }
  } 
    else {
      blinkState = true; // always visible if not low
    }

  if (signalStrength != lastSignal ||
    batteryLevel != lastBattery ||
    blinkState != lastBlinkState) {

    tft.fillRect(140, 5, 100, 25, COLOR_BG);

    drawSignalBars(150, 10, signalStrength);
    if (blinkState) {
      drawBattery(200, 10, batteryLevel);
    }
    else {
    // clear battery area when "off"
    tft.fillRect(200, 10, 30, 20, COLOR_BG);
}

    lastSignal = signalStrength;
    lastBattery = batteryLevel;
    lastBlinkState = blinkState;
  }

  // simulate (keep this outside the condition)
  signalStrength = (millis() / 1000) % 5;
  batteryLevel = (sin(millis() / 2000.0) * 50) + 50;
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
    (dpadFocusVisible && focusIndex == 5),
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  int startY = 40;
  int spacing = 45;

  for (int i = 0; i < 5; i++) {

    int y = startY + (i * spacing);
    bool linked = hasLinkedReversePeers(activeModel, i);

    // Labels
    tft.setTextColor(linked ? COLOR_ACCENT_HI : COLOR_TEXT);
    tft.setCursor(20, y + 10);
    tft.print("CH");
    tft.print(i + 1);

    if (linked) {
      tft.setCursor(58, y + 10);
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

  for (int i = 0; i < 5; i++) {

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

  int offsetTopDown = 34;
  int offsetBottomDown = 5;
  int offsetRight = 5;
  int offsetLeftRight = 20;

  trimBtnLeftX  = cx - s - 20 + offsetLeftRight;
  trimBtnRightX = cx + s - TRIM_BTN_SIZE - 4 + offsetRight;

  trimBtnTopY = cy - s - TRIM_BTN_SIZE - 4 + offsetTopDown;
  if (trimBtnTopY < 40) trimBtnTopY = 40;

  trimBtnBottomY = cy + s - TRIM_BTN_SIZE - 4 + offsetBottomDown;

  int bxLeft = cx - s - 20 + offsetLeftRight;
  tft.fillRoundRect(trimBtnLeftX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(trimBtnLeftX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);
  if (currentScreen == SCREEN_TRIM && dpadFocusVisible && focusIndex == 2) {
    tft.drawRoundRect(trimBtnLeftX - 2, cy - 16, TRIM_BTN_SIZE + 4, TRIM_BTN_SIZE + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawString("-", bxLeft + 10, cy - 10);

  int bxRight = cx + s - TRIM_BTN_SIZE - 4 + offsetRight;
  tft.fillRoundRect(trimBtnRightX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(trimBtnRightX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);
  if (currentScreen == SCREEN_TRIM && dpadFocusVisible && focusIndex == 3) {
    tft.drawRoundRect(trimBtnRightX - 2, cy - 16, TRIM_BTN_SIZE + 4, TRIM_BTN_SIZE + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawString("+", bxRight + 9, cy - 10);

  int byTop = cy - s - TRIM_BTN_SIZE - 4 + offsetTopDown;
  if (byTop < 40) byTop = 40;
  tft.fillRoundRect(cx - 14, trimBtnTopY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(cx - 14, trimBtnTopY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);
  if (currentScreen == SCREEN_TRIM && dpadFocusVisible && focusIndex == 4) {
    tft.drawRoundRect(cx - 16, trimBtnTopY - 2, TRIM_BTN_SIZE + 4, TRIM_BTN_SIZE + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawString("+", cx - 4, byTop + 4);

  int byBottom = cy + s - TRIM_BTN_SIZE - 4 + offsetBottomDown;
  tft.fillRoundRect(cx - 14, trimBtnBottomY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(cx - 14, trimBtnBottomY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);
  if (currentScreen == SCREEN_TRIM && dpadFocusVisible && focusIndex == 5) {
    tft.drawRoundRect(cx - 16, trimBtnBottomY - 2, TRIM_BTN_SIZE + 4, TRIM_BTN_SIZE + 4, 6, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawString("-", cx - 4, byBottom + 6);
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
  drawButtonBubble(130, BACK_BTN_Y, 100, BTN_HEIGHT, navLabel, false, (dpadFocusVisible && focusIndex == 1), 40);

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

  int cx = TRIM_CENTER_X;
  int cy = TRIM_CENTER_Y;
  int s  = TRIM_SIZE;

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

  // ===== UPDATE VALUE TEXT =====
  tft.fillRect(60, 40, 120, 20, COLOR_BG);

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(70, 45);
  tft.print("X:");
  tft.print(targetX);

  tft.setCursor(130, 45);
  tft.print("Y:");
  tft.print(targetY);

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
    (dpadFocusVisible && focusIndex == 5),
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  // ===== CHANNEL ROWS =====
  int startY = 85;
  int spacing = 34;

  for (int i = 0; i < 5; i++) {

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

  for (int i = 0; i < 5; i++) {

    if (!failsafeDirty[i] && !failsafeNeedsRedraw) continue;

    int y = startY + (i * spacing);

    int tx = 120;
    int ty = y;
    int tw = 80;
    int th = 30;

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

// ==== KEYBOARD DYNAMIC ====
void drawKeyboardDynamic() {

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
  tft.drawString(mixNumpadEditingRate ? "Rate" : "Offset", MIX_NUMPAD_X + 10, MIX_NUMPAD_Y + 8, 2);

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

  char valueText[12];
  MixData &mix = models[activeModel].mixes[selectedMixIndex];

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
    snprintf(valueText, sizeof(valueText), "CH%d > CH%d",
             mix.source + 1, mix.destination + 1);
  } else {
    snprintf(valueText, sizeof(valueText), "Mix %d Disabled", selectedMixIndex + 1);
  }
  tft.drawCentreString(valueText, 120, 78, 2);

  const char* labels[5] = {"Enable", "Source", "Dest", "Rate", "Offset"};
  const int rowY[5] = {
    MIX_ROW_ENABLE_Y, MIX_ROW_SOURCE_Y, MIX_ROW_DEST_Y, MIX_ROW_RATE_Y, MIX_ROW_OFFS_Y
  };

  tft.setTextColor(COLOR_TEXT);
  for (int i = 0; i < 5; i++) {
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
  tft.drawString("OFF", MIX_FIELD_X - 34, MIX_ROW_ENABLE_Y + 8, 2);
  tft.drawString("ON", 208, MIX_ROW_ENABLE_Y + 8, 2);

  int knobRadius = 9;
  int knobX = mix.enabled
    ? (MIX_FIELD_X + MIX_FIELD_W - knobRadius - 3)
    : (MIX_FIELD_X + knobRadius + 3);
  tft.fillCircle(knobX, MIX_ROW_ENABLE_Y + (MIX_FIELD_H / 2), knobRadius,
                 mix.enabled ? COLOR_ACCENT : COLOR_TEXT);

  snprintf(valueText, sizeof(valueText), "CH%d", mix.source + 1);
  tft.fillRoundRect(MIX_FIELD_X, MIX_ROW_SOURCE_Y, MIX_FIELD_W, MIX_FIELD_H, 8, COLOR_PANEL);
  tft.drawRoundRect(MIX_FIELD_X, MIX_ROW_SOURCE_Y, MIX_FIELD_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  if (dpadFocusVisible && focusIndex == 6) {
    tft.drawRoundRect(MIX_FIELD_X - 2, MIX_ROW_SOURCE_Y - 2, MIX_FIELD_W + 4, MIX_FIELD_H + 4, 8, COLOR_ACCENT_HI);
  }
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString(valueText, MIX_FIELD_X + (MIX_FIELD_W / 2), MIX_ROW_SOURCE_Y + 8, 2);

  snprintf(valueText, sizeof(valueText), "CH%d", mix.destination + 1);
  tft.fillRoundRect(MIX_FIELD_X, MIX_ROW_DEST_Y, MIX_FIELD_W, MIX_FIELD_H, 8, COLOR_PANEL);
  tft.drawRoundRect(MIX_FIELD_X, MIX_ROW_DEST_Y, MIX_FIELD_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  if (dpadFocusVisible && focusIndex == 7) {
    tft.drawRoundRect(MIX_FIELD_X - 2, MIX_ROW_DEST_Y - 2, MIX_FIELD_W + 4, MIX_FIELD_H + 4, 8, COLOR_ACCENT_HI);
  }
  tft.setTextColor(COLOR_TEXT);
  tft.drawCentreString(valueText, MIX_FIELD_X + (MIX_FIELD_W / 2), MIX_ROW_DEST_Y + 8, 2);

  tft.fillRoundRect(MIX_MINUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.drawRoundRect(MIX_MINUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT_HI);
  if (dpadFocusVisible && focusIndex == 8) {
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
  else if (dpadFocusVisible && focusIndex == 9) {
    tft.drawRoundRect(MIX_VALUE_X - 2, MIX_ROW_RATE_Y - 2, MIX_VALUE_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }

  tft.fillRoundRect(MIX_PLUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.drawRoundRect(MIX_PLUS_X, MIX_ROW_RATE_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT_HI);
  if (dpadFocusVisible && focusIndex == 10) {
    tft.drawRoundRect(MIX_PLUS_X - 2, MIX_ROW_RATE_Y - 2, MIX_ADJUST_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawCentreString("+", MIX_PLUS_X + (MIX_ADJUST_W / 2), MIX_ROW_RATE_Y + 8, 2);

  tft.fillRoundRect(MIX_MINUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.drawRoundRect(MIX_MINUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT_HI);
  if (dpadFocusVisible && focusIndex == 11) {
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
  else if (dpadFocusVisible && focusIndex == 12) {
    tft.drawRoundRect(MIX_VALUE_X - 2, MIX_ROW_OFFS_Y - 2, MIX_VALUE_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }

  tft.fillRoundRect(MIX_PLUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT);
  tft.drawRoundRect(MIX_PLUS_X, MIX_ROW_OFFS_Y, MIX_ADJUST_W, MIX_FIELD_H, 8, COLOR_ACCENT_HI);
  if (dpadFocusVisible && focusIndex == 13) {
    tft.drawRoundRect(MIX_PLUS_X - 2, MIX_ROW_OFFS_Y - 2, MIX_ADJUST_W + 4, MIX_FIELD_H + 4, 8, COLOR_TEXT);
  }
  tft.setTextColor(COLOR_BG);
  tft.drawCentreString("+", MIX_PLUS_X + (MIX_ADJUST_W / 2), MIX_ROW_OFFS_Y + 8, 2);

  mixingNeedsRedraw = false;
}

// ==== CONTROLLER ICON ====
void drawControllerIcon(int x, int y, int s, uint16_t iconColor) {

  int w = 24 * s;
  int h = 24 * s;

  int cx = x + w / 2;
  int cy = y + h / 2;

  // outer body
  drawThickRoundRect(x, y + 4*s, w, 16*s, 6*s, iconColor);

  // grips
  drawThickCircle(x + 6*s,  y + 12*s, 4*s, iconColor);
  drawThickCircle(x + 18*s, y + 12*s, 4*s, iconColor);

  // sticks
  drawThickCircle(cx - 6*s, cy, 2*s, iconColor);
  drawThickCircle(cx + 6*s, cy, 2*s, iconColor);

  // antenna
  drawThickLine(cx, y + 4*s, cx, y, iconColor);
}

// ==== MODEL ICON ====
void drawCarIcon(int x, int y, int s, uint16_t iconColor) {

  int ax = x + 4*s;
  int baseY = y + 8*s;
  int tipY  = baseY - 6*s;

  // body
  drawThickRoundRect(x + 2*s, y + 8*s, 20*s, 10*s, 3*s, iconColor);

  // roof
  drawThickLine(x + 6*s, y + 8*s, x + 10*s, y + 4*s, iconColor);
  drawThickLine(x + 10*s, y + 4*s, x + 16*s, y + 4*s, iconColor);
  drawThickLine(x + 16*s, y + 4*s, x + 20*s, y + 8*s, iconColor);

  // wheels
  drawThickCircle(x + 6*s, y + 20*s, 3*s, iconColor);
  drawThickCircle(x + 18*s, y + 20*s, 3*s, iconColor);

  // antenna
  drawThickLine(ax, baseY, ax, tipY, iconColor);
  drawThickCircle(ax, tipY, 2*s, iconColor);
}

// ==== HOME ICON ====
void drawHomeIcon(int x, int y, int s, uint16_t iconColor) {

  int w = 24 * s;

  // ===== ROOF =====
  drawThickLine(x + 2*s,  y + 12*s, x + 12*s, y + 4*s, iconColor);
  drawThickLine(x + 12*s, y + 4*s,  x + 22*s, y + 12*s, iconColor);

  // ===== BODY =====
  drawThickRoundRect(x + 4*s, y + 12*s, 16*s, 10*s, 2*s, iconColor);

  // ===== DOOR =====
  drawThickLine(x + 11*s, y + 18*s, x + 11*s, y + 22*s, iconColor);
  tft.fillRect(x + 10*s, y + 18*s, 4*s, 4*s, iconColor);  
}
// ==== REVERSE ICON ====
void drawReverseIcon(int x, int y, int s, uint16_t iconColor) {

  int cx = x + 12*s;

  // ===== TOP ARROW =====
  drawThickLine(x + 4*s, y + 8*s, x + 18*s, y + 8*s, iconColor);
  drawThickLine(x + 14*s, y + 5*s, x + 18*s, y + 8*s, iconColor);
  drawThickLine(x + 14*s, y + 11*s, x + 18*s, y + 8*s, iconColor);

  // ===== BOTTOM ARROW =====
  drawThickLine(x + 18*s, y + 16*s, x + 4*s, y + 16*s, iconColor);
  drawThickLine(x + 8*s, y + 13*s, x + 4*s, y + 16*s, iconColor);
  drawThickLine(x + 8*s, y + 19*s, x + 4*s, y + 16*s, iconColor);
}

// ==== TRIM ICON ====
void drawTrimIcon(int x, int y, int s, uint16_t iconColor) {

  int cx = x + 12*s;
  int cy = y + 12*s;

  // ===== CENTER LINE =====
  drawThickLine(cx, cy - 5*s, cx, cy + 5*s, iconColor);

  // ===== PLUS (LEFT - moved =====
  drawThickLine(cx - 9*s, cy, cx - 5*s, cy, iconColor);              // horizontal
  drawThickLine(cx - 7*s, cy - 2*s, cx - 7*s, cy + 2*s, iconColor);  // vertical

  // ===== MINUS (RIGHT =====
  drawThickLine(cx + 5*s, cy, cx + 9*s, cy, iconColor);
}

// ==== FAILSAFE ICON ====
void drawFailsafeIcon(int x, int y, int s, uint16_t iconColor) {

  // ===== TRIANGLE =====
  drawThickLine(x + 12*s, y + 4*s,  x + 4*s,  y + 20*s, iconColor);
  drawThickLine(x + 4*s,  y + 20*s, x + 20*s, y + 20*s, iconColor);
  drawThickLine(x + 20*s, y + 20*s, x + 12*s, y + 4*s,  iconColor);

  // ===== EXCLAMATION =====
  drawThickLine(x + 12*s, y + 9*s, x + 12*s, y + 15*s, iconColor);
  drawThickCircle(x + 12*s, y + 18*s, 1*s, iconColor);
}

// ==== MODEL NAME ICON ====
void drawModelNameIcon(int x, int y, int s, uint16_t iconColor) {

  int w = 24 * s;
  int h = 24 * s;

  // ===== TAG BODY (square) =====
  drawThickRoundRect(x + 3*s, y + 5*s, 18*s, 14*s, 3*s, iconColor);

  // ===== "NAME" TEXT =====
  int ty = y + 12*s;

  // N
  drawThickLine(x + 6*s, ty + 3*s, x + 6*s, ty - 3*s, iconColor);
  drawThickLine(x + 6*s, ty - 3*s, x + 8*s, ty + 3*s, iconColor);
  drawThickLine(x + 8*s, ty + 3*s, x + 8*s, ty - 3*s, iconColor);

  // A
  drawThickLine(x + 10*s, ty + 3*s, x + 11*s, ty - 3*s, iconColor);
  drawThickLine(x + 12*s, ty + 3*s, x + 11*s, ty - 3*s, iconColor);
  drawThickLine(x + 10*s, ty,     x + 12*s, ty,     iconColor);

  // M
  drawThickLine(x + 14*s, ty + 3*s, x + 14*s, ty - 3*s, iconColor);
  drawThickLine(x + 14*s, ty - 3*s, x + 15*s, ty,       iconColor);
  drawThickLine(x + 15*s, ty,       x + 16*s, ty - 3*s, iconColor);
  drawThickLine(x + 16*s, ty - 3*s, x + 16*s, ty + 3*s, iconColor);

  // E
  drawThickLine(x + 18*s, ty + 3*s, x + 18*s, ty - 3*s, iconColor);
  drawThickLine(x + 18*s, ty - 3*s, x + 20*s, ty - 3*s, iconColor);
  drawThickLine(x + 18*s, ty,       x + 19*s, ty,       iconColor);
  drawThickLine(x + 18*s, ty + 3*s, x + 20*s, ty + 3*s, iconColor);

  // ===== OPTIONAL UNDERLINE =====
  drawThickLine(x + 7*s, y + 16*s, x + 17*s, y + 16*s, iconColor);
}

// ==== DRIVE TYPE ICON ====
void drawDriveTypeIcon(int x, int y, int s, uint16_t iconColor) {

  int size = 24 * s;

  drawTankIcon(x, y, size, size, iconColor);
}

// ==== MIXING ICON ====
void drawMixingIcon(int x, int y, int s, uint16_t iconColor) {

  int cy = y + 12*s;

  // ===== LEFT ARROW (up) =====
  drawThickLine(x + 8*s,  y + 18*s, x + 8*s,  y + 6*s, iconColor);
  drawThickLine(x + 5*s,  y + 10*s, x + 8*s,  y + 6*s, iconColor);
  drawThickLine(x + 11*s, y + 10*s, x + 8*s,  y + 6*s, iconColor);

  // ===== RIGHT ARROW (down) =====
  drawThickLine(x + 16*s, y + 6*s,  x + 16*s, y + 18*s, iconColor);
  drawThickLine(x + 13*s, y + 14*s, x + 16*s, y + 18*s, iconColor);
  drawThickLine(x + 19*s, y + 14*s, x + 16*s, y + 18*s, iconColor);
}
