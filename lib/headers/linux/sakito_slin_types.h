#ifndef SAKITO_LINUX_TYPES_H
#define SAKITO_LINUX_TYPES_H

#include <pthread.h>
#include "../sakito_multi_def.h"

#define SOCKET int
#define CONSOLE_FSTR "sak1to\x1b[38;5;200m-\x1b[39mconsole:~%s$ "
#define INTERACT_FSTR "\x1b[38;5;200m┌\x1b[39m%d\x1b[38;5;200m─\x1b[39m%s\n\x1b[38;5;200m└\x1b[39m%s>"
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define _FILE_OFFSET_BITS 64
#define INVALID_FILE -1

typedef struct {
	// Client hostname.
	char* host;

	// Client socket.
	SOCKET sock;

} Conn;

typedef struct {
	// Server buffer.
	char buf[BUFLEN + 9]; // BUFLEN + space for 'command code' + "cmd /C " + '\0'

	// Server socket for accepting connections.
	SOCKET listen_socket;

	// Pthread object for handling execution/termination of accept_conns thread.
	pthread_t acp_thread;

	// Flag for race condition checks.
	int THRD_FLAG;

	// Array of Conn objects/structures.
	Conn* clients;

	// Memory blocks allocated.
	size_t clients_alloc;

	// Amount of memory used.
	size_t clients_sz;

} Server_map;

typedef int s_file;

#endif
