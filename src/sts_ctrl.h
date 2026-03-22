#pragma once
#include "sc_servo.h"
#include "sts_servo.h"
#include <math.h>

// Mutex protecting all servo_bus access (Serial1).
// Must be taken before any read/write to servo_bus or Servo* methods.
SemaphoreHandle_t servo_bus_mutex = nullptr;

// === Servo configuration ===
// Set to 1 for STS servos, 2 for SC servos
int SERVO_TYPE_SELECT = 2;    // 1=STS, 2=SC

// Derived constants (set in servoInit based on SERVO_TYPE_SELECT)
float ServoDigitalRange;
float ServoAngleRange;
float ServoDigitalMiddle;
int   ServoInitACC;
int   ServoMaxSpeed;
int   MaxSpeed_X;
int   ServoInitSpeed;

// Per-servo polymorphic pointers, allocated during scan.
// servos[id] is non-null for detected servos, null otherwise.
Servo* servos[253] = {nullptr};

// Fallback instances for servos not yet typed (before scan completes)
SCServo  sc_servo_fallback(&servo_bus, 0);
STSServo sts_servo_fallback(&servo_bus, 0);

// active_servo: default type for un-typed operations
Servo*   active_servo = nullptr;

// Get the Servo* for a given ID.
// Returns the typed servo from the servos[] array if available,
// otherwise falls back to the global default with the given ID.
inline Servo* servoForId(uint8_t id) {
  if (servos[id]) return servos[id];
  // Fallback for untyped servos (before scan or if inference failed)
  if (SERVO_TYPE_SELECT == 1) {
    sts_servo_fallback = STSServo(&servo_bus, id);
    return &sts_servo_fallback;
  } else {
    sc_servo_fallback = SCServo(&servo_bus, id);
    return &sc_servo_fallback;
  }
}


bool Torque_List[253];

// Feedback buffers indexed by servo ID
int16_t  loadRead[253];
int16_t  speedRead[253];
byte     voltageRead[253];
int      currentRead[253];
int16_t  posRead[253];
int16_t  goalRead[253];
int16_t  modeRead[253];
int16_t  temperRead[253];
uint8_t  alarmRead[253];

// Active servo list & search state
byte listID[253];
byte searchNum = 0;
bool searchedStatus = false;
bool searchFinished = false;
bool searchCmd      = false;
byte activeNumInList = 0;
int16_t activeServoSpeed = 100;
bool feedback_include_speed = false;


// ----------- Core functions -----------

void servoWritePosEx(byte id, uint16_t position, uint16_t speed, uint8_t acc) {
  auto* s = servoForId(id);
  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  if (s->type() == ServoBusApi::ServoType::STS && acc > 0 && s->info_loaded()) {
    static_cast<STSServo*>(s)->move_to_encoder_angle_with_accel(position, speed, acc);
  } else {
    servo_bus.set_servo_type(s->type());
    servo_bus.write_position(id, position, 0, speed);
  }
  xSemaphoreGive(servo_bus_mutex);
}

void servoWritePos(byte id, uint16_t position, uint16_t time_ms, uint16_t speed) {
  auto* s = servoForId(id);
  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  servo_bus.set_servo_type(s->type());
  servo_bus.write_position(id, position, time_ms, speed);
  xSemaphoreGive(servo_bus_mutex);
}


void getFeedBack(byte servoID) {
  auto* s = servoForId(servoID);

  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  auto pos = s->read_encoder_angle();
  if (!pos) { xSemaphoreGive(servo_bus_mutex); return; }
  posRead[servoID] = *pos;

  auto goal = s->read_goal_position();
  if (goal) goalRead[servoID] = *goal;

  if (feedback_include_speed) {
    auto spd = s->read_speed();
    if (spd) speedRead[servoID] = *spd;
  }

  auto load = s->read_load();
  if (load) loadRead[servoID] = *load;

  auto volt = s->read_voltage();
  if (volt) voltageRead[servoID] = (byte)(*volt * 10);  // store as raw (tenths of volts)

  auto temp = s->read_temperature();
  if (temp) temperRead[servoID] = (int16_t)*temp;

  auto cur = s->read_current();
  if (cur) currentRead[servoID] = *cur;

  auto mode = s->read_mode();
  if (mode) modeRead[servoID] = *mode;

  auto torq = s->is_torque_enabled();
  if (torq) Torque_List[servoID] = *torq;

  auto alarm = s->read_alarm_status();
  if (alarm) alarmRead[servoID] = *alarm;
  xSemaphoreGive(servo_bus_mutex);
}


void getFeedBackFast(byte servoID) {
  auto* s = servoForId(servoID);

  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  auto pos = s->read_encoder_angle();
  if (pos) posRead[servoID] = *pos;

  if (feedback_include_speed) {
    auto spd = s->read_speed();
    if (spd) speedRead[servoID] = *spd;
  }
  xSemaphoreGive(servo_bus_mutex);
}


void servoInit() {
  servo_bus_mutex = xSemaphoreCreateMutex();

  Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
  servo_bus.set_serial(&Serial1);
  servo_bus.set_echo_enabled(false);    // Waveshare board: HW TX/RX isolation
  while (!Serial1) {}

  // Configure type-specific constants
  if (SERVO_TYPE_SELECT == 1) {
    ServoDigitalRange  = 4095.0;
    ServoAngleRange    = 360.0;
    ServoDigitalMiddle = 2047.0;
    ServoInitACC       = 100;
    ServoMaxSpeed      = 4000;
    MaxSpeed_X         = 4000;
    ServoInitSpeed     = 2000;
    active_servo = &sts_servo_fallback;
    servo_bus.set_servo_type(ServoBusApi::ServoType::STS);
  } else {
    ServoDigitalRange  = 1023.0;
    ServoAngleRange    = 210.0;
    ServoDigitalMiddle = 511.0;
    ServoInitACC       = 0;
    ServoMaxSpeed      = 1500;
    MaxSpeed_X         = 1500;
    ServoInitSpeed     = 1500;
    active_servo = &sc_servo_fallback;
    servo_bus.set_servo_type(ServoBusApi::ServoType::SC);
  }

  for (int i = 0; i < 253; i++) {
    Torque_List[i] = true;
  }
}


void setMiddle(byte InputID) {
  if (SERVO_TYPE_SELECT == 1) {
    auto* s = servoForId(InputID);
    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    servo_bus.set_servo_type(s->type());
    servo_bus.set_offset(InputID, 0);
    xSemaphoreGive(servo_bus_mutex);
  }
}


void setMode(byte InputID, byte InputMode) {
  auto* s = servoForId(InputID);
  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  if (InputMode == 0) {
    s->restore_position_mode();
  } else if (InputMode == 3) {
    s->enable_motor_mode();
  }
  xSemaphoreGive(servo_bus_mutex);
}


void setID(byte ID_select, byte ID_set) {
  if (ID_set > MAX_ID) { MAX_ID = ID_set; }
  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  servo_bus.set_servo_type(active_servo->type());
  servo_bus.set_servo_id_permanent(ID_select, ID_set);
  xSemaphoreGive(servo_bus_mutex);
}


void servoStop(byte servoID) {
  auto* s = servoForId(servoID);
  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  s->disable_torque();
  delay(10);
  s->enable_torque();
  xSemaphoreGive(servo_bus_mutex);
}


void servoTorque(byte servoID, uint8_t enableCMD) {
  auto* s = servoForId(servoID);
  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  if (enableCMD) {
    s->enable_torque();
  } else {
    s->disable_torque();
  }
  xSemaphoreGive(servo_bus_mutex);
}