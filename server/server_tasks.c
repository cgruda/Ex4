/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * server program
 *
 * server_tasks.c
 * 
 * server_tasks module handles all tasks that are not
 * part of the threads FSM and the game itself.
 * this includs wrappers for the message module,
 * controling creation and closing of threads from main thread,
 * and server intilization and cleanup.
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS		// FIXME:

#pragma comment(lib, "ws2_32.lib")

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
#include "game.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

void print_usage()
{
	printf("\nusage:\n\tserver.exe <port>\n\n");
}

int check_input(struct serv_env *p_env, int argc, char **argv)
{
	int ret_val = E_SUCCESS;
	int port;

	/* check number of arguments */
	if (argc != SERVER_ARGC) {
		print_usage();
		return E_FAILURE;
	}

	/* check port number */
	port = strtol(argv[1], NULL, 10);
	if (!((port > 0) && (port < MAX_PORT))) {
		printf("\n%s is not a valid port", argv[2]);
		ret_val = E_FAILURE;
	}
	p_env->server_port = port;

	if (ret_val != E_SUCCESS)
		print_usage();

	return ret_val;
}

int serv_quit_init(struct serv_env *p_env)
{
	int res;
	
	/* create file handle for stdin */
	p_env->h_stdin = CreateFileA(PATH_STDIN,           /* standard input    */
				     GENERIC_READ,         /* we want to read   */
				     FILE_SHARE_READ,      /* others may use    */
				     NULL,                 /* default security  */
				     OPEN_EXISTING,        /* stdin exists      */
				     FILE_FLAG_OVERLAPPED, /* asynchronous read */
				     NULL);                /* no template       */
	if (p_env->h_stdin == INVALID_HANDLE_VALUE) {
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
	 * error is detected by checking winapi last error.
	 * io_pending designates i/o pending completion,
	 * and is thus not treated as an error. */
	res = ReadFile(p_env->h_stdin, p_env->buffer, 4, NULL, &p_env->olp_stdin);
	if ((res) || (GetLastError() != ERROR_IO_PENDING)) {
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
	p_env->server_skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p_env->server_skt == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* set server address */
	p_env->server.sin_family      = AF_INET;
	p_env->server.sin_addr.s_addr = htonl(INADDR_ANY);
	p_env->server.sin_port        = htons(p_env->server_port);

	/* bind socket to ip and port */
	res = bind(p_env->server_skt, (PSOCKADDR)&p_env->server, sizeof(SOCKADDR));
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* place socket in listening state */
	res = listen(p_env->server_skt, SOMAXCONN);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	return E_SUCCESS;
}

int serv_ctrl_init(struct serv_env *p_env)
{
	/* semaphore to restrict max players */
	p_env->h_client_approve_smpr = CreateSemaphore(NULL, GAME_MAX_PLAYERS, GAME_MAX_PLAYERS, NULL);
	if (!p_env->h_client_approve_smpr) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* event for signal threads to abort */
	p_env->h_abort_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!p_env->h_abort_evt) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* mutex to access server-thread bitmap */
	p_env->h_client_approve_mtx = CreateMutex(NULL, FALSE, NULL);
	if (!p_env->h_client_approve_mtx) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* bitmap for thread creation control */
	p_env->thread_bitmap = THREAD_BITMAP_INIT_MASK;

	return E_SUCCESS;
}

int server_init(struct serv_env *p_env)
{
	DBG_TRACE_INIT(TRACE_SERVER, SERVER);
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

	/* communication logic init */
	res = game_init(&p_env->game);
	if (res != E_SUCCESS)
		return res;

	DBG_TRACE_FUNC(TRACE_SERVER, SERVER);
	return E_SUCCESS;
}

bool server_quit(struct serv_env *p_env)
{
	DWORD wait_code;
	int res;

	/* check stdin input event */
	wait_code = WaitForSingleObject(p_env->olp_stdin.hEvent, 100);
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
	if (strncmp(p_env->buffer, SERVER_EXIT_COMMAND, SERVER_EXIT_COMMAND_LEN)) {
		if (!ResetEvent(p_env->olp_stdin.hEvent)) {
			PRINT_ERROR(E_WINAPI);
			p_env->last_err = E_WINAPI;
			return true;
		}
		/* read stdin asynchronously for exit command.
		 * asynchronous read must always return false.
		 * error is detected by checking winapi last error.
		 * io_pending designates i/o pending completion,
		 * and is thus not treated as an error. */
		res = ReadFile(p_env->h_stdin, p_env->buffer, SERVER_EXIT_COMMAND_LEN - 1, NULL, &p_env->olp_stdin);
		if ((res) || (GetLastError() != ERROR_IO_PENDING)) {
			PRINT_ERROR(E_WINAPI);
			p_env->last_err = E_WINAPI;
			return true;
		}
		return false;
	} else {
		DBG_TRACE_STR(TRACE_SERVER, SERVER, "server_quit!");	
		return true;
	}
}

int server_destroy_clients(struct serv_env *p_env)
{
	DBG_TRACE_FUNC(TRACE_SERVER, SERVER);
	int res;
	
	/* set abort event */
	if (!SetEvent(p_env->h_abort_evt)) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	res = server_check_thread_status(p_env, SERVER_WAIT_ON_THREAD_MS);

	return res;
}

int serv_clnt_connect(struct serv_env *p_env)
{
	int res;
	int new_skt;
	struct client *p_client = NULL;
	HANDLE *p_h_client_thread = NULL;
	FD_SET readfs;
	DWORD idx;
	TIMEVAL tv = {0, SERVER_TCP_WAIT_CONNECT_US};

	// char buffer[100];

	FD_ZERO(&readfs);
	FD_SET(p_env->server_skt, &readfs);

	/* wait for socket to be signald */
	res = select(p_env->server_skt + 1, &readfs, NULL, NULL, &tv);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_WINSOCK;
	}

	/* socket is signald */
	if (res) {

		res = server_lock(p_env);
		if (res != E_SUCCESS)
			return res;

		/* check if more connections can be accepted */
		if (!BitScanForward(&idx, ~p_env->thread_bitmap)) {
			DBG_TRACE_STR(TRACE_SERVER, SERVER, "max TCP connections! incoming connection refused");
			return E_SUCCESS;
		}

		/* accept incoming connection */
		new_skt = accept(p_env->server_skt, NULL, NULL);
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
		p_h_client_thread = &p_env->h_client_thread[idx];
		*p_h_client_thread = CreateThread(NULL, 0, client_thread, (LPVOID)p_client, 0, NULL);
		if (!*p_h_client_thread) {
			PRINT_ERROR(E_WINAPI);
			if (closesocket(p_client->skt) == SOCKET_ERROR) {
				PRINT_ERROR(E_WINSOCK);
				return E_WINSOCK;
			}
			return E_WINAPI;
		}

		/* mark thread handle as taken */
		SET_BIT(p_env->thread_bitmap, idx);
		DBG_TRACE_STR(TRACE_SERVER, SERVER, "start TCP connection %d", idx);

		res = server_release(p_env);
		if (res != E_SUCCESS)
			return res;
	}
	
	return E_SUCCESS;
}

int server_check_thread_status(struct serv_env *p_env, int ms)
{
	DWORD wait_code;
	HANDLE *p_h_client_thread = NULL;
	int res = E_SUCCESS;

	res = server_lock(p_env);
	if (res != E_SUCCESS)
		return res;

	/* loop all thread positions */
	for (int idx = 0; (idx < MAX_CONNECTIONS) && (res == E_SUCCESS); idx++) {

		/* check that thread is active */
		if (!TEST_BIT(p_env->thread_bitmap, idx))
			continue;
		
		/* check if thread exited - if did do cleanup */
		p_h_client_thread = &p_env->h_client_thread[idx];
		wait_code = WaitForSingleObject(*p_h_client_thread, ms);
		switch (wait_code)
		{
		case WAIT_TIMEOUT:
			break;
		case WAIT_OBJECT_0:
			if (!CloseHandle(*p_h_client_thread)) {
				PRINT_ERROR(E_WINAPI);
				res = E_WINAPI;
			}
			DBG_TRACE_STR(TRACE_SERVER, SERVER, "end TCP connection   %d", idx);
			CLR_BIT(p_env->thread_bitmap, idx);
			break;
		case WAIT_FAILED:
			/* fall through */
		default:
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
		}
	}

	res |= server_release(p_env);

	return res;
}

bool server_check_abort(struct serv_env *p_env)
{
	DWORD wait_code;

	/* check if main thread signald abort */
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
	DBG_TRACE_FUNC(TRACE_SERVER, SERVER);
	int ret_val = p_env->last_err;

	if (p_env->olp_stdin.hEvent) {
		if (!CloseHandle(p_env->olp_stdin.hEvent)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (p_env->h_stdin) {
		if (!CloseHandle(p_env->h_stdin)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (p_env->server_skt != INVALID_SOCKET) {
		if (closesocket(p_env->server_skt) == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			ret_val = E_WINSOCK;
		}
	}

	if (p_env->h_client_approve_smpr) {
		if (!CloseHandle(p_env->h_client_approve_smpr)) {
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

	if (p_env->h_client_approve_mtx) {
		if (!CloseHandle(p_env->h_client_approve_mtx)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (game_cleanup(&p_env->game) != E_SUCCESS) {
		ret_val = E_WINAPI;
	}

	if (WSACleanup()) {
		PRINT_ERROR(E_WINSOCK);
		ret_val = E_WINSOCK;
	}

	return ret_val;
}

int server_send_msg(struct client *p_client, int type, char *p0, char *p1, char *p2, char *p3)
{
	DBG_TRACE_FUNC(TRACE_THREAD, p_client->username);
	struct msg *p_msg = NULL;
	int res;

	/* create message */
	p_msg = new_msg(type, p0, p1, p2, p3);
	if (p_msg == NULL)
		return E_STDLIB;

	/* send message */
	res = send_msg(p_client->skt, &p_msg);
	DBG_TRACE_MSG(TRACE_THREAD, p_client->username, p_msg);

	/* free message */
	free_msg(&p_msg);

	return res;
}

int server_recv_msg(struct client *p_client, struct msg **p_p_msg, int timeout_sec)
{
	if (p_client->connected)
		DBG_TRACE_FUNC(TRACE_THREAD, p_client->username);
	
	TIMEVAL tv;
	int res = E_SUCCESS;

	/* convert to time inceremtns */
	int incerments = timeout_sec * (SEC2MS / (MSG_TIME_INCERMENT_USEC / MS2US));

	/* wait is split to allow checking abort from main thread */
	while (incerments--) {

		if (server_check_abort(p_client->p_env))
			return E_INTERNAL;

		
		tv.tv_sec  = 0;
		tv.tv_usec = MSG_TIME_INCERMENT_USEC;
		res = recv_msg(p_p_msg, p_client->skt, &tv);
		if (res == E_TIMEOUT)
			continue;
		else
			break;
	}

	/* trace */
	if (p_client->connected && (res == E_SUCCESS))
		DBG_TRACE_MSG(TRACE_THREAD, p_client->username, *p_p_msg);

	return res;
}

int server_release(struct serv_env *p_env)
{
	if (!ReleaseMutex(p_env->h_client_approve_mtx)) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	return E_SUCCESS;
}

int server_lock(struct serv_env *p_env)
{
	DWORD wait_code;

	wait_code = WaitForSingleObject(p_env->h_client_approve_mtx, SERVER_LOCK_WAIT_MS);
	switch (wait_code) {
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	case WAIT_OBJECT_0:
		return E_SUCCESS;
	case WAIT_TIMEOUT:
		PRINT_ERROR(E_INTERNAL);
		return E_TIMEOUT;
	default:
		return E_FAILURE;
	}
}
