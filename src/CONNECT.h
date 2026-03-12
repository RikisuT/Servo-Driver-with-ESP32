// https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
#include <WiFi.h>
#include <WebServer.h>
#include "WEBPAGE.h"


WebServer server(80);


void handleRoot() {
  server.send(200, "text/html", index_html);
}


void webCtrlServer(){
  server.on("/", handleRoot);

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
      json += ",\"hasCurrent\":" + String(s && s->current_supported() ? "true" : "false");
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
      json += ",\"goal\":" + String(goalRead[id]);
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

  server.on("/api/scan_status", [](){
    String json = "{\"scanning\":";
    json += searchedStatus ? "true" : "false";
    json += ",\"finished\":";
    json += searchFinished ? "true" : "false";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/stop", [](){
    if(!server.hasArg("id")){
      server.send(400, "application/json", "{\"error\":\"missing id\"}");
      return;
    }
    int id = server.arg("id").toInt();
    if(id < 0 || id > 252 || !servos[id]){
      server.send(404, "application/json", "{\"error\":\"servo not found\"}");
      return;
    }
    int mode = modeRead[id];
    if(mode == 3) {
      servoForId(id)->set_motor_speed(0);
    } else {
      servoStop(id);
    }
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.on("/api/torque", [](){
    if(!server.hasArg("id") || !server.hasArg("enable")){
      server.send(400, "application/json", "{\"error\":\"missing id or enable\"}");
      return;
    }
    int id = server.arg("id").toInt();
    int enable = server.arg("enable").toInt();

    if(id < 0 || id > 252 || !servos[id]){
      server.send(404, "application/json", "{\"error\":\"servo not found\"}");
      return;
    }

    servoTorque(id, enable);
    Torque_List[id] = (enable != 0);
    server.send(200, "application/json", "{\"ok\":true}");
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


