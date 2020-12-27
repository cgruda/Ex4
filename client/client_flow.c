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
 * the first state in the FSM is: STATE_CONNECT_ATTEMPT
 * the last state in the FSM is: STATE_EXIT
 *
 * in some cases a state will directly call a differet function,
 * without changing the actual state. this is valid operation.
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "client_tasks.h"
#include "client_flow.h"
#include "tasks.h"
#include "message.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

int flow_clnt_connect_attempt(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(!p_env->approved);

	int res;

	/* create socket */
	p_env->skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p_env->skt == INVALID_SOCKET) {
		PRINT_ERROR(E_WINSOCK);
		p_env->last_error = E_WINSOCK;
		return STATE_EXIT;
	}

	/* connect socket with server - TCP connection */
	res = connect(p_env->skt, (PSOCKADDR)&p_env->server, sizeof(SOCKADDR));
	if(res == SOCKET_ERROR) {
		p_env->last_error = E_WINSOCK;
		return STATE_CONNECT_FAILURE;
	}

	/* TCP connection success */
	UI_PRINT(UI_CONNECTED, p_env->serv_ip, p_env->serv_port);

	/* go to send client request */
	return STATE_CLIENT_REQUEST;
}

int flow_clnt_disconnect(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(p_env->approved);

	/* send disconncet message. if error occures there
	 * is nothing to be done, i.e we exit anyway. so no
	 * purpose in getting or cheking result. error print
	 * will be called from within call */
	cilent_send_msg(p_env, MSG_CLIENT_DISCONNECT, NULL);

	/* change approval state */
	p_env->approved = false;

	return STATE_EXIT;
}

int flow_clnt_connect_failure(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	int res;

	/* send disconnect if was approved. result
	 * does not matter since we are in an error
	 * flow in any case. error messages will be
	 * printed from whithin call */
	if (p_env->approved)
		flow_clnt_disconnect(p_env);

	/* print failure message */
	UI_PRINT(UI_CONNECT_FAIL, p_env->serv_ip, p_env->serv_port);

	/* close socket - terminate TCP connection */
	res = closesocket(p_env->skt);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		p_env->last_error = E_WINSOCK;
		return STATE_EXIT;
	}

	/* go to reconnect menu */
	return STATE_RECONNECT_MENU;
}

flow_clnt_reconnect_menu(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	int choice;

	/* print menu and await choice */
	UI_PRINT(UI_MENU_CONNECT);
	UI_GET(&choice);

	/* parse choice */
	switch (choice)
	{
	case CHOICE_RECONNECT:
		return STATE_CONNECT_ATTEMPT;
	case CHOICE_EXIT:
		return STATE_EXIT;
	default:
		PRINT_ERROR(E_INPUT);
		p_env->last_error = E_INPUT;
		return STATE_EXIT;
	}
}

int flow_clnt_client_request(struct client_env *p_env)
{
	struct msg *p_msg = NULL;
	int res, next_state;

	/* send client request */
	res = cilent_send_msg(p_env, MSG_CLIENT_REQUEST, p_env->username);
	if (res != E_SUCCESS) {
		p_env->last_error = res;
		return STATE_CONNECT_FAILURE;
	}
	
	/* recieve server answer */
	res = client_recv_msg(&p_msg, p_env, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_env->last_error = res;
		return STATE_CONNECT_FAILURE;
	}

	/* interpet server response */
	switch (p_msg->type)
	{
	case MSG_SERVER_APPROVED:
		next_state = STATE_CONNECT_APPROVED;
		break;
	case MSG_SERVER_DENIED:
		next_state = STATE_CONNECT_DENIED;
		break;
	default:
		p_env->last_error = E_FLOW;
		next_state = STATE_UNDEFINED_FLOW;
	}

	/* free recieved message */
	free_msg(&p_msg);

	return next_state;
}

int flow_clnt_connect_denied(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(!p_env->approved);

	/* print message */
	UI_PRINT(UI_CONNECT_DENY, p_env->serv_ip, p_env->serv_port);

	/* go to reconnect menu */
	return STATE_RECONNECT_MENU;
}

int flow_clnt_connect_approved(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(!p_env->approved);

	struct msg *p_msg;
	int res, next_state;

	/* change approval state */
	p_env->approved = true;

	/* await main menu message */
	res = client_recv_msg(&p_msg, p_env, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_env->last_error = res;
		return STATE_CONNECT_FAILURE;
	}

	/* interpet server response */
	switch (p_msg->type)
	{
	case MSG_SERVER_MAIN_MENU:
		next_state = STATE_MAIN_MENU;
		break;
	default:
		next_state = STATE_UNDEFINED_FLOW;
	}

	/* free resource */
	free_msg(&p_msg);

	/* go to next state */
	return next_state;
}

int flow_clnt_main_menu(struct client_env *p_env)
{	
	DBG_FUNC_STAMP();
	assert(p_env->approved);
	int choice;

	/* print main menu and await choice */
	UI_PRINT(UI_MENU_PLAY);
	UI_GET(&choice);

	/* parse decision */
	switch (choice)
	{
	case CHOICE_PLAY:
		// TODO: fallthroug for now
	case CHOICE_QUIT:
		return STATE_DISCONNECT;
	default:
		PRINT_ERROR(E_INPUT);
		p_env->last_error = E_INPUT;
		return STATE_DISCONNECT;
	}
}

int flow_clnt_undefined_flow(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	PRINT_ERROR(E_FLOW);
	
	/* handle accorsing to approval state */
	if (p_env->approved)
		return STATE_DISCONNECT;
	else
		return STATE_EXIT;
}

// flow functions
int(*clnt_flow[STATE_MAX])(struct client_env *p_env) =
{
	[STATE_CONNECT_ATTEMPT]  = flow_clnt_connect_attempt,
	[STATE_CONNECT_FAILURE]  = flow_clnt_connect_failure,
	[STATE_CONNECT_APPROVED] = flow_clnt_connect_approved,
	[STATE_CONNECT_DENIED]   = flow_clnt_connect_denied,
	[STATE_UNDEFINED_FLOW]   = flow_clnt_undefined_flow,
	[STATE_DISCONNECT]       = flow_clnt_disconnect,
	[STATE_MAIN_MENU]        = flow_clnt_main_menu,
	[STATE_CLIENT_REQUEST]   = flow_clnt_client_request,
	[STATE_RECONNECT_MENU]   = flow_clnt_reconnect_menu,
};