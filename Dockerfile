# ── Stage 1: Build the C++ server ────────────────────────────────────────────
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    g++ make libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY src/ ./src/
COPY Makefile ./
RUN make

# ── Stage 2: Runtime (nginx + C++ server) ────────────────────────────────────
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    nginx libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# C++ binary
COPY --from=builder /app/quiz_server ./quiz_server

# Frontend static files → nginx web root
COPY public/ /usr/share/nginx/html/

# Nginx config
COPY nginx.conf /etc/nginx/conf.d/default.conf
RUN rm -f /etc/nginx/sites-enabled/default

# Start script: launch C++ server in background, then nginx in foreground
RUN echo '#!/bin/sh\n/app/quiz_server &\nnginx -g "daemon off;"' > /start.sh \
    && chmod +x /start.sh

EXPOSE 80

CMD ["/start.sh"]
