#include "uwAFS_interface.h"
#include "uwAFS.h"
#include <string>
#include <cstring>
#include <vector>

int wrap_getattr(const char *path, struct stat *statbuf){
    return UWAFSClient::Instance()->GetAttr(path, statbuf);
}

/*int wrap_afsopen(const char *path, struct fuse_file_info *fi){
    return UWAFSClient::Instance()->AfsOpen(path, fi);
}*/

int wrap_mkdir(const char *path, mode_t mode){
    return UWAFSClient::Instance()->MkDir(path,mode);
}

int wrap_rmdir(const char *path){
    return UWAFSClient::Instance()->RmDir(path);
}


int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler){
    return UWAFSClient::Instance()->ReadDir(path, buf, filler);
}

int wrap_open(const char *path, struct fuse_file_info *fi){
    return UWAFSClient::Instance()->Open(path, fi);
}

int wrap_read(int fd, char* buf, size_t size, size_t offset){
    return UWAFSClient::Instance()->Read(fd, buf, size, offset);
}

// int wrap_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
//     return UWAFSClient::Instance()->Write(path, buf, size, offset, fi);
// }

int wrap_close(const char* path, int fd){
    return UWAFSClient::Instance()->Close(path, fd);
}

int wrap_rename(char* old_path, char* new_path){
    return UWAFSClient::Instance()->Rename(old_path, new_path);
}

int wrap_flush(char* path, struct fuse_file_info *fi){
    return UWAFSClient::Instance()->Flush(path, fi);
}

int wrap_mknod(const char *path, mode_t mode, dev_t dev){
    return UWAFSClient::Instance()->Create(path, mode, dev);
}

int wrap_unlink(const char *path){
    return UWAFSClient::Instance()->Unlink(path);
}