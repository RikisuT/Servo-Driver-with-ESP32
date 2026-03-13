#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "webpage.h"


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
      json += ",\"range\":" + String(s ? s->full_range() : (int)ServoDigitalRange);
      json += ",\"middle\":" + String(s ? s->full_range() / 2 : (int)ServoDigitalMiddle);
      json += ",\"hasCurrent\":" + String(s && s->current_supported() ? "true" : "false");
      String sname = get_servo_name(id);
      json += ",\"name\":\"";
      // Escape quotes in name for JSON safety
      for (unsigned int ci = 0; ci < sname.length(); ci++) {
        char ch = sname.charAt(ci);
        if (ch == '"' || ch == '\\') json += '\\';
        json += ch;
      }
      json += "\"";
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
      json += ",\"range\":" + String(s ? s->full_range() : (int)ServoDigitalRange);
      json += ",\"alarm\":" + String(alarmRead[id]);
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

    int range = servos[id]->full_range();
    pos = constrain(pos, 0, range);
    speed = constrain(speed, 1, (int)ServoMaxSpeed);
    acc = constrain(acc, 0, 255);

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
      xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
      servoForId(id)->set_motor_speed(0);
      xSemaphoreGive(servo_bus_mutex);
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

  server.on("/api/torque_limit", [](){
    if(!server.hasArg("id")){
      server.send(400, "application/json", "{\"error\":\"missing id\"}");
      return;
    }
    int id = server.arg("id").toInt();
    if(id < 0 || id > 252 || !servos[id]){
      server.send(404, "application/json", "{\"error\":\"servo not found\"}");
      return;
    }
    if(servos[id]->type() != ServoBusApi::ServoType::STS){
      server.send(400, "application/json", "{\"error\":\"torque limit only supported on STS servos\"}");
      return;
    }
    auto* sts = static_cast<STSServo*>(servos[id]);
    if(server.hasArg("value")){
      int val = server.arg("value").toInt();
      if(val < 0) val = 0;
      if(val > 1023) val = 1023;
      xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
      sts->set_torque_limit((uint16_t)val);
      xSemaphoreGive(servo_bus_mutex);
      server.send(200, "application/json", "{\"ok\":true}");
    } else {
      xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
      uint16_t val = sts->read_torque_limit();
      xSemaphoreGive(servo_bus_mutex);
      server.send(200, "application/json", "{\"value\":" + String(val) + "}");
    }
  });

  server.on("/api/angle_limits", [](){
    if(!server.hasArg("id")){
      server.send(400, "application/json", "{\"error\":\"missing id\"}");
      return;
    }
    int id = server.arg("id").toInt();
    if(id < 0 || id > 252 || !servos[id]){
      server.send(404, "application/json", "{\"error\":\"servo not found\"}");
      return;
    }
    if(server.hasArg("min") && server.hasArg("max")){
      int minVal = server.arg("min").toInt();
      int maxVal = server.arg("max").toInt();
      int range = servos[id]->full_range();
      if(minVal < 0 || maxVal < 0 || minVal > range || maxVal > range || minVal >= maxVal){
        server.send(400, "application/json", "{\"error\":\"invalid angle limits\"}");
        return;
      }
      xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
      bool ok = servos[id]->write_angle_limits((uint16_t)minVal, (uint16_t)maxVal);
      xSemaphoreGive(servo_bus_mutex);
      if(ok){
        server.send(200, "application/json", "{\"ok\":true}");
      } else {
        server.send(500, "application/json", "{\"error\":\"write failed\"}");
      }
    } else {
      xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
      auto limits = servos[id]->read_angle_limits();
      xSemaphoreGive(servo_bus_mutex);
      if(limits){
        String json = "{\"min\":" + String(limits->min_angle) + ",\"max\":" + String(limits->max_angle) + "}";
        server.send(200, "application/json", json);
      } else {
        server.send(500, "application/json", "{\"error\":\"read failed\"}");
      }
    }
  });

  server.on("/api/set_id", [](){
    if(!server.hasArg("id") || !server.hasArg("new_id")){
      server.send(400, "application/json", "{\"error\":\"missing id or new_id\"}");
      return;
    }
    int id = server.arg("id").toInt();
    int newId = server.arg("new_id").toInt();
    if(id < 0 || id > 252 || !servos[id]){
      server.send(404, "application/json", "{\"error\":\"servo not found\"}");
      return;
    }
    if(newId < 0 || newId > 252){
      server.send(400, "application/json", "{\"error\":\"new_id out of range\"}");
      return;
    }
    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    bool ok = servos[id]->set_id((uint8_t)newId);
    xSemaphoreGive(servo_bus_mutex);
    if(ok){
      // Move servo pointer to new ID slot
      servos[newId] = servos[id];
      servos[id] = nullptr;
      // Migrate servo name to new ID
      migrate_servo_name((uint8_t)id, (uint8_t)newId);
      // Update listID
      for(int i = 0; i < searchNum; i++){
        if(listID[i] == id){ listID[i] = newId; break; }
      }
      server.send(200, "application/json", "{\"ok\":true,\"new_id\":" + String(newId) + "}");
    } else {
      server.send(500, "application/json", "{\"error\":\"set_id failed\"}");
    }
  });

  server.on("/api/safety", [](){
    if(!server.hasArg("id")){
      server.send(400, "application/json", "{\"error\":\"missing id\"}");
      return;
    }
    int id = server.arg("id").toInt();
    if(id < 0 || id > 252 || !servos[id]){
      server.send(404, "application/json", "{\"error\":\"servo not found\"}");
      return;
    }
    bool is_sts = servos[id]->type() == ServoBusApi::ServoType::STS;
    if(server.hasArg("max_temp")){
      bool ok = false;
      if(is_sts){
        auto* sts = static_cast<STSServo*>(servos[id]);
        STSServo::SafetyConfig cfg;
        cfg.max_temp = constrain(server.arg("max_temp").toInt(), 0, 255);
        cfg.max_voltage = constrain(server.arg("max_voltage").toInt(), 0, 255);
        cfg.min_voltage = constrain(server.arg("min_voltage").toInt(), 0, 255);
        cfg.max_torque = constrain(server.arg("max_torque").toInt(), 0, 1000);
        cfg.protection_current = constrain(server.arg("prot_current").toInt(), 0, 511);
        cfg.protective_torque = constrain(server.arg("prot_torque").toInt(), 0, 100);
        cfg.protection_time = constrain(server.arg("prot_time").toInt(), 0, 254);
        cfg.overload_torque = constrain(server.arg("overload_torque").toInt(), 0, 100);
        cfg.overcurrent_prot_time = constrain(server.arg("overcurrent_time").toInt(), 0, 254);
        cfg.unload_conditions = constrain(server.arg("unload").toInt(), 0, 63);
        cfg.led_alarm_conditions = constrain(server.arg("led_alarm").toInt(), 0, 63);
        xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
        ok = sts->write_safety_config(cfg);
        xSemaphoreGive(servo_bus_mutex);
      } else {
        auto* sc = static_cast<SCServo*>(servos[id]);
        SCServo::SafetyConfig cfg;
        cfg.max_temp = constrain(server.arg("max_temp").toInt(), 0, 100);
        cfg.max_voltage = constrain(server.arg("max_voltage").toInt(), 0, 254);
        cfg.min_voltage = constrain(server.arg("min_voltage").toInt(), 0, 254);
        cfg.max_torque = constrain(server.arg("max_torque").toInt(), 0, 1000);
        cfg.protective_torque = constrain(server.arg("prot_torque").toInt(), 0, 100);
        cfg.protection_time = constrain(server.arg("prot_time").toInt(), 0, 254);
        cfg.overload_torque = constrain(server.arg("overload_torque").toInt(), 0, 100);
        cfg.unload_conditions = constrain(server.arg("unload").toInt(), 0, 37);
        cfg.led_alarm_conditions = constrain(server.arg("led_alarm").toInt(), 0, 37);
        xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
        ok = sc->write_safety_config(cfg);
        xSemaphoreGive(servo_bus_mutex);
      }
      if(ok) server.send(200, "application/json", "{\"ok\":true}");
      else server.send(500, "application/json", "{\"error\":\"write failed\"}");
    } else {
      String json = "{";
      if(is_sts){
        auto* sts = static_cast<STSServo*>(servos[id]);
        xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
        auto cfg = sts->read_safety_config();
        xSemaphoreGive(servo_bus_mutex);
        if(!cfg){ server.send(500, "application/json", "{\"error\":\"read failed\"}"); return; }
        json += "\"max_temp\":" + String(cfg->max_temp);
        json += ",\"max_voltage\":" + String(cfg->max_voltage);
        json += ",\"min_voltage\":" + String(cfg->min_voltage);
        json += ",\"max_torque\":" + String(cfg->max_torque);
        json += ",\"prot_current\":" + String(cfg->protection_current);
        json += ",\"prot_torque\":" + String(cfg->protective_torque);
        json += ",\"prot_time\":" + String(cfg->protection_time);
        json += ",\"overload_torque\":" + String(cfg->overload_torque);
        json += ",\"overcurrent_time\":" + String(cfg->overcurrent_prot_time);
        json += ",\"unload\":" + String(cfg->unload_conditions);
        json += ",\"led_alarm\":" + String(cfg->led_alarm_conditions);
      } else {
        auto* sc = static_cast<SCServo*>(servos[id]);
        xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
        auto cfg = sc->read_safety_config();
        xSemaphoreGive(servo_bus_mutex);
        if(!cfg){ server.send(500, "application/json", "{\"error\":\"read failed\"}"); return; }
        json += "\"max_temp\":" + String(cfg->max_temp);
        json += ",\"max_voltage\":" + String(cfg->max_voltage);
        json += ",\"min_voltage\":" + String(cfg->min_voltage);
        json += ",\"max_torque\":" + String(cfg->max_torque);
        json += ",\"prot_torque\":" + String(cfg->protective_torque);
        json += ",\"prot_time\":" + String(cfg->protection_time);
        json += ",\"overload_torque\":" + String(cfg->overload_torque);
        json += ",\"unload\":" + String(cfg->unload_conditions);
        json += ",\"led_alarm\":" + String(cfg->led_alarm_conditions);
      }
      json += "}";
      server.send(200, "application/json", json);
    }
  });

  server.on("/api/set_name", [](){
    if(!server.hasArg("id")){
      server.send(400, "application/json", "{\"error\":\"missing id\"}");
      return;
    }
    int id = server.arg("id").toInt();
    if(id < 0 || id > 252){
      server.send(400, "application/json", "{\"error\":\"id out of range\"}");
      return;
    }
    if(server.hasArg("name")){
      String name = server.arg("name");
      if(name.length() > 20) name = name.substring(0, 20);
      set_servo_name((uint8_t)id, name);
      server.send(200, "application/json", "{\"ok\":true}");
    } else {
      String name = get_servo_name((uint8_t)id);
      server.send(200, "application/json", "{\"name\":\"" + name + "\"}");
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
  setSTA();
}


