/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client side
 *
 * client_tasks.h
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#ifndef __CLIENT_TASKS_H__
#define __CLIENT_TASKS_H__

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <windows.h>
#include <stdbool.h>

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */

#define CLIENT_ARGC          4
#define MAX_USERNAME_LEN     20

/*
 ==============================================================================
 * STRUCTURES
 ==============================================================================
 */

struct client_env {
	int skt;
	char  *server_ip;
	USHORT server_port;
	char *username;
	bool approved;
	SOCKADDR_IN server;
	int last_error;
};

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

/**
 ******************************************************************************
 * @brief check input arguments to program, and set env struct
 * @param p_env pointer to env
 * @param argc from main
 * @param argv from main
 * @return E_SUCCESS or E_FAILURE
 ******************************************************************************
 */
int check_input(struct client_env *p_env, int argc, char** argv);

/**
 ******************************************************************************
 * @brief initialize client resources
 * @param p_env pointer to env
 * @return STATE_EXIT on failure, STATE_CONNECT_ATTEMPT on success
 ******************************************************************************
 */
int client_init(struct client_env *p_env);

/**
 ******************************************************************************
 * @brief cleanup client resources
 * @param p_env pointer to env
 * @return E_SUCCESS on success
 ******************************************************************************
 */
int client_cleanup(struct client_env *p_env);

/**
 ******************************************************************************
 * @brief wrapper for creating and sending a message from the client
 * @param p_env pointer to client env
 * @param type of message to be sent
 * @param param to be sent (client sends single parameter)
 * @return E_SUCCESS - all good
 *         E_FAILURE - bad input to function
 *         E_STDLIB  - mem allocation error
 *         E_WINSOCK - socket error
 ******************************************************************************
 */
int cilent_send_msg(struct client_env *p_env, int type, char *param);

/**
 ******************************************************************************
 * @brief wrapper for recieving a message from the server
 * @param p_env pointer to client env
 * @param p_p_msg pointer to message pointer to hold recieved message
 * @param timeout_sec
 * @return E_SUCESS  - all good                - need to free msg
 *         E_MESSAGE - message was corrupted   - no need to free msg
 *         E_TIMEOUT - timeout waiting for msg - no need to free msg
 *         E_STDLIB  - mem error               - no need to free msg
 *         E_WINSOCK - socket error            - no need to free msg
 ******************************************************************************
 */
int client_recv_msg(struct msg **p_p_msg, struct client_env *p_env, int timeout_sec);


#endif // __CLIENT_TASKS_H__
