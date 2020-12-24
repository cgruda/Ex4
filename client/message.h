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

#ifndef __MESSAGES_H__
#define __MESSAGES_H__

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */

#define MSG_TIMEOUT_SEC_DEFAULT      15
#define MSG_TIMEOUT_SEC_MAX          30
#define MSG_MAX_PARAMS               4

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

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
 * @return err_value code
 ******************************************************************************
 */
int send_msg(int skt, struct msg *p_msg);

/**
 ******************************************************************************
 * @brief recieve message on socket
 * @param skt socket
 * @param timout_sec maximum time in seconds to wait for message to arrive
 * @return pointer to allocated message, NULL on error or timeout
 ******************************************************************************
 */
struct msg *recv_msg(int skt, int timeout_sec);

#endif // __MESSAGES_H__
