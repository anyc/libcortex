
#include <stdio.h>
#include <stdlib.h>

#include <crtx/core.h>
#include <crtx/dict.h>

#include "ipquery.h"

int main(unsigned int argc, char **argv) {
	int rc;
	struct crtx_dll *interfaces, *if_iter;
	
	
	ipq_init();
	
	rc = ipq_get_ips(&interfaces);
// 	for (if_iter = interfaces; if_iter; if_iter++) {
// 		
// 	}
	
	ipq_finish();
	
	return 0;
}
