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

char *msg_type_2_str[MSG_MAX] =
{
	[MSG_CLIENT_REQUEST]             = "CLIENT_REQUEST",
	[MSG_CLIENT_VERSUS]              = "CLIENT_VERSUS",
	[MSG_CLIENT_SETUP]               = "CLIENT_SETUP",
	[MSG_CLIENT_PLAYER_MOVE]         = "CLIENT_PLAYER_MOVE",
	[MSG_CLIENT_DISCONNECT]          = "CLIENT_DISCONNECT",
	[MSG_SERVER_MAIN_MENUE]          = "SERVER_MAIN_MENUE",
	[MSG_SERVER_APPROVED]            = "SERVER_APPROVED",
	[MSG_SERVER_DENIED]              = "SERVER_DENIED",
	[MSG_SERVER_INVITE]              = "SERVER_INVITE",
	[MSG_SERVER_SETUP_REQUEST]       = "SERVER_SETUP_REQUEST",
	[MSG_SERVER_PLAYER_MOVE_REQUEST] = "SERVER_PLAYER_MOVE_REQUEST",
	[MSG_SERVER_WIN]                 = "SERVER_WIN",
	[MSG_SERVER_DRAW]                = "SERVER_DRAW",
	[MSG_SERVER_NO_OPONENTS]         = "SERVER_NO_OPONENTS",
	[MSG_SERVER_OPPONENT_QUIT]       = "SERVER_OPPONENT_QUIT"
};

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
		return E_FAILURE;
	}

	/* socket for connection with server */
	p_env->cnct_skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p_env->cnct_skt == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_FAILURE;
	}

	/* set server addres */
	p_env->server.sin_family      = AF_INET;
	p_env->server.sin_addr.s_addr = inet_addr(p_env->args.serv_ip);
	p_env->server.sin_port        = htons(p_env->args.serv_port);

	/* connect socket with server */
	res = connect(p_env->cnct_skt, (PSOCKADDR)&p_env->server, sizeof(SOCKADDR));
	if(res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_FAILURE;
	}

	// dbg
	struct msg *p_msg = recv_msg(p_env->cnct_skt, TIMEOUT_SEC_DEFAULT);
	print_msg(p_msg);
	free_msg(&p_msg);

	return E_SUCCESS;
}

int client_cleanup(struct client_env *p_env)
{
	DBG_PRINT("client_cleanup\n");
	int ret_val = E_SUCCESS;

	if (p_env->cnct_skt != INVALID_SOCKET) {
		if (closesocket(p_env->cnct_skt) == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			ret_val = E_FAILURE;
		}
	}

	if (WSACleanup()) {
		PRINT_ERROR(E_WINSOCK);
		ret_val = E_FAILURE;
	}

	return ret_val;
}

//==============================================================================

struct msg *parse_buff_2_msg(char *buff)
{
	struct msg *p_msg = NULL;
	char *token = NULL;
	char *context;
	int idx, token_len;

	/* reset mssage struct */
	p_msg = calloc(1, sizeof(*p_msg));
	if (p_msg == NULL) {
		PRINT_ERROR(E_STDLIB);
		NULL;
	}

	/* parse message type */
	token = strtok_s(buff, ":", &context);
	for (int i = 0; i < MSG_MAX; i++) {
		if (strcmp(token, msg_type_2_str[i]) == 0) {
			p_msg->type = i;
			break;
		}
	}

	/* parse params */
	while(token = strtok_s(NULL, ";", &context)) {
		idx = p_msg->param_cnt;
		token_len = strlen(token);
		p_msg->param_lst[idx] = calloc(token_len + 1, sizeof(char));
		if (!p_msg->param_lst[idx]) {
			PRINT_ERROR(E_STDLIB);
			free_msg(&p_msg);
			return NULL;
		}
		memcpy(p_msg->param_lst[idx], token, token_len);
		p_msg->param_cnt++;
	}

	return p_msg;
}

void free_msg(struct msg **p_p_msg)
{
	/* sanity */
	if (!p_p_msg || !*p_p_msg)
		return;

	/* free message mem */
	struct msg *p_msg = *p_p_msg;
	for (int i = 0; i < p_msg->param_cnt; i++)
		free(p_msg->param_lst[i]);
	free(p_msg);
	p_p_msg = NULL;
}

void print_msg(struct msg *p_msg)
{
	printf("\n\tmsg:\n");
	if (!p_msg) {
		printf("\t\tNULL\n");
	} else {
		printf("\t\ttype:     %s\n", msg_type_2_str[p_msg->type]);
		for (int i = 0; i < p_msg->param_cnt; i++)
			printf("\t\tparam[%d]: %s\n", i, p_msg->param_lst[i]);
	}
	printf("\n");
}

struct msg *recv_msg(int skt, int timeout_sec)
{
	DBG_PRINT("recv_msg\n");
	char buff[100] = {0}; // FIXME:
	int res;
	int ret_val = E_SUCCESS;
	struct msg *p_msg = NULL;
	
	FD_SET readfs;
	FD_ZERO(&readfs);
	FD_SET(skt, &readfs);
	TIMEVAL time = {timeout_sec, 0};

	/* wait for message to arrive in socket */
	res = select(0, &readfs, NULL, NULL, &time);
	if (!res) {
		PRINT_ERROR(E_TIMEOUT);
		return NULL;
	} else if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return NULL;
	}

	/* recieve message */ // FIXME: partial recv
	res = recv(skt, buff, 100, 0);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return NULL;
	}

	/* parse message */
	p_msg = parse_buff_2_msg(buff);
	return p_msg;
}