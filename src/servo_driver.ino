const char* AP_SSID = "ESP32_DEV";
const char* AP_PWD  = "12345678";

#include "wifi_credentials.h"

static constexpr int S_RXD = 18;
static constexpr int S_TXD = 19;

static constexpr int S_SCL = 22;
static constexpr int S_SDA = 21;

static constexpr int RGB_LED   = 23;
static constexpr int NUMPIXELS = 10;

int MAX_ID = 20;

String MAC_ADDRESS;
IPAddress IP_ADDRESS;
byte   SERVO_NUMBER;
byte   WIFI_MODE;
int    WIFI_RSSI;

static constexpr int threadingInterval = 50;
static constexpr int clientInterval    = 1;

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include "rgb_ctrl.h"
#include "sts_ctrl.h"
#include "connect.h"
#include "board_dev.h"


void setup() {
  Serial.begin(115200);
  while(!Serial) {}

  getMAC();
  
  boardDevInit();

  RGBcolor(0, 64, 255);

  servoInit();

  wifiInit();

  webServerSetup();

  RGBoff();

  delay(1000);
  pingAll(true);

  threadInit();
}


void loop() {
  delay(300000);
}
