import os

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
        context.Result(result[0])
        return result[0]

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
        context.Result(result[0])
        return result[0]


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
        context.Result(result[0])
        return result[0]

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

    def CheckFile(self, filename):
        return os.path.exists(filename)

class APRConfigure(APRConfigureBase):
    def __init__(self, env):
        APRConfigureBase.__init__(self, env)
