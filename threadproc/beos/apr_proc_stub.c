#include <kernel/OS.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

struct pipefd {
	int in;
	int out;
	int err;
	char ** envp;
};

int main(int argc, char *argv[]) {
/* we expect the following...
 * 
 * argv[0] = this stub
 * argv[1] = directory to run in...
 * argv[2] = progname to execute
 * rest of arguments to be passed to program
 */
	char *progname = argv[2];
	char *directory = argv[1];
	struct pipefd *pfd;
	thread_id sender;
	void *buffer;
	int indes[2];
	int outdes[2];
	int errdes[2];
	char ** newargs;
	int i = 0;
	char * readbuffer;
	size_t readbuf = 100;
	
	readbuffer = (char*)malloc(sizeof(char) * readbuf);
	newargs = (char**)malloc(sizeof(char*) * (argc - 1));
  
	buffer = (void*)malloc(sizeof(struct pipefd));
	/* this will block until we get the data */
	receive_data(&sender, buffer, sizeof(struct pipefd));
	pfd = (struct pipefd*)buffer;
	
	if (pfd->in > STDERR_FILENO) {
		if (pipe(indes) == -1)return (-1);
		if (dup2(pfd->in, indes[0]) != indes[0]) return (-1);
		if (dup2(indes[0], STDIN_FILENO) != STDIN_FILENO) return (-1);
	}
	if (pfd->out > STDERR_FILENO) {
		if (pipe(outdes) == -1)return (-1);
		if (dup2(pfd->out, outdes[1]) != outdes[1]) return (-1);
		if (dup2(outdes[1], STDOUT_FILENO) != STDOUT_FILENO) return (-1);
	}
	if (pfd->err > STDERR_FILENO) {
		if (pipe(errdes) == -1)return (-1);
		if (dup2(pfd->err, errdes[1]) != errdes[1]) return (-1);
		if (dup2(errdes[1], STDERR_FILENO) != STDERR_FILENO) return (-1);
	}

	for	(i=3;i<=argc;i++){
	    newargs[i-3] = argv[i];
	}
	
	/* tell the caller we're OK to start */
	send_data(sender,1,NULL,0);

	if (directory != NULL)
		chdir(directory);
	execve (progname, newargs, NULL);

	return (-1);
}