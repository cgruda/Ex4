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
	case E_TIMEOUT:
		printf("Timeout Error\n");
		break;
	default:
		printf("Unknown Error\n");
	}
}

int check_input(struct client_env *p_env, int argc, char** argv)
{
	struct args *p_args = &p_env->args;
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
	p_args->serv_ip = argv[1];

	/* check port number */
	port = strtol(argv[2], NULL, 10);
	if (!((port > 0) && (port < MAX_PORT))) {
		printf("\n%s is not a valid port", argv[2]);
		ret_val = E_FAILURE;
	}
	p_args->serv_port = port;

	/* check user name*/
	res = strlen(argv[3]) <= MAX_USERNAME_LEN;
	for (char *c = argv[3]; *c && res; c++)
		res = (isdigit(*c) || (isalpha(*c)));
	if (!res) {
		printf("\n'%s' is not a valid username. must contain numbers "
		       "and letters only, and be up to %d charachters long",
		        argv[3], MAX_USERNAME_LEN);
		ret_val = E_FAILURE;
	}
	p_args->user_name = argv[3];

	if (ret_val != E_SUCCESS)
		print_usage();

	return ret_val;
}

int client_init(struct client_env *p_env)
{
	DBG_PRINT("client_init\n");
	WSADATA	wsa_data;
	int res;

	/* WinSockApi startup */
	res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (res) {
		printf("Error: WSAStartup returnd with code 0x%X\n", res);
		return E_WINSOCK;
	}

	/* socket for connection with server */
	p_env->cnct_skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p_env->cnct_skt == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* set server addres */
	p_env->server.sin_family      = AF_INET;
	p_env->server.sin_addr.s_addr = inet_addr(p_env->args.serv_ip);
	p_env->server.sin_port        = htons(p_env->args.serv_port);

	return E_SUCCESS;
}

int cilent_connect_to_game(struct client_env *p_env)
{
	struct msg *p_msg_tx = NULL;
	struct msg *p_msg_rx = NULL;
	int res;
	
	/* connect socket with server */
	res = connect(p_env->cnct_skt, (PSOCKADDR)&p_env->server, sizeof(SOCKADDR));
	if(res == SOCKET_ERROR) {
		UI_PRINT(UI_CONNECT_FAIL, p_env->args.serv_ip, p_env->args.serv_port);
		return S_CONNECT_FAILURE;
		// PRINT_ERROR(E_WINSOCK);
		// return E_WINSOCK;
	}

	UI_PRINT(UI_CONNECT_PRE, p_env->args.serv_ip, p_env->args.serv_port);

	/* create client_request */
	p_msg_tx = new_message(MSG_CLIENT_REQUEST, p_env->username, NULL, NULL, NULL);
	if (p_msg_tx == NULL)
		return E_STDLIB;

	/* send client request */
	res = send_msg(p_env->cnct_skt, p_msg_tx);
	if (res != E_SUCCESS) {
		free_msg(&p_msg_tx);
		return res;
	}
	free_msg(&p_msg_tx);
	
	/* recieve server answer */
	res = recv_msg(&p_msg_rx, p_env->cnct_skt, MSG_TIMEOUT_SEC_DEFAULT);
	switch (res) {
	case E_SUCCESS:
		break;
	case E_TIMEOUT:
		UI_PRINT(UI_CONNECT_FAIL, p_env->args.serv_ip, p_env->args.serv_port);
		return S_CONNECT_FAILURE;
	default:
		return res;
	}

	/* interpet server response */
	switch (p_msg_rx->type)
	{
	case MSG_SERVER_APPROVED:
		res = S_CONNECT_SUCCESS;
		break;
	case MSG_SERVER_DENIED:
		UI_PRINT(UI_CONNECT_DENY, p_env->args.serv_ip, p_env->args.serv_port);
		res = S_CONNECT_FAILURE;

		break;
	default:
		res = S_UNDEFINED_STATE;
		break;
	}

	free(p_msg_rx);

	return res;
}

int client_cleanup(struct client_env *p_env)
{
	DBG_PRINT("client_cleanup\n");
	int ret_val = E_SUCCESS;

	if (p_env->cnct_skt != INVALID_SOCKET) {
		if (closesocket(p_env->cnct_skt) == SOCKET_ERROR) {
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
