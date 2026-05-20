#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#define PROGRESS_BAR_WIDTH 30
#define VERSION "0.4.2"
#define MAX_FILTERS 64
#define MAX_LABELS 32

/* ── Label struct ── */
typedef struct {
    char mount[256];
    char label[256];
} LabelEntry;

/* ── Forward declarations ── */
static bool should_include_mountpoint(const char *mountpoint,
    char *include_filters[], int include_count,
    char *exclude_filters[], int exclude_count,
    char *type_include_filters[], int type_include_count,
    char *type_exclude_filters[], int type_exclude_count,
    const char *filesystem_type, bool has_cli_args,
    bool bypass_type_filter);

static const char *get_label(const char *mount, LabelEntry labels[], int label_count);
static const char *strip_dev(const char *device);

static void print_usage(const char *prog);
static void print_version(void);
static int is_virtual_fs(const char *type);

/* ── Help text ── */
static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Display disk usage for mounted filesystems.\n\n");
    printf("Options:\n");
    printf("  --include <path>        Only show mount points containing <path>\n");
    printf("  --exclude <path>        Exclude mount points containing <path>\n");
    printf("  --type-include <type>   Only show filesystems of <type> (e.g. ext4, xfs)\n");
    printf("  --type-exclude <type>   Exclude filesystems of <type>\n");
    printf("  --label <mount>=<name>  Assign a custom label (e.g. --label /=disk1)\n");
    printf("  --tmux                  Compact single-line output for tmux status bar\n");
    printf("  --json                  Output in JSON format (machine-readable)\n");
    printf("  -h, --help              Show this help message\n");
    printf("  --version               Show version\n\n");
    printf("Examples:\n");
    printf("  %s\n", prog);
    printf("  %s --tmux\n", prog);
    printf("  %s --label /=disk1 --label /mnt/data=disk2 --tmux\n", prog);
    printf("  %s --include /home --type-include ext4\n", prog);
    printf("  %s --type-exclude tmpfs\n", prog);
    printf("\nTmux integration:\n");
    printf("  Add to ~/.tmux.conf:\n");
    printf("    set -g status-right \"#(%s --tmux) %%H:%%M\"\n", prog);
    printf("  With labels:\n");
    printf("    set -g status-right \"#(%s --label /=root --tmux) %%H:%%M\"\n", prog);
}

static void print_version(void) {
    printf("showmydisk version %s\n", VERSION);
}

static int is_virtual_fs(const char *type) {
    return strcmp(type, "proc") == 0 ||
           strcmp(type, "sysfs") == 0 ||
           strcmp(type, "devtmpfs") == 0 ||
           strcmp(type, "devpts") == 0 ||
           strcmp(type, "cgroup") == 0 ||
           strcmp(type, "cgroup2") == 0;
}

/* ── Label helpers ── */
static const char *get_label(const char *mount, LabelEntry labels[], int label_count) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(mount, labels[i].mount) == 0)
            return labels[i].label;
    }
    return NULL;
}

/* Strip /dev/ prefix from device path to get basename */
static const char *strip_dev(const char *device) {
    if (device == NULL) return NULL;
    const char *p = strrchr(device, '/');
    return p ? p + 1 : device;
}

/* ── Main ── */
int main(int argc, char *argv[]) {
    FILE *fp;
    struct mntent *mnt;
    struct statvfs vfs;
    double usage;
    int used_blocks;
    char *progress_bar;
    long long total_size_kb;

    char *include_filters[MAX_FILTERS];
    char *exclude_filters[MAX_FILTERS];
    char *type_include_filters[MAX_FILTERS];
    char *type_exclude_filters[MAX_FILTERS];

    LabelEntry labels[MAX_LABELS];
    int label_count = 0;

    int include_count = 0;
    int exclude_count = 0;
    int type_include_count = 0;
    int type_exclude_count = 0;
    int i;

    bool has_cli_args = false;
    bool show_usage    = false;
    bool tmux_mode     = false;
    bool json_mode     = false;
    int exit_code      = 0;

    /* Initialize arrays */
    memset(include_filters, 0, sizeof(include_filters));
    memset(exclude_filters, 0, sizeof(exclude_filters));
    memset(type_include_filters, 0, sizeof(type_include_filters));
    memset(type_exclude_filters, 0, sizeof(type_exclude_filters));
    memset(labels, 0, sizeof(labels));

    /* Parse command-line arguments */
    for (i = 1; i < argc; i++) {
        has_cli_args = true;

        if (strcmp(argv[i], "--include") == 0 && i + 1 < argc) {
            if (include_count < MAX_FILTERS)
                include_filters[include_count++] = argv[++i];
        } else if (strcmp(argv[i], "--exclude") == 0 && i + 1 < argc) {
            if (exclude_count < MAX_FILTERS)
                exclude_filters[exclude_count++] = argv[++i];
        } else if (strcmp(argv[i], "--type-include") == 0 && i + 1 < argc) {
            if (type_include_count < MAX_FILTERS)
                type_include_filters[type_include_count++] = argv[++i];
        } else if (strcmp(argv[i], "--type-exclude") == 0 && i + 1 < argc) {
            if (type_exclude_count < MAX_FILTERS)
                type_exclude_filters[type_exclude_count++] = argv[++i];
        } else if (strcmp(argv[i], "--label") == 0 && i + 1 < argc) {
            i++;
            char *arg = argv[i];
            char *eq = strchr(arg, '=');
            if (eq && label_count < MAX_LABELS) {
                int mlen = (int)(eq - arg);
                if (mlen > 0 && mlen < 256) {
                    strncpy(labels[label_count].mount, arg, mlen);
                    labels[label_count].mount[mlen] = '\0';
                    strncpy(labels[label_count].label, eq + 1, 255);
                    labels[label_count].label[255] = '\0';
                    label_count++;
                }
            }
        } else if (strcmp(argv[i], "--tmux") == 0) {
            tmux_mode = true;
        } else if (strcmp(argv[i], "--json") == 0) {
            json_mode = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_usage = true;
            exit_code  = 0;
            break;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else {
            fprintf(stderr, "Error: Invalid argument '%s'\n\n", argv[i]);
            show_usage = true;
            exit_code  = 1;
            break;
        }
    }

    if (show_usage) {
        print_usage(argv[0]);
        return exit_code;
    }

    /* If no args, default to monitoring /home on ext4/ext3 */
    if (!has_cli_args && !tmux_mode && !json_mode) {
        include_filters[0] = "/home";
        include_count = 1;
        type_include_filters[0] = "ext4";
        type_include_filters[1] = "ext3";
        type_include_count = 2;
    }

    /* tmux/json without custom filters: show all non-virtual filesystems */
    if ((tmux_mode || json_mode) && include_count == 0 && type_include_count == 0) {
        include_filters[0] = "/";
        include_count = 1;
    }

    bool bypass_type_filter = (tmux_mode || json_mode) && type_include_count == 0;

    /* ── JSON preamble ── */
    if (json_mode) {
        printf("[\n");
    }

    fp = setmntent("/etc/mtab", "r");
    if (fp == NULL) {
        perror("setmntent");
        return 1;
    }

    bool first_entry = true;
    bool any_output  = false;

    while ((mnt = getmntent(fp)) != NULL) {
        if (mnt == NULL || mnt->mnt_dir == NULL || mnt->mnt_type == NULL)
            continue;

        if (is_virtual_fs(mnt->mnt_type))
            continue;

        /* Apply filters */
        if (!should_include_mountpoint(mnt->mnt_dir,
                include_filters, include_count,
                exclude_filters, exclude_count,
                type_include_filters, type_include_count,
                type_exclude_filters, type_exclude_count,
                mnt->mnt_type, has_cli_args, bypass_type_filter))
            continue;

        if (statvfs(mnt->mnt_dir, &vfs) != 0) {
            /* silent: statvfs failed */
            continue;
        }

        if (vfs.f_blocks == 0) {
            /* silent: zero-block fs */
            continue;
        }

        usage = (double)(vfs.f_blocks - vfs.f_bfree) / vfs.f_blocks * 100.0;
        total_size_kb = (long long)vfs.f_blocks * vfs.f_frsize / 1024;

        /* ── Determine display name ── */
        const char *display_name = NULL;

        /* Check for user-defined label */
        if (label_count > 0) {
            display_name = get_label(mnt->mnt_dir, labels, label_count);
        }

        /* If --label was given but this mount has no label, skip it */
        if (label_count > 0 && display_name == NULL)
            continue;

        /* Fallback: device basename (strip /dev/) */
        if (display_name == NULL && mnt->mnt_fsname != NULL) {
            display_name = strip_dev(mnt->mnt_fsname);
        }

        /* Last resort: mount point */
        if (display_name == NULL) {
            display_name = mnt->mnt_dir;
        }

        /* ── Json mode ── */
        if (json_mode) {
            if (!first_entry) printf(",\n");
            first_entry = false;
            printf("  {\"mount\":\"%s\",\"device\":\"%s\",\"name\":\"%s\",\"type\":\"%s\",\"total_kb\":%lld,\"usage_pct\":%.1f}",
                   mnt->mnt_dir, mnt->mnt_fsname ? mnt->mnt_fsname : "", display_name,
                   mnt->mnt_type, total_size_kb, usage);
            any_output = true;
            continue;
        }

        /* ── Tmux mode ── */
        if (tmux_mode) {
            printf("%s:%.0f%% ", display_name, usage);
            any_output = true;
            continue;
        }

        /* ── Normal mode ── */
        progress_bar = malloc(PROGRESS_BAR_WIDTH + 3);
        if (progress_bar == NULL) {
            perror("malloc");
            endmntent(fp);
            return 1;
        }

        used_blocks = (int)(usage * PROGRESS_BAR_WIDTH / 100.0);

        progress_bar[0] = '[';
        for (int j = 1; j <= used_blocks; j++)
            progress_bar[j] = '#';
        for (int j = used_blocks + 1; j <= PROGRESS_BAR_WIDTH; j++)
            progress_bar[j] = ' ';
        progress_bar[PROGRESS_BAR_WIDTH + 1] = ']';
        progress_bar[PROGRESS_BAR_WIDTH + 2] = '\0';

        printf("Mount Point:       %s\n", mnt->mnt_dir);
        printf("Device:            %s\n", mnt->mnt_fsname ? mnt->mnt_fsname : "");
        printf("Display Name:      %s\n", display_name);
        printf("Filesystem Type:   %s\n", mnt->mnt_type);
        printf("Total Size:        %lld KB\n", total_size_kb);
        printf("Usage:             %.1f%%\n", usage);
        printf("Progress:          %s\n", progress_bar);
        printf("\n");

        free(progress_bar);
        any_output = true;
    }

    endmntent(fp);

    /* ── JSON postamble ── */
    if (json_mode) {
        if (!any_output) printf("\n");
        printf("]\n");
    }

    /* ── Tmux: trailing newline so shell substitution doesn't eat output ── */
    if (tmux_mode && any_output) {
        printf("\n");
    }

    return 0;
}

/* ── Filter logic ── */
static bool should_include_mountpoint(const char *mountpoint,
    char *include_filters[], int include_count,
    char *exclude_filters[], int exclude_count,
    char *type_include_filters[], int type_include_count,
    char *type_exclude_filters[], int type_exclude_count,
    const char *filesystem_type, bool has_cli_args,
    bool bypass_type_filter)
{
    (void)has_cli_args;
    bool include      = false;
    bool type_include = false;

    /* Path: if user gave --include, respect it; otherwise default to /home */
    if (include_count == 0) {
        include = (strstr(mountpoint, "/home") != NULL);
    } else {
        for (int i = 0; i < include_count; i++) {
            if (strstr(mountpoint, include_filters[i]) != NULL) {
                include = true;
                break;
            }
        }
    }

    /* Path exclude */
    for (int i = 0; i < exclude_count; i++) {
        if (strstr(mountpoint, exclude_filters[i]) != NULL)
            return false;
    }

    /* Type: if user gave --type-include, respect it; otherwise default to ext4/ext3 */
    if (bypass_type_filter) {
        /* tmux/json mode: show all non-virtual filesystem types */
        type_include = true;
    } else if (type_include_count == 0) {
        type_include = (strcmp(filesystem_type, "ext4") == 0 ||
                        strcmp(filesystem_type, "ext3") == 0);
    } else {
        for (int i = 0; i < type_include_count; i++) {
            if (strcmp(filesystem_type, type_include_filters[i]) == 0) {
                type_include = true;
                break;
            }
        }
    }

    /* Type exclude */
    for (int i = 0; i < type_exclude_count; i++) {
        if (strcmp(filesystem_type, type_exclude_filters[i]) == 0)
            return false;
    }

    return include && type_include;
}
