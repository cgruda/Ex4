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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */

#define DBG_ENABLE           1
#define ARGC                 4
#define MAX_USERNAME_LEN     20
#define MAX_PORT             65536

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
#define __FILENAME__   (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define DBG_STAMP()     printf("[%-14s;%-3d] ", __FILENAME__, __LINE__)

// print error message
#define PRINT_ERROR(err_val)   do {DBG_STAMP(); print_error((err_val));} while (0)

// for debuging
#if DBG_ENABLE
#define DBG_PRINT(...)  do {DBG_STAMP(); printf(__VA_ARGS__);} while (0)
#define DBG_FUNC_STAMP()  do {DBG_STAMP(); printf("$$$ %s\n", __func__);} while (0)

#else
#define DBG_PRINT(...)
#define DBG_FUNC_STAMP()
#endif

/*
 ==============================================================================
 * STRUCTURES
 ==============================================================================
 */

struct client_env
{
	char  *serv_ip;
	unsigned short serv_port;
	char  *username;

	int skt;
	bool connected;
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
 * @brief print error message to stdin
 * @param err_val (enum err_val)
 ******************************************************************************
 */
void print_error(int err_val);

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
 * @brief TODO:
 * @param 
 * @return
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

#endif // __CLIENT_TASKS_H__
