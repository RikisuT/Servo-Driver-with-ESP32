import { useState, useEffect, useCallback } from "react";

const POS = { min: 0, max: 4095 };
const SPD = { min: 0, max: 1000 };
const STEP_SIZES = [1, 10, 100];

const MOCK_SERVOS = [
  { id: 11, position: 43, actualPosition: 43, speed: 0, load: 0, voltage: 9.9, temp: 27, current: 48, moving: false, mode: "Motor", torque: true, speedSet: 100,
    minAngle: 0, maxAngle: 4095, maxTorque: 1000, acceleration: 0, goalTime: 0,
    pCoeff: 32, dCoeff: 32, iCoeff: 0, speedP: 10, speedI: 200,
    protectionCurrent: 100, maxVoltage: 90, minVoltage: 40, maxTemp: 70,
    protectiveTorque: 20, overloadTorque: 40, protectionTime: 200,
    offset: 0, phase: 0, cwDeadZone: 0, ccwDeadZone: 0, eepromLocked: true },
  { id: 12, position: 2048, actualPosition: 2041, speed: 0, load: 12, voltage: 9.85, temp: 29, current: 120, moving: false, mode: "Servo", torque: true, speedSet: 200,
    minAngle: 0, maxAngle: 4095, maxTorque: 1000, acceleration: 50, goalTime: 0,
    pCoeff: 32, dCoeff: 32, iCoeff: 0, speedP: 10, speedI: 200,
    protectionCurrent: 100, maxVoltage: 90, minVoltage: 40, maxTemp: 70,
    protectiveTorque: 20, overloadTorque: 40, protectionTime: 200,
    offset: 0, phase: 0, cwDeadZone: 0, ccwDeadZone: 0, eepromLocked: true },
  { id: 13, position: 890, actualPosition: 743, speed: 0, load: 5, voltage: 9.92, temp: 26, current: 65, moving: true, mode: "Servo", torque: false, speedSet: 100,
    minAngle: 500, maxAngle: 3500, maxTorque: 800, acceleration: 20, goalTime: 0,
    pCoeff: 32, dCoeff: 32, iCoeff: 0, speedP: 10, speedI: 200,
    protectionCurrent: 100, maxVoltage: 90, minVoltage: 40, maxTemp: 70,
    protectiveTorque: 20, overloadTorque: 40, protectionTime: 200,
    offset: 0, phase: 253, cwDeadZone: 0, ccwDeadZone: 0, eepromLocked: true },
];

const DEFAULT_PRESETS = [
  { name: "Center All", positions: { 11: 2048, 12: 2048, 13: 2048 } },
  { name: "Zero All", positions: { 11: 0, 12: 0, 13: 0 } },
];

/* ── Check: generate warnings for a servo ── */
function getWarnings(servo) {
  const w = [];
  if (servo.id === 1) w.push("ID is factory default (1) — set a unique ID before adding more servos");
  if (servo.minAngle === 0 && servo.maxAngle === 4095) w.push("Angle limits at full range — set mechanical limits to protect your assembly");
  if (servo.maxTorque >= 1000) w.push("Max torque at factory default (1000) — consider lowering to protect your mechanism");
  if (servo.acceleration === 0 && servo.mode === "Servo") w.push("No acceleration set — motion will be jerky");
  if (!servo.eepromLocked) w.push("EEPROM is unlocked — lock to save settings to flash");
  if (servo.temp > 55) w.push(`Temperature is high (${servo.temp}°C)`);
  if (servo.voltage < 6.0) w.push(`Voltage low (${servo.voltage.toFixed(1)}V)`);
  if (servo.minAngle >= servo.maxAngle) w.push("Min angle ≥ max angle — limits are invalid");
  return w;
}

/* ── Jog button ── */
function JogBtn({ label, onClick }) {
  return (
    <button onClick={onClick} style={{
      width: 28, height: 28, borderRadius: 6, fontSize: 14, fontWeight: 700,
      background: "rgba(255,255,255,0.04)", border: "1px solid rgba(255,255,255,0.08)",
      color: "#9499ad", cursor: "pointer", fontFamily: "var(--mono)",
      display: "flex", alignItems: "center", justifyContent: "center",
      padding: 0, transition: "all 0.12s", lineHeight: 1,
    }}>{label}</button>
  );
}

/* ── Step size selector ── */
function StepSelector({ step, onChangeStep }) {
  return (
    <div style={{ display: "flex", gap: 2, alignItems: "center" }}>
      <span style={{ fontSize: 9, color: "#3d4250", marginRight: 3 }}>±</span>
      {STEP_SIZES.map((s) => (
        <button key={s} onClick={() => onChangeStep(s)} style={{
          padding: "2px 6px", borderRadius: 4, fontSize: 9, fontWeight: 600,
          background: step === s ? "rgba(108,140,255,0.15)" : "transparent",
          border: step === s ? "1px solid rgba(108,140,255,0.25)" : "1px solid transparent",
          color: step === s ? "#6c8cff" : "#4a4f62",
          cursor: "pointer", fontFamily: "var(--mono)", transition: "all 0.12s",
        }}>{s}</button>
      ))}
    </div>
  );
}

/* ── Slider + direct text input + jog buttons ── */
function SliderField({ value, min, max, onChange, label, accent = "#6c8cff", jogOnly = false }) {
  const [text, setText] = useState(String(value));
  const [focused, setFocused] = useState(false);
  const [step, setStep] = useState(10);
  useEffect(() => { if (!focused) setText(String(value)); }, [value, focused]);
  const commit = (v) => {
    const n = Math.min(max, Math.max(min, parseInt(v) || min));
    onChange(n); setText(String(n));
  };
  const jog = (dir) => onChange(Math.min(max, Math.max(min, value + dir * step)));
  const pct = ((value - min) / (max - min)) * 100;

  if (jogOnly) {
    return (
      <div style={{ flex: 1, minWidth: 0 }}>
        {label && <span style={{ fontSize: 10, fontWeight: 600, color: "#666e82", textTransform: "uppercase", letterSpacing: "0.08em", display: "block", marginBottom: 8 }}>{label}</span>}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "center", gap: 12 }}>
          <button onClick={() => jog(-1)} style={{
            width: 48, height: 48, borderRadius: 10, fontSize: 22, fontWeight: 700,
            background: "rgba(108,140,255,0.08)", border: "1px solid rgba(108,140,255,0.2)",
            color: "#6c8cff", cursor: "pointer", fontFamily: "var(--mono)",
            display: "flex", alignItems: "center", justifyContent: "center",
            padding: 0, transition: "all 0.12s", lineHeight: 1,
          }}>−</button>
          <input type="text" value={text}
            onChange={(e) => setText(e.target.value)}
            onFocus={() => setFocused(true)}
            onBlur={(e) => { setFocused(false); commit(e.target.value); }}
            onKeyDown={(e) => e.key === "Enter" && e.target.blur()}
            style={{
              background: "transparent", border: "none",
              borderBottom: `2px solid ${focused ? accent : "#282c37"}`,
              color: "#e2e6f0", fontSize: 24, fontFamily: "var(--mono)",
              fontWeight: 800, width: 80, textAlign: "center", outline: "none",
              padding: "0 0 2px", transition: "border-color 0.15s",
            }}
          />
          <button onClick={() => jog(1)} style={{
            width: 48, height: 48, borderRadius: 10, fontSize: 22, fontWeight: 700,
            background: "rgba(108,140,255,0.08)", border: "1px solid rgba(108,140,255,0.2)",
            color: "#6c8cff", cursor: "pointer", fontFamily: "var(--mono)",
            display: "flex", alignItems: "center", justifyContent: "center",
            padding: 0, transition: "all 0.12s", lineHeight: 1,
          }}>+</button>
        </div>
        <div style={{ display: "flex", justifyContent: "center", marginTop: 8 }}>
          <StepSelector step={step} onChangeStep={setStep} />
        </div>
      </div>
    );
  }

  return (
    <div style={{ flex: 1, minWidth: 0 }}>
      <div style={{ display: "flex", justifyContent: label ? "space-between" : "flex-end", alignItems: "baseline", marginBottom: 6 }}>
        {label && <span style={{ fontSize: 10, fontWeight: 600, color: "#666e82", textTransform: "uppercase", letterSpacing: "0.08em" }}>{label}</span>}
        <div style={{ display: "flex", alignItems: "center", gap: 6 }}>
          <JogBtn label="−" onClick={() => jog(-1)} />
          <input type="text" value={text}
            onChange={(e) => setText(e.target.value)}
            onFocus={() => setFocused(true)}
            onBlur={(e) => { setFocused(false); commit(e.target.value); }}
            onKeyDown={(e) => e.key === "Enter" && e.target.blur()}
            style={{
              background: "transparent", border: "none",
              borderBottom: `1.5px solid ${focused ? accent : "#282c37"}`,
              color: "#e2e6f0", fontSize: 15, fontFamily: "var(--mono)",
              fontWeight: 700, width: 58, textAlign: "center", outline: "none",
              padding: "0 0 1px", transition: "border-color 0.15s",
            }}
          />
          <JogBtn label="+" onClick={() => jog(1)} />
        </div>
      </div>
      <input type="range" min={min} max={max} value={value}
        onChange={(e) => onChange(parseInt(e.target.value))}
        style={{
          width: "100%", height: 4, appearance: "none", borderRadius: 2,
          outline: "none", cursor: "pointer",
          background: `linear-gradient(90deg, ${accent} ${pct}%, #1a1d25 ${pct}%)`,
        }}
      />
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginTop: 2 }}>
        <span style={{ fontSize: 8, color: "#3d4250" }}>{min}</span>
        <StepSelector step={step} onChangeStep={setStep} />
        <span style={{ fontSize: 8, color: "#3d4250" }}>{max}</span>
      </div>
    </div>
  );
}

/* ── Compact numeric field for config ── */
function ConfigField({ label, value, onChange, min = 0, max = 9999, width = 52, suffix, action }) {
  const [text, setText] = useState(String(value));
  const [focused, setFocused] = useState(false);
  useEffect(() => { if (!focused) setText(String(value)); }, [value, focused]);
  const commit = (v) => {
    const n = Math.min(max, Math.max(min, parseInt(v) || min));
    onChange(n); setText(String(n));
  };
  return (
    <div style={{ display: "flex", alignItems: "center", gap: 8, minHeight: 28 }}>
      <span style={{ fontSize: 10, color: "#555b6e", minWidth: 100 }}>{label}</span>
      <input type="text" value={text}
        onChange={(e) => setText(e.target.value)}
        onFocus={() => setFocused(true)}
        onBlur={(e) => { setFocused(false); commit(e.target.value); }}
        onKeyDown={(e) => e.key === "Enter" && e.target.blur()}
        style={{
          background: "rgba(255,255,255,0.03)", border: `1px solid ${focused ? "rgba(108,140,255,0.3)" : "rgba(255,255,255,0.06)"}`,
          borderRadius: 5, color: "#e2e6f0", fontSize: 12, fontFamily: "var(--mono)",
          fontWeight: 600, width, textAlign: "center", outline: "none",
          padding: "3px 6px", transition: "border-color 0.15s",
        }}
      />
      {suffix && <span style={{ fontSize: 9, color: "#3d4250" }}>{suffix}</span>}
      {action}
    </div>
  );
}

/* ── Toggle pill ── */
function Toggle({ on, onToggle, label }) {
  return (
    <button onClick={onToggle} style={{
      display: "inline-flex", alignItems: "center", gap: 6,
      padding: "5px 12px", borderRadius: 6, border: "none", cursor: "pointer",
      background: on ? "rgba(80,200,120,0.12)" : "rgba(255,255,255,0.04)",
      color: on ? "#50c878" : "#555b6e", fontSize: 11, fontWeight: 700,
      fontFamily: "inherit", letterSpacing: "0.02em", transition: "all 0.15s",
    }}>
      <span style={{
        width: 6, height: 6, borderRadius: "50%",
        background: on ? "#50c878" : "#444857",
        boxShadow: on ? "0 0 6px rgba(80,200,120,0.4)" : "none",
        transition: "all 0.15s",
      }} />
      {label}
    </button>
  );
}

/* ── Small button ── */
function Btn({ children, onClick, active, variant, style: sx }) {
  const color = variant === "danger" ? "#e85d5d" : variant === "accent" ? "#6c8cff" : variant === "warn" ? "#e8943a" : "#9499ad";
  return (
    <button onClick={onClick} style={{
      padding: "5px 11px", borderRadius: 6, fontSize: 11, fontWeight: 600,
      background: active ? `${color}18` : "rgba(255,255,255,0.03)",
      border: `1px solid ${active ? `${color}33` : "rgba(255,255,255,0.06)"}`,
      color: active ? color : "#7a7f92",
      cursor: "pointer", fontFamily: "inherit", transition: "all 0.12s", whiteSpace: "nowrap",
      ...sx,
    }}>{children}</button>
  );
}

/* ── Capture button (tiny inline) ── */
function CaptureBtn({ label, onClick }) {
  return (
    <button onClick={onClick} style={{
      padding: "2px 6px", borderRadius: 4, fontSize: 9, fontWeight: 600,
      background: "rgba(108,140,255,0.08)", border: "1px solid rgba(108,140,255,0.15)",
      color: "#6c8cff", cursor: "pointer", fontFamily: "inherit",
      transition: "all 0.12s", whiteSpace: "nowrap",
    }}>{label}</button>
  );
}

/* ── Config sub-group header ── */
function ConfigGroup({ title, children }) {
  return (
    <div style={{ marginBottom: 10 }}>
      <div style={{
        fontSize: 9, fontWeight: 700, color: "#3d4250", textTransform: "uppercase",
        letterSpacing: "0.12em", marginBottom: 6, paddingTop: 6,
        borderTop: "1px solid rgba(255,255,255,0.03)",
      }}>{title}</div>
      <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
        {children}
      </div>
    </div>
  );
}

/* ── Warning banner ── */
function WarningBanner({ warnings, onDismiss }) {
  if (!warnings.length) return null;
  return (
    <div style={{
      background: "rgba(232,148,58,0.06)", border: "1px solid rgba(232,148,58,0.15)",
      borderRadius: 8, padding: "10px 14px", marginTop: 10,
    }}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 6 }}>
        <span style={{ fontSize: 10, fontWeight: 700, color: "#e8943a", textTransform: "uppercase", letterSpacing: "0.08em" }}>
          ⚠ {warnings.length} warning{warnings.length > 1 ? "s" : ""}
        </span>
        <button onClick={onDismiss} style={{
          background: "none", border: "none", color: "#555b6e", fontSize: 10,
          cursor: "pointer", fontFamily: "inherit",
        }}>dismiss</button>
      </div>
      {warnings.map((w, i) => (
        <div key={i} style={{ fontSize: 11, color: "#c4956a", lineHeight: 1.5, paddingLeft: 4 }}>
          • {w}
        </div>
      ))}
    </div>
  );
}

/* ── Collapsible config section ── */
function ConfigSection({ servo, onUpdate }) {
  const [open, setOpen] = useState(false);
  const [newId, setNewId] = useState("");
  const u = (field) => (val) => onUpdate({ [field]: val });

  return (
    <div style={{ borderTop: "1px solid rgba(255,255,255,0.04)", marginTop: 14 }}>
      <button onClick={() => setOpen(!open)} style={{
        background: "none", border: "none", cursor: "pointer", fontFamily: "inherit",
        display: "flex", alignItems: "center", gap: 6, padding: "10px 0 4px", width: "100%",
        color: "#4a4f62", fontSize: 10, fontWeight: 600, textTransform: "uppercase", letterSpacing: "0.1em",
      }}>
        <span style={{
          display: "inline-block", transition: "transform 0.2s",
          transform: open ? "rotate(90deg)" : "rotate(0deg)", fontSize: 9,
        }}>▶</span>
        Config
        {!servo.eepromLocked && <span style={{
          marginLeft: 8, fontSize: 9, color: "#e8943a", fontWeight: 700, letterSpacing: "0.05em",
        }}>EEPROM UNLOCKED</span>}
      </button>
      {open && (
        <div style={{ padding: "8px 0 4px" }}>

          {/* EEPROM Lock */}
          <div style={{ display: "flex", gap: 6, marginBottom: 12, alignItems: "center" }}>
            <Btn variant={servo.eepromLocked ? "default" : "danger"} active={!servo.eepromLocked}
              onClick={() => onUpdate({ eepromLocked: !servo.eepromLocked })}>
              {servo.eepromLocked ? "Unlock EEPROM" : "Lock EEPROM (Save)"}
            </Btn>
            <span style={{ fontSize: 9, color: "#4a4f62" }}>
              {servo.eepromLocked ? "Unlock before changing persistent settings" : "Changes are live — lock to write to flash"}
            </span>
          </div>

          {/* Mode */}
          <ConfigGroup title="Mode">
            <div style={{ display: "flex", alignItems: "center", gap: 6, flexWrap: "wrap" }}>
              <span style={{ fontSize: 10, color: "#555b6e", minWidth: 100 }}>Operating</span>
              <Btn active={servo.mode === "Servo"} onClick={() => onUpdate({ mode: "Servo" })}>Servo</Btn>
              <Btn active={servo.mode === "Motor"} onClick={() => onUpdate({ mode: "Motor" })}>Motor</Btn>
            </div>
            <ConfigField label="Phase (dir)" value={servo.phase} onChange={u("phase")} max={255} />
            <ConfigField label="Offset" value={servo.offset} onChange={u("offset")} min={-2048} max={2047} />
          </ConfigGroup>

          {/* Safety Limits — with capture buttons */}
          <ConfigGroup title="Safety Limits">
            <ConfigField label="Min Angle" value={servo.minAngle} onChange={u("minAngle")} max={4095}
              action={<>
                <CaptureBtn label="← Capture" onClick={() => onUpdate({ minAngle: servo.actualPosition })} />
                <CaptureBtn label="Go →" onClick={() => onUpdate({ position: servo.minAngle })} />
              </>}
            />
            <ConfigField label="Max Angle" value={servo.maxAngle} onChange={u("maxAngle")} max={4095}
              action={<>
                <CaptureBtn label="← Capture" onClick={() => onUpdate({ maxAngle: servo.actualPosition })} />
                <CaptureBtn label="Go →" onClick={() => onUpdate({ position: servo.maxAngle })} />
              </>}
            />
            <ConfigField label="Max Torque" value={servo.maxTorque} onChange={u("maxTorque")} max={1000} />
            <div style={{ display: "flex", gap: 16, flexWrap: "wrap" }}>
              <ConfigField label="Min Voltage" value={servo.minVoltage} onChange={u("minVoltage")} max={255} />
              <ConfigField label="Max Voltage" value={servo.maxVoltage} onChange={u("maxVoltage")} max={255} />
            </div>
            <ConfigField label="Max Temp" value={servo.maxTemp} onChange={u("maxTemp")} max={100} suffix="°C" />
          </ConfigGroup>

          {/* Motion */}
          <ConfigGroup title="Motion">
            <ConfigField label="Acceleration" value={servo.acceleration} onChange={u("acceleration")} max={254} />
            <ConfigField label="Goal Time" value={servo.goalTime} onChange={u("goalTime")} max={65535} width={64} suffix="ms" />
            <div style={{ display: "flex", gap: 16, flexWrap: "wrap" }}>
              <ConfigField label="CW Dead Zone" value={servo.cwDeadZone} onChange={u("cwDeadZone")} max={255} />
              <ConfigField label="CCW Dead Zone" value={servo.ccwDeadZone} onChange={u("ccwDeadZone")} max={255} />
            </div>
          </ConfigGroup>

          {/* PID Tuning */}
          <ConfigGroup title="PID Tuning">
            <div style={{ display: "flex", gap: 16, flexWrap: "wrap" }}>
              <ConfigField label="P" value={servo.pCoeff} onChange={u("pCoeff")} max={255} width={40} />
              <ConfigField label="I" value={servo.iCoeff} onChange={u("iCoeff")} max={255} width={40} />
              <ConfigField label="D" value={servo.dCoeff} onChange={u("dCoeff")} max={255} width={40} />
            </div>
            <div style={{ display: "flex", gap: 16, flexWrap: "wrap" }}>
              <ConfigField label="Speed P" value={servo.speedP} onChange={u("speedP")} max={255} width={40} />
              <ConfigField label="Speed I" value={servo.speedI} onChange={u("speedI")} max={255} width={40} />
            </div>
          </ConfigGroup>

          {/* Protection */}
          <ConfigGroup title="Protection">
            <ConfigField label="Prot. Current" value={servo.protectionCurrent} onChange={u("protectionCurrent")} max={511} />
            <ConfigField label="Prot. Torque" value={servo.protectiveTorque} onChange={u("protectiveTorque")} max={255} />
            <ConfigField label="Overload Torque" value={servo.overloadTorque} onChange={u("overloadTorque")} max={255} />
            <ConfigField label="Prot. Time" value={servo.protectionTime} onChange={u("protectionTime")} max={255} suffix="ms" />
          </ConfigGroup>

          {/* Actions */}
          <ConfigGroup title="Actions">
            <div style={{ display: "flex", gap: 6, flexWrap: "wrap", alignItems: "center" }}>
              <Btn onClick={() => {}}>Set Middle</Btn>
            </div>
            <div style={{ display: "flex", gap: 6, alignItems: "center", marginTop: 4 }}>
              <span style={{ fontSize: 10, color: "#555b6e" }}>Set New ID</span>
              <input type="text" value={newId} onChange={(e) => setNewId(e.target.value)}
                placeholder={String(servo.id)}
                style={{
                  background: "rgba(255,255,255,0.03)", border: "1px solid rgba(255,255,255,0.06)",
                  borderRadius: 5, color: "#e2e6f0", fontSize: 12, fontFamily: "var(--mono)",
                  fontWeight: 600, width: 40, textAlign: "center", outline: "none", padding: "3px 6px",
                }}
              />
              <Btn variant="warn" onClick={() => {
                const n = parseInt(newId);
                if (n && n >= 1 && n <= 253) { onUpdate({ id: n }); setNewId(""); }
              }}>Apply</Btn>
            </div>
          </ConfigGroup>

        </div>
      )}
    </div>
  );
}

/* ── Single servo card ── */
function ServoCard({ servo, onUpdate }) {
  const isMotor = servo.mode === "Motor";
  const [warnings, setWarnings] = useState([]);
  const [showWarnings, setShowWarnings] = useState(false);

  const runCheck = () => {
    const w = getWarnings(servo);
    setWarnings(w);
    setShowWarnings(true);
  };

  return (
    <div style={{
      background: "#13151b", border: "1px solid rgba(255,255,255,0.05)",
      borderRadius: 12, padding: "16px 20px",
    }}>
      {/* Header: ID, mode, telemetry, torque */}
      <div style={{
        display: "flex", alignItems: "center", gap: 10, marginBottom: 16, flexWrap: "wrap",
      }}>
        <span style={{ fontSize: 16, fontFamily: "var(--mono)", fontWeight: 800, color: "#e2e6f0" }}>
          #{servo.id}
        </span>
        <span style={{
          fontSize: 9, padding: "2px 7px", borderRadius: 4, fontWeight: 600,
          background: "rgba(108,140,255,0.1)", color: "#6c8cff",
        }}>{servo.mode}</span>
        {servo.moving && <span style={{
          fontSize: 9, padding: "2px 7px", borderRadius: 4, fontWeight: 600,
          background: "rgba(80,200,120,0.1)", color: "#50c878",
          animation: "pulse 1.5s ease-in-out infinite",
        }}>Moving</span>}

        <div style={{ marginLeft: "auto", display: "flex", alignItems: "baseline", gap: 12, flexWrap: "wrap" }}>
          {[
            { l: "V", v: servo.voltage.toFixed(1) },
            { l: "Load", v: servo.load, w: servo.load > 50 },
            { l: "Temp", v: `${servo.temp}°`, w: servo.temp > 50 },
            { l: "mA", v: Math.round(servo.current * 6.5), w: servo.current > 80 },
          ].map((s) => (
            <span key={s.l} style={{
              fontSize: 11, fontFamily: "var(--mono)", color: s.w ? "#e8943a" : "#555b6e",
            }}>
              <span style={{ fontSize: 9, color: "#3d4250", marginRight: 3 }}>{s.l}</span>{s.v}
            </span>
          ))}
        </div>

        <Toggle on={servo.torque} onToggle={() => onUpdate({ torque: !servo.torque })} label="Torque" />
      </div>

      {/* Position: full width, setpoint/actual */}
      <div>
        <div style={{ display: "flex", alignItems: "baseline", justifyContent: "space-between", marginBottom: 2 }}>
          <div style={{ display: "flex", alignItems: "baseline", gap: 12 }}>
            <span style={{ fontSize: 10, fontWeight: 600, color: "#666e82", textTransform: "uppercase", letterSpacing: "0.08em" }}>Position</span>
            <span style={{ fontSize: 10, color: "#3d4250" }}>
              Actual: <span style={{
                fontFamily: "var(--mono)", fontWeight: 700, fontSize: 12,
                color: Math.abs(servo.actualPosition - servo.position) > 20 ? "#e8943a" : "#50c878",
              }}>{servo.actualPosition}</span>
            </span>
          </div>
        </div>
        <SliderField value={servo.position} min={POS.min} max={POS.max}
          onChange={(v) => onUpdate({ position: v })} jogOnly={isMotor} label="" />
        {!isMotor && (() => {
          const actualPct = ((servo.actualPosition - POS.min) / (POS.max - POS.min)) * 100;
          return (
            <div style={{ position: "relative", height: 0, marginTop: -18 }}>
              <div style={{
                position: "absolute", left: `${actualPct}%`, top: 0,
                width: 2, height: 6, background: "#50c878", borderRadius: 1,
                transform: "translateX(-1px)", opacity: 0.7,
              }} />
            </div>
          );
        })()}
      </div>

      {/* Quick actions + speed + torque limit + check */}
      <div style={{ display: "flex", gap: 6, marginTop: 14, flexWrap: "wrap", alignItems: "center" }}>
        <Btn variant="accent" onClick={() => onUpdate({ position: 2048 })}>Center</Btn>
        <Btn onClick={() => onUpdate({ position: servo.minAngle })}>Min</Btn>
        <Btn onClick={() => onUpdate({ position: servo.maxAngle })}>Max</Btn>
        <Btn onClick={() => {}}>Stop</Btn>
        <Btn variant="danger" onClick={() => onUpdate({ torque: false })}>Release</Btn>
        <Btn variant="warn" onClick={runCheck}>Check</Btn>
      </div>
      <div style={{ display: "flex", gap: 8, marginTop: 8, flexWrap: "wrap", alignItems: "center" }}>
        <div style={{ display: "flex", alignItems: "center", gap: 6 }}>
          <span style={{ fontSize: 10, color: "#555b6e" }}>Speed</span>
          <input type="text" defaultValue={servo.speedSet}
            onBlur={(e) => {
              const v = Math.min(SPD.max, Math.max(SPD.min, parseInt(e.target.value) || 0));
              onUpdate({ speedSet: v }); e.target.value = v;
            }}
            onKeyDown={(e) => e.key === "Enter" && e.target.blur()}
            style={{
              background: "rgba(255,255,255,0.03)", border: "1px solid rgba(255,255,255,0.06)",
              borderRadius: 5, color: "#e2e6f0", fontSize: 12, fontFamily: "var(--mono)",
              fontWeight: 600, width: 48, textAlign: "center", outline: "none", padding: "3px 6px",
            }}
          />
        </div>
        <div style={{ display: "flex", alignItems: "center", gap: 6 }}>
          <span style={{ fontSize: 10, color: "#555b6e" }}>Torque Limit</span>
          <input type="text" defaultValue={servo.maxTorque}
            onBlur={(e) => {
              const v = Math.min(1000, Math.max(0, parseInt(e.target.value) || 0));
              onUpdate({ maxTorque: v }); e.target.value = v;
            }}
            onKeyDown={(e) => e.key === "Enter" && e.target.blur()}
            style={{
              background: "rgba(255,255,255,0.03)", border: "1px solid rgba(255,255,255,0.06)",
              borderRadius: 5, color: servo.maxTorque >= 1000 ? "#e8943a" : "#e2e6f0",
              fontSize: 12, fontFamily: "var(--mono)",
              fontWeight: 600, width: 48, textAlign: "center", outline: "none", padding: "3px 6px",
            }}
          />
          <span style={{ fontSize: 9, color: "#3d4250" }}>/1000</span>
        </div>
      </div>

      {/* Warnings */}
      {showWarnings && <WarningBanner warnings={warnings} onDismiss={() => setShowWarnings(false)} />}

      {/* Config */}
      <ConfigSection servo={servo} onUpdate={onUpdate} />
    </div>
  );
}

/* ── Preset bar ── */
function PresetBar({ presets, onApply, onSave, onDelete }) {
  return (
    <div style={{
      display: "flex", alignItems: "center", gap: 6, flexWrap: "wrap", padding: "10px 0",
    }}>
      <span style={{
        fontSize: 10, fontWeight: 600, color: "#4a4f62", textTransform: "uppercase",
        letterSpacing: "0.08em", marginRight: 2,
      }}>Presets</span>
      {presets.map((p, i) => (
        <div key={i} style={{ position: "relative", display: "inline-flex" }}>
          <button onClick={() => onApply(p)} style={{
            padding: "5px 11px", borderRadius: 6, fontSize: 11, fontWeight: 600,
            background: "rgba(255,255,255,0.03)", border: "1px solid rgba(255,255,255,0.06)",
            color: "#9499ad", cursor: "pointer", fontFamily: "inherit", transition: "all 0.12s",
          }}>{p.name}</button>
          {i >= 2 && (
            <button onClick={() => onDelete(i)} style={{
              position: "absolute", top: -4, right: -4, width: 14, height: 14,
              borderRadius: "50%", background: "#1a1d25", border: "1px solid #3d4250",
              color: "#666e82", fontSize: 8, cursor: "pointer", display: "flex",
              alignItems: "center", justifyContent: "center", fontFamily: "inherit", padding: 0,
              lineHeight: 1,
            }}>×</button>
          )}
        </div>
      ))}
      <button onClick={onSave} style={{
        padding: "5px 11px", borderRadius: 6, fontSize: 11, fontWeight: 600,
        background: "transparent", border: "1px dashed rgba(255,255,255,0.1)",
        color: "#555b6e", cursor: "pointer", fontFamily: "inherit",
      }}>+ Save</button>
    </div>
  );
}

/* ── App ── */
export default function ServoDriver() {
  const [servos, setServos] = useState(MOCK_SERVOS);
  const [presets, setPresets] = useState(DEFAULT_PRESETS);
  const [scanning, setScanning] = useState(false);

  const updateServo = (id, u) =>
    setServos((prev) => prev.map((s) => (s.id === id ? { ...s, ...u } : s)));

  const savePreset = () => {
    const positions = {};
    servos.forEach((s) => { positions[s.id] = s.position; });
    setPresets((p) => [...p, { name: `Snap ${p.length + 1}`, positions }]);
  };

  const applyPreset = (preset) => {
    setServos((prev) =>
      prev.map((s) => preset.positions[s.id] != null ? { ...s, position: preset.positions[s.id] } : s)
    );
  };

  const deletePreset = (i) => setPresets((p) => p.filter((_, idx) => idx !== i));
  const scan = () => { setScanning(true); setTimeout(() => setScanning(false), 1200); };

  return (
    <div style={{
      minHeight: "100vh", background: "#0c0e13", color: "#e2e6f0",
      fontFamily: "var(--sans)",
    }}>
      <style>{`
        :root {
          --sans: 'DM Sans', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
          --mono: 'JetBrains Mono', 'SF Mono', 'Fira Code', 'Consolas', monospace;
        }
        @import url('https://fonts.googleapis.com/css2?family=DM+Sans:wght@400;500;600;700;800&family=JetBrains+Mono:wght@400;500;600;700;800&display=swap');
        * { box-sizing: border-box; }
        input[type="range"]::-webkit-slider-thumb {
          -webkit-appearance: none; width: 14px; height: 14px; border-radius: 50%;
          background: #e2e6f0; border: 2px solid #6c8cff; cursor: pointer;
          box-shadow: 0 0 5px rgba(108,140,255,0.25);
        }
        button:hover { filter: brightness(1.2); }
        @keyframes pulse {
          0%, 100% { opacity: 1; }
          50% { opacity: 0.5; }
        }
      `}</style>

      {/* Top bar */}
      <header style={{
        display: "flex", alignItems: "center", justifyContent: "space-between",
        padding: "12px 20px", borderBottom: "1px solid rgba(255,255,255,0.04)",
      }}>
        <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
          <div style={{
            width: 7, height: 7, borderRadius: "50%", background: "#50c878",
            boxShadow: "0 0 6px rgba(80,200,120,0.4)",
          }} />
          <span style={{ fontSize: 13, fontWeight: 700, letterSpacing: "0.04em" }}>SERVO DRIVER</span>
          <span style={{ fontSize: 10, color: "#3d4250", fontFamily: "var(--mono)" }}>ESP32</span>
        </div>
        <button onClick={scan} style={{
          padding: "5px 14px", borderRadius: 6, fontSize: 11, fontWeight: 600,
          background: "rgba(255,255,255,0.04)", border: "1px solid rgba(255,255,255,0.06)",
          color: "#9499ad", cursor: "pointer", fontFamily: "inherit",
        }}>{scanning ? "Scanning…" : "Scan"}</button>
      </header>

      {/* Content */}
      <div style={{ maxWidth: 680, margin: "0 auto", padding: "8px 16px 40px" }}>
        <PresetBar presets={presets} onApply={applyPreset} onSave={savePreset} onDelete={deletePreset} />

        <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
          {servos.map((s) => (
            <ServoCard key={s.id} servo={s} onUpdate={(u) => updateServo(s.id, u)} />
          ))}
        </div>

        {servos.length === 0 && (
          <div style={{ textAlign: "center", padding: "60px 20px", color: "#3d4250" }}>
            <div style={{ fontSize: 14, marginBottom: 8 }}>No servos found</div>
            <div style={{ fontSize: 11 }}>Hit Scan to search for connected devices</div>
          </div>
        )}
      </div>
    </div>
  );
}
