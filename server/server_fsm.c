/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client side
 *
 * client_tasks.c
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */
#define _WINSOCK_DEPRECATED_NO_WARNINGS // FIXME:
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "Shlwapi.lib")


/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <stdio.h>
#include <assert.h>
#include "Shlwapi.h"
#include "server_tasks.h"
#include "server_fsm.h"
#include "message.h"
#include "tasks.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

DWORD WINAPI clnt_thread(LPVOID param)
{
	struct client *p_clnt = NULL;
	struct serv_env  *p_env = NULL;
	struct msg *p_msg = NULL;
	int state = STATE_CONNECT;

	p_clnt = (struct client*)param;
	p_env = p_clnt->p_env;

	/* thread state machine */
	while (state != STATE_THREAD_EXIT) {

		/* check for abort signal from main thread */
		if (server_check_abort(p_env))
			state = STATE_ABORT_THREAD;

		/* preform state and get next state */
		state = (*server_fsm[state])(p_clnt);
	}

	/* thread cleanup is done at fsm_thread_cleanup,
	 * which is the only state that calls exit */
	
	ExitThread((DWORD)p_clnt->last_err);
}


int flow_serv_abort(struct client *p_clnt)
{
	if (p_clnt->username)
		DBG_TRACE_FUNC(T, p_clnt->username);

	if (p_clnt->username && (p_clnt->last_err != E_SUCCESS))
		DBG_TRACE_STR(T, p_clnt->username, "abort due to error: 0x%X", p_clnt->last_err);
	
	if (p_clnt->connected)
		return STATE_DISCONNECT;
	else
		return STATE_THREAD_CLEANUP;
}



flow_serv_disconnect(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	assert(p_clnt->connected);

	// int res;

	if (p_clnt->playing) {
		game_session_end(p_clnt); // FIXME: another look needed
	}

	if (!ReleaseSemaphore(p_clnt->p_env->h_players_smpr, 1, NULL)) {
		PRINT_ERROR(E_WINSOCK);
		// signal main a critical error????
	}

	p_clnt->connected = false;

	return STATE_THREAD_CLEANUP;
}


flow_serv_thread_cleanup(struct client *p_clnt)
{
	if (p_clnt->username) {
		DBG_TRACE_FUNC(T, p_clnt->username);
		free(p_clnt->username);
	}

	if (p_clnt->opponent_username)
		free(p_clnt->opponent_username);

	if (closesocket(p_clnt->skt) == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		p_clnt->last_err = E_WINSOCK;
	}

	return STATE_THREAD_EXIT;
}

flow_serv_main_menu(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	assert(p_clnt->connected);
	
	int res;
	struct msg *p_msg;
	int next_state;

	res = server_send_msg(p_clnt, MSG_SERVER_MAIN_MENU, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMOUT_SEC_HUMAN_MAX);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	switch (p_msg->type)
	{
	case MSG_CLIENT_VERSUS:
		next_state = STATE_ASK_FOR_GAME;
		break;
	case MSG_CLIENT_DISCONNECT:
		next_state = STATE_DISCONNECT;
		break;
	default:
		p_clnt->last_err = E_FLOW;
		next_state = STATE_ABORT_THREAD;
		break;
	}

	free_msg(&p_msg);
	return next_state;
}


int flow_serv_ask_for_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res, next_state;
	struct game *p_game = NULL;
	char buff[25] = {0}; // FIXME:

	/* enter a game */
	res = game_session_start(p_clnt);
	switch (res) {
	case E_SUCCESS:
		break;
	case E_TIMEOUT:
		res = game_session_end(p_clnt);
		if (res != E_SUCCESS) {
			p_clnt->last_err = res;
			return STATE_ABORT_THREAD;
		}
		return STATE_NO_OPPONENTS;
	default:
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	memcpy(buff, p_clnt->username, strlen(p_clnt->username));

	res = session_sequence(p_clnt, buff);
	switch (res) {
	case E_TIMEOUT:
		res = game_session_end(p_clnt);
		if (res != E_SUCCESS) {
			p_clnt->last_err = res;
			next_state = STATE_ABORT_THREAD;
			break;
		}
		next_state = STATE_NO_OPPONENTS;
		break;
	case E_SUCCESS:
		p_clnt->opponent_username = calloc(strlen(buff) + 1, sizeof(char));
		if (!p_clnt->opponent_username) {
			p_clnt->last_err = E_STDLIB;
			next_state = STATE_ABORT_THREAD;
			break;
		}
		memcpy(p_clnt->opponent_username, buff, strlen(buff));
		next_state = STATE_GAME_INVITE;
		break;
	default:
		p_clnt->last_err = res;
		next_state = STATE_ABORT_THREAD;
		break;
	}

	return next_state;
}


int flow_serv_invite_and_setup(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res, next_state;
	struct msg *p_msg = NULL;
	char dummy[25] = {0};

	/* send invite */
	res = server_send_msg(p_clnt, MSG_SERVER_INVITE, p_clnt->opponent_username, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	/* send setup request */
	res = server_send_msg(p_clnt, MSG_SERVER_SETUP_REQUEST, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	/* wait for client setup reponse */
	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMOUT_SEC_HUMAN_MAX);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	if (p_msg->type != MSG_CLIENT_SETUP) {
		free_msg(&p_msg);
		p_clnt->last_err = E_FLOW;
		return STATE_ABORT_THREAD;
	}

	memcpy(p_clnt->setup_numbers, p_msg->param_lst[0], 4); // FIXME:
	DBG_TRACE_STR(T, p_clnt->username, "setup: %s", p_clnt->setup_numbers);

	res = session_sequence(p_clnt, dummy); // TODO: does not work: we go to abort.
	switch (res) {
	case E_SUCCESS:
		next_state = STATE_PLAYER_MOVE;
		break;
	case E_TIMEOUT:
		next_state = STATE_OPONENT_QUIT;
		break;
	default:
		p_clnt->last_err = E_FLOW; // FIXME:
		next_state = STATE_ABORT_THREAD;
	}

	free_msg(&p_msg);
	return next_state;
}




int flow_serv_no_opponents(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res;

	res = server_send_msg(p_clnt, MSG_SERVER_NO_OPONENTS, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	return STATE_MAIN_MENU;
}


int flow_serv_connect_approve(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	assert(p_clnt->connected);
	int res;
	
	res = server_send_msg(p_clnt, MSG_SERVER_APPROVED, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	return STATE_MAIN_MENU;
}

int flow_serv_connect_deny(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	assert(p_clnt->connected);
	
	int res;
	
	res = server_send_msg(p_clnt, MSG_SERVER_DENIED, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	return STATE_THREAD_CLEANUP;
}

int flow_serv_connect(struct client *p_clnt)
{
	assert(!p_clnt->connected);
	
	struct msg *p_msg = NULL;
	int res, state;
	DWORD wait_code;

	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}
	
	if (p_msg->type != MSG_CLIENT_REQUEST) {
		free_msg(&p_msg);
		p_clnt->last_err = E_FLOW;
		return STATE_ABORT_THREAD;
	}

	p_clnt->username = calloc(strlen(p_msg->param_lst[0]) + 1, sizeof(char));
	if (!p_clnt->username) {
		PRINT_ERROR(E_STDLIB);
		free_msg(&p_msg);
		return STATE_ABORT_THREAD;
	}
	memcpy(p_clnt->username, p_msg->param_lst[0], strlen(p_msg->param_lst[0]));
	DBG_TRACE_INIT(T, p_clnt->username);
	DBG_TRACE_FUNC(T, p_clnt->username);
	DBG_TRACE_MSG(T, p_clnt->username, p_msg);

	p_clnt->connected = true;

	wait_code = WaitForSingleObject(p_clnt->p_env->h_players_smpr, 5000);
	switch (wait_code) {
	case WAIT_OBJECT_0:
		state = STATE_CONNECT_APPROVE;
		break;
	case WAIT_TIMEOUT:
		state = STATE_CONNECT_DENY;
		break;
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
		/* fall through */
	default:
		state = STATE_ABORT_THREAD;
	}

	free_msg(&p_msg);

	return state;
}

int flow_serv_opponent_quit(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res;
	
	res = server_send_msg(p_clnt, MSG_SERVER_OPPONENT_QUIT, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	res = game_session_end(p_clnt);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	return STATE_MAIN_MENU;
}

int flow_serv_player_move(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	
	int res;
	struct msg *p_msg = NULL;
	char buffer1[25] = {0};
	char buffer2[25] = {0};
	int bulls, cows;
	bool i_won = false, op_won = false;

	// ask player for guess
	res = server_send_msg(p_clnt, MSG_SERVER_PLAYER_MOVE_REQUEST, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMOUT_SEC_HUMAN_MAX);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_ABORT_THREAD;
	}

	memcpy(buffer1, p_msg->param_lst[0], strlen(p_msg->param_lst[0]));
	free_msg(&p_msg);

	// send my guess to op thread, i get opp guess (now kept in buff1)
	res = session_sequence(p_clnt, buffer1);
	switch (res) {
	case E_SUCCESS:
		break;
	case E_TIMEOUT:
		return STATE_OPONENT_QUIT;
		break;
	default:
		p_clnt->last_err = E_FLOW; // FIXME:
		return STATE_ABORT_THREAD;
	}

	// calc op bulls and cows of op compared to mine
	DBG_TRACE_STR(T, p_clnt->username, "buffer1=%s, setup_num=%s", buffer1, p_clnt->setup_numbers);
	bulls = game_bulls(buffer1, p_clnt->setup_numbers);
	cows = game_cows(buffer1, p_clnt->setup_numbers);
	if (bulls == 4) {
		op_won = true;
	}

	// send op his bulls and cows, and get mine (now in buff2)
	sprintf_s(buffer2, 4, "%d.%d", bulls, cows);
	res = session_sequence(p_clnt, buffer2);
	switch (res) {
	case E_SUCCESS:
		break;
	case E_TIMEOUT:
		return STATE_OPONENT_QUIT;
		break;
	default:
		p_clnt->last_err = E_FLOW; // FIXME:
		return STATE_ABORT_THREAD;
	}

	sscanf_s(buffer2, "%d.%d", &bulls, &cows);
	if (bulls == 4)
		i_won = true;
	
	buffer2[1] = 0;
	if (i_won && op_won) {
		res = server_send_msg(p_clnt, MSG_SERVER_DRAW, NULL, NULL, NULL, NULL);
		if (res != E_SUCCESS) {
			p_clnt->last_err = res;
			return STATE_ABORT_THREAD;
		}
		res = game_session_end(p_clnt);
		if (res != E_SUCCESS) {
			p_clnt->last_err = res;
			return STATE_ABORT_THREAD;
		}
		return STATE_MAIN_MENU;
	} else if (!i_won && !op_won) {
		res = server_send_msg(p_clnt, MSG_SERVER_GAME_RESULTS, &buffer2[0] , &buffer2[2], p_clnt->opponent_username, buffer1); // !!!!!!!!!!
		if (res != E_SUCCESS) {
			p_clnt->last_err = res;
			return STATE_ABORT_THREAD;
		}
		res = game_session_end(p_clnt);
		if (res != E_SUCCESS) {
			p_clnt->last_err = res;
			return STATE_ABORT_THREAD;
		}
		return STATE_MAIN_MENU;
	} else {
		memcpy(buffer1, p_clnt->setup_numbers, strlen(p_clnt->setup_numbers));
		res = session_sequence(p_clnt, buffer1);
		switch (res) {
		case E_SUCCESS:
			break;
		case E_TIMEOUT:
			return STATE_OPONENT_QUIT; // FIXME:
		default:
			p_clnt->last_err = E_FLOW; // FIXME:
			return STATE_ABORT_THREAD;
		}
		char *winner_name = i_won ? p_clnt->username : p_clnt->opponent_username;
		res = server_send_msg(p_clnt, MSG_SERVER_WIN, winner_name , buffer1, NULL, NULL);
		if (res != E_SUCCESS) {
			p_clnt->last_err = res;
			return STATE_ABORT_THREAD;
		}
		res = game_session_end(p_clnt);
		if (res != E_SUCCESS) {
			p_clnt->last_err = res;
			return STATE_ABORT_THREAD;
		}
		return STATE_MAIN_MENU;
	}
}


// fsm functions
int(*server_fsm[STATE_MAX])(struct client *p_clnt) =
{
	[STATE_CONNECT]         = flow_serv_connect,
	[STATE_DISCONNECT]      = flow_serv_disconnect,
	[STATE_ABORT_THREAD]    = flow_serv_abort,
	[STATE_CONNECT_APPROVE] = flow_serv_connect_approve,
	[STATE_CONNECT_DENY]    = flow_serv_connect_deny,
	[STATE_MAIN_MENU]       = flow_serv_main_menu,
	[STATE_THREAD_CLEANUP]  = flow_serv_thread_cleanup,
	[STATE_ASK_FOR_GAME]    = flow_serv_ask_for_game,
	[STATE_GAME_INVITE]     = flow_serv_invite_and_setup, // temp
	[STATE_NO_OPPONENTS]    = flow_serv_no_opponents,
	[STATE_OPONENT_QUIT]    = flow_serv_opponent_quit,
	[STATE_PLAYER_MOVE]     = flow_serv_player_move,
};