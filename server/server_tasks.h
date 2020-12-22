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

// error messages
#define E_MSG_NONE NULL
#define E_MSG_NULL_PTR "Null Pointer"
#define E_MSG_MAX_WAIT "Max Wait-time passed"
#define E_MSG_BUF_FULL "Buffer is full"
#define E_MSG_INPT_ERR "Input file has bad values"

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
	E_STDLIB,
	E_WINAPI,
	E_WINSOCK,
};

enum msg_type
{
	/* client messages */
	MSG_CLIENT_REQUEST,
	MSG_CLIENT_VERSUS,
	MSG_CLIENT_SETUP,
	MSG_CLIENT_PLAYER_MOVE,
	MSG_CLIENT_DISCONNECT,
	/* server messgaes */
	MSG_SERVER_MAIN_MENUE,
	MSG_SERVER_APPROVED,
	MSG_SERVER_DENIED,
	MSG_SERVER_INVITE,
	MSG_SERVER_SETUP_REQUEST,
	MSG_SERVER_PLAYER_MOVE_REQUEST,
	MSG_SERVER_WIN,
	MSG_SERVER_DRAW,
	MSG_SERVER_NO_OPONENTS,
	MSG_SERVER_OPPONENT_QUIT,
	MSG_MAX
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

struct server_env
{
	struct args args;
	
	// for exit sync from stdin
	HANDLE h_file_stdin;
	OVERLAPPED olp_stdin;
	char buffer[7];

	int skt;
	//FD_SET read_fds;
	SOCKADDR_IN server;
	char *username;
};

#define MAX_PLAYERS 2
#define MAX_PARAM 4

struct msg
{
	int type;
	int param_cnt;
	char *param_lst[MAX_PARAM];
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
bool check_server_exit(HANDLE* p_h_stdin);

int check_input(struct server_env* p_env, int argc, char** argv);
int my_atoi(char *str, int *p_result);
void print_error(int err_val);
bool server_exit_test(struct server_env *p_env);
int server_init(struct server_env *p_env);
int server_cleanup(struct server_env *p_env);

#endif // __SERVER_TASKS_H__
