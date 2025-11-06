const flowEl = document.getElementById('flow');
const humEl = document.getElementById('humidity');
const flowingEl = document.getElementById('flowing');
const bannerEl = document.getElementById('banner');
const bannerText = document.getElementById('banner-text');
const bannerTitle = document.getElementById('banner-title');
const connDot = document.getElementById('conn-dot');
const connText = document.getElementById('conn-text');
const viaEl = document.getElementById('via');
const modeEl = document.getElementById('mode');

function setConn(connected, via) {
  connDot.classList.remove('ok', 'err');
  connDot.classList.add(connected ? 'ok' : 'err');
  connText.textContent = connected ? 'Connected' : 'Disconnected';
  viaEl.textContent = via ? `via ${via}` : 'via —';
}

function setBanner(alert) {
  if (alert === 'LEAK') {
    bannerEl.classList.remove('hidden');
    bannerTitle.textContent = 'EMERGENCY — LEAK';
    bannerText.textContent = 'Leak detected by sensor. Act immediately.';
  } else if (alert === 'HIGH_FLOW') {
    bannerEl.classList.remove('hidden');
    bannerTitle.textContent = 'EMERGENCY — HIGH FLOW';
    bannerText.textContent = 'Abnormally high flow rate detected.';
  } else {
    bannerEl.classList.add('hidden');
  }
}

function start() {
  const es = new EventSource('/events');
  es.onmessage = (ev) => {
    try {
      const d = JSON.parse(ev.data);
      flowEl.textContent = d.flow_lpm.toFixed(2);
      humEl.textContent = d.humidity_pct.toFixed(1);
      flowingEl.textContent = d.flowing ? 'Yes' : 'No';
      setConn(d.connection === 'CONNECTED', d.via);
      setBanner(d.alert);
      modeEl.textContent = d.via;
    } catch(e) {
      console.error('bad event', e);
    }
  };
  es.onerror = () => { setConn(false); };
}

start();
