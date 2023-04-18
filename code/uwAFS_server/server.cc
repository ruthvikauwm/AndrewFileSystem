#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <errno.h>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <grpc++/grpc++.h>
#include "../uwAFS_client/cacheManager.h"

#include "../uwAFS.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::ServerReader;
using grpc::Status;

using namespace uwAFS;



std::map<std::string, std::size_t> m;

class UWAFS_Server final : public UWAFS::Service {
    private:
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
        const std::string path_prefix;
        std::string basedir = "";
        static CacheManager *cache;

        UWAFS_Server(): path_prefix("/tmp/serverfs") {
            if(cache == NULL){
                cache = new CacheManager("/tmp/serverfs"); // TODO: get rid of /tmp/base in params
            }
        }

        std::string getFolderPath(const std::string &path) {
            const size_t found = path.find_last_of("/\\");
            return (found == std::string::npos) ? path : path.substr(0, found);
        }

        std::string getTruePath(std::string path){
            std::string::size_type i = path.find(basedir);
            std::string client_path = path;
            if(i != std::string::npos){
                //std::cout << "NOTICE:: found "<< basedir << "in "<< path << "\n";
                if(client_path[client_path.length() - 1] == '/'){
                    client_path.erase(i, basedir.length()); // Remove all but last /
                } else{
                    client_path.erase(i, basedir.length());
                }
                //std::cout << "NOTICE:: client_path is now " << client_path << "\n";
            }
            return path_prefix + client_path;

        }

        Status GetAttr(ServerContext* context, const GetAttrReq* request,
                        GetAttrReply* reply) override {
            
            if(basedir.empty()){
                basedir = getFolderPath(request->path());
                //std::cout << "NOTICE:: saving " << request->path() << " as basedir\n";
            }

            struct stat statbuf;
            std::string path = getTruePath(request->path());
            //std::cout << "NOTICE:: received a getattr request from client with path " << path << "\n";
            int retstat = lstat(path.c_str(), &statbuf);

            if (retstat < 0) {
                //std::cout << "NOTICE:: error with lstat of path " << path << "\n";
                reply->set_err(-errno);
                return Status::OK;
            } 

            //Populate the response
            Owner owner;
            owner.set_uid(statbuf.st_uid);
            owner.set_gid(statbuf.st_gid);

            Attr attributes;
            attributes.set_dev(statbuf.st_dev);
            attributes.set_ino(statbuf.st_ino);
            attributes.set_mode(statbuf.st_mode);
            attributes.set_nlink(statbuf.st_nlink);
            attributes.mutable_owner()->CopyFrom(owner);
            attributes.set_rdev(statbuf.st_rdev);
            attributes.set_size(statbuf.st_size);
            attributes.set_blksize(statbuf.st_blksize);
            attributes.set_blocks(statbuf.st_blocks);
            attributes.set_atime(statbuf.st_atime);
            attributes.set_mtime(statbuf.st_mtime);
            attributes.set_ctime(statbuf.st_ctime);

            reply->mutable_attr()->CopyFrom(attributes);
            reply->set_err(retstat);
            //std::cout << "NOTICE:: sent a reply to client\n";
            return Status::OK;
        }

         Status MkDir(ServerContext* context, const MkDirReq* request, MkDirReply* reply)  override
        {
            std::string path = getTruePath(request->path());

            reply->set_err(0);
            int res=mkdir(path.c_str(),request->mode());
            if(res==-1){
                reply->set_err(-errno);
                return Status::OK;
            }
            reply->set_err(res);
                return Status::OK;
        }

        Status ReadDir(ServerContext* context, const ReadDirReq* request, ReadDirReply* reply) override {
            // default errno = 0
            reply->set_err(0);
            std::string path = getTruePath(request->path());

            DIR *dp;
            struct dirent *de;

            dp = opendir(path.c_str());
            if (dp == NULL) {
                std::cout << "NOTICE:: err";
                reply->set_err(-errno);
                return Status::OK;
            }

            while((de = readdir(dp)) != NULL){
                //std::cout << "NOTICE:: adding dirent with name " << de->d_name << "\n";
                DirEnt *dent = reply->add_dirents();
                dent->set_name(de->d_name);
                dent->set_inodenum(de->d_ino);
                dent->set_mode(de->d_type << 12);
            }
            
            closedir(dp);
            return Status::OK;
        }

        Status RmDir(ServerContext* context, const RmDirReq* request, RmDirReply* reply) override {
            std::string path = getTruePath(request->path());

            reply->set_err(0);
            int res=rmdir(path.c_str());
            if(res==-1){
                reply->set_err(-errno);
                return Status::OK;
            }
            reply->set_err(res);
            return Status::OK;

        }


        Status GetHash(ServerContext* context, const GetHashReq* request, GetHashReply* reply) override {
            std::string path = getTruePath(request->path());

            struct stat st;
            int res = stat(path.c_str(), &st);
            if(res != 0){
                reply->set_exists(0);
                return Status::OK;
            }
            std::string buf;
            buf.resize(sizeof(struct stat));
            memcpy(&buf[0], &st, sizeof(struct stat));

            std::size_t hash = std::hash<std::string>{}(buf);

            if(CheckIfCached(m, path) == 0){
                std::cout << m[path] << "\n";
                m[path] = hash;
            }
             
            reply->set_hash(hash);
            reply->set_size(st.st_size);
            reply->set_exists(1);
            return Status::OK;
        }

        Status Read(ServerContext* context, const ReadReq* request, ServerWriter<ReadReply>* writer) override {
            std::string path = getTruePath(request->path());
            ReadReply* reply = new ReadReply();
            
            int fd = open(path.c_str(), O_RDONLY);
            if(fd == -1){
                reply->set_size(-1);
                return Status::OK;
            }

            std::string buf;
            buf.resize(request->size());
            int bytes = pread(fd, &buf[0], request->size(), request->offset());
            close(fd);

            if(bytes == -1){
                reply->set_size(-1);
            }

            // Maximum rcp payload size is 4 mb, I'm gonna send 2mb packets
            int unsent = bytes;
            int pkt_size = 2 * 1024 * 1024;
            int curr = 0;

            while(unsent > 0){
                int curr_size = std::min(pkt_size, unsent);
                reply->set_buf(buf.substr(curr, curr_size));
                reply->set_size(curr_size);
                curr += pkt_size;
                unsent -= pkt_size;
                writer->Write(*reply);
            }
            return Status::OK;
        }

        Status Write(ServerContext* context, ServerReader<WriteReq>* reader, WriteReply* reply) override {
            std::string path;
            WriteReq request;
            int bytes_written = 0;
            std::fstream file;
            //std::cout << "NOTICE: Write request\n";

            
            while(reader->Read(&request)){
                //std::cout << "NOTICE: Entered loop, bytes written is "<<bytes_written<<"\n";
                path = getTruePath(request.path());
                if(bytes_written == 0){
                    file.open(path.c_str(), std::ios::out);
                    std::cout << "Server received write request for file path " << path <<"\n";
                }
                if(!file){
                    reply->set_size(-1);
                    std::cout << "NOTICE: Err 1\n";
                    return Status::OK;
                }
                std::string buf = request.buf();
                //std::cout << "Buf contains " << buf << "\n";
                file << buf;
                bytes_written += request.size();
            }
            file.flush();
            file.close();
            reply->set_size(bytes_written);

            // Update server cache
            struct stat st;
            int res = stat(path.c_str(), &st);
            std::string buf;
            buf.resize(sizeof(struct stat));
            memcpy(&buf[0], &st, sizeof(struct stat));

            std::size_t hash = std::hash<std::string>{}(buf);
            m[path] = hash;
            reply->set_new_hash(m[path]);
            
            //std::cout << "NOTICE: Write request complete\n";
            return Status::OK;

        }

        Status Rename(ServerContext* context, const RenameReq* request, RenameReply* reply) override {
            std::string old_path = getTruePath(request->old_path());
            std::string new_path = getTruePath(request->new_path());
            int res = rename(old_path.c_str(), new_path.c_str());
            if(res == -1){
                reply->set_err(-errno);
                return Status::OK;
            }

            // Update server cache
            struct stat st;
            res = stat(new_path.c_str(), &st);
            if(res == -1){
                reply->set_err(-errno);
                return Status::OK;
            }
            std::string buf;
            buf.resize(sizeof(struct stat));
            memcpy(&buf[0], &st, sizeof(struct stat));

            std::size_t hash = std::hash<std::string>{}(buf);
            m.erase(old_path);
            m[new_path] = hash;

            reply->set_err(res);
            return Status::OK;
        }

        Status Create(ServerContext* context, const CreateReq* request, CreateReply* reply) override {
            std::string path = getTruePath(request->path());
            std::cout << "--- Creating file " << path << "---\n";
            int res = mknod(path.c_str(), request->mode(), request->dev());
            if(res == -1){
                std::cout << "\tFailed to create file " << path << "\n";
                reply->set_err(-errno);
            }else{
                reply->set_err(res);
                struct stat st;
                res = stat(path.c_str(), &st);
                if(res == -1){
                    reply->set_err(-errno);
                    return Status::OK;
                }
                std::string buf;
                buf.resize(sizeof(struct stat));
                memcpy(&buf[0], &st, sizeof(struct stat));

                std::size_t hash = std::hash<std::string>{}(buf);
                reply->set_new_hash(hash);
            }
            return Status::OK;
        }

        Status Unlink(ServerContext* context, const UnlinkReq* request, UnlinkReply* reply) override {
            std::string path = getTruePath(request->path());
            std::cout << "--- Unlinking file " << path << "---\n";
            int res = unlink(path.c_str());
            if(res == -1){
                std::cout << "\tFailed to unlink file " << path << "\n";
                reply->set_err(-errno);
            }else{
                reply->set_err(res);
            }
            m.erase(path);
            return Status::OK;
        }

};

CacheManager* UWAFS_Server::cache = NULL;


void RunServer() {
    std::string server_address("0.0.0.0:50051");
    UWAFS_Server service;

    ServerBuilder builder;
    
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();

    return 0;
}