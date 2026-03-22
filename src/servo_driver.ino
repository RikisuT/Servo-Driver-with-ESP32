static constexpr int S_RXD = 18;
static constexpr int S_TXD = 19;

static constexpr int S_SCL = 22;
static constexpr int S_SDA = 21;

static constexpr int RGB_LED   = 23;
static constexpr int NUMPIXELS = 10;
static constexpr int HOST_SERIAL_BAUD = 500000;

int MAX_ID = 20;

String MAC_ADDRESS;
byte   SERVO_NUMBER;

static constexpr int threadingInterval = 50;
static constexpr int clientInterval    = 1;

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#define CONTROL_CORE 0
#define TELEMETRY_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#define CONTROL_CORE 1
#define TELEMETRY_CORE 0
#endif

#include "rgb_ctrl.h"
#include "preferences_config.h"
#include "sts_ctrl.h"
#include "serial_bridge.h"
#include "board_dev.h"


void setup() {
  Serial.begin(HOST_SERIAL_BAUD);
  while(!Serial) {}
  
  boardDevInit();

  preferencesSetup();

  RGBcolor(0, 64, 255);

  servoInit();

  RGBoff();

  delay(1000);
  pingAll(true);

  threadInit();
}


void loop() {
  delay(300000);
}
