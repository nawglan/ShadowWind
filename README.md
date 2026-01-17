# ShadowWind MUD

A Multi-User Dungeon (MUD) game engine based on CircleMUD 3.0.

## Status

This codebase is in **maintenance mode**. No new features are planned.

## Prerequisites

- GCC compiler
- libsodium development library
- clang-format (for code formatting)
- cppcheck (for static analysis)

### Ubuntu/Debian

```bash
sudo apt-get install gcc make libsodium-dev clang-format cppcheck
```

## Building

```bash
# Build the MUD binary
make build

# Build utilities (autowiz, sign, lookup_process)
make -C src utils

# Build everything
make -C src all
```

The compiled binary will be at `bin/shadowwind`.

## Running

```bash
# Start the MUD on port 9999
./bin/shadowwind -d lib 9999

# Or use the autorun script for automatic restart
./autorun
```

## Docker

```bash
# Build Docker image
make docker-build

# Run with Docker
docker run -p 9999:9999 shadowwind
```

## Development

```bash
# Format code
make format

# Check formatting (CI)
make format-check

# Run static analysis
make lint
```

## Directory Structure

- `src/` - Source code
- `lib/` - Game data (world files, text, etc.)
- `bin/` - Compiled binaries
- `docs/` - Documentation
- `webclient/` - Web-based MUD client
- `deploy/` - Kubernetes deployment files

## License

CircleMUD License - See `license` file for details.
