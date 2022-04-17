#ifndef SAKITO_CORE_FUNCS_H
#define SAKITO_CORE_FUNCS_H

#include <string.h>
#include <stdint.h>
#include "nbo_encoding.h"
#include "os_check.h"

#define SUCCESS 1
#define FAILURE -1
#define EXIT_SUCCESS 0
#define FTRANSFER_START "1"


#ifdef OS_WIN
#include <ws2tcpip.h>
#include "windows/sakito_swin_types.h"
HANDLE s_win_openf(const LPCTSTR filename, const DWORD desired_access, const DWORD creation_dispostion);

uint64_t s_win_fsize(HANDLE h_file);

int s_win_sendf(const SOCKET socket, HANDLE h_file, char* const buf, uint64_t f_size);

int s_win_recvf(const SOCKET socket, HANDLE h_file, char* const buf, uint64_t f_size);

BOOL s_win_cp(HANDLE child_stdout_write, const LPSTR buf);
#elif defined OS_LIN
#include "linux/sakito_slin_types.h"
#endif

#ifdef SERVER
#define MEM_CHUNK 5
#define INVALID_CLIENT_ID -1
#define S_BACKGROUND -100
#define FILE_NOT_FOUND 1
#define INIT_CONN "1"
#define FTRANSFER_FINISHED "1"
#define RESTART_CLIENT " "
#define C_BACKGROUND "5"

// Type for function pointer for console functions.
typedef void (*console_func)(Server_map* s_map);

// Type for function pointer for server functions.
typedef int (*server_func)(char*, size_t, SOCKET);
#endif

// Function to copy uint64_t bytes to new memory block/location to abide strict aliasing.
static inline uint64_t s_ntohll_conv(char* const buf) 
{
	uint64_t uint64_new;
	memcpy(&uint64_new, buf, sizeof(uint64_new));

	// Return deserialized bytes.
	return ntohll(uint64_new);
}

#endif
