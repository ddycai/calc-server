build: parser.c stack.c stack.h parser.h calc-server.c calc-client.c
	gcc calc-server.c parser.c stack.c -o calc-server
	gcc calc-client.c -o calc-client
