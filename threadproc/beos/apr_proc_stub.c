#include <kernel/OS.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

struct pipefd {
	int in;
	int out;
	int err;
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
	char ** newargs;
	int i = 0;
	
	newargs = (char**)malloc(sizeof(char*) * (argc - 1));
  
	buffer = (void*)malloc(sizeof(struct pipefd));
	/* this will block until we get the data */
	receive_data(&sender, buffer, sizeof(struct pipefd));
	pfd = (struct pipefd*)buffer;
	
	if (pfd->in > STDERR_FILENO) {
		if (dup2(pfd->in, STDIN_FILENO) != STDIN_FILENO) return (-1);
	    close (pfd->in);
	}
	if (pfd->out > STDERR_FILENO) {
		if (dup2(pfd->out, STDOUT_FILENO) != STDOUT_FILENO) return (-1);
	    close (pfd->out);
	}
	if (pfd->err > STDERR_FILENO) {
		if (dup2(pfd->err, STDERR_FILENO) != STDERR_FILENO) return (-1);
	    close (pfd->err);
	}

	for	(i=3;i<=argc;i++){
	    newargs[i-3] = argv[i];
	}
	
	/* tell the caller we're OK to start */
	send_data(sender,1,NULL,0);

	if (directory != NULL)
		chdir(directory);
	execve (progname, newargs, environ);

	return (-1);
}