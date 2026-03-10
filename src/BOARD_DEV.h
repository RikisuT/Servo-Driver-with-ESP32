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
    searchNum = 0;
    searchedStatus = true;
    searchFinished = false;
    int PingStatus;
    for(int i = 0; i <= MAX_ID; i++){
      servo_bus.set_servo_type(active_servo->type());
      auto PingResult = servo_bus.ping(i);

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0,0);
      display.println(F("Searching Servos..."));
      display.print(F("MAX_ID "));display.print(MAX_ID);
      display.print(F("-Ping:"));display.println(i);
      display.print(F("Detected:"));

      for(int i = 0; i < searchNum; i++){
        display.print(listID[i]);display.print(F(" "));
      }
      display.display();

      if(PingResult.has_value()){
        listID[searchNum] = i;
        searchNum++;
      }
      // delay(1);
    }
    for(int i = 0; i< searchNum; i++){
      Serial.print(listID[i]);Serial.print(" ");
      Serial.println("");
    }
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
  while(1){
    getFeedBack(listID[activeNumInList]);
    getWifiStatus();
    screenUpdate();
    delay(threadingInterval);
    pingAll(searchCmd);
  }
}


void clientThreading(void *pvParameter){
  while(1){
    server.handleClient();
    delay(clientInterval);
  }
}


void threadInit(){
  xTaskCreatePinnedToCore(&InfoUpdateThreading, "InfoUpdate", 4000, NULL, 5, &ScreenUpdateHandle, ARDUINO_RUNNING_CORE);
  xTaskCreate(&clientThreading, "Client", 4000, NULL, 5, &ClientCmdHandle);
}