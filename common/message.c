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
 * messages MUST be created only by using new_msg()
 * and freed only by using free_msg(). static messages
 * are un-supporetd by this module, and may result with
 * un-expected behaviour of the program.
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

/*
 ==============================================================================
 * PRAGMAS
 ==============================================================================
 */

#define _CRT_SECURE_NO_WARNINGS // FIXME:
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
#include "tasks.h"

/*
 ==============================================================================
 * GLOBAL VARS
 ==============================================================================
 */

char *msg_type_2_str[MSG_MAX] = {
	[MSG_CLIENT_REQUEST]             = "CLIENT_REQUEST",
	[MSG_CLIENT_VERSUS]              = "CLIENT_VERSUS",
	[MSG_CLIENT_SETUP]               = "CLIENT_SETUP",
	[MSG_CLIENT_PLAYER_MOVE]         = "CLIENT_PLAYER_MOVE",
	[MSG_CLIENT_DISCONNECT]          = "CLIENT_DISCONNECT",
	[MSG_SERVER_MAIN_MENU]           = "SERVER_MAIN_MENU",
	[MSG_SERVER_APPROVED]            = "SERVER_APPROVED",
	[MSG_SERVER_DENIED]              = "SERVER_DENIED",
	[MSG_SERVER_INVITE]              = "SERVER_INVITE",
	[MSG_SERVER_SETUP_REQUEST]       = "SERVER_SETUP_REQUEST",
	[MSG_SERVER_PLAYER_MOVE_REQUEST] = "SERVER_PLAYER_MOVE_REQUEST",
	[MSG_SERVER_GAME_RESULTS]        = "SERVER_GAME_RESULTS",
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

int msg_buff_len(struct msg *p_msg)
{
	int buff_len = 0;
	
	/* type str length */
	buff_len += strlen(msg_type_2_str[p_msg->type]);

	/* params len including delimiters */
	if (p_msg->param_cnt) {
		buff_len += p_msg->param_cnt;
		for (int i = 0; i < p_msg->param_cnt; i++)
			buff_len += strlen(p_msg->param_lst[i]);
	}

	/* add 1 for newly requested '\n' at message end */
	return buff_len + 1;
}

int msg_2_buff(char *buff, struct msg *p_msg)
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

	/* as instructed in forum */
	buff[offset++] = '\n';

	return offset;
}

int new_msg_param(char **p_param_dst, char *param_src)
{
	/* null param is fine */
	if (!param_src)
		return E_SUCCESS;

	/* reset params */	
	*p_param_dst = NULL;

	/* allocate param mem */
	int mem_size = strlen(param_src) + 1;
	*p_param_dst = calloc(mem_size, sizeof(char));
	if (!*p_param_dst) {
		PRINT_ERROR(E_STDLIB);
		return E_STDLIB;
	}

	/* fill param */
	memcpy(*p_param_dst, param_src, mem_size - 1);

	return E_SUCCESS;
}

int buff_2_msg(char *buff, struct msg **p_p_msg)
{
	struct msg *p_msg = NULL;
	char *token = NULL;
	char *context = NULL;
	int idx, res = E_SUCCESS;

	*p_p_msg = NULL;

	/* allocate mem */
	p_msg = calloc(1, sizeof(*p_msg));
	if (!p_msg) {
		PRINT_ERROR(E_STDLIB);
		return E_STDLIB;
	}

	/* parse message type */
	p_msg->type = MSG_INVALID;
	token = strtok_s(buff, ":\n", &context);
	for (int i = MSG_MIN; i < MSG_MAX; i++) {
		if (strcmp(token, msg_type_2_str[i]) == 0) {
			p_msg->type = i;
			break;
		}
	}

	/* corrupt message */
	if (p_msg->type == MSG_INVALID) {
		free_msg(&p_msg);
		PRINT_ERROR(E_MESSAGE);
		return E_MESSAGE;
	}

	/* parse params */
	while(token = strtok_s(NULL, ";\n", &context)) {
		idx = p_msg->param_cnt;
		if (idx >= MSG_MAX_PARAMS) {
			free_msg(&p_msg);
			PRINT_ERROR(E_MESSAGE);
			return E_MESSAGE;
		}
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

struct msg *new_msg(int type, char *p0, char *p1, char *p2, char *p3)
{
	struct msg *p_msg = NULL;
	char *param_lst = NULL;
	int res = E_SUCCESS;

	/* allocate message */
	p_msg = calloc(1, sizeof(*p_msg));
	if (!p_msg) {
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
		free_msg(&p_msg);
		return NULL;
	}

	/* set params count */
	for (int i = 0; i < MSG_MAX_PARAMS && p_msg->param_lst[i]; i++)
		p_msg->param_cnt++;

	return p_msg;
}

void free_msg(struct msg **p_p_msg)
{
	/* sanity */
	if (!p_p_msg)
		return;
	if (!*p_p_msg)
		return;

	/* free message params mem */
	for (int i = 0; i < (*p_p_msg)->param_cnt; i++)
		free((*p_p_msg)->param_lst[i]);

	/* free message mem */
	free(*p_p_msg);
	*p_p_msg = NULL;
}

int send_msg(int skt, struct msg **p_p_msg)
{
	char *buffer = NULL;
	struct msg *p_msg = *p_p_msg;
	int ret_val = E_SUCCESS;
	int buff_len, bytes_sent = 0;
	int res;

	/* sanity */
	if (!p_msg)
		return E_FAILURE;

	/* do-while(0) for easy cleanup */
	do {
		/* allocate send buffer */
		buff_len = msg_buff_len(p_msg);
		buffer = calloc(buff_len + 1, sizeof(*buffer));
		if (!buffer) {
			PRINT_ERROR(E_STDLIB);
			ret_val = E_STDLIB;
			break;
		}

		/* fill buffer with messgae */
		msg_2_buff(buffer, p_msg);

		/* send message */
		while (bytes_sent < (buff_len + 1)) {

			res = send(skt, (buffer + bytes_sent), (buff_len + 1), 0);
			if (res == SOCKET_ERROR) {
				ret_val = E_WINSOCK;
				break;
			}

			if (ret_val != E_SUCCESS)
				break;
			
			bytes_sent += res;
			buff_len -= bytes_sent;
		}

	} while (0);

	/* cleanup */
	if (buffer)
		free(buffer);

	return ret_val;
}

int recv_msg(struct msg **p_p_msg, int skt, PTIMEVAL p_timeout)
{
	char buff[MSG_BUFF_MAX] = {0};
	int res, ret_val = E_SUCCESS;
	int msg_len;
	FD_SET readfs;

	FD_ZERO(&readfs);
	FD_SET(skt, &readfs);

	/* wait for message to arrive in socket */
	res = select(0, &readfs, NULL, NULL, p_timeout);
	if (res == 0)
		return E_TIMEOUT;
	else if (res == SOCKET_ERROR)
		return E_WINSOCK;

	/* peek untill got a full message, infdicated by
	 * EOL, then reed inly up to there, leaving next
	 * messages still in socket queue */
	do {
		res = recv(skt, buff, MSG_BUFF_MAX, MSG_PEEK);
		if (res == 0)
			return E_GRACEFUL;
		else if (res == SOCKET_ERROR)
			return E_WINSOCK;
		
		if (strchr(buff, '\n'))
			break;
	} while (1);

	/* recieve message */
	msg_len = strlen(buff);
	memset(buff, 0, MSG_BUFF_MAX);
	res = recv(skt, buff, msg_len + 1, 0);
	if (res == 0)
		return E_GRACEFUL;
	else if (res == SOCKET_ERROR)
		return E_WINSOCK;

	/* parse message */
	res = buff_2_msg(buff, p_p_msg);

	return res;
}

#if DBG_TRACE
char *dbg_trace_msg_2_str(struct msg *p_msg)
{
	char *p, *str = calloc(MSG_BUFF_MAX, sizeof(char)); // FIXME:
	if (!str) {
		PRINT_ERROR(E_INTERNAL);
		exit(E_FAILURE);
	}
	p = str;

	if (!p_msg) {
		sprintf(p, "\n\t\tNULL\n");
		p += strlen(p);
	} else {
		sprintf(p, "\n\t\ttype:     %s\n", msg_type_2_str[p_msg->type]);
		p += strlen(p);
		for (int i = 0; i < p_msg->param_cnt; i++) {
			sprintf(p, "\t\tparam[%d]: %s\n", i, p_msg->param_lst[i]);
			p += strlen(p);
		}
	}
	sprintf(p, "\n");
	return str;
}
#endif