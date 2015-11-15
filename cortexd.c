
#include "core.h"

int main(int argc, char *argv[]) {
	unsigned int i;
	
	while (static_plugins[i].id) {
		static_plugins[i].init();
		i++;
	}
}
