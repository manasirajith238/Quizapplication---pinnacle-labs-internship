/* ============================================================
   Quiz Challenge — shared helpers (session, API, status)
   Included on every page.
   ============================================================ */

const API = 'http://localhost:8765';

// ── Session (per-tab, simplified auth model) ───────────────────
// Stores { username, password, role }. Password is re-sent on each
// protected request because this server has no session tokens.
const Session = {
  get() {
    try {
      const raw = sessionStorage.getItem('quiz_session');
      return raw ? JSON.parse(raw) : null;
    } catch (e) { return null; }
  },
  set(session) {
    sessionStorage.setItem('quiz_session', JSON.stringify(session));
  },
  clear() {
    sessionStorage.removeItem('quiz_session');
  },
  requireLogin() {
    const s = this.get();
    if (!s) {
      window.location.href = 'login.html';
      return null;
    }
    return s;
  }
};

function logout() {
  Session.clear();
  window.location.href = 'login.html';
}

// ── Server status pill (id="server-status") ───────────────────
async function checkServerStatus() {
  const dot = document.getElementById('status-dot');
  const text = document.getElementById('status-text');
  if (!dot || !text) return true;
  try {
    await fetch(API + '/api/health');
    dot.className = 'dot on';
    text.textContent = 'Server connected · port 8765';
    return true;
  } catch (e) {
    dot.className = 'dot off';
    text.textContent = 'Server offline';
    return false;
  }
}

// ── Render the logged-in topbar into #topbar ───────────────────
function renderTopbar(activePage) {
  const el = document.getElementById('topbar');
  if (!el) return;
  const session = Session.get();
  if (!session) return;

  const navItem = (href, label, key) =>
    `<a class="btn-ghost ${activePage === key ? 'active' : ''}" href="${href}">${label}</a>`;

  el.innerHTML = `
    <div class="topbar-left">
      <div class="topbar-id">
        Signed in as <span class="who">${escapeHtml(session.username)}</span>
        <span class="role-badge">${escapeHtml(session.role)}</span>
      </div>
    </div>
    <div class="topbar-nav">
      ${navItem('home.html', 'Home', 'home')}
      ${navItem('index.html', 'Quiz', 'quiz')}
      ${navItem('dashboard.html', 'Dashboard', 'dashboard')}
      <button class="btn-ghost danger" onclick="logout()">Log out</button>
    </div>
  `;
  // Style nav <a> like buttons (anchor tags don't get button reset by default)
  el.querySelectorAll('.topbar-nav a').forEach(a => {
    a.style.textDecoration = 'none';
    a.style.display = 'inline-flex';
    a.style.alignItems = 'center';
  });
}

// ── Utilities ────────────────────────────────────────────────
function escapeHtml(s) {
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
