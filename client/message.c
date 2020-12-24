/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client program
 *
 * message.c
 * 
 * this module handles the messages that are the base
 * of the communication protocol used by the server
 * and client for playing a game.
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

/*
 ==============================================================================
 * PRAGMAS
 ==============================================================================
 */

#pragma comment(lib, "ws2_32.lib")

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <string.h>
#include <stdio.h>
#include "winsock2.h"
#include "message.h"
#include "client_tasks.h"

/*
 ==============================================================================
 * GLOBAL VARS
 ==============================================================================
 */

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

int msg_len(struct msg *p_msg)
{
	int msg_len = 0;
	
	msg_len += strlen(msg_type_2_str[p_msg->type]);

	if (p_msg->param_cnt) {
		msg_len += p_msg->param_cnt;
		for (int i = 0; i < p_msg->param_cnt; i++)
			msg_len += strlen(p_msg->param_lst[i]);
	}

	DBG_PRINT("msg_len = %d\n", msg_len);
	return msg_len;
}

int print_msg_2_buff(char *buff, struct msg *p_msg)
{
	char *str = msg_type_2_str[p_msg->type];
	int offset = 0;

	/* copy message text into buffer */
	memcpy(buff, str, strlen(str));
	offset += strlen(str);

	/* copy params into buffer */
	for (int i = 0; i < p_msg->param_cnt; i++) {
		buff[offset++] = i ? ';' : ':';
		str = p_msg->param_lst[i];
		memcpy(buff + offset, str, strlen(str));
		offset += strlen(str);
	}

	DBG_PRINT("print_msg_2_buff: %s\n", buff);
	return offset;
}

struct msg *parse_buff_2_msg(char *buff)
{
	struct msg *p_msg = NULL;
	char *token = NULL;
	char *context = NULL;
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

int send_msg(int skt, struct msg *p_msg)
{
	char *buffer = NULL;
	int ret_val = E_FAILURE;
	int buff_len;
	int res;

	/* sanity */
	if (!p_msg)
		return ret_val;

	/* do-while(0) for easy cleanup */
	do {
		/* allocate send buffer */
		buff_len = msg_len(p_msg);
		buffer = calloc(buff_len + 1, sizeof(*buffer)); // FIXME: +1 is temporary for debug only
		if (buffer == NULL) {
			PRINT_ERROR(E_STDLIB);
			break;
		}

		/* fill buffer with messgae */
		print_msg_2_buff(buffer, p_msg);

		/* send message */
		res = send(skt, buffer, buff_len, 0);
		if (res == SOCKET_ERROR) { // FIXME: partial send
			PRINT_ERROR(E_WINSOCK);
			break;
		}

		/* message has been sent */
		ret_val = E_SUCCESS;

	} while (0);

	/* cleanup */
	if (buffer)
		free(buffer);

	return ret_val;
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