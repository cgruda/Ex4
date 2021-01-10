/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client program
 *
 * client_tasks.c
 * 
 * this module hols clients tasks
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS

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
#include "client_fsm.h"
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
	if (argc != CLIENT_ARGC) {
		print_usage();
		return E_FAILURE;
	}

	/* check server ip */
	if (inet_addr(argv[1]) == INADDR_NONE) {
		printf("\n%s is not a valid ip addres", argv[1]);
		ret_val = E_FAILURE;
	}
	p_env->server_ip = argv[1];

	/* check port number */
	port = strtol(argv[2], NULL, 10);
	if (!((port > 0) && (port < MAX_PORT))) {
		printf("\n%s is not a valid port", argv[2]);
		ret_val = E_FAILURE;
	}
	p_env->server_port = port;

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
	WSADATA	wsa_data;
	int res;

	/* WinSockApi startup */
	res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (res) {
		printf("Error: WSAStartup returnd with code 0x%X\n", res);
		p_env->last_err = E_WINSOCK;
		return CLIENT_FSM_EXIT;
	}

	/* set server address */
	p_env->skt                    = INVALID_SOCKET;
	p_env->server.sin_family      = AF_INET;
	p_env->server.sin_addr.s_addr = inet_addr(p_env->server_ip);
	p_env->server.sin_port        = htons(p_env->server_port);

	return CLIENT_FSM_CONNECT;
}

int client_send_msg(struct client_env *p_env, int type, char *param)
{
	struct msg *p_msg = NULL;
	int res;

	/* create message */
	p_msg = new_msg(type, param, NULL, NULL, NULL);
	if (p_msg == NULL)
		return E_STDLIB;

	/* send message */
	res = send_msg(p_env->skt, &p_msg);

	/* free message */
	free_msg(&p_msg);

	return res;
}

int client_recv_msg(struct msg **p_p_msg, struct client_env *p_env, int timeout_sec)
{
	int res;
	TIMEVAL tv = {0};

	/* set timeout and call recieve */
	tv.tv_sec = timeout_sec;
	res = recv_msg(p_p_msg, p_env->skt, &tv);

	return res;
}

int client_cleanup(struct client_env *p_env)
{
	int ret_val = p_env->last_err;

	if (WSACleanup()) {
		PRINT_ERROR(E_WINSOCK);
		ret_val = E_WINSOCK;
	}

	return ret_val;
}

bool client_game_input_get(char *buff)
{
	/* scan stdin */
	scanf_s(" %s", buff, 5);

	/* check validity */
	if (strlen(buff) < 4)
		return false;

	/* check validity */
	for (int i = 0; i < 4; i++)
		for (int j = i + 1; j < 4; j++)
			if ((buff[i] == buff[j]) || buff[i] < '0' || buff[i] > '9')
				return false;

	return true;
}
