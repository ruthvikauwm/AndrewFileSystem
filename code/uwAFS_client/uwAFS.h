#ifndef uwAFS_h
#define uwAFS_h

#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <fuse.h>

#include <grpc++/grpc++.h>

#include "../uwAFS.grpc.pb.h"

#include "cacheManager.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;
using grpc::Status;

using namespace uwAFS;

class UWAFSClient {
private:
    std::unique_ptr<UWAFS::Stub> stub_;
    static UWAFSClient *_instance;
    static CacheManager *cache;

    int AddToCache(std::map<std::string, std::size_t> m, std::string path, std::size_t hash){
        m[path] = hash;
        return 1;
    }

    // Check if the cache is currently storing a last access date for the file
    int CheckIfCached(std::map<std::string, std::size_t> m, std::string path){
        if(m.find(path) == m.end()){
            return 0;
        }
        return 1;
    }

    int Rename(std::map<std::string, std::size_t> m, std::string old_path, std::string new_path){
            auto it = m.find(old_path);
            if(it != m.end()){
                std::swap(m[new_path], it->second);
                m.erase(old_path);
            }
            //std::cout << "renaming " << old_path << " to " << new_path << "\n";
            rename(old_path.c_str(), new_path.c_str());
            //rename((write_marker_dir + old_path).c_str(), (write_marker_dir + new_path).c_str());
            return 0;
        }

public:
    static UWAFSClient *Instance();

	UWAFSClient();
	~UWAFSClient();

	UWAFSClient(std::shared_ptr<Channel> channel)
        : stub_(UWAFS::NewStub(channel)) {}
        
    int GetAttr(const std::string& path, struct stat *buf);
    //int AfsOpen(const char *path, struct fuse_file_info *fi);
    int MkDir(const char *path, mode_t mode);
    int RmDir(const char *path);
    int ReadDir(const char *path, void *buf, fuse_fill_dir_t filler);
    int Open(const char* path, struct fuse_file_info *fi);
    int Read(int fd, char* buf, size_t size, size_t offset);
    int Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int Close(const char *path, int fd);
    int Rename(char* old_path, char* new_path);
    int Flush(const char* path, struct fuse_file_info *fi);
    int Create(const char *path, mode_t mode, dev_t dev);
    int Unlink(const char *path);

};

#endif