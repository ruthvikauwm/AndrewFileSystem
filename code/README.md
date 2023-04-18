# uwAFS

Please note that if our slides are missing, it is because we were granted a slight grace period, allowed to demo on Tuesday. We will dump our slides here once we complete them, we had to make some changes to our implementation.

### Building

Prerequisites:

- CentOS: `dnf install -y gcc -y cmake fuse fuse-devel`
- Ubuntu: `apt-get install -y gcc cmake fuse libfuse-dev`
- FreeBSD: `pkg install gcc cmake fusefs-libs pkgconf`
- OpenBSD: `pkg_add cmake`
- macOS: `brew install --cask osxfuse`

```sh
$ cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build --parallel
```

### Packages

[![Packaging status](https://repology.org/badge/vertical-allrepos/fusefs:unreliablefs.svg)](https://repology.org/project/fusefs:unreliablefs/versions)

### Using

**Server**
```sh
$ cd code
$ mkdir /tmp/serverfs
$ ./build/uwAFS_server/server
```

Note: client appears to only work in debug mode. Also, make sure to put your server's hostname and ip address in the `server.config` file.

**Client**
```sh
$ cd code
$ mkdir /tmp/fs
$ mkdir /tmp/base
$ ./build/uwAFS_client/uwafs /tmp/fs -basedir=/tmp/base -seed=1618680646
```

You can run the client in debug mode, which is rather helpful, by adding the `-d` flag.


