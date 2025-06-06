/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "intern.h"
#include "core.h"
#include "popen.h"
#include "fork.h"

#ifndef CRTX_TEST

// this function is called by the child immediately after the fork
void post_fork_child_callback(void *cb_data) {
	struct crtx_popen_listener *plstnr;
	
	
	plstnr = (struct crtx_popen_listener *) cb_data;
	
	// In the child process, set the fstd* variables to their final values and
	// set the listener FDs the child will use to -1 as libcortex could shutdown
	// all the old listeners inherited from the parent process and would close
	// their FDs otherwise.
	
	if (plstnr->fstdin == -1) {
		if (plstnr->stdin_wq_lstnr.write)
			plstnr->fstdin = plstnr->stdin_lstnr.fds[CRTX_READ_END];
		else
			plstnr->fstdin = STDIN_FILENO;
	}
	
	if (plstnr->fstdout == -1) {
		if (plstnr->stdout_cb)
			plstnr->fstdout = plstnr->stdout_lstnr.fds[CRTX_WRITE_END];
		else
			plstnr->fstdout = STDOUT_FILENO;
	}
	
	if (plstnr->fstderr == -1) {
		if (plstnr->stderr_cb)
			plstnr->fstderr = plstnr->stderr_lstnr.fds[CRTX_WRITE_END];
		else
			plstnr->fstderr = STDERR_FILENO;
	}
	
	plstnr->stdin_lstnr.fds[CRTX_READ_END] = -1;
	plstnr->stdout_lstnr.fds[CRTX_WRITE_END] = -1;
	plstnr->stderr_lstnr.fds[CRTX_WRITE_END] = -1;
}

// this function is called by the child process after libcortex has reinitialized
// itself after the fork and it starts the actual executable/command
void after_fork_reinit_cb(void *reinit_cb_data) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) reinit_cb_data;
	
	if (plstnr->filepath)
		CRTX_DBG("exec \"%s\" (fds %d %d %d)\n", plstnr->filepath, plstnr->fstdin, plstnr->fstdout, plstnr->fstderr);
	else
		CRTX_DBG("exec \"%s\" (fds %d %d %d)\n", plstnr->filename, plstnr->fstdin, plstnr->fstdout, plstnr->fstderr);
	
	if (plstnr->argv) {
		char **arg;
		
		arg = plstnr->argv;
		while (arg && *arg) {
			CRTX_DBG("\targ: %s\n", *arg);
			arg++;
		}
	}
	
	if (plstnr->fstdin != STDIN_FILENO) {
		rv = dup2(plstnr->fstdin, STDIN_FILENO);
		if (rv == -1) {
			CRTX_ERROR("dup2(stdin, %d) failed: %s\n", plstnr->fstdin, strerror(errno));
			exit(1);
		}
		close(plstnr->fstdin);
	}
	
	if (plstnr->fstdout != STDOUT_FILENO) {
		rv = dup2(plstnr->fstdout, STDOUT_FILENO);
		if (rv == -1) {
			CRTX_ERROR("dup2(stdout, %d) failed: %s\n", plstnr->fstdout, strerror(errno));
			exit(1);
		}
		close(plstnr->fstdout);
	}
	
	if (plstnr->fstderr != STDERR_FILENO) {
		rv = dup2(plstnr->fstderr, STDERR_FILENO);
		if (rv == -1) {
			CRTX_ERROR("dup2(stderr, %d) failed: %s\n", plstnr->fstderr, strerror(errno));
			exit(1);
		}
		close(plstnr->fstderr);
	}
	
	if (plstnr->argv == 0) {
		if (plstnr->filepath) {
			plstnr->argv = (char*[]) { (char*) plstnr->filepath, 0 };
		} else {
			plstnr->argv = (char*[]) { (char*) plstnr->filename, 0 };
		}
	}
	
	/*
	if (plstnr->envp == 0) {
		// required for portability if an empty environment shall be created
		plstnr->envp = (char**) malloc(sizeof(char*)*1);
		plstnr->envp[0] = 0;
	}
	*/
	
	if (plstnr->chdir) {
		rv = chdir(plstnr->chdir);
		if (rv < 0) {
			CRTX_ERROR("chdir to \"%s\" failed: %s\n", plstnr->chdir, strerror(errno));
		}
	}
	
	if (plstnr->pre_exec)
		plstnr->pre_exec(plstnr, plstnr->pre_exec_userdata);
	
	if (plstnr->envp) {
// 		rv = execlp(plstnr->cmd, plstnr->cmd, (char*) 0);
		if (plstnr->filepath) {
			rv = execve(plstnr->filepath, plstnr->argv, plstnr->envp);
		} else {
			rv = execvpe(plstnr->filename, plstnr->argv, plstnr->envp);
		}
	} else {
		if (plstnr->filepath) {
			rv = execv(plstnr->filepath, plstnr->argv);
		} else {
			rv = execvp(plstnr->filename, plstnr->argv);
		}
	}
	if (rv) {
		if (plstnr->filepath) {
			CRTX_ERROR("exec*(%s) failed: %s\n", plstnr->filepath, strerror(errno));
		} else {
			CRTX_ERROR("exec*(%s) failed: %s\n", plstnr->filename, strerror(errno));
		}
		exit(1);
	}
	
	// this should not be reachable
	crtx_init_shutdown();
}

// callback that is executed if this listener is configured to react on new data from stdout or stderr
static int pipe_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_pipe_listener *pipe_lstnr;
	struct crtx_popen_listener *popen_lstnr;
	
	popen_lstnr = (struct crtx_popen_listener *) userdata;
	pipe_lstnr = (struct crtx_pipe_listener*) event->origin;
	
	if (pipe_lstnr == &popen_lstnr->stdout_lstnr) {
		if (popen_lstnr->stdout_cb)
			popen_lstnr->stdout_cb(popen_lstnr->stdout_lstnr.fds[0], popen_lstnr->stdout_cb_data);
	} else
	if (pipe_lstnr == &popen_lstnr->stderr_lstnr) {
		if (popen_lstnr->stderr_cb)
			popen_lstnr->stderr_cb(popen_lstnr->stderr_lstnr.fds[0], popen_lstnr->stderr_cb_data);
	} else {
		CRTX_ERROR("unexpected listener in popen's pipe_event_handler(): %p\n", pipe_lstnr);
	}
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) listener;
	
	// this listener has no event sources, it depends on other sub-listeners
	plstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
	if (plstnr->fstdin == -1 && plstnr->stdin_wq_lstnr.write) {
		// if we are started again, the write end of the pipe might have been closed to indicate
		// the child that there was no more input and we have to reopen the pipe and setup the
		// writequeue again
		
		// we call "setup" again and not "start" as we just want to open the pipe (again) and
		// not to add the read end to the event loop
		rv = crtx_setup_listener("pipe", &plstnr->stdin_lstnr);
		if (rv) {
			CRTX_ERROR("start_listener(pipe) failed\n");
			return 0;
		}
		
		if (plstnr->stdin_wq_lstnr.write_fd != plstnr->stdin_lstnr.fds[CRTX_WRITE_END]) {
			plstnr->stdin_wq_lstnr.write_fd = plstnr->stdin_lstnr.fds[CRTX_WRITE_END];
			
			rv = crtx_setup_listener("writequeue", &plstnr->stdin_wq_lstnr);
			if (rv) {
				CRTX_ERROR("create_listener(writequeue) failed\n");
				return 0;
			}
		}
		
		rv = crtx_start_listener(&plstnr->stdin_wq_lstnr.base);
		if (rv) {
			CRTX_ERROR("starting writequeue listener failed: %s\n", strerror(-rv));
			return rv;
		}
	}
	
	if (plstnr->fstdout == -1 && plstnr->stdout_cb) {
		rv = crtx_start_listener(&plstnr->stdout_lstnr.base);
		if (rv) {
			CRTX_ERROR("starting stdout pipe listener failed\n");
			return rv;
		}
	}
	
	if (plstnr->fstderr == -1 && plstnr->stderr_cb) {
		rv = crtx_start_listener(&plstnr->stderr_lstnr.base);
		if (rv) {
			CRTX_ERROR("starting stderr pipe listener failed\n");
			return rv;
		}
	}
	
	rv = crtx_start_listener(&plstnr->fork_lstnr.base);
	if (rv) {
		CRTX_ERROR("starting fork listener failed\n");
		return rv;
	}
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
	struct crtx_popen_listener *plstnr;
	
	
	plstnr = (struct crtx_popen_listener *) listener;
	
	
	// TODO also stop the child if it is still running?
	
// 	int rv;
// 	rv = crtx_stop_listener(&plstnr->fork_lstnr.base);
// 	if (rv) {
// 		CRTX_ERROR("stopping fork listener failed\n");
// 		return rv;
// 	}
	
	crtx_stop_listener(&plstnr->stdin_wq_lstnr.base);
	crtx_stop_listener(&plstnr->stdout_lstnr.base);
	crtx_stop_listener(&plstnr->stderr_lstnr.base);
	
	return 0;
}

static void shutdown_listener(struct crtx_listener_base *listener) {
	struct crtx_popen_listener *plstnr;
	
	
	plstnr = (struct crtx_popen_listener *) listener;
	
	if (plstnr->stdout_cb) {
		crtx_shutdown_listener(&plstnr->stdout_lstnr.base);
		// only close the write end on child side
		if (plstnr->fork_lstnr.pid != 0)
			close(plstnr->fstdout);
	}
	if (plstnr->stderr_cb) {
		crtx_shutdown_listener(&plstnr->stderr_lstnr.base);
		if (plstnr->fork_lstnr.pid != 0)
			close(plstnr->fstderr);
	}
	
	return;
}

// callback that is executed if the child process terminates
static int fork_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	(void) sessiondata;
	struct crtx_popen_listener *plstnr;
	
	
	plstnr = (struct crtx_popen_listener*) userdata;
	
	crtx_stop_listener(&plstnr->base);
	
	if (event->type == CRTX_FORK_ET_CHILD_STOPPED) {
		plstnr->rc = event->data.int32;
		
		CRTX_VDBG("child done with rc %d\n", plstnr->rc);
	} else
	if (event->type == CRTX_FORK_ET_CHILD_KILLED) {
		plstnr->rc = event->data.int32;
		
		CRTX_VDBG("child killed with rc %d\n", plstnr->rc);
	} else {
		CRTX_ERROR("unexpected event sent by fork listener %d\n", event->type);
	}
	
	crtx_add_event(plstnr->base.graph, event);
	
	return 0;
}

struct crtx_listener_base *crtx_setup_popen_listener(void *options) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) options;
	
	plstnr->base.start_listener = &start_listener;
	plstnr->base.stop_listener = &stop_listener;
	plstnr->base.shutdown = &shutdown_listener;
	
	plstnr->fork_lstnr.reinit_immediately = 1;
	plstnr->fork_lstnr.reinit_cb = &after_fork_reinit_cb;
	plstnr->fork_lstnr.reinit_cb_data = plstnr;
	plstnr->fork_lstnr.post_fork_child_callback = &post_fork_child_callback;
	plstnr->fork_lstnr.post_fork_child_cb_data = plstnr;
	
	rv = crtx_setup_listener("fork", &plstnr->fork_lstnr);
	if (rv) {
		CRTX_ERROR("create_listener(fork) failed\n");
		return 0;
	}
	
	if (!plstnr->fork_lstnr.base.graph->tasks)
		crtx_create_task(plstnr->fork_lstnr.base.graph, 0, "popen_fork_event_handler", &fork_event_handler, plstnr);
	
	if (plstnr->fstdin == -1 && plstnr->stdin_wq_lstnr.write) {
		rv = crtx_setup_listener("pipe", &plstnr->stdin_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(pipe) failed\n");
			return 0;
		}
		
		plstnr->stdin_wq_lstnr.write_fd = plstnr->stdin_lstnr.fds[CRTX_WRITE_END];
		
		rv = crtx_setup_listener("writequeue", &plstnr->stdin_wq_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(writequeue) failed\n");
			return 0;
		}
	}
	
	if (plstnr->fstdout == -1 && plstnr->stdout_cb) {
		plstnr->stdout_lstnr.fd_event_handler = pipe_event_handler;
		plstnr->stdout_lstnr.fd_event_handler_data = plstnr;
		
		rv = crtx_setup_listener("pipe", &plstnr->stdout_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(pipe) failed\n");
			return 0;
		}
	}
	
	if (plstnr->fstderr == -1 && plstnr->stderr_cb) {
		plstnr->stderr_lstnr.fd_event_handler = pipe_event_handler;
		plstnr->stderr_lstnr.fd_event_handler_data = plstnr;
		
		rv = crtx_setup_listener("pipe", &plstnr->stderr_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(pipe) failed\n");
			return 0;
		}
	}
	
	return &plstnr->base;
}

// can be used by the user application to indicate the child process that there
// will be no more data on stdin
void crtx_popen_close_stdin(struct crtx_popen_listener *lstnr) {
	crtx_stop_listener(&lstnr->stdin_wq_lstnr.base);
	
	close(lstnr->stdin_wq_lstnr.write_fd);
	lstnr->stdin_wq_lstnr.write_fd = -1;
	
	lstnr->stdin_lstnr.fds[CRTX_WRITE_END] = -1;
}

void crtx_popen_clear_lstnr(struct crtx_popen_listener *lstnr) {
	memset(lstnr, 0, sizeof(struct crtx_popen_listener));
	
	lstnr->fstdin = -1;
	lstnr->fstdout = -1;
	lstnr->fstderr = -1;
	
	crtx_pipe_clear_lstnr(&lstnr->stdin_lstnr);
	crtx_pipe_clear_lstnr(&lstnr->stdout_lstnr);
	crtx_pipe_clear_lstnr(&lstnr->stderr_lstnr);
}

CRTX_DEFINE_ALLOC_FUNCTION(popen)

void crtx_popen_init() {
}

void crtx_popen_finish() {
}


#else /* CRTX_TEST */

#define TEXT "First text output"
char buffer[] = TEXT;
size_t to_write = sizeof(TEXT);
size_t to_write2 = sizeof(TEXT);
int repeat = 1;
struct crtx_popen_listener plist;
struct crtx_popen_listener plist2;

static int popen_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_FORK_ET_CHILD_STOPPED || event->type == CRTX_FORK_ET_CHILD_KILLED) {
		printf("child (%zu) done with rc %d, shutdown parent\n", (size_t) userdata, event->data.int32);
		if (userdata == 0) {
			crtx_init_shutdown();
		} else {
			if (repeat == 1) {
				int ret;
				
				// ret = crtx_setup_listener("popen", &plist2);
				// if (ret) {
				// 	CRTX_ERROR("create_listener(popen) failed\n");
				// 	return 0;
				// }
				
				printf("starting p2 again\n");
				
				ret = crtx_start_listener(&plist2.base);
				if (ret) {
					CRTX_ERROR("starting second popen listener failed\n");
					return 0;
				}
				
				repeat = 0;
			}
		}
	} else {
		printf("unexpected event\n");
	}
	
	return 0;
}

// static int pipe_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_pipe_listener *lstnr;
// 	ssize_t r;
// 	char buf[256];
// 	
// 	lstnr = (struct crtx_pipe_listener *) userdata;

static void stdout_cb(int fd, void *userdata) {
	ssize_t r;
	char buf[256];
	
	printf("P1: read from %d\n", fd);
	r = read(fd, buf, sizeof(buf)-1);
	if (r < 0) {
		CRTX_ERROR("read from signal selfpipe %d failed: %s\n", fd, strerror(r));
		return;
	}
	buf[r] = 0;
	printf("P1 STDOUT: %s\n", buf);
	
	return;
}

static void stdout_cb2(int fd, void *userdata) {
	ssize_t r;
	char buf[256];
	
	printf("P2: read from %d\n", fd);
	r = read(fd, buf, sizeof(buf)-1);
	if (r < 0) {
		CRTX_ERROR("read from signal selfpipe %d failed: %s\n", fd, strerror(r));
		return;
	}
	buf[r] = 0;
	printf("P2: STDOUT: %s\n", buf);
	
	return;
}

static int wq_write(struct crtx_writequeue_listener *wqueue, void *userdata) {
	ssize_t n_bytes;
	size_t *buflen;
	
	if (userdata == &plist) {
		buflen = &to_write;
	} else {
		buflen = &to_write2;
	}
	
	while (1) {
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
				if (userdata == &plist) {
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

int popen_main(int argc, char **argv) {
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
		// "id; echo stdout_test; echo stderr_test >&2; pwd; echo sleep 5; exit 1;",
		"id; echo waiting...; cat; sleep 5; echo \"last output\"; exit 1;",
		// "exec cat",
		0
		};
	
	if (getuid() == 0) {
		plist.fork_lstnr.user = "nobody";
		plist.fork_lstnr.group = "nogroup";
	}
	
	plist.stdin_wq_lstnr.write = &wq_write;
	plist.stdin_wq_lstnr.write_userdata = &plist;
	
	plist.stdout_cb = &stdout_cb;
	
	plist.pre_exec = &pre_exec;
	
	ret = crtx_setup_listener("popen", &plist);
	if (ret) {
		CRTX_ERROR("create_listener(popen) failed\n");
		return 0;
	}
	
	crtx_create_task(plist.base.graph, 0, "popen_event", &popen_event_handler, 0);
	
	ret = crtx_start_listener(&plist.base);
	if (ret) {
		CRTX_ERROR("starting popen listener failed\n");
		return 0;
	}
	
	if (1) {
		// setup process 2
		
		plist2.filepath = "/bin/sh";
		plist2.argv = (char*[]) { (char*) plist.filepath, "-c", "cat; sleep 1; echo \"intermediate p2 output\"; exit 0;", 0};
		
		plist2.stdout_cb = &stdout_cb2;
		
		plist2.stdin_wq_lstnr.write = &wq_write;
		plist2.stdin_wq_lstnr.write_userdata = &plist2;
		
		ret = crtx_setup_listener("popen", &plist2);
		if (ret) {
			CRTX_ERROR("create_listener(popen) failed\n");
			return 0;
		}
		
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

// CRTX_TEST_MAIN(popen_main);
int main(int argc, char **argv) {
	int i;
// 	crtx_init();
// 	crtx_handle_std_signals();
	i = popen_main(argc, argv);
// 	crtx_finish();
	return i;
}
#endif
