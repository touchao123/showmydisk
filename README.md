# showmydisk

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A lightweight C utility to display disk usage for mounted filesystems.
Supports filtering, JSON output, tmux integration, and custom labels.

## Features

- Show disk usage with progress bars
- Filter by mount path and filesystem type
- **Tmux integration**: device names or custom labels in status bar
- JSON output for scripting
- Lightweight, written in C

## Usage

```
Usage: showmydisk [OPTIONS]

Display disk usage for mounted filesystems.

Options:
  --include <path>        Only show mount points containing <path>
  --exclude <path>        Exclude mount points containing <path>
  --type-include <type>   Only show filesystems of <type> (e.g. ext4, xfs)
  --type-exclude <type>   Exclude filesystems of <type>
  --label <mount>=<name>  Assign a custom label (e.g. --label /=disk1)
  --tmux                  Compact single-line output for tmux status bar
  --json                  Output in JSON format (machine-readable)
  -h, --help              Show this help message
  --version               Show version
```

## Examples

### Normal mode

```bash
$ showmydisk
Mount Point:       /
Filesystem Type:   ext4
Total Size:        59356416 KB
Usage:             12.3%
Progress:          [####                            ]
```

### Tmux integration

Add to `~/.tmux.conf`:

```tmux
# Show device name (e.g. mmcblk0p2:12%)
set -g status-right "#(showmydisk --tmux) %H:%M"

# Or with custom labels (recommended)
set -g status-right "#(showmydisk --label /=root --tmux) %H:%M"
```

Output examples:

```bash
# Default: shows device basename
$ showmydisk --tmux
mmcblk0p2:12%

# Custom labels: --label <mount>=<name>
$ showmydisk --label /=root --tmux
root:12%

# Multiple labels
$ showmydisk --label /=disk1 --label /boot/firmware=boot --tmux
disk1:12% boot:45%
```

### JSON output

```bash
$ showmydisk --json
[
  {"mount":"/","device":"/dev/mmcblk0p2","name":"mmcblk0p2","type":"ext4","total_kb":59356416,"usage_pct":12.3}
]
```

### Filtering

```bash
# Only show /home partition
showmydisk --include /home

# Exclude tmpfs filesystems
showmydisk --type-exclude tmpfs

# Show ext4 filesystems containing /data
showmydisk --include /data --type-include ext4
```

## Requirements

- Linux (uses `/etc/mtab` for mount info)
- C compiler (gcc/clang) for building
- CMake (optional, for building with cmake)

## Build & Install

```bash
# Build
gcc -Wall -Wextra -O2 -o showmydisk showmydisk.c

# Install
sudo cp showmydisk /usr/local/bin/
```

Or using Makefile:

```bash
make
sudo make install
```

## Version History

- **v0.4.0** — Device names in tmux mode, `--label` support, `-h` flag, JSON field expansion
- **v0.3.0** — Fix `-h` unsupported, fix `--tmux` empty output on non-/home mounts
- **v0.2.0** — Initial release with tmux/json/filter support

## License

MIT
