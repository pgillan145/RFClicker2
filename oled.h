static const unsigned char PROGMEM BT_bmp[] = {
 0b00000001,0b00000000,
 0b00000001,0b10000000,
 0b00100001,0b01000000,
 0b00010001,0b00100000,
 0b00001001,0b00010000,
 0b00000101,0b00100000,
 0b00000011,0b01000000,
 0b00000001,0b10000000,
 0b00000001,0b10000000,
 0b00000011,0b01000000,
 0b00000101,0b00100000,
 0b00001001,0b00010000,
 0b00010001,0b00100000,
 0b00100001,0b01000000,
 0b00000001,0b10000000,
 0b00000001,0b00000000 
};

String oled_menu[4];
uint32_t last_status = millis();
String last_status_str = "starting up";

void clearOledMenu();
void oledStatus();
void oledStatus(String text);
void oledStatus(String text, uint16_t status_ttl);
void updateOledMenu(BLEDevice central, BLECharacteristic menuChar);
