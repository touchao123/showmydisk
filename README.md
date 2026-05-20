# showmydisk

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A lightweight C utility to display disk usage for mounted filesystems.
Supports filtering, JSON output, tmux integration, and custom labels.

## Features

- Show disk usage with progress bars
- Filter by mount path and filesystem type
- **Tmux integration**: device names or custom labels in status bar
- **--label acts as filter**: only labeled mounts appear in output
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
  --label <mount>=<name>  Assign label and filter: only labeled mounts shown
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
Device:            /dev/mmcblk0p2
Display Name:      mmcblk0p2
Filesystem Type:   ext4
Total Size:        59356416 KB
Usage:             12.3%
Progress:          [####                            ]
```

### Tmux integration

Add to `~/.tmux.conf`:

```tmux
# Device names (shows all non-virtual mounts)
set -g status-right "#(showmydisk --tmux) %H:%M"

# With labels (only labeled mounts shown, recommended)
set -g status-right "#(showmydisk --label /=root --label /boot/firmware=boot --tmux) %H:%M"
```

**--label also acts as a filter** — only mounts with assigned labels appear:

```bash
# Without label: all non-virtual mounts (can be noisy)
$ showmydisk --tmux
tmpfs:10% mmcblk0p2:12% mmcblk0p1:15% tmpfs:8%

# With label: only the mounts you care about
$ showmydisk --label /=disk1 --label /boot/firmware=boot --tmux
disk1:12% boot:15%

# Single label
$ showmydisk --label /=root --tmux
root:12%
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

## Build & Install

```bash
gcc -Wall -Wextra -O2 -o showmydisk showmydisk.c
sudo cp showmydisk /usr/local/bin/
```

Or using Makefile:

```bash
make
sudo make install
```

## Version History

- **v0.4.2** — `--label` acts as filter (only labeled mounts shown)
- **v0.4.1** — Fix type filter bypass in tmux/json mode, fix multi-label output
- **v0.4.0** — Device names in tmux mode, `--label` support, `-h` flag, JSON fields
- **v0.3.0** — Fix `-h` unsupported, fix `--tmux` empty output
- **v0.2.0** — Initial release with tmux/json/filter support

## License

MIT
