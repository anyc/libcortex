
#include <stdio.h>
#include <unistd.h>

#include "core.h"

int main(int argc, char *argv[]) {
	cortex_init();
	
	sleep(10);
// 	while (1) {
// 		sleep(1);
// 	}
	
	cortex_finish();
	
	return 0;
}
