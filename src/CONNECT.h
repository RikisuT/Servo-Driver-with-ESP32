// https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
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
      Torque_List[id] = false;
      break;
    case 4:  // Torque on
      servoTorque(id, 1);
      Torque_List[id] = true;
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
    case 16:setID(id, servotoSet);break;
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

    if(Torque_List[listID[activeNumInList]]){
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

  // === JSON API endpoints ===

  server.on("/api/scan", [](){
    String json = "{\"servos\":[";
    for(int i = 0; i < searchNum; i++){
      if(i > 0) json += ",";
      uint8_t id = listID[i];
      Servo* s = servos[id];
      bool isSTS = s && (s->type() == ServoBusApi::ServoType::STS);
      json += "{\"id\":" + String(id);
      json += ",\"type\":\"" + String(isSTS ? "STS" : "SC") + "\"";
      json += ",\"range\":" + String(s ? s->max_encoder_angle() : (int)ServoDigitalRange);
      json += ",\"middle\":" + String(s ? s->max_encoder_angle() / 2 : (int)ServoDigitalMiddle);
      json += "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/api/status_all", [](){
    String json = "{\"servos\":[";
    for(int i = 0; i < searchNum; i++){
      if(i > 0) json += ",";
      uint8_t id = listID[i];
      Servo* s = servos[id];
      bool isSTS = s && (s->type() == ServoBusApi::ServoType::STS);
      json += "{\"id\":" + String(id);
      json += ",\"type\":\"" + String(isSTS ? "STS" : "SC") + "\"";
      json += ",\"pos\":" + String(posRead[id]);
      json += ",\"speed\":" + String(speedRead[id]);
      json += ",\"load\":" + String(loadRead[id]);
      json += ",\"voltage\":" + String(float(voltageRead[id]) / 10.0, 1);
      json += ",\"temp\":" + String(temperRead[id]);
      json += ",\"current\":" + String(currentRead[id]);
      json += ",\"mode\":" + String(modeRead[id]);
      json += ",\"torque\":" + String(Torque_List[id] ? "true" : "false");
      json += ",\"range\":" + String(s ? s->max_encoder_angle() : (int)ServoDigitalRange);
      json += "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/api/setpos", [](){
    if(!server.hasArg("id") || !server.hasArg("pos")){
      server.send(400, "application/json", "{\"error\":\"missing id or pos\"}");
      return;
    }
    int id = server.arg("id").toInt();
    int pos = server.arg("pos").toInt();
    int speed = server.hasArg("speed") ? server.arg("speed").toInt() : activeServoSpeed;
    int acc = server.hasArg("acc") ? server.arg("acc").toInt() : ServoInitACC;

    if(id < 0 || id > 252 || !servos[id]){
      server.send(404, "application/json", "{\"error\":\"servo not found\"}");
      return;
    }

    servoWritePosEx(id, pos, speed, acc);
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/rescan", [](){
    searchCmd = true;
    server.send(200, "application/json", "{\"ok\":true,\"message\":\"scan started\"}");
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
  setSTA();
}


