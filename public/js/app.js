/* ============================================================
   Quiz Challenge — shared helpers
   ============================================================ */
const API = 'https://quizapplication-pinnacle-labs-internship-production.up.railway.app';

const Session = {
  get() {
    try { return JSON.parse(sessionStorage.getItem('quiz_session')); }
    catch(e) { return null; }
  },
  set(s)  { sessionStorage.setItem('quiz_session', JSON.stringify(s)); },
  clear() { sessionStorage.removeItem('quiz_session'); },
  requireLogin() {
    const s = this.get();
    if (!s) { window.location.href = 'login.html'; return null; }
    return s;
  }
};

function logout() {
  Session.clear();
  window.location.href = 'login.html';
}

async function checkServerStatus() {
  const dot  = document.getElementById('status-dot');
  const text = document.getElementById('status-text');
  try {
    await fetch(API + '/api/health');
    if (dot)  dot.className  = 'dot on';
    if (text) text.textContent = 'Server connected · port 8765';
    return true;
  } catch(e) {
    if (dot)  dot.className  = 'dot off';
    if (text) text.textContent = 'Server offline';
    return false;
  }
}

function renderTopbar(activePage) {
  const el = document.getElementById('topbar');
  if (!el) return;
  const s = Session.get();
  if (!s) return;
  const nav = (href, label, key) =>
    `<a class="btn-ghost${activePage===key?' active':''}" href="${href}">${label}</a>`;
  el.innerHTML = `
    <div class="topbar-id">
      ${escapeHtml(s.username)}<span class="role-badge">${escapeHtml(s.role)}</span>
    </div>
    <div class="topbar-nav">
      ${nav('home.html','Home','home')}
      ${s.role!=='admin'?nav('index.html','Quiz','quiz'):''}
      ${nav('dashboard.html','Dashboard','dashboard')}
      <button class="btn-ghost danger" onclick="logout()">Log out</button>
    </div>`;
  el.querySelectorAll('.topbar-nav a').forEach(a => {
    a.style.textDecoration = 'none';
  });
}

function escapeHtml(s) {
  return String(s)
    .replace(/&/g,'&amp;').replace(/</g,'&lt;')
    .replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
