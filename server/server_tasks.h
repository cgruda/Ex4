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

#include <windows.h>
#include <stdbool.h>
#include "game.h"

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */

// input arguments count
#define SERVER_ARGC                  3

// allow 3 threads to be created
#define MAX_CONNECTIONS              3
#define THREAD_BITMAP_INIT_MASK      0xFFFFFFF8

// stdin path
#define PATH_STDIN                   "CONIN$"

/*
 ==============================================================================
 * STRUCTURES
 ==============================================================================
 */

struct client {
	int skt;
	bool connected;
	bool playing;
	char *username;
	char *opp_username;
	int   opp_pos;
	char setup_numbers[5];
	HANDLE *p_h_play_evt;
	int last_err;
	struct serv_env *p_env;
};

struct serv_env {
	WSADATA	wsa_data;
	int server_skt;
	char *server_ip;
	USHORT server_port;
	SOCKADDR_IN server;
	HANDLE h_client_approve_smpr;
	HANDLE h_abort_evt;
	HANDLE h_stdin;
	OVERLAPPED olp_stdin;
	char buffer[7]; // FIXME: use define
	DWORD thread_bitmap;
	struct client client[MAX_CONNECTIONS];
	HANDLE h_client_thread[MAX_CONNECTIONS];
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
 * @param p_client pointer to client object
 * @param type of message to be sent
 * @param p0-3 parameters to be sent
 * @return E_SUCCESS - all good
 *         E_FAILURE - bad input to function
 *         E_STDLIB  - mem allocation error
 *         E_WINSOCK - socket error
 ******************************************************************************
 */
int server_send_msg(struct client *p_client, int type,
	char *p0, char *p1, char *p2, char *p3);

/**
 ******************************************************************************
 * @brief wrapper for receiving message from client, this wrapper splits
 *        the timeout into MSG_TIME_INCERMENT to allow checking if thread
 *        got abort from main thread.
 * @param p_client pointer to client object
 * @param p_p_msg pointer to message pointer to get message
 * @param timeout sec timeout in seconds
 * @return E_SUCCESS - all good
 *         E_FAILURE - bad input to function
 *         E_STDLIB  - mem allocation error
 *         E_WINSOCK - socket error
 ******************************************************************************
 */
int server_recv_msg(struct client *p_client, struct msg **p_p_msg, int timeout_sec);

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


#endif // __SERVER_TASKS_H__
