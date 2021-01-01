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
#include "game.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

DWORD WINAPI client_thread(LPVOID param)
{
	struct client *p_clnt = NULL;
	struct serv_env  *p_env = NULL;
	struct msg *p_msg = NULL;
	int state = SERVER_FSM_CONNECT;

	p_clnt = (struct client*)param;
	p_env = p_clnt->p_env;

	/* thread state machine */
	while (state != SERVER_FSM_EXIT) {

		/* check for abort signal from main thread */
		if (server_check_abort(p_env))
			state = SERVER_FSM_ABORT;

		/* preform state and get next state */
		state = (*server_fsm[state])(p_clnt);
	}

	/* thread cleanup is done at fsm_thread_cleanup,
	 * which is the only state that calls exit state */
	
	ExitThread((DWORD)p_clnt->last_err);
}

int server_fsm_abort(struct client *p_clnt)
{
	/* trace */
	if (p_clnt->username)
		DBG_TRACE_FUNC(T, p_clnt->username);

	/* trace */
	if (p_clnt->username && (p_clnt->last_err != E_SUCCESS))
		DBG_TRACE_STR(T, p_clnt->username, "abort due to error: 0x%X", p_clnt->last_err);
	
	/* redirect to disconnect or cleanup */
	if (p_clnt->connected)
		return SERVER_FSM_DISCONNECT;
	else
		return SERVER_FSM_CLEANUP;
}

int server_fsm_disconnect(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	/* exit game. no ponit to check error since
	 * thread is exiting anyway. error print will
	 * come from within call */
	if (p_clnt->playing)
		game_session_end(p_clnt);

	/* allowing other clients to be approved by server */
	if (!ReleaseSemaphore(p_clnt->p_env->h_players_smpr, 1, NULL))
		PRINT_ERROR(E_WINSOCK);

	/* mark as not connected */
	p_clnt->connected = false;

	/* go to cleanup */
	return SERVER_FSM_CLEANUP;
}

int server_fsm_cleanup(struct client *p_clnt)
{
	/* free resources */
	if (p_clnt->username) {
		DBG_TRACE_FUNC(T, p_clnt->username);
		free(p_clnt->username);
	}

	if (p_clnt->opp_username)
		free(p_clnt->opp_username);

	/* close threads socket */
	if (closesocket(p_clnt->skt) == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		p_clnt->last_err = E_WINSOCK;
	}

	/* stop threads execution loop */
	return SERVER_FSM_EXIT;
}

int server_fsm_main_menu(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	
	int res;
	struct msg *p_msg;
	int next_state;

	/* send main menu */
	res = server_send_msg(p_clnt, MSG_SERVER_MAIN_MENU, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	/* recieve client choice - 15 minute wait */
	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMOUT_SEC_HUMAN_MAX);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	/* parse client choice */
	switch (p_msg->type)
	{
	case MSG_CLIENT_VERSUS:
		next_state = SERVER_FSM_GAME_REQ;
		break;
	case MSG_CLIENT_DISCONNECT:
		next_state = SERVER_FSM_DISCONNECT;
		break;
	default:
		p_clnt->last_err = E_FLOW;
		next_state = SERVER_FSM_ABORT;
	}

	/* free resource */
	free_msg(&p_msg);

	return next_state;
}

int server_fsm_ask_for_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res, next_state;
	struct game *p_game = NULL;
	char buff[25] = {0}; // FIXME:

	/* start a game session */
	res = game_session_start(p_clnt);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	res = game_sequence(p_clnt, p_clnt->username, buff);
	switch (res) {
	case E_TIMEOUT:
		next_state = SERVER_FSM_NO_OPP;
		break;
	case E_SUCCESS:
		p_clnt->opp_username = calloc(strlen(buff) + 1, sizeof(char));
		if (!p_clnt->opp_username) {
			p_clnt->last_err = E_STDLIB;
			next_state = SERVER_FSM_ABORT;
			break;
		}
		memcpy(p_clnt->opp_username, buff, strlen(buff));
		next_state = SERVER_FSM_INVITE;
		break;
	default:
		p_clnt->last_err = res;
		next_state = SERVER_FSM_ABORT;
		break;
	}

	return next_state;
}

int server_fsm_invite_and_setup(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res, next_state;
	struct msg *p_msg = NULL;
	char dummy[25] = {0}; // FIXME:

	/* send invite */
	res = server_send_msg(p_clnt, MSG_SERVER_INVITE, p_clnt->opp_username, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	/* send setup request */
	res = server_send_msg(p_clnt, MSG_SERVER_SETUP_REQUEST, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	/* wait for client setup reponse */
	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMOUT_SEC_HUMAN_MAX);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	if (p_msg->type != MSG_CLIENT_SETUP) {
		free_msg(&p_msg);
		p_clnt->last_err = E_FLOW;
		return SERVER_FSM_ABORT;
	}

	memcpy(p_clnt->setup_numbers, p_msg->param_lst[0], 4); // FIXME:
	DBG_TRACE_STR(T, p_clnt->username, "setup: %s", p_clnt->setup_numbers);

	res = game_sequence(p_clnt, dummy, dummy);
	switch (res) {
	case E_SUCCESS:
		next_state = SERVER_FSM_PLAY_MOVE;
		break;
	case E_TIMEOUT:
		next_state = SERVER_FSM_OPP_QUIT;
		break;
	default:
		p_clnt->last_err = E_INTERNAL; // FIXME:
		next_state = SERVER_FSM_ABORT;
	}

	free_msg(&p_msg);
	return next_state;
}

int server_fsm_no_opponents(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res;

	res = game_session_end(p_clnt);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	res = server_send_msg(p_clnt, MSG_SERVER_NO_OPONENTS, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	return SERVER_FSM_MENU;
}

int server_fsm_connect_approve(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	assert(p_clnt->connected);
	int res;
	
	res = server_send_msg(p_clnt, MSG_SERVER_APPROVED, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	return SERVER_FSM_MENU;
}

int server_fsm_connect_deny(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	assert(p_clnt->connected);
	
	int res;
	
	res = server_send_msg(p_clnt, MSG_SERVER_DENIED, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	return SERVER_FSM_CLEANUP;
}

int server_fsm_connect(struct client *p_clnt)
{
	assert(!p_clnt->connected);
	
	struct msg *p_msg = NULL;
	int res, state;
	DWORD wait_code;

	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}
	
	if (p_msg->type != MSG_CLIENT_REQUEST) {
		free_msg(&p_msg);
		p_clnt->last_err = E_FLOW;
		return SERVER_FSM_ABORT;
	}

	p_clnt->username = calloc(strlen(p_msg->param_lst[0]) + 1, sizeof(char));
	if (!p_clnt->username) {
		PRINT_ERROR(E_STDLIB);
		free_msg(&p_msg);
		return SERVER_FSM_ABORT;
	}
	memcpy(p_clnt->username, p_msg->param_lst[0], strlen(p_msg->param_lst[0]));
	DBG_TRACE_INIT(T, p_clnt->username);
	DBG_TRACE_FUNC(T, p_clnt->username);
	DBG_TRACE_MSG(T, p_clnt->username, p_msg);

	p_clnt->connected = true;

	wait_code = WaitForSingleObject(p_clnt->p_env->h_players_smpr, 5000);
	switch (wait_code) {
	case WAIT_OBJECT_0:
		state = SERVER_FSM_APPROVE;
		break;
	case WAIT_TIMEOUT:
		state = SERVER_FSM_DENY;
		break;
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
		/* fall through */
	default:
		state = SERVER_FSM_ABORT;
	}

	free_msg(&p_msg);

	return state;
}

int server_fsm_opponent_quit(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res;
	
	res = server_send_msg(p_clnt, MSG_SERVER_OPPONENT_QUIT, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	res = game_session_end(p_clnt);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	return SERVER_FSM_MENU;
}

int server_fsm_player_move(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	
	int res = E_SUCCESS, next_state = SERVER_FSM_ABORT;
	struct msg *p_msg = NULL;
	int bulls = 0, cows = 0;
	bool i_won = false, opp_won = false;
	char opp_guess[10] = {0};
	char result[10] = {0}, result2[10] = {0};
	char b[2] = {0}, c[2] = {0};
	char *winner_name = NULL;
	
	/* request plalyer to make a guess */
	res = server_send_msg(p_clnt, MSG_SERVER_PLAYER_MOVE_REQUEST, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	/* wait for guess */
	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMOUT_SEC_HUMAN_MAX);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	/* send guess to opponent, recieve opponent guess */
	res = game_sequence(p_clnt, p_msg->param_lst[0], opp_guess);
	switch (res) {
	case E_SUCCESS:
		next_state = SERVER_FSM_PLAY_MOVE;
		break;
	case E_TIMEOUT:
		next_state = SERVER_FSM_OPP_QUIT;
		break;
	default:
		p_clnt->last_err = res;
		next_state = SERVER_FSM_ABORT;
	}

	free_msg(&p_msg);

	/* state exit point */
	if (next_state != SERVER_FSM_PLAY_MOVE)
		return next_state;

	/* calc opponents bulls and cows */
	bulls = game_bulls(opp_guess, p_clnt->setup_numbers);
	cows = game_cows(opp_guess, p_clnt->setup_numbers);
	DBG_TRACE_STR(T, p_clnt->username, "opp_guess=%s (setup_num=%s): b=%d, c=%d", opp_guess, p_clnt->setup_numbers, bulls, cows);

	/* check if opponent won */
	if (bulls == 4) // FIXME:
		opp_won = true;

	/* send opponent his bulls and cows, and get my own */
	sprintf_s(result, 4, "%d.%d", bulls, cows);
	res = game_sequence(p_clnt, result, result2);
	switch (res) {
	case E_SUCCESS:
		break;
	case E_TIMEOUT:
		return SERVER_FSM_OPP_QUIT;
	default:
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}
	sscanf_s(result2, "%d.%d", &bulls, &cows);
	DBG_TRACE_STR(T, p_clnt->username, "my_guess resulted with: b=%d, c=%d (%s)", bulls, cows, result2);
	/* check if i won */
	if (bulls == 4) // FIXME:
		i_won = true;
	DBG_TRACE_STR(T, p_clnt->username, "my_guess resulted with: b=%d, c=%d (%s), i_wan=%d", bulls, cows, result2, i_won);


	/* check results */
	b[0] = '0' + bulls;
	c[0] = '0' + cows;
	if (i_won && opp_won) {
		res = server_send_msg(p_clnt, MSG_SERVER_DRAW, NULL, NULL, NULL, NULL);
		next_state = SERVER_FSM_MENU;
	} else if (!i_won && !opp_won) {
		res = server_send_msg(p_clnt, MSG_SERVER_GAME_RESULTS, b, c, p_clnt->opp_username, opp_guess);
		next_state = SERVER_FSM_PLAY_MOVE;
	} else {
		winner_name = i_won ? p_clnt->username : p_clnt->opp_username;
		res = game_sequence(p_clnt, p_clnt->setup_numbers, opp_guess);
		switch (res) {
		case E_SUCCESS:
			break;
		case E_TIMEOUT:
			return SERVER_FSM_OPP_QUIT; // FIXME:
		default:
			p_clnt->last_err = res;
			return SERVER_FSM_ABORT;
		}
		res = server_send_msg(p_clnt, MSG_SERVER_WIN, winner_name, opp_guess, NULL, NULL);
		next_state = SERVER_FSM_MENU;
	}

	if (next_state != SERVER_FSM_PLAY_MOVE)
		res = game_session_end(p_clnt);

	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return SERVER_FSM_ABORT;
	}

	return next_state;
}

// fsm functions
int(*server_fsm[SERVER_FSM_MAX])(struct client *p_clnt) =
{
	[SERVER_FSM_CONNECT]         = server_fsm_connect,
	[SERVER_FSM_DISCONNECT]      = server_fsm_disconnect,
	[SERVER_FSM_ABORT]    = server_fsm_abort,
	[SERVER_FSM_APPROVE] = server_fsm_connect_approve,
	[SERVER_FSM_DENY]    = server_fsm_connect_deny,
	[SERVER_FSM_MENU]       = server_fsm_main_menu,
	[SERVER_FSM_CLEANUP]  = server_fsm_cleanup,
	[SERVER_FSM_GAME_REQ]    = server_fsm_ask_for_game,
	[SERVER_FSM_INVITE]     = server_fsm_invite_and_setup, // temp
	[SERVER_FSM_NO_OPP]    = server_fsm_no_opponents,
	[SERVER_FSM_OPP_QUIT]    = server_fsm_opponent_quit,
	[SERVER_FSM_PLAY_MOVE]     = server_fsm_player_move,
};