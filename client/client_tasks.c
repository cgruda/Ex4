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

#include <windows.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "client_tasks.h"
#include "client_flow.h"
#include "message.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

void print_usage()
{
	printf("\nusage:\n\tclient.exe <server ip> <port> <username>\n\n");
}

void print_error(int err_val)
{
	printf("Error: ");

	/* print relevant error */
	switch (err_val) {
	case E_STDLIB:
		printf("errno = %d\n", errno);
		break;
	case E_WINAPI:
		printf("WinAPI Error 0x%X\n", GetLastError());
		break;
	case E_WINSOCK:
		printf("Windows Socket Error %d\n", WSAGetLastError());
		break;
	case E_INTERNAL:
		printf("Internal Error\n");
		break;
	case E_MESSAGE:
		printf("Message Error\n");
		break;
	case E_INPUT:
		printf("Input Error\n");
		break;
	case E_TIMEOUT:
		printf("Timeout Error\n");
		break;
	default:
		printf("Unknown Error 0x%02X\n", err_val);
	}
}

int check_input(struct client_env *p_env, int argc, char** argv)
{
	DBG_FUNC_STAMP();
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
	DBG_FUNC_STAMP();
	WSADATA	wsa_data;
	int res;

	/* WinSockApi startup */
	res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (res) {
		printf("Error: WSAStartup returnd with code 0x%X\n", res);
		p_env->last_error = E_WINSOCK;
		return STATE_EXIT;
	}

	/* set server addres */
	p_env->skt                    = INVALID_SOCKET;
	p_env->server.sin_family      = AF_INET;
	p_env->server.sin_addr.s_addr = inet_addr(p_env->serv_ip);
	p_env->server.sin_port        = htons(p_env->serv_port);

	return STATE_CONNECT_ATTEMPT;
}


// client send message wrapper
int cilent_send_msg(struct client_env *p_env, int type, char *param)
{
	DBG_FUNC_STAMP();
	struct msg *p_msg = NULL;
	int res;

	/* create message */
	p_msg = new_message(type, param, NULL, NULL, NULL);
	if (p_msg == NULL)
		return E_STDLIB;

	/* send message */
	res = send_msg(p_env->skt, &p_msg);

	/* free message */
	free_msg(&p_msg);

	return res;
}

int client_cleanup(struct client_env *p_env)
{
	DBG_FUNC_STAMP();
	int ret_val = p_env->last_error;

	if (p_env->skt != INVALID_SOCKET) {
		if (closesocket(p_env->skt) == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			ret_val = E_WINSOCK;
		}
	}

	if (WSACleanup()) {
		PRINT_ERROR(E_WINSOCK);
		ret_val = E_WINSOCK;
	}

	return ret_val;
}
