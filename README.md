# showmydisk

> A lightweight CLI tool to display disk usage for mounted filesystems — built for shell lovers and tmux users.

## Install

### From source

```bash
git clone https://github.com/touchao123/showmydisk.git
cd showmydisk
make
sudo make install
```

### With Docker

```bash
docker build -t showmydisk .
docker run --rm -v /:/host:ro showmydisk /host
```

## Usage

### Default mode — monitor `/home` on `ext4/ext3`

```bash
showmydisk
```

```
Running with default settings: monitoring /home (ext4, ext3)

Mount Point:       /home
Filesystem Type:   ext4
Total Size:        102400000 KB
Usage:             42.3%
Progress:          [████████████░░░░░░░░░░░░░░░░░░]
```

### Filter by mount path

```bash
# Only show mounts containing /home
showmydisk --include /home

# Show multiple paths
showmydisk --include / --include /home

# Exclude certain paths
showmydisk --exclude /snap
```

### Filter by filesystem type

```bash
# Only ext4
showmydisk --type-include ext4

# Only ext4 and xfs
showmydisk --type-include ext4 --type-include xfs

# Exclude tmpfs
showmydisk --type-exclude tmpfs

# Combine path and type filters
showmydisk --include /home --type-include xfs
```

## Tmux integration

Add to `~/.tmux.conf`:

```tmux
# Compact disk usage in status bar
set -g status-right "#(showmydisk --tmux) %H:%M"
```

Output in status bar looks like:

```
/home:42% /:67%
```

The `--tmux` flag prints a single compact line with mount point + usage percentage, perfect for tmux status bars.

If you want a shorter path:

```bash
showmydisk --include /home --tmux
```

Output:

```
/home:42%
```

## Options

| Flag | Description |
|------|-------------|
| `--include <path>` | Only show mount points containing `<path>` |
| `--exclude <path>` | Exclude mount points containing `<path>` |
| `--type-include <type>` | Only show filesystems of `<type>` (e.g. `ext4`, `xfs`) |
| `--type-exclude <type>` | Exclude filesystems of `<type>` |
| `--tmux` | Compact single-line output for tmux status bar |
| `--json` | JSON output (machine-readable) |
| `--help` | Show help message |
| `--version` | Show version |

## JSON output (for scripting)

```bash
showmydisk --json
```

```json
[
  {"mount":"/home","type":"ext4","total_kb":102400000,"usage_pct":42.3},
  {"mount":"/data","type":"xfs","total_kb":512000000,"usage_pct":67.1}
]
```

## License

MIT
