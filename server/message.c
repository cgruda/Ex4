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
#include "server_tasks.h"

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

int new_msg_param(char **p_param_dst, char *param_src)
{
	if (!param_src)
		return E_SUCCESS;
	
	int mem_size = strlen(param_src) + 1;

	*p_param_dst = calloc(mem_size, sizeof(char));
	if (!*p_param_dst) {
		PRINT_ERROR(E_STDLIB);
		return E_STDLIB;
	}

	memcpy(*p_param_dst, param_src, mem_size - 1);

	return E_SUCCESS;
}

int parse_buff_2_msg(char *buff, struct msg **p_p_msg)
{
	struct msg *p_msg = NULL;
	char *token = NULL;
	char *context = NULL;
	int idx, res = E_SUCCESS;

	*p_p_msg = NULL;

	/* reset mssage struct */
	p_msg = calloc(1, sizeof(*p_msg));
	if (p_msg == NULL) {
		PRINT_ERROR(E_STDLIB);
		return E_STDLIB;
	}

	/* parse message type */
	p_msg->type = MSG_INVALID;
	token = strtok_s(buff, ":", &context);
	for (int i = MSG_MIN; i < MSG_MAX; i++) {
		if (strcmp(token, msg_type_2_str[i]) == 0) {
			p_msg->type = i;
			break;
		}
	}

	/* corrupt message */
	if (p_msg->type == MSG_INVALID) {
		free(&p_msg);
		return E_MESSAGE;
	}

	/* parse params */
	while(token = strtok_s(NULL, ";", &context)) {
		idx = p_msg->param_cnt;
		res = new_msg_param(&p_msg->param_lst[idx], token);
		if (res != E_SUCCESS) {
			free_msg(&p_msg);
			return res;
		}
		p_msg->param_cnt++;
	}

	*p_p_msg = p_msg;

	return E_SUCCESS;
}

struct msg *new_message(int type, char *p0, char *p1, char *p2, char *p3)
{
	struct msg *p_msg = NULL;
	char *param_lst   = NULL;
	int res = E_SUCCESS;

	/* allocate message */
	p_msg = calloc(1, sizeof(*p_msg));
	if (p_msg == NULL) {
		PRINT_ERROR(E_STDLIB);
		return NULL;
	}

	/* set message type */
	p_msg->type = type;

	/* set message params */
	res |= new_msg_param(&p_msg->param_lst[0], p0);
	res |= new_msg_param(&p_msg->param_lst[1], p1);
	res |= new_msg_param(&p_msg->param_lst[2], p2);
	res |= new_msg_param(&p_msg->param_lst[3], p3);

	if (res != E_SUCCESS) {
		free(&p_msg);
		return NULL;
	}

	/* set params count */
	for(; p_msg->param_lst[p_msg->type]; p_msg->type++);

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
			ret_val = E_STDLIB;
			break;
		}

		/* fill buffer with messgae */
		print_msg_2_buff(buffer, p_msg);

		/* send message */
		res = send(skt, buffer, buff_len, 0);
		if (res == SOCKET_ERROR) { // FIXME: partial send
			PRINT_ERROR(E_WINSOCK);
			ret_val = E_WINSOCK;
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

int recv_msg(struct msg **p_p_msg, int skt, int timeout_sec)
{
	char buff[100] = {0}; // FIXME:
	int res;
	int ret_val = E_SUCCESS;
	
	FD_SET readfs;
	FD_ZERO(&readfs);
	FD_SET(skt, &readfs);
	TIMEVAL time = {timeout_sec, 0};

	/* wait for message to arrive in socket */
	res = select(0, &readfs, NULL, NULL, &time);
	if (!res) {
		PRINT_ERROR(E_TIMEOUT);
		return E_TIMEOUT;
	} else if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* recieve message */ // FIXME: partial recv
	res = recv(skt, buff, 100, 0);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* parse message */
	res = parse_buff_2_msg(buff, p_p_msg);

	return res;
}