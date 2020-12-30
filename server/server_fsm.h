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

#ifndef __SERVER_FSM_H__
#define __SERVER_FSM_H__

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

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

enum state_clnt
{
	STATE_THREAD_EXIT,
	STATE_CONNECT,
	STATE_CONNECT_FAILURE,
	STATE_CONNECT_SUCCESS,
	STATE_CONNECT_DENIED,
	STATE_ABORT_THREAD,
	STATE_DISCONNECT,
	STATE_CONNECT_APPROVE,
	STATE_CONNECT_DENY,
	STATE_MAIN_MENU,
	STATE_THREAD_CLEANUP,
	STATE_ASK_FOR_GAME,
	STATE_GAME_INVITE,
	STATE_NO_OPPONENTS,
	STATE_MAX
};


/*
 ==============================================================================
 * MACROS
 ==============================================================================
 */

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

int(*server_fsm[STATE_MAX])(struct client *p_clnt);

/**
 ******************************************************************************
 * @brief TODO:
 * @param 
 * @param 
 * @return 
 ******************************************************************************
 */
DWORD WINAPI clnt_thread(LPVOID param);


#endif // __SERVER_FSM_H__
