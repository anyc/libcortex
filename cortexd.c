
#include <stdio.h>

#include "core.h"

int main(int argc, char *argv[]) {
	unsigned int i;
	
	init_core();
	
	i=0;
	while (static_plugins[i].id) {
		printf("initialize \"%s\"\n", static_plugins[i].id);
		
		static_plugins[i].init();
		i++;
	}
	
	while (1) {
		sleep(1);
	}
}
