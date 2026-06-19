# Quiz Challenge

> Built as part of my internship at **Pinnacle Labs**. A trivia quiz application
> with a C++ backend (REST API over raw sockets, SQLite storage) and a
> multi-page HTML/CSS/JS frontend featuring user accounts, a per-question
> countdown timer, a persistent leaderboard, and an admin dashboard.

## Features

- **Login / Register** — username + password, stored in SQLite
- **Role-based accounts** — `user` and `admin`
- **Quiz** — 8 randomised questions, 20-second timer per question
- **Leaderboard** — top 10 scores, sorted by score then fastest time; persisted to SQLite
- **Admin dashboard** — total users, quizzes taken, average score, recent results (admin only)
- **Professional UI** — Space Grotesk + Inter + JetBrains Mono type system, dark mode, responsive

## Project structure

```
quiz-app/
├── src/
│   └── main.cpp          # C++ HTTP server (REST API + SQLite)
├── public/
│   ├── css/
│   │   └── style.css     # Shared design system
│   ├── js/
│   │   └── app.js        # Shared session helpers + API base
│   ├── login.html        # Login / register page (entry point)
│   ├── index.html        # Quiz page
│   └── dashboard.html    # Leaderboard + admin dashboard
├── Makefile
├── .gitignore
├── LICENSE
└── README.md
```

## Requirements

- A C++17 compiler (`g++` or `clang++`)
- **SQLite3 development library**
  - Ubuntu/Debian: `sudo apt install libsqlite3-dev`
  - macOS (Homebrew): `brew install sqlite3`
  - Windows (MSYS2/MinGW): `pacman -S mingw-w64-x86_64-sqlite3`
- `make` (optional)
- A modern web browser

## Build & run

### Linux / macOS

```bash
make
./quiz_server
```

### Windows (MinGW / MSYS2)

```bash
make
quiz_server.exe
```

Or compile manually:

```bash
# Linux / macOS
g++ -std=c++17 -O2 -o quiz_server src/main.cpp -lpthread -lsqlite3

# Windows
g++ -std=c++17 -O2 -o quiz_server.exe src/main.cpp -lws2_32 -lsqlite3
```

The server listens on **http://localhost:8765** and creates `quiz.db` on first run.

## Running the app

1. Build and start the server — leave the terminal open.
2. Open **`public/login.html`** in your browser (double-click or drag into a tab).
3. Register an account or log in with the default admin credentials.
4. Navigate between pages using the top bar.

### Default admin account

On first run, the server seeds one admin account:

| Username | Password  |
|----------|-----------|
| `admin`  | `admin123`|

Change this before deploying:

```bash
sqlite3 quiz.db "UPDATE users SET password='newpassword' WHERE username='admin';"
```

## API endpoints

| Method | Path                 | Auth           | Description                                        |
|--------|----------------------|----------------|----------------------------------------------------|
| GET    | `/api/health`        | —              | Server status + question count                     |
| GET    | `/api/questions`     | —              | All quiz questions (no answers)                    |
| POST   | `/api/answer`        | —              | `{questionId, answer}` → correct / incorrect       |
| POST   | `/api/register`      | —              | `{username, password}` → create account            |
| POST   | `/api/login`         | —              | `{username, password}` → returns role              |
| GET    | `/api/leaderboard`   | —              | Top 10 scores                                      |
| POST   | `/api/leaderboard`   | user+password  | `{username, password, score, total, timeMs}`       |
| POST   | `/api/admin/stats`   | admin only     | `{username, password}` → aggregate stats           |

## Security notes

This project is built for learning/demo purposes:

- Passwords are stored **in plain text** — use bcrypt/argon2 for production.
- No session tokens — credentials are re-sent on each protected request; use HTTPS if deployed.
- No rate limiting on login/register endpoints.

## Push to GitHub

```bash
git init
git add .
git commit -m "Initial commit: Quiz Challenge — Pinnacle Labs internship"
git branch -M main
git remote add origin https://github.com/<your-username>/quiz-challenge.git
git push -u origin main
```

## License

MIT — see [LICENSE](LICENSE).
