# Changelog

All notable changes to ShadowWind MUD are documented in this file.

Based on CircleMUD 3.00pl4 (ESC v0.5b).

## [Unreleased]

### Added
- Comprehensive code documentation across 10 critical source files
- CHANGELOG.md for tracking version history

### Changed
- Updated .gitignore with better organization
- Made autorun script portable (no hardcoded paths)

### Removed
- Obsolete documentation files (cvs.howto, sw_v0-5b.doc)
- Redundant README stub file

---

## 2026-01-17 - Maintenance Mode Preparation

### Added
- Code formatting configuration (.clang-format, .editorconfig)
- Static analysis configuration (.clang-tidy)
- CI pipeline for format/lint checks
- Include guards for all header files

### Changed
- Applied clang-format across entire codebase
- Updated README.md with deployment instructions

### Fixed
- Buffer overflows and resource leaks (cppcheck findings)
- Header dependency issues

### Security
- Replaced unsafe string functions (sprintf, strcpy, strcat)
- Added -D_FORTIFY_SOURCE=2 and -fstack-protector-strong flags

---

## 2026-01-14 - Security Hardening

### Fixed
- Critical security vulnerabilities in C codebase
- Input validation improvements
- MOBprog mpdelay errors in conditional blocks
- Malformed if/else/endif blocks in mobprogs

### Security
- Replaced sprintf/strcpy/strcat with safe_snprintf alternatives
- Fixed unsafe string functions in 15+ source files

---

## 2026-01-09 - Web Client

### Added
- MUD-aware webclient with telnet protocol support
- Docker containerization
- Kubernetes deployment manifests

### Fixed
- Webclient backspace allowing deletion beyond input

---

## 2026-01-07 - Modern Linux Compatibility

### Added
- safe_snprintf macro for buffer overflow detection

### Fixed
- All remaining compiler warnings
- Compilation errors for modern Linux/glibc
- -Wrestrict warnings in string operations
- Converted sprintf to snprintf throughout codebase

---

## Pre-2026 - Legacy

Original ShadowWind MUD based on CircleMUD 3.0 with extensive modifications:
- Custom spell/magic system with memorization
- MOBprogram scripting for NPC behavior
- Dynamic maze generation
- Event-based timing system
- Player rent/crash-save system
