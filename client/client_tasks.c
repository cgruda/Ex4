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

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <windows.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "client_tasks.h"
#include "client_flow.h"
#include "message.h"
#include "tasks.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

void print_usage()
{
	printf("\nusage:\n\tclient.exe <server ip> <port> <username>\n\n");
}

int check_input(struct client_env *p_env, int argc, char** argv)
{
	int ret_val = E_SUCCESS, res;
	int port;

	/* check number of arguments */
	if (argc != ARGC) {
		print_usage();
		return E_FAILURE;
	}

	/* check server ip */
	if (inet_addr(argv[1]) == INADDR_NONE) {
		printf("\n%s is not a valid ip addres", argv[1]);
		ret_val = E_FAILURE;
	}
	p_env->serv_ip = argv[1];

	/* check port number */
	port = strtol(argv[2], NULL, 10);
	if (!((port > 0) && (port < MAX_PORT))) {
		printf("\n%s is not a valid port", argv[2]);
		ret_val = E_FAILURE;
	}
	p_env->serv_port = port;

	/* check user name*/
	res = strlen(argv[3]) <= MAX_USERNAME_LEN;
	for (char *c = argv[3]; *c && res; c++)
		res = (isdigit(*c) || (isalpha(*c)));
	if (!res) {
		printf("\n'%s' is not a valid username. must contain numbers "
		       "and letters only, and be up to %d charachters long\n",
		        argv[3], MAX_USERNAME_LEN);
		ret_val = E_FAILURE;
	}
	p_env->username = argv[3];

	if (ret_val != E_SUCCESS)
		print_usage();

	return ret_val;
}

int client_init(struct client_env *p_env)
{
	DBG_TRACE_INIT(C, p_env->username);
	WSADATA	wsa_data;
	int res;

	/* WinSockApi startup */
	res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (res) {
		printf("Error: WSAStartup returnd with code 0x%X\n", res);
		p_env->last_error = E_WINSOCK;
		return STATE_EXIT;
	}

	/* set server address */
	p_env->skt                    = INVALID_SOCKET;
	p_env->server.sin_family      = AF_INET;
	p_env->server.sin_addr.s_addr = inet_addr(p_env->serv_ip);
	p_env->server.sin_port        = htons(p_env->serv_port);

	return STATE_CONNECT_ATTEMPT;
}

int cilent_send_msg(struct client_env *p_env, int type, char *param)
{
	DBG_TRACE_FUNC(C, p_env->username);
	struct msg *p_msg = NULL;
	int res;

	/* create message */
	p_msg = new_msg(type, param, NULL, NULL, NULL);
	if (p_msg == NULL)
		return E_STDLIB;

	/* send message */
	res = send_msg(p_env->skt, &p_msg);

	DBG_TRACE_MSG(C, p_env->username, p_msg);

	/* free message */
	free_msg(&p_msg);

	return res;
}

int client_recv_msg(struct msg **p_p_msg, struct client_env *p_env, int timeout_sec)
{
	DBG_TRACE_FUNC(C, p_env->username);
	int res;
	TIMEVAL tv = {0};

	tv.tv_sec = timeout_sec;
	res = recv_msg(p_p_msg, p_env->skt, &tv);
	if (res != E_SUCCESS)
		p_env->last_error = res;
	
	DBG_TRACE_MSG(C, p_env->username, *p_p_msg);

	return res;
}


int client_cleanup(struct client_env *p_env)
{
	DBG_TRACE_FUNC(C, p_env->username);
	int ret_val = p_env->last_error;

	if (WSACleanup()) {
		PRINT_ERROR(E_WINSOCK);
		ret_val = E_WINSOCK;
	}

	return ret_val;
}
