/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client side
 *
 * server_fsm.h
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#ifndef __SERVER_FSM_H__
#define __SERVER_FSM_H__

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

enum server_fsm {
	SERVER_FSM_EXIT,
	SERVER_FSM_CONNECT,
	SERVER_FSM_ABORT,
	SERVER_FSM_DISCONNECT,
	SERVER_FSM_APPROVE,
	SERVER_FSM_DENY,
	SERVER_FSM_MENU,
	SERVER_FSM_CLEANUP,
	SERVER_FSM_GAME_REQ,
	SERVER_FSM_INVITE,
	SERVER_FSM_NO_OPP,
	SERVER_FSM_PLAY_MOVE,
	SERVER_FSM_OPP_QUIT,
	SERVER_FSM_MAX
};

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

int(*server_fsm[SERVER_FSM_MAX])(struct client *p_clnt);

/**
 ******************************************************************************
 * @brief thread for handling new TCP connections into server.
 *        thread executs the server_fsm (finite state machine) which states
 *        are defined in enum server_fsm
 * @param param pointer to client struct
 * @return returns last_err as recoreded in client struct
 ******************************************************************************
 */
DWORD WINAPI client_thread(LPVOID param);


#endif // __SERVER_FSM_H__
