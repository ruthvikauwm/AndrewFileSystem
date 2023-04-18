#include "uwAFS.h"
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <dirent.h>
#define PATH_MAX 4096

UWAFSClient* UWAFSClient::_instance = NULL;
CacheManager* UWAFSClient::cache = NULL;
std::map<std::string, std::size_t> m;

UWAFSClient* UWAFSClient::Instance() {
    //std::cout << "NOTICE:: performing Instance()\n";
	if(_instance == NULL) {
        //std::cout << "Notice:: _instance is NULL\n";
        FILE* fp = fopen("./server.config", "r");
        if(fp != NULL){
            char host_info[200];
            fscanf(fp, "%s", host_info);
            //std::cout << "Connecting to " << host_info << "\n" << std::flush;
            _instance = new UWAFSClient(grpc::CreateChannel(host_info, grpc::InsecureChannelCredentials()));
        }
	}

    if(cache == NULL){
        cache = new CacheManager("/tmp/base"); // TODO: get rid of /tmp/base in params
    }
	return _instance;
}

UWAFSClient::UWAFSClient() {

}

UWAFSClient::~UWAFSClient() {

}



int UWAFSClient::GetAttr(const std::string& path, struct stat *buf) {
    GetHashReq request;
    request.set_path(path);

    GetHashReply reply;
    ClientContext context;
    Status st = stub_->GetHash(&context, request, &reply);
    std::size_t server_hash;

    if(cache->CheckExists(path) == 0){
        //std::cout << "NOTICE:: file " << path << " exists locally\n";
        // Check attr on server
        if(st.ok()){
            server_hash = reply.hash();
            // std::cout << "\n\nLocal hash: "<< path << " : " << m[path];
            // std::cout << "\nServer hash: " << server_hash << "\n\n";
            if(server_hash == 0){
                //std::cout << "NOTICE:: Server has no record of this file.\n";
                return -2;
            }
            if(CheckIfCached(m, path) && m[path] == server_hash){
               // Local is same or newer than server's
               stat(path.c_str(), buf);
               return 0;
            }
            // stat(path.c_str(), buf);
            //    return 0;
        }

    }
    GetAttrReq req;
    req.set_path(path);

    GetAttrReply rep;
    ClientContext context2;
    //std::cout << "NOTICE:: sending getattr request to server\n";
    Status status = stub_->GetAttr(&context2, req, &rep);

    if (status.ok()) {
        //std::cout << "NOTICE:: received a response from server\n";
        Attr attributes = rep.attr();
        buf->st_dev = attributes.dev();
	    buf->st_ino = attributes.ino();
	    buf->st_mode = attributes.mode();
	    buf->st_nlink = attributes.nlink();
	    Owner owner = attributes.owner();
	    buf->st_uid = owner.uid();
        buf->st_gid = owner.gid();
        buf->st_rdev = attributes.rdev();
	    buf->st_size = attributes.size();
	    buf->st_blksize = attributes.blksize();
	    buf->st_blocks = attributes.blocks();
	    buf->st_atime = attributes.atime();
	    buf->st_mtime = attributes.mtime();
	    buf->st_ctime = attributes.ctime();
        return rep.err();
    } else {
        //std::cout << "NOTICE:: did not receive a response from server\n";
        return -1;
    }
}

int UWAFSClient::ReadDir(const char *path, void *buf, fuse_fill_dir_t filler){
    std::string path_str(path);
    ReadDirReq request;
    request.set_path(path_str);

    ClientContext context;
    ReadDirReply reply;
    //std::cout << "NOTICE:: sending ReadDir request to server\n";
    grpc::Status status = stub_->ReadDir( &context, request, &reply );

    // gRPC lists: https://stackoverflow.com/questions/70520658/grpc-use-of-lists-in-protobuf
    if (status.ok()) {
        //std::cout << "NOTICE:: received a response from server for ReadDir\n";
        for(int i = 0; i < reply.dirents_size(); i++){
            struct stat st;
            memset(&st, 0, sizeof(struct stat));
            st.st_ino = reply.dirents(i).inodenum();
            st.st_mode = reply.dirents(i).mode() << 12;
            if(filler(buf, reply.dirents(i).name().c_str(), &st, 0) != 0){
                break;
            }
        }
        //std::cout << "NOTICE:: finished processing readdir request\n";
        return 0;
        
    } else {
        //std::cout << "NOTICE:: did not receive a response from server for ReadDir\n";
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        throw status.error_message();
    }
    return -1;
}

int UWAFSClient::MkDir(const char *path,mode_t mode){
    mkdir(path, mode);
    std::string path_str(path);
    MkDirReq request;
    request.set_path(path);
    request.set_mode(mode);
    ClientContext context;
    MkDirReply reply;
    //std::cout << "NOTICE:: sending AfsMkDir request to server\n";
    Status status = stub_->MkDir(&context, request, &reply);

    return status.ok()?reply.err():-1;

}

int UWAFSClient::RmDir(const char *path){
    rmdir(path);
    std::string path_str(path);
    RmDirReq request;
    request.set_path(path);
    ClientContext context;
    RmDirReply reply;
    //std::cout << "NOTICE:: sending AfsRmDir request to server\n";
    Status status = stub_->RmDir(&context, request, &reply);

    return status.ok()?reply.err():-1;
}

int UWAFSClient::Open(const char* path, struct fuse_file_info *fi){
    // Check if file exists in server
    GetHashReq request;
    request.set_path(path);

    GetHashReply reply;
    ClientContext context;
    Status st = stub_->GetHash(&context, request, &reply);
    std::size_t server_hash;

    if(st.ok()){
        server_hash = reply.hash();
        if(server_hash == 0){
            //std::cout << "NOTICE:: Server has no record of this file.\n";
            return -2;
        }
        // std::cout << "\n\nLocal hash: "<< path << " : " << cache->GetStatHash(path);
        // std::cout << "\nServer hash: " << server_hash << "\n\n";

        // If file exists locally
        if(cache->CheckExists(path) == 0){
            //std::cout << "NOTICE:: file " << path << " exists\n";
            // If file is cached and not stale
            if(CheckIfCached(m, path) && m[path] == server_hash){
                //std::cout << "Local open of "<<path<<"\n";
                fi->fh = open(path, O_RDWR);
                return 0;
            }
        }
    }
    
    // File is either stale or doesn't exist locally
    //std::cout << "--- Opening server file, file is " << reply.size() << " bytes ---\n";

    std::string buf;
    buf.reserve(reply.size());

    ReadReq req;
    req.set_path(path);
    req.set_offset(0);
    req.set_size(reply.size());

    ReadReply rep;
    ClientContext context2;

    std::unique_ptr<ClientReader<ReadReply> > reader(stub_->Read(&context2, req));
    
    while(reader->Read(&rep)){
        
        buf += rep.buf();
        if(rep.size() < 0){
            break;
        }
    }
    
    Status status = reader->Finish();

    // std::cout << "buf says: " << buf << "\n";
    cache->LocalWrite(path, buf, fi->fh, 1);
    m[path] = server_hash;
    std::cout << "--- Adding " << path << " to cache with hash "<<m[path]<<"---\n";

    return 0;
}

int UWAFSClient::Read(int fd, char* buf, size_t size, size_t offset){
    return cache->LocalRead(fd, buf, size, offset);
}

int UWAFSClient::Close(const char *path, int fd){
    //std::cout << "\n\nNOTICE:: CLOSE\n";
    // Ensure file exists on server
    GetHashReq request;
    request.set_path(path);

    GetHashReply reply;
    ClientContext context;
    Status st = stub_->GetHash(&context, request, &reply);
    std::size_t server_hash;

    if(st.ok()){
        server_hash = reply.hash();
        if(server_hash == 0){
            //std::cout << "NOTICE:: Server has no record of this file.\n";
            return -2;
        }
        // Ensure file exists locally and fd is valid
        if(fd != -1){
            if(cache->CheckExists(path) == 1){
                // Idk how this could happen
                std::cout << "--- In close, somehow file "<<path<<" not in cache, " << m[path]<<" ---\n";
            }
            if(access(("/tmp/base" + std::string(path)).c_str(), F_OK) == 0){
                std::cout << "--- File has not been modified, no need to write ---\n";
                return 0;
            }
            
            // if(cache->LocalTotalRead(path, buf) != 0){
            //     std::cout << "ERR !\n\n";
            //     return -1;
            // }

            WriteReq request;
            WriteReply reply;

            ClientContext context;
            std::unique_ptr<ClientWriter<WriteReq> > writer(stub_->Write(&context, &reply));

            struct stat stat_buf;
            int rc = stat(path, &stat_buf);
            if(rc != 0){
                std::cout << "--- Couldn't stat the file ---\n";
                return -1;
            }

            // Maximum rcp payload size is 4 mb, I'm gonna send 2mb packets
            int unsent = stat_buf.st_size;
            int pkt_size = 2 * 1024 * 1024;
            int curr = 0;
            

            //std::cout << "--- Sending file contents to server, "<< unsent << " bytes ---\n";
            char* buf = (char*)std::malloc(pkt_size);
            while(unsent >= 0){
                int curr_size = std::min(pkt_size, unsent);
                request.set_path(path);
                request.set_size(curr_size);
                request.set_offset(curr);
                std::cout << "--- buf size is "<<unsent<<" ---\n";
                cache->LocalRead(fd, buf, curr_size, curr);
                request.set_buf(buf);
                curr += pkt_size;
                unsent -= pkt_size;
                writer->Write(request);
            }
            free(buf);
            writer->WritesDone();
            Status status = writer->Finish();
            cache->RemoveMod(path);
            m[path] = reply.new_hash();

            std::cout << "--- Server received " << reply.size() << " bytes, new hash is" << reply.new_hash() << " ---\n";

            return close(fd);
        }
    }
    return -1;
}

int UWAFSClient::Rename(char* old_path, char* new_path){
    Rename(m, old_path, new_path);
    RenameReq request;
    request.set_old_path(old_path);
    request.set_new_path(new_path);

    RenameReply reply;
    ClientContext context;
    Status st = stub_->Rename(&context, request, &reply);
    return 0;
}

int UWAFSClient::Flush(const char* path, struct fuse_file_info *fi){
    if(fi->fh != -1){
        //std::cout << "NOTICE:: Flushing " << path << "\n";
        int ret = fsync(fi->fh);
        close(dup(fi->fh));
        cache->AddMod(path);
        return 0;
    }
    return -1;
}

int UWAFSClient::Create(const char* path, mode_t mode, dev_t dev){
    std::cout << "--- CREATING FILE " << path << "---\n";
    CreateReq request;
    request.set_path(path);
    request.set_mode(mode);
    request.set_dev(dev);

    CreateReply reply;
    ClientContext context;
    Status st = stub_->Create(&context, request, &reply);
    if(reply.err() != -1){
         m[path] = reply.new_hash();
    }
   
    mknod(path, mode, dev);
    return reply.err();
}

int UWAFSClient::Unlink(const char* path){
    UnlinkReq request;
    request.set_path(path);

    UnlinkReply reply;
    ClientContext context;
    Status st = stub_->Unlink(&context, request, &reply);
    remove(path);
    m.erase(path);
    return reply.err();
}