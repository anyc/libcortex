
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "core.h"
#include "socket.h"

void socket_send(struct socket_req *req) {
	int sockfd = 0, n = 0;
	int ret;
	char recvBuff[1024];
	struct sockaddr_in serv_addr;
	
	
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0); ASSERT(sockfd >= 0);
	
	memset(recvBuff, '0',sizeof(recvBuff));
	memset(&serv_addr, '0', sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(1234);
	
	ret = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr); ASSERT(ret >= 0);
	
	ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); ASSERT(ret >= 0);
	
	ret = write(sockfd, req, sizeof(struct socket_req)); ASSERT(ret >= 0);
	
	ret = write(sockfd, req->type, req->type_length); ASSERT(ret >= 0);
	
	ret = write(sockfd, req->data, req->data_length); ASSERT(ret >= 0);
	
	close(sockfd);
	
	// 	while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0) {
	// 		recvBuff[n] = 0;
	// 		if(fputs(recvBuff, stdout) == EOF)
	// 		{
	// 			printf("\n Error : Fputs error\n");
	// 		}
	// 	} 
	// 	
	// 	if(n < 0)
	// 	{
	// 		printf("\n Read error \n");
	// 	} 
}

void main(int argc, char **argv) {
	struct socket_req req;
	
	if (argc < 3) {
		printf("Usage: %s <event_type> <event_data>\n", argv[0]);
		exit(1);
	}
	
	req.type = argv[1];
	req.type_length = strlen(req.type);
	
	req.data = argv[2];
	req.data_length = strlen(req.data);
	
	socket_send(&req);
}