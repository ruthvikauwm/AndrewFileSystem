#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <cstdlib>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <iterator>

#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>

#include <openssl/sha.h>


#include <string.h>


class CacheManager {
    private:
        std::string cache_prefix; // NOTE: Might not need this, depends if FUSE treats / or /tmp/base/ as root
        
        std::string write_marker_dir = "/tmp/base/.writes";
        std::string mod_suffix = ".diff";

    public:
        std::map<std::string, std::size_t> opens;
        // Constructor, pass in base_dir which will usually be /tmp/base
        CacheManager() {};
	    ~CacheManager() {};

        CacheManager(std::string base_dir): cache_prefix(base_dir) { 
            mkdir(write_marker_dir.c_str(), 0777);
        }; 


        // Shave filename off end of path, leaving the directory hierarchy
        std::string GetFolder(const std::string path) {
            const size_t found = path.find_last_of("/\\");
            return (found == std::string::npos) ? path : path.substr(0, found);
        }

        // Check if a file exists
        int CheckExists(std::string path){
            return access(path.c_str(), F_OK);
        }

        // Get current time
        std::size_t GetStatHash(std::string path){
            return opens[path];
            // struct stat st;
            // stat(path.c_str(), &st);
            // std::string buf;
            // buf.resize(sizeof(struct stat));
            // memcpy(&buf[0], &st, sizeof(struct stat));
            // return std::hash<std::string>{}(buf);
        }

        // https://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
        static int MakeDir(const char *path, mode_t mode){
            struct stat st;
            int status = 0;

            if (stat(path, &st) != 0){
                /* Directory does not exist. EEXIST for race condition */
                if (mkdir(path, mode) != 0 && errno != EEXIST){
                    status = -1;
                }
            } else if (!S_ISDIR(st.st_mode)){
                errno = ENOTDIR;
                status = -1;
            }

            return(status);
        }

        // https://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
        int MakePath(const char *path, mode_t mode){
            char *pp;
            char *sp;
            int status;
            char *copypath = strdup(path);

            status = 0;
            pp = copypath;
            while (status == 0 && (sp = strchr(pp, '/')) != 0){
                if (sp != pp){
                    /* Neither root nor double slash in path */
                    *sp = '\0';
                    status = MakeDir(copypath, mode);
                    *sp = '/';
                }
                pp = sp + 1;
            }

            if (status == 0){
                status = MakeDir(path, mode);
            }

            free(copypath);
            return (status);
        }

        int AddToCache(std::string path, std::size_t hash){
            opens[path] = hash;
            return 1;
        }

        // Check if the cache is currently storing a last access date for the file
        int CheckIfCached(std::string path){
            if(opens.find(path) == opens.end()){
                return 0;
            }
            return 1;
        }

        // Reading part of a local file into buf
        int LocalRead(uint64_t fd, char* buf, size_t size, size_t offset) {
            int res = pread(fd, buf, size, offset);
            return res;
        }

        int LocalRead(uint64_t fd, std::string buf, size_t size, size_t offset) {
            int res = pread(fd, &buf[0], size, offset);
            return res;
        }

        // Checks if file has been modified locally
        int CheckMods(std::string path){
            return access((path + mod_suffix).c_str(), F_OK);
        }

        int AddMod(std::string path){
            std::cout << "\nCreating file with path " << (path + mod_suffix) <<"\n\n";
            // std::string dirs = GetFolder(path);
            // MakePath((write_marker_dir + dirs).c_str(), 0777);
            std::fstream file;
            file.open((path + mod_suffix).c_str(), std::ios::out);
            file.close();
            return 0;
        }

        int RemoveMod(std::string path){
            return remove((path + mod_suffix).c_str());
        }

        // Reading a full local file into buf
        int LocalTotalRead(std::string path, std::string& buf) {
            std::ifstream f(path.c_str());
            if(f){
                std::ostringstream ss;
                ss << f.rdbuf();
                buf = ss.str();
                return 0;
            }
            return 1;
        }

        // Write a file locally
        int LocalWrite(std::string path, std::string data, int fh, int from_server){
            // Create any necessary parent directories
            std::string dirs = GetFolder(path);
            MakePath(dirs.c_str(), 0700);
            //MakeDir((write_marker_dir + dirs).c_str(), 0777);

            if(from_server == 0){
            // Create a marker file such that the client knows, on close, to send the changes to server
                int fd = open((path + mod_suffix).c_str(), O_RDWR | O_CREAT);
                if(fd < 0){
                    std::cout << "ERROR: Error creating the write marker file";
                    close(fd);
                    return 0;
                }
                close(fd);
            }

            // Open the file and send the file descriptor to FUSE
            int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            fh = fd;

            // Write to the file
            int res = pwrite(fd, &data[0], data.size(), 0);
            lseek(fd, 0, SEEK_CUR);
            if(res == -1){
                close(fd);
                return -errno;
            }
            return 0;
        }

        int Rename(std::string old_path, std::string new_path){
            auto it = opens.find(old_path);
            if(it != opens.end()){
                std::swap(opens[new_path], it->second);
                opens.erase(old_path);
            }
            //std::cout << "renaming " << old_path << " to " << new_path << "\n";
            rename(old_path.c_str(), new_path.c_str());
            //rename((write_marker_dir + old_path).c_str(), (write_marker_dir + new_path).c_str());
            return 0;
        }

        int Remove(std::string path){
            opens.erase(path);
            return 0;
        }



};

#endif


