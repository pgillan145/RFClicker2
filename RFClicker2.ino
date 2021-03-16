
// settings
#define CEREAL

// buttons
#define RIGHT           2
#define DOWN            3
#define LEFT            4
#define UP              5  

#define HOLD_TIMEOUT    300   // how long a button needs to be held down before it triggers an input and resets
#define BOUNCE_TIMEOUT  35   // the minimum time a button needs to be held down to count as a click
#define SEND_BLINK_TIME 100  // how long the connection light brightness should jump when data is "sent".

// oled
#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
#define SCREEN_ADDRESS  0x3C
#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define BMP_HEIGHT      16
#define BMP_WIDTH       16
#define OLED_CHAR_SIZE  8

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#include <RFClickerLib.h>
#include <ArduinoBLE.h>

BLEService buttonService("1101");
BLECharacteristic buttonChar("2101", BLERead | BLENotify, HISTORY_LENGTH*2);
//char button_history[HISTORY_LENGTH*2];

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("setup()");
  //arrayFill(0, button_history, HISTORY_LENGTH*2);
  pinMode(RIGHT, INPUT_PULLDOWN);
  pinMode(DOWN, INPUT_PULLDOWN);
  pinMode(LEFT, INPUT_PULLDOWN);  
  pinMode(UP, INPUT_PULLDOWN);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.setRotation(2);

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }
  BLE.scanForUuid("1101");
}

byte button_status = 0;
byte last_button_status = 0;
uint8_t count = 0;
int32_t right_millis = 0;
bool right_held = false;
int32_t down_millis = 0;
bool down_held = false;
int32_t left_millis = 0;
bool left_held = false;
int32_t up_millis = 0;
bool up_held = false;
bool connected = false;
uint32_t now = 0;
uint32_t last_send = 0;
String device_name = "";

void loop() {
  now = millis();
  BLEDevice peripheral = BLE.available();
  if (peripheral) {
    BLE.stopScan();
    controller(peripheral);
    BLE.scanForUuid("1101");
  }
  OledStatus();
}

void controller(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");
  if (peripheral.connect()) {
    Serial.print("Connected to ");
    Serial.println(peripheral.deviceName());
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering service 0x101 ...");
  if (peripheral.discoverService("1101")) {
    Serial.println("Service discovered");
  } else {
    Serial.println("Attribute discovery failed.");
    peripheral.disconnect();

    // Why?
    // while (1);
    return;
  }

  // retrieve the simple key characteristic
  BLECharacteristic buttonChar = peripheral.characteristic("2101");

  // subscribe to the simple key characteristic
  Serial.println("Subscribing to button characteristic ...");
  if (!buttonChar) {
    Serial.println("no button characteristic found!");
    peripheral.disconnect();
    return;
  } else if (!buttonChar.canSubscribe()) {
    Serial.println("button characteristic is not subscribable!");
    peripheral.disconnect();
    return;
  } else if (!buttonChar.subscribe()) {
    Serial.println("subscription failed!");
    peripheral.disconnect();
    return;
  } else {
    Serial.println("Subscribed");
  }
  //count = 0;
  //arrayFill(0, button_history, HISTORY_LENGTH*2);
  //buttonChar.writeValue(button_history, HISTORY_LENGTH*2, true);
  
  connected = true;
  device_name = peripheral.deviceName();
  OledStatus("CONNECTED");

  while (peripheral.connected()) {
    now = millis();
    if (digitalRead(RIGHT) == HIGH) {
        OledStatus("button1", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON1);
    }
    else {
      CLRBUTTON(button_status, BUTTON1);
    }
    if (digitalRead(DOWN) == HIGH) {
        OledStatus("button2", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON2);
    }
    else {
      CLRBUTTON(button_status, BUTTON2);
    }

    if (digitalRead(LEFT) == HIGH) {
        OledStatus("button3", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON3);
    }
    else {
      CLRBUTTON(button_status, BUTTON3);
    }
    
    if (digitalRead(UP) == HIGH) {
        OledStatus("button4", SEND_BLINK_TIME);
        ADDBUTTON(button_status, BUTTON4);
    }
    else {
      CLRBUTTON(button_status, BUTTON4);
    }

    if (button_status != last_button_status) {
      buttonChar.writeValue(button_status);
      last_button_status = button_status;
    }
      
/*
    if (button_status != button_history[(HISTORY_LENGTH*2)-1]) {
      count++;
      if (count == 0) {
        count = 1;
      }
      arrayPush(count, button_history, HISTORY_LENGTH*2);
      arrayPush(button_status, button_history, HISTORY_LENGTH*2);
      last_send = now;
      //buttonChar.writeValue(button_history, HISTORY_LENGTH*2, true);
    }
    if (now - last_send > SEND_BLINK_TIME) {
      //analogWrite(CONNECTED, 128);
    }
    */
    
    OledStatus();
  } // peripheral.connected()
  Serial.println("disconnected");
  connected = false;
  device_name = "";
  OledStatus("DISCONNECTED");
  return;
} // controller
