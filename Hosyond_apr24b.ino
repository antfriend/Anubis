#include <RAK14014_FT6336U.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <EEPROM.h>

#include "anubis_rgb565.h"

  TFT_eSPI tft = TFT_eSPI();
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
  BTN_MODEL_NAME,
  BTN_OPTION_2,
  BTN_OPTION_3,
  BTN_OPTION_4,
  BTN_OPTION_5
  };

  enum DriveType {
  DRIVE_TANK,
  DRIVE_CAR,
  DRIVE_QUAD_X
  };

  #define MAX_MODELS 4

  struct ModelData {
  char name[20];

  bool reverse[5];
  bool failsafe[5];

  int trimX[2];
  int trimY[2];
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

  bool uiNeedsRedraw = true;
  bool touchActive = false;
  bool waitingForRelease = false;
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

  // placeholder values for now (you’ll replace with real captured values later)
  int failsafeValue[5] = { 0, 0, 0, 0, 0 };

  bool isInside(int x, int y, int bx, int by, int bw, int bh);
  int mapTouch(int val, int in_min, int in_max, int out_min, int out_max);
  void drawSplash();

  void initModelDefaults(int i) {
    for (int ch = 0; ch < 5; ch++) {
      models[i].reverse[ch] = false;
      models[i].failsafe[ch] = false;
    }

    for (int g = 0; g < 2; g++) {
      models[i].trimX[g] = 0;
      models[i].trimY[g] = 0;
    }

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

  const char* keyboardLayout[KB_ROWS][KB_COLS] = {
  {"1","2","3","4","5","6","7","8","9","0"},
  {"Q","W","E","R","T","Y","U","I","O","P"},
  {"A","S","D","F","G","H","J","K","L","<"},
  {"Z","X","C","V","B","N","M","","OK",""},
  {" ","","","","","","","","",""}
  };

  // ===== MODEL NAME =====
  String modelNames[4] = {"", "", "", ""};
  int selectedModelIndex = -1;
  bool modelNameNeedsRedraw = true;
  bool modelNameDirty = true;
  bool inputBoxSelected = false;

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

  EEPROM.begin(512);

  loadModels();

  // FIRST BOOT CHECK
  for (int i = 0; i < MAX_MODELS; i++) {
  initModelDefaults(i);
  }
  saveModels();

  // sync names
  for (int i = 0; i < 4; i++) {
    modelNames[i] = String(models[i].name);
  }

  // SET ACTIVE MODEL
  activeModel = 0;
  currentModelName = String(models[0].name);

  // optional polish
  trimRenderX = models[activeModel].trimX[currentTrimPage];
  trimRenderY = models[activeModel].trimY[currentTrimPage];
  }

void loop() {
    
  if (currentScreen != lastScreen) {

    fullRedraw = true;
    uiNeedsRedraw = true;

    lastScreen = currentScreen;
  }

  // read raw
  int rawX = touchPanel.read_touch1_x();
  int rawY = touchPanel.read_touch1_y();

  bool select = digitalRead(2) == LOW;
  bool down  = digitalRead(3) == LOW;
  bool left  = digitalRead(14) == LOW;
  bool right = digitalRead(21) == LOW;

  bool userActive = false;

  leftThrottle  = sin(millis() / 500.0);
  rightThrottle = cos(millis() / 500.0);
  leftY  = cos(millis() / 500.0);
  rightY = sin(millis() / 500.0);

  m1 = (sin(millis() / 400.0) + 1) / 2;
  m2 = (cos(millis() / 400.0) + 1) / 2;
  m3 = (sin(millis() / 600.0) + 1) / 2;
  m4 = (cos(millis() / 600.0) + 1) / 2;

  static float lastLX = 0;
  static float lastRX = 0;
  static unsigned long lastDpadTime = 0;

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

  if (abs(leftThrottle - lastLX) > 0.005 ||
      abs(rightThrottle - lastRX) > 0.005) {

    uiNeedsRedraw = true;

    lastLX = leftThrottle;
    lastRX = rightThrottle;
  }

  if (currentScreen == SCREEN_SPLASH) {
    drawSplash();
    fullRedraw = true;
    return;
  }

  if (millis() - lastDpadTime > 150) {

    bool didInput = false;

    if (currentScreen == SCREEN_MAIN) {

      if (down) {
        selectedButton = BTN_MENU;
        fullRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }

      if (select && selectedButton == BTN_MENU) {
        currentScreen = SCREEN_MENU;
        selectedButton = BTN_NONE;
        fullRedraw = true;
        uiNeedsRedraw = true;
        didInput = true;
      }
    }

  else if (currentScreen == SCREEN_MENU) {

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
      currentScreen = SCREEN_MAIN;
      selectedButton = BTN_NONE;
      fullRedraw = true;
      uiNeedsRedraw = true;
      didInput = true;
      userActive = true;
    }

    if (select) {

      if (selectedButton == BTN_BACK) currentScreen = SCREEN_MAIN;
      else if (selectedButton == BTN_CTRL) currentScreen = SCREEN_CONTROLLER_SETTINGS;
      else if (selectedButton == BTN_MODEL) currentScreen = SCREEN_MODEL_SETTINGS;

      selectedButton = BTN_NONE;
      uiNeedsRedraw = true;
      didInput = true;
      userActive = true;
    }
  }

    if (didInput) {
      lastDpadTime = millis();
    }
  }

  // ===== TOUCH DETECTION =====
  static int lastRawX = 0;
  static int lastRawY = 0;
  static unsigned long lastTouchTime = 0;

  bool isTouching = false;

  // detect movement
  if (abs(rawX - lastRawX) > 3 || abs(rawY - lastRawY) > 3) {
    lastTouchTime = millis();
  }

  // consider "touching" for 100ms after last movement
  if (millis() - lastTouchTime < 100) {
    isTouching = true;
  }

  lastRawX = rawX;
  lastRawY = rawY;

  int x = mapTouch(rawX, 0, 240, 0, 240);
  int y = mapTouch(rawY, 320, 0, 320, 0);

  x = constrain(x, 0, 240);
  y = constrain(y, 0, 320);

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
  waitingForRelease = false;
  pressedButton = BTN_NONE;

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

  if (nextScreen != currentScreen) {
    currentScreen = nextScreen;
    fullRedraw = true;
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
    drawTopBarDynamic();
    }

    uiNeedsRedraw = false;
  }

  if (keyboardActive) {
    if (keyboardNeedsRedraw) {
      drawKeyboardStatic();
      keyboardNeedsRedraw = false;
    }
  drawKeyboardDynamic();
  }

  switch (currentScreen) {
  }
      
  // ===== TOUCH =====
  if (isTouching) {

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
    selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  // ===== MAIN BUTTONS =====
  drawButtonBubble(
    CTRL_BTN_X, 60,
    CTRL_BTN_W, CTRL_BTN_H,
    "Controller",
    pressedButton == BTN_CTRL,
    selectedButton == BTN_CTRL,
    100);

  drawButtonBubble(
    MODEL_BTN_X, 150,
    MODEL_BTN_W, MODEL_BTN_H,
    "Model",
    pressedButton == BTN_MODEL,
    selectedButton == BTN_MODEL,
    100);
}

  void drawControllerSettings() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

    tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  drawButtonBubble(20, 50, 200, BTN_HEIGHT, "Reverse", false, false, 100);
  drawButtonBubble(20, 120, 200, BTN_HEIGHT, "Trim", false, false, 100);
  drawButtonBubble(20, 190, 200, BTN_HEIGHT, "Failsafe", false, false, 100);
  }

  void drawModelSettings() {
  tft.fillScreen(COLOR_BG);

  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK",
    pressedButton == BTN_BACK,
    selectedButton == BTN_BACK,
    BACK_TEXT_OFFSET);

    tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  drawButtonBubble(20, 50, 200, BTN_HEIGHT, "Model Name", false, false, 100);
  drawButtonBubble(20, 120, 200, BTN_HEIGHT, "Drive Type", false, false, 100);
  drawButtonBubble(20, 190, 200, BTN_HEIGHT, "Mixing", false, false, 100);
}

void closeKeyboard() {
  if (!keyboardActive) return;

  keyboardActive = false;
  keyboardNeedsRedraw = true;

  // force full redraw of underlying screen
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

void saveModels() {
  int addr = 0;

  for (int i = 0; i < MAX_MODELS; i++) {
    EEPROM.put(addr, models[i]);
    addr += sizeof(ModelData);
  }

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

  if (keyboardActive && handleKeyboardTouch(x, y)) return;

  // ===== MAIN =====
  if (currentScreen == SCREEN_MAIN) {

    if (isInside(x, y, MENU_BTN_X, MENU_BTN_Y, MENU_BTN_W, MENU_BTN_H)) {
      pressedButton = BTN_MENU;
      waitingForRelease = true;
      nextScreen = SCREEN_MENU;
      fullRedraw = true;
      uiNeedsRedraw = true;
      return;
    }

    if (isInside(x, y, modelPanelX, modelPanelY, modelPanelW, modelPanelH)) {
      pressedButton = BTN_DRIVE_TYPE;
      waitingForRelease = true;
      nextScreen = SCREEN_MODEL_SETTINGS;
      selectedButton = BTN_DRIVE_TYPE;
      uiNeedsRedraw = true;
      return;
    }
  }

  // ===== MENU =====
  else if (currentScreen == SCREEN_MENU) {

    if (isInside(x, y, 10, FOOTER_Y + 5, 100, BTN_HEIGHT)) {
      pressedButton = BTN_BACK;
      waitingForRelease = true;
      nextScreen = SCREEN_MAIN;
      fullRedraw = true;
      uiNeedsRedraw = true;
      return;
    }

    else if (isInside(x, y, CTRL_BTN_X, 60, CTRL_BTN_W, CTRL_BTN_H)) {
      pressedButton = BTN_CTRL;
      waitingForRelease = true;
      nextScreen = SCREEN_CONTROLLER_SETTINGS;
      uiNeedsRedraw = true;
      return;
    }

    else if (isInside(x, y, MODEL_BTN_X, 150, MODEL_BTN_W, MODEL_BTN_H)) {
      pressedButton = BTN_MODEL;
      waitingForRelease = true;
      nextScreen = SCREEN_MODEL_SETTINGS;
      uiNeedsRedraw = true;
      return;
    }
  }

  // ===== CONTROLLER SETTINGS =====
  else if (currentScreen == SCREEN_CONTROLLER_SETTINGS) {

    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
      pressedButton = BTN_BACK;
      waitingForRelease = true;
      nextScreen = SCREEN_MENU;
      uiNeedsRedraw = true;
      return;
    }

    else if (isInside(x, y, 20, 50, 200, BTN_HEIGHT)) {
      pressedButton = BTN_REVERSE;
      waitingForRelease = true;
      nextScreen = SCREEN_REVERSE;
      uiNeedsRedraw = true;
      return;
    }

    else if (isInside(x, y, 20, 120, 200, BTN_HEIGHT)) {
      pressedButton = BTN_TRIM;
      waitingForRelease = true;
      nextScreen = SCREEN_TRIM;
      uiNeedsRedraw = true;
      return;
    }

    else if (isInside(x, y, 20, 190, 200, BTN_HEIGHT)) {
      pressedButton = BTN_FAILSAFE;
      waitingForRelease = true;
      nextScreen = SCREEN_FAILSAFE;
      uiNeedsRedraw = true;
      return;
    }
  }

  // ===== MODEL SETTINGS =====
  else if (currentScreen == SCREEN_MODEL_SETTINGS) {

  // ===== BACK =====
  if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {
    pressedButton = BTN_BACK;
    waitingForRelease = true;
    nextScreen = SCREEN_MENU;
    uiNeedsRedraw = true;
    return;
  }

  // ===== MODEL NAME =====
  else if (isInside(x, y, 20, 50, 200, BTN_HEIGHT)) {
    pressedButton = BTN_MODEL_NAME;   // or BTN_MODEL_NAME if you want to define it
    waitingForRelease = true;
    nextScreen = SCREEN_MODEL_NAME;
    uiNeedsRedraw = true;
    return;
  }

  // ===== DRIVE TYPE =====
  else if (isInside(x, y, 20, 120, 200, BTN_HEIGHT)) {
    // (future)
  }

  // ===== MIXING =====
  else if (isInside(x, y, 20, 190, 200, BTN_HEIGHT)) {
    // (future)
  }
}

  // ===== REVERSE =====
  else if (currentScreen == SCREEN_REVERSE) {

    if (isInside(x, y, BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H)) {

  pressedButton = BTN_BACK;
  waitingForRelease = true;
  nextScreen = SCREEN_CONTROLLER_SETTINGS;


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
          models[activeModel].reverse[i] = !models[activeModel].reverse[i];
          saveModels();
          reverseChannelDirty[i] = true;
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

      nextScreen = SCREEN_CONTROLLER_SETTINGS;

      waitingForRelease = true;
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

    pressedButton = BTN_BACK;
    waitingForRelease = true;
    nextScreen = SCREEN_CONTROLLER_SETTINGS;
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
      pressedButton = BTN_BACK;
      waitingForRelease = true;
      nextScreen = SCREEN_MODEL_SETTINGS;
      return;
    }

    // ===== INPUT BOX =====
    if (isInside(x, y, 10, 65, 220, 30)) {
      if (!waitingForRelease) {
        keyboardActive = true;
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

          activeModel = i;
selectedModelIndex = i;

currentModelName = String(models[i].name);

// sync trim render
trimRenderX = models[i].trimX[currentTrimPage];
trimRenderY = models[i].trimY[currentTrimPage];

// ===== FORCE UI UPDATES =====
trimNeedsRedraw = true;
reverseNeedsRedraw = true;
failsafeNeedsRedraw = true;
trimDirty = true;

// 🔥 THIS WAS MISSING
modelNameDirty = true;
modelNameNeedsRedraw = true;

for (int c = 0; c < 5; c++) {
  reverseChannelDirty[c] = true;
  failsafeDirty[c] = true;
}

uiNeedsRedraw = true;
waitingForRelease = true;
        }

        return;
      }

      listY += 25;
    }

    // ===== DELETE BUTTONS =====
    int y = 130;

    for (int i = 0; i < 4; i++) {

    int bx = 200;
    int by = y - 2;
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
      y += 25;
    }

    // ===== TAP ABOVE KEYBOARD CLOSES IT ===== 
    if (keyboardActive && y < kbY) {
      if (!waitingForRelease) {
        closeKeyboard();
        inputBoxSelected = false;
        waitingForRelease = true;
      }
    return;
    }
  }
}

bool handleKeyboardTouch(int x, int y) {

  if (!keyboardActive) return false;

  for (int r = 0; r < KB_ROWS; r++) {
    for (int c = 0; c < KB_COLS; c++) {

      const char* key = keyboardLayout[r][c];
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

          // ===== BACKSPACE =====
          if (strcmp(key, "<") == 0) {
            if (keyboardBuffer.length() > 0) {
              keyboardBuffer.remove(keyboardBuffer.length() - 1);
              modelNameDirty = true;
            }
          }

          // ===== OK / SAVE =====
          else if (strcmp(key, "OK") == 0) {

            if (keyboardBuffer.length() > 0) {

              // ===== SHIFT LIST (UI) =====
              for (int i = 3; i > 0; i--) {
                modelNames[i] = modelNames[i - 1];
              }

              modelNames[0] = keyboardBuffer;

              // ===== SHIFT MODELS (REAL DATA) =====
              for (int i = 3; i > 0; i--) {
                models[i] = models[i - 1];
              }

              // ===== INIT NEW MODEL =====
              initModelDefaults(0);

              // set name into struct
              strncpy(models[0].name, keyboardBuffer.c_str(), 19);
              models[0].name[19] = '\0';

              // SET ACTIVE MODEL (THIS IS THE LINE YOU ASKED ABOUT)
              activeModel = 0;
              currentModelName = String(models[0].name);

              // sync trim render immediately (prevents jump)
              trimRenderX = models[0].trimX[currentTrimPage];
              trimRenderY = models[0].trimY[currentTrimPage];

              saveModels();
            }

            // ===== CLEAR INPUT =====
              keyboardBuffer = "";

            // ===== FORCE UI UPDATE =====
            modelNameDirty = true;
            modelNameNeedsRedraw = true;
            uiNeedsRedraw = true;

            // ===== CLOSE KEYBOARD =====
            closeKeyboard();
          }

          // ===== NORMAL INPUT =====
          else {

            // optional: prevent overflow (fits your input box better)
            if (keyboardBuffer.length() < 18) {
              keyboardBuffer += key;
              modelNameDirty = true;
            }
          }

          keyboardNeedsRedraw = true;
          uiNeedsRedraw = true;
          waitingForRelease = true;
        }

        return true;
      }
    }
  }

  return false;
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
    currentScreen = SCREEN_MAIN;
    fullRedraw = true;
    uiNeedsRedraw = true;
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
    else {
    drawQuadXIcon(iconX, iconY, iconW, iconH, COLOR_ACCENT);
    }
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
int rightX = panelX + panelW / 2;
int rightW = panelW / 2;

drawRightPanel(rightX, modelY, rightW, modelH);

if (currentDrive == DRIVE_TANK) {

static float lastLeft = 0;
static float lastRight = 0;

if (abs(leftThrottle - lastLeft) > 0.01 ||
    abs(rightThrottle - lastRight) > 0.01) {

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
  drawTankBars(iconX, iconY, iconW, iconH, leftThrottle, rightThrottle);

  lastLeft = leftThrottle;
  lastRight = rightThrottle;
}
}

  // ===== MENU BUTTON =====
  // redraw only this button (it handles its own visuals)
  drawButtonBubble(
    MENU_BTN_X, MENU_BTN_Y, MENU_BTN_W, MENU_BTN_H,
    "MENU",
    pressedButton == BTN_MENU,
    selectedButton == BTN_MENU,
    55
  );
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

void drawStickBase(int x, int y, int size) {

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
    false,
    BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  int startY = 40;
  int spacing = 45;

  for (int i = 0; i < 5; i++) {

    int y = startY + (i * spacing);

    // Labels
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(20, y + 10);
    tft.print("CH");
    tft.print(i + 1);

    // Toggle outline
    tft.drawRoundRect(120, y, 80, 30, 10, COLOR_ACCENT);

    int tx = 120;
    int ty = y;
    int tw = 80;
    int th = 30;

    // LEFT label (OFF)
    tft.setTextColor(COLOR_TEXT);
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
void drawTrimStatic() {
  tft.fillScreen(COLOR_BG);

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(90, 20);

  // BACK
  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK", false, false, BACK_TEXT_OFFSET);

  // NEXT button (right side)
  const char* navLabel = (currentTrimPage == 0) ? "NEXT" : "PREV";
  drawButtonBubble(130, BACK_BTN_Y, 100, BTN_HEIGHT, navLabel, false, false, 40);

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

  // grid spacing
  int step = 20;

  // vertical grid lines
  for (int i = -s; i <= s; i += step) {
    tft.drawFastVLine(cx + i, cy - s, s * 2, TFT_DARKGREY);
  }

  // horizontal grid lines
  for (int i = -s; i <= s; i += step) {
    tft.drawFastHLine(cx - s, cy + i, s * 2, TFT_DARKGREY);
  }

  // ===== CROSSHAIR =====
  tft.drawLine(cx - s, cy, cx + s, cy, COLOR_ACCENT);
  tft.drawLine(cx - s, cy + 1, cx + s, cy + 1, COLOR_ACCENT);

  tft.drawLine(cx, cy - s, cx, cy + s, COLOR_ACCENT);
  tft.drawLine(cx + 1, cy - s, cx + 1, cy + s, COLOR_ACCENT);

  // ===== BUTTONS =====
  int offsetTopDown = 34;
  int offsetBottomDown = 5;
  int offsetRight = 5;
  int offsetLeftRight = 20;

  trimBtnLeftX  = cx - s - 20 + offsetLeftRight;
  trimBtnRightX = cx + s - TRIM_BTN_SIZE - 4 + offsetRight;

  trimBtnTopY = cy - s - TRIM_BTN_SIZE - 4 + offsetTopDown;
  if (trimBtnTopY < 40) trimBtnTopY = 40;

  trimBtnBottomY = cy + s - TRIM_BTN_SIZE - 4 + offsetBottomDown;

  // LEFT (-X)
  int bxLeft = cx - s - 20 + offsetLeftRight;

  tft.fillRoundRect(trimBtnLeftX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(trimBtnLeftX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);

  tft.setTextColor(COLOR_BG);
  tft.drawString("-", bxLeft + 10, cy - 10);

  // RIGHT (+X)
  int bxRight = cx + s - TRIM_BTN_SIZE - 4 + offsetRight;

  tft.fillRoundRect(trimBtnRightX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(trimBtnRightX, cy - 14, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);

  tft.setTextColor(COLOR_BG);
  tft.drawString("+", bxRight + 9, cy - 10);

  // TOP (+Y)
  int byTop = cy - s - TRIM_BTN_SIZE - 4 + offsetTopDown;
  if (byTop < 40) byTop = 40;

  tft.fillRoundRect(cx - 14, trimBtnTopY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(cx - 14, trimBtnTopY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);

  tft.setTextColor(COLOR_BG);
  tft.drawString("+", cx - 4, byTop + 4);

  // BOTTOM (-Y)
  int byBottom = cy + s - TRIM_BTN_SIZE - 4 + offsetBottomDown;

  tft.fillRoundRect(cx - 14, trimBtnBottomY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT);
  tft.drawRoundRect(cx - 14, trimBtnBottomY, TRIM_BTN_SIZE, TRIM_BTN_SIZE, 6, COLOR_ACCENT_HI);

  tft.setTextColor(COLOR_BG);
  tft.drawString("-", cx - 4, byBottom + 6);
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

  // ===== ERASE PREVIOUS DOT =====
  tft.fillCircle(lastPx, lastPy, 7, COLOR_BG);

  // ===== REDRAW CROSSHAIR CENTER =====
  tft.drawLine(cx - 30, cy, cx + 30, cy, COLOR_ACCENT);
  tft.drawLine(cx, cy - 30, cx, cy + 30, COLOR_ACCENT);

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
    false,
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

      const char* key = keyboardLayout[r][c];
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

      const char* key = keyboardLayout[r][c];
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

  const char* key = keyboardLayout[kbCursorRow][kbCursorCol];

  if (strlen(key) == 0) return;

  // ===== BACKSPACE =====
  if (strcmp(key, "<") == 0) {
    if (keyboardBuffer.length() > 0) {
      keyboardBuffer.remove(keyboardBuffer.length() - 1);
      modelNameDirty = true;
    }
  }

  // ===== OK =====
  else if (strcmp(key, "OK") == 0) {

    if (keyboardBuffer.length() > 0) {

      for (int i = 3; i > 0; i--) {
        modelNames[i] = modelNames[i - 1];
        models[i] = models[i - 1];
      }

      initModelDefaults(0);

      strncpy(models[0].name, keyboardBuffer.c_str(), 19);
      models[0].name[19] = '\0';

      modelNames[0] = keyboardBuffer;

      activeModel = 0;
      currentModelName = String(models[0].name);

      trimRenderX = models[0].trimX[currentTrimPage];
      trimRenderY = models[0].trimY[currentTrimPage];

      saveModels();
    }

    keyboardBuffer = "";
    closeKeyboard();
  }

  // ===== NORMAL KEY =====
  else {
    if (keyboardBuffer.length() < 18) {
      keyboardBuffer += key;
      modelNameDirty = true;
    }
  }

  keyboardNeedsRedraw = true;
  uiNeedsRedraw = true;
}

// ==== MODEL NAME STATIC ====
void drawModelNameStatic() {

  tft.fillScreen(COLOR_BG);

  // ===== BACK =====
  drawButtonBubble(
    BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
    "BACK", false, false, BACK_TEXT_OFFSET);

  tft.drawFastHLine(0, FOOTER_Y - 5, 240, COLOR_ACCENT);

  // ===== LABEL =====
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 45);
  tft.print("Model Name:");

  // ===== INPUT BOX =====
  tft.drawRoundRect(10, 65, 220, 30, 6, COLOR_ACCENT);

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

    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(208, y);
    tft.print("-");
  }

  modelNameDirty = false;
  modelNameNeedsRedraw = false;
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
