import os
import sys

class APRConfigureBase:
    def __init__(self, env):
        self.env = env

    def Check_apr_big_endian(self, context):
        import struct
        context.Message("Checking for big endianess... ")
        array = struct.pack('cccc', '\x01', '\x02', '\x03', '\x04')
        i = struct.unpack('i', array)
        if i == struct.unpack('>i', array):
            context.Result('yes')
            return 1
        else:
            context.Result('no')
            return 0

    def CheckFile(self, context, path):
        context.Message("Checking if %s exists... " % (path))
        if os.path.exists(path):
            context.Result('yes')
            return 1
        else:
            context.Result('no')
            return 0

    def CheckTypesCompatible(self, context, t1, t2, includes):
        context.Message('Checking %s is the same as %s... ' % (t1, t2))
        source = """
    %s
void main(void)
{
    int foo[0 - !__builtin_types_compatible_p(%s, %s)];
}
    """  % (includes, t1, t2)
        result = context.TryCompile(source, '.c')
        self.env.Filter(CPPFLAGS = ['-D_LARGEFILE64_SOURCE'])
        context.Result(result)
        return result

    def Check_apr_atomic_builtins(self, context):
        context.Message('Checking whether the compiler provides atomic builtins... ')
        source = """
int main()
{
    unsigned long val = 1010, tmp, *mem = &val;

    if (__sync_fetch_and_add(&val, 1010) != 1010 || val != 2020)
        return 1;

    tmp = val;

    if (__sync_fetch_and_sub(mem, 1010) != tmp || val != 1010)
        return 1;

    if (__sync_sub_and_fetch(&val, 1010) != 0 || val != 0)
        return 1;

    tmp = 3030;

    if (__sync_val_compare_and_swap(mem, 0, tmp) != 0 || val != tmp)
        return 1;

    if (__sync_lock_test_and_set(&val, 4040) != 3030)
        return 1;

    mem = &tmp;

    if (__sync_val_compare_and_swap(&mem, &tmp, &val) != &tmp)
        return 1;

    __sync_synchronize();

    if (mem != &val)
        return 1;

    return 0;
}
    """
        result = context.TryRun(source, '.c')
        context.Result(result[0] == 1)
        return result[0] == 1

    def Check_apr_ebcdic(self, context):
        context.Message('Checking whether system uses EBCDIC.. ')
        source = """
int main(void) { 
  return (unsigned char)'A' != (unsigned char)0xC1; 
}"""
        result = context.TryRun(source, '.c')
        context.Result(result[0] == 1)
        return result[0] == 1
        
    def Check_apr_nonblock_inherited(self, context):
        context.Message('Checking whether O_NONBLOCK setting is inherited from listening sockets... ')
        source = """
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
int main(void) {
    int listen_s, connected_s, client_s;
    int listen_port, rc;
    struct sockaddr_in sa;
    socklen_t sa_len;

    listen_s = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_s < 0) {
        perror("socket");
        exit(1);
    }
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
#ifdef BEOS
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif
    /* leave port 0 to get ephemeral */
    rc = bind(listen_s, (struct sockaddr *)&sa, sizeof sa);
    if (rc < 0) {
        perror("bind for ephemeral port");
        exit(1);
    }
    /* find ephemeral port */
    sa_len = sizeof(sa);
    rc = getsockname(listen_s, (struct sockaddr *)&sa, &sa_len);
    if (rc < 0) {
        perror("getsockname");
        exit(1);
    }
    listen_port = sa.sin_port;
    rc = listen(listen_s, 5);
    if (rc < 0) {
        perror("listen");
        exit(1);
    }
    rc = fcntl(listen_s, F_SETFL, O_NONBLOCK);
    if (rc < 0) {
        perror("fcntl(F_SETFL)");
        exit(1);
    }
    client_s = socket(AF_INET, SOCK_STREAM, 0);
    if (client_s < 0) {
        perror("socket");
        exit(1);
    }
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port   = listen_port;
#ifdef BEOS
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif
    /* leave sin_addr all zeros to use loopback */
    rc = connect(client_s, (struct sockaddr *)&sa, sizeof sa);
    if (rc < 0) {
        perror("connect");
        exit(1);
    }
    sa_len = sizeof sa;
    connected_s = accept(listen_s, (struct sockaddr *)&sa, &sa_len);
    if (connected_s < 0) {
        perror("accept");
        exit(1);
    }
    rc = fcntl(connected_s, F_GETFL, 0);
    if (rc < 0) {
        perror("fcntl(F_GETFL)");
        exit(1);
    }
    if (!(rc & O_NONBLOCK)) {
        fprintf(stderr, "O_NONBLOCK is not set in the child.\n");
        exit(1);
    }
    return 0;
}"""
        result = context.TryRun(source, '.c')
        context.Result(result[0] == 1)
        return result[0] == 1

    def Check_apr_largefile64(self, context):
        context.Message('Checking whether to enable -D_LARGEFILE64_SOURCE... ')
        self.env.AppendUnique(CPPFLAGS = ['-D_LARGEFILE64_SOURCE'])
        source = """
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void main(void)
{
    int fd, ret = 0;
    struct stat64 st;
    off64_t off = 4242;

    if (sizeof(off64_t) != 8 || sizeof(off_t) != 4)
       exit(1);
    if ((fd = open("conftest.lfs", O_LARGEFILE|O_CREAT|O_WRONLY, 0644)) < 0)
       exit(2);
    if (ftruncate64(fd, off) != 0)
       ret = 3;
    else if (fstat64(fd, &st) != 0 || st.st_size != off)
       ret = 4;
    else if (lseek64(fd, off, SEEK_SET) != off)
       ret = 5;
    else if (close(fd) != 0)
       ret = 6;
    else if (lstat64("conftest.lfs", &st) != 0 || st.st_size != off)
       ret = 7;
    else if (stat64("conftest.lfs", &st) != 0 || st.st_size != off)
       ret = 8;
    unlink("conftest.lfs");

    exit(ret);
}"""
        result = context.TryRun(source, '.c')
        self.env.Filter(CPPFLAGS = ['-D_LARGEFILE64_SOURCE'])
        context.Result(result[0] == 1)
        return result[0] == 1


    def Check_apr_mmap_mapping_dev_zero(self, context):
        context.Message('Checking for mmap that can map /dev/zero... ')
        source = """
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
    int main()
    {
        int fd;
        void *m;
        fd = open("/dev/zero", O_RDWR);
        if (fd < 0) {
            return 1;
        }
        m = mmap(0, sizeof(void*), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (m == (void *)-1) {  /* aka MAP_FAILED */
            return 2;
        }
        if (munmap(m, sizeof(void*)) < 0) {
            return 3;
        }
        return 0;
    }
        """
        result = context.TryRun(source, '.c')
        context.Result(result[0] == 1)
        return result[0] == 1

    def Check_apr_semaphores(self, context):
        context.Message('Checking for sem_open, sem_close, sem_unlink... ')
        source = """
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#ifndef SEM_FAILED
#define SEM_FAILED (-1)
#endif
main()
{
    sem_t *psem;
    const char *sem_name = "/apr_autoconf";

    psem = sem_open(sem_name, O_CREAT, 0644, 1);
    if (psem == (sem_t *)SEM_FAILED) {
	exit(1);
    }
    sem_close(psem);
    psem = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
    if (psem != (sem_t *)SEM_FAILED) {
        sem_close(psem);
        exit(1);
    }
    sem_unlink(sem_name);
    exit(0);
}
        """
        result = context.TryCompile(source, '.c')
        context.Result(result)
        return result

    def Check_apr_check_tcp_nodelay_inherited(self, context):
        context.Message('Checking for tcp nodelay inherited... ')
        source = """
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
int main(void) {
    int listen_s, connected_s, client_s;
    int listen_port, rc;
    struct sockaddr_in sa;
    socklen_t sa_len;
    socklen_t option_len;
    int option;

    listen_s = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_s < 0) {
        perror("socket");
        exit(1);
    }
    option = 1;
    rc = setsockopt(listen_s, IPPROTO_TCP, TCP_NODELAY, &option, sizeof option);
    if (rc < 0) {
        perror("setsockopt TCP_NODELAY");
        exit(1);
    }
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
#ifdef BEOS
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif
    /* leave port 0 to get ephemeral */
    rc = bind(listen_s, (struct sockaddr *)&sa, sizeof sa);
    if (rc < 0) {
        perror("bind for ephemeral port");
        exit(1);
    }
    /* find ephemeral port */
    sa_len = sizeof(sa);
    rc = getsockname(listen_s, (struct sockaddr *)&sa, &sa_len);
    if (rc < 0) {
        perror("getsockname");
        exit(1);
    }
    listen_port = sa.sin_port;
    rc = listen(listen_s, 5);
    if (rc < 0) {
        perror("listen");
        exit(1);
    }
    client_s = socket(AF_INET, SOCK_STREAM, 0);
    if (client_s < 0) {
        perror("socket");
        exit(1);
    }
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port   = listen_port;
#ifdef BEOS
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif
    /* leave sin_addr all zeros to use loopback */
    rc = connect(client_s, (struct sockaddr *)&sa, sizeof sa);
    if (rc < 0) {
        perror("connect");
        exit(1);
    }
    sa_len = sizeof sa;
    connected_s = accept(listen_s, (struct sockaddr *)&sa, &sa_len);
    if (connected_s < 0) {
        perror("accept");
        exit(1);
    }
    option_len = sizeof option;
    rc = getsockopt(connected_s, IPPROTO_TCP, TCP_NODELAY, &option, &option_len);
    if (rc < 0) {
        perror("getsockopt");
        exit(1);
    }
    if (!option) {
        fprintf(stderr, "TCP_NODELAY is not set in the child.\n");
        exit(1);
    }
    return 0;
} """ 
        result = context.TryRun(source, '.c') 
        context.Result(result[0] == 1)
        return result[0] == 1

    def Check_apr_semun(self, context):
        context.Message('Checking for semun... ')
        source = """
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
main()
{
    union semun arg;
    semctl(0, 0, 0, arg);
    exit(0);
}
        """
        result = context.TryCompile(source, '.c')
        context.Result(result)
        return result

    def Check_apr_sctp(self, context):
        context.Message('Checking for sctp support... ')
        source = """
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>
#include <stdlib.h>
int main(void) {
    int s, opt = 1;
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
       exit(1);
    if (setsockopt(s, IPPROTO_SCTP, SCTP_NODELAY, &opt, sizeof(int)) < 0)
       exit(2);
    exit(0);
}
"""
        result = context.TryRun(source, '.c') 
        context.Result(result[0] == 1)
        return result[0] == 1

class APRConfigure(APRConfigureBase):
    def __init__(self, env):
        APRConfigureBase.__init__(self, env)
