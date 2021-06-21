all:
	gcc -o client client.c
	gcc -o server server.c
	./dump_recv_queue.sh
