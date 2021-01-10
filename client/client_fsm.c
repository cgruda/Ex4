/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client program
 *
 * client_fsm.c
 * 
 * this is part of the client FSM (finite state machine) module.
 * each function is this module a state the client can be in.
 * each function (state) returns the next state. moving between
 * states is done using an array of funtions, where functions
 * are indexed by their states, as defined in enum state_clnt.
 *
 * the first state in the FSM is: CLIENT_FSM_CONNECT
 * the last state in the FSM is: CLIENT_FSM_EXIT
 *
 * in some cases a state will directly call a differet function,
 * without changing the actual state. this is valid operation.
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS// FIXME:

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include "winsock2.h"
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "client_tasks.h"
#include "client_fsm.h"
#include "tasks.h"
#include "message.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

int client_fsm_connect(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);

	int res;

	/* create socket */
	p_env->skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p_env->skt == INVALID_SOCKET) {
		PRINT_ERROR(E_WINSOCK);
		p_env->last_err = E_WINSOCK;
		return CLIENT_FSM_EXIT;
	}

	/* connect socket with server - TCP connection */
	res = connect(p_env->skt, (PSOCKADDR)&p_env->server, sizeof(SOCKADDR));
	if(res == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAECONNREFUSED) {
			PRINT_ERROR(E_WINSOCK);
			p_env->last_err = E_WINSOCK;
		}
		return CLIENT_FSM_CONNECT_FAIL;
	}

	/* go to send client request */
	return CLIENT_FSM_REQUEST;
}

int client_fsm_disconnect(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);

	int res;

	/* send disconncet message, return value does not matter
	 * since we are on way out anyway. arror messages will
	 * be printed from within call */
	res = client_send_msg(p_env, MSG_CLIENT_DISCONNECT, NULL);

	/* message was sent successfully, signal end of session
	 * and that client has no more data to send */
	if (res == E_SUCCESS) {
		if (shutdown(p_env->skt, SD_SEND) == SOCKET_ERROR)
			PRINT_ERROR(E_WINSOCK);
		DBG_TRACE_STR(TRACE_CLIENT, p_env->username, "gracefull disconnect");
	}
	
	/*closesocket is called at client_cleanup() */

	/* change approval state */
	p_env->approved = false;

	return CLIENT_FSM_EXIT;
}

int client_fsm_connect_fail(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	int res;

	DBG_TRACE_STR(TRACE_CLIENT, p_env->username, "Error: %d", p_env->last_err);

	/* send disconnect if was approved. result
	 * does not matter since we are in an error
	 * flow in any case. error messages will be
	 * printed from whithin call */
	if (p_env->approved)
		client_fsm_disconnect(p_env);

	/* print failure message */
	UI_PRINT(UI_CONNECT_FAIL, p_env->server_ip, p_env->server_port);

	/* close socket - terminate TCP connection */
	res = closesocket(p_env->skt);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		p_env->last_err = E_WINSOCK;
		return CLIENT_FSM_EXIT;
	}

	/* go to reconnect menu */
	return CLIENT_FSM_RECONNECT;
}

client_fsm_reconnect(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	int choice;

	/* print menu and await choice */
	UI_PRINT(UI_MENU_CONNECT);
	UI_GET(&choice);

	/* parse choice */
	switch (choice) {
	case CHOICE_RECONNECT:
		return CLIENT_FSM_CONNECT;
	case CHOICE_EXIT:
		return CLIENT_FSM_EXIT;
	default:
		print_error(E_INPUT);
		p_env->last_err = E_INPUT;
		return CLIENT_FSM_EXIT;
	}
}

int client_fsm_request(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	struct msg *p_msg = NULL;
	int res, next_state;

	/* send client request */
	res = client_send_msg(p_env, MSG_CLIENT_REQUEST, p_env->username);
	if (res != E_SUCCESS) {
		p_env->last_err = res;
		return CLIENT_FSM_CONNECT_FAIL;
	}
	
	/* recieve server answer */
	res = client_recv_msg(&p_msg, p_env, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_env->last_err = res;
		return CLIENT_FSM_CONNECT_FAIL;
	}

	/* interpet server response */
	switch (p_msg->type) {
	case MSG_SERVER_APPROVED:
		next_state = CLIENT_FSM_APPROVED;
		break;
	case MSG_SERVER_DENIED:
		next_state = CLIENT_FSM_DENIED;
		break;
	default:
		p_env->last_err = E_FLOW;
		next_state = CLIENT_FSM_UNDEFINED;
	}

	/* free recieved message */
	free_msg(&p_msg);

	return next_state;
}

int client_fsm_denied(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	assert(!p_env->approved);

	/* print message */
	UI_PRINT(UI_CONNECT_DENY, p_env->server_ip, p_env->server_port);

	/* go to reconnect menu */
	return CLIENT_FSM_RECONNECT;
}

int client_fsm_approved(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	assert(!p_env->approved);

	/* change approval state */
	p_env->approved = true;

	/* print user notification */
	UI_PRINT(UI_CONNECTED, p_env->server_ip, p_env->server_port);

	/* go to next state */
	return CLIENT_FSM_MAIN_MENU;
}

int client_fsm_main_menu(struct client_env *p_env)
{	
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	assert(p_env->approved);
	struct msg *p_msg;
	int res, choice;

	/* await main menu message */
	res = client_recv_msg(&p_msg, p_env, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_env->last_err = res;
		return CLIENT_FSM_CONNECT_FAIL;
	}

	/* interpet server message */
	if  (p_msg->type != MSG_SERVER_MAIN_MENU) {
		free_msg(&p_msg);	
		return CLIENT_FSM_UNDEFINED;
	}

	/* free resource */
	free_msg(&p_msg);

	/* print main menu and await choice */
	UI_PRINT(UI_MENU_PLAY);
	UI_GET(&choice);

	/* parse decision */
	switch (choice)
	{
	case CHOICE_PLAY:
		return CLIENT_FSM_GAME_REQ;
	case CHOICE_QUIT:
		return CLIENT_FSM_DISCONNECT;
	default:
		print_error(E_INPUT);
		p_env->last_err = E_INPUT;
		return CLIENT_FSM_DISCONNECT;
	}
}

int client_fsm_invite_setup(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	struct msg *p_msg = NULL;
	char buff[UI_INPUT_LEN] = {0};
	int res;
	
	UI_PRINT(UI_GAME_START);

	res = client_recv_msg(&p_msg, p_env, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_env->last_err = res;
		return CLIENT_FSM_CONNECT_FAIL;
	}

	if (p_msg->type != MSG_SERVER_SETUP_REQUEST) {
		free_msg(&p_msg);
		return CLIENT_FSM_UNDEFINED;
	}

	free_msg(&p_msg);

	UI_PRINT(UI_GAME_CHOOSE);

	if (!client_game_input_get(buff)) {
		print_error(E_INPUT);
		p_env->last_err = E_INPUT;
		return CLIENT_FSM_DISCONNECT;
	}

	res = client_send_msg(p_env, MSG_CLIENT_SETUP, buff);
	if (res != E_SUCCESS) {
		p_env->last_err = res;
		return CLIENT_FSM_CONNECT_FAIL;
	}

	return CLIENT_FSM_GAME_MOVE;
}

int client_fsm_game_req(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	assert(p_env->approved);

	struct msg *p_msg = NULL;
	int res, next_state;

	res = client_send_msg(p_env, MSG_CLIENT_VERSUS, NULL);
	if (res != E_SUCCESS) {
		p_env->last_err = res;
		return CLIENT_FSM_CONNECT_FAIL;
	}

	res = client_recv_msg(&p_msg, p_env, MSG_TIMEOUT_SEC_MAX);
	if (res != E_SUCCESS) {
		p_env->last_err = res;
		return CLIENT_FSM_CONNECT_FAIL;
	}

	switch (p_msg->type) {
	case MSG_SERVER_NO_OPONENTS:
		next_state = CLIENT_FSM_MAIN_MENU;
		break;
	case MSG_SERVER_INVITE:
		next_state = CLIENT_FSM_INVITE_SETUP;
		break;
	default:
		next_state = CLIENT_FSM_UNDEFINED;
		break;
	}

	free_msg(&p_msg);

	return next_state;
}

int client_fsm_undefined(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	PRINT_ERROR(E_FLOW);
	
	/* handle accorsing to approval state */
	if (p_env->approved)
		return CLIENT_FSM_DISCONNECT;
	else
		return CLIENT_FSM_EXIT;
}

int client_fsm_game_move(struct client_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_CLIENT, p_env->username);
	struct msg *p_msg = NULL;
	int res, next_state;
	char buff[UI_INPUT_LEN] = {0};

	res = client_recv_msg(&p_msg, p_env, MSG_TIMOUT_SEC_HUMAN_MAX);
	if (res != E_SUCCESS) {
		p_env->last_err = res;
		return CLIENT_FSM_CONNECT_FAIL;
	}

	switch (p_msg->type) {
	case MSG_SERVER_PLAYER_MOVE_REQUEST:
		UI_PRINT(UI_GAME_GUESS);

		if (!client_game_input_get(buff)) {
			print_error(E_INPUT);
			p_env->last_err = E_INPUT;
			next_state = CLIENT_FSM_DISCONNECT;
			break;
		}

		res = client_send_msg(p_env, MSG_CLIENT_PLAYER_MOVE, buff);
		if (res != E_SUCCESS) {
			p_env->last_err = res;
			next_state = CLIENT_FSM_CONNECT_FAIL;
			break;
		}

		next_state = CLIENT_FSM_GAME_MOVE;
		break;
	case MSG_SERVER_GAME_RESULTS:
		UI_PRINT(UI_GAME_STAGE, p_msg->param_lst[0], p_msg->param_lst[1],
					p_msg->param_lst[2], p_msg->param_lst[3]);
		next_state = CLIENT_FSM_GAME_MOVE;
		break;
	case MSG_SERVER_WIN:
		UI_PRINT(UI_GAME_WIN, p_msg->param_lst[0], p_msg->param_lst[1]);
		next_state = CLIENT_FSM_MAIN_MENU;
		break;
	case MSG_SERVER_DRAW:
		UI_PRINT(UI_GAME_DRAW);
		next_state = CLIENT_FSM_MAIN_MENU;
		break;
	case MSG_SERVER_OPPONENT_QUIT:
		UI_PRINT(UI_GAME_STOP);
		next_state = CLIENT_FSM_MAIN_MENU;
		break;
	default:
		next_state = CLIENT_FSM_UNDEFINED;
		break;
	}

	free_msg(&p_msg);

	return next_state;
}

// client finite state machine functions array defenition
int(*clnt_flow[CLIENT_FSM_MAX])(struct client_env *p_env) = {
	[CLIENT_FSM_CONNECT]      = client_fsm_connect,
	[CLIENT_FSM_CONNECT_FAIL] = client_fsm_connect_fail,
	[CLIENT_FSM_APPROVED]     = client_fsm_approved,
	[CLIENT_FSM_DENIED]       = client_fsm_denied,
	[CLIENT_FSM_UNDEFINED]    = client_fsm_undefined,
	[CLIENT_FSM_DISCONNECT]   = client_fsm_disconnect,
	[CLIENT_FSM_MAIN_MENU]    = client_fsm_main_menu,
	[CLIENT_FSM_REQUEST]      = client_fsm_request,
	[CLIENT_FSM_RECONNECT]    = client_fsm_reconnect,
	[CLIENT_FSM_GAME_REQ]     = client_fsm_game_req,
	[CLIENT_FSM_INVITE_SETUP] = client_fsm_invite_setup,
	[CLIENT_FSM_GAME_MOVE]    = client_fsm_game_move,
};
