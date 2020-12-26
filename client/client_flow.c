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
#include "message.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */


int flow_clnt_connect_success(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(!p_env->connected);

	struct msg *p_msg;
	int res;

	p_env->connected = true;

	res = recv_msg(&p_msg, p_env->skt, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_env->last_error = res;
		return STATE_RECIEVE_FAILURE;
	}

	/* interpet server response */
	switch (p_msg->type)
	{
	case MSG_SERVER_MAIN_MENU:
		res = STATE_MAIN_MENU;
		break;
	default:
		res = STATE_UNDEFINED_FLOW;
	}

	free(&p_msg);
	return res;
}





int flow_clnt_main_menu(struct client_env *p_env)
{	
	DBG_FUNC_STAMP();
	assert(p_env->connected);
	int choice;

	UI_PRINT(UI_MENU_PLAY);
	UI_GET(&choice);

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



int flow_clnt_disconnect(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(p_env->connected);

	/* send disconncet message. if error occures there
	 * is nothing to be done, i.e we exit anyway. so no
	 * purpose in getting or cheking result. error print
	 * will be called from within call */
	cilent_send_msg(p_env, MSG_CLIENT_DISCONNECT, NULL);

	p_env->connected = false;

	return STATE_EXIT;
}




int flow_clnt_connect_denied(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(!p_env->connected);

	UI_PRINT(UI_CONNECT_DENY, p_env->serv_ip, p_env->serv_port);

	return STATE_CONNECT_FAILURE;
}





int flow_clnt_undefined_flow(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	PRINT_ERROR(E_FLOW);
	
	if (p_env->connected)
		return STATE_DISCONNECT;
	else
		return STATE_EXIT;
}





int flow_clnt_recieve_failure(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	switch (p_env->last_error)
	{
	case E_TIMEOUT:
		UI_PRINT(UI_CONNECT_FAIL, p_env->serv_ip, p_env->serv_port);
		return STATE_CONNECT_FAILURE;
	default:
		PRINT_ERROR(p_env->last_error);
		return STATE_EXIT;
	}	
}



int flow_clnt_connect_failure(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(!p_env->connected);

	int choice;
	int res;

	// check why connection failed?
	PRINT_ERROR(p_env->last_error);

	/* close socket */
	res = closesocket(p_env->skt);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		p_env->last_error = E_WINSOCK;
		return STATE_EXIT;
	}

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


int flow_clnt_connect_attempt(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	assert(!p_env->connected);

	struct msg *p_msg = NULL;
	int res, state;

	/* create socket */
	p_env->skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p_env->skt == INVALID_SOCKET) {
		PRINT_ERROR(E_WINSOCK);
		p_env->last_error = E_WINSOCK;
		return STATE_EXIT;
	}

	/* connect socket with server */
	res = connect(p_env->skt, (PSOCKADDR)&p_env->server, sizeof(SOCKADDR));
	if(res == SOCKET_ERROR) {
		UI_PRINT(UI_CONNECT_FAIL, p_env->serv_ip, p_env->serv_port);
		p_env->last_error = E_WINSOCK;
		return STATE_CONNECT_FAILURE;
	}

	/* TCP connection success */
	UI_PRINT(UI_CONNECT_PRE, p_env->serv_ip, p_env->serv_port);

	/* send client request */
	res = cilent_send_msg(p_env, MSG_CLIENT_REQUEST, p_env->username);
	if (res != E_SUCCESS) {
		p_env->last_error = res;
		return STATE_CONNECT_FAILURE;
	}
	
	/* recieve server answer */
	res = recv_msg(&p_msg, p_env->skt, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_env->last_error = res;
		return STATE_RECIEVE_FAILURE;
	}

	/* interpet server response */
	switch (p_msg->type)
	{
	case MSG_SERVER_APPROVED:
		state = STATE_CONNECT_SUCCESS;
		break;
	case MSG_SERVER_DENIED:
		state = STATE_CONNECT_DENIED;
		break;
	default:
		p_env->last_error = E_FLOW;
		state = STATE_UNDEFINED_FLOW;
	}

	/* free recieved message */
	free_msg(&p_msg);

	return state;
}


// flow functions
int(*clnt_flow[STATE_MAX])(struct client_env *p_env) =
{
	[STATE_CONNECT_ATTEMPT] = flow_clnt_connect_attempt,
	[STATE_CONNECT_FAILURE] = flow_clnt_connect_failure,
	[STATE_CONNECT_SUCCESS] = flow_clnt_connect_success,
	[STATE_CONNECT_DENIED]  = flow_clnt_connect_denied,
	[STATE_UNDEFINED_FLOW]  = flow_clnt_undefined_flow,
	[STATE_RECIEVE_FAILURE] = flow_clnt_recieve_failure,
	[STATE_DISCONNECT]      = flow_clnt_disconnect,
	[STATE_MAIN_MENU]       = flow_clnt_main_menu,
};