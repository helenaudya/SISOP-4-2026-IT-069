#define FUSE_USE_VERSION 28

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>


static char source_dir[1024];

void fullpath(char fpath[1024], const char *path)
{
    sprintf(fpath, "%s%s", source_dir, path);
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{

    int res;

     memset(stbuf, 0, sizeof(struct stat));

    // FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0)
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 4096;

        return 0;
    }

    char fpath[1024];

    fullpath(fpath, path);

    res = lstat(fpath, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    DIR *dp;
    struct dirent *de;

    char fpath[1024];
    fullpath(fpath, path);

    dp = opendir(fpath);

    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL)
    {
        struct stat st;

        memset(&st, 0, sizeof(st));

        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    filler(buf, "tujuan.txt", NULL, 0);
    return 0;
}

static int xmp_open(const char *path,
                    struct fuse_file_info *fi)
{
    // FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0)
        return 0;

    int res;

    char fpath[1024];
    fullpath(fpath, path);

    res = open(fpath, fi->flags);

    if (res == -1)
        return -errno;

    close(res);

    return 0;
}

static int xmp_read(const char *path,
                    char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
{
    (void) fi;

     // FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0)
    {
        static char content[4096];

        strcpy(content, "Tujuan Mas Amba: ");

        for (int i = 1; i <= 7; i++)
        {
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%d.txt", source_dir, i);

            FILE *fp = fopen(filepath, "r");

            if (!fp)
                continue;

            char line[256];

            while (fgets(line, sizeof(line), fp))
            {
                if (strncmp(line, "KOORD:", 6) == 0)
                {
                    char *frag = line + 6;

                    while (*frag == ' ')
                        frag++;

                    frag[strcspn(frag, "\n")] = '\0';

                    strcat(content, frag);
                }
            }

            fclose(fp);
        }

        strcat(content, "\n");

        size_t len = strlen(content);

        if (offset < len)
        {
            if (offset + size > len)
                size = len - offset;

            memcpy(buf, content + offset, size);
        }
        else
        {
            size = 0;
        }

        return size;
    }

    int fd;
    int res;

    char fpath[1024];
    fullpath(fpath, path);

    fd = open(fpath, O_RDONLY);

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);

    if (res == -1)
        res = -errno;

    close(fd);

    return res;
}

static const struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <source_dir> <mount_point>\n", argv[0]);
        return 1;
    }

    realpath(argv[1], source_dir);

    for (int i = 1; i < argc - 1; i++)
    {
        argv[i] = argv[i + 1];
    }

    argc--;

    return fuse_main(argc, argv, &xmp_oper, NULL);
}
