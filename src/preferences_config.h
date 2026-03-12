#pragma once
#include <Preferences.h>

Preferences preferences;

void preferencesSetup(){
  preferences.begin("servo", false);
}

String get_servo_name(uint8_t id) {
  String key = "name_" + String(id);
  return preferences.getString(key.c_str(), "");
}

void set_servo_name(uint8_t id, const String& name) {
  String key = "name_" + String(id);
  if (name.length() == 0) {
    preferences.remove(key.c_str());
  } else {
    preferences.putString(key.c_str(), name);
  }
}

void migrate_servo_name(uint8_t old_id, uint8_t new_id) {
  String name = get_servo_name(old_id);
  if (name.length() > 0) {
    set_servo_name(new_id, name);
    set_servo_name(old_id, "");
  }
}
