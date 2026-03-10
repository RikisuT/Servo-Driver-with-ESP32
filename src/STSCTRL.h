#include "servo_bus_api.h"
#include <math.h>

// servo_bus is the global ServoBusApi instance (defined at end of servo_bus_api.h)

// Compatibility typedefs (were in old SCServo INST.h)
typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;


// === SC Servo defaults (matches original config) ===
float ServoDigitalRange = 1023.0;
float ServoAngleRange   = 210.0;
float ServoDigitalMiddle= 511.0;
#define ServoInitACC      0
#define ServoMaxSpeed     1500
#define MaxSpeed_X        1500
#define ServoInitSpeed    1500
int SERVO_TYPE_SELECT = 2;    // 1=STS, 2=SC
int MAX_MIN_OFFSET = 30;

// === ST Servo settings (uncomment to switch default) ===
// float ServoDigitalRange = 4095.0;
// float ServoAngleRange   = 360.0;
// float ServoDigitalMiddle= 2047.0;
// #define ServoInitACC      100
// #define ServoMaxSpeed     4000
// #define MaxSpeed_X        4000
// #define ServoInitSpeed    2000
// int SERVO_TYPE_SELECT = 1;


// Helper: get the ServoBusApi::ServoType from the global SERVO_TYPE_SELECT
inline ServoBusApi::ServoType currentServoType() {
  return (SERVO_TYPE_SELECT == 1) ? ServoBusApi::ServoType::STS : ServoBusApi::ServoType::SC;
}

// Helper: get the correct LOCK register for the current servo type
inline ServoBusApi::Register currentLockRegister() {
  return (SERVO_TYPE_SELECT == 1) ? ServoBusApi::Register::lock_sts : ServoBusApi::Register::lock_sc;
}


// set the servo ID list.
byte ID_List[253];
bool Torque_List[253];

// []: the ID of the servo.
// the buffer of the information read from the active servo.
s16  loadRead[253];
s16  speedRead[253];
byte voltageRead[253];
int  currentRead[253];
s16  posRead[253];
s16  modeRead[253];
s16  temperRead[253];

// []: the num of the active servo.
// use listID[activeNumInList] to get the ID of the active servo.
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

// the buffer of the bytes read from USB-C and servos. 
int usbRead;
int stsRead;


// ----------- Compatibility wrappers for old st.WritePosEx / st.WritePos -----------

// Replaces st.WritePosEx(ID, Position, Speed, ACC)
// For SC servos: ACC is ignored (set to 0), Time is 0.
// For STS servos with ACC > 0: uses write_position_sts_with_accel.
void servoWritePosEx(byte id, s16 position, u16 speed, u8 acc) {
  Serial.printf("servoWritePosEx: id=%d pos=%d speed=%d acc=%d type=%d\n", id, position, speed, acc);
  servo_bus.set_servo_type(currentServoType());
  if (SERVO_TYPE_SELECT == 1 && acc > 0) {
    // STS with acceleration: handle sign bit for direction
    uint16_t pos = (uint16_t)position;
    if (position < 0) {
      pos = (uint16_t)(-position) | (1 << 15);
    }
    servo_bus.write_position_sts_with_accel(id, pos, speed, acc);
  } else {
    // SC, or STS without acceleration
    bool ok = servo_bus.write_position(id, (uint16_t)position, 0, speed);
    Serial.printf("  write_position result=%d error=%d\n", ok, (int)servo_bus.last_error());
  }
}

// Replaces st.WritePos(ID, Position, Time, Speed)
void servoWritePos(byte id, u16 position, u16 time_ms, u16 speed) {
  Serial.printf("servoWritePos: id=%d pos=%d time=%d speed=%d\n", id, position, time_ms, speed);
  servo_bus.set_servo_type(currentServoType());
  servo_bus.write_position(id, position, time_ms, speed);
}


// ----------- Core functions (same signatures as original) -----------

void getFeedBack(byte servoID){
  servo_bus.set_servo_type(currentServoType());

  auto pos = servo_bus.read_position(servoID);
  if(pos){
    posRead[servoID] = *pos;

    auto spd = servo_bus.read_speed(servoID);
    if(spd) speedRead[servoID] = *spd;

    auto load = servo_bus.read_load(servoID);
    if(load) loadRead[servoID] = *load;

    auto volt = servo_bus.read_voltage(servoID);
    if(volt) voltageRead[servoID] = *volt;

    auto temp = servo_bus.read_temperature(servoID);
    if(temp) temperRead[servoID] = *temp;

    // Current: read 2 bytes from present_current registers
    auto cur_bytes = servo_bus.read_register(servoID,
        ServoBusApi::Register::present_current_l, 2);
    if(cur_bytes){
      // Unpack 16-bit value using correct byte order
      int16_t cur;
      if(SERVO_TYPE_SELECT == 1){
        // STS: little-endian (low byte first)
        cur = (int16_t)(cur_bytes[0] | (cur_bytes[1] << 8));
      } else {
        // SC: big-endian (high byte first)
        cur = (int16_t)((cur_bytes[0] << 8) | cur_bytes[1]);
      }
      if(cur & (1 << 15)) cur = -(cur & ~(1 << 15));
      currentRead[servoID] = cur;
    }

    // Mode detection:
    // SC servos: mode is inferred from angle limits (both 0 → motor mode 3)
    // STS servos: read MODE register directly
    if(SERVO_TYPE_SELECT == 2){
      auto limits = servo_bus.read_angle_limits(servoID);
      if(limits){
        modeRead[servoID] = (limits->min_angle == 0 && limits->max_angle == 0) ? 3 : 0;
      }
    } else {
      auto mode_val = servo_bus.read_byte(servoID, ServoBusApi::Register::mode);
      if(mode_val) modeRead[servoID] = *mode_val;
    }
  }else{
    // Serial.println("FeedBack err");
  }
}


void servoInit(){
  Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
  servo_bus.set_serial(&Serial1);
  servo_bus.set_echo_enabled(false);    // Waveshare board: HW TX/RX isolation
  servo_bus.set_servo_type(currentServoType());
  while(!Serial1) {}

  for (int i = 0; i < MAX_ID; i++){
    Torque_List[i] = true;
  }
}


void setMiddle(byte InputID){
  // CalibrationOfs was a no-op for SC servos (returned -1).
  // For STS servos: reset the offset calibration to 0.
  if(SERVO_TYPE_SELECT == 1){
    servo_bus.set_servo_type(ServoBusApi::ServoType::STS);
    servo_bus.set_offset(InputID, 0);
  }
}


void setMode(byte InputID, byte InputMode){
  servo_bus.set_servo_type(currentServoType());

  // Unlock EEPROM
  servo_bus.write_byte(InputID, currentLockRegister(), 0);

  if(InputMode == 0){
    // Restore servo (position) mode
    if(SERVO_TYPE_SELECT == 1){
      // STS: set max angle to 4095, mode register to 0
      uint8_t angle_params[3];
      angle_params[0] = ServoBusApi::to_byte(ServoBusApi::Register::max_angle_limit_l);
      angle_params[1] = 4095 & 0xFF;          // low byte (LE)
      angle_params[2] = (4095 >> 8) & 0xFF;   // high byte
      servo_bus.send_command(InputID, ServoBusApi::Instruction::write, angle_params, 3);
      uint8_t resp[ServoBusApi::MIN_PACKET_SIZE];
      servo_bus.read_response(resp, ServoBusApi::MIN_PACKET_SIZE);

      servo_bus.write_byte(InputID, ServoBusApi::Register::mode, 0);
    }
    else if(SERVO_TYPE_SELECT == 2){
      // SC: restore angle limits (20..1003)
      uint8_t params[5];
      params[0] = ServoBusApi::to_byte(ServoBusApi::Register::min_angle_limit_l);
      // SC is big-endian: high byte first
      params[1] = (20 >> 8) & 0xFF;    params[2] = 20 & 0xFF;
      params[3] = (1003 >> 8) & 0xFF;  params[4] = 1003 & 0xFF;
      servo_bus.send_command(InputID, ServoBusApi::Instruction::write, params, 5);
      uint8_t resp[ServoBusApi::MIN_PACKET_SIZE];
      servo_bus.read_response(resp, ServoBusApi::MIN_PACKET_SIZE);
    }
  }
  else if(InputMode == 3){
    // Motor / PWM mode
    if(SERVO_TYPE_SELECT == 1){
      // STS: set mode to 3, max angle to 0
      servo_bus.write_byte(InputID, ServoBusApi::Register::mode, 3);

      uint8_t angle_params[3];
      angle_params[0] = ServoBusApi::to_byte(ServoBusApi::Register::max_angle_limit_l);
      angle_params[1] = 0;  angle_params[2] = 0;
      servo_bus.send_command(InputID, ServoBusApi::Instruction::write, angle_params, 3);
      uint8_t resp[ServoBusApi::MIN_PACKET_SIZE];
      servo_bus.read_response(resp, ServoBusApi::MIN_PACKET_SIZE);
    }
    else if(SERVO_TYPE_SELECT == 2){
      // SC: set both angle limits to 0
      uint8_t params[5];
      params[0] = ServoBusApi::to_byte(ServoBusApi::Register::min_angle_limit_l);
      params[1] = 0; params[2] = 0; params[3] = 0; params[4] = 0;
      servo_bus.send_command(InputID, ServoBusApi::Instruction::write, params, 5);
      uint8_t resp[ServoBusApi::MIN_PACKET_SIZE];
      servo_bus.read_response(resp, ServoBusApi::MIN_PACKET_SIZE);
    }
  }

  // Lock EEPROM
  servo_bus.write_byte(InputID, currentLockRegister(), 1);
}


void setID(byte ID_select, byte ID_set){
  if(ID_set > MAX_ID){ MAX_ID = ID_set; }
  servo_bus.set_servo_type(currentServoType());
  servo_bus.set_servo_id_permanent(ID_select, ID_set);
}


void servoStop(byte servoID){
  servo_bus.set_servo_type(currentServoType());
  servo_bus.disable_torque(servoID);
  delay(10);
  servo_bus.enable_torque(servoID);
}


void servoTorque(byte servoID, u8 enableCMD){
  servo_bus.set_servo_type(currentServoType());
  if(enableCMD){
    servo_bus.enable_torque(servoID);
  } else {
    servo_bus.disable_torque(servoID);
  }
}