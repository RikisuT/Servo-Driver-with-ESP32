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
.mode-badge{font-size:11px;padding:2px 8px;border-radius:4px;font-weight:700;background:rgba(108,140,255,0.12);color:var(--accent)}
.moving-badge{font-size:11px;padding:2px 8px;border-radius:4px;font-weight:700;background:rgba(80,200,120,0.12);color:var(--green);animation:pulse 1.5s ease-in-out infinite}
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

/* Step selector */
.step-row{display:flex;gap:3px;align-items:center}
.step-row .pm{font-size:12px;color:var(--dim);margin-right:2px}
.step-btn{padding:3px 9px;border-radius:5px;font-size:13px;font-weight:600;background:transparent;border:1px solid transparent;color:var(--faint);font-family:var(--mono);transition:all .12s}
.step-btn.active{background:rgba(108,140,255,0.15);border-color:rgba(108,140,255,0.25);color:var(--accent)}
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
      if(d.servos){servos=d.servos.map(function(s){var o=servoById(s.id);return mk(s,o)});renderAll();startPolling()}
      scanning=false;document.getElementById('scanBtn').textContent='Scan';setConn(true);
    });
  });
}
function initialLoad(){
  api('/api/scan',function(d){
    if(d.servos&&d.servos.length>0){servos=d.servos.map(function(s){return mk(s,null)});renderAll();startPolling()}
    setConn(true);
  });
}
function mk(s,old){
  return{id:s.id,type:s.type,range:s.range,middle:s.middle,hasCurrent:s.hasCurrent!==false,
    pos:0,goal:0,speed:0,load:0,voltage:0,temp:0,current:0,
    mode:0,torque:true,step:old?old.step:10,setpoint:old?old.setpoint:-1,inited:old?old.inited:false};
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
      s.voltage=sd.voltage;s.temp=sd.temp;s.current=sd.current;
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
  h+='<span class="mode-badge" id="mode-'+s.id+'">'+(isM?'Motor':'Servo')+'</span>';
  h+='<span class="moving-badge" id="moving-'+s.id+'" style="display:none">Moving</span>';
  h+='<button class="torque-btn '+(s.torque?'torque-on':'torque-off')+'" id="tbtn-'+s.id+'" onclick="toggleTorque('+s.id+')">';
  h+='<span class="torque-dot"></span>Torque</button></div>';

  // Telemetry
  h+='<div class="telem">';
  h+='<span><span class="tl">V </span><span id="tv-'+s.id+'">'+fv(s.voltage)+'</span></span>';
  h+='<span><span class="tl">Load </span><span id="tl-'+s.id+'">'+s.load+'</span></span>';
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
    h+='</div>';
    h+='<div class="slider-bounds"><span>0</span><span>'+s.range+'</span></div>';
    h+='</div></div>';
  }
  h+='<div style="margin-top:4px">'+stepHtml(s)+'</div>';
  h+='</div>'; // pos-block

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
  el=document.getElementById('tv-'+s.id);if(el){el.textContent=fv(s.voltage);el.className=s.voltage<6?'tw':''}
  el=document.getElementById('tl-'+s.id);if(el){el.textContent=s.load;el.className=Math.abs(s.load)>50?'tw':''}
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

function sendPos(id,pos){var x=new XMLHttpRequest();x.open('GET','/api/setpos?id='+id+'&pos='+pos+'&speed=500',true);x.send()}

initialLoad();
</script>
</body>
</html>
)rawliteral";
