
// settings
#undef CEREAL

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
#define MODES           2
#define BUTTONS         10


// oled
#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
#define SCREEN_ADDRESS  0x3C
#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define BMP_HEIGHT      16
#define BMP_WIDTH       16
#define OLED_CHAR_SIZE  8

#define BLUETOOTH
#include <PGardLib.h>
#include <RFClickerLib.h>
#include "oled.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoBLE.h>
#include <Scheduler.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

volatile uint32_t now = millis();
volatile uint32_t last_clicked[BUTTONS];
volatile uint32_t last_released[BUTTONS];
void setup() {
  PGardLibSetup();
  //arrayFill(0, button_history, HISTORY_LENGTH*2);
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
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    SPL(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.setRotation(2);
  

  if (!BLE.begin()) {
    SPL("starting BLE failed!");
    while (1);
  }
  //BLE.setTimeout(1);
  BLE.scanForUuid(BUTTONSVC_UUID);
  
  arrayFill(0, last_clicked, BUTTONS);
  arrayFill(1, last_released, BUTTONS);
  Scheduler.startLoop(clickLoop);
  //Scheduler.startLoop(blueLoop);
}

bool connected = false;
String device_name = "";
int mode = 0;
BLEDevice peripherals[8];


//BLEDevice peripheral;

void loop() {
  now = millis();
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
      if (peripheral) {
        arrayUnshift(peripheral, peripherals, 8);
      }
      if (clicked(BTMENU)) {
        SPL("switching to controller mode");
        BLE.scanForUuid(BUTTONSVC_UUID);
        mode = 0;
      }
      else {
        for (int i = 0; i< 4; i++) {
          if (peripherals[i]) {
            oled_menu[i] = peripherals[i].deviceName();
          }
          else {
            oled_menu[i] = "";
          }
        }
        // hardware tests
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
}

void blueLoop() {
  BLEDevice peripheral = BLE.available();
  if (mode == 0) {
    peripheral = BLE.available();
  }
  yield();
}

void controller(BLEDevice peripheral) {
  // connect to the peripheral
  SP("Connecting to ");
  SP(peripheral.localName());
  SP("...");
  if (peripheral.connect()) {
    SPL("connected");
  } else {
    SPL("failed");
    return;
  }

  // discover peripheral attributes
  SPL("Discovering service...");
  if (peripheral.discoverService(BUTTONSVC_UUID)) {
    SPL("discovered");
  } else {
    SPL("failed");
    peripheral.disconnect();
    // Why?
    // while (1);
    return;
  }

  // retrieve the simple key characteristic
  BLECharacteristic buttonChar = peripheral.characteristic(BUTTONCHAR_UUID);
  
  if (!buttonChar) {
    SPL("no button characteristic found!");
    peripheral.disconnect();
    return;
  }
  /* 
   *  since we're only writing to these -- not reading -- do we need to subscribe?
  SPL("subscribing...");
  if (!buttonChar.canSubscribe()) {
    SPL("not subscribeable");
    peripheral.disconnect();
    return;
  } 
  else if (!buttonChar.subscribe()) {
    SPL("failed");
    peripheral.disconnect();
    return;
  } 
  else {
    SPL("subscribed");
  }
  */
  
  BLECharacteristic menuChar = peripheral.characteristic(MENUCHAR_UUID);
  if (menuChar && menuChar.canRead() && menuChar.canSubscribe()) {
    updateOledMenu(BLE.central(), menuChar);
    SP("subscribing to menu...");
    if (menuChar.subscribe()) {
      SPL("subscribed");
      menuChar.setEventHandler(BLEWritten, updateOledMenu);
    }
  }
  
  connected = true;
  device_name = peripheral.localName();
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
void clickLoop() {
  now = millis();
  for (int button = 0; button < BUTTONS; button++) {
    if (digitalRead(button) == LOW) {
      if (last_clicked[button] < last_released[button]) {
        //SP("button ");
        //SP(button);
        //SPL(" clicked");
        last_clicked[button] = now;
      }
    }
    else { // button == HIGH
      if (last_clicked[button] > last_released[button]) {
        if (last_released[button] - last_clicked[button] > BOUNCE_TIMEOUT) {
          //SP("button ");
          //SP(button);
          //SPL(" released");
          last_released[button] = now;
        }
        else {
          last_clicked[button] = 0;
          last_released[button] = 1;
        }
      }
      else if (last_clicked[button] > 0 && last_released[button] > last_clicked[button] && last_clicked[button] < now - 1000) {
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
  now = millis();
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
