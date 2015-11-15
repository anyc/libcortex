

struct socket_req {
	unsigned int type_length;
	unsigned int data_length;
	char *type;
	char *data;
};


void socket_init();
void socket_send(struct socket_req *req);