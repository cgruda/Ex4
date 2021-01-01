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

#ifndef __CLIENT_FLOW_H__
#define __CLIENT_FLOW_H__

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */

// user interface strings
#define UI_CONNECTED    "\nConnected to server on %s:%d\n"
#define UI_CONNECT_FAIL "\nFailed connecting to server on %s:%d.\n"
#define UI_CONNECT_DENY "\nServer on %s:%d denied the connection request\n"
#define UI_GAME_START   "\nGame is on!\n"
#define UI_GAME_CHOOSE  "\nChoose your 4 digits:\n"
#define UI_GAME_GUESS   "\nChoose your guess:\n"
#define UI_GAME_STAGE   "\nBulls: %s\n" "Cows: %s\n" "%s played: %s\n"
#define UI_GAME_WIN     "\n%s won!\n" "opponent number was %s\n"
#define UI_GAME_DRAW    "\nIt's a tie\n"
#define UI_GAME_STOP    "\nOpponent quit\n"
#define UI_MENU_CONNECT "\nChoose what to do next:\n" "1. Try to reconnect\n" "2. Exit\n"
#define UI_MENU_PLAY    "\nChoose what to do next:\n" "1. Play against another client\n" "2. Quit\n"

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

enum state_clnt
{
	STATE_EXIT,
	STATE_CONNECT_ATTEMPT,
	STATE_CONNECT_FAILURE,
	STATE_CONNECT_APPROVED,
	STATE_CONNECT_DENIED,
	STATE_DISCONNECT,
	STATE_UNDEFINED_FLOW,
	STATE_MAIN_MENU,
	STATE_RECONNECT_MENU,
	STATE_CLIENT_REQUEST,
	STATE_ASK_FOR_GAME,
	STATE_INVITE_AND_SETUP,
	STATE_GAME_PLAY,
	STATE_MAX
};

enum user_choice
{
	CHOICE_INVALID,
	/* connect menu */
	CHOICE_RECONNECT = 1,
	CHOICE_EXIT      = 2,
	/* play menu */
	CHOICE_PLAY      = 1,
	CHOICE_QUIT      = 2,
};

/*
 ==============================================================================
 * MACROS
 ==============================================================================
 */

#define UI_PRINT           printf_s
#define UI_GET(p_choice)   scanf_s("%d", p_choice)

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

int(*clnt_flow[STATE_MAX])(struct client_env *p_env);

/**
 ******************************************************************************
 * @brief 
 * @param 
 * @param 
 * @return 
 ******************************************************************************
 */
int flow_clnt_connect_attempt(struct client_env *p_env);
int flow_clnt_connect_failure(struct client_env *p_env);


#endif // __CLIENT_FLOW_H__
