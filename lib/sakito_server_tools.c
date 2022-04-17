/* 
Coded by d4rkstat1c.
Use educationally/legally.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "headers/os_check.h"
#ifdef OS_WIN
#include <WS2tcpip.h>
#include "headers/windows/sakito_swin_types.h"
#include "headers/sakito_server_common.h"
#include "headers/sakito_multi_def.h"

// Closing a NT kernel handle/SOCKET.
#define s_closesocket(socket) closesocket(socket)

// Function to gracefully terminate server.
void terminate_server(const SOCKET socket, const char* const error) 
{
	int err_code = EXIT_SUCCESS;
	if (error) 
	{
		fprintf(stderr, "%s: %ld\n", error, WSAGetLastError());
		err_code = 1;
	}

	closesocket(socket);
	WSACleanup();
	exit(err_code);
}

// Mutex lock functionality.
void s_mutex_lock(Server_map* const s_map) 
{
	// If mutex is currently locked wait until it is unlocked.
	WaitForSingleObject(s_map->ghMutex, INFINITE);
}

// Mutex unlock functionality.
void s_mutex_unlock(Server_map* const s_map) 
{
	// Unlock mutex.
	ReleaseMutex(s_map->ghMutex);
}
#elif defined OS_LIN
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "headers/linux/sakito_slin_types.h"

// Closing a socket to match Windows' closesocket() WINAPI's signature.
#define s_closesocket(socket) close(socket);


// Mutex lock for pthread race condition checks.
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// Variable for mutex condition.
pthread_cond_t  consum = PTHREAD_COND_INITIALIZER;


// wrapper for terminating server.
void terminate_server(int listen_socket, const char* const error) 
{
	close(listen_socket);
	int err_code = EXIT_SUCCESS;
	if (error) 
	{
		err_code = 1;
		perror(error);
	}

	exit(err_code);
}

// Mutex unlock functionality.
void s_mutex_lock(Server_map* const s_map) 
{
	// Wait until THRD_FLAG evaluates to false.
	while (s_map->THRD_FLAG)
		pthread_cond_wait(&consum, &lock);

	pthread_mutex_lock(&lock);

	// We're now locking the mutex so we can modify shared memory in a thread safe manner.
	s_map->THRD_FLAG = 1;
}

// Mutex unlocking functionality.
void s_mutex_unlock(Server_map* const s_map) 
{
	pthread_mutex_unlock(&lock);

	// Set THRD_FLAG to false to communicate with mutex_lock() that we have finished modifying shared memory.
	s_map->THRD_FLAG = 0;
}
#endif

#define MEM_CHUNK 5
#define INVALID_CLIENT_ID -1
#define BACKGROUND -100
#define FILE_NOT_FOUND 1

void s_read_stdin(char* const buf, size_t* cmd_len) 
{
	char c;
	while ((c = getchar()) != '\n' && *cmd_len < BUFLEN)
		buf[(*cmd_len)++] = c;
}

// Function to validate client identifier prior to interaction.
int s_validate_id(Server_map* const s_map) 
{
	int client_id;
	client_id = atoi(s_map->buf+9);

	if (!s_map->clients_sz || client_id < 0 || client_id > s_map->clients_sz - 1)
		return INVALID_CLIENT_ID;

	return client_id;
}

// Function to compare two strings (combined logic of strcmp and strncmp).
char s_compare(const char* buf, const char* str) 
{
	while (*str)
		if (*buf++ != *str++)
			return 0;

	return 1;
}

// Function to return function pointer based on parsed command.
void* s_parse_cmd(char* const buf, size_t* cmd_len, const char commands[][11], void** func_array, void* default_func) 
{
	// Set all bytes in buffer to zero.
	memset(buf, '\0', BUFLEN);

	// Read command string from stdin.
	s_read_stdin(buf, cmd_len);

	if (*cmd_len)
	{
		// Parse stdin string and return its corresponding function pointer.
		while (*func_array)
		{
			if (s_compare(buf, *commands))
				return *func_array;
			commands++;
			func_array++;
		}
	}

	return default_func;
}

// Function to bind socket to specified port.
void bind_socket(const SOCKET listen_socket) 
{
	// Create sockaddr_in structure.
	struct sockaddr_in sin;

	// Assign member values.
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind ip address and port to listen_socket.
	if (bind(listen_socket, (struct sockaddr*)&sin, sizeof(sin)) != 0)
		terminate_server(listen_socket, "Socket bind failed with error");

	// Place the listen_socket in listen state.
	if (listen(listen_socket, SOMAXCONN) != 0)
		terminate_server(listen_socket, "An error occured while placing the socket in listening state");
}

void add_client(Server_map* const s_map, char* const host, const SOCKET client_socket) 
{
	// Lock our mutex to prevent race conditions from occurring with s_delete_client()
	s_mutex_lock(s_map);

	if (s_map->clients_sz == s_map->clients_alloc)
		s_map->clients = realloc(s_map->clients, (s_map->clients_alloc += MEM_CHUNK) * sizeof(Conn));

	// Add hostname string and client_socket file descriptor to s_map->clients structure.
	s_map->clients[s_map->clients_sz].host = host;
	s_map->clients[s_map->clients_sz].sock = client_socket;
	s_map->clients_sz++;

	// Unlock our mutex now.
	s_mutex_unlock(s_map);
}

void s_accept_conns(Server_map* const s_map) 
{
	// Assign member values to connection map object/structure.
	s_map->clients_alloc = MEM_CHUNK;
	s_map->clients_sz = 0;
	s_map->clients = malloc(s_map->clients_alloc * sizeof(Conn));

	while (1) 
	{
		struct sockaddr_in client;
		int c_size = sizeof(client);

		// Client socket object.
		const SOCKET client_socket = accept(s_map->listen_socket, (struct sockaddr*)&client, &c_size);
		if (client_socket == INVALID_SOCKET)
			terminate_server(s_map->listen_socket, "Error accepting client connection");

		// Client's remote name and port.
		char host[NI_MAXHOST] = { 0 };
		char service[NI_MAXHOST] = { 0 };

		// Get hostname and port to print to stdout.
		if (getnameinfo((struct sockaddr*)&client, c_size, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) 
		{
			printf("%s connected on port %s\n", host, service);
		}
		else 
		{
			inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
			printf("%s connected on port %hu\n", host, ntohs(client.sin_port));
		}

		// Add client oriented data to s_map object.
		add_client(s_map, host, client_socket);
	}
}

void resize_conns(Server_map* const s_map, int client_id) 
{
	// If there's more than one connection: resize the clients structure member values.
	if (s_map->clients_sz > 1) 
	{
		int max_index = s_map->clients_sz-1;
		for (size_t i = client_id; i < max_index; i++) 
		{
			s_map->clients[i].sock = s_map->clients[i + 1].sock;
			s_map->clients[i].host = s_map->clients[i + 1].host;
		}
		s_map->clients[max_index].sock = 0;
		s_map->clients[max_index].host = NULL;
	}

	s_map->clients_sz--;
}

// Function to resize s_map array/remove and close connection.
void s_delete_client(Server_map* const s_map, const int client_id) 
{
	// Lock our mutex to prevent race conditions from occurring with accept_conns().
	s_mutex_lock(s_map);

	// If the file descriptor is open: close it.
	if (s_map->clients[client_id].sock)
		s_closesocket(s_map->clients[client_id].sock);

	// Resize clients member values to remove client.
	resize_conns(s_map, client_id);

	// Unlock our mutex now.
	s_mutex_unlock(s_map);
	printf("Client: \"%s\" disconnected.\n\n", s_map->clients[client_id].host);
}
