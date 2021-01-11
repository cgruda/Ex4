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

#define UI_INPUT_LEN    6

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

enum client_fsm {
	CLIENT_FSM_EXIT,
	CLIENT_FSM_CONNECT,
	CLIENT_FSM_CONNECT_FAIL,
	CLIENT_FSM_APPROVED,
	CLIENT_FSM_DENIED,
	CLIENT_FSM_DISCONNECT,
	CLIENT_FSM_UNDEFINED,
	CLIENT_FSM_MAIN_MENU,
	CLIENT_FSM_RECONNECT,
	CLIENT_FSM_REQUEST,
	CLIENT_FSM_GAME_REQ,
	CLIENT_FSM_INVITE_SETUP,
	CLIENT_FSM_GAME_MOVE,
	CLIENT_FSM_MAX
};

enum user_choice {
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

// client finite state machine functions array declaration
int(*client_fsm[CLIENT_FSM_MAX])(struct client_env *p_env);


#endif // __CLIENT_FLOW_H__
