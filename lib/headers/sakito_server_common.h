#ifndef SAKITO_SERVER_FUNCS_H
#define SAKITO_SERVER_FUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "os_check.h"
#define SERVER
#ifdef OS_WIN
#include <WS2tcpip.h>
#include <Windows.h>
#include <inttypes.h>
#include "windows/sakito_swin_types.h"
#pragma comment(lib, "ws2_32.lib")
#elif defined OS_LIN
#include <unistd.h>
#include <netdb.h>
#include "linux/sakito_slin_types.h"
#endif
#include "sakito_core_funcs.h"
#include "sakito_multi_def.h"
void terminate_server(const SOCKET listen_socket, const char* const error);

void s_closesocket(SOCKET socket);

void s_mutex_lock(Server_map* const s_map);

void s_mutex_unlock(Server_map* const s_map);

void s_read_stdin(char* const buf, size_t* cmd_len);

int s_validate_id(Server_map* const s_map);

char s_compare(const char* buf, const char* str);

void* s_parse_cmd(char* const buf,
				size_t *cmd_len,
				const char commands[][11],
				void** func_array,
				void* default_func);

void s_accept_conns(Server_map* const s_map);

void s_delete_client(Server_map* const s_map, const int client_id);

void get_cwd(char* buf);

int s_tcp_send(SOCKET socket, char* buf, size_t count);

int s_tcp_recv(SOCKET socket, char* buf, size_t count);

int s_write_stdout(const char* buf, size_t count);

void s_close_file(s_file file);

void s_closesocket(SOCKET socket);

void s_mutex_lock(Server_map* const s_map);

void s_mutex_unlock(Server_map* const s_map);

s_file s_open_file(const char* filename, int rw_flag);

int s_recv_file(const SOCKET socket, s_file file, char* const buf, uint64_t f_size);

uint64_t s_file_size(s_file file);

int s_send_file(const SOCKET socket, s_file file, char* const buf, uint64_t f_size);

void s_exec_cmd(Server_map* const s_map);

void s_terminate_console(Server_map* const s_map);

void s_read_stdin(char* const buf, size_t* cmd_len);

void s_init(Server_map* const s_map);

// Function to copy uint16_t bytes to new memory block/location to abide strict aliasing.
static inline uint16_t s_ntohs_conv(char* const buf) 
{
	uint16_t uint16_new;
	memcpy(&uint16_new, buf, sizeof(uint16_t));

	// Return deserialized bytes.
	return ntohs(uint16_new);
}

#endif
