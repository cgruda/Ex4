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

#define _WINSOCK_DEPRECATED_NO_WARNINGS // FIXME:

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

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
#define ARGC                         3

// 2 clients can be approved
#define MAX_PLAYERS                  2

// allow 3 threads to be created
#define MAX_CONNECTIONS              3
#define THREAD_BITMAP_INIT_MASK      0xFFFFFFF8

// standard input value
#define PATH_STDIN                   "CONIN$"
#define PATH_GAME_SESSION            "GameSession.txt"

#define FILE_SHARE_ALL     (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
#define GENERIC_READ_WRITE (GENERIC_READ | GENERIC_WRITE)

/*
 ==============================================================================
 * STRUCTURES
 ==============================================================================
 */

struct game
{
	bool valid;
	HANDLE h_play_evt[2];
	HANDLE h_game_session_file;
	HANDLE h_game_mtx;
	HANDLE h_game_abort_avt;
};

struct client
{
	int id;
	int last_err;
	int skt;
	struct serv_env *p_env;
	char *username;
	char *opponent_username;
	bool connected;
	bool playing;
	int game_session_offset;
	HANDLE *play_evt;
	HANDLE h_game_session_file;
	int position;
};

struct serv_env
{
	// server handling params
	WSADATA	wsa_data;
	int serv_skt;
	char *serv_ip;
	USHORT serv_port;
	SOCKADDR_IN server;

	// control params
	HANDLE h_players_smpr;
	HANDLE h_abort_evt;
	HANDLE h_game_file;
	HANDLE h_game_mtx;
	HANDLE h_file_stdin;
	OVERLAPPED olp_stdin;
	char buffer[7]; // FIXME:

	// clients
	DWORD thread_bitmap;
	struct client client[MAX_CONNECTIONS];
	HANDLE h_clnt_thread[MAX_CONNECTIONS];

	struct game game;

	int last_err;
};

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

/**
 ******************************************************************************
 * @brief check program input
 * @param p_env pointer to server enviroment
 * @return E_SUCCESS on success
 ******************************************************************************
 */
int check_input(struct serv_env *p_env, int argc, char **argv);

/**
 ******************************************************************************
 * @brief check if exit command was given
 * @param p_env pointer to server env
 * @return true  - got exit or encountered errors (see p_env->last_err)
 *         false - otherwise
 ******************************************************************************
 */
bool server_quit(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief initilize server resources and start logic that checks exit command
 * @param p_env pointer to server env
 * @return E_SUCCESS on success
 ******************************************************************************
 */
int server_init(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief clenup all server resources
 * @param p_env pointer to server env
 * @return last error from p_env->last_err
 ******************************************************************************
 */
int server_cleanup(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief check if abort event was set by main thread
 * @param p_env pointer to server env
 * @return true or false
 ******************************************************************************
 */
bool server_check_abort(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief TCP connection entrance function: listens for new connections and
 *        opens new thread if there are available resources.
 * @param p_env pointer to server env
 * @return E_SUCESS on success
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
int server_send_msg(struct client *p_clnt, int type,
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
int server_recv_msg(struct client *p_clnt, struct msg **p_p_msg, int timeout_sec);

/**
 ******************************************************************************
 * @brief destroy threads handling clients one by one
 * @param p_env pointer to server enviroment
 * @return E_SUCCESS on success
 ******************************************************************************
 */
int server_destroy_clients(struct serv_env *p_env);

/**
 ******************************************************************************
 * @brief check if a thread has ended, and if it did free its resources to
 *        allow new connections to utilize resources
 * @param p_env pointer to server enviroment
 * @return E_SUCCESS on success
 ******************************************************************************
 */
int server_check_thread_status(struct serv_env *p_env, int ms);

/**
 ******************************************************************************
 * @brief TODO:
 * @param p_clnt pointer to client object
 * @return E_SUCCESS on success
 ******************************************************************************
 */
int game_session_start(struct client *p_clnt);
int session_sequece(struct client *p_clnt, char *buffer);
int game_session_end(struct client *p_clnt);


#endif // __SERVER_TASKS_H__
