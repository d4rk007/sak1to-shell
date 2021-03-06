/* 
Coded by d4rkstat1c.
Use educationally/legally.
*/
#ifndef SAKITO_SERVER_TOOLS_H
#define SAKITO_SERVER_TOOLS_H

#define MEM_CHUNK 5
#define INVALID_CLIENT_ID -1
#define BACKGROUND -100
#define FILE_NOT_FOUND 1

/*

Below contains the various header files which link various sakito-API functions which will be compiled conditionally based on the operating system,
currently supports only linux and windows systems.  The APIs have a matching signature allowing for cross-platform compilation.

*/
#ifdef OS_WIN
	#include "sakito_swin_utils.h"
#elif defined OS_LIN
	#include "sakito_slin_utils.h"
#endif

// Typedef for function pointer for console functions.
typedef void (*console_func)(Server_map *s_map);

// Typedef for function pointer for server functions.
typedef int (*server_func)(char*, size_t, SOCKET);

/*
Below are functions related to string parsing and IO.
*/

// Function to validate client identifier prior to interaction.
int validate_id(Server_map* const s_map) 
{
	int client_id;
	client_id = atoi(s_map->buf+9);

	if (!s_map->clients_sz || client_id < 0 || client_id > s_map->clients_sz - 1)
		return INVALID_CLIENT_ID;

	return client_id;
}

// Function to compare two strings (combined logic of strcmp and strncmp).
int compare(const char* buf, const char* str) 
{
	while (*str)
		if (*buf++ != *str++)
			return 0;

	return 1;
}

// Function to read/store stdin in buffer until \n is detected.
void get_line(char* const buf, size_t *cmd_len) 
{
	char c;

	while ((c = getchar()) != '\n' && *cmd_len < BUFLEN)
		buf[(*cmd_len)++] = c;
}

// Function to return function pointer based on parsed command.
void* parse_cmd(char* const buf, size_t *cmd_len, int cmds_len, const char commands[5][11], void** func_array, void* default_func) 
{
	get_line(buf, cmd_len);

	if (*cmd_len > 1)
		// Parse stdin string and return its corresponding function pointer.
		for (int i = 0; i < cmds_len; i++)
			if (compare(buf, commands[i]))
				return func_array[i];

	return default_func;
}

// Function to copy uint64_t bytes to new memory block/location to abide strict aliasing.
static inline uint32_t ntohl_conv(char* const buf) 
{
	uint32_t new;
	memcpy(&new, buf, sizeof(new));

	// Return deserialized bytes.
	return ntohl(new);
}

#endif
