
#include <stdio.h>
#include <unistd.h>

#include "core.h"

int main(int argc, char *argv[]) {
	unsigned int i;
	
	init_core();
	
	i=0;
	while (static_modules[i].id) {
		printf("initialize \"%s\"\n", static_modules[i].id);
		
		static_modules[i].init();
		i++;
	}
	
	while (1) {
		sleep(1);
	}
}
