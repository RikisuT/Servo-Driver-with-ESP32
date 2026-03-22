#pragma once
#include <Wire.h>
TaskHandle_t ScreenUpdateHandle;
TaskHandle_t ClientCmdHandle;
TaskHandle_t DisplayUpdateHandle;

// SSD1306
#include <Adafruit_SSD1306.h>
static constexpr int SCREEN_WIDTH   = 128;
static constexpr int SCREEN_HEIGHT  = 32;
static constexpr int OLED_RESET     = -1;
static constexpr int SCREEN_ADDRESS = 0x3C;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool g_scan_verbose = false;


void InitScreen(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.display();
}


void screenUpdate(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  // Row1.
  display.println(F("ST3215 Driver"));
  // Row2.
  if(searchNum){
    display.print(F("V:"));
    display.print(float(voltageRead[listID[activeNumInList]])/10);
    display.print(F(" ID:"));
    display.println(listID[activeNumInList]);
  } else {
    display.println(F("No servos"));
  }
  // Row3.
  display.print(F("SER:"));
  display.println(HOST_SERIAL_BAUD);

  // Row4.
  if(searchNum){
    display.print(F("N:"));display.print(searchNum);display.print(F(" ID:"));display.print(listID[activeNumInList]);
    display.print(F("-"));display.print(modeRead[listID[activeNumInList]]);
    display.print(F(" POS:"));display.println(posRead[listID[activeNumInList]]);
  }
  else{
    display.println(F("No servo detected."));
  }
  display.display();
}


void pingAll(bool searchCommand){
  if(searchCommand){
    RGBcolor(0, 255, 64);

    // Clean up old servo objects
    for(int i = 0; i < 253; i++){
      if(servos[i]){
        delete servos[i];
        servos[i] = nullptr;
      }
    }

    searchNum = 0;
    searchedStatus = true;
    searchFinished = false;
    unsigned long scanStart = millis();
    for(int i = 0; i <= MAX_ID; i++){
      // Either servo type works for ping
      xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
      servo_bus.set_servo_type(ServoBusApi::ServoType::STS);
      auto PingResult = servo_bus.ping(i);
      xSemaphoreGive(servo_bus_mutex);

      if(g_scan_verbose && ((i % 8) == 0 || i == MAX_ID)){
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println(F("Searching Servos..."));
        display.print(F("MAX_ID "));display.print(MAX_ID);
        display.print(F("-Ping:"));display.println(i);
        display.print(F("Detected:"));

        for(int j = 0; j < searchNum; j++){
          display.print(listID[j]);display.print(F(" "));
        }
        display.display();
      }

      if(PingResult.has_value()){
        // Infer servo type (SC vs STS) and create typed object
        xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
        auto inferredType = Servo::infer_servo_type(&servo_bus, i);
        if(inferredType){
          if(*inferredType == ServoBusApi::ServoType::SC){
            servos[i] = new SCServo(&servo_bus, i);
          } else {
            servos[i] = new STSServo(&servo_bus, i);
          }
        } else {
          // Fallback to configured default if inference fails
          if(g_scan_verbose){
            Serial.printf("Servo %d: type inference failed, using default\n", i);
          }
          if(SERVO_TYPE_SELECT == 1){
            servos[i] = new STSServo(&servo_bus, i);
          } else {
            servos[i] = new SCServo(&servo_bus, i);
          }
        }
        servos[i]->read_info();  // Load angle limits from EEPROM
        xSemaphoreGive(servo_bus_mutex);

        const char* typeStr = (servos[i]->type() == ServoBusApi::ServoType::STS) ? "STS" : "SC";
        if(g_scan_verbose){
          Serial.printf("Servo %d: %s (range %d-%d)\n", i, typeStr,
                         servos[i]->min_encoder_angle(), servos[i]->max_encoder_angle());
        }

        listID[searchNum] = i;
        searchNum++;
      }
    }
    if(g_scan_verbose){
      for(int i = 0; i < searchNum; i++){
        Serial.print(listID[i]);Serial.print(" ");
      }
      Serial.println();
    }
    Serial.printf("SCAN_DONE found=%u elapsed_ms=%lu\n", (unsigned)searchNum, (unsigned long)(millis() - scanStart));
    searchedStatus = false;
    searchFinished = true;
    searchCmd      = false;
    RGBoff();
  }
}


void boardDevInit(){
    Wire.begin(S_SDA, S_SCL);
    InitScreen();
    InitRGB();
}




void InfoUpdateThreading(void *pvParameter){
  unsigned long lastFastPoll = 0;
  unsigned long lastFullPoll = 0;
  int nextIndex = 0;
  while(1){
    // Handle rescan requests from serial commands
    if(searchCmd){
      pingAll(true);
      nextIndex = 0;
    }

    unsigned long now = millis();
    unsigned long sinceWriteMs = now - g_last_write_cmd_ms;
    unsigned long fastPeriodMs = (sinceWriteMs < 250) ? 40 : 20;

    // Poll one servo per cycle for low-latency command path.
    if(searchNum > 0 && (now - lastFastPoll) >= fastPeriodMs){
      if(nextIndex >= searchNum){
        nextIndex = 0;
      }
      getFeedBackFast(listID[nextIndex]);
      nextIndex++;
      lastFastPoll = now;
    }

    // Refresh full telemetry only when writes are idle to avoid long mutex stalls.
    if(searchNum > 0 && (now - lastFullPoll) >= 1000 && sinceWriteMs >= 250){
      int idx = activeNumInList;
      if(idx >= searchNum){
        idx = 0;
      }
      getFeedBack(listID[idx]);
      lastFullPoll = now;
    }

    delay(threadingInterval);
  }
}


void displayThreading(void *pvParameter){
  while(1){
    screenUpdate();
    delay(500);
  }
}


void clientThreading(void *pvParameter){
  while(1){
    serialBridgeLoop();
    delay(clientInterval);
  }
}


void threadInit(){
  xTaskCreatePinnedToCore(&InfoUpdateThreading, "InfoUpdate", 8192, NULL, 5, &ScreenUpdateHandle, TELEMETRY_CORE);
  xTaskCreatePinnedToCore(&clientThreading, "Client", 8192, NULL, 6, &ClientCmdHandle, CONTROL_CORE);
  xTaskCreatePinnedToCore(&displayThreading, "Display", 4096, NULL, 1, &DisplayUpdateHandle, TELEMETRY_CORE);
}