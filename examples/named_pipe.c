
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <core.h>

/*
 * This example creates a named pipe/fifo under the path given as first commandline
 * parameter and printfs the data another process writes into the pipe.
 */

static int fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char *pipe_path;
	ssize_t n_bytes;
	char buf[32];
	struct crtx_evloop_callback *el_cb;
	int flags;
	
	// el_cb is a management structure for a callback that also contains information
	// about why this callback has been called
	el_cb = (struct crtx_evloop_callback *) event->data.pointer;
	
	// pointer we registered with crtx_listener_add_fd() below
	pipe_path = (char *) userdata;
	
	printf("activity on fd %d: ", el_cb->fd_entry->l_fd); crtx_event_flags2str(stdout, el_cb->triggered_flags); printf("\n");
	
	// if there is something to read, print the data
	if (el_cb->triggered_flags & CRTX_EVLOOP_READ) {
		n_bytes = read(el_cb->fd_entry->l_fd, buf, sizeof(buf)-1);
		if (n_bytes < 0) {
			fprintf(stderr, "read() failed: %s\n", strerror(errno));
			return 0;
		}
		buf[n_bytes] = 0;
		
		printf("RX %s: \"%s\"\n", pipe_path, buf);
		
		/*
		 * As an example we could change the timeout and the type of events we
		 * are interested in.
		 */
		
		// in microseconds
		crtx_evloop_set_timeout_rel(el_cb, 3000000);
		
		flags = CRTX_EVLOOP_ALL;
		if (flags != el_cb->crtx_event_flags) {
			el_cb->crtx_event_flags = flags;
			crtx_evloop_enable_cb(el_cb);
		}
	}
	
	// is something else going on with our fd?
	if (el_cb->triggered_flags & ~CRTX_EVLOOP_READ) {
		if (el_cb->triggered_flags == CRTX_EVLOOP_TIMEOUT) {
			printf("timeout\n");
			
			crtx_evloop_set_timeout_rel(el_cb, 3000000);
			
			return 0;
		}
		
		/*
		 * If there is an error like the writer closed the pipe, we have we have
		 * to reopen the pipe.
		 */
		
		// first deactivate this callback and close the fd
		crtx_evloop_disable_cb(el_cb);
		close(el_cb->fd_entry->l_fd);
		
		// open the pipe again and set the fd
		el_cb->fd_entry->l_fd = open(pipe_path, O_RDONLY | O_NONBLOCK);
		if (el_cb->fd_entry->l_fd < 0) {
			fprintf(stderr, "opening \"%s\" failed: %s\n", pipe_path, strerror(errno));
			return -errno;
		}
		
		// set the timeout and enable the callback again
		crtx_evloop_set_timeout_rel(el_cb, 3000000);
		crtx_evloop_enable_cb(el_cb);
	}
	
	return 0;
}

int main(int argc, char **argv) {
	int fd, rv;
	char *pipe_path;
	struct crtx_listener_base *lstnr;
	
	
	pipe_path = argv[1];
	
	/*
	 * create and open a regular named pipe/fifo in read-only mode
	 */
	
	rv = mkfifo(pipe_path, 0666);
	if (rv) {
		fprintf(stderr, "mkfifo(\"%s\") failed: %s\n", pipe_path, strerror(errno));
		return -errno;
	}
	
	// also set O_NONBLOCK as else it would not return before a writer opened the pipe
	fd = open(pipe_path, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "opening \"%s\" failed: %s\n", pipe_path, strerror(errno));
		return -errno;
	}
	
	/*
	 * initialize libcortex and a generic listener with the file descriptor of
	 * the pipe/fifo
	 */
	
	crtx_init();
	crtx_handle_std_signals();
	
	lstnr = crtx_calloc_listener_base();
	if (!lstnr) {
		fprintf(stderr, "crtx_alloc_listener() failed\n");
		return -1;
	}
	
	rv = crtx_setup_listener("", lstnr);
	if (rv) {
		CRTX_ERROR("create_listener() failed: %d\n", rv);
		return rv;
	}
	
	// tell libcortex that, if the listener is started, it should call
	// fd_event_handler on all events and at least call the function every 3 seconds
	rv = crtx_listener_add_fd(lstnr,
			0, fd, // int fd,
			CRTX_EVLOOP_ALL, // int event_flags,
			3000000, // uint64_t timeout_us,
			fd_event_handler, // crtx_handle_task_t event_handler,
			pipe_path, // void *event_handler_data,
			0, // crtx_evloop_error_cb_t error_cb,
			0 // void *error_cb_data
		);
	if (rv) {
		CRTX_ERROR("crtx_evloop_init_listener() failed: %d\n", rv);
		return rv;
	}
	
	// start the listener, i.e. add the the fd to the event loop in this example
	rv = crtx_start_listener(lstnr);
	if (rv) {
		CRTX_ERROR("crtx_start_listener() failed: %d\n", rv);
		return rv;
	}
	
	// Start the event loop. This function only returns when the application terminates
	crtx_loop();
	
	/*
	 * cleanup
	 */
	
	crtx_shutdown_listener(lstnr);
	
	crtx_finish();
	
	close(fd);
	free(lstnr);
	unlink(pipe_path);
	
	return 0;
}
