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
#include <stdint.h>

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */
// debug prints enable
#define DBG_ENABLE     1

// input arguments count
#define ARGC           4

// for debug use
#define __FILENAME__   (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// wating times
#define MAX_WAIT_TIME_ALL_MS (10 * MIN2SEC * SEC2MS)
#define SEC2MS               1000
#define MIN2SEC              60

#define MAX_USERNAME_LEN     20
#define MAX_PORT             65536

#define TIMEOUT_SEC_DEFAULT      15
#define TIMEOUT_SEC_MAX          30

// user interface prints
#define UI_CONNECT_FAIL "Failed connecting to server on %s:%d.\n"
#define UI_CONNECT_DENY "Server on %s:%d denied the connection request\n"
#define UI_GAME_START   "Game is on!\n"
#define UI_GAME_CHOOSE  "Choose your 4 digits:\n"
#define UI_GAME_GUESS   "Choose your guess:\n"
#define UI_GAME_STAGE   "Bulls: %d\n" "Cows: %d\n" "%s played: %d\n"
#define UI_GAME_WIN     "%s won!\n" "opponent number was %d\n"
#define UI_GAME_DRAW    "It's a tie\n"
#define UI_GAME_STOP    "Opponent quit\n"
#define UI_MENU_CONNECT "Choose what to do next:\n" "1. Try to reconnect\n" "2. Exit\n"
#define UI_MENU_PALY    "Choose what to do next:\n" "1. Play against another client\n" "2. Quit\n"

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */
enum err_value
{
	// general error vals
	E_SUCCESS = 0,
	E_FAILURE,
	// specific error vals
	E_INTERNAL,
	E_TIMEOUT,
	E_STDLIB,
	E_WINAPI,
	E_WINSOCK,
};

enum msg_type
{
	// cleint messages
	MSG_CLIENT_REQUEST,
	MSG_CLIENT_VERSUS,
	MSG_CLIENT_SETUP,
	MSG_CLIENT_PLAYER_MOVE,
	MSG_CLIENT_DISCONNECT,
	// server mesgaes
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
	char *serv_ip;
	uint16_t serv_port;
	char *user_name;
};

struct client_env
{
	struct args args;
	int cnct_skt;
	FD_SET read_fds;
	SOCKADDR_IN server;
	char *username;
};


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
int check_input(struct client_env *p_env, int argc, char** argv);
int client_init(struct client_env *p_env);
int client_cleanup(struct client_env *p_env);
void print_msg(struct msg *p_msg);
void free_msg(struct msg **p_p_msg);
struct msg *parse_buff_2_msg(char *buff);
struct msg *recv_msg(int skt, int timeout_sec);


#endif // __CLIENT_TASKS_H__
