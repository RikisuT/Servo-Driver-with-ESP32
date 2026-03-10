#include <Wire.h>
TaskHandle_t ScreenUpdateHandle;
TaskHandle_t ClientCmdHandle;

// SSD1306: 0x3C
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels, 32 as default.
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


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
  display.println(IP_ADDRESS);
  // Row2.
  display.print(float(voltageRead[listID[activeNumInList]])/10);display.print(F("V"));
  display.print(F(" "));
  if(WIFI_MODE == 1){display.println(AP_SSID);}
  else if(WIFI_MODE == 2){display.println(STA_SSID);}
  else if(WIFI_MODE == 3){display.print(F("TRY:"));display.println(STA_SSID);}
  // Row3.
  display.print(F("WIFI:"));

  if(WIFI_MODE == 1){display.print(F(" AP "));display.println(AP_SSID);}
  else if(WIFI_MODE == 2){display.print(F(" STA "));display.println(STA_SSID);}
  else if(WIFI_MODE == 3){display.print(F(" TRY:"));display.print(STA_SSID);display.println(F(""));}

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
    for(int i = 0; i <= MAX_ID; i++){
      // Either servo type works for ping
      servo_bus.set_servo_type(ServoBusApi::ServoType::STS);
      auto PingResult = servo_bus.ping(i);

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

      if(PingResult.has_value()){
        // Infer servo type (SC vs STS) and create typed object
        auto inferredType = Servo::infer_servo_type(&servo_bus, i);
        if(inferredType){
          if(*inferredType == ServoBusApi::ServoType::SC){
            servos[i] = new SCServo(&servo_bus, i);
          } else {
            servos[i] = new STSServo(&servo_bus, i);
          }
        } else {
          // Fallback to configured default if inference fails
          Serial.printf("Servo %d: type inference failed, using default\n", i);
          if(SERVO_TYPE_SELECT == 1){
            servos[i] = new STSServo(&servo_bus, i);
          } else {
            servos[i] = new SCServo(&servo_bus, i);
          }
        }
        servos[i]->read_info();  // Load angle limits from EEPROM

        const char* typeStr = (servos[i]->type() == ServoBusApi::ServoType::STS) ? "STS" : "SC";
        Serial.printf("Servo %d: %s (range %d-%d)\n", i, typeStr,
                       servos[i]->min_encoder_angle(), servos[i]->max_encoder_angle());

        listID[searchNum] = i;
        searchNum++;
      }
    }
    for(int i = 0; i < searchNum; i++){
      Serial.print(listID[i]);Serial.print(" ");
    }
    Serial.println();
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
  unsigned long lastScreen = 0;
  while(1){
    // Handle rescan requests from the web UI
    if(searchCmd){
      pingAll(true);
    }

    // Poll telemetry for all detected servos
    for(int i = 0; i < searchNum; i++){
      getFeedBack(listID[i]);
    }
    // Update screen/wifi at a slower rate (every 500ms)
    unsigned long now = millis();
    if(now - lastScreen > 500){
      getWifiStatus();
      screenUpdate();
      lastScreen = now;
    }
    delay(threadingInterval);
  }
}


void clientThreading(void *pvParameter){
  while(1){
    server.handleClient();
    delay(clientInterval);
  }
}


void threadInit(){
  xTaskCreatePinnedToCore(&InfoUpdateThreading, "InfoUpdate", 8192, NULL, 5, &ScreenUpdateHandle, ARDUINO_RUNNING_CORE);
  xTaskCreate(&clientThreading, "Client", 8192, NULL, 5, &ClientCmdHandle);
}