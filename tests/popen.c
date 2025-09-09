/*
 * This example shows how to use libcortex to asynchronously execute another process,
 * write to its stdin and read from its stdout.
 *
 * Mario Kicherer (dev@kicherer.org) 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>

#include "core.h"
#include "popen.h"

#define TEXT "First text output"
char buffer[] = TEXT;
size_t to_write = sizeof(TEXT);
size_t to_write2 = sizeof(TEXT);
int repeat = 1;
struct crtx_popen_listener plist;
struct crtx_popen_listener plist2;

static int popen_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_FORK_ET_CHILD_STOPPED || event->type == CRTX_FORK_ET_CHILD_KILLED) {
		printf("PARENT: child (%zu) done with rc %d\n", (size_t) userdata, event->data.int32);
		
		if (userdata == 0) {
			printf("PARENT: will shutdown\n");
			crtx_init_shutdown();
		} else {
			if (repeat == 1) {
				int ret;
				
				printf("PARENT: starting child1 again\n");
				
				ret = crtx_start_listener(&plist2.base);
				if (ret) {
					CRTX_ERROR("starting second popen listener failed\n");
					return 0;
				}
				
				repeat = 0;
			}
		}
	} else {
		printf("PARENT: unexpected event\n");
	}
	
	return 0;
}

static void stdout_cb(int fd, void *userdata) {
	ssize_t r;
	char *s, *newline;
	char buf[4096];
	
	// printf("PARENT: reading from CHILD%d fd %d\n", (int)(ptrdiff_t)userdata, fd);
	
	while (1) {
		r = read(fd, buf, sizeof(buf)-1);
		if (r < 0) {
			if (r != -EAGAIN && r != -EPERM)
				CRTX_ERROR("read from signal selfpipe %d failed: %s\n", fd, strerror(-r));
			break;
		}
		buf[r] = 0;
		
		// prefix every line in output
		s=buf;
		while (1) {
			newline = strchr(s, '\n');
			if (!newline) {
				printf("CHILD%d: %s\n", (int)(ptrdiff_t)userdata, s);
				break;
			}
			
			printf("CHILD%d: %.*s\n", (int)(ptrdiff_t)userdata, (int) (newline - s), s);
			s = newline + 1;
			if (*s == 0)
				break;
		}
	}
	
	return;
}

static int wq_write(struct crtx_writequeue_listener *wqueue, void *userdata) {
	ssize_t n_bytes;
	size_t *buflen;
	
	if (userdata == 0) {
		buflen = &to_write;
	} else {
		buflen = &to_write2;
	}
	
	while (1) {
		printf("PARENT: writing \"%.*s\" to child %d via fd %d\n", (int) *buflen, buffer, (int)(ptrdiff_t)userdata, wqueue->write_fd);
		n_bytes = write(wqueue->write_fd, buffer, *buflen);
		if (n_bytes < 0) {
			if (errno == EAGAIN || errno == ENOBUFS) {
				printf("received EAGAIN\n");
				
				crtx_start_listener(&wqueue->base);
				
				return EAGAIN;
			} else {
				printf("error %s\n", strerror(errno));
				return -errno;
			}
		} else {
			*buflen -= n_bytes;
			if (*buflen == 0) {
				if (userdata == 0) {
					crtx_popen_close_stdin(&plist);
				} else {
					crtx_popen_close_stdin(&plist2);
				}
				break;
			}
		}
	}
	
	return 0;
}

void pre_exec(struct crtx_popen_listener *plstnr, void *pre_exec_userdata) {
	char *s;
	int rv;
	
	printf("still open FDs before fork:\n");
	rv = asprintf(&s, "ls -lsh /proc/%d/fd/", getpid());
	if (rv < 0)
		fprintf(stderr, "asprintf failed\n");
	rv = system(s);
	if (rv < 0)
		fprintf(stderr, "system() failed\n");
	free(s);
}

int main(int argc, char **argv) {
	int ret;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	crtx_popen_clear_lstnr(&plist);
	crtx_popen_clear_lstnr(&plist2);
	
	plist.fstderr = STDERR_FILENO;
	plist2.fstderr = STDERR_FILENO;
	
	// setup process 1
	
	plist.filepath = "/bin/sh";
	plist.argv = (char*[]) {
		(char*) plist.filepath,
		"-c",
		"id; echo \"waiting...\"; cat; sleep 5; echo \"final child0 output\"; exit 1;",
		0
	};
	
	if (getuid() == 0) {
		plist.fork_lstnr.user = "nobody";
		plist.fork_lstnr.group = "nogroup";
	}
	
	plist.stdin_wq_lstnr.write = &wq_write;
	plist.stdin_wq_lstnr.write_userdata = (void*) 0;
	
	plist.stdout_lstnr.fd_flags = O_NONBLOCK;
	
	plist.stdout_cb = &stdout_cb;
	plist2.stdout_cb_data = (void*) 0;
	
	// plist.pre_exec = &pre_exec;
	
	ret = crtx_setup_listener("popen", &plist);
	if (ret) {
		CRTX_ERROR("create_listener(popen) failed\n");
		return 0;
	}
	
	// printf("PARENT: child 0 FD in: %d out: %d\n", plist.stdin_lstnr.fds[CRTX_WRITE_END], plist.stdout_lstnr.fds[CRTX_READ_END]);
	
	crtx_create_task(plist.base.graph, 0, "popen_event", &popen_event_handler, 0);
	
	ret = crtx_start_listener(&plist.base);
	if (ret) {
		CRTX_ERROR("starting popen listener failed\n");
		return 0;
	}
	
	if (1) {
		// setup process 2
		
		plist2.filepath = "/bin/sh";
		plist2.argv = (char*[]) { (char*) plist.filepath, "-c", "cat; sleep 1; echo \"final child1 output\"; exit 0;", 0};
		
		plist2.stdout_cb = &stdout_cb;
		plist2.stdout_cb_data = (void*) 1;
		
		plist2.stdout_lstnr.fd_flags = O_NONBLOCK;
		
		plist2.stdin_wq_lstnr.write = &wq_write;
		plist2.stdin_wq_lstnr.write_userdata = (void*) 1;
		
		ret = crtx_setup_listener("popen", &plist2);
		if (ret) {
			CRTX_ERROR("create_listener(popen) failed\n");
			return 0;
		}
		
		// printf("PARENT: child 1 FD in: %d out: %d\n", plist2.stdin_lstnr.fds[CRTX_WRITE_END], plist2.stdout_lstnr.fds[CRTX_READ_END]);
		
		crtx_create_task(plist2.base.graph, 0, "popen_event", &popen_event_handler, (void*) 1);
		
		ret = crtx_start_listener(&plist2.base);
		if (ret) {
			CRTX_ERROR("starting second popen listener failed\n");
			return 0;
		}
	}
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}
