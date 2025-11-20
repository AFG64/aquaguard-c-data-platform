/* DOM hooks */
const flowEl = document.getElementById('flow');
const humEl = document.getElementById('humidity');
const tempEl = document.getElementById('temperature');
const pressureEl = document.getElementById('pressure');
const bannerEl = document.getElementById('banner');
const bannerText = document.getElementById('banner-text');
const bannerTitle = document.getElementById('banner-title');
const connDot = document.getElementById('conn-dot');
const connText = document.getElementById('conn-text');
const connPill = document.getElementById('conn-pill');
const viaEl = document.getElementById('via');
const modeEl = document.getElementById('mode');
const lastUpdatedEl = document.getElementById('last-updated');
const flowTrendEl = document.getElementById('flow-trend');
const humTrendEl = document.getElementById('hum-trend');
const tempTrendEl = document.getElementById('temp-trend');
const pressureTrendEl = document.getElementById('pressure-trend');
const eventLog = document.getElementById('event-log');
const clearLogBtn = document.getElementById('clear-log');
const testToneBtn = document.getElementById('test-tone');
const alertSound = document.getElementById('alert-sound');

/* Sparklines */
const flowSpark = document.getElementById('flow-spark').getContext('2d');
const humSpark = document.getElementById('hum-spark').getContext('2d');
const tempSpark = document.getElementById('temp-spark').getContext('2d');
const pressureSpark = document.getElementById('pressure-spark').getContext('2d');
const sparkCtx = {
  flow: flowSpark,
  hum: humSpark,
  temp: tempSpark,
  pressure: pressureSpark,
};
const sparkColors = {
  flow: '#4cc9f0',
  hum: '#ffd166',
  temp: '#ef5350',
  pressure: '#7bd4a5',
};
const sparkHistory = {
  flow: [],
  hum: [],
  temp: [],
  pressure: []
};
const SPARK_MAX = 60;

/* State */
let es = null;
let prev = { flow: null, hum: null, temp: null, pressure: null };
const LIMITS = {
  flowHigh: 45,
  flowLow: 0.3,
  humidity: 80,
  temp: 50,
  pressure: 120
};

/* Utils */
const clamp = (n, min, max) => Math.max(min, Math.min(max, n));
const fmtTime = (t = new Date()) =>
  t.toLocaleTimeString(undefined, { hour:'2-digit', minute:'2-digit', second:'2-digit' });
const setAlertHighlight = (el, isHigh) => {
  if (!el) return;
  el.classList.toggle('value-alert', !!isHigh);
};
const describeAlerts = (alerts=[]) => alerts.map(code => {
  switch(code){
    case 'HIGH_FLOW': return 'High flow';
    case 'LOW_FLOW': return 'Low flow';
    case 'HIGH_HUMIDITY': return 'High humidity';
    case 'HIGH_TEMP': return 'High temperature';
    case 'HIGH_PRESSURE': return 'High pressure';
    default: return code;
  }
});

function setConn(connected, via) {
  connDot.classList.remove('ok', 'err');
  connDot.classList.add(connected ? 'ok' : 'err');
  connText.textContent = connected ? 'Connected' : 'Disconnected';
  viaEl.textContent = via ? `via ${via}` : 'via —';
  modeEl.textContent = via || '—';
  connPill.title = connected ? 'Gateway connected' : 'Gateway disconnected';
}

function setBanner(alertsList) {
  if (alertsList && alertsList.length) {
    const human = describeAlerts(alertsList);
    bannerEl.classList.remove('hidden');
    bannerTitle.textContent = 'EMERGENCY — CHECK SENSORS';
    bannerText.textContent = human.join(', ');
    const btn = document.getElementById('emergency-link');
    if (btn) btn.textContent = `Call emergency — ${human.join(' + ')}`;
    alertSound?.play().catch(()=>{});
  } else {
    bannerEl.classList.add('hidden');
    const btn = document.getElementById('emergency-link');
    if (btn) btn.textContent = 'Call emergency';
  }
}

function setTrend(el, prevVal, nextVal) {
  el.classList.remove('up','down','equal');
  if (prevVal == null || nextVal == null) {
    el.textContent = '—';
    return;
  }
  const d = nextVal - prevVal;
  if (Math.abs(d) < 1e-6) {
    el.classList.add('equal');
    el.textContent = 'steady';
  } else if (d > 0) {
    el.classList.add('up');
    el.textContent = `up ${d.toFixed(2)}`;
  } else {
    el.classList.add('down');
    el.textContent = `down ${(Math.abs(d)).toFixed(2)}`;
  }
}

function drawSpark(ctx, series, color = '#4cc9f0') {
  const w = ctx.canvas.width;
  const h = ctx.canvas.height;
  // clear
  ctx.clearRect(0,0,w,h);
  if (series.length < 2) return;

  const min = Math.min(...series);
  const max = Math.max(...series);
  const range = max - min || 1;

  ctx.beginPath();
  ctx.moveTo(0, h - ((series[0]-min)/range)*h);
  for (let i=1;i<series.length;i++){
    const x = (i/(series.length-1))*w;
    const y = h - ((series[i]-min)/range)*h;
    ctx.lineTo(x,y);
  }
  ctx.lineWidth = 2;
  ctx.strokeStyle = color;
  ctx.stroke();

  // fill gradient
  const g = ctx.createLinearGradient(0,0,0,h);
  g.addColorStop(0, color + '80'); // alpha-ish
  g.addColorStop(1, 'transparent');
  ctx.lineTo(w,h); ctx.lineTo(0,h); ctx.closePath();
  ctx.fillStyle = g;
  ctx.fill();
}

function pushSpark(key, v) {
  const arr = sparkHistory[key];
  arr.push(v);
  if (arr.length > SPARK_MAX) arr.shift();
  const ctx = sparkCtx[key];
  if (!ctx) return;
  drawSpark(ctx, arr, sparkColors[key] || '#4cc9f0');
}

function logEvent(text, level='info'){
  const li = document.createElement('li');
  const ts = document.createElement('span');
  ts.className = 'log-badge';
  ts.textContent = fmtTime();
  const msg = document.createElement('span');
  msg.textContent = text;
  if (level === 'warn') msg.style.color = 'var(--warn)';
  if (level === 'err') msg.style.color = 'var(--err)';
  li.appendChild(ts); li.appendChild(msg);
  eventLog.prepend(li);
  // keep last 100
  while (eventLog.children.length > 100) eventLog.removeChild(eventLog.lastChild);
}

/* Stream */
function connectStream(){
  if (es) es.close();
  es = new EventSource('/events');
  es.onopen = () => { setConn(true, ''); logEvent('SSE connected'); };
  es.onmessage = (ev) => {
    try{
      const d = JSON.parse(ev.data);
      // values
      const flow = Number(d.flow_lpm ?? NaN);
      const hum = Number(d.humidity_pct ?? NaN);
      const temp = Number(d.temperature_c ?? NaN);
      const pressure = Number(d.pressure_kpa ?? NaN);
      const alertsList = Array.isArray(d.alerts) ? d.alerts : [];
      const via = d.via ?? '—';

      // UI update
      if (!Number.isNaN(flow)) {
        setTrend(flowTrendEl, prev.flow, flow);
        prev.flow = flow;
        flowEl.textContent = flow.toFixed(2);
        pushSpark('flow', clamp(flow, -1e6, 1e6));
        setAlertHighlight(flowEl, flow > LIMITS.flowHigh || flow < LIMITS.flowLow);
      }
      if (!Number.isNaN(hum)) {
        setTrend(humTrendEl, prev.hum, hum);
        prev.hum = hum;
        humEl.textContent = hum.toFixed(1);
        pushSpark('hum', clamp(hum, -1e6, 1e6));
        setAlertHighlight(humEl, hum > LIMITS.humidity);
      }
      if (!Number.isNaN(temp)) {
        setTrend(tempTrendEl, prev.temp, temp);
        prev.temp = temp;
        tempEl.textContent = temp.toFixed(1);
        pushSpark('temp', clamp(temp, -1e6, 1e6));
        setAlertHighlight(tempEl, temp > LIMITS.temp);
      }
      if (!Number.isNaN(pressure)) {
        setTrend(pressureTrendEl, prev.pressure, pressure);
        prev.pressure = pressure;
        pressureEl.textContent = pressure.toFixed(1);
        pushSpark('pressure', clamp(pressure, -1e6, 1e6));
        setAlertHighlight(pressureEl, pressure > LIMITS.pressure);
      }
      setConn(d.connection === 'CONNECTED', via);
      setBanner(alertsList);
      modeEl.textContent = via;
      lastUpdatedEl.textContent = fmtTime();

      // log interesting events
      if (alertsList.length) logEvent(`Alerts: ${alertsList.join(', ')}`, 'err');
    }catch(e){
      console.error('bad event', e);
      logEvent('Malformed data received', 'warn');
    }
  };
  es.onerror = () => {
    setConn(false);
    logEvent('SSE error / disconnected', 'warn');
  };
}

function init(){
  connectStream();

  clearLogBtn?.addEventListener('click', ()=>{ eventLog.innerHTML=''; });
  testToneBtn?.addEventListener('click', ()=>{ alertSound?.play().catch(()=>{}); });
}

init();
