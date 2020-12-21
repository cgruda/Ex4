// /**
//  * ISP_HW_4_2020
//  * Bulls & Cows
//  * client side
//  *
//  * client_tasks.c
//  * 
//  * by: Chaim Gruda
//  *     Nir Beiber
//  */

// /*
//  ==============================================================================
//  * INCLUDES
//  ==============================================================================
//  */
// #include <windows.h>
// #include <errno.h>
// #include <string.h>
// #include <stdio.h>
// #include <stdbool.h>
// #include "client_tasks.h"

// const char *msg_type_2_str[MSG_MAX] =
// {
// 	[MSG_CLIENT_REQUEST]             = "CLIENT_REQUEST",
// 	[MSG_CLIENT_VERSUS]              = "CLIENT_VERSUS",
// 	[MSG_CLIENT_SETUP]               = "CLIENT_SETUP",
// 	[MSG_CLIENT_PLAYER_MOVE]         = "CLIENT_PLAYER_MOVE",
// 	[MSG_CLIENT_DISCONNECT]          = "CLIENT_DISCONNECT",
// 	[MSG_SERVER_MAIN_MENUE]          = "SERVER_MAIN_MENUE",
// 	[MSG_SERVER_APPROVED]            = "SERVER_APPROVED",
// 	[MSG_SERVER_DENIED]              = "SERVER_DENIED",
// 	[MSG_SERVER_INVITE]              = "SERVER_INVITE",
// 	[MSG_SERVER_SETUP_REQUEST]       = "SERVER_SETUP_REQUEST",
// 	[MSG_SERVER_PLAYER_MOVE_REQUEST] = "SERVER_PLAYER_MOVE_REQUEST",
// 	[MSG_SERVER_WIN]                 = "SERVER_WIN",
// 	[MSG_SERVER_DRAW]                = "SERVER_DRAW",
// 	[MSG_SERVER_NO_OPONENTS]         = "SERVER_NO_OPONENTS",
// 	[MSG_SERVER_OPPONENT_QUIT]       = "SERVER_OPPONENT_QUIT"
// };

// /*
//  ==============================================================================
//  * FUNCTION DEFENITIONS
//  ==============================================================================
//  */
// int my_atoi(char *str, int *p_result)
// {
// 	if (!str || !p_result)
// 		return E_FAILURE;

// 	for (int i = 0; i < (int)strlen(str); ++i)
// 	{
// 		if (!isdigit(str[i]))
// 		{
// 			if (i == 0)
// 			{
// 				if ((str[i] == '-') || (str[i] == '+'))
// 					continue;
// 			}

// 			*p_result = 0;
// 			return E_FAILURE;
// 		}
// 	}

// 	*p_result = strtol(str, NULL, 10);

// 	if (errno == ERANGE)
// 	{
// 		*p_result = 0;
// 		return E_FAILURE;
// 	}

// 	return E_SUCCESS;
// }

// //==============================================================================

// void print_usage()
// {
// 	printf("\nusage:\n\tclient.exe <server ip> <port> <username>\n\n");
// }

// //==============================================================================

// void print_error(int err_val, char *err_msg)
// {
// 	printf("Error: ");

// 	// print relevant error
// 	switch (err_val) {
// 	case E_STDLIB:
// 		printf("errno = %d\n", errno);
// 		break;
// 	case E_WINAPI:
// 		printf("WinAPI Error 0x%X\n", GetLastError());
// 		break;
// 	case E_WINSOCK:
// 		printf("WSA Error 0x%X\n", WSAGetLastError());
// 		break;
// 	case E_INTERNAL:
// 		if (err_msg) {
// 			printf("%s\n", err_msg);
// 		}
// 	default:
// 		printf("Unknown Error\n");
// 	}
// }

// //==============================================================================

// int check_input(struct client_env *p_env, int argc, char** argv)
// {
// 	struct args *p_args = &p_env->args;
// 	int ret_val = E_SUCCESS;

// 	// check number of arguments
// 	if (argc != ARGC)
// 	{
// 		print_usage();
// 		return E_FAILURE;
// 	}

// 	// check server ip
// 	p_args->server_addr = inet_addr(argv[1]);
// 	if (p_args->server_addr == INADDR_NONE)
// 	{
// 		printf("\n%s is not a valid IP addres", argv[1]);
// 		ret_val = E_FAILURE;
// 	}

// 	// check port number
// 	p_args->server_port = strtol(argv[2], NULL, 10);
// 	if (!((p_args->server_port > 0) && (p_args->server_port < 65536))) // FIXME:
// 	{
// 		printf("\n%s is not a valid port", argv[2]);
// 		ret_val = E_FAILURE;
// 	}

// 	p_args->user_name = argv[3];

// 	if (ret_val != E_SUCCESS)
// 		print_usage();

// 	return ret_val;
// }

// //==============================================================================

// int client_init(struct client_env *p_env)
// {
// 	WSADATA	wsaData;
// 	int res;

// 	// WinSockApi statup
// 	if (res = WSAStartup(MAKEWORD(2, 2), &wsaData)) {
// 		printf("Error: WSAStartup returnd with code 0x%X\n", res);
// 		return E_FAILURE;
// 	}

// 	// create socket
// 	if (p_env->skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) < 0) {
// 		PRINT_ERROR(E_WINSOCK, E_MSG_NONE);
// 		return E_FAILURE;
// 	}

// 	// server addres
// 	p_env->server.sin_family      = AF_INET;
// 	p_env->server.sin_addr.s_addr = p_env->args.server_addr;
// 	p_env->server.sin_port        = p_env->args.server_port;

// 	// connect socket
// 	if(connect(p_env->skt, (struct sockaddr*)&p_env->server, sizeof(struct sockaddr))) {
// 		PRINT_ERROR(E_WINSOCK, E_MSG_NONE);
// 		return E_FAILURE;
// 	}

// 	return E_SUCCESS;
// }

// //==============================================================================

// int client_cleanup(struct client_env *p_env)
// {
// 	int ret_val = E_SUCCESS;
// 	int skt;

// 	if (p_env->skt != INVALID_SOCKET) {
// 		if (closesocket(p_env->skt) < 0) {
// 			PRINT_ERROR(E_WINSOCK, E_MSG_NONE);
// 			ret_val = E_FAILURE;
// 		}
// 	}

// 	if (WSACleanup())
// 	{
// 		PRINT_ERROR(E_WINSOCK, E_MSG_NONE);
// 		ret_val = E_FAILURE;
// 	}

// 	return ret_val;
// }

// //==============================================================================

// int msglen(struct msg *p_msg)
// {
// 	int msg_len = 0;
	
// 	msg_len += strlen(msg_type_2_str[p_msg->type]);

// 	if (p_msg->param_cnt) {
// 		msg_len += 1 + (p_msg->param_cnt - 1); // ':' and ';'
// 		for (int i = 0; i < p_msg->param_cnt; i++)
// 			msg_len += strlen(p_msg->param_lst[i]);
// 	}

// 	return msg_len;
// }

// //==============================================================================

// int build_msg(char *buff, struct msg *p_msg)
// {
// 	int offset = 0;
// 	char *str = msg_type_2_str[p_msg->type];

// 	// copy message text into buffer
// 	memcpy(buff, str, strlen(str));
// 	offset += strlen(str);

// 	// copy params into buffer
// 	for (int i = 0; i < p_msg->param_cnt; i++) {
// 		buff[offset++] = i ? ';' : ':';
// 		str = p_msg->param_lst[i];
// 		memcpy(buff + offset, str, strlen(str));
// 		offset += strlen(str);
// 	}

// 	// return buffer length
// 	return offset;
// }

// //==============================================================================

// int client_send_msg(struct client_env *p_env, struct msg *p_msg)
// {
// 	PSOCKADDR p_server = (PSOCKADDR)&p_env->server;
// 	SOCKET skt = p_env->skt;
// 	char *buff = NULL;
// 	int ret_val = E_SUCCESS;
// 	int buff_len;

// 	// do-wile(0) for easy cleanup
// 	do {
// 		// allocate buffer for sending message
// 		buff_len = msglen(p_msg);
// 		if ((buff = calloc(buff_len, sizeof(*buff))) == NULL) {
// 			PRINT_ERROR(E_STDLIB, E_MSG_NONE);
// 			ret_val = E_FAILURE;
// 			break;
// 		}

// 		// fill buffer with messgae
// 		build_msg(buff, p_msg);

// 		// send message // FIXME: partial send
// 		if (sendto(p_env->skt, buff, strlen(buff), 0, p_server, sizeof(*p_server)) == SOCKET_ERROR) {
// 			PRINT_ERROR(E_WINSOCK, E_MSG_NONE);
// 			ret_val = E_FAILURE;
// 		}
// 	} while (0);

// 	if (buff)
// 		free(buff);

// 	return ret_val;
// }

// //==============================================================================

// int parse_buff_to_msg(char *buff, struct msg *p_msg)
// {
// 	char *str;
// 	int idx;
// 	int ret_val = E_SUCCESS;

// 	// reset mssage struct
// 	memset(p_msg, 0, sizeof(*p_msg));

// 	// parse message type
// 	for (int i = 0; i < MSG_MAX; i++) {
// 		str = msg_type_2_str[i];
// 		if (strncmp(str, buff, strlen(str)) == 0) {
// 			p_msg->type = i;
// 			break;
// 		}
// 	}

// 	// parse params
// 	while(str = strtok_s(buff + 1 + strlen(str), ";:", NULL)) {
// 		idx = p_msg->param_cnt;
// 		p_msg->param_lst[idx] = calloc(strlen(str) + 1, sizeof(char));
// 		if (!p_msg->param_lst[idx]) {
// 			PRINT_ERROR(E_STDLIB, E_MSG_NONE);
// 			ret_val = E_FAILURE;
// 			break;
// 		}
// 		p_msg->param_cnt++;
// 	}

// 	return ret_val;
// }

// //==============================================================================

// void free_msg(struct msg **p_p_msg)
// {
// 	// sanity
// 	if (!p_p_msg || !*p_p_msg)
// 		return;

// 	// free message mem
// 	struct msg *p_msg = *p_p_msg;
// 	for (int i = 0; i < p_msg->param_cnt; i++)
// 		free(p_msg->param_lst[i]);
// 	free(p_msg);
// 	p_p_msg = NULL;
// }

// //==============================================================================

// int client_recv_msg(struct client_env *p_env, struct msg *p_msg, int sec)
// {
// 	SOCKET skt = p_env->skt;
// 	char buff[100]; // FIXME:
// 	int res;
// 	int ret_val = E_SUCCESS;
// 	TIMEVAL time = {sec, 0};
// 	int buff_len;

// 	FD_SET(skt, &p_env->read_fds);
	
// 	res = select(skt + 1, &p_env->read_fds, NULL, NULL, &time);
// 	if (!res) {
// 		PRINT_ERR(E_INTERNAL, "select timout");
// 		return E_FAILURE;
// 	} else if (res == SOCKET_ERROR) {
// 		PRINT_ERR(E_WINSOCK, E_MSG_NONE);
// 		return E_FAILURE;
// 	}

// 	// recieve message // FIXME: partial recv
// 	if (recv(p_env->skt, buff, 100, 0) == SOCKET_ERROR) {
// 		PRINT_ERROR(E_WINSOCK, E_MSG_NONE);
// 		return E_FAILURE;
// 	}

// 	// parse message
// 	if (parse_buff_to_msg(buff, p_msg)) {
// 		PRINT_ERROR(E_INTERNAL, "parse msg failure\n");
// 		return E_FAILURE;
// 	}

// 	free(buff);

// 	return ret_val;
// }