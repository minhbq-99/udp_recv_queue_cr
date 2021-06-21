#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <linux/in.h>
#include <assert.h>
#include <unistd.h>
#include <net/if.h>

#define PORT 3000
#define BUF_SIZE 256

#define err_msg(msg) do {perror(msg); exit(1);} while(0)


struct udp_dump {
	struct in_addr client_addr;
	char buffer[BUF_SIZE];
};

int create_server_sock()
{
	int server_sock, ret;
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr = inet_addr("0.0.0.0"),
		.sin_port = htons(PORT)
	};

	server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server_sock < 0)
		err_msg("socket");

	ret = bind(server_sock, (const struct sockaddr *) &addr, sizeof(addr));
	if (ret < 0)
		err_msg("bind");

	return server_sock;
}

struct udp_dump *checkpoint(int server_sock)
{
	struct udp_dump *dump;
	struct sockaddr_in client_addr;
	int ret;
	unsigned int addr_len;
	char client_ip[INET_ADDRSTRLEN];

	dump = malloc(sizeof(*dump));
	if (!dump) {
		fprintf(stderr, "malloc\n");
		exit(1);
	}

	addr_len = sizeof(client_addr);
	ret = recvfrom(server_sock, dump->buffer, BUF_SIZE, 0,
		       (struct sockaddr *) &client_addr, &addr_len);
	if (ret < 0)
		err_msg("recv-data");

	dump->client_addr = client_addr.sin_addr;
	inet_ntop(AF_INET, &dump->client_addr, client_ip, sizeof(client_ip));
	printf("Dump packet from: %s\n", client_ip);

	return dump;
}

void restore(struct udp_dump *dump)
{
	int client_sock, ret, val = 1;
	struct msghdr msg = {0};
	struct cmsghdr *cm;
	struct iovec iov = {0};
	char control[CMSG_SPACE(sizeof(struct in_pktinfo))];
	struct in_pktinfo pktinfo = {
		.ipi_spec_dst = dump->client_addr,
	};
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr = inet_addr("127.0.0.1"),
		.sin_port = htons(PORT)
	};

	client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_sock < 0)
		err_msg("client-socket");

	ret = setsockopt(client_sock, SOL_IP, IP_TRANSPARENT, &val, sizeof(val));
	if (ret < 0)
		err_msg("setsockopt-transparent");

	iov.iov_base = dump->buffer;
	iov.iov_len = BUF_SIZE;

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_name = &addr;
	msg.msg_namelen = sizeof(addr);

	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);

	cm = CMSG_FIRSTHDR(&msg);
	cm->cmsg_level = SOL_IP;
	cm->cmsg_type = IP_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
	*(struct in_pktinfo *) CMSG_DATA(cm) = pktinfo;

	ret = sendmsg(client_sock, &msg, 0);
	if (ret < 0)
		err_msg("sendmsg");

	close(client_sock);
}

void recv_packet(int server_sock, struct udp_dump *dump)
{
	struct sockaddr_in client_addr;
	unsigned int addr_len;
	int ret;
	char buffer[BUF_SIZE];
	char client_ip[INET_ADDRSTRLEN];

	addr_len = sizeof(client_addr);
	ret = recvfrom(server_sock, buffer, BUF_SIZE, 0,
		       (struct sockaddr *) &client_addr, &addr_len);
	if (ret < 0)
		err_msg("recv-data");

	assert(!memcmp(buffer, dump->buffer, BUF_SIZE));
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
	printf("Recv packet from: %s\n", client_ip);
}

int main()
{
	int server_sock, ret;
	struct pollfd pfd;
	struct udp_dump *dump;

	server_sock = create_server_sock();

	pfd.fd = server_sock;
	pfd.events = POLLIN;
	ret = poll(&pfd, 1, -1);
	if (ret < 0)
		err_msg("poll");

	printf("Checkpointing ...\n");
	dump = checkpoint(server_sock);
	close(server_sock);

	server_sock = create_server_sock();
	printf("Restoring ...\n");
	restore(dump);

	recv_packet(server_sock, dump);

	free(dump);
	close(server_sock);
	
	return 0;
}
