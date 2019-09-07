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

// getpwname
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>


#include "intern.h"
#include "core.h"
#include "popen.h"
#include "fork.h"

#ifndef CRTX_TEST

void reinit_cb(void *reinit_cb_data) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) reinit_cb_data;
	
	if (plstnr->filepath)
		printf("exec %s (%d %d %d)\n", plstnr->filepath, plstnr->stdin, plstnr->stdout, plstnr->stderr);
	else
		printf("exec %s (%d %d %d)\n", plstnr->filename, plstnr->stdin, plstnr->stdout, plstnr->stderr);
	
	rv = dup2(plstnr->stdin, STDIN_FILENO);
	if (rv == -1) {
		ERROR("dup2(stdin) failed: %s\n", strerror(errno));
		exit(1);
	}
	
	rv = dup2(plstnr->stdout, STDOUT_FILENO);
	if (rv == -1) {
		ERROR("dup2(stdout) failed: %s\n", strerror(errno));
		exit(1);
	}
	
	rv = dup2(plstnr->stderr, STDERR_FILENO);
	if (rv == -1) {
		ERROR("dup2(stderr) failed: %s\n", strerror(errno));
		exit(1);
	}
	
	if (plstnr->group) {
		struct group grp;
		struct group *p_grp;
		char *buf;
		long bufsize;
		
		bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
		if (bufsize == -1)
			bufsize = 1024;
		
		buf = calloc(1, bufsize);
		
		rv = getgrnam_r(plstnr->group, &grp, buf, bufsize, &p_grp);
		if (rv != 0 || p_grp == 0) {
			ERROR("getgrpnam_r(%s) failed: %s\n", plstnr->group, strerror(errno));
			exit(1);
		}
		
		DBG("change group to %s (%d)\n", plstnr->group, grp.gr_gid);
		
		rv = setgid(grp.gr_gid);
		if (rv == -1) {
			ERROR("setgid(%d) failed: %s\n", grp.gr_gid, strerror(errno));
			exit(1);
		}
		
		free(buf);
	}
	
	if (plstnr->user) {
		struct passwd pw;
		struct passwd *p_pw;
		char *buf;
		long bufsize;
		
		bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (bufsize == -1)
			bufsize = 1024;
		
		buf = calloc(1, bufsize);
		
		rv = getpwnam_r(plstnr->user, &pw, buf, bufsize, &p_pw);
		if (rv != 0 || p_pw == 0) {
			ERROR("getpwnam_r(%s) failed: %s\n", plstnr->user, strerror(errno));
			exit(1);
		}
		
		DBG("change user to %s (%d %d)\n", plstnr->user, pw.pw_uid, pw.pw_gid);
		
		rv = setuid(pw.pw_uid);
		if (rv == -1) {
			ERROR("setuid(%d) failed: %s\n", pw.pw_uid, strerror(errno));
			exit(1);
		}
		
		if (!plstnr->group) {
			rv = setgid(pw.pw_gid);
			if (rv == -1) {
				ERROR("setgid(%d) failed: %s\n", pw.pw_gid, strerror(errno));
				exit(1);
			}
		}
		
		free(buf);
	}
	
	if (plstnr->argv == 0) {
		if (plstnr->filepath) {
			plstnr->argv = (char*[]) { plstnr->filepath, 0 };
		} else {
			plstnr->argv = (char*[]) { plstnr->filename, 0 };
		}
	}
	
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
		ERROR("exec*(%s) failed: %s\n", plstnr->filepath, strerror(errno));
		exit(1);
	}
	
	crtx_init_shutdown();
}

static char pipe_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_pipe_listener *lstnr;
	ssize_t r;
	char buf[256];
	
	lstnr = (struct crtx_pipe_listener *) userdata;
	
	r = read(lstnr->fds[0], buf, sizeof(buf)-1);
	if (r < 0) {
		ERROR("read from signal selfpipe %d failed: %s\n", lstnr->fds[0], strerror(r));
		return r;
	}
	buf[r] = 0;
	printf("PIPE: %s\n", buf);
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) listener;
	
	plstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
	rv = crtx_start_listener(&plstnr->stdin_lstnr.base);
	if (rv) {
		ERROR("starting writequeue listener failed\n");
		return rv;
	}
	// preserve fd as it would be closed by the after_fork_shutdown otherwise
	plstnr->stdin = plstnr->stdin_lstnr.fds[0];
	plstnr->stdin_lstnr.fds[0] = 0;
	
	plstnr->stdin_wq_lstnr.write_fd = plstnr->stdin_lstnr.fds[1];
// 	plstnr->stdin_wq_lstnr.write = &wq_write;
	rv = crtx_start_listener(&plstnr->stdin_wq_lstnr.base);
	if (rv) {
		ERROR("starting writequeue listener failed\n");
		return rv;
	}
	
	rv = crtx_start_listener(&plstnr->stdout_lstnr.base);
	if (rv) {
		ERROR("starting writequeue listener failed\n");
		return rv;
	}
	plstnr->stdout = plstnr->stdout_lstnr.fds[1];
	plstnr->stdout_lstnr.fds[1] = 0;
	
	rv = crtx_start_listener(&plstnr->stderr_lstnr.base);
	if (rv) {
		ERROR("starting writequeue listener failed\n");
		return rv;
	}
	plstnr->stderr = plstnr->stderr_lstnr.fds[1];
	plstnr->stderr_lstnr.fds[1] = 0;
	
	rv = crtx_start_listener(&plstnr->fork_lstnr.base);
	if (rv) {
		ERROR("starting fork listener failed\n");
		return rv;
	}
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
// 	int rv;
	
// 	rv = crtx_stop_listener(&plstnr->fork_lstnr.base);
// 	if (rv) {
// 		ERROR("stopping fork listener failed\n");
// 		return rv;
// 	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_popen_listener(void *options) {
	struct crtx_popen_listener *plstnr;
	int rv;
	
	
	plstnr = (struct crtx_popen_listener *) options;
	
	plstnr->base.start_listener = &start_listener;
	plstnr->base.stop_listener = &stop_listener;
	
	
	plstnr->fork_lstnr.reinit_cb = &reinit_cb;
	plstnr->fork_lstnr.reinit_cb_data = plstnr;
	
	rv = crtx_create_listener("fork", &plstnr->fork_lstnr);
	if (rv) {
		ERROR("create_listener(fork) failed\n");
		return 0;
	}
	
	plstnr->stdin_lstnr.fd_event_handler = pipe_event_handler;
	plstnr->stdin_lstnr.fd_event_handler_data = &plstnr->stdin_lstnr;
	
	rv = crtx_create_listener("pipe", &plstnr->stdin_lstnr);
	if (rv) {
		ERROR("create_listener(pipe) failed\n");
		return 0;
	}
	
	rv = crtx_create_listener("writequeue", &plstnr->stdin_wq_lstnr);
	if (rv) {
		ERROR("create_listener(writequeue) failed\n");
		return 0;
	}
	
	plstnr->stdout_lstnr.fd_event_handler = pipe_event_handler;
	plstnr->stdout_lstnr.fd_event_handler_data = &plstnr->stdout_lstnr;
	
	rv = crtx_create_listener("pipe", &plstnr->stdout_lstnr);
	if (rv) {
		ERROR("create_listener(pipe) failed\n");
		return 0;
	}
	
	plstnr->stderr_lstnr.fd_event_handler = pipe_event_handler;
	plstnr->stderr_lstnr.fd_event_handler_data = &plstnr->stderr_lstnr;
	
	rv = crtx_create_listener("pipe", &plstnr->stderr_lstnr);
	if (rv) {
		ERROR("create_listener(pipe) failed\n");
		return 0;
	}
	
// 	crtx_create_task(fork_lstnr.base.graph, 0, "fork_event_handler", fork_event_handler, 0);
	
	return &plstnr->base;
}

void crtx_popen_init() {
}

void crtx_popen_finish() {
}


#else /* CRTX_TEST */

#define BUF_SIZE 1024
char buffer[BUF_SIZE];

static char popen_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_FORK_EVT_CHILD_STOPPED) {
		printf("child done, shutdown parent\n");
		crtx_init_shutdown();
	} else {
		printf("unexpected event\n");
	}
	
	return 0;
}

static int wq_write(struct crtx_writequeue *wqueue, void *userdata) {
	ssize_t n_bytes;
	
	while (1) {
		n_bytes = write(wqueue->write_fd, buffer, BUF_SIZE);
		if (n_bytes < 0) {
			if (errno == EAGAIN || errno == ENOBUFS) {
				printf("received EAGAIN\n");
				
				crtx_writequeue_start(wqueue);
				
				return EAGAIN;
			} else {
				printf("error %s\n", strerror(errno));
				exit(1);
			}
		} else {
		}
	}
}


int popen_main(int argc, char **argv) {
	struct crtx_popen_listener plist;
	int ret;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	memset(&plist, 0, sizeof(struct crtx_popen_listener));
	
// 	plist.filepath = "/bin/sh";
// 	plist.argv = (char*[]) {plist.filepath, "-c", "whoami; groups;", 0};
	plist.filename = "id";
	plist.user = "nobody";
	plist.group = "audio";
	
	plist.stdin_wq_lstnr.write = &wq_write;
	plist.stdin_wq_lstnr.write_userdata = 0;
	
	ret = crtx_create_listener("popen", &plist);
	if (ret) {
		ERROR("create_listener(popen) failed\n");
		return 0;
	}
	
	crtx_create_task(plist.fork_lstnr.base.graph, 0, "popen_event", &popen_event_handler, 0);
	
	ret = crtx_start_listener(&plist.base);
	if (ret) {
		ERROR("starting popen listener failed\n");
		return 0;
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
