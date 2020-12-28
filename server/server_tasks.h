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

// input arguments count
#define ARGC           3


#define MAX_PLAYERS 2

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

/*
 ==============================================================================
 * MACROS
 ==============================================================================
 */

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
	char *username;
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

/**
 ******************************************************************************
 * @brief wrapper for receiving message from client, this wrapper splits
 *        the timeout into MSG_TIME_INCERMENT to allow checking if thread
 *        got abort from main thread.
 * @param p_clnt pointer to client object
 * @param p_p_msg pointer to message pointer to get message
 * @param timeout sec timeout in seconds
 * @return E_SUCCESS - all good
 *         E_FAILURE - bad input to function
 *         E_STDLIB  - mem allocation error
 *         E_WINSOCK - socket error
 ******************************************************************************
 */
int server_recv_msg(struct clnt_args *p_clnt, struct msg **p_p_msg, int timeout_sec);



int server_destroy_clients(struct serv_env *p_env);



#endif // __SERVER_TASKS_H__
