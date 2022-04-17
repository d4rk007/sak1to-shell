/* 
Coded by d4rkstat1c.
Use educationally/legally.
*/
#include <WS2tcpip.h>
#include <Windows.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../headers/sakito_core_funcs.h"
#include "../headers/windows/sakito_swin_types.h"
#include "../headers/sakito_server_common.h"
#include "../headers/sakito_multi_def.h"

void bind_socket(const SOCKET listen_socket);
void s_accept_conns(Server_map* const s_map);

/*
Below contains windows specific sakito API functions.
*/

// Store current working directory in a provided buffer.
void get_cwd(char* buf)
{
	GetCurrentDirectory(BUFLEN, buf);
}

// Writing/sending data to a given NT kernel handle (SOCKET).
int s_tcp_send(SOCKET socket, char* buf, size_t count)
{
	return send(socket, buf, count, 0);
}

// Reading/receiving data from a given NT kernel handle (SOCKET).
int s_tcp_recv(SOCKET socket, char* buf, size_t count)
{
	return recv(socket, buf, count, 0);
}

// Calculating the size of a file via HANDLE.
uint64_t s_file_size(s_file file)
{
	return s_win_fsize(file); 
}

// Writing/sending a file's bytes to client (upload).
int s_send_file(SOCKET socket, s_file file, char* buf, uint64_t f_size)
{
	return s_win_sendf(socket, file, buf, f_size);
}

// Reading/receiving a file's bytes from client (download).
int s_recv_file(SOCKET socket, s_file file, char* buf, uint64_t f_size)
{
	return s_win_recvf(socket, file, buf, f_size);
}

// Closing a given HANDLE.
void s_close_file(s_file file)
{
	CloseHandle(file);
}

// Closing a NT kernel handle/SOCKET.
void s_closesocket(SOCKET socket)
{
	closesocket(socket);
}

// WriteFile() WINAPI wrapper for writing data to stdout.
int s_write_stdout(const char* buf, size_t count) 
{
	HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!WriteFile(std_out, buf, count, NULL, NULL))
		return FAILURE;

	return SUCCESS;
}

// Create a socket object.
const SOCKET create_socket() 
{
	// Initialize winsock.
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &wsData);

	// Create the server socket object.
	const SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_socket == INVALID_SOCKET) 
	{
		fprintf(stderr, "Socket creation failed: %ld\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}

	int optval = 1;
	if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) != 0)
		terminate_server(listen_socket, "Error setting socket options");

	return listen_socket;
}

// Open/create a file (s_file is a typedef for HANDLE).
s_file s_open_file(const char *filename, int rw_flag) 
{
	// Supports only READ/WRITE file modes/operations.
	if (rw_flag == WRITE)
		return s_win_openf(filename, GENERIC_WRITE, CREATE_ALWAYS);
	else if (rw_flag == READ)
		return s_win_openf(filename, GENERIC_READ, OPEN_EXISTING);
	
	return INVALID_FILE;
}

// Execute a command via the host machine.
void s_exec_cmd(Server_map* const s_map) 
{
	char cmd_buf[8+BUFLEN] = "cmd /C ";
	strcat(cmd_buf, s_map->buf);
	s_win_cp(NULL, cmd_buf);
	fputc('\n', stdout);
}

// Terminate the console application and server.
void s_terminate_console(Server_map* const s_map) 
{
	// Quit accepting connections.
	TerminateThread(s_map->acp_thread, 0);

	// if there's any connections close them before exiting.
	if (s_map->clients_sz) 
	{
		for (size_t i = 0; i < s_map->clients_sz; i++)
			closesocket(s_map->clients[i].sock);

		// Free allocated memory.
		free(s_map->clients);
	}

	// Stop accepting connections.
	terminate_server(s_map->listen_socket, NULL);
}

// Thread to recursively accept connections.
DWORD WINAPI accept_conns_thread(LPVOID* lp_param) 
{
	// Call sakito wrapper function to accept incoming connections.
	Server_map* const s_map = (Server_map*)lp_param;

	// Create our socket object.
	s_map->listen_socket = create_socket();

	// Bind socket to port.
	bind_socket(s_map->listen_socket);

	// Call wrapper function to accept incoming connections.
	s_accept_conns(s_map);

	return FAILURE;
}

// Initialization API of the console application and server.
void s_init(Server_map* const s_map) 
{
	// Mutex lock for preventing race conditions.
	s_map->ghMutex = CreateMutex(NULL, FALSE, NULL);

	// Begin accepting connections.
	s_map->acp_thread = CreateThread(0, 0, accept_conns_thread, s_map, 0, 0);
}
