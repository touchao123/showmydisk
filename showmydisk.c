#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define PROGRESS_BAR_WIDTH 30

bool should_include_mountpoint(const char *mountpoint, char *include_filters[], int include_count, char *exclude_filters[], int exclude_count, char *type_include_filters[], int type_include_count, char *type_exclude_filters[], int type_exclude_count, const char* filesystem_type, bool has_cli_args);

int main(int argc, char *argv[]) {
    FILE *fp;
    struct mntent *mnt;
    struct statvfs vfs;
    double usage;
    int used_blocks, unused_blocks;
    char *progress_bar;
    long long total_size_kb;

    char *include_filters[10];
    char *exclude_filters[10];
    char *type_include_filters[10];
    char *type_exclude_filters[10];

    int include_count = 0;
    int exclude_count = 0;
    int type_include_count = 0;
    int type_exclude_count = 0;
    int i;

    bool has_cli_args = false; // 用于跟踪是否提供了任何命令行参数
    bool show_usage = false; // 用于标记是否需要显示 usage 信息

    // 初始化过滤器数组
    for (i = 0; i < 10; i++) {
        include_filters[i] = NULL;
        exclude_filters[i] = NULL;
        type_include_filters[i] = NULL;
        type_exclude_filters[i] = NULL;
    }

    // 解析命令行参数
    for (i = 1; i < argc; i++) {
        has_cli_args = true; // 设置为 true，因为提供了至少一个参数

        if (strcmp(argv[i], "--include") == 0 && i + 1 < argc) {
            if (include_count < 10) {
                include_filters[include_count] = argv[i + 1];
                include_count++;
                i++;
            } else {
                fprintf(stderr, "Warning: Too many include filters.  Ignoring additional ones.\n");
                i++;
            }
        } else if (strcmp(argv[i], "--exclude") == 0 && i + 1 < argc) {
            if (exclude_count < 10) {
                exclude_filters[exclude_count] = argv[i + 1];
                exclude_count++;
                i++;
            } else {
                fprintf(stderr, "Warning: Too many exclude filters. Ignoring additional ones.\n");
                i++;
            }
        } else if (strcmp(argv[i], "--type-include") == 0 && i + 1 < argc) {
            if (type_include_count < 10) {
                type_include_filters[type_include_count] = argv[i + 1];
                type_include_count++;
                i++;
            } else {
                fprintf(stderr, "Warning: Too many type-include filters.  Ignoring additional ones.\n");
                i++;
            }
        } else if (strcmp(argv[i], "--type-exclude") == 0 && i + 1 < argc) {
            if (type_exclude_count < 10) {
                type_exclude_filters[type_exclude_count] = argv[i + 1];
                type_exclude_count++;
                i++;
            } else {
                fprintf(stderr, "Warning: Too many type-exclude filters. Ignoring additional ones.\n");
                i++;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            show_usage = true; // 显示 usage
            break; // 停止解析参数
        } else {
            fprintf(stderr, "Error: Invalid argument '%s'\n", argv[i]); // 错误信息
            show_usage = true;
            break; // 停止解析参数
        }
    }

    // 显示 usage 信息
    if (show_usage) {
        fprintf(stderr, "Usage: %s [--include <path>]... [--exclude <path>]... [--type-include <type>]... [--type-exclude <type>]...\n", argv[0]);
        return show_usage ? 1 : 0; // 如果因为--help而显示，返回0, 错误返回1
    }

    // 如果没有提供命令行参数，则设置默认值
    if (!has_cli_args) {
        printf("Running with default settings: monitoring /home (ext4, ext3)\n\n");

        include_filters[0] = "/home";
        include_count = 1;

        type_include_filters[0] = "ext4";
        type_include_filters[1] = "ext3";
        type_include_count = 2;
    }

    fp = setmntent("/etc/mtab", "r");
    if (fp == NULL) {
        perror("setmntent");
        return 1;
    }

    while ((mnt = getmntent(fp)) != NULL) {
        if (mnt == NULL || mnt->mnt_dir == NULL || mnt->mnt_type == NULL) {
            fprintf(stderr, "Warning: Invalid mount entry, skipping\n");
            continue;
        }

        //跳过一些虚拟文件系统
        if (strcmp(mnt->mnt_type, "proc") == 0 || strcmp(mnt->mnt_type, "sysfs") == 0 || strcmp(mnt->mnt_type, "devtmpfs") == 0 || strcmp(mnt->mnt_type, "devpts") == 0) {
            continue;
        }


        // 应用过滤器
        if (!should_include_mountpoint(mnt->mnt_dir, include_filters, include_count, exclude_filters, exclude_count, type_include_filters, type_include_count, type_exclude_filters, type_exclude_count, mnt->mnt_type, has_cli_args)) {
            continue;
        }

        if (statvfs(mnt->mnt_dir, &vfs) == 0) {
            if (vfs.f_blocks == 0) {
                fprintf(stderr, "Warning: filesystem has zero blocks, skipping %s\n", mnt->mnt_dir);
                continue;
            }

            usage = (double)(vfs.f_blocks - vfs.f_bfree) / vfs.f_blocks * 100.0;

            progress_bar = (char*)malloc(PROGRESS_BAR_WIDTH + 3);
            if (progress_bar == NULL) {
                perror("malloc");
                endmntent(fp);
                return 1;
            }

            used_blocks = (int)(usage * PROGRESS_BAR_WIDTH / 100.0);
            unused_blocks = PROGRESS_BAR_WIDTH - used_blocks;

            progress_bar[0] = '[';
            for (int j = 1; j <= used_blocks; j++) {
                progress_bar[j] = '#';
            }
            for (int j = used_blocks + 1; j <= PROGRESS_BAR_WIDTH; j++) {
                progress_bar[j] = ' ';
            }
            progress_bar[PROGRESS_BAR_WIDTH + 1] = ']';
            progress_bar[PROGRESS_BAR_WIDTH + 2] = '\0';

            total_size_kb = (long long)vfs.f_blocks * vfs.f_frsize / 1024;

            printf("Mount Point: %s\n", mnt->mnt_dir);
            printf("Filesystem Type: %s\n", mnt->mnt_type);
            printf("Total Size: %lld KB\n", total_size_kb);
            printf("Usage: %.1f%%\n", usage);
            printf("Progress: %s\n", progress_bar);
            printf("\n");

            free(progress_bar);
        } else {
            perror(mnt->mnt_dir);
            fprintf(stderr, "Error getting information for mount point: %s, errno: %d, error string: %s\n", mnt->mnt_dir, errno, strerror(errno));
        }
    }

    endmntent(fp);
    return 0;
}

bool should_include_mountpoint(const char *mountpoint, char *include_filters[], int include_count, char *exclude_filters[], int exclude_count, char *type_include_filters[], int type_include_count, char *type_exclude_filters[], int type_exclude_count, const char* filesystem_type, bool has_cli_args) {
    bool include = false;
    bool type_include = false;

    // 处理路径 include/exclude
    // 如果没有提供命令行参数或手动指定 include 过滤器，则应用默认的 /home 过滤器
    if (!has_cli_args || include_count == 0) {
        // 检查是否匹配默认的 /home 过滤器
        if (strstr(mountpoint, "/home") != NULL) {
            include = true;
        } else {
            include = false; // 不匹配 /home 则不包含
        }
    }  else { // 如果提供了命令行参数，则使用用户指定的过滤器
        for (int i = 0; i < include_count; i++) {
            if (strstr(mountpoint, include_filters[i]) != NULL) {
                include = true;
                break;
            }
        }
    }


    for (int i = 0; i < exclude_count; i++) {
        if (strstr(mountpoint, exclude_filters[i]) != NULL) {
            return false; // 排除
        }
    }

    // 处理类型 include/exclude
    // 如果没有提供命令行参数或手动指定类型 include 过滤器，则应用默认的 ext4 和 ext3 过滤器
      if (!has_cli_args || type_include_count == 0) {
        // 检查是否匹配默认的 ext4 和 ext3 过滤器
        for (int i = 0; i < 2; i++) {
            if ( (strcmp(filesystem_type, "ext4") == 0 && i==0)||  (strcmp(filesystem_type, "ext3") == 0 && i==1)) {

                type_include = true;
                break;
            }else{
              type_include = false;
            }
        }
    } else {// 如果提供了命令行参数，则使用用户指定的过滤器
        for (int i = 0; i < type_include_count; i++) {
            if (strcmp(filesystem_type, type_include_filters[i]) == 0) {
                type_include = true;
                break;
            }
        }
    }


    for (int i = 0; i < type_exclude_count; i++) {
        if (strcmp(filesystem_type, type_exclude_filters[i]) == 0) {
            return false; // 排除
        }
    }

    return include && type_include; // 只有路径和类型都满足 include 条件才返回 true
}
