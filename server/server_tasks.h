/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * server side
 *
 * client_tasks.h
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#ifndef __SERVER_TASKS_H__
#define __SERVER_TASKS_H__

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */
// debug prints enable
#define DBG_ENABLE     1

// input arguments count
#define ARGC           3

// for debug use
#define __FILENAME__   (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// wating times
#define MAX_WAIT_TIME_ALL_MS (10 * MIN2SEC * SEC2MS)
#define SEC2MS               1000
#define MIN2SEC              60

#define MAX_PORT             65536


#define MAX_PLAYERS 2
#define MSG_MAX_PARAMS 4

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

enum err_value
{
	E_SUCCESS = 0,
	E_FAILURE,
	E_INTERNAL,
	E_TIMEOUT,
        E_MESSAGE,
	E_STDLIB,
	E_WINAPI,
	E_WINSOCK,
};

/*
 ==============================================================================
 * MACROS
 ==============================================================================
 */
// debug stamp [file;line]
#define DBG_STAMP()     printf("[%-9s;%-3d] ", __FILENAME__, __LINE__)


// for debuging
#if DBG_ENABLE
#define DBG_PRINT(...)  do {DBG_STAMP(); printf(__VA_ARGS__);} while (0)
#define DBG_FUNC_STAMP()  do {DBG_STAMP(); printf("$$$ %s\n", __func__);} while (0)
#else
#define DBG_PRINT(...)
#endif

// print error message
#define PRINT_ERROR(err_val)   do {DBG_STAMP(); print_error((err_val));} while (0)


/*
 ==============================================================================
 * STRUCTURES
 ==============================================================================
 */

struct args
{
	char *server_ip;
	uint16_t server_port;
};

struct player
{
	int id;
	int clnt_skt;
	HANDLE h_thread;
	bool wating_play;
};

struct server_env
{
	// input args
	struct args args;
	// quit server params
	HANDLE h_file_stdin;
	OVERLAPPED olp_stdin;
	char buffer[7];
	// server handling params
	int serv_skt;
	SOCKADDR_IN server;
	// client handling
	struct player player_db[MAX_PLAYERS];
	int player_cnt;


	// temp
	HANDLE h_clnt_thread;


	//FD_SET read_fds;
};

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */
/**
 ******************************************************************************
 * @brief 
 * @param 
 * @param 
 * @return 
 ******************************************************************************
 */

int check_input(struct server_env* p_env, int argc, char** argv);
int my_atoi(char *str, int *p_result);
bool server_quit(struct server_env *p_env);
int server_init(struct server_env *p_env);
int server_cleanup(struct server_env *p_env);
int server_accept_client(struct server_env *p_env);
void print_error(int err_val);




#endif // __SERVER_TASKS_H__
