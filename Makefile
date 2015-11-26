CC = gcc
LIBS = -lpthread

all:serverchat clientchat

serverchat: server_chat.o
	$(CC) $(CFLAGS) $(LIBS) -o server_chat server_chat_v2.c

clientchat: client_chat.o
	$(CC) $(CFLAGS) -o client_chat client_chat.c

clean:
	rm -f *.o server_chat *~
	rm -f *.o client_chat *~
