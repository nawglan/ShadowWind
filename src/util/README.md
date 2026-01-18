# ShadowWind Utility Programs

This directory contains standalone utility programs for MUD administration.
These are **not** part of the main MUD server binary.

## Build

Utilities are built separately via the Makefile targets (from src/):
```bash
make ../bin/sign
make ../bin/autowiz
# etc.
```

## Utility Status

### Active / Maintained

| Program | Description | Status |
|---------|-------------|--------|
| `sign.c` | TCP port text display (login banner) | **Active** |
| `autowiz.c` | Auto-generate wizlist from playerfile | **Active** |
| `lookup_process.c` | Process lookup utility | **Active** |

### Legacy / Use With Caution

| Program | Description | Status |
|---------|-------------|--------|
| `cleanall.c` | Clean up orphaned player files | Legacy - review before use |
| `delobjs.c` | Delete object files for deleted players | Legacy - review before use |
| `purgeplay.c` | Purge old players from playerfile | Legacy - review before use |
| `fixmobs.c` | Fix/convert mobile files | Legacy - large, complex |
| `converter.c` | World file format converter | Legacy - old format support |

### Broken / Do Not Use

| Program | Description | Status |
|---------|-------------|--------|
| `nukeold.c` | Archive old player files | **BROKEN** - uses undeclared variables |
| `nukegold.c` | Zero out mob gold values | Legacy - limited use case |
| `ascii.c` | Minimal ASCII test | Trivial test file |

## Notes

- These utilities work with the binary playerfile format (`lib/etc/players`)
- Most were written for CircleMUD 3.0 and may need updates for ShadowWind changes
- Always backup data before running any purge/delete utilities
- The `sign` program runs as a daemon on a separate port for pre-login text

## Files

- `sign.text` - Sample text file for the sign daemon
