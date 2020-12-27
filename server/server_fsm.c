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
#include <assert.h>
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
	struct clnt_args *p_clnt = NULL;
	struct serv_env  *p_env = NULL;
	struct msg *p_msg = NULL;
	int state = STATE_CONNECT_ATTEMPT;

	p_clnt = (struct clnt_args*)param;
	p_env = p_clnt->p_env;

	do {
		if (server_check_abort(p_env))
			state = STATE_ABORT_THREAD;

		state = (*server_fsm[state])(p_clnt);

	} while (state != STATE_THREAD_EXIT);

	ExitThread((DWORD)p_clnt->last_err);
}


int flow_serv_undefined_flow(struct clnt_args *p_clnt)
{
	DBG_FUNC_STAMP();
	PRINT_ERROR(E_FLOW);
	return STATE_ABORT_THREAD;
}



flow_serv_disconnect(struct clnt_args *p_clnt)
{
	DBG_FUNC_STAMP();
	assert(p_clnt->connected);

	// int res;

	// what if during game??

	if (!ReleaseSemaphore(p_clnt->p_env->h_players_smpr, 1, NULL)) {
		PRINT_ERROR(E_WINSOCK);
		// signal main a critical error????
	}

	p_clnt->connected = false;

	return STATE_THREAD_CLEANUP;
}


flow_serv_thread_cleanup(struct clnt_args *p_clnt)
{
	DBG_FUNC_STAMP();
	assert(!p_clnt->connected);

	if (closesocket(p_clnt->skt) == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		p_clnt->last_err = E_WINSOCK;
	}

	return STATE_THREAD_EXIT;
}

int flow_serv_abort_thread(struct clnt_args *p_clnt)
{
	DBG_FUNC_STAMP();

	/* disconnect thread, errors dont affect flow */
	if (p_clnt->connected)
		flow_serv_disconnect(p_clnt);

	/* cleanup thread, errors dont affect flow */
	flow_serv_thread_cleanup(p_clnt);

	return STATE_THREAD_EXIT;

}


// int flow_clnt_recieve_failure(struct client_env *p_env)
// {
// 	DBG_FUNC_STAMP();
// 	switch (p_env->last_error)
// 	{
// 	case E_TIMEOUT:
// 		UI_PRINT(UI_CONNECT_FAIL, p_env->serv_ip, p_env->serv_port);
// 		return STATE_CONNECT_FAILURE;
// 	default:
// 		PRINT_ERROR(p_env->last_error);
// 		return STATE_THREAD_EXIT;
// 	}	
// }



flow_serv_main_menu(struct clnt_args *p_clnt)
{
	DBG_FUNC_STAMP();
	assert(p_clnt->connected);
	
	int res;
	struct msg *p_msg;
	int state;

	res = server_send_msg(p_clnt, MSG_SERVER_MAIN_MENU, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_DISCONNECT;
	}

	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMOUT_SEC_HUMAN_MAX);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_DISCONNECT;
	}

	switch (p_msg->type)
	{
	case MSG_CLIENT_VERSUS:
		state = STATE_DISCONNECT; // FIXME: temporary
		/* code */
		break;
	case MSG_CLIENT_DISCONNECT:
		state = STATE_DISCONNECT;
		break;
	default:
		state = STATE_UNDEFINED_FLOW;
		break;
	}

	free_msg(&p_msg);
	return STATE_THREAD_EXIT;
}


int flow_serv_connect_approve(struct clnt_args *p_clnt)
{
	DBG_FUNC_STAMP();
	assert(p_clnt->connected);
	int res;
	
	res = server_send_msg(p_clnt, MSG_SERVER_APPROVED, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_DISCONNECT;
	}

	return STATE_MAIN_MENU;
}

int flow_serv_connect_deny(struct clnt_args *p_clnt)
{
	DBG_FUNC_STAMP();
	assert(!p_clnt->connected);
	
	int res;
	
	res = server_send_msg(p_clnt, MSG_SERVER_DENIED, NULL, NULL, NULL, NULL);
	if (res != E_SUCCESS)
		p_clnt->last_err = res;

	return STATE_THREAD_CLEANUP;
}

int flow_serv_connect(struct clnt_args *p_clnt)
{
	DBG_FUNC_STAMP();
	assert(!p_clnt->connected);
	
	struct msg *p_msg = NULL;
	int res, state;
	DWORD wait_code;

	res = server_recv_msg(p_clnt, &p_msg, MSG_TIMEOUT_SEC_DEFAULT);
	if (res != E_SUCCESS) {
		p_clnt->last_err = res;
		return STATE_THREAD_CLEANUP;
	}
	
	if (p_msg->type != MSG_CLIENT_REQUEST) {
		free_msg(&p_msg);
		return STATE_THREAD_CLEANUP;
	}

	wait_code = WaitForSingleObject(p_clnt->p_env->h_players_smpr, 5000);
	switch (wait_code) {
	case WAIT_OBJECT_0:
		state = STATE_CONNECT_APPROVE;
		p_clnt->connected = true;
		break;
	case WAIT_TIMEOUT:
		state = STATE_CONNECT_DENY;
		break;
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
		/* fall through */
	default:
		state = STATE_CONNECT_DENY;
	}

	free_msg(&p_msg);

	return state;
}


// fsm functions
int(*server_fsm[STATE_MAX])(struct clnt_args *p_clnt) =
{
	[STATE_CONNECT_ATTEMPT] = flow_serv_connect,
	[STATE_DISCONNECT]      = flow_serv_disconnect,
	[STATE_UNDEFINED_FLOW]  = flow_serv_undefined_flow,
	[STATE_CONNECT_APPROVE] = flow_serv_connect_approve,
	[STATE_CONNECT_DENY]    = flow_serv_connect_deny,
	[STATE_MAIN_MENU]       = flow_serv_main_menu,
	[STATE_THREAD_CLEANUP]  = flow_serv_thread_cleanup,
	[STATE_ABORT_THREAD]    = flow_serv_abort_thread,
};