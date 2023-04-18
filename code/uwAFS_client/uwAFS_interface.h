#ifndef uwAFS_interface
#define uwAFS_interface

#ifdef __cplusplus
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <dirent.h>
#include <fuse.h>

#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC int wrap_getattr(const char *path, struct stat *statbuf);
//EXTERNC int wrap_afsopen(const char *path, struct fuse_file_info *fi);
EXTERNC int wrap_mkdir(const char *path, mode_t mode);
EXTERNC int wrap_rmdir(const char *path);
EXTERNC int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler);
EXTERNC int wrap_unlink(const char *path);
EXTERNC int wrap_open(const char *path, struct fuse_file_info *fi);
EXTERNC int wrap_read(int fd, char* buf, size_t size, size_t offset);
//EXTERNC int wrap_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
EXTERNC int wrap_close(const char* path, int fd);
EXTERNC int wrap_rename(char* old_path, char* new_path);
EXTERNC int wrap_flush(char* path, struct fuse_file_info *fi);
EXTERNC int wrap_mknod(const char *path, mode_t mode, dev_t dev);
EXTERNC int wrap_unlink(const char *path);

#undef EXTERNC

#endif