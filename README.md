# ShadowWind MUD

A Multi-User Dungeon (MUD) game engine based on CircleMUD 3.0.

## Status

This codebase is in **maintenance mode**. No new features are planned.

## Prerequisites

### Build Requirements
- GCC compiler
- make
- libsodium development library

### Development Tools
- clang-format (for code formatting)
- cppcheck (for static analysis)

### Deployment Requirements
- Docker (for containerized deployment)
- kubectl (for Kubernetes deployment)

### Ubuntu/Debian

```bash
# Build essentials
sudo apt-get install gcc make libsodium-dev

# Development tools
sudo apt-get install clang-format cppcheck

# Deployment tools (optional)
sudo apt-get install docker.io
# For kubectl, see: https://kubernetes.io/docs/tasks/tools/
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
# Build MUD Docker image
make docker-build

# Build webclient Docker image
make webclient-build

# Push images to registry (default: localhost:30500)
make docker-push
make webclient-push

# Run locally with Docker
docker run -p 9999:9999 localhost:30500/shadowwind:latest
```

## Kubernetes Deployment

```bash
# Full deployment (builds, pushes, and deploys)
make deploy

# Or step by step:
make k8s-deploy      # Deploy to Kubernetes
make k8s-status      # Check deployment status
make k8s-logs        # View MUD container logs
make k8s-logs-webclient  # View webclient logs
make k8s-port-forward    # Port-forward webclient to localhost:8080
make k8s-delete      # Remove deployment
```

Kubernetes manifests are in `deploy/k8s/`.

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
