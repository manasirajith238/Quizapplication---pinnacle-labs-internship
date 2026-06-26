# Quiz Challenge

> Built as part of my internship at **Pinnacle Labs**. A trivia quiz application
> with a C++ backend (REST API over raw sockets, SQLite storage) and a
> multi-page HTML/CSS/JS frontend featuring user accounts, a per-question
> countdown timer, a persistent leaderboard, and an admin dashboard.

🚀 **Live demo:** [https://quizapplication--production.up.railway.app](https://quizapplication--production.up.railway.app)

---

## Features

- **Login / Register** — username + password, stored in SQLite
- **Role-based accounts** — `user` and `admin`
- **Quiz** — 10 randomised questions, 20-second timer per question
- **Leaderboard** — top 10 scores, sorted by score then fastest time; persisted to SQLite
- **Admin dashboard** — total users, quizzes taken, average score, recent results (admin only)
- **Professional UI** — Space Grotesk + Inter + JetBrains Mono type system, dark mode, responsive

---

## Tech stack

| Layer     | Technology                                      |
|-----------|-------------------------------------------------|
| Backend   | C++17 — raw TCP sockets, hand-rolled HTTP server |
| Database  | SQLite3 (embedded, zero config)                 |
| Frontend  | Vanilla HTML / CSS / JavaScript (no frameworks) |
| Hosting   | Railway (Docker-based deployment)               |

---

## Project structure

```
quiz-app/
├── src/
│   └── main.cpp          # C++ HTTP server (REST API + SQLite)
├── public/
│   ├── css/
│   │   └── style.css     # Shared design system
│   ├── js/
│   │   └── app.js        # Shared session helpers + API base URL
│   ├── login.html        # Login / register page (entry point)
│   ├── index.html        # Quiz page
│   └── dashboard.html    # Leaderboard + admin dashboard
├── Dockerfile            # Multi-stage build for Railway deployment
├── railway.json          # Railway service configuration
├── Makefile
├── .gitignore
├── LICENSE
└── README.md
```

---

## Deployment (Railway)

This app is deployed on [Railway](https://railway.app) using Docker.

### One-time setup

1. Fork or clone this repo and push it to your GitHub account.
2. Go to [railway.app](https://railway.app) → **New Project → Deploy from GitHub repo**.
3. Select this repository. Railway auto-detects the `Dockerfile` and starts building.
4. Once deployed, go to **Settings → Networking → Generate Domain** to get your public URL.
5. Update `API_BASE` in `public/js/app.js` to match your Railway domain, then push again.

### Environment variables

Railway automatically injects a `PORT` environment variable. The server reads it on startup and falls back to `8765` if not set — no manual configuration needed.

### Data persistence

The SQLite database (`quiz.db`) is created at `/app/quiz.db` inside the container. To persist data across redeploys, add a **Railway Volume** mounted at `/app`.

---

## Running locally

### Requirements

- A C++17 compiler (`g++` or `clang++`)
- SQLite3 development library
  - Ubuntu/Debian: `sudo apt install libsqlite3-dev`
  - macOS: `brew install sqlite3`
  - Windows (MSYS2): `pacman -S mingw-w64-x86_64-sqlite3`
- `make`

### Linux / macOS

```bash
make
./quiz_server
```

### Windows (MinGW / MSYS2)

```bash
make
./quiz_server.exe
```

### Manual compile

```bash
# Linux / macOS
g++ -std=c++17 -O2 -o quiz_server src/main.cpp -lpthread -lsqlite3

# Windows
g++ -std=c++17 -O2 -o quiz_server.exe src/main.cpp -lws2_32 -lsqlite3
```

Open `http://localhost:8765` in your browser after starting the server.

### Running with Docker locally

```bash
docker build -t quiz-app .
docker run -p 8080:8080 quiz-app
```

---

## API endpoints

| Method | Path               | Auth          | Description                                   |
|--------|--------------------|---------------|-----------------------------------------------|
| GET    | `/api/health`      | —             | Server status + question count                |
| GET    | `/api/questions`   | —             | All quiz questions (no answers)               |
| POST   | `/api/answer`      | —             | `{questionId, answer}` → correct / incorrect  |
| POST   | `/api/register`    | —             | `{username, password}` → create account       |
| POST   | `/api/login`       | —             | `{username, password}` → returns role         |
| GET    | `/api/leaderboard` | —             | Top 10 scores                                 |
| POST   | `/api/leaderboard` | user+password | `{username, password, score, total, timeMs}`  |
| POST   | `/api/admin/stats` | admin only    | `{username, password}` → aggregate stats      |

---

## Security notes

This project is built for learning and demo purposes:

- Passwords are stored in plain text — a production version would use bcrypt or argon2.
- No session tokens — credentials are re-sent on each protected request; Railway provides HTTPS which mitigates exposure in transit.
- No rate limiting on login/register endpoints.

---

## Internship

Built during my internship at **Pinnacle Labs** as a full-stack project demonstrating:
- Systems programming in C++ (networking, threading, database integration)
- Frontend development without frameworks
- Containerised deployment with Docker and Railway

---

## License

MIT — see [LICENSE](./LICENSE).
