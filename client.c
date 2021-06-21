#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_IP "192.168.1.1"
#define PORT 3000
#define BUF_SIZE 256
#define err_msg(msg) do {perror(msg); exit(1);} while(0)

int main()
{
	int sock, ret;
	char buffer[BUF_SIZE];
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr = inet_addr(SERVER_IP),
		.sin_port = htons(PORT)
	};

	memset(buffer, 'A', sizeof(buffer));
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
		err_msg("socket");

	ret = sendto(sock, buffer, sizeof(buffer), 0,
		     (const struct sockaddr *) &addr, sizeof(addr));
	if (ret < 0)
		err_msg("send-data");
	
	return 0;
}
