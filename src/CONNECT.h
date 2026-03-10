// https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>
#include "WEBPAGE.h"


// Create AsyncWebServer object on port 80
WebServer server(80);


// select the ID of active servo.
void activeID(int cmdInput){
  activeNumInList += cmdInput;
  if(activeNumInList >= searchNum){
    activeNumInList = 0;
  }
  else if(activeNumInList < 0){
    activeNumInList = searchNum;
  }
}


void activeSpeed(int cmdInput){
  activeServoSpeed += cmdInput;
  if (activeServoSpeed > ServoMaxSpeed){
    activeServoSpeed = ServoMaxSpeed;
  }
  else if(activeServoSpeed < 0){
    activeServoSpeed = 0;
  }
}


int rangeCtrl(int rawInput, int minInput, int maxInput){
  if(rawInput > maxInput){
    return maxInput;
  }
  else if(rawInput < minInput){
    return minInput;
  }
  else{
    return rawInput;
  }
}


void activeCtrl(int cmdInput){
  byte id = listID[activeNumInList];
  int mode = modeRead[id];
  Serial.printf("activeCtrl: cmd=%d mode=%d servoType=%d\n", cmdInput, mode, SERVO_TYPE_SELECT);

  switch(cmdInput){
    case 1:  // Middle
      servoWritePosEx(id, ServoDigitalMiddle, activeServoSpeed, ServoInitACC);
      break;
    case 2:  // Stop
      if(mode == 0) {
        servoStop(id);
      } else if(mode == 3) {
        servoForId(id)->set_motor_speed(0);
      }
      break;
    case 3:  // Release (torque off)
      servoTorque(id, 0);
      Torque_List[activeNumInList] = false;
      break;
    case 4:  // Torque on
      servoTorque(id, 1);
      Torque_List[activeNumInList] = true;
      break;
    case 5:  // Position+ / Motor CW
      if(mode == 0) {
        servoWritePosEx(id, ServoDigitalRange - MAX_MIN_OFFSET, activeServoSpeed, ServoInitACC);
      } else if(mode == 3) {
        servoForId(id)->set_motor_speed(activeServoSpeed);
      }
      break;
    case 6:  // Position- / Motor CCW
      if(mode == 0) {
        servoWritePosEx(id, MAX_MIN_OFFSET, activeServoSpeed, ServoInitACC);
      } else if(mode == 3) {
        servoForId(id)->set_motor_speed(-activeServoSpeed);
      }
      break;
    case 7:activeSpeed(100);break;
    case 8:activeSpeed(-100);break;
    case 9:servotoSet += 1;if(servotoSet > 250){servotoSet = 0;}break;
    case 10:servotoSet -= 1;if(servotoSet < 0){servotoSet = 0;}break;
    case 11:setMiddle(id);break;
    case 12:setMode(id, 0);break;
    case 13:setMode(id, 3);break;
    case 14:SERIAL_FORWARDING = true;break;
    case 15:SERIAL_FORWARDING = false;break;
    case 16:setID(id, servotoSet);break;

    case 17:DEV_ROLE = 0;break;
    case 18:DEV_ROLE = 1;break;
    case 19:DEV_ROLE = 2;break;

    case 20:RAINBOW_STATUS = 1;break;
    case 21:RAINBOW_STATUS = 0;break;
  }
}


void handleRoot() {
 server.send(200, "text/html", index_html); //Send web page
}


void handleID() {
  if(!searchedStatus && searchFinished){
    String IDmessage = "ID:";
    for(int i = 0; i< searchNum; i++){
      IDmessage += String(listID[i]) + " ";
    }
    server.send(200, "text/plane", IDmessage);
  }
  else if(searchedStatus){
    String IDmessage = "Searching...";
    server.send(200, "text/plane", IDmessage);
  }
}


void handleSTS() {
  String stsValue = "Active ID:" + String(listID[activeNumInList]);
  if(voltageRead[listID[activeNumInList]] != -1){
    stsValue += "  Position:" + String(posRead[listID[activeNumInList]]);
    if(DEV_ROLE == 0){
      stsValue += "<p>Device Mode: Normal";
    }
    else if(DEV_ROLE == 1){
      stsValue += "<p>Device Mode: Leader";
    }
    else if(DEV_ROLE == 2){
      stsValue += "<p>Device Mode: Follower";
    }
    stsValue += "<p>Voltage:" + String(float(voltageRead[listID[activeNumInList]])/10);
    stsValue += "  Load:" + String(loadRead[listID[activeNumInList]]);
    stsValue += "<p>Speed:" + String(speedRead[listID[activeNumInList]]);

    stsValue += "  Temper:" + String(temperRead[listID[activeNumInList]]);
    stsValue += "<p>Speed Set:" + String(activeServoSpeed);
    stsValue += "<p>ID to Set:" + String(servotoSet);
    stsValue += "<p>Mode:";
    if(modeRead[listID[activeNumInList]] == 0){
      stsValue += "Servo Mode";
    }
    else if(modeRead[listID[activeNumInList]] == 3){
      stsValue += "Motor Mode";
    }

    if(Torque_List[activeNumInList]){
      stsValue += "<p>Torque On";
    }
    else{
      stsValue += "<p>Torque Off";
    }
  }
  else{
    stsValue += " FeedBack err";
  }
  server.send(200, "text/plane", stsValue); //Send ADC value only to client ajax request
}


void webCtrlServer(){
    server.on("/", handleRoot);
    server.on("/readID", handleID);
    server.on("/readSTS", handleSTS);

    server.on("/cmd", [](){
    int cmdT = server.arg(0).toInt();
    int cmdI = server.arg(1).toInt();
    int cmdA = server.arg(2).toInt();
    int cmdB = server.arg(3).toInt();

    switch(cmdT){
      case 0:activeID(cmdI);break;
      case 1:activeCtrl(cmdI);break;
      case 9:searchCmd = true;break;
    }
  });

  // Start server
  server.begin();
  Serial.println("Server Starts.");
}


void webServerSetup(){
  webCtrlServer();
}


void getMAC(){
  WiFi.mode(WIFI_AP_STA);
  MAC_ADDRESS = WiFi.macAddress();
  Serial.print("MAC:");
  Serial.println(WiFi.macAddress());
}


void getIP(){
  IP_ADDRESS = WiFi.localIP();
}


void setAP(){
  WiFi.softAP(AP_SSID, AP_PWD);
  IPAddress myIP = WiFi.softAPIP();
  IP_ADDRESS = myIP;
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  WIFI_MODE = 1;
}


void setSTA(){
  WIFI_MODE = 3;
  Serial.print("Connecting to ");
  Serial.println(STA_SSID);
  WiFi.begin(STA_SSID, STA_PWD);

  // Wait up to 10 seconds for connection.
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    WIFI_MODE = 2;
    getIP();
    Serial.print("Connected to ");
    Serial.println(STA_SSID);
    Serial.print("IP address: ");
    Serial.println(IP_ADDRESS);
  } else {
    Serial.println("STA connection failed, falling back to AP mode.");
    WiFi.disconnect();
    setAP();
  }
}


void getWifiStatus(){
  if(WiFi.status() == WL_CONNECTED){
    WIFI_MODE = 2;
    getIP();
    WIFI_RSSI = WiFi.RSSI();
  }
  else if(WiFi.status() == WL_CONNECTION_LOST && WIFI_MODE == 2){
    WIFI_MODE = 3;
    WiFi.reconnect();
  }
}


void wifiInit(){
  DEV_ROLE = DEFAULT_ROLE;
  setSTA();
}


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if(DEV_ROLE == 2){
    memcpy(&myData, incomingData, sizeof(myData));
    myData.Spd_send = abs(myData.Spd_send);
    if(myData.Spd_send < 50){
      myData.Spd_send = 200;
    }
    servoWritePosEx(myData.ID_send, myData.POS_send, abs(myData.Spd_send), 0);

    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("POS: ");
    Serial.println(myData.POS_send);
    Serial.print("SPEED: ");
    Serial.println(abs(myData.Spd_send));
  }
}


void espNowInit(){
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  MAC_ADDRESS = WiFi.macAddress();
  Serial.print("MAC:");
  Serial.println(WiFi.macAddress());
}