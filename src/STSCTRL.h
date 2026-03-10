#include "sc_servo.h"
#include "sts_servo.h"
#include <math.h>

// servo_bus is the global ServoBusApi instance (defined at end of servo_bus_api.h)

// Compatibility typedefs (were in old SCServo INST.h)
typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;


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
int   MAX_MIN_OFFSET = 30;

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


// set the servo ID list.
byte ID_List[253];
bool Torque_List[253];

// Feedback buffers indexed by servo ID
s16  loadRead[253];
s16  speedRead[253];
byte voltageRead[253];
int  currentRead[253];
s16  posRead[253];
s16  modeRead[253];
s16  temperRead[253];

// Active servo list & search state
byte listID[253];
byte searchNum = 0;
bool searchedStatus = false;
bool searchFinished = false;
bool searchCmd      = false;
byte activeNumInList = 0;
s16 activeServoSpeed = 100;
byte servotoSet = 0;

// linkageBuffer to save the angle.
float linkageBuffer[50];


// ----------- Core functions -----------

void servoWritePosEx(byte id, s16 position, u16 speed, u8 acc) {
  auto* s = servoForId(id);
  if (s->type() == ServoBusApi::ServoType::STS && acc > 0 && s->info_loaded()) {
    static_cast<STSServo*>(s)->move_to_encoder_angle_with_accel(
        (uint16_t)abs(position) | (position < 0 ? (1 << 15) : 0), speed, acc);
  } else {
    servo_bus.set_servo_type(s->type());
    servo_bus.write_position(id, (uint16_t)position, 0, speed);
  }
}

void servoWritePos(byte id, u16 position, u16 time_ms, u16 speed) {
  auto* s = servoForId(id);
  servo_bus.set_servo_type(s->type());
  servo_bus.write_position(id, position, time_ms, speed);
}


void getFeedBack(byte servoID) {
  auto* s = servoForId(servoID);

  auto pos = s->read_encoder_angle();
  if (!pos) return;
  posRead[servoID] = *pos;

  auto spd = s->read_speed();
  if (spd) speedRead[servoID] = *spd;

  auto load = s->read_load();
  if (load) loadRead[servoID] = *load;

  auto volt = s->read_voltage();
  if (volt) voltageRead[servoID] = (byte)(*volt * 10);  // store as raw (tenths of volts)

  auto temp = s->read_temperature();
  if (temp) temperRead[servoID] = (s16)*temp;

  auto cur = s->read_current();
  if (cur) currentRead[servoID] = *cur;

  auto mode = s->read_mode();
  if (mode) modeRead[servoID] = *mode;

  auto torq = s->is_torque_enabled();
  if (torq) Torque_List[servoID] = *torq;
}


void servoInit() {
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
    servo_bus.set_servo_type(s->type());
    servo_bus.set_offset(InputID, 0);
  }
}


void setMode(byte InputID, byte InputMode) {
  auto* s = servoForId(InputID);
  if (InputMode == 0) {
    s->restore_position_mode();
  } else if (InputMode == 3) {
    s->enable_motor_mode();
  }
}


void setID(byte ID_select, byte ID_set) {
  if (ID_set > MAX_ID) { MAX_ID = ID_set; }
  servo_bus.set_servo_type(active_servo->type());
  servo_bus.set_servo_id_permanent(ID_select, ID_set);
}


void servoStop(byte servoID) {
  auto* s = servoForId(servoID);
  s->disable_torque();
  delay(10);
  s->enable_torque();
}


void servoTorque(byte servoID, u8 enableCMD) {
  auto* s = servoForId(servoID);
  if (enableCMD) {
    s->enable_torque();
  } else {
    s->disable_torque();
  }
}