
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "intern.h"
#include "core.h"
#include "signals.h"

int main(int argc, char *argv[]) {
	crtx_init();
	crtx_handle_std_signals();
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}
