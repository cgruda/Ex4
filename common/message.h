/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client program
 *
 * messages.h
 * 
 * this is the header file of the messages module
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#include "tasks.h"

#ifndef __MESSAGES_H__
#define __MESSAGES_H__

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */

#define MSG_TIMEOUT_INFINITE         -1
#define MSG_TIMEOUT_SEC_DEFAULT      15
#define MSG_TIMEOUT_SEC_MAX          30
#define MSG_TIMOUT_SEC_HUMAN_MAX     900 // 15 mins
#define MSG_MAX_PARAMS               4
#define MSG_TIME_INCERMENT_SEC       0  // 0 sec
#define MSG_TIME_INCERMENT_USEC      (50 * MS2US)  // 50 msec
/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

enum msg_type
{
	MSG_INVALID,
	MSG_MIN,
	MSG_CLIENT_REQUEST = MSG_MIN,
	MSG_CLIENT_VERSUS,
	MSG_CLIENT_SETUP,
	MSG_CLIENT_PLAYER_MOVE,
	MSG_CLIENT_DISCONNECT,
	MSG_SERVER_MAIN_MENU,
	MSG_SERVER_APPROVED,
	MSG_SERVER_DENIED,
	MSG_SERVER_INVITE,
	MSG_SERVER_SETUP_REQUEST,
	MSG_SERVER_PLAYER_MOVE_REQUEST,
	MSG_SERVER_GAME_RESULTS,
	MSG_SERVER_WIN,
	MSG_SERVER_DRAW,
	MSG_SERVER_NO_OPONENTS,
	MSG_SERVER_OPPONENT_QUIT,
	MSG_MAX
};

/*
 ==============================================================================
 * STRUCTURES
 ==============================================================================
 */

struct msg
{
	int type;
	int param_cnt;
	char *param_lst[MSG_MAX_PARAMS];
};

/*
 ==============================================================================
 * FUNCTIONS DECLARATION
 ==============================================================================
 */

/**
 ******************************************************************************
 * @brief print messgae in human readable format to stdout
 * @param p_msg pointer to message
 ******************************************************************************
 */
void print_msg(struct msg *p_msg);

/**
 ******************************************************************************
 * @brief allocate new message and set params
 * @param type of message
 * @param p0-p3 pointers to param0-3 strings
 * @return pointer to new allocated message, NULL on failure
 ******************************************************************************
 */
struct msg *new_msg(int type, char *p0, char *p1, char *p2, char *p3);

/**
 ******************************************************************************
 * @brief free message allocated memory
 * @param p_p_msg pointer to message pointer, message pointer is set to NULL
 ******************************************************************************
 */
void free_msg(struct msg **p_p_msg);

/**
 ******************************************************************************
 * @brief send message on socket
 * @param skt socket
 * @param p_msg pointer to message struct
 * @return E_SUCCESS - all good
 *         E_FAILURE - bad input to function
 *         E_STDLIB  - mem allocation error
 *         E_WINSOCK - socket error
 ******************************************************************************
 */
int send_msg(int skt, struct msg **p_p_msg);

/**
 ******************************************************************************
 * @brief recieve message on socket
 * @param p_p_msg pointer to message pointer to hold recieved message,
 * @param skt socket
 * @param timout_sec maximum time in seconds to wait for message to arrive
 * @return E_SUCESS  - all good                - need to free msg
 *         E_MESSAGE - message was corrupted   - no need to free msg
 *         E_TIMEOUT - timeout waiting for msg - no need to free msg
 *         E_STDLIB  - mem error               - no need to free msg
 *         E_WINSOCK - socket error            - no need to free msg
 ******************************************************************************
 */
int recv_msg(struct msg **p_p_msg, int skt, PTIMEVAL p_timeout);


#if DBG_TRACE
char *dbg_trace_msg(struct msg *p_msg);
#endif


#endif // __MESSAGES_H__
