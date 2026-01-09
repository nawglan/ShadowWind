# Multi-stage build: compile in build stage, run in minimal runtime
FROM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    libsodium-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY src/ ./src/
COPY lib/ ./lib/

# Create bin directory and build the binary + utilities
RUN mkdir -p bin && cd src && make clean && make all

# Runtime stage
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    libsodium23 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binaries and game data
COPY --from=builder /build/bin/shadowwind ./bin/shadowwind
COPY --from=builder /build/bin/lookup_process ./bin/lookup_process
COPY --from=builder /build/lib ./lib

# Create runtime directories for player data
RUN mkdir -p /app/lib/plrdata/A-E /app/lib/plrdata/F-J /app/lib/plrdata/K-O \
             /app/lib/plrdata/P-T /app/lib/plrdata/U-Z /app/lib/plrdata/ZZZ \
    && mkdir -p /app/lib/plrlogs/A-E /app/lib/plrlogs/F-J /app/lib/plrlogs/K-O \
                /app/lib/plrlogs/P-T /app/lib/plrlogs/U-Z /app/lib/plrlogs/ZZZ \
    && mkdir -p /app/lib/plrobjs/A-E /app/lib/plrobjs/F-J /app/lib/plrobjs/K-O \
                /app/lib/plrobjs/P-T /app/lib/plrobjs/U-Z /app/lib/plrobjs/ZZZ \
    && mkdir -p /app/lib/plrtext/A-E /app/lib/plrtext/F-J /app/lib/plrtext/K-O \
                /app/lib/plrtext/P-T /app/lib/plrtext/U-Z /app/lib/plrtext/ZZZ \
    && mkdir -p /app/lib/etc /app/lib/house /tmp/shadowwind

EXPOSE 9999

CMD ["./bin/shadowwind", "-d", "lib", "9999"]
