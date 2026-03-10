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
--bg:#0c0e13;--card:#13151b;--border:rgba(255,255,255,0.05);
--text:#e2e6f0;--dim:#555b6e;--label:#666e82;--faint:#3d4250;
--accent:#6c8cff;--green:#50c878;--orange:#e8943a;--red:#e85d5d;
--mono:'SF Mono','Fira Code','Consolas','Courier New',monospace;
--sans:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;
}
html{background:var(--bg);color:var(--text);font-family:var(--sans)}
body{max-width:680px;margin:0 auto;padding:0 12px 40px}
button{font-family:inherit;cursor:pointer;border:none;outline:none;-webkit-tap-highlight-color:transparent}
button:hover{filter:brightness(1.15)}
input{font-family:var(--mono)}

/* Header */
.hdr{display:flex;align-items:center;justify-content:space-between;padding:12px 4px;border-bottom:1px solid var(--border)}
.hdr-title{display:flex;align-items:center;gap:8px}
.hdr-dot{width:7px;height:7px;border-radius:50%;background:var(--green);box-shadow:0 0 6px rgba(80,200,120,0.4)}
.hdr-name{font-size:13px;font-weight:700;letter-spacing:0.04em}
.hdr-sub{font-size:10px;color:var(--faint);font-family:var(--mono)}
.scan-btn{padding:5px 14px;border-radius:6px;font-size:11px;font-weight:600;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.06);color:#9499ad}

/* Cards container */
.cards{display:flex;flex-direction:column;gap:10px;margin-top:10px}
.no-servos{text-align:center;padding:60px 20px;color:var(--faint)}

/* Card */
.card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:14px 16px}
@media(min-width:400px){.card{padding:16px 20px}}

/* Card header */
.card-hdr{display:flex;align-items:center;gap:8px;margin-bottom:12px;flex-wrap:wrap}
.servo-id{font-size:16px;font-family:var(--mono);font-weight:800}
.mode-badge{font-size:9px;padding:2px 7px;border-radius:4px;font-weight:600;background:rgba(108,140,255,0.1);color:var(--accent)}
.moving-badge{font-size:9px;padding:2px 7px;border-radius:4px;font-weight:600;background:rgba(80,200,120,0.1);color:var(--green);animation:pulse 1.5s ease-in-out infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}
.telem{margin-left:auto;display:flex;align-items:baseline;gap:10px;flex-wrap:wrap}
.telem span{font-size:11px;font-family:var(--mono);color:var(--dim)}
.telem .tl{font-size:9px;color:var(--faint);margin-right:2px}
.telem .tw{color:var(--orange)}

/* Torque toggle */
.torque-btn{display:inline-flex;align-items:center;gap:5px;padding:5px 10px;border-radius:6px;font-size:11px;font-weight:700;letter-spacing:0.02em;transition:all .15s}
.torque-on{background:rgba(80,200,120,0.12);color:var(--green)}
.torque-off{background:rgba(255,255,255,0.04);color:var(--dim)}
.torque-dot{width:6px;height:6px;border-radius:50%;transition:all .15s}
.torque-on .torque-dot{background:var(--green);box-shadow:0 0 6px rgba(80,200,120,0.4)}
.torque-off .torque-dot{background:#444857}

/* Position section */
.pos-hdr{display:flex;align-items:baseline;justify-content:space-between;margin-bottom:4px}
.pos-label{font-size:10px;font-weight:600;color:var(--label);text-transform:uppercase;letter-spacing:0.08em}
.pos-actual{font-size:10px;color:var(--faint)}
.pos-actual-val{font-family:var(--mono);font-weight:700;font-size:12px}
.pos-match{color:var(--green)}
.pos-diff{color:var(--orange)}

/* Slider row */
.slider-row{position:relative}
.slider-input-row{display:flex;align-items:center;justify-content:flex-end;gap:6px;margin-bottom:6px}
input[type=range]{width:100%;height:4px;-webkit-appearance:none;appearance:none;border-radius:2px;outline:none;cursor:pointer;background:#1a1d25}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;border-radius:50%;background:var(--text);border:2px solid var(--accent);cursor:pointer;box-shadow:0 0 5px rgba(108,140,255,0.25)}
.slider-bounds{display:flex;justify-content:space-between;align-items:center;margin-top:2px}
.slider-bounds span{font-size:8px;color:var(--faint)}

/* Actual position marker */
.actual-marker{position:absolute;bottom:18px;width:2px;height:8px;background:var(--green);border-radius:1px;opacity:0.7;pointer-events:none;transition:left 0.3s}

/* Jog button (small, beside text input) */
.jog-sm{width:28px;height:28px;border-radius:6px;font-size:14px;font-weight:700;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);color:#9499ad;display:flex;align-items:center;justify-content:center;font-family:var(--mono);padding:0;transition:all .12s;line-height:1}

/* Jog button (large, motor mode) */
.jog-lg{width:48px;height:48px;border-radius:10px;font-size:22px;font-weight:700;background:rgba(108,140,255,0.08);border:1px solid rgba(108,140,255,0.2);color:var(--accent);display:flex;align-items:center;justify-content:center;font-family:var(--mono);padding:0;transition:all .12s;line-height:1}
.jog-center{display:flex;align-items:center;justify-content:center;gap:12px;margin-top:4px}

/* Direct text input */
.pos-text{background:transparent;border:none;border-bottom:1.5px solid #282c37;color:var(--text);font-size:15px;font-family:var(--mono);font-weight:700;width:58px;text-align:center;outline:none;padding:0 0 1px;transition:border-color .15s}
.pos-text:focus{border-bottom-color:var(--accent)}
.pos-text-lg{font-size:24px;font-weight:800;width:80px;border-bottom-width:2px}

/* Step selector */
.step-row{display:flex;gap:2px;align-items:center}
.step-row .pm{font-size:9px;color:var(--faint);margin-right:3px}
.step-btn{padding:2px 6px;border-radius:4px;font-size:9px;font-weight:600;background:transparent;border:1px solid transparent;color:#4a4f62;font-family:var(--mono);transition:all .12s}
.step-btn.active{background:rgba(108,140,255,0.15);border-color:rgba(108,140,255,0.25);color:var(--accent)}
</style>
</head>
<body>

<!-- Header -->
<div class="hdr">
  <div class="hdr-title">
    <div class="hdr-dot" id="connDot"></div>
    <span class="hdr-name">SERVO DRIVER</span>
    <span class="hdr-sub">ESP32</span>
  </div>
  <button class="scan-btn" id="scanBtn" onclick="doScan()">Scan</button>
</div>

<!-- Cards -->
<div class="cards" id="cards">
  <div class="no-servos" id="emptyMsg">
    <div style="font-size:14px;margin-bottom:8px">No servos found</div>
    <div style="font-size:11px">Hit Scan to search for connected devices</div>
  </div>
</div>

<script>
'use strict';
var servos=[], pollTimer=null, scanning=false;

// --- API helpers ---
function api(path,cb){
  var x=new XMLHttpRequest();
  x.onreadystatechange=function(){
    if(x.readyState===4){
      if(x.status===200){try{cb(JSON.parse(x.responseText))}catch(e){}}
      else{setConn(false)}
    }
  };
  x.open('GET',path,true);
  x.timeout=3000;
  x.ontimeout=function(){setConn(false)};
  x.send();
}

function setConn(ok){
  document.getElementById('connDot').style.background=ok?'var(--green)':'var(--red)';
  document.getElementById('connDot').style.boxShadow=ok?'0 0 6px rgba(80,200,120,0.4)':'0 0 6px rgba(232,93,93,0.4)';
}

// --- Scan ---
function doScan(){
  if(scanning) return;
  scanning=true;
  var btn=document.getElementById('scanBtn');
  btn.textContent='Scanning\u2026';
  api('/api/rescan',function(){
    // Wait for scan to finish, then fetch results
    setTimeout(function pollScan(){
      api('/api/scan',function(d){
        if(d.servos){
          servos=d.servos.map(function(s){
            // Preserve step per servo
            var old=servoById(s.id);
            return{id:s.id,type:s.type,range:s.range,middle:s.middle,
                   pos:0,speed:0,load:0,voltage:0,temp:0,current:0,
                   mode:0,torque:true,step:old?old.step:10,setpoint:s.middle};
          });
          renderAll();
          startPolling();
        }
        scanning=false;
        btn.textContent='Scan';
        setConn(true);
      });
    },3000);
  });
}

// Initial load: fetch scan data without triggering rescan
function initialLoad(){
  api('/api/scan',function(d){
    if(d.servos && d.servos.length>0){
      servos=d.servos.map(function(s){
        return{id:s.id,type:s.type,range:s.range,middle:s.middle,
               pos:0,speed:0,load:0,voltage:0,temp:0,current:0,
               mode:0,torque:true,step:10,setpoint:s.middle};
      });
      renderAll();
      startPolling();
    }
    setConn(true);
  });
}

function servoById(id){
  for(var i=0;i<servos.length;i++) if(servos[i].id===id) return servos[i];
  return null;
}

// --- Polling ---
function startPolling(){
  if(pollTimer) clearInterval(pollTimer);
  pollTimer=setInterval(pollStatus,800);
  pollStatus();
}

function pollStatus(){
  api('/api/status_all',function(d){
    if(!d.servos) return;
    setConn(true);
    for(var i=0;i<d.servos.length;i++){
      var sd=d.servos[i], s=servoById(sd.id);
      if(!s) continue;
      s.pos=sd.pos; s.speed=sd.speed; s.load=sd.load;
      s.voltage=sd.voltage; s.temp=sd.temp; s.current=sd.current;
      s.mode=sd.mode; s.torque=sd.torque; s.range=sd.range;
      updateCard(s);
    }
  });
}

// --- Render all cards ---
function renderAll(){
  var c=document.getElementById('cards');
  c.innerHTML='';
  if(servos.length===0){
    c.innerHTML='<div class="no-servos"><div style="font-size:14px;margin-bottom:8px">No servos found</div><div style="font-size:11px">Hit Scan to search for connected devices</div></div>';
    return;
  }
  for(var i=0;i<servos.length;i++) c.appendChild(buildCard(servos[i]));
}

// --- Build a single card ---
function buildCard(s){
  var div=document.createElement('div');
  div.className='card';
  div.id='card-'+s.id;
  var isMotor=(s.mode===3);

  // Card header
  var hdr='<div class="card-hdr">';
  hdr+='<span class="servo-id">#'+s.id+'</span>';
  hdr+='<span class="mode-badge" id="mode-'+s.id+'">'+(isMotor?'Motor':'Servo')+'</span>';
  hdr+='<span class="moving-badge" id="moving-'+s.id+'" style="display:none">Moving</span>';
  hdr+='<div class="telem">';
  hdr+='<span><span class="tl">V</span><span id="tv-'+s.id+'">'+s.voltage.toFixed(1)+'</span></span>';
  hdr+='<span><span class="tl">Load</span><span id="tl-'+s.id+'">'+s.load+'</span></span>';
  hdr+='<span><span class="tl">Temp</span><span id="tt-'+s.id+'">'+s.temp+'\u00b0</span></span>';
  hdr+='<span><span class="tl">mA</span><span id="tc-'+s.id+'">'+s.current+'</span></span>';
  hdr+='</div>';
  hdr+='<button class="torque-btn '+(s.torque?'torque-on':'torque-off')+'" id="tbtn-'+s.id+'" onclick="toggleTorque('+s.id+')">';
  hdr+='<span class="torque-dot"></span>Torque</button>';
  hdr+='</div>';

  // Position header (actual)
  var pos='<div class="pos-hdr">';
  pos+='<div style="display:flex;align-items:baseline;gap:10px">';
  pos+='<span class="pos-label">Position</span>';
  pos+='<span class="pos-actual">Actual: <span class="pos-actual-val pos-match" id="actual-'+s.id+'">'+s.pos+'</span></span>';
  pos+='</div></div>';

  // Position control
  var ctrl='';
  if(isMotor){
    // Motor mode: jog-only
    ctrl+='<div class="jog-center">';
    ctrl+='<button class="jog-lg" onclick="jog('+s.id+',-1)">\u2212</button>';
    ctrl+='<input type="text" class="pos-text pos-text-lg" id="sp-'+s.id+'" value="'+s.setpoint+'" onblur="commitPos('+s.id+',this)" onkeydown="if(event.key===\'Enter\')this.blur()">';
    ctrl+='<button class="jog-lg" onclick="jog('+s.id+',1)">+</button>';
    ctrl+='</div>';
    ctrl+='<div style="display:flex;justify-content:center;margin-top:8px">';
    ctrl+=stepHtml(s);
    ctrl+='</div>';
  } else {
    // Servo mode: slider + jog + text input
    ctrl+='<div class="slider-input-row">';
    ctrl+='<button class="jog-sm" onclick="jog('+s.id+',-1)">\u2212</button>';
    ctrl+='<input type="text" class="pos-text" id="sp-'+s.id+'" value="'+s.setpoint+'" onblur="commitPos('+s.id+',this)" onkeydown="if(event.key===\'Enter\')this.blur()">';
    ctrl+='<button class="jog-sm" onclick="jog('+s.id+',1)">+</button>';
    ctrl+='</div>';
    ctrl+='<div class="slider-row">';
    ctrl+='<input type="range" min="0" max="'+s.range+'" value="'+s.setpoint+'" id="sl-'+s.id+'" oninput="sliderMove('+s.id+',this.value)" onchange="sliderDone('+s.id+',this.value)">';
    ctrl+='<div class="actual-marker" id="mk-'+s.id+'" style="left:'+pct(s.pos,s.range)+'%"></div>';
    ctrl+='</div>';
    ctrl+='<div class="slider-bounds"><span>0</span>';
    ctrl+=stepHtml(s);
    ctrl+='<span>'+s.range+'</span></div>';
  }

  div.innerHTML=hdr+pos+ctrl;
  return div;
}

function stepHtml(s){
  var sizes=[1,10,100];
  var h='<div class="step-row" id="steps-'+s.id+'"><span class="pm">\u00b1</span>';
  for(var i=0;i<sizes.length;i++){
    h+='<button class="step-btn'+(s.step===sizes[i]?' active':'')+'" onclick="setStep('+s.id+','+sizes[i]+')">'+sizes[i]+'</button>';
  }
  h+='</div>';
  return h;
}

function pct(val,range){return range>0?((val/range)*100):0}

// --- Card update (telemetry, no full rebuild) ---
function updateCard(s){
  var el;
  // Mode badge
  el=document.getElementById('mode-'+s.id);
  if(el){
    var isMotor=(s.mode===3);
    var wasMotor=(el.textContent==='Motor');
    el.textContent=isMotor?'Motor':'Servo';
    // If mode changed, rebuild the card
    if(isMotor!==wasMotor) rebuildCard(s);
  }
  // Moving badge
  el=document.getElementById('moving-'+s.id);
  if(el) el.style.display=(s.speed!==0)?'inline':'none';
  // Telemetry
  el=document.getElementById('tv-'+s.id);
  if(el){el.textContent=s.voltage.toFixed(1);el.parentElement.className=s.voltage<6?'tw':''}
  el=document.getElementById('tl-'+s.id);
  if(el){el.textContent=s.load;el.parentElement.className=Math.abs(s.load)>50?'tw':''}
  el=document.getElementById('tt-'+s.id);
  if(el){el.textContent=s.temp+'\u00b0';el.parentElement.className=s.temp>50?'tw':''}
  el=document.getElementById('tc-'+s.id);
  if(el) el.textContent=s.current;
  // Torque button
  el=document.getElementById('tbtn-'+s.id);
  if(el) el.className='torque-btn '+(s.torque?'torque-on':'torque-off');
  // Actual position
  el=document.getElementById('actual-'+s.id);
  if(el){
    el.textContent=s.pos;
    var diff=Math.abs(s.pos-s.setpoint);
    el.className='pos-actual-val '+(diff>20?'pos-diff':'pos-match');
  }
  // Slider value (don't update if user is dragging)
  el=document.getElementById('sl-'+s.id);
  if(el && document.activeElement!==el) el.value=s.setpoint;
  // Actual marker
  el=document.getElementById('mk-'+s.id);
  if(el) el.style.left=pct(s.pos,s.range)+'%';
  // Setpoint text (don't update if focused)
  el=document.getElementById('sp-'+s.id);
  if(el && document.activeElement!==el) el.value=s.setpoint;
}

function rebuildCard(s){
  var old=document.getElementById('card-'+s.id);
  if(!old) return;
  var nw=buildCard(s);
  old.parentNode.replaceChild(nw,old);
}

// --- Slider interaction ---
var sliderSendTimer={};
function sliderMove(id,val){
  var s=servoById(id);if(!s) return;
  val=parseInt(val);
  s.setpoint=val;
  var el=document.getElementById('sp-'+id);
  if(el) el.value=val;
  // Throttle sends to ~50ms
  if(!sliderSendTimer[id]){
    sliderSendTimer[id]=setTimeout(function(){
      sendPos(id,s.setpoint);
      sliderSendTimer[id]=null;
    },50);
  }
}

function sliderDone(id,val){
  var s=servoById(id);if(!s) return;
  s.setpoint=parseInt(val);
  sendPos(id,s.setpoint);
}

// --- Text input commit ---
function commitPos(id,el){
  var s=servoById(id);if(!s) return;
  var v=parseInt(el.value)||0;
  v=Math.max(0,Math.min(s.range,v));
  s.setpoint=v;
  el.value=v;
  var sl=document.getElementById('sl-'+id);
  if(sl) sl.value=v;
  sendPos(id,v);
}

// --- Jog ---
function jog(id,dir){
  var s=servoById(id);if(!s) return;
  var v=s.setpoint+dir*s.step;
  v=Math.max(0,Math.min(s.range,v));
  s.setpoint=v;
  var el=document.getElementById('sp-'+id);if(el) el.value=v;
  var sl=document.getElementById('sl-'+id);if(sl) sl.value=v;
  sendPos(id,v);
}

// --- Step selector ---
function setStep(id,size){
  var s=servoById(id);if(!s) return;
  s.step=size;
  // Update button states
  var row=document.getElementById('steps-'+id);
  if(!row) return;
  var btns=row.querySelectorAll('.step-btn');
  for(var i=0;i<btns.length;i++){
    var bsize=parseInt(btns[i].textContent);
    btns[i].className='step-btn'+(bsize===size?' active':'');
  }
}

// --- Torque toggle ---
function toggleTorque(id){
  var s=servoById(id);if(!s) return;
  var cmd=s.torque?'cmd?inputT=1&inputI=3&inputA=0&inputB=0':'cmd?inputT=1&inputI=4&inputA=0&inputB=0';
  // Need to select this servo first, then send torque command
  // Use the legacy /cmd endpoint which operates on the "active" servo
  // Better: use a dedicated API. For now, directly toggle via the old mechanism
  // by temporarily selecting the servo
  s.torque=!s.torque;
  updateCard(s);
  // Send via setpos-style API or legacy cmd - we need a torque API
  // For now: use legacy system
  var x=new XMLHttpRequest();
  x.open('GET','/api/torque?id='+id+'&enable='+(s.torque?'1':'0'),true);
  x.send();
}

// --- Send position ---
function sendPos(id,pos){
  var x=new XMLHttpRequest();
  x.open('GET','/api/setpos?id='+id+'&pos='+pos+'&speed=500',true);
  x.send();
}

// --- Init ---
initialLoad();
</script>
</body>
</html>
)rawliteral";