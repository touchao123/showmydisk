# showmydisk


gcc showmydisk.c -o showmydisk
cp showmydisk /usr/bin/


# 只显示 ext4 文件系统
./showmydisk --type-include ext4
# 显示 ext4 和 xfs 文件系统
./showmydisk --type-include ext4 --type-include xfs
# 排除 tmpfs 文件系统
./showmydisk --type-exclude tmpfs
# 监控/home，但是只监控xfs文件系统。
./showmydisk --include /home --type-include xfs
