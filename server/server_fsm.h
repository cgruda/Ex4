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
	STATE_CONNECT_ATTEMPT,
	STATE_CONNECT_FAILURE,
	STATE_CONNECT_SUCCESS,
	STATE_CONNECT_DENIED,
	STATE_UNDEFINED_FLOW,
	STATE_DISCONNECT,
	STATE_CONNECT_APPROVE,
	STATE_CONNECT_DENY,
	STATE_MAIN_MENU,
	STATE_THREAD_CLEANUP,
	STATE_ABORT_THREAD,
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

int(*server_fsm[STATE_MAX])(struct clnt_args *p_clnt);

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
