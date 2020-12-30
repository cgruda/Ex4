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
#define _CRT_SECURE_NO_WARNINGS		// FIXME:

#pragma comment(lib, "ws2_32.lib")
#include "winsock2.h"

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "server_tasks.h"
#include "server_fsm.h"
#include "message.h"
#include "tasks.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

void print_usage()
{
	printf("\nusage:\n\tserver.exe <server ip> <port>\n\n");
}

int check_input(struct serv_env *p_env, int argc, char **argv)
{
	int ret_val = E_SUCCESS;
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

	if (ret_val != E_SUCCESS)
		print_usage();

	return ret_val;
}

int serv_quit_init(struct serv_env *p_env)
{
	int res;
	
	/* create file handle for stdin */
	p_env->h_file_stdin = CreateFileA(PATH_STDIN,              /* standard input    */
					  GENERIC_READ,         /* we want to read   */
					  FILE_SHARE_READ,      /* others may use    */
					  NULL,                 /* default security  */
					  OPEN_EXISTING,        /* stdin exists      */
					  FILE_FLAG_OVERLAPPED, /* asynchronous read */
					  NULL);                /* no template       */
	if (p_env->h_file_stdin == INVALID_HANDLE_VALUE) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* create event handle for overlapped stdin read */
	p_env->olp_stdin.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (p_env->olp_stdin.hEvent == NULL) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* read stdin asynchronously for exit command.
	 * asynchronous read must always return false.
	 * error is detected by checking WSA last error.
	 * io_pending designates i/o pending completion,
	 * and is thus not treated as an error. */
	res = ReadFile(p_env->h_file_stdin, p_env->buffer, 4, NULL, &p_env->olp_stdin); // FIXME: 4
	if ((res) || (WSAGetLastError() != ERROR_IO_PENDING)) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	return E_SUCCESS;
}

int serv_comm_init(struct serv_env *p_env)
{
	int res;

	/* windows socket api init */
	res = WSAStartup(MAKEWORD(2, 2), &p_env->wsa_data);
	if (res) {
		printf("Error: WSAStartup returnd with code 0x%X\n", res);
		return E_WINSOCK;
	}

	/* create socket */
	p_env->serv_skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p_env->serv_skt == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* set server address */
	p_env->server.sin_family      = AF_INET;
	p_env->server.sin_addr.s_addr = inet_addr(p_env->serv_ip);
	p_env->server.sin_port        = htons(p_env->serv_port);

	/* bind socket to ip and port */
	res = bind(p_env->serv_skt, (PSOCKADDR)&p_env->server, sizeof(SOCKADDR));
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* place socket in listening state */
	res = listen(p_env->serv_skt, SOMAXCONN);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	return E_SUCCESS;
}

serv_ctrl_init(struct serv_env *p_env)
{
	/* semaphore to restrict max players */
	p_env->h_players_smpr = CreateSemaphore(NULL, MAX_PLAYERS, MAX_PLAYERS, NULL);
	if (p_env->h_players_smpr == NULL) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	p_env->h_abort_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (p_env->h_players_smpr == NULL) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	p_env->h_game_mtx = CreateMutex(NULL, FALSE, NULL);
	if (p_env->h_game_mtx == NULL) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	p_env->thread_bitmap = THREAD_BITMAP_INIT_MASK; // FIXME: allow three threads to be created

	return E_SUCCESS;
}


int server_init(struct serv_env *p_env)
{
	DBG_TRACE_INIT(S, SERVER);
	int res;

	/* quit logic init */
	res = serv_quit_init(p_env);
	if (res != E_SUCCESS)
		return res;

	/* control logic init */
	res = serv_ctrl_init(p_env);
	if (res != E_SUCCESS)
		return res;

	/* communication logic init */
	res = serv_comm_init(p_env);
	if (res != E_SUCCESS)
		return res;

	DBG_TRACE_FUNC(S, SERVER);
	return E_SUCCESS;
}

bool server_quit(struct serv_env *p_env) // TODO: split 2
{
	DWORD wait_code;
	int res;

	/* check stdin input event */
	wait_code = WaitForSingleObject(p_env->olp_stdin.hEvent, 1);
	switch (wait_code) {
	case WAIT_OBJECT_0:
		break;
	case WAIT_TIMEOUT:
		p_env->last_err = E_TIMEOUT;
		return false;
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
		/* fall through */
	default:
		p_env->last_err = E_WINAPI;
		return true;
	}

	/* if command is NOT "exit" - reset read event
	 * and start a new asynchronous read. else */
	if (strncmp(p_env->buffer, "exit", 5)) { // FIXME: exitXXX...
		if (!ResetEvent(p_env->olp_stdin.hEvent)) {
			PRINT_ERROR(E_WINAPI);
			p_env->last_err = E_WINAPI;
			return true;
		}
		/* read stdin asynchronously for exit command.
	 	 * asynchronous read must always return false.
	 	 * error is detected by checking WSA last error.
	 	 * io_pending designates i/o pending completion,
	 	 * and is thus not treated as an error. */
		res = ReadFile(p_env->h_file_stdin, p_env->buffer, 4, NULL, &p_env->olp_stdin);
		if ((res) || (WSAGetLastError() != ERROR_IO_PENDING)) {
			PRINT_ERROR(E_WINAPI);
			p_env->last_err = E_WINAPI;
			return true;
		}
		return false;
	} else {
		DBG_TRACE_STR(S, SERVER, "server_quit!");	
		return true;
	}
}


int server_destroy_clients(struct serv_env *p_env)
{
	DBG_TRACE_FUNC(S, SERVER);
	int res;
	
	/* set abort event */
	if (!SetEvent(p_env->h_abort_evt)) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	res = server_check_thread_status(p_env, 200);

	return res;
}



int serv_clnt_connect(struct serv_env *p_env)
{
	int res;
	int new_skt;
	struct client *p_client = NULL;
	HANDLE *p_h_clnt_thread = NULL;
	FD_SET readfs;
	DWORD idx;
	TIMEVAL tv = {0, 1000}; // FIXME:

	// char buffer[100];

	FD_ZERO(&readfs);
	FD_SET(p_env->serv_skt, &readfs);

	/* wait for socket to be signald */
	res = select(p_env->serv_skt + 1, &readfs, NULL, NULL, &tv);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* socket is signald */
	if (res) {

		/* check if more connections can be accepted */
		if (!BitScanForward(&idx, ~p_env->thread_bitmap)) {
			DBG_TRACE_STR(S, SERVER, "max TCP connections! incoming connection refused");
			return E_SUCCESS;
		}

		/* accept incoming connection */
		new_skt = accept(p_env->serv_skt, NULL, NULL);
		if (new_skt == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			return E_WINSOCK;
		}

		/* set client params */
		p_client = &p_env->client[idx];
		memset(p_client, 0, sizeof(*p_client));
		p_client->id = idx;
		p_client->skt = new_skt;
		p_client->p_env = p_env;

		/* create new thread to handle connection */
		p_h_clnt_thread = &p_env->h_clnt_thread[idx];
		*p_h_clnt_thread = CreateThread(NULL, 0, clnt_thread, (LPVOID)p_client, 0, NULL);
		if (!*p_h_clnt_thread) {
			PRINT_ERROR(E_WINAPI);
			if (closesocket(p_client->skt) == SOCKET_ERROR) {
				PRINT_ERROR(E_WINSOCK);
				return E_WINSOCK;
			}
			return E_WINAPI;
		}

		/* mark thread handle as taken */
		SET_BIT(p_env->thread_bitmap, idx);
		DBG_TRACE_STR(S, SERVER, "start TCP connection %d", idx);
	}
	
	return E_SUCCESS;
}


int server_check_thread_status(struct serv_env *p_env, int ms)
{
	DWORD wait_code;
	HANDLE *p_h_clnt_thread = NULL;

	for (int idx = 0; idx < MAX_CONNECTIONS; idx++) {

		if (!TEST_BIT(p_env->thread_bitmap, idx))
			continue;
		
		p_h_clnt_thread = &p_env->h_clnt_thread[idx];
		wait_code = WaitForSingleObject(*p_h_clnt_thread, ms);
		switch (wait_code)
		{
		case WAIT_TIMEOUT:
			break;
		case WAIT_OBJECT_0:
			if (!CloseHandle(*p_h_clnt_thread)) {
				PRINT_ERROR(E_WINAPI);
				return E_WINAPI;
			}
			DBG_TRACE_STR(S, SERVER, "end TCP connection   %d", idx);
			CLR_BIT(p_env->thread_bitmap, idx);
			break;
		case WAIT_FAILED:
			/* fall through */
		default:
			PRINT_ERROR(E_WINAPI);
			return E_WINAPI;
		}
	}

	return E_SUCCESS;
}


bool server_check_abort(struct serv_env *p_env)
{
	DWORD wait_code;

	wait_code = WaitForSingleObject(p_env->h_abort_evt, 0);
	switch (wait_code)
	{
	case WAIT_TIMEOUT:
		return false;
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
	default:
		return true;
	}
}


int server_cleanup(struct serv_env *p_env)
{
	DBG_TRACE_FUNC(S, SERVER);
	int ret_val = p_env->last_err;

	if (p_env->olp_stdin.hEvent) {
		if (!CloseHandle(p_env->olp_stdin.hEvent)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (p_env->h_file_stdin) {
		if (!CloseHandle(p_env->h_file_stdin)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (p_env->serv_skt != INVALID_SOCKET) {
		if (closesocket(p_env->serv_skt) == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			ret_val = E_WINSOCK;
		}
	}

	if (p_env->h_players_smpr) {
		if (!CloseHandle(p_env->h_players_smpr)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (p_env->h_abort_evt) {
		if (!CloseHandle(p_env->h_abort_evt)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (p_env->h_game_mtx) {
		if (!CloseHandle(p_env->h_game_mtx)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (WSACleanup()) {
		PRINT_ERROR(E_WINSOCK);
		ret_val = E_WINSOCK;
	}

	return ret_val;
}

int server_send_msg(struct client *p_clnt, int type, char *p0, char *p1, char *p2, char *p3)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	struct msg *p_msg = NULL;
	int res;

	/* create message */
	p_msg = new_msg(type, p0, p1, p2, p3);
	if (p_msg == NULL)
		return E_STDLIB;

	/* send message */
	res = send_msg(p_clnt->skt, &p_msg);
	DBG_TRACE_MSG(T, p_clnt->username, p_msg);

	/* free message */
	free_msg(&p_msg);

	return res;
}

int server_recv_msg(struct client *p_clnt, struct msg **p_p_msg, int timeout_sec)
{
	if (p_clnt->connected)
		DBG_TRACE_FUNC(T, p_clnt->username);
	
	TIMEVAL tv;
	int res = E_SUCCESS;

	int incerments = timeout_sec * (SEC2MS / (MSG_TIME_INCERMENT_USEC / MS2US));

	while (incerments--) {

		if (server_check_abort(p_clnt->p_env))
			return E_INTERNAL;

		tv.tv_sec  = 0;
		tv.tv_usec = MSG_TIME_INCERMENT_USEC;
		res = recv_msg(p_p_msg, p_clnt->skt, &tv);
		if (res == E_TIMEOUT)
			continue;
		else
			break;
	}

	if (p_clnt->connected)
		DBG_TRACE_MSG(T, p_clnt->username, *p_p_msg);

	return res;
}






enum player_position
{
	SLAVE,
	MASTER,
};





int game_session_lock(HANDLE *mtx)
{
	DWORD wait_code;

	wait_code = WaitForSingleObject(*mtx, 5000);
	switch (wait_code) {
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	case WAIT_OBJECT_0:
		return E_SUCCESS;
	case WAIT_TIMEOUT:
		return E_TIMEOUT;
	default:
		return E_FAILURE;
	}
}


int game_session_release(HANDLE *mtx)
{
	int res;

	res = ReleaseMutex(*mtx);
	if (!res) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	return E_SUCCESS;
}



int destroy_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res = E_SUCCESS;
	struct game *p_game = &p_clnt->p_env->game;

	res = game_session_lock(&p_clnt->p_env->h_game_mtx);
	if (res != E_SUCCESS)
		return res;

	if (!CloseHandle(p_game->h_play_evt[0])) {
		PRINT_ERROR(E_WINAPI);
		res = E_WINAPI;
	}
	
	if (!CloseHandle(p_game->h_play_evt[1])) {
		PRINT_ERROR(E_WINAPI);
		res = E_WINAPI;
	}

	if (!DeleteFileA(PATH_GAME_SESSION)) {
		PRINT_ERROR(E_WINAPI);
		res = E_WINAPI;
	}

	p_game->valid = false;
	memset(p_game, 0, sizeof(*p_game));

	res |= game_session_release(&p_clnt->p_env->h_game_mtx);

	return res;
}


int leave_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	int position = p_clnt->position;
	struct game *p_game = &p_clnt->p_env->game;

	p_clnt->play_evt = NULL;
	p_clnt->playing = false;

	return E_SUCCESS;
}


int game_session_end(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	int res = E_SUCCESS;

	struct game *p_game = &p_clnt->p_env->game;
	int position = p_clnt->position;

	if (position == MASTER) {
		leave_game(p_clnt);
		res = destroy_game(p_clnt);
	} else {
		leave_game(p_clnt);
	}

	return res;
}





int join_game(struct client *p_clnt, int position)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	struct game *p_game = &p_clnt->p_env->game;

	p_clnt->position = position;
	p_clnt->play_evt = &p_game->h_play_evt[position];
	p_clnt->playing = true;

	return E_SUCCESS;
}


int create_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	struct game *p_game = &p_clnt->p_env->game;
	HANDLE h_file = NULL;
	int res;

	do {
		p_game->h_play_evt[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!p_game->h_play_evt[0]) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		p_game->h_play_evt[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!p_game->h_play_evt[1]) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		/* create session file */
		h_file = CreateFileA(PATH_GAME_SESSION,     /* GameSession path    */
				     0,                     /* no access needed    */
				     0,                     /* no share            */
				     NULL,                  /* default security    */
				     CREATE_ALWAYS,         /* create file         */
				     FILE_ATTRIBUTE_NORMAL, /* asynchronous read   */
				     NULL);                 /* no template         */
		if (!h_file) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}
	
		if (!CloseHandle(h_file)) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		res = E_SUCCESS;

	} while (0);

	if (res != E_SUCCESS) { // FIXME:
		CloseHandle(p_game->h_play_evt[0]);
		CloseHandle(p_game->h_play_evt[1]);
		CloseHandle(h_file);
		return res;
	}

	p_game->valid = true;
	return E_SUCCESS;
}

int game_session_start(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	int res = E_SUCCESS;

	/* aquaire game mutex */
	res = game_session_lock(&p_clnt->p_env->h_game_mtx);
	if (res != E_SUCCESS)
		return res;

	if (!p_clnt->p_env->game.valid) {
		res = create_game(p_clnt);
		if (res == E_SUCCESS)
			join_game(p_clnt, MASTER);
	} else {
		join_game(p_clnt, SLAVE);
	}

	res |= game_session_release(&p_clnt->p_env->h_game_mtx);

	return res;
}


int game_session_write(struct client *p_clnt, char *data)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	DBG_TRACE_STR(T, p_clnt->username, "write: %s", data);

	HANDLE h_file = NULL;
	int res;
	int buffer_size;
	char *buffer = NULL;

	do {
		h_file = CreateFileA(PATH_GAME_SESSION,
				     GENERIC_WRITE,
				     0,
				     NULL,
				     OPEN_EXISTING,
				     FILE_ATTRIBUTE_NORMAL,
				     NULL);
		if (!h_file) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}


		buffer_size = strlen(data) + 1;
		buffer = calloc(buffer_size, 1);
		if (!buffer) {
			PRINT_ERROR(E_STDLIB);
			res = E_STDLIB;
			break;
		}

		memcpy(buffer, data, strlen(data));

		/* write */
		res = WriteFile(h_file, buffer, buffer_size, NULL, NULL);
		if (!res) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		/* indicate write done */
		if (!SetEvent(*(p_clnt->play_evt))) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		res = E_SUCCESS;

	} while (0);
	
	/* cleanup */
	if (buffer)
		free(buffer);
	if (h_file) {
		if (!CloseHandle(h_file)) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
		}
	}

	return res;
}


int game_session_read(struct client *p_clnt, char *buffer)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	HANDLE h_file = NULL;
	int res;

	do {
		h_file = CreateFileA(PATH_GAME_SESSION,
				     GENERIC_READ,
				     0,
				     NULL,
				     OPEN_EXISTING,
				     FILE_ATTRIBUTE_NORMAL,
				     NULL);
		if (!h_file) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		res = ReadFile(h_file, buffer, 23, NULL, NULL);
		if (!res) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		res = E_SUCCESS;

	} while (0);

	if (h_file) {
		if (!CloseHandle(h_file)) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
		}
	}

	return res;

}





int session_sequece(struct client *p_clnt, char *buffer)
{
	struct game *p_game = &p_clnt->p_env->game;
	int position = p_clnt->position;
	int opponent_position = (position == MASTER) ? SLAVE : MASTER;
	HANDLE *h_opponent_evt = p_game->h_play_evt[opponent_position];
	int res;
	DWORD wait_code;
	char write[25] = {0};
	memcpy(write, buffer, strlen(buffer));

	/* acquire file lock */
	res = game_session_lock(&p_clnt->p_env->h_game_mtx);
	if (res != E_SUCCESS)
		return res;

	/* check if opponent wrote data */
	wait_code = WaitForSingleObject(h_opponent_evt, 0);
	switch (wait_code) {
	case WAIT_OBJECT_0:
		/* read, then write */
		res = game_session_read(p_clnt, buffer);
		if (res != E_SUCCESS)
			break;
		res = game_session_write(p_clnt, write);
		break;
	case WAIT_TIMEOUT:
		/* write, then wait */
		res = game_session_write(p_clnt, write);
		if (res != E_SUCCESS)
			break;
		/* relase lock (to allow opponent to write) */
		res = game_session_release(&p_clnt->p_env->h_game_mtx);
		if (res != E_SUCCESS)
			break;
		/* wait for opponents signal */
		wait_code = WaitForSingleObject(h_opponent_evt, 7000);
		switch (wait_code) {
		case WAIT_TIMEOUT:
			/* opponent didnt write */
			res = E_TIMEOUT;
			break;
		case WAIT_OBJECT_0:
			/* lock to read opponent data */
			res = game_session_lock(&p_clnt->p_env->h_game_mtx);
			if (res != E_SUCCESS)
				break;
			/* read */
			res = game_session_read(p_clnt, buffer);
			if (res != E_SUCCESS)
				break;
			break;
		case WAIT_FAILED:
			PRINT_ERROR(E_WINAPI);
		default:
			res = E_WINAPI;
		}
		break;
	
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
	default:
		res = E_WINAPI;
	}

	if (res != E_TIMEOUT)
		res |= game_session_release(&p_clnt->p_env->h_game_mtx);

	return res;
}