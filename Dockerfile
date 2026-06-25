# ── Stage 1: Build ──────────────────────────────────────────────────────────
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src/ ./src/
COPY Makefile ./

RUN make

# ── Stage 2: Runtime ─────────────────────────────────────────────────────────
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary
COPY --from=builder /app/quiz_server ./quiz_server

# Copy the frontend static files
COPY public/ ./public/

# Railway injects PORT env var; our server must listen on it
# The app currently hardcodes 8765 — we handle that via an env var below
ENV PORT=8765

EXPOSE 8765

CMD ["./quiz_server"]
