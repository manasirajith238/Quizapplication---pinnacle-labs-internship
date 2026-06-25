#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstring>
#include <thread>
#include <mutex>
#include <sqlite3.h>
#include <cstdlib>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  #define close(s) closesocket(s)
  typedef int socklen_t;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  typedef int SOCKET;
  #define INVALID_SOCKET (-1)
#endif

// ────────────────────────────────────────────────────────────────
// Quiz question data (in-memory, unchanged from earlier versions)
// ────────────────────────────────────────────────────────────────
struct Option {
    std::string id;
    std::string text;
};

struct Question {
    int id;
    std::string text;
    std::vector<Option> options;
    std::string correctAnswer;
    std::string explanation;
    std::string category;
};

std::vector<Question> questions = {
    {1, "What does CPU stand for?",
     {{"A","Central Processing Unit"},{"B","Computer Personal Unit"},{"C","Central Program Utility"},{"D","Core Processing Unit"}},
     "A", "CPU stands for Central Processing Unit — the primary component that executes instructions.", "Technology"},
    {2, "Which planet is closest to the Sun?",
     {{"A","Venus"},{"B","Earth"},{"C","Mercury"},{"D","Mars"}},
     "C", "Mercury is the closest planet to the Sun, orbiting at an average distance of 57.9 million km.", "Science"},
    {3, "What is the chemical symbol for Gold?",
     {{"A","Go"},{"B","Gd"},{"C","Gl"},{"D","Au"}},
     "D", "Au comes from the Latin word 'Aurum', meaning gold.", "Science"},
    {4, "In which year did World War II end?",
     {{"A","1943"},{"B","1944"},{"C","1945"},{"D","1946"}},
     "C", "World War II ended in 1945 with Germany surrendering in May and Japan in September.", "History"},
    {5, "What is the largest ocean on Earth?",
     {{"A","Atlantic Ocean"},{"B","Indian Ocean"},{"C","Arctic Ocean"},{"D","Pacific Ocean"}},
     "D", "The Pacific Ocean is the largest, covering more than 165 million km2 — larger than all landmasses combined.", "Geography"},
    {6, "Who wrote 'Romeo and Juliet'?",
     {{"A","Charles Dickens"},{"B","William Shakespeare"},{"C","Jane Austen"},{"D","Mark Twain"}},
     "B", "Romeo and Juliet was written by William Shakespeare, believed to be around 1594-1596.", "Literature"},
    {7, "What is the square root of 144?",
     {{"A","11"},{"B","12"},{"C","13"},{"D","14"}},
     "B", "sqrt(144) = 12, because 12 x 12 = 144.", "Mathematics"},
    {8, "Which programming language was created by Bjarne Stroustrup?",
     {{"A","Java"},{"B","Python"},{"C","C++"},{"D","Rust"}},
     "C", "C++ was created by Bjarne Stroustrup at Bell Labs starting in 1979 as an extension of C.", "Technology"},
    {9, "What is the capital of Japan?",
     {{"A","Osaka"},{"B","Kyoto"},{"C","Hiroshima"},{"D","Tokyo"}},
     "D", "Tokyo is the capital and most populous city of Japan.", "Geography"},
    {10, "How many bones are in the adult human body?",
     {{"A","196"},{"B","206"},{"C","216"},{"D","226"}},
     "B", "An adult human body has 206 bones. Babies are born with around 270-300 bones that fuse over time.", "Science"}
};

// ────────────────────────────────────────────────────────────────
// SQLite setup
// ────────────────────────────────────────────────────────────────
sqlite3* db = nullptr;
std::mutex dbMutex;

bool execSQL(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

void initDB() {
    int rc = sqlite3_open("quiz.db", &db);
    if (rc) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        exit(1);
    }

    execSQL(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT UNIQUE NOT NULL,"
        "  password TEXT NOT NULL,"
        "  role TEXT NOT NULL DEFAULT 'user'"
        ");"
    );

    execSQL(
        "CREATE TABLE IF NOT EXISTS quiz_results ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id INTEGER NOT NULL,"
        "  username TEXT NOT NULL,"
        "  score INTEGER NOT NULL,"
        "  total INTEGER NOT NULL,"
        "  time_ms INTEGER NOT NULL,"
        "  created_at TEXT DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(user_id) REFERENCES users(id)"
        ");"
    );

    // Seed a default admin account if no admin exists yet.
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE role='admin';", -1, &stmt, nullptr);
    int adminCount = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) adminCount = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (adminCount == 0) {
        sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO users (username, password, role) VALUES ('admin', 'admin123', 'admin');",
            -1, &stmt, nullptr);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        std::cout << "Seeded default admin account -> username: admin / password: admin123\n";
        std::cout << "Change this password in production!\n";
    }
}

// ────────────────────────────────────────────────────────────────
// JSON helpers
// ────────────────────────────────────────────────────────────────
std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::string getParam(const std::string& body, const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = body.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t')) pos++;
    if (pos >= body.size()) return "";
    if (body[pos] == '"') {
        pos++;
        std::string val;
        while (pos < body.size() && body[pos] != '"') {
            if (body[pos] == '\\' && pos + 1 < body.size()) { val += body[pos+1]; pos += 2; continue; }
            val += body[pos++];
        }
        return val;
    } else {
        std::string val;
        while (pos < body.size() && body[pos] != ',' && body[pos] != '}') val += body[pos++];
        // trim whitespace
        while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r')) val.pop_back();
        return val;
    }
}

// ────────────────────────────────────────────────────────────────
// Quiz question / answer JSON builders
// ────────────────────────────────────────────────────────────────
std::string buildQuestionsJson() {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < questions.size(); ++i) {
        const auto& q = questions[i];
        ss << "{";
        ss << "\"id\":" << q.id << ",";
        ss << "\"text\":\"" << escapeJson(q.text) << "\",";
        ss << "\"category\":\"" << escapeJson(q.category) << "\",";
        ss << "\"options\":[";
        for (size_t j = 0; j < q.options.size(); ++j) {
            ss << "{\"id\":\"" << q.options[j].id << "\",\"text\":\"" << escapeJson(q.options[j].text) << "\"}";
            if (j + 1 < q.options.size()) ss << ",";
        }
        ss << "]";
        ss << "}";
        if (i + 1 < questions.size()) ss << ",";
    }
    ss << "]";
    return ss.str();
}

std::string buildAnswerJson(int qid, const std::string& chosen) {
    for (const auto& q : questions) {
        if (q.id == qid) {
            bool correct = (chosen == q.correctAnswer);
            std::ostringstream ss;
            ss << "{";
            ss << "\"correct\":" << (correct ? "true" : "false") << ",";
            ss << "\"correctAnswer\":\"" << q.correctAnswer << "\",";
            ss << "\"explanation\":\"" << escapeJson(q.explanation) << "\"";
            ss << "}";
            return ss.str();
        }
    }
    return "{\"error\":\"Question not found\"}";
}

// ────────────────────────────────────────────────────────────────
// Auth helpers
// ────────────────────────────────────────────────────────────────
// Returns: 0 = not found / wrong password, otherwise user id.
// Also fills outRole with 'user' or 'admin'.
int verifyUser(const std::string& username, const std::string& password, std::string& outRole) {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id, password, role FROM users WHERE username = ?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    int userId = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string storedPass = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (storedPass == password) {
            userId = sqlite3_column_int(stmt, 0);
            outRole = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        }
    }
    sqlite3_finalize(stmt);
    return userId;
}

bool isAdmin(const std::string& username, const std::string& password) {
    std::string role;
    int uid = verifyUser(username, password, role);
    return uid != 0 && role == "admin";
}

// ────────────────────────────────────────────────────────────────
// Route handlers
// ────────────────────────────────────────────────────────────────
std::string handleRegister(const std::string& body) {
    std::string username = getParam(body, "username");
    std::string password = getParam(body, "password");
    if (username.empty() || password.empty()) {
        return "{\"error\":\"Username and password are required\"}";
    }
    if (username.size() < 3 || password.size() < 4) {
        return "{\"error\":\"Username must be 3+ chars and password 4+ chars\"}";
    }

    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "INSERT INTO users (username, password, role) VALUES (?, ?, 'user');", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return "{\"error\":\"Username already taken\"}";
    }
    return "{\"ok\":true,\"message\":\"Account created. You can now log in.\"}";
}

std::string handleLogin(const std::string& body) {
    std::string username = getParam(body, "username");
    std::string password = getParam(body, "password");
    if (username.empty() || password.empty()) {
        return "{\"error\":\"Username and password are required\"}";
    }

    std::string role;
    int userId = verifyUser(username, password, role);
    if (userId == 0) {
        return "{\"error\":\"Invalid username or password\"}";
    }

    std::ostringstream ss;
    ss << "{";
    ss << "\"ok\":true,";
    ss << "\"userId\":" << userId << ",";
    ss << "\"username\":\"" << escapeJson(username) << "\",";
    ss << "\"role\":\"" << role << "\"";
    ss << "}";
    return ss.str();
}

std::string handleSaveResult(const std::string& body) {
    std::string username = getParam(body, "username");
    std::string password = getParam(body, "password");
    std::string scoreStr  = getParam(body, "score");
    std::string totalStr  = getParam(body, "total");
    std::string timeStr   = getParam(body, "timeMs");

    if (username.empty() || password.empty() || scoreStr.empty() || totalStr.empty() || timeStr.empty()) {
        return "{\"error\":\"Missing fields\"}";
    }

    std::string role;
    int userId = verifyUser(username, password, role);
    if (userId == 0) {
        return "{\"error\":\"Invalid username or password\"}";
    }

    int score  = std::stoi(scoreStr);
    int total  = std::stoi(totalStr);
    int timeMs = std::stoi(timeStr);

    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT INTO quiz_results (user_id, username, score, total, time_ms) VALUES (?, ?, ?, ?, ?);",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, score);
    sqlite3_bind_int(stmt, 4, total);
    sqlite3_bind_int(stmt, 5, timeMs);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return "{\"ok\":true}";
}

std::string buildLeaderboardJson() {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT username, score, total, time_ms FROM quiz_results "
        "ORDER BY score DESC, time_ms ASC LIMIT 10;",
        -1, &stmt, nullptr);

    std::ostringstream ss;
    ss << "[";
    int rank = 1;
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) ss << ",";
        first = false;
        std::string uname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int score   = sqlite3_column_int(stmt, 1);
        int total   = sqlite3_column_int(stmt, 2);
        int timeMs  = sqlite3_column_int(stmt, 3);
        ss << "{";
        ss << "\"rank\":" << rank++ << ",";
        ss << "\"name\":\"" << escapeJson(uname) << "\",";
        ss << "\"score\":" << score << ",";
        ss << "\"total\":" << total << ",";
        ss << "\"timeMs\":" << timeMs;
        ss << "}";
    }
    sqlite3_finalize(stmt);
    ss << "]";
    return ss.str();
}

// Admin stats: total users, total quizzes taken, average score, top 5 results
std::string buildAdminStatsJson(const std::string& username, const std::string& password) {
    if (!isAdmin(username, password)) {
        return "{\"error\":\"Admin access required\"}";
    }

    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt* stmt;

    int totalUsers = 0, totalQuizzes = 0;
    double avgScorePct = 0.0;

    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users;", -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) totalUsers = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM quiz_results;", -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) totalQuizzes = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "SELECT AVG(CAST(score AS REAL) / total) FROM quiz_results;", -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        avgScorePct = sqlite3_column_double(stmt, 0) * 100.0;
    }
    sqlite3_finalize(stmt);

    // Recent results (last 10)
    std::ostringstream recent;
    recent << "[";
    sqlite3_prepare_v2(db,
        "SELECT username, score, total, time_ms, created_at FROM quiz_results "
        "ORDER BY id DESC LIMIT 10;", -1, &stmt, nullptr);
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) recent << ",";
        first = false;
        std::string uname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int score  = sqlite3_column_int(stmt, 1);
        int total  = sqlite3_column_int(stmt, 2);
        int timeMs = sqlite3_column_int(stmt, 3);
        std::string createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        recent << "{";
        recent << "\"username\":\"" << escapeJson(uname) << "\",";
        recent << "\"score\":" << score << ",";
        recent << "\"total\":" << total << ",";
        recent << "\"timeMs\":" << timeMs << ",";
        recent << "\"createdAt\":\"" << escapeJson(createdAt) << "\"";
        recent << "}";
    }
    sqlite3_finalize(stmt);
    recent << "]";

    std::ostringstream ss;
    ss << "{";
    ss << "\"totalUsers\":" << totalUsers << ",";
    ss << "\"totalQuizzes\":" << totalQuizzes << ",";
    ss << "\"avgScorePct\":" << avgScorePct << ",";
    ss << "\"recentResults\":" << recent.str();
    ss << "}";
    return ss.str();
}

// ────────────────────────────────────────────────────────────────
// HTTP plumbing
// ────────────────────────────────────────────────────────────────
std::string buildResponse(int status, const std::string& contentType, const std::string& body) {
    std::string statusText = (status == 200) ? "OK"
                            : (status == 400) ? "Bad Request"
                            : (status == 401) ? "Unauthorized"
                            : (status == 404) ? "Not Found"
                            : "Error";
    std::ostringstream ss;
    ss << "HTTP/1.1 " << status << " " << statusText << "\r\n";
    ss << "Content-Type: " << contentType << "\r\n";
    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "Access-Control-Allow-Origin: *\r\n";
    ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type\r\n";
    ss << "Connection: close\r\n";
    ss << "\r\n";
    ss << body;
    return ss.str();
}

void handleClient(SOCKET clientSock) {
    char buf[8192] = {};
    int len = recv(clientSock, buf, sizeof(buf) - 1, 0);
    if (len <= 0) { close(clientSock); return; }

    std::string request(buf, len);
    std::istringstream reqStream(request);
    std::string method, path, version;
    reqStream >> method >> path >> version;

    auto bodyPos = request.find("\r\n\r\n");
    std::string body = (bodyPos != std::string::npos) ? request.substr(bodyPos + 4) : "";

    std::string response;

    if (method == "OPTIONS") {
        response = buildResponse(200, "text/plain", "");
    } else if (method == "GET" && path == "/api/questions") {
        response = buildResponse(200, "application/json", buildQuestionsJson());
    } else if (method == "POST" && path == "/api/answer") {
        std::string qidStr = getParam(body, "questionId");
        std::string answer = getParam(body, "answer");
        if (qidStr.empty() || answer.empty()) {
            response = buildResponse(400, "application/json", "{\"error\":\"Missing fields\"}");
        } else {
            int qid = std::stoi(qidStr);
            response = buildResponse(200, "application/json", buildAnswerJson(qid, answer));
        }
    } else if (method == "POST" && path == "/api/register") {
        std::string result = handleRegister(body);
        bool ok = result.find("\"error\"") == std::string::npos;
        response = buildResponse(ok ? 200 : 400, "application/json", result);
    } else if (method == "POST" && path == "/api/login") {
        std::string result = handleLogin(body);
        bool ok = result.find("\"error\"") == std::string::npos;
        response = buildResponse(ok ? 200 : 401, "application/json", result);
    } else if (method == "POST" && path == "/api/leaderboard") {
        std::string result = handleSaveResult(body);
        bool ok = result.find("\"error\"") == std::string::npos;
        response = buildResponse(ok ? 200 : 401, "application/json", result);
    } else if (method == "GET" && path == "/api/leaderboard") {
        response = buildResponse(200, "application/json", buildLeaderboardJson());
    } else if (method == "POST" && path == "/api/admin/stats") {
        std::string username = getParam(body, "username");
        std::string password = getParam(body, "password");
        std::string result = buildAdminStatsJson(username, password);
        bool ok = result.find("\"error\"") == std::string::npos;
        response = buildResponse(ok ? 200 : 401, "application/json", result);
    } else if (method == "GET" && path == "/api/health") {
        response = buildResponse(200, "application/json", "{\"status\":\"ok\",\"questions\":" + std::to_string(questions.size()) + "}");
    } else {
        response = buildResponse(404, "application/json", "{\"error\":\"Not found\"}");
    }

    send(clientSock, response.c_str(), (int)response.size(), 0);
    close(clientSock);
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    initDB();

    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::stoi(port_env) : 8765;
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) { std::cerr << "Socket error\n"; return 1; }

    char opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind error\n"; return 1;
    }
    listen(serverSock, 10);
    std::cout << "Quiz C++ Server running on port " << port << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock == INVALID_SOCKET) continue;
        std::thread(handleClient, clientSock).detach();
    }

    close(serverSock);
    sqlite3_close(db);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
