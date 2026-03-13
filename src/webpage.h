#pragma once
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
<title>Servo Driver</title>
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<link rel="icon" href="data:,">
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{
--bg:#0c0e13;--card:#15171e;--border:rgba(255,255,255,0.07);
--text:#eef0f6;--mid:#b0b5c5;--dim:#8890a4;--label:#9ba2b4;--faint:#5a6173;
--accent:#6c8cff;--green:#50c878;--orange:#e8943a;--red:#e85d5d;
--mono:'SF Mono','Fira Code','Consolas','Courier New',monospace;
--sans:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;
}
html{background:var(--bg);color:var(--text);font-family:var(--sans)}
body{max-width:800px;margin:0 auto;padding:0 10px 40px}
button{font-family:inherit;cursor:pointer;border:none;outline:none;-webkit-tap-highlight-color:transparent}
button:active{filter:brightness(1.2)}
input{font-family:var(--mono)}

/* Header */
.hdr{display:flex;align-items:center;justify-content:space-between;padding:10px 2px;border-bottom:1px solid var(--border)}
.hdr-title{display:flex;align-items:center;gap:8px}
.hdr-dot{width:8px;height:8px;border-radius:50%;background:var(--green);box-shadow:0 0 6px rgba(80,200,120,0.4)}
.hdr-name{font-size:15px;font-weight:700;letter-spacing:0.04em}
.hdr-sub{font-size:11px;color:var(--faint);font-family:var(--mono)}
.scan-btn{padding:7px 16px;border-radius:6px;font-size:13px;font-weight:600;background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.1);color:var(--mid)}

/* Cards */
.cards{display:flex;flex-direction:column;gap:8px;margin-top:8px}
.no-servos{text-align:center;padding:60px 20px;color:var(--dim);font-size:15px}

/* Card */
.card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:12px 14px}
@media(min-width:400px){.card{padding:14px 18px}}

/* Card header: ID + mode + torque */
.card-hdr{display:flex;align-items:center;gap:8px;flex-wrap:wrap}
.servo-id{font-size:20px;font-family:var(--mono);font-weight:800}
.name-inline{background:transparent;border:1px solid transparent;border-radius:4px;color:var(--mid);font-size:14px;font-weight:600;width:90px;outline:none;padding:2px 4px;margin-left:4px;transition:border-color .15s,background .15s}
.name-inline:hover{border-color:rgba(255,255,255,0.15)}
.name-inline:focus{border-color:var(--accent);background:rgba(0,0,0,0.3);color:var(--text)}
.name-inline::placeholder{color:var(--dim);font-weight:400}
.mode-badge{font-size:11px;padding:2px 8px;border-radius:4px;font-weight:700;background:rgba(108,140,255,0.12);color:var(--accent)}
.moving-badge{font-size:11px;padding:2px 8px;border-radius:4px;font-weight:700;background:rgba(80,200,120,0.12);color:var(--green);animation:pulse 1.5s ease-in-out infinite}
.alarm-badge{font-size:10px;padding:2px 6px;border-radius:4px;font-weight:700;background:rgba(232,93,93,0.15);color:var(--red);animation:pulse 1.5s ease-in-out infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}

/* Torque toggle */
.torque-btn{display:inline-flex;align-items:center;gap:5px;padding:5px 11px;border-radius:6px;font-size:13px;font-weight:700;letter-spacing:0.02em;transition:all .15s;margin-left:auto}
.torque-on{background:rgba(80,200,120,0.15);color:var(--green)}
.torque-off{background:rgba(255,255,255,0.05);color:var(--dim)}
.torque-dot{width:7px;height:7px;border-radius:50%;transition:all .15s}
.torque-on .torque-dot{background:var(--green);box-shadow:0 0 6px rgba(80,200,120,0.4)}
.torque-off .torque-dot{background:#555}

/* Telemetry row */
.telem{display:flex;align-items:baseline;gap:12px;flex-wrap:wrap;margin:6px 0 8px}
.telem span{font-size:14px;font-family:var(--mono);color:var(--mid);font-weight:600}
.telem .tl{font-size:11px;color:var(--dim);margin-right:2px;font-weight:500}
.telem .tw{color:var(--orange) !important}

/* Position block — everything grouped */
.pos-block{background:rgba(255,255,255,0.025);border-radius:8px;padding:10px 12px;margin-top:2px}

/* Position info row */
.pos-info{display:flex;align-items:center;gap:10px;flex-wrap:wrap;margin-bottom:8px}
.pos-lbl{font-size:12px;font-weight:700;color:var(--label);text-transform:uppercase;letter-spacing:0.06em}
.pos-val{font-family:var(--mono);font-weight:700;font-size:16px}
.pos-match{color:var(--green)}
.pos-diff{color:var(--orange)}

/* Controls row */
.ctrl-row{display:flex;align-items:center;gap:6px}
.ctrl-row-center{justify-content:center}

/* Jog small */
.jog-sm{width:38px;height:38px;border-radius:7px;font-size:20px;font-weight:700;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);color:var(--mid);display:flex;align-items:center;justify-content:center;font-family:var(--mono);padding:0;transition:all .12s;line-height:1}

/* Jog large (motor) */
.jog-lg{width:52px;height:52px;border-radius:10px;font-size:24px;font-weight:700;background:rgba(108,140,255,0.1);border:1px solid rgba(108,140,255,0.25);color:var(--accent);display:flex;align-items:center;justify-content:center;font-family:var(--mono);padding:0;transition:all .12s;line-height:1}

/* Position text input */
.pos-text{background:rgba(0,0,0,0.3);border:1.5px solid rgba(255,255,255,0.1);border-radius:6px;color:var(--text);font-size:18px;font-family:var(--mono);font-weight:700;width:72px;text-align:center;outline:none;padding:5px 2px;transition:border-color .15s}
.pos-text:focus{border-color:var(--accent)}
.pos-text-lg{font-size:26px;font-weight:800;width:90px;padding:6px 2px}

/* Slider */
.slider-row{position:relative;margin-top:4px}
input[type=range]{width:100%;height:6px;-webkit-appearance:none;appearance:none;border-radius:3px;outline:none;cursor:pointer;background:#1a1d25}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;border-radius:50%;background:var(--text);border:2px solid var(--accent);cursor:pointer;box-shadow:0 0 5px rgba(108,140,255,0.3)}
.slider-bounds{display:flex;justify-content:space-between;margin-top:2px}
.slider-bounds span{font-size:10px;color:var(--faint);font-family:var(--mono)}

/* Actual marker on slider */
.actual-marker{position:absolute;top:0;width:3px;height:10px;background:var(--green);border-radius:1px;opacity:0.8;pointer-events:none;transition:left 0.3s}
.limit-marker{position:absolute;top:-2px;width:2px;height:14px;border-radius:1px;opacity:0.6;pointer-events:none;transition:left 0.3s}
.limit-min{background:var(--orange)}
.limit-max{background:var(--orange)}

/* Step selector */
.step-row{display:flex;gap:3px;align-items:center}
.step-row .pm{font-size:12px;color:var(--dim);margin-right:2px}
.step-btn{padding:3px 9px;border-radius:5px;font-size:13px;font-weight:600;background:transparent;border:1px solid transparent;color:var(--faint);font-family:var(--mono);transition:all .12s}
.step-btn.active{background:rgba(108,140,255,0.15);border-color:rgba(108,140,255,0.25);color:var(--accent)}

/* Quick action buttons */
.qa-row{display:flex;gap:5px;flex-wrap:wrap;margin-top:8px}
.qa-btn{padding:5px 10px;border-radius:6px;font-size:12px;font-weight:600;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);color:var(--mid);transition:all .12s}


/* Inline fields row */
.fields-row{display:flex;gap:12px;flex-wrap:wrap;margin-top:8px;align-items:center}
.field-grp{display:flex;align-items:center;gap:4px}
.field-lbl{font-size:11px;color:var(--dim);font-weight:600;text-transform:uppercase;letter-spacing:0.04em}
.field-input{background:rgba(0,0,0,0.3);border:1.5px solid rgba(255,255,255,0.1);border-radius:5px;color:var(--text);font-size:14px;font-family:var(--mono);font-weight:600;width:58px;text-align:center;outline:none;padding:3px 2px;transition:border-color .15s}
.field-input:focus{border-color:var(--accent)}
.field-input.warn{color:var(--orange);border-color:rgba(232,148,58,0.3)}

/* Config section */
.cfg-toggle{display:flex;align-items:center;gap:6px;margin-top:10px;padding:6px 0;cursor:pointer;border:none;background:none;color:var(--dim);font-size:12px;font-weight:600;letter-spacing:0.04em;text-transform:uppercase}
.cfg-toggle:hover{color:var(--mid)}
.cfg-arrow{transition:transform .2s;font-size:10px}
.cfg-arrow.open{transform:rotate(90deg)}
.cfg-body{display:none;padding:8px 0 2px}
.cfg-body.open{display:block}
.cfg-row{display:flex;gap:12px;flex-wrap:wrap;align-items:center;margin-bottom:8px}
.cfg-save{padding:4px 12px;border-radius:5px;font-size:12px;font-weight:600;background:rgba(108,140,255,0.15);border:1px solid rgba(108,140,255,0.25);color:var(--accent);transition:all .12s}
.cfg-save:hover{background:rgba(108,140,255,0.25)}
.cfg-save.danger{background:rgba(232,93,93,0.12);border-color:rgba(232,93,93,0.25);color:var(--red)}
.cfg-save.danger:hover{background:rgba(232,93,93,0.22)}
.cfg-status{font-size:11px;color:var(--green);font-weight:600;margin-left:4px;opacity:0;transition:opacity .3s}
.cfg-status.show{opacity:1}
/* Protection table */
.prot-hdr{display:flex;align-items:center;gap:6px;margin-bottom:4px}
.prot-row{display:flex;align-items:center;gap:6px;margin-bottom:5px;flex-wrap:wrap}
.prot-lbl{width:62px;font-size:11px;color:var(--dim);font-weight:600;text-transform:uppercase;flex-shrink:0;letter-spacing:0.03em}
.prot-fields{display:flex;align-items:center;gap:4px;flex:1;min-width:0;flex-wrap:wrap}
.prot-chk{width:16px;height:16px;accent-color:var(--accent);margin:0}
.help-btn{width:18px;height:18px;border-radius:50%;border:1px solid rgba(255,255,255,0.15);background:rgba(255,255,255,0.05);color:var(--dim);font-size:10px;font-weight:700;cursor:pointer;display:inline-flex;align-items:center;justify-content:center;flex-shrink:0;position:relative;outline:none}
.help-btn:hover,.help-btn:focus{color:var(--accent);border-color:rgba(108,140,255,0.4)}
.help-tip{display:none;position:absolute;bottom:calc(100% + 6px);right:-4px;background:#1a1c24;border:1px solid rgba(255,255,255,0.15);border-radius:6px;padding:6px 8px;font-size:11px;color:var(--mid);font-weight:400;width:220px;z-index:10;text-transform:none;letter-spacing:0;line-height:1.3;box-shadow:0 2px 8px rgba(0,0,0,0.4)}
.help-btn:hover .help-tip,.help-btn:focus .help-tip{display:block}
</style>
</head>
<body>

<div class="hdr">
  <div class="hdr-title">
    <div class="hdr-dot" id="connDot"></div>
    <span class="hdr-name">SERVO DRIVER</span>
    <span class="hdr-sub">ESP32</span>
  </div>
  <button class="scan-btn" id="scanBtn" onclick="doScan()">Scan</button>
</div>

<div class="cards" id="cards">
  <div class="no-servos">No servos found &mdash; hit Scan</div>
</div>

<script>
'use strict';
var servos=[],pollTimer=null,scanning=false;

function api(path,cb){
  var x=new XMLHttpRequest();
  x.onreadystatechange=function(){
    if(x.readyState===4){
      if(x.status===200){try{cb(JSON.parse(x.responseText))}catch(e){}}
      else{setConn(false)}
    }
  };
  x.open('GET',path,true);x.timeout=3000;
  x.ontimeout=function(){setConn(false)};
  x.send();
}
function setConn(ok){
  var d=document.getElementById('connDot');
  d.style.background=ok?'var(--green)':'var(--red)';
  d.style.boxShadow=ok?'0 0 6px rgba(80,200,120,0.4)':'0 0 6px rgba(232,93,93,0.4)';
}

function doScan(){
  if(scanning) return; scanning=true;
  document.getElementById('scanBtn').textContent='Scanning\u2026';
  api('/api/rescan',function(){
    pollScanDone();
  });
}
function pollScanDone(){
  api('/api/scan_status',function(st){
    if(st.scanning || !st.finished){
      setTimeout(pollScanDone,500);
      return;
    }
    api('/api/scan',function(d){
      if(d.servos){servos=d.servos.map(function(s){var o=servoById(s.id);return mk(s,o)});renderAll();startPolling();loadTorqueLimits();loadAngleLimits()}
      scanning=false;document.getElementById('scanBtn').textContent='Scan';setConn(true);
    });
  });
}
function initialLoad(){
  api('/api/scan',function(d){
    if(d.servos&&d.servos.length>0){servos=d.servos.map(function(s){return mk(s,null)});renderAll();startPolling();loadTorqueLimits();loadAngleLimits()}
    setConn(true);
  });
}
function mk(s,old){
  return{id:s.id,type:s.type,range:s.range,middle:s.middle,hasCurrent:s.hasCurrent!==false,
    name:s.name||old&&old.name||'',
    pos:0,goal:0,speed:0,load:0,voltage:0,temp:0,current:0,alarm:0,
    mode:0,torque:true,step:old?old.step:10,setpoint:old?old.setpoint:-1,inited:old?old.inited:false,
    speedSet:old?old.speedSet:500,torqueLimit:old?old.torqueLimit:-1,
    limMin:old?old.limMin:-1,limMax:old?old.limMax:-1,
    safetyLoaded:false,maxTemp:-1,minVoltage:-1,maxVoltage:-1,maxTorqueEprom:-1,protCurrent:-1,protTorque:-1,protTime:-1,overloadTorque:-1,overcurrentTime:-1,unloadBits:-1,ledBits:-1};
}
function servoById(id){for(var i=0;i<servos.length;i++)if(servos[i].id===id)return servos[i];return null}

// Polling at 500ms
function startPolling(){if(pollTimer)clearInterval(pollTimer);pollTimer=setInterval(pollStatus,100);pollStatus()}
function pollStatus(){
  api('/api/status_all',function(d){
    if(!d.servos)return;setConn(true);
    for(var i=0;i<d.servos.length;i++){
      var sd=d.servos[i],s=servoById(sd.id);if(!s)continue;
      s.pos=sd.pos;s.goal=sd.goal;s.speed=sd.speed;s.load=sd.load;
      s.voltage=sd.voltage;s.temp=sd.temp;s.current=sd.current;s.alarm=sd.alarm||0;
      s.mode=sd.mode;s.torque=sd.torque;s.range=sd.range;
      if(!s.inited){s.setpoint=s.pos;s.inited=true;rebuildCard(s)}
      updateCard(s);
    }
  });
}

function renderAll(){
  var c=document.getElementById('cards');c.innerHTML='';
  if(!servos.length){c.innerHTML='<div class="no-servos">No servos found &mdash; hit Scan</div>';return}
  for(var i=0;i<servos.length;i++)c.appendChild(buildCard(servos[i]));
}

function buildCard(s){
  var div=document.createElement('div');div.className='card';div.id='card-'+s.id;
  var isM=(s.mode===3),h='';

  // Header: ID + mode + torque
  h+='<div class="card-hdr">';
  h+='<span class="servo-id">#'+s.id+'</span>';
  h+='<input type="text" class="name-inline" id="name-'+s.id+'" value="'+(s.name||'')+'" maxlength="20" placeholder="\u2014" onblur="commitName('+s.id+',this)" onkeydown="if(event.key===\'Enter\')this.blur()">';
  h+='<span class="mode-badge" id="mode-'+s.id+'">'+(isM?'Motor':'Servo')+'</span>';
  h+='<span class="moving-badge" id="moving-'+s.id+'" style="display:none">Moving</span>';
  h+='<span class="alarm-badge" id="alarm-'+s.id+'" style="display:none"></span>';
  h+='<button class="torque-btn '+(s.torque?'torque-on':'torque-off')+'" id="tbtn-'+s.id+'" onclick="toggleTorque('+s.id+')">';
  h+='<span class="torque-dot"></span>Torque</button></div>';

  // Telemetry
  h+='<div class="telem">';
  h+='<span><span class="tl">V </span><span id="tv-'+s.id+'">'+fv(s.voltage)+'</span></span>';
  h+='<span><span class="tl">Load </span><span id="tl-'+s.id+'">'+(s.load/10).toFixed(0)+'%</span></span>';
  h+='<span><span class="tl">Temp </span><span id="tt-'+s.id+'">'+s.temp+'\u00b0</span></span>';
  if(s.hasCurrent)h+='<span><span class="tl">mA </span><span id="tc-'+s.id+'">'+s.current+'</span></span>';
  h+='</div>';

  // Position block
  var tOff=!s.torque;
  h+='<div class="pos-block"'+(tOff?' style="opacity:0.3;pointer-events:none"':'')+'>';
  // Info row: Position | Actual: XXX
  h+='<div class="pos-info">';
  h+='<span class="pos-lbl">Position</span>';
  h+='<span style="font-size:13px;color:var(--dim)">Goal: <span id="goal-'+s.id+'">'+s.goal+'</span></span>';
  h+='<span style="font-size:13px;color:var(--dim)">Actual: <span class="pos-val pos-match" id="actual-'+s.id+'">'+s.pos+'</span></span>';
  h+='</div>';

  if(isM){
    h+='<div class="ctrl-row ctrl-row-center">';
    h+='<button class="jog-lg" onclick="jog('+s.id+',-1)">\u2212</button>';
    h+='<input type="text" class="pos-text pos-text-lg" id="sp-'+s.id+'" value="'+s.setpoint+'" onblur="commitPos('+s.id+',this)" onkeydown="if(event.key===\'Enter\')this.blur()">';
    h+='<button class="jog-lg" onclick="jog('+s.id+',1)">+</button></div>';
  } else {
    h+='<div class="ctrl-row">';
    h+='<button class="jog-sm" onclick="jog('+s.id+',-1)">\u2212</button>';
    h+='<input type="text" class="pos-text" id="sp-'+s.id+'" value="'+s.setpoint+'" onblur="commitPos('+s.id+',this)" onkeydown="if(event.key===\'Enter\')this.blur()">';
    h+='<button class="jog-sm" onclick="jog('+s.id+',1)">+</button>';
    h+='<div style="flex:1;min-width:0;margin-left:8px">';
    h+='<div class="slider-row">';
    h+='<input type="range" min="0" max="'+s.range+'" value="'+s.setpoint+'" id="sl-'+s.id+'" oninput="sliderMove('+s.id+',this.value)" onchange="sliderDone('+s.id+',this.value)">';
    h+='<div class="actual-marker" id="mk-'+s.id+'" style="left:'+pct(s.pos,s.range)+'%"></div>';
h+='<div class="limit-marker limit-min" id="lm-min-'+s.id+'" style="left:'+(s.limMin>=0?pct(s.limMin,s.range):0)+'%;display:'+(s.limMin>=0?'block':'none')+'"></div>';
h+='<div class="limit-marker limit-max" id="lm-max-'+s.id+'" style="left:'+(s.limMax>=0?pct(s.limMax,s.range):100)+'%;display:'+(s.limMax>=0?'block':'none')+'"></div>';
    h+='</div>';
    h+='<div class="slider-bounds"><span>0</span><span>'+s.range+'</span></div>';
    h+='</div></div>';
  }
  h+='<div style="margin-top:4px">'+stepHtml(s)+'</div>';

  // Quick action buttons
  h+='<div class="qa-row">';
  h+='<button class="qa-btn" onclick="quickAction('+s.id+',\'min\')">Min</button>';
  h+='<button class="qa-btn" onclick="quickAction('+s.id+',\'center\')">Center</button>';
  h+='<button class="qa-btn" onclick="quickAction('+s.id+',\'max\')">Max</button>';
  h+='</div>';

  // Speed + torque limit fields
  h+='<div class="fields-row">';
  h+='<div class="field-grp"><span class="field-lbl">Speed</span>';
  h+='<input type="text" class="field-input" id="spd-'+s.id+'" value="'+s.speedSet+'" onblur="commitSpeed('+s.id+',this)" onkeydown="if(event.key===\'Enter\')this.blur()"></div>';
  if(s.type==='STS'){
    var tlWarn=s.torqueLimit===1023||s.torqueLimit===-1;
    h+='<div class="field-grp"><span class="field-lbl">Torque Limit</span>';
    h+='<input type="text" class="field-input'+(tlWarn?' warn':'')+'" id="tl-inp-'+s.id+'" value="'+(s.torqueLimit>=0?(s.torqueLimit/10.23).toFixed(0):'...')+'" onblur="commitTorqueLimit('+s.id+',this)" onkeydown="if(event.key===\'Enter\')this.blur()">';
    h+='<span class="field-lbl">%</span></div>';
  }
  h+='</div>';

  h+='</div>'; // pos-block

  // Config section (collapsible)
  h+='<button class="cfg-toggle" onclick="toggleCfg('+s.id+')">';
  h+='<span class="cfg-arrow" id="cfg-arr-'+s.id+'">\u25b6</span> Config</button>';
  h+='<div class="cfg-body" id="cfg-'+s.id+'">';

  // Angle limits
  h+='<div class="cfg-row">';
  h+='<div class="field-grp"><span class="field-lbl">Min Pos</span>';
  h+='<input type="text" class="field-input" id="cfg-min-'+s.id+'" value="'+(s.limMin>=0?s.limMin:'...')+'"></div>';
  h+='<div class="field-grp"><span class="field-lbl">Max Pos</span>';
  h+='<input type="text" class="field-input" id="cfg-max-'+s.id+'" value="'+(s.limMax>=0?s.limMax:'...')+'"></div>';
  h+='<button class="cfg-save" onclick="saveLimits('+s.id+')">Save Limits</button>';
  h+='<span class="cfg-status" id="cfg-lim-ok-'+s.id+'">Saved \u2713</span>';
  h+='</div>';

  // Safety config (STS and SC)
  var isSTS=s.type==='STS';
  h+='<div class="cfg-row" style="margin-top:4px">';
  h+='<div class="field-grp"><span class="field-lbl">Max Torque</span>';
  h+='<input type="text" class="field-input" id="cfg-maxtorq-'+s.id+'" value="'+(s.maxTorqueEprom>=0?(s.maxTorqueEprom/10).toFixed(0):'...')+'">';
  h+='<span class="field-lbl">%</span></div>';
  h+='<span class="help-btn" tabindex="0">?<span class="help-tip">Hard cap on drive duty cycle. Not a protection\u2014always enforced.</span></span>';
  h+='</div>';
  // Protection header
  h+='<div class="prot-hdr" style="margin-top:6px">';
  h+='<span class="prot-lbl" style="font-weight:700;color:var(--label)">PROTECT</span>';
  h+='<div class="prot-fields"></div>';
  h+='<span class="field-lbl" style="width:16px;text-align:center;font-size:9px">CUT</span>';
  h+='<span class="field-lbl" style="width:16px;text-align:center;font-size:9px">LED</span>';
  h+='<span style="width:18px"></span>';
  h+='</div>';
  // Temp (bit2=4)
  h+='<div class="prot-row">';
  h+='<span class="prot-lbl">Temp</span>';
  h+='<div class="prot-fields">';
  h+='<input type="text" class="field-input" style="width:42px" id="cfg-maxtemp-'+s.id+'" value="'+(s.maxTemp>=0?s.maxTemp:'...')+'">';
  h+='<span class="field-lbl">\u00b0C</span>';
  h+='</div>';
  h+='<input type="checkbox" class="prot-chk" id="cut-temp-'+s.id+'">';
  h+='<input type="checkbox" class="prot-chk" id="led-temp-'+s.id+'">';
  h+='<span class="help-btn" tabindex="0">?<span class="help-tip">Triggers when motor temperature exceeds threshold.</span></span>';
  h+='</div>';
  // Voltage (bit0=1)
  h+='<div class="prot-row">';
  h+='<span class="prot-lbl">Voltage</span>';
  h+='<div class="prot-fields">';
  h+='<input type="text" class="field-input" style="width:42px" id="cfg-minv-'+s.id+'" value="'+(s.minVoltage>=0?(s.minVoltage/10).toFixed(1):'...')+'">';
  h+='<span class="field-lbl">\u2013</span>';
  h+='<input type="text" class="field-input" style="width:42px" id="cfg-maxv-'+s.id+'" value="'+(s.maxVoltage>=0?(s.maxVoltage/10).toFixed(1):'...')+'">';
  h+='<span class="field-lbl">V</span>';
  h+='</div>';
  h+='<input type="checkbox" class="prot-chk" id="cut-volt-'+s.id+'">';
  h+='<input type="checkbox" class="prot-chk" id="led-volt-'+s.id+'">';
  h+='<span class="help-btn" tabindex="0">?<span class="help-tip">Triggers when supply voltage is outside min\u2013max range. Cut is OFF by default (LED only).</span></span>';
  h+='</div>';
  // Current (bit3=8) - STS only
  if(isSTS){
  h+='<div class="prot-row">';
  h+='<span class="prot-lbl">Current</span>';
  h+='<div class="prot-fields">';
  h+='<input type="text" class="field-input" id="cfg-protcur-'+s.id+'" value="'+(s.protCurrent>=0?Math.round(s.protCurrent*6.5):'...')+'">';
  h+='<span class="field-lbl">mA</span>';
  h+='<input type="text" class="field-input" style="width:42px" id="cfg-overcurtime-'+s.id+'" value="'+(s.overcurrentTime>=0?(s.overcurrentTime/100).toFixed(1):'...')+'">';
  h+='<span class="field-lbl">s</span>';
  h+='</div>';
  h+='<input type="checkbox" class="prot-chk" id="cut-cur-'+s.id+'">';
  h+='<input type="checkbox" class="prot-chk" id="led-cur-'+s.id+'">';
  h+='<span class="help-btn" tabindex="0">?<span class="help-tip">Triggers when motor current exceeds threshold for the given duration. Default 3250mA / 2.0s.</span></span>';
  h+='</div>';
  }
  // Overload (bit5=32)
  var timeDiv=isSTS?100:25;
  h+='<div class="prot-row">';
  h+='<span class="prot-lbl">Overload</span>';
  h+='<div class="prot-fields">';
  h+='<input type="text" class="field-input" style="width:38px" id="cfg-overload-'+s.id+'" value="'+(s.overloadTorque>=0?s.overloadTorque:'...')+'">';
  h+='<span class="field-lbl">%</span>';
  h+='<input type="text" class="field-input" style="width:42px" id="cfg-prottime-'+s.id+'" value="'+(s.protTime>=0?(s.protTime/timeDiv).toFixed(1):'...')+'">';
  h+='<span class="field-lbl">s \u2192</span>';
  h+='<input type="text" class="field-input" style="width:38px" id="cfg-prottorq-'+s.id+'" value="'+(s.protTorque>=0?s.protTorque:'...')+'">';
  h+='<span class="field-lbl">%</span>';
  h+='</div>';
  h+='<input type="checkbox" class="prot-chk" id="cut-ol-'+s.id+'">';
  h+='<input type="checkbox" class="prot-chk" id="led-ol-'+s.id+'">';
  h+='<span class="help-btn" tabindex="0">?<span class="help-tip">If drive duty cycle exceeds threshold % for the duration, output drops to lower %. Both are % of max drive output.</span></span>';
  h+='</div>';
  // Angle (bit4=16) - STS only
  if(isSTS){
  h+='<div class="prot-row">';
  h+='<span class="prot-lbl">Angle</span>';
  h+='<div class="prot-fields"></div>';
  h+='<input type="checkbox" class="prot-chk" id="cut-ang-'+s.id+'">';
  h+='<input type="checkbox" class="prot-chk" id="led-ang-'+s.id+'">';
  h+='<span class="help-btn" tabindex="0">?<span class="help-tip">Triggers if position goes outside configured angle limits.</span></span>';
  h+='</div>';
  // Sensor (bit1=2)
  h+='<div class="prot-row">';
  h+='<span class="prot-lbl">Sensor</span>';
  h+='<div class="prot-fields"></div>';
  h+='<input type="checkbox" class="prot-chk" id="cut-sens-'+s.id+'">';
  h+='<input type="checkbox" class="prot-chk" id="led-sens-'+s.id+'">';
  h+='<span class="help-btn" tabindex="0">?<span class="help-tip">Triggers if the magnetic encoder sensor has an error.</span></span>';
  h+='</div>';
  }
  h+='<div class="cfg-row">';
  h+='<button class="cfg-save" onclick="saveSafety('+s.id+')">Save Protection</button>';
  h+='<span class="cfg-status" id="cfg-safety-ok-'+s.id+'"></span>';
  h+='</div>';

  // Set ID
  h+='<div class="cfg-row">';
  h+='<div class="field-grp"><span class="field-lbl">New ID</span>';
  h+='<input type="text" class="field-input" id="cfg-newid-'+s.id+'" value="'+s.id+'"></div>';
  h+='<button class="cfg-save" onclick="saveId('+s.id+')">Set ID</button>';
  h+='<span class="cfg-status" id="cfg-id-ok-'+s.id+'"></span>';
  h+='</div>';

  h+='</div>'; // cfg-body

  div.innerHTML=h;return div;
}

function stepHtml(s){
  var sz=[1,10,100],h='<div class="step-row" id="steps-'+s.id+'"><span class="pm">\u00b1</span>';
  for(var i=0;i<sz.length;i++)h+='<button class="step-btn'+(s.step===sz[i]?' active':'')+'" onclick="setStep('+s.id+','+sz[i]+')">'+sz[i]+'</button>';
  return h+'</div>';
}
function pct(v,r){return r>0?((v/r)*100):0}
function fv(v){return typeof v==='number'?v.toFixed(1):'--'}

function updateCard(s){
  var el;
  el=document.getElementById('mode-'+s.id);
  if(el){var isM=(s.mode===3),wasM=(el.textContent==='Motor');el.textContent=isM?'Motor':'Servo';if(isM!==wasM)rebuildCard(s)}
  el=document.getElementById('moving-'+s.id);if(el)el.style.display=(s.speed!==0)?'inline':'none';
  el=document.getElementById('alarm-'+s.id);if(el){if(s.alarm){var ab=[];if(s.alarm&1)ab.push('VOLT');if(s.alarm&2)ab.push('SENS');if(s.alarm&4)ab.push('TEMP');if(s.alarm&8)ab.push('CUR');if(s.alarm&16)ab.push('ANG');if(s.alarm&32)ab.push('LOAD');el.textContent=ab.join(' ');el.style.display='inline'}else{el.style.display='none'}}
  el=document.getElementById('tv-'+s.id);if(el){el.textContent=fv(s.voltage);el.className=s.voltage<6?'tw':''}
  el=document.getElementById('tl-'+s.id);if(el){el.textContent=(s.load/10).toFixed(0)+'%';el.className=Math.abs(s.load)>500?'tw':''}
  el=document.getElementById('tt-'+s.id);if(el){el.textContent=s.temp+'\u00b0';el.className=s.temp>50?'tw':''}
  el=document.getElementById('tc-'+s.id);if(el)el.textContent=s.current;
  el=document.getElementById('tbtn-'+s.id);if(el)el.className='torque-btn '+(s.torque?'torque-on':'torque-off');
  el=document.getElementById('goal-'+s.id);if(el)el.textContent=s.goal;
  el=document.getElementById('actual-'+s.id);
  if(el){el.textContent=s.pos;el.className='pos-val '+(Math.abs(s.pos-s.goal)>20?'pos-diff':'pos-match')}
  el=document.getElementById('sl-'+s.id);if(el&&document.activeElement!==el)el.value=s.setpoint;
  el=document.getElementById('mk-'+s.id);if(el)el.style.left=pct(s.pos,s.range)+'%';
  el=document.getElementById('sp-'+s.id);if(el&&document.activeElement!==el)el.value=s.setpoint;
}
function rebuildCard(s){var old=document.getElementById('card-'+s.id);if(!old)return;old.parentNode.replaceChild(buildCard(s),old)}

var slT={};
function sliderMove(id,val){
  var s=servoById(id);if(!s)return;val=parseInt(val);s.setpoint=val;
  var el=document.getElementById('sp-'+id);if(el)el.value=val;
  var ae=document.getElementById('actual-'+id);if(ae)ae.className='pos-val pos-diff';
  if(!slT[id]){slT[id]=setTimeout(function(){sendPos(id,s.setpoint);slT[id]=null},50)}
}
function sliderDone(id,val){var s=servoById(id);if(!s)return;s.setpoint=parseInt(val);sendPos(id,s.setpoint)}

function commitPos(id,el){
  var s=servoById(id);if(!s)return;
  var v=Math.max(0,Math.min(s.range,parseInt(el.value)||0));
  s.setpoint=v;el.value=v;
  var sl=document.getElementById('sl-'+id);if(sl)sl.value=v;
  sendPos(id,v);
}

function jog(id,dir){
  var s=servoById(id);if(!s)return;
  var v=Math.max(0,Math.min(s.range,s.setpoint+dir*s.step));s.setpoint=v;
  var el=document.getElementById('sp-'+id);if(el)el.value=v;
  var sl=document.getElementById('sl-'+id);if(sl)sl.value=v;
  sendPos(id,v);
}

function setStep(id,size){
  var s=servoById(id);if(!s)return;s.step=size;
  var row=document.getElementById('steps-'+id);if(!row)return;
  var btns=row.querySelectorAll('.step-btn');
  for(var i=0;i<btns.length;i++){var bs=parseInt(btns[i].textContent);btns[i].className='step-btn'+(bs===size?' active':'')}
}

function toggleTorque(id){
  var s=servoById(id);if(!s)return;s.torque=!s.torque;
  if(s.torque){s.setpoint=s.pos}
  rebuildCard(s);
  var x=new XMLHttpRequest();x.open('GET','/api/torque?id='+id+'&enable='+(s.torque?'1':'0'),true);x.send();
}

function quickAction(id,act){
  var s=servoById(id);if(!s)return;
  if(act==='center'){s.setpoint=s.middle;sendPos(id,s.setpoint);syncUI(id)}
  else if(act==='min'){s.setpoint=0;sendPos(id,s.setpoint);syncUI(id)}
  else if(act==='max'){s.setpoint=s.range;sendPos(id,s.setpoint);syncUI(id)}

}
function syncUI(id){
  var s=servoById(id);if(!s)return;
  var el=document.getElementById('sp-'+id);if(el)el.value=s.setpoint;
  var sl=document.getElementById('sl-'+id);if(sl)sl.value=s.setpoint;
}

function commitSpeed(id,el){
  var s=servoById(id);if(!s)return;
  var v=Math.max(0,parseInt(el.value)||0);s.speedSet=v;el.value=v;
}

function commitTorqueLimit(id,el){
  var s=servoById(id);if(!s)return;
  var pct=Math.max(0,Math.min(100,parseFloat(el.value)||0));
  var v=Math.round(pct*10.23);s.torqueLimit=v;el.value=pct.toFixed(0);
  el.className='field-input'+(v>=1023?' warn':'');
  api('/api/torque_limit?id='+id+'&value='+v,function(){});
}

function sendPos(id,pos){var s=servoById(id);var spd=s?s.speedSet:500;var x=new XMLHttpRequest();x.open('GET','/api/setpos?id='+id+'&pos='+pos+'&speed='+spd,true);x.send()}

// Load torque limits for STS servos after scan
function loadTorqueLimits(){
  for(var i=0;i<servos.length;i++){
    if(servos[i].type==='STS')(function(s){
      api('/api/torque_limit?id='+s.id,function(d){if(d.value!==undefined){s.torqueLimit=d.value;var el=document.getElementById('tl-inp-'+s.id);if(el){el.value=(d.value/10.23).toFixed(0);el.className='field-input'+(d.value>=1023?' warn':'')}}});
    })(servos[i]);
  }
}

// Load angle limits for all servos after scan
function loadAngleLimits(){
  for(var i=0;i<servos.length;i++){(function(s){
    api('/api/angle_limits?id='+s.id,function(d){
      if(d.min!==undefined){s.limMin=d.min;s.limMax=d.max;
        var el=document.getElementById('cfg-min-'+s.id);if(el)el.value=d.min;
        el=document.getElementById('cfg-max-'+s.id);if(el)el.value=d.max;
        updateLimitMarkers(s)}
    });
  })(servos[i])}
}

function toggleCfg(id){
  var body=document.getElementById('cfg-'+id);
  var arr=document.getElementById('cfg-arr-'+id);
  if(!body)return;
  var open=body.classList.toggle('open');
  if(arr)arr.classList.toggle('open',open);
  // Lazy-load limits on first open
  var s=servoById(id);
  if(open&&s){
    if(s.limMin<0)loadAngleLimitsFor(s);
    if(!s.safetyLoaded)loadSafetyFor(s);
  }
}
function loadAngleLimitsFor(s){
  api('/api/angle_limits?id='+s.id,function(d){
    if(d.min!==undefined){s.limMin=d.min;s.limMax=d.max;
      var el=document.getElementById('cfg-min-'+s.id);if(el)el.value=d.min;
      el=document.getElementById('cfg-max-'+s.id);if(el)el.value=d.max;
      updateLimitMarkers(s)}
  });
}
function updateLimitMarkers(s){
  var mn=document.getElementById('lm-min-'+s.id);
  var mx=document.getElementById('lm-max-'+s.id);
  if(mn){mn.style.left=pct(s.limMin,s.range)+'%';mn.style.display=s.limMin>=0?'block':'none'}
  if(mx){mx.style.left=pct(s.limMax,s.range)+'%';mx.style.display=s.limMax>=0?'block':'none'}
}

function flashStatus(elId,msg){
  var el=document.getElementById(elId);if(!el)return;
  el.textContent=msg;el.classList.add('show');
  setTimeout(function(){el.classList.remove('show')},2000);
}

function saveLimits(id){
  var s=servoById(id);if(!s)return;
  var minEl=document.getElementById('cfg-min-'+id);
  var maxEl=document.getElementById('cfg-max-'+id);
  if(!minEl||!maxEl)return;
  var minV=parseInt(minEl.value)||0;
  var maxV=parseInt(maxEl.value)||0;
  api('/api/angle_limits?id='+id+'&min='+minV+'&max='+maxV,function(d){
    if(d.ok){
      flashStatus('cfg-lim-ok-'+id,'Saved \u2713');
      // Read back to confirm and update markers
      api('/api/angle_limits?id='+id,function(rd){
        if(rd.min!==undefined){s.limMin=rd.min;s.limMax=rd.max;
          minEl.value=rd.min;maxEl.value=rd.max;
          updateLimitMarkers(s)}
      });
    }
    else{flashStatus('cfg-lim-ok-'+id,'Error \u2717')}
  });
}

function loadSafetyFor(s){
  api('/api/safety?id='+s.id,function(d){
    if(d.max_temp===undefined)return;
    var isSTS=s.type==='STS';
    var timeDiv=isSTS?100:25;
    s.safetyLoaded=true;
    s.maxTemp=d.max_temp;s.maxVoltage=d.max_voltage;s.minVoltage=d.min_voltage;
    s.maxTorqueEprom=d.max_torque;
    s.protTorque=d.prot_torque;s.protTime=d.prot_time;s.overloadTorque=d.overload_torque;
    s.unloadBits=d.unload;s.ledBits=d.led_alarm;
    if(isSTS){s.protCurrent=d.prot_current;s.overcurrentTime=d.overcurrent_time;}
    var el;
    el=document.getElementById('cfg-maxtemp-'+s.id);if(el)el.value=d.max_temp;
    el=document.getElementById('cfg-minv-'+s.id);if(el)el.value=(d.min_voltage/10).toFixed(1);
    el=document.getElementById('cfg-maxv-'+s.id);if(el)el.value=(d.max_voltage/10).toFixed(1);
    el=document.getElementById('cfg-maxtorq-'+s.id);if(el)el.value=(d.max_torque/10).toFixed(0);
    if(isSTS){
      el=document.getElementById('cfg-protcur-'+s.id);if(el)el.value=Math.round(d.prot_current*6.5);
      el=document.getElementById('cfg-overcurtime-'+s.id);if(el)el.value=(d.overcurrent_time/100).toFixed(1);
    }
    el=document.getElementById('cfg-overload-'+s.id);if(el)el.value=d.overload_torque;
    el=document.getElementById('cfg-prottime-'+s.id);if(el)el.value=(d.prot_time/timeDiv).toFixed(1);
    el=document.getElementById('cfg-prottorq-'+s.id);if(el)el.value=d.prot_torque;
    var u=d.unload,l=d.led_alarm;
    var bits=isSTS?[['temp',4],['volt',1],['cur',8],['ol',32],['ang',16],['sens',2]]:[['temp',4],['volt',1],['ol',32]];
    for(var i=0;i<bits.length;i++){var b=bits[i];
      el=document.getElementById('cut-'+b[0]+'-'+s.id);if(el)el.checked=!!(u&b[1]);
      el=document.getElementById('led-'+b[0]+'-'+s.id);if(el)el.checked=!!(l&b[1]);
    }
  });
}
function saveSafety(id){
  var s=servoById(id);if(!s)return;
  var isSTS=s.type==='STS';
  var timeDiv=isSTS?100:25;
  var mt=parseInt(document.getElementById('cfg-maxtemp-'+id).value)||0;
  var minv=Math.round(parseFloat(document.getElementById('cfg-minv-'+id).value||0)*10);
  var maxv=Math.round(parseFloat(document.getElementById('cfg-maxv-'+id).value||0)*10);
  var mtq=Math.round((parseFloat(document.getElementById('cfg-maxtorq-'+id).value)||0)*10);
  var ol=parseInt(document.getElementById('cfg-overload-'+id).value)||0;
  var ptm=Math.round(parseFloat(document.getElementById('cfg-prottime-'+id).value||0)*timeDiv);
  var pt=parseInt(document.getElementById('cfg-prottorq-'+id).value)||0;
  var pc=0,oct=0;
  if(isSTS){
    pc=Math.round((parseInt(document.getElementById('cfg-protcur-'+id).value)||0)/6.5);
    oct=Math.round(parseFloat(document.getElementById('cfg-overcurtime-'+id).value||0)*100);
  }
  var unload=0,led=0;
  var bits=isSTS?[['temp',4],['volt',1],['cur',8],['ol',32],['ang',16],['sens',2]]:[['temp',4],['volt',1],['ol',32]];
  for(var i=0;i<bits.length;i++){var b=bits[i];
    var ce=document.getElementById('cut-'+b[0]+'-'+id);if(ce&&ce.checked)unload|=b[1];
    var le=document.getElementById('led-'+b[0]+'-'+id);if(le&&le.checked)led|=b[1];
  }
  var url='/api/safety?id='+id+'&max_temp='+mt+'&max_voltage='+maxv+'&min_voltage='+minv+'&max_torque='+mtq+'&prot_torque='+pt+'&prot_time='+ptm+'&overload_torque='+ol+'&unload='+unload+'&led_alarm='+led;
  if(isSTS)url+='&prot_current='+pc+'&overcurrent_time='+oct;
  api(url,function(d){
    if(d.ok){flashStatus('cfg-safety-ok-'+id,'Saved \u2713');loadSafetyFor(s)}
    else{flashStatus('cfg-safety-ok-'+id,'Error \u2717')}
  });
}
function commitName(id,el){
  var s=servoById(id);if(!s)return;
  var name=el.value.trim().substring(0,20);
  if(name===s.name)return;
  api('/api/set_name?id='+id+'&name='+encodeURIComponent(name),function(d){
    if(d.ok)s.name=name;
  });
}
function saveId(id){
  var s=servoById(id);if(!s)return;
  var el=document.getElementById('cfg-newid-'+id);if(!el)return;
  var newId=parseInt(el.value);
  if(isNaN(newId)||newId<0||newId>253){flashStatus('cfg-id-ok-'+id,'Invalid');return}
  if(newId===id){flashStatus('cfg-id-ok-'+id,'Same ID');return}
  if(!confirm('Change servo ID from '+id+' to '+newId+'?'))return;
  api('/api/set_id?id='+id+'&new_id='+newId,function(d){
    if(d.ok){
      s.id=newId;
      flashStatus('cfg-id-ok-'+id,'Now #'+newId);
      // Reload after brief delay so card rebuilds with new ID
      setTimeout(function(){
        api('/api/scan',function(dd){
          if(dd.servos){servos=dd.servos.map(function(sv){var o=servoById(sv.id);return mk(sv,o)});renderAll();startPolling()}
        });
      },500);
    } else {flashStatus('cfg-id-ok-'+id,'Failed')}
  });
}

initialLoad();
</script>
</body>
</html>
)rawliteral";
