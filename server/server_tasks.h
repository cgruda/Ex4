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

enum err_val
{
	E_SUCCESS = 0,
	E_FAILURE,
	E_INTERNAL,
	E_TIMEOUT,
	E_MESSAGE,
	E_INPUT,
	E_STDLIB,
	E_WINAPI,
	E_WINSOCK,
	E_FLOW,
	E_MAX
};

/*
 ==============================================================================
 * MACROS
 ==============================================================================
 */
// debug stamp [file;line]
#define DBG_STAMP()     printf("[%-14s;%-3d] ", __FILENAME__, __LINE__)


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

struct clnt_args
{
	int last_err;
	int skt;
	struct serv_env *p_env;
	bool connected;
	bool playing;
};

// struct player
// {
// 	int id;
// 	int skt;
// 	HANDLE h_thread;
// 	bool wating_play;
// };

struct serv_env
{
	// quit server params
	HANDLE h_file_stdin;
	OVERLAPPED olp_stdin;
	char buffer[7];

	// server handling params
	WSADATA	wsa_data;
	char *serv_ip;
	unsigned short serv_port;
	SOCKADDR_IN server;
	int serv_skt;

	// control params
	HANDLE h_players_smpr;
	HANDLE h_abort_evt;

	// clients
	HANDLE h_clnt_thread; // FIXME: need to support more tham 1. possibly use realloc
	int clnt_cnt;
	// client handling
	// struct player player_db[MAX_PLAYERS];
	// int player_cnt;

	int last_err;
};

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

/**
 ******************************************************************************
 * @brief TODO:
 * @param 
 * @return 
 ******************************************************************************
 */
int check_input(struct serv_env *p_env, int argc, char **argv);

/**
 ******************************************************************************
 * @brief TODO:
 * @param 
 * @return 
 ******************************************************************************
 */
int my_atoi(char *str, int *p_result);

/**
 ******************************************************************************
 * @brief check if exit command was given, and if did - abort all threads
 * @param p_env pointer to server env
 * @return true  - got exit or encountered errors (see p_env->last_err)
 *         false - otherwise
 ******************************************************************************
 */
bool server_quit(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief TODO:
 * @param 
 * @return 
 ******************************************************************************
 */
int server_init(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief TODO:
 * @param 
 * @return 
 ******************************************************************************
 */
int server_cleanup(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief TODO:
 * @param 
 * @return 
 ******************************************************************************
 */
bool server_check_abort(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief TODO:
 * @param 
 * @return 
 ******************************************************************************
 */
int serv_clnt_connect(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief print error message to stdin
 * @param err_val (enum err_val)
 ******************************************************************************
 */
void print_error(int err_val);

/**
 ******************************************************************************
 * @brief wrapper for creating and sending message from server to client
 * @param p_clnt pointer to client object
 * @param type of message to be sent
 * @param p0-3 parameters to be sent
 * @return E_SUCCESS - all good
 *         E_FAILURE - bad input to function
 *         E_STDLIB  - mem allocation error
 *         E_WINSOCK - socket error
 ******************************************************************************
 */
int server_send_msg(struct clnt_args *p_clnt, int type,
	char *p0, char *p1, char *p2, char *p3);

#endif // __SERVER_TASKS_H__
