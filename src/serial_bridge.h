#pragma once

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

static char g_serial_rx_buf[192];
static uint16_t g_serial_rx_len = 0;
static bool g_serial_rx_overflow = false;
static uint32_t g_serial_rx_overflow_count = 0;
static bool g_stream_enabled = false;
static uint32_t g_stream_period_ms = 100;
static uint32_t g_last_stream_ms = 0;
enum class TelemetryMode : uint8_t { POS = 0, POS_SPEED = 1 };
static TelemetryMode g_telem_mode = TelemetryMode::POS;

static constexpr uint8_t CMD_QUEUE_CAPACITY = 24;
extern bool g_scan_verbose;
static bool g_binary_enabled = true;
static uint32_t g_last_write_cmd_ms = 0;
static bool g_exec_mode_enabled = true;
static uint16_t g_exec_period_ms = 5;
static uint32_t g_last_exec_ms = 0;
static bool g_stage_dirty[253] = {false};
static uint16_t g_stage_pos[253] = {0};
static uint16_t g_stage_speed[253] = {0};
static uint8_t g_stage_acc[253] = {0};
static uint16_t g_stage_dirty_count = 0;

static constexpr uint8_t BIN_SOF = 0x7E;
static constexpr uint8_t BIN_MAX_PAYLOAD = 96;

enum class BinCmd : uint8_t {
  SET = 0x01,
  GETP = 0x02,
  PING = 0x03,
  STREAM = 0x04,
  SETN = 0x05,
  FASTCFG = 0x06,
};

enum class CommandKind : uint8_t {
  SET,
  GWRITE,
  GET,
  GETP,
  GETPS,
  SCAN,
  LIST,
  OTHER,
};

struct CommandQueue {
  char lines[CMD_QUEUE_CAPACITY][192];
  uint8_t head = 0;
  uint8_t tail = 0;
  uint8_t count = 0;
};

struct BridgePerfStats {
  uint32_t rx_lines = 0;
  uint32_t overflow_lines = 0;
  uint32_t queue_drops = 0;
  uint32_t enqueued_write = 0;
  uint32_t enqueued_misc = 0;
  uint32_t processed_write = 0;
  uint32_t processed_misc = 0;
  uint32_t queue_high_water_write = 0;
  uint32_t queue_high_water_misc = 0;
  uint32_t exec_us_last = 0;
  uint32_t exec_us_max = 0;
  uint32_t cmd_set = 0;
  uint32_t cmd_gwrite = 0;
  uint32_t cmd_get = 0;
  uint32_t cmd_getp = 0;
  uint32_t cmd_getps = 0;
  uint32_t cmd_scan = 0;
  uint32_t cmd_list = 0;
  uint32_t cmd_other = 0;
  uint32_t exec_us_max_set = 0;
  uint32_t exec_us_max_getp = 0;
  uint32_t exec_us_max_scan = 0;
  uint32_t exec_us_max_gwrite = 0;
  uint32_t bin_rx = 0;
  uint32_t bin_ok = 0;
  uint32_t bin_err_crc = 0;
  uint32_t bin_err_len = 0;
  uint32_t bin_set = 0;
  uint32_t bin_setn = 0;
  uint32_t bin_getp = 0;
  uint32_t bin_ping = 0;
  uint32_t bin_fastcfg = 0;
  uint32_t bin_set_us_max = 0;
  uint32_t bin_setn_us_max = 0;
  uint32_t bin_getp_us_max = 0;
  uint32_t bin_ping_us_max = 0;
  uint32_t staged_updates = 0;
  uint32_t staged_overwrites = 0;
  uint32_t staged_flush_cycles = 0;
  uint32_t staged_applied = 0;
  uint32_t staged_flush_us_max = 0;
};

static CommandQueue g_write_queue;
static CommandQueue g_misc_queue;
static BridgePerfStats g_perf;
static bool g_bin_set_noack = false;

struct BinRxState {
  bool active = false;
  uint8_t cmd = 0;
  uint8_t seq = 0;
  uint8_t len = 0;
  uint8_t idx = 0;
  uint8_t payload[BIN_MAX_PAYLOAD];
  uint8_t crc = 0;
};

static BinRxState g_bin_rx;

static uint8_t crc8_xor(uint8_t seed, uint8_t v) {
  return static_cast<uint8_t>(seed ^ v);
}

static uint16_t readLe16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

static void writeLe16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v & 0xFF);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}

static void sendBinFrame(uint8_t cmd, uint8_t seq, const uint8_t* payload, uint8_t len) {
  uint8_t crc = 0;
  crc = crc8_xor(crc, cmd);
  crc = crc8_xor(crc, seq);
  crc = crc8_xor(crc, len);
  for (uint8_t i = 0; i < len; i++) {
    crc = crc8_xor(crc, payload ? payload[i] : 0);
  }

  Serial.write(BIN_SOF);
  Serial.write(cmd);
  Serial.write(seq);
  Serial.write(len);
  if (len > 0 && payload) {
    Serial.write(payload, len);
  }
  Serial.write(crc);
}

static bool queuePush(CommandQueue& q, const char* line) {
  if (q.count >= CMD_QUEUE_CAPACITY) {
    return false;
  }
  strncpy(q.lines[q.tail], line, sizeof(q.lines[q.tail]) - 1);
  q.lines[q.tail][sizeof(q.lines[q.tail]) - 1] = '\0';
  q.tail = (q.tail + 1) % CMD_QUEUE_CAPACITY;
  q.count++;
  return true;
}

static bool queuePop(CommandQueue& q, char* out, size_t out_size) {
  if (q.count == 0 || out_size == 0) {
    return false;
  }
  strncpy(out, q.lines[q.head], out_size - 1);
  out[out_size - 1] = '\0';
  q.head = (q.head + 1) % CMD_QUEUE_CAPACITY;
  q.count--;
  return true;
}

static bool isWritePriorityCommand(const char* line) {
  if (!line || !line[0]) return false;

  char local[64];
  strncpy(local, line, sizeof(local) - 1);
  local[sizeof(local) - 1] = '\0';
  char* cmd = strtok(local, " \t");
  if (!cmd) return false;

  return
    strcasecmp(cmd, "SET") == 0 ||
    strcasecmp(cmd, "GWRITE") == 0 ||
    strcasecmp(cmd, "AWRITE") == 0 ||
    strcasecmp(cmd, "ACTION") == 0 ||
    strcasecmp(cmd, "TORQUE") == 0 ||
    strcasecmp(cmd, "STOP") == 0 ||
    strcasecmp(cmd, "MODE") == 0 ||
    strcasecmp(cmd, "MIDDLE") == 0 ||
    strcasecmp(cmd, "SETID") == 0 ||
    strcasecmp(cmd, "LIMITS") == 0 ||
    strcasecmp(cmd, "TORQUE_LIMIT") == 0 ||
    strcasecmp(cmd, "WHEEL") == 0 ||
    strcasecmp(cmd, "SCAN") == 0;
}

static CommandKind commandKindFromLine(const char* line) {
  if (!line || !line[0]) return CommandKind::OTHER;

  char local[64];
  strncpy(local, line, sizeof(local) - 1);
  local[sizeof(local) - 1] = '\0';
  char* cmd = strtok(local, " \t");
  if (!cmd) return CommandKind::OTHER;

  if (strcasecmp(cmd, "SET") == 0) return CommandKind::SET;
  if (strcasecmp(cmd, "GWRITE") == 0) return CommandKind::GWRITE;
  if (strcasecmp(cmd, "GET") == 0) return CommandKind::GET;
  if (strcasecmp(cmd, "GETP") == 0) return CommandKind::GETP;
  if (strcasecmp(cmd, "GETPS") == 0) return CommandKind::GETPS;
  if (strcasecmp(cmd, "SCAN") == 0) return CommandKind::SCAN;
  if (strcasecmp(cmd, "LIST") == 0) return CommandKind::LIST;
  return CommandKind::OTHER;
}

static void trackCommandPerf(CommandKind kind, uint32_t dt_us) {
  switch (kind) {
    case CommandKind::SET:
      g_perf.cmd_set++;
      if (dt_us > g_perf.exec_us_max_set) g_perf.exec_us_max_set = dt_us;
      break;
    case CommandKind::GWRITE:
      g_perf.cmd_gwrite++;
      if (dt_us > g_perf.exec_us_max_gwrite) g_perf.exec_us_max_gwrite = dt_us;
      break;
    case CommandKind::GET:
      g_perf.cmd_get++;
      break;
    case CommandKind::GETP:
      g_perf.cmd_getp++;
      if (dt_us > g_perf.exec_us_max_getp) g_perf.exec_us_max_getp = dt_us;
      break;
    case CommandKind::GETPS:
      g_perf.cmd_getps++;
      break;
    case CommandKind::SCAN:
      g_perf.cmd_scan++;
      if (dt_us > g_perf.exec_us_max_scan) g_perf.exec_us_max_scan = dt_us;
      break;
    case CommandKind::LIST:
      g_perf.cmd_list++;
      break;
    default:
      g_perf.cmd_other++;
      break;
  }
}

static bool isWriteKind(CommandKind kind) {
  return
    kind == CommandKind::SET ||
    kind == CommandKind::GWRITE ||
    kind == CommandKind::SCAN;
}

static bool enqueueCommandLine(const char* line) {
  bool write_priority = isWritePriorityCommand(line);
  CommandQueue& target = write_priority ? g_write_queue : g_misc_queue;
  bool ok = queuePush(target, line);
  if (!ok) {
    g_perf.queue_drops++;
    return false;
  }

  if (write_priority) {
    g_perf.enqueued_write++;
    if (target.count > g_perf.queue_high_water_write) {
      g_perf.queue_high_water_write = target.count;
    }
  } else {
    g_perf.enqueued_misc++;
    if (target.count > g_perf.queue_high_water_misc) {
      g_perf.queue_high_water_misc = target.count;
    }
  }
  return true;
}

static void trimInPlace(char* s) {
  if (!s) return;
  size_t len = strlen(s);
  size_t start = 0;
  while (start < len && (s[start] == ' ' || s[start] == '\t')) {
    start++;
  }
  size_t end = len;
  while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
    end--;
  }
  if (start > 0) {
    memmove(s, s + start, end - start);
  }
  s[end - start] = '\0';
}

static bool parseU16(const char* s, uint16_t& out) {
  if (!s) return false;
  char* endptr = nullptr;
  long v = strtol(s, &endptr, 10);
  if (endptr == s || *endptr != '\0' || v < 0 || v > 65535) return false;
  out = static_cast<uint16_t>(v);
  return true;
}

static bool parseU8(const char* s, uint8_t& out) {
  uint16_t tmp = 0;
  if (!parseU16(s, tmp) || tmp > 255) return false;
  out = static_cast<uint8_t>(tmp);
  return true;
}

static bool parseI16(const char* s, int16_t& out) {
  if (!s) return false;
  char* endptr = nullptr;
  long v = strtol(s, &endptr, 10);
  if (endptr == s || *endptr != '\0' || v < -32768 || v > 32767) return false;
  out = static_cast<int16_t>(v);
  return true;
}

static bool ensureServoExists(uint8_t id) {
  if (id > 252 || !servos[id]) {
    Serial.println("ERR id_not_found");
    return false;
  }
  return true;
}

static bool parseCsvU8(const char* s, std::vector<uint8_t>& out) {
  if (!s || !*s) return false;
  char local[128];
  strncpy(local, s, sizeof(local) - 1);
  local[sizeof(local) - 1] = '\0';

  char* tok = strtok(local, ",");
  while (tok) {
    uint8_t v;
    if (!parseU8(tok, v)) return false;
    out.push_back(v);
    tok = strtok(nullptr, ",");
  }
  return !out.empty();
}

static bool parseCsvU16(const char* s, std::vector<uint16_t>& out) {
  if (!s || !*s) return false;
  char local[128];
  strncpy(local, s, sizeof(local) - 1);
  local[sizeof(local) - 1] = '\0';

  char* tok = strtok(local, ",");
  while (tok) {
    uint16_t v;
    if (!parseU16(tok, v)) return false;
    out.push_back(v);
    tok = strtok(nullptr, ",");
  }
  return !out.empty();
}

static bool allSameServoType(const std::vector<uint8_t>& ids, ServoBusApi::ServoType& out_type) {
  if (ids.empty()) return false;
  if (!ensureServoExists(ids[0])) return false;
  out_type = servos[ids[0]]->type();
  for (size_t i = 1; i < ids.size(); i++) {
    if (!ensureServoExists(ids[i])) return false;
    if (servos[ids[i]]->type() != out_type) return false;
  }
  return true;
}

static void serialSendFeedback(uint8_t id) {
  if (id > 252 || !servos[id]) {
    Serial.printf("ERR id_not_found %u\n", id);
    return;
  }

  getFeedBack(id);
  Serial.printf(
    "FB id=%u pos=%d goal=%d speed=%d voltage=%.1f temp=%d current=%d torque=%u mode=%d alarm=%u\n",
    id,
    posRead[id],
    goalRead[id],
    speedRead[id],
    (float)voltageRead[id] / 10.0f,
    temperRead[id],
    currentRead[id],
    Torque_List[id] ? 1 : 0,
    modeRead[id],
    alarmRead[id]
  );
}

static bool readPosOnly(uint8_t id, int16_t& pos_out) {
  auto* s = servoForId(id);
  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  auto pos = s->read_encoder_angle();
  xSemaphoreGive(servo_bus_mutex);
  if (!pos) return false;
  pos_out = static_cast<int16_t>(*pos);
  posRead[id] = pos_out;
  return true;
}

static bool readPosSpeed(uint8_t id, int16_t& pos_out, int16_t& speed_out) {
  auto* s = servoForId(id);
  xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
  auto pos = s->read_encoder_angle();
  auto spd = s->read_speed();
  xSemaphoreGive(servo_bus_mutex);
  if (!pos) return false;
  pos_out = static_cast<int16_t>(*pos);
  posRead[id] = pos_out;
  if (spd) {
    speed_out = *spd;
    speedRead[id] = speed_out;
  }
  return true;
}

static void serialSendPosOnly(uint8_t id) {
  int16_t pos = 0;
  if (!readPosOnly(id, pos)) {
    Serial.printf("ERR pos_read_failed id=%u\n", id);
    return;
  }
  Serial.printf("FBP id=%u pos=%d\n", id, pos);
}

static void serialSendPosSpeed(uint8_t id) {
  int16_t pos = 0;
  int16_t spd = 0;
  if (!readPosSpeed(id, pos, spd)) {
    Serial.printf("ERR pos_read_failed id=%u\n", id);
    return;
  }
  Serial.printf("FBPS id=%u pos=%d speed=%d\n", id, pos, spd);
}

static bool stageSetCommand(uint8_t id, uint16_t pos, uint16_t speed, uint8_t acc, uint16_t* applied_pos = nullptr) {
  if (id > 252 || !servos[id]) {
    return false;
  }

  uint16_t range = servos[id]->full_range();
  if (pos > range) pos = range;
  if (speed > ServoMaxSpeed) speed = ServoMaxSpeed;

  if (applied_pos) {
    *applied_pos = pos;
  }

  if (!g_exec_mode_enabled) {
    servoWritePosEx(id, pos, speed, acc);
    g_last_write_cmd_ms = millis();
    return true;
  }

  if (g_stage_dirty[id]) {
    g_perf.staged_overwrites++;
  } else {
    g_stage_dirty[id] = true;
    g_stage_dirty_count++;
  }

  g_stage_pos[id] = pos;
  g_stage_speed[id] = speed;
  g_stage_acc[id] = acc;
  g_perf.staged_updates++;
  return true;
}

static void flushStagedSetCommandsIfDue() {
  if (!g_exec_mode_enabled || g_stage_dirty_count == 0) {
    return;
  }

  uint32_t now = millis();
  if ((now - g_last_exec_ms) < g_exec_period_ms) {
    return;
  }
  g_last_exec_ms = now;

  uint32_t t0 = micros();
  uint16_t applied_now = 0;
  for (uint16_t id = 0; id <= 252; id++) {
    if (!g_stage_dirty[id]) {
      continue;
    }
    g_stage_dirty[id] = false;
    if (g_stage_dirty_count > 0) {
      g_stage_dirty_count--;
    }
    servoWritePosEx(static_cast<uint8_t>(id), g_stage_pos[id], g_stage_speed[id], g_stage_acc[id]);
    applied_now++;
  }

  g_last_write_cmd_ms = now;
  g_perf.staged_flush_cycles++;
  g_perf.staged_applied += applied_now;
  uint32_t dt = micros() - t0;
  if (dt > g_perf.staged_flush_us_max) {
    g_perf.staged_flush_us_max = dt;
  }
}

static bool applySetCommand(uint8_t id, uint16_t pos, uint16_t speed, uint8_t acc, uint16_t* applied_pos = nullptr) {
  return stageSetCommand(id, pos, speed, acc, applied_pos);
}

static void handleBinaryCommand(uint8_t cmd, uint8_t seq, const uint8_t* payload, uint8_t len) {
  uint8_t rsp[16] = {0};

  if (cmd == static_cast<uint8_t>(BinCmd::SET)) {
    g_perf.bin_set++;
    if (len != 6) {
      rsp[0] = 2;
      sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
      return;
    }

    uint8_t id = payload[0];
    uint16_t pos = readLe16(payload + 1);
    uint16_t speed = readLe16(payload + 3);
    uint8_t acc = payload[5];

    uint16_t applied_pos = pos;
    if (!applySetCommand(id, pos, speed, acc, &applied_pos)) {
      rsp[0] = 3;
      sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
      return;
    }

    if (g_bin_set_noack) {
      return;
    }

    rsp[0] = 0;
    rsp[1] = id;
    writeLe16(rsp + 2, applied_pos);
    sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 4);
    return;
  }

  if (cmd == static_cast<uint8_t>(BinCmd::SETN)) {
    g_perf.bin_setn++;
    if (len == 0 || (len % 6) != 0) {
      rsp[0] = 2;
      sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
      return;
    }

    uint8_t applied = 0;
    uint8_t rejected = 0;
    const uint8_t count = static_cast<uint8_t>(len / 6);
    for (uint8_t i = 0; i < count; i++) {
      const uint8_t* item = payload + (i * 6);
      uint8_t id = item[0];
      uint16_t pos = readLe16(item + 1);
      uint16_t speed = readLe16(item + 3);
      uint8_t acc = item[5];
      if (applySetCommand(id, pos, speed, acc)) {
        applied++;
      } else {
        rejected++;
      }
    }

    if (g_bin_set_noack) {
      return;
    }

    rsp[0] = 0;
    rsp[1] = applied;
    rsp[2] = rejected;
    sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 3);
    return;
  }

  if (cmd == static_cast<uint8_t>(BinCmd::GETP)) {
    g_perf.bin_getp++;
    if (len != 1) {
      rsp[0] = 2;
      sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
      return;
    }

    uint8_t id = payload[0];
    int16_t pos = 0;
    if (id > 252 || !servos[id] || !readPosOnly(id, pos)) {
      rsp[0] = 3;
      sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
      return;
    }

    rsp[0] = 0;
    rsp[1] = id;
    writeLe16(rsp + 2, static_cast<uint16_t>(pos));
    sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 4);
    return;
  }

  if (cmd == static_cast<uint8_t>(BinCmd::PING)) {
    g_perf.bin_ping++;
    if (len != 1) {
      rsp[0] = 2;
      sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
      return;
    }
    uint8_t id = payload[0];
    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    auto res = servo_bus.ping(id);
    xSemaphoreGive(servo_bus_mutex);
    rsp[0] = res.has_value() ? 0 : 3;
    rsp[1] = id;
    sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 2);
    return;
  }

  if (cmd == static_cast<uint8_t>(BinCmd::STREAM)) {
    if (len != 3) {
      rsp[0] = 2;
      sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
      return;
    }
    g_stream_enabled = (payload[0] != 0);
    uint16_t period = readLe16(payload + 1);
    if (period >= 10) {
      g_stream_period_ms = period;
    }
    rsp[0] = 0;
    sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
    return;
  }

  if (cmd == static_cast<uint8_t>(BinCmd::FASTCFG)) {
    g_perf.bin_fastcfg++;
    if (len != 1) {
      rsp[0] = 2;
      sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
      return;
    }

    uint8_t flags = payload[0];
    g_bin_set_noack = (flags & 0x01) != 0;

    rsp[0] = 0;
    rsp[1] = g_bin_set_noack ? 1 : 0;
    sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 2);
    return;
  }

  rsp[0] = 1;
  sendBinFrame(static_cast<uint8_t>(cmd | 0x80), seq, rsp, 1);
}

static bool processBinaryByte(uint8_t b) {
  if (!g_binary_enabled) return false;

  if (!g_bin_rx.active) {
    if (b == BIN_SOF) {
      g_bin_rx.active = true;
      g_bin_rx.cmd = 0;
      g_bin_rx.seq = 0;
      g_bin_rx.len = 0;
      g_bin_rx.idx = 0;
      g_bin_rx.crc = 0;
      return true;
    }
    return false;
  }

  if (g_bin_rx.idx == 0) {
    g_bin_rx.cmd = b;
    g_bin_rx.crc = crc8_xor(g_bin_rx.crc, b);
    g_bin_rx.idx++;
    return true;
  }

  if (g_bin_rx.idx == 1) {
    g_bin_rx.seq = b;
    g_bin_rx.crc = crc8_xor(g_bin_rx.crc, b);
    g_bin_rx.idx++;
    return true;
  }

  if (g_bin_rx.idx == 2) {
    g_bin_rx.len = b;
    g_bin_rx.crc = crc8_xor(g_bin_rx.crc, b);
    if (g_bin_rx.len > BIN_MAX_PAYLOAD) {
      g_perf.bin_err_len++;
      g_bin_rx.active = false;
      return true;
    }
    g_bin_rx.idx++;
    if (g_bin_rx.len == 0) {
      // Next byte will be CRC.
    }
    return true;
  }

  uint8_t payload_start_idx = 3;
  uint8_t payload_end_idx = static_cast<uint8_t>(payload_start_idx + g_bin_rx.len);
  if (g_bin_rx.idx >= payload_start_idx && g_bin_rx.idx < payload_end_idx) {
    uint8_t pi = static_cast<uint8_t>(g_bin_rx.idx - payload_start_idx);
    g_bin_rx.payload[pi] = b;
    g_bin_rx.crc = crc8_xor(g_bin_rx.crc, b);
    g_bin_rx.idx++;
    return true;
  }

  // CRC byte
  g_perf.bin_rx++;
  if (b != g_bin_rx.crc) {
    g_perf.bin_err_crc++;
    g_bin_rx.active = false;
    return true;
  }

  g_perf.bin_ok++;
  uint32_t t0 = micros();
  handleBinaryCommand(g_bin_rx.cmd, g_bin_rx.seq, g_bin_rx.payload, g_bin_rx.len);
  uint32_t dt = micros() - t0;
  g_perf.exec_us_last = dt;
  if (dt > g_perf.exec_us_max) {
    g_perf.exec_us_max = dt;
  }
  if (g_bin_rx.cmd == static_cast<uint8_t>(BinCmd::SET) && dt > g_perf.bin_set_us_max) {
    g_perf.bin_set_us_max = dt;
  } else if (g_bin_rx.cmd == static_cast<uint8_t>(BinCmd::SETN) && dt > g_perf.bin_setn_us_max) {
    g_perf.bin_setn_us_max = dt;
  } else if (g_bin_rx.cmd == static_cast<uint8_t>(BinCmd::GETP) && dt > g_perf.bin_getp_us_max) {
    g_perf.bin_getp_us_max = dt;
  } else if (g_bin_rx.cmd == static_cast<uint8_t>(BinCmd::PING) && dt > g_perf.bin_ping_us_max) {
    g_perf.bin_ping_us_max = dt;
  }
  g_bin_rx.active = false;
  return true;
}

static void handleSerialCommand(const char* line) {
  if (!line || !line[0]) {
    return;
  }

  char buf[192];
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  char* cmd = strtok(buf, " \t");
  if (!cmd) return;

  if (strcasecmp(cmd, "HELP") == 0) {
    Serial.println("OK commands: HELP, PERF [RESET], BIN <0|1>, BFAST <0|1>, EXEC <0|1> [period_ms], SCANVERBOSE <0|1>, SCAN, LIST, INFO <id>, GET <id>, GETP <id>, GETPS <id>, TMODE <POS|POSSPD>, TRATE <ms>, AREAD <id>, SET <id> <pos> [speed] [acc], GWRITE <ids_csv> <pos_csv> [time_ms] [speed], GREAD <ids_csv>, AWRITE <id> <reg> <value> [bytes], ACTION [id], TORQUE <id> <0|1>, STOP <id>, MODE <id> <0|3>, MIDDLE <id>, SETID <id> <new_id>, LIMITS <id> [min max], TORQUE_LIMIT <id> [value], WHEEL <id> <signed_speed>, STREAM <0|1> [ms], PING <id>");
    return;
  }

  if (strcasecmp(cmd, "BIN") == 0) {
    uint8_t en = 0;
    if (!parseU8(strtok(nullptr, " \t"), en)) {
      Serial.println("ERR usage BIN <0|1>");
      return;
    }
    g_binary_enabled = (en != 0);
    Serial.printf("OK bin=%u\n", g_binary_enabled ? 1 : 0);
    return;
  }

  if (strcasecmp(cmd, "PERF") == 0) {
    char* action = strtok(nullptr, " \t");
    if (action && strcasecmp(action, "RESET") == 0) {
      g_perf = BridgePerfStats{};
      g_serial_rx_overflow_count = 0;
      Serial.println("OK perf_reset");
      return;
    }

    Serial.printf(
      "PERF rx=%lu overflow=%lu qdrop=%lu wq=%u mq=%u wq_max=%lu mq_max=%lu enq_w=%lu enq_m=%lu proc_w=%lu proc_m=%lu exec_last_us=%lu exec_max_us=%lu\n",
      (unsigned long)g_perf.rx_lines,
      (unsigned long)g_perf.overflow_lines,
      (unsigned long)g_perf.queue_drops,
      (unsigned)g_write_queue.count,
      (unsigned)g_misc_queue.count,
      (unsigned long)g_perf.queue_high_water_write,
      (unsigned long)g_perf.queue_high_water_misc,
      (unsigned long)g_perf.enqueued_write,
      (unsigned long)g_perf.enqueued_misc,
      (unsigned long)g_perf.processed_write,
      (unsigned long)g_perf.processed_misc,
      (unsigned long)g_perf.exec_us_last,
      (unsigned long)g_perf.exec_us_max
    );
    Serial.printf(
      "PERF_CMD set=%lu gwrite=%lu get=%lu getp=%lu getps=%lu scan=%lu list=%lu other=%lu max_set_us=%lu max_getp_us=%lu max_scan_us=%lu max_gwrite_us=%lu bin_rx=%lu bin_ok=%lu bin_crc_err=%lu bin_len_err=%lu bin_set=%lu bin_setn=%lu bin_getp=%lu bin_ping=%lu bin_fastcfg=%lu bin_set_us_max=%lu bin_setn_us_max=%lu bin_getp_us_max=%lu bin_ping_us_max=%lu bin_noack=%u exec_mode=%u exec_period_ms=%u stage_q=%u stage_updates=%lu stage_overwrites=%lu stage_flush=%lu stage_applied=%lu stage_flush_us_max=%lu scan_verbose=%u\n",
      (unsigned long)g_perf.cmd_set,
      (unsigned long)g_perf.cmd_gwrite,
      (unsigned long)g_perf.cmd_get,
      (unsigned long)g_perf.cmd_getp,
      (unsigned long)g_perf.cmd_getps,
      (unsigned long)g_perf.cmd_scan,
      (unsigned long)g_perf.cmd_list,
      (unsigned long)g_perf.cmd_other,
      (unsigned long)g_perf.exec_us_max_set,
      (unsigned long)g_perf.exec_us_max_getp,
      (unsigned long)g_perf.exec_us_max_scan,
      (unsigned long)g_perf.exec_us_max_gwrite,
      (unsigned long)g_perf.bin_rx,
      (unsigned long)g_perf.bin_ok,
      (unsigned long)g_perf.bin_err_crc,
      (unsigned long)g_perf.bin_err_len,
      (unsigned long)g_perf.bin_set,
      (unsigned long)g_perf.bin_setn,
      (unsigned long)g_perf.bin_getp,
      (unsigned long)g_perf.bin_ping,
      (unsigned long)g_perf.bin_fastcfg,
      (unsigned long)g_perf.bin_set_us_max,
      (unsigned long)g_perf.bin_setn_us_max,
      (unsigned long)g_perf.bin_getp_us_max,
      (unsigned long)g_perf.bin_ping_us_max,
      g_bin_set_noack ? 1u : 0u,
      g_exec_mode_enabled ? 1u : 0u,
      (unsigned)g_exec_period_ms,
      (unsigned)g_stage_dirty_count,
      (unsigned long)g_perf.staged_updates,
      (unsigned long)g_perf.staged_overwrites,
      (unsigned long)g_perf.staged_flush_cycles,
      (unsigned long)g_perf.staged_applied,
      (unsigned long)g_perf.staged_flush_us_max,
      g_scan_verbose ? 1u : 0u
    );
    return;
  }

  if (strcasecmp(cmd, "EXEC") == 0) {
    uint8_t en = 0;
    if (!parseU8(strtok(nullptr, " \t"), en)) {
      Serial.println("ERR usage EXEC <0|1> [period_ms]");
      return;
    }

    char* p = strtok(nullptr, " \t");
    if (p) {
      uint16_t period = 0;
      if (!parseU16(p, period) || period < 1) {
        Serial.println("ERR invalid_exec_period_ms");
        return;
      }
      g_exec_period_ms = period;
    }

    g_exec_mode_enabled = (en != 0);
    if (!g_exec_mode_enabled) {
      // Flush pending staged updates immediately when disabling staged execution.
      for (uint16_t id = 0; id <= 252; id++) {
        if (!g_stage_dirty[id]) {
          continue;
        }
        g_stage_dirty[id] = false;
        servoWritePosEx(static_cast<uint8_t>(id), g_stage_pos[id], g_stage_speed[id], g_stage_acc[id]);
      }
      g_stage_dirty_count = 0;
      g_last_write_cmd_ms = millis();
    }

    Serial.printf("OK exec_mode=%u period_ms=%u\n", g_exec_mode_enabled ? 1 : 0, (unsigned)g_exec_period_ms);
    return;
  }

  if (strcasecmp(cmd, "BFAST") == 0) {
    uint8_t en = 0;
    if (!parseU8(strtok(nullptr, " \t"), en)) {
      Serial.println("ERR usage BFAST <0|1>");
      return;
    }
    g_bin_set_noack = (en != 0);
    Serial.printf("OK bfast=%u\n", g_bin_set_noack ? 1 : 0);
    return;
  }

  if (strcasecmp(cmd, "SCANVERBOSE") == 0) {
    uint8_t en = 0;
    if (!parseU8(strtok(nullptr, " \t"), en)) {
      Serial.println("ERR usage SCANVERBOSE <0|1>");
      return;
    }
    g_scan_verbose = (en != 0);
    Serial.printf("OK scan_verbose=%u\n", g_scan_verbose ? 1 : 0);
    return;
  }

  if (strcasecmp(cmd, "SCAN") == 0) {
    searchCmd = true;
    Serial.println("OK scan_started");
    return;
  }

  if (strcasecmp(cmd, "LIST") == 0) {
    Serial.print("OK ids=");
    for (int i = 0; i < searchNum; i++) {
      if (i) Serial.print(',');
      Serial.print(listID[i]);
    }
    Serial.println();
    return;
  }

  if (strcasecmp(cmd, "GET") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage GET <id>");
      return;
    }
    if (!ensureServoExists(id)) return;
    if (g_telem_mode == TelemetryMode::POS) {
      serialSendPosOnly(id);
    } else {
      serialSendPosSpeed(id);
    }
    return;
  }

  if (strcasecmp(cmd, "GETP") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage GETP <id>");
      return;
    }
    if (!ensureServoExists(id)) return;
    serialSendPosOnly(id);
    return;
  }

  if (strcasecmp(cmd, "GETPS") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage GETPS <id>");
      return;
    }
    if (!ensureServoExists(id)) return;
    serialSendPosSpeed(id);
    return;
  }

  if (strcasecmp(cmd, "TMODE") == 0) {
    char* mode_s = strtok(nullptr, " \t");
    if (!mode_s) {
      Serial.println("ERR usage TMODE <POS|POSSPD>");
      return;
    }

    if (strcasecmp(mode_s, "POS") == 0) {
      g_telem_mode = TelemetryMode::POS;
      feedback_include_speed = false;
      Serial.println("OK tmode=POS");
      return;
    }
    if (strcasecmp(mode_s, "POSSPD") == 0) {
      g_telem_mode = TelemetryMode::POS_SPEED;
      feedback_include_speed = true;
      Serial.println("OK tmode=POSSPD");
      return;
    }

    Serial.println("ERR usage TMODE <POS|POSSPD>");
    return;
  }

  if (strcasecmp(cmd, "TRATE") == 0) {
    uint16_t period = 0;
    if (!parseU16(strtok(nullptr, " \t"), period) || period < 10) {
      Serial.println("ERR usage TRATE <ms>=10..65535");
      return;
    }
    g_stream_period_ms = period;
    Serial.printf("OK trate=%lu\n", (unsigned long)g_stream_period_ms);
    return;
  }

  if (strcasecmp(cmd, "INFO") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage INFO <id>");
      return;
    }
    if (!ensureServoExists(id)) return;
    Servo* s = servos[id];
    const char* type_str = (s->type() == ServoBusApi::ServoType::STS) ? "STS" : "SC";
    Serial.printf(
      "INFO id=%u type=%s range=%u min=%u max=%u hasCurrent=%u\n",
      id,
      type_str,
      (unsigned)s->full_range(),
      (unsigned)s->min_encoder_angle(),
      (unsigned)s->max_encoder_angle(),
      s->current_supported() ? 1 : 0
    );
    return;
  }

  if (strcasecmp(cmd, "SET") == 0) {
    uint8_t id;
    uint16_t pos;
    uint16_t speed = activeServoSpeed;
    uint8_t acc = ServoInitACC;

    if (!parseU8(strtok(nullptr, " \t"), id) || !parseU16(strtok(nullptr, " \t"), pos)) {
      Serial.println("ERR usage SET <id> <pos> [speed] [acc]");
      return;
    }

    if (!ensureServoExists(id)) return;

    char* speed_s = strtok(nullptr, " \t");
    if (speed_s && !parseU16(speed_s, speed)) {
      Serial.println("ERR invalid_speed");
      return;
    }

    char* acc_s = strtok(nullptr, " \t");
    if (acc_s && !parseU8(acc_s, acc)) {
      Serial.println("ERR invalid_acc");
      return;
    }

    uint16_t applied_pos = pos;
    if (!applySetCommand(id, pos, speed, acc, &applied_pos)) {
      Serial.println("ERR set_failed");
      return;
    }
    Serial.printf("OK set id=%u pos=%u speed=%u acc=%u\n", id, applied_pos, speed, acc);
    return;
  }

  if (strcasecmp(cmd, "GWRITE") == 0) {
    char* ids_s = strtok(nullptr, " \t");
    char* pos_s = strtok(nullptr, " \t");
    uint16_t time_ms = 0;
    uint16_t speed = 0;

    if (!ids_s || !pos_s) {
      Serial.println("ERR usage GWRITE <ids_csv> <pos_csv> [time_ms] [speed]");
      return;
    }

    std::vector<uint8_t> ids;
    std::vector<uint16_t> pos;
    if (!parseCsvU8(ids_s, ids) || !parseCsvU16(pos_s, pos) || ids.size() != pos.size()) {
      Serial.println("ERR invalid_ids_or_positions");
      return;
    }

    char* t_s = strtok(nullptr, " \t");
    if (t_s && !parseU16(t_s, time_ms)) {
      Serial.println("ERR invalid_time_ms");
      return;
    }

    char* spd_s = strtok(nullptr, " \t");
    if (spd_s && !parseU16(spd_s, speed)) {
      Serial.println("ERR invalid_speed");
      return;
    }

    ServoBusApi::ServoType type;
    if (!allSameServoType(ids, type)) {
      Serial.println("ERR ids_missing_or_mixed_types");
      return;
    }

    std::vector<uint16_t> times(ids.size(), time_ms);
    std::vector<uint16_t> speeds(ids.size(), speed);

    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    servo_bus.set_servo_type(type);
    bool ok = servo_bus.sync_write_positions(ids, pos, times, speeds);
    xSemaphoreGive(servo_bus_mutex);

    if (!ok) {
      Serial.println("ERR gwrite_failed");
      return;
    }
    Serial.printf("OK gwrite count=%u\n", (unsigned)ids.size());
    return;
  }

  if (strcasecmp(cmd, "GREAD") == 0) {
    char* ids_s = strtok(nullptr, " \t");
    if (!ids_s) {
      Serial.println("ERR usage GREAD <ids_csv>");
      return;
    }

    std::vector<uint8_t> ids;
    if (!parseCsvU8(ids_s, ids)) {
      Serial.println("ERR invalid_ids");
      return;
    }

    ServoBusApi::ServoType type;
    if (!allSameServoType(ids, type)) {
      Serial.println("ERR ids_missing_or_mixed_types");
      return;
    }

    if (type != ServoBusApi::ServoType::STS) {
      // SC sync_read support is not reliable; fall back to per-servo reads.
      for (size_t i = 0; i < ids.size(); i++) {
        serialSendFeedback(ids[i]);
      }
      Serial.printf("OK gread_fallback count=%u\n", (unsigned)ids.size());
      return;
    }

    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    servo_bus.set_servo_type(type);
    auto vals = servo_bus.sync_read_positions(ids);
    xSemaphoreGive(servo_bus_mutex);

    for (size_t i = 0; i < ids.size(); i++) {
      if (i < vals.size() && vals[i].has_value()) {
        Serial.printf("GREAD id=%u pos=%d\n", ids[i], *vals[i]);
      } else {
        Serial.printf("GREAD id=%u pos=NA\n", ids[i]);
      }
    }
    Serial.printf("OK gread count=%u\n", (unsigned)ids.size());
    return;
  }

  if (strcasecmp(cmd, "AWRITE") == 0) {
    uint8_t id;
    uint8_t reg;
    uint16_t val;
    uint8_t bytes = 0;
    if (!parseU8(strtok(nullptr, " \t"), id) ||
        !parseU8(strtok(nullptr, " \t"), reg) ||
        !parseU16(strtok(nullptr, " \t"), val)) {
      Serial.println("ERR usage AWRITE <id> <reg> <value> [bytes]");
      return;
    }
    if (!ensureServoExists(id)) return;

    char* b_s = strtok(nullptr, " \t");
    if (b_s) {
      if (!parseU8(b_s, bytes) || (bytes != 1 && bytes != 2)) {
        Serial.println("ERR bytes_must_be_1_or_2");
        return;
      }
    } else {
      bytes = (val <= 0xFF) ? 1 : 2;
    }

    uint8_t params[3];
    int param_count = 0;
    params[param_count++] = reg;
    if (bytes == 1) {
      params[param_count++] = static_cast<uint8_t>(val & 0xFF);
    } else {
      if (servos[id]->type() == ServoBusApi::ServoType::STS) {
        params[param_count++] = static_cast<uint8_t>(val & 0xFF);
        params[param_count++] = static_cast<uint8_t>((val >> 8) & 0xFF);
      } else {
        params[param_count++] = static_cast<uint8_t>((val >> 8) & 0xFF);
        params[param_count++] = static_cast<uint8_t>(val & 0xFF);
      }
    }

    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    servo_bus.set_servo_type(servos[id]->type());
    bool ok = servo_bus.send_command(id, ServoBusApi::Instruction::reg_write, params, param_count);
    xSemaphoreGive(servo_bus_mutex);

    if (!ok) {
      Serial.println("ERR awrite_failed");
      return;
    }
    Serial.printf("OK awrite id=%u reg=%u value=%u bytes=%u\n", id, reg, (unsigned)val, bytes);
    return;
  }

  if (strcasecmp(cmd, "ACTION") == 0) {
    uint8_t id = static_cast<uint8_t>(ServoBusApi::Protocol::broadcast_id);
    char* id_s = strtok(nullptr, " \t");
    if (id_s && !parseU8(id_s, id)) {
      Serial.println("ERR usage ACTION [id]");
      return;
    }

    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    bool ok = servo_bus.send_command(id, ServoBusApi::Instruction::reg_action, nullptr, 0);
    xSemaphoreGive(servo_bus_mutex);
    if (!ok) {
      Serial.println("ERR action_failed");
      return;
    }
    Serial.printf("OK action id=%u\n", id);
    return;
  }

  if (strcasecmp(cmd, "AREAD") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage AREAD <id>");
      return;
    }
    if (!ensureServoExists(id)) return;
    // Fast cached read from latest polling cycle.
    Serial.printf(
      "AREAD id=%u pos=%d goal=%d speed=%d voltage=%.1f temp=%d current=%d torque=%u mode=%d alarm=%u\n",
      id,
      posRead[id],
      goalRead[id],
      speedRead[id],
      (float)voltageRead[id] / 10.0f,
      temperRead[id],
      currentRead[id],
      Torque_List[id] ? 1 : 0,
      modeRead[id],
      alarmRead[id]
    );
    return;
  }

  if (strcasecmp(cmd, "TORQUE") == 0) {
    uint8_t id;
    uint8_t en;
    if (!parseU8(strtok(nullptr, " \t"), id) || !parseU8(strtok(nullptr, " \t"), en)) {
      Serial.println("ERR usage TORQUE <id> <0|1>");
      return;
    }
    if (!ensureServoExists(id)) return;
    servoTorque(id, en ? 1 : 0);
    Torque_List[id] = (en != 0);
    Serial.printf("OK torque id=%u enabled=%u\n", id, en ? 1 : 0);
    return;
  }

  if (strcasecmp(cmd, "STOP") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage STOP <id>");
      return;
    }
    if (!ensureServoExists(id)) return;
    servoStop(id);
    Serial.printf("OK stop id=%u\n", id);
    return;
  }

  if (strcasecmp(cmd, "MODE") == 0) {
    uint8_t id;
    uint8_t mode;
    if (!parseU8(strtok(nullptr, " \t"), id) || !parseU8(strtok(nullptr, " \t"), mode)) {
      Serial.println("ERR usage MODE <id> <0|3>");
      return;
    }
    if (mode != 0 && mode != 3) {
      Serial.println("ERR mode_must_be_0_or_3");
      return;
    }
    if (!ensureServoExists(id)) return;
    setMode(id, mode);
    Serial.printf("OK mode id=%u value=%u\n", id, mode);
    return;
  }

  if (strcasecmp(cmd, "MIDDLE") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage MIDDLE <id>");
      return;
    }
    if (!ensureServoExists(id)) return;
    setMiddle(id);
    Serial.printf("OK middle id=%u\n", id);
    return;
  }

  if (strcasecmp(cmd, "SETID") == 0) {
    uint8_t id;
    uint8_t new_id;
    if (!parseU8(strtok(nullptr, " \t"), id) || !parseU8(strtok(nullptr, " \t"), new_id)) {
      Serial.println("ERR usage SETID <id> <new_id>");
      return;
    }
    if (!ensureServoExists(id)) return;
    if (new_id > 252) {
      Serial.println("ERR invalid_new_id");
      return;
    }

    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    bool ok = servos[id]->set_id(new_id);
    xSemaphoreGive(servo_bus_mutex);
    if (!ok) {
      Serial.println("ERR setid_failed");
      return;
    }

    servos[new_id] = servos[id];
    servos[id] = nullptr;
    for (int i = 0; i < searchNum; i++) {
      if (listID[i] == id) {
        listID[i] = new_id;
        break;
      }
    }
    Serial.printf("OK setid old=%u new=%u\n", id, new_id);
    return;
  }

  if (strcasecmp(cmd, "LIMITS") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage LIMITS <id> [min max]");
      return;
    }
    if (!ensureServoExists(id)) return;

    char* min_s = strtok(nullptr, " \t");
    char* max_s = strtok(nullptr, " \t");
    if (!min_s && !max_s) {
      xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
      auto lim = servos[id]->read_angle_limits();
      xSemaphoreGive(servo_bus_mutex);
      if (!lim) {
        Serial.println("ERR limits_read_failed");
        return;
      }
      Serial.printf("LIMITS id=%u min=%u max=%u\n", id, (unsigned)lim->min_angle, (unsigned)lim->max_angle);
      return;
    }

    uint16_t min_v;
    uint16_t max_v;
    if (!min_s || !max_s || !parseU16(min_s, min_v) || !parseU16(max_s, max_v) || min_v >= max_v) {
      Serial.println("ERR usage LIMITS <id> [min max]");
      return;
    }

    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    bool ok = servos[id]->write_angle_limits(min_v, max_v);
    xSemaphoreGive(servo_bus_mutex);
    if (!ok) {
      Serial.println("ERR limits_write_failed");
      return;
    }
    Serial.printf("OK limits id=%u min=%u max=%u\n", id, min_v, max_v);
    return;
  }

  if (strcasecmp(cmd, "TORQUE_LIMIT") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage TORQUE_LIMIT <id> [value]");
      return;
    }
    if (!ensureServoExists(id)) return;
    if (servos[id]->type() != ServoBusApi::ServoType::STS) {
      Serial.println("ERR torque_limit_requires_sts");
      return;
    }

    char* val_s = strtok(nullptr, " \t");
    auto* sts = static_cast<STSServo*>(servos[id]);
    if (!val_s) {
      xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
      uint16_t val = sts->read_torque_limit();
      xSemaphoreGive(servo_bus_mutex);
      Serial.printf("TORQUE_LIMIT id=%u value=%u\n", id, (unsigned)val);
      return;
    }

    uint16_t val;
    if (!parseU16(val_s, val) || val > 1023) {
      Serial.println("ERR invalid_torque_limit");
      return;
    }
    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    bool ok = sts->set_torque_limit(val);
    xSemaphoreGive(servo_bus_mutex);
    if (!ok) {
      Serial.println("ERR torque_limit_write_failed");
      return;
    }
    Serial.printf("OK torque_limit id=%u value=%u\n", id, (unsigned)val);
    return;
  }

  if (strcasecmp(cmd, "WHEEL") == 0) {
    uint8_t id;
    int16_t speed;
    if (!parseU8(strtok(nullptr, " \t"), id) || !parseI16(strtok(nullptr, " \t"), speed)) {
      Serial.println("ERR usage WHEEL <id> <signed_speed>");
      return;
    }
    if (!ensureServoExists(id)) return;
    if (servos[id]->type() != ServoBusApi::ServoType::STS) {
      Serial.println("ERR wheel_requires_sts");
      return;
    }
    auto* sts = static_cast<STSServo*>(servos[id]);
    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    bool mode_ok = sts->enable_wheel_mode();
    bool vel_ok = mode_ok ? sts->set_wheel_velocity(speed) : false;
    xSemaphoreGive(servo_bus_mutex);
    if (!mode_ok || !vel_ok) {
      Serial.println("ERR wheel_command_failed");
      return;
    }
    Serial.printf("OK wheel id=%u speed=%d\n", id, (int)speed);
    return;
  }

  if (strcasecmp(cmd, "STREAM") == 0) {
    uint8_t en;
    if (!parseU8(strtok(nullptr, " \t"), en)) {
      Serial.println("ERR usage STREAM <0|1> [period_ms]");
      return;
    }

    char* p = strtok(nullptr, " \t");
    if (p) {
      uint16_t period;
      if (!parseU16(p, period) || period < 10) {
        Serial.println("ERR invalid_period_ms");
        return;
      }
      g_stream_period_ms = period;
    }

    g_stream_enabled = (en != 0);
    Serial.printf("OK stream enabled=%u period_ms=%lu\n", g_stream_enabled ? 1 : 0, (unsigned long)g_stream_period_ms);
    return;
  }

  if (strcasecmp(cmd, "PING") == 0) {
    uint8_t id;
    if (!parseU8(strtok(nullptr, " \t"), id)) {
      Serial.println("ERR usage PING <id>");
      return;
    }

    xSemaphoreTake(servo_bus_mutex, portMAX_DELAY);
    auto res = servo_bus.ping(id);
    xSemaphoreGive(servo_bus_mutex);

    if (res.has_value()) {
      Serial.printf("OK ping id=%u\n", id);
    } else {
      Serial.printf("ERR ping_failed id=%u\n", id);
    }
    return;
  }

  Serial.println("ERR unknown_command");
}

static void processCommandQueues() {
  // Prefer write/control commands, but keep room for read/utility commands.
  uint8_t write_budget = 6;
  uint8_t misc_budget = 3;
  char line[192];

  while (write_budget > 0 && queuePop(g_write_queue, line, sizeof(line))) {
    CommandKind kind = commandKindFromLine(line);
    uint32_t t0 = micros();
    handleSerialCommand(line);
    uint32_t dt = micros() - t0;
    g_perf.exec_us_last = dt;
    if (dt > g_perf.exec_us_max) {
      g_perf.exec_us_max = dt;
    }
    trackCommandPerf(kind, dt);
    if (isWriteKind(kind)) {
      g_last_write_cmd_ms = millis();
    }
    g_perf.processed_write++;
    write_budget--;
  }

  while (misc_budget > 0 && queuePop(g_misc_queue, line, sizeof(line))) {
    CommandKind kind = commandKindFromLine(line);
    uint32_t t0 = micros();
    handleSerialCommand(line);
    uint32_t dt = micros() - t0;
    g_perf.exec_us_last = dt;
    if (dt > g_perf.exec_us_max) {
      g_perf.exec_us_max = dt;
    }
    trackCommandPerf(kind, dt);
    g_perf.processed_misc++;
    misc_budget--;
  }
}

void serialBridgeLoop() {
  flushStagedSetCommandsIfDue();

  while (Serial.available() > 0) {
    uint8_t b = static_cast<uint8_t>(Serial.read());
    if (processBinaryByte(b)) {
      continue;
    }

    char c = static_cast<char>(b);
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      if (g_serial_rx_overflow) {
        g_serial_rx_overflow_count++;
        g_perf.overflow_lines++;
        Serial.printf("ERR line_too_long dropped=%lu\n", (unsigned long)g_serial_rx_overflow_count);
      } else {
        g_serial_rx_buf[g_serial_rx_len] = '\0';
        trimInPlace(g_serial_rx_buf);
        if (g_serial_rx_buf[0] != '\0') {
          g_perf.rx_lines++;
          if (!enqueueCommandLine(g_serial_rx_buf)) {
            Serial.printf("ERR queue_full dropped=%lu\n", (unsigned long)g_perf.queue_drops);
          }
        }
      }
      g_serial_rx_len = 0;
      g_serial_rx_overflow = false;
      continue;
    }

    if (g_serial_rx_overflow) {
      continue;
    }

    if (g_serial_rx_len < (sizeof(g_serial_rx_buf) - 1)) {
      g_serial_rx_buf[g_serial_rx_len++] = c;
    } else {
      g_serial_rx_overflow = true;
    }
  }

  processCommandQueues();
  flushStagedSetCommandsIfDue();

  if (!g_stream_enabled) {
    return;
  }

  // Do not run stream reads when write queue still has pending commands.
  if (g_write_queue.count > 0) {
    return;
  }

  uint32_t now = millis();
  if ((now - g_last_stream_ms) < g_stream_period_ms) {
    return;
  }
  g_last_stream_ms = now;

  for (int i = 0; i < searchNum; i++) {
    if (g_telem_mode == TelemetryMode::POS) {
      serialSendPosOnly(listID[i]);
    } else {
      serialSendPosSpeed(listID[i]);
    }
  }
}
