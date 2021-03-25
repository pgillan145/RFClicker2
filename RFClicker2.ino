
// settings
#define CEREAL
//#define EEPROM 0x50    //Address of 24LC256 eeprom chip

// buttons
#define BTMENU          2 // white
#define POWER           3 // black
#define MENURIGHT       4 // orange/yellow
#define RIGHT           5 // red
#define DOWN            6 // green
#define LEFT            7 // blue
#define MENULEFT        8 // brown/black
#define UP              9 // yellow

#define HOLD_TIMEOUT    300   // how long a button needs to be held down before it triggers an input and resets
#define BOUNCE_TIMEOUT  35   // the minimum time a button needs to be held down to count as a click
#define SEND_BLINK_TIME 100  // how long the connection light brightness should jump when data is "sent".
#define BUTTONS         10


// oled
#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
#define SCREEN_ADDRESS  0x3C
#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define BMP_HEIGHT      16
#define BMP_WIDTH       16
#define OLED_CHAR_SIZE  8

#include <PGardLib.h>
#include <RFClickerLib.h>
#include "oled.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoBLE.h>
#include <Scheduler.h>

#define MODES           2
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

volatile uint32_t now = 0;
volatile uint32_t one_sec = now + 1000;
bool right_now = false;
volatile uint32_t last_clicked[BUTTONS];
volatile uint32_t last_released[BUTTONS];

void setup() {
  PGardLibSetup();

  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(LEFT, INPUT_PULLUP);  
  pinMode(UP, INPUT_PULLUP);
  pinMode(MENURIGHT, INPUT_PULLUP);
  pinMode(MENULEFT, INPUT_PULLUP);
  pinMode(POWER, INPUT_PULLUP);
  pinMode(BTMENU, INPUT_PULLUP);
  digitalWrite(RIGHT, HIGH);
  digitalWrite(DOWN, HIGH);
  digitalWrite(LEFT, HIGH);
  digitalWrite(UP, HIGH);
  digitalWrite(MENURIGHT, HIGH);
  digitalWrite(MENULEFT, HIGH);
  digitalWrite(POWER, HIGH);
  digitalWrite(BTMENU, HIGH);

  //clearLog();
  //String oldlog = readLog();
  //SPL("OLD LOG:");
  //SPL(oldlog);
  //SPL("-----------------");
  //SPL("");

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    SPL(F("SSD1306 allocation failed"));
    errorBlink();  
  }
  display.setRotation(2);
  oledStatus("SETUP BLE");  
  
  while (!BLE.begin()) {
    SPL("starting BLE failed!");
    for (int i=0; i<4; i++) {
      oledStatus("BLE FAILED");
      delay(500);
      oledStatus("");
      delay(500);
    }
    oledStatus("SETUP BLE");  
  }
  //BLE.setTimeout(1);
  BLE.scanForUuid(BUTTONSVC_UUID);
  
  arrayFill(0, last_clicked, BUTTONS);
  arrayFill(1, last_released, BUTTONS);
  oledStatus("SETUP CLICK");
  Scheduler.startLoop(clickLoop);
  oledStatus("END SETUP");  
} // setup()

bool connected = false;
String device_name = "";
int mode = 0;
BLEDevice peripherals[8];

void loop() {
  now = millis();
  right_now = false;
  if (now >= one_sec) {
    right_now = true;
    one_sec = now + 1000;
  }
  
  BLEDevice peripheral = BLE.available();
  switch(mode) {
    case 0:      
      if (peripheral) {
        BLE.stopScan();
        SP("got a peripheral:");
        SPL(peripheral.localName());
        controller(peripheral);
        BLE.scanForUuid(BUTTONSVC_UUID);
      }
      else {
        if (clicked(BTMENU)) {
          BLE.stopScan();
          SPL("switching to local mode");
          mode = 1;
        }
      }
      break;
    case 1:
      /*
      if (peripheral) {
        arrayUnshift(peripheral, peripherals, 8);
      }
      */
      if (clicked(BTMENU)) {
        SPL("switching to controller mode");
        BLE.scanForUuid(BUTTONSVC_UUID);
        mode = 0;
      }
      else {
        /*
        for (int i = 0; i< 4; i++) {
          if (peripherals[i]) {
            oled_menu[i] = peripherals[i].deviceName();
          }
          else {
            oled_menu[i] = "";
          }
        }
        */
        for (int i = 0; i< OLED_MENU_ITEMS; i++) {
          if (peripherals[i]) {
            updateOledMenu(i, peripherals[i].deviceName());
          }
          else {
            updateOledMenu(i, "");
          }
        }
        // hardware tests -- TODO, make a menu option that implements this mode.
        if (clicked(RIGHT)) { oledStatus("button1", SEND_BLINK_TIME); }
        if (clicked(DOWN)) { oledStatus("button2", SEND_BLINK_TIME); }
        if (clicked(LEFT)) { oledStatus("button3", SEND_BLINK_TIME); }
        if (clicked(UP)) { oledStatus("button4", SEND_BLINK_TIME); }
        if (clicked(MENULEFT)) { oledStatus("button6", SEND_BLINK_TIME); }
        if (clicked(MENURIGHT)) { oledStatus("button5", SEND_BLINK_TIME); }
        if (clicked(POWER)) { oledStatus("power", SEND_BLINK_TIME); }
      }
      break;
  } // switch(mode)
  oledStatus();
} // loop()

void controller(BLEDevice peripheral) {
  // connect to the peripheral
  device_name = peripheral.localName();
  oledStatus("CONNECTING");
  oledStatus(device_name);
  if (!peripheral.connect()) {
    oledStatus("FAILED");
    device_name = "";
    return;
  }
  // discover peripheral attributes
  oledStatus("DISCOVERING");
  if (peripheral.discoverService(BUTTONSVC_UUID)) {
    oledStatus("DISCOVERED");
  } else {
    oledStatus("FAILED");
    device_name = "";
    peripheral.disconnect();
    // Why?
    // while (1);
    return;
  }
  
  // retrieve the simple key characteristic
  BLECharacteristic buttonChar = peripheral.characteristic(BUTTONCHAR_UUID);
  if (!buttonChar) {
    oledStatus("NO BTN");
    device_name = "";
    peripheral.disconnect();
    return;
  }
  /* 
   *  since we're only writing to these -- not reading -- do we need to subscribe?*/
  oledStatus("BTN-SUBSCRIBE");
  if (!buttonChar.canSubscribe() || !buttonChar.subscribe()) {
    oledStatus("FAILED");
    device_name = "";
    peripheral.disconnect();
    return;
  } 
  else {
    oledStatus("SUBSCRIBED");
  }
  
  BLECharacteristic menuChar = peripheral.characteristic(MENUCHAR_UUID);
  if (menuChar && menuChar.canRead() && menuChar.canSubscribe()) {
    updateOledMenu(BLE.central(), menuChar);
    oledStatus("MENU-SUBSCRIBE");
    if (menuChar.subscribe()) {
      oledStatus("SUBSCRIBED");
      menuChar.setEventHandler(BLEWritten, updateOledMenu);
    }
    else {
      oledStatus("FAILED");
      device_name = "";
      peripheral.disconnect();
      return;
    }
  }
  
  connected = true;
  oledStatus("CONNECTED");
  byte button_status = 0;
  byte last_button_status = 0;

  while (peripheral.connected()) {
    now = millis();
    if (clicked(RIGHT)) {
        oledStatus("button1", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON1);
    }
    else {
      CLRBUTTON(button_status, BUTTON1);
    }
    if (clicked(DOWN) > 0) {
        oledStatus("button2", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON2);
    }
    else {
      CLRBUTTON(button_status, BUTTON2);
    }

    if (clicked(LEFT) > 0) {
        oledStatus("button3", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON3);
    }
    else {
      CLRBUTTON(button_status, BUTTON3);
    }
    if (clicked(UP)) {
        oledStatus("button4", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON4);
    }
    else {
      CLRBUTTON(button_status, BUTTON4);
    }
    if (clicked(MENURIGHT) > 0) {
        oledStatus("button5", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON5);
    }
    else {
      CLRBUTTON(button_status, BUTTON5);
    }
    if (clicked(MENULEFT) > 0) {
        oledStatus("button6", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON6);
    }
    else {
      CLRBUTTON(button_status, BUTTON6);
    }

    if (button_status != last_button_status) {
      buttonChar.writeValue(button_status);
      last_button_status = button_status;
    }

    if (clicked(BTMENU)) {
      mode = 1;
      peripheral.disconnect();
    }      
    oledStatus();
  } // peripheral.connected()
  SPL("disconnected");
  clearOledMenu();
  connected = false;
  device_name = "";
  oledStatus("DISCONNECTED");
  return;
} // controller()

/*
 * Store all the button clicks in an array that we can query later.  Handles the debouncing logic, and discards any stale
 * clicks that aren't queried after a second.
 */
uint32_t click_now = 0;
uint32_t click_one_sec = now + 1000;
bool click_right_now = false;
void clickLoop() {
  click_now = millis();
  click_right_now = false;
  if (click_now >= click_one_sec) {
    click_right_now = true;
    click_one_sec = click_now + 1000;
  }

  if (click_right_now) {
    SP("now:"); SPL(click_now);
    for (int button = 0; button < BUTTONS; button++) {
      SP("button "); SP(button); SP(" last clicked:"); SP(last_clicked[button]);
      SP(" last released:"); SPL(last_released[button]);
    }
  }

  for (int button = 0; button < BUTTONS; button++) {
    if (digitalRead(button) == LOW) {
      if (last_clicked[button] < last_released[button]) {
        SP("button ");
        SP(button);
        SPL(" clicked");
        last_clicked[button] = click_now;
      }
    }
    else { // button == HIGH
      if (last_clicked[button] > last_released[button]) {
        if (last_released[button] - last_clicked[button] > BOUNCE_TIMEOUT) {
          SP("button ");
          SP(button);
          SPL(" released");
          last_released[button] = click_now;
        }
        else {
          SP("button ");
          SP(button);
          SPL(" bounced");
          last_clicked[button] = 0;
          last_released[button] = 1;
        }
      }
      else if (last_clicked[button] > 0 && last_released[button] >= last_clicked[button] && last_clicked[button] < click_now - 1000) {
          SP("button ");
          SP(button);
          SPL(" discarded");
        // this click happened more than a second ago and we never did anything with it -- discard
        last_clicked[button] = 0;
        last_released[button] = 1;
      }
    }
  }
  yield();
}

// returns the amount of time a pin was last in the LOW position.
int16_t clicked(uint8_t button) {
  //SP("checking button ");
  //SPL(button);
  if (last_clicked[button] > 0 && last_released[button] > last_clicked[button]) {
    int16_t held = last_released[button] - last_clicked[button] ;
    // reset these values so we don't ever count the same click twice.
    last_clicked[button] = 0;
    last_released[button] = 1;
    return held;
  }
  //SP("done checking button ");
  //SPL(button);
  return 0;
}
