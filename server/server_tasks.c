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
#define _WINSOCK_DEPRECATED_NO_WARNINGS

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


int check_input(struct serv_env *p_env, int argc, char** argv)
{
	DBG_PRINT("check_input\n");
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
	p_env->h_file_stdin = CreateFileA("CONIN$",	// FIXME: stdin
					  GENERIC_READ,
					  FILE_SHARE_READ,
					  NULL,
					  OPEN_EXISTING,
					  FILE_FLAG_OVERLAPPED,
					  NULL);
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

	/* 
	 * read stdin asynchronously for exit command.
	 * asynchronous read must always return false.
	 * error is detected by checking WSA last error.
	 * io_pending designates i/o pending completion,
	 * and is thus not treated as an error.
	 */ 
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

	return E_SUCCESS;
}


int server_init(struct serv_env *p_env)
{
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

	DBG_FUNC_STAMP();
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

	/*
	 * if command is NOT "exit" - reset read event
	 * and start a new asynchronous read. else, signal
	 * runing threads to end and wait for them, then return
	 * true to signal main thread it can do cleanup
	 */
	if (strncmp(p_env->buffer, "exit", 5)) { // FIXME: exitXXX...
		if (!ResetEvent(p_env->olp_stdin.hEvent)) {
			PRINT_ERROR(E_WINAPI);
			p_env->last_err = E_WINAPI;
			return true;
		}
		res = ReadFile(p_env->h_file_stdin, p_env->buffer, 4, NULL, &p_env->olp_stdin);
		if ((res) || (WSAGetLastError() != ERROR_IO_PENDING)) {
			PRINT_ERROR(E_WINAPI);
			p_env->last_err = E_WINAPI;
			return true;
		}
		return false;
	} else {
		/* set abort event */
		if (!SetEvent(p_env->h_abort_evt)) {
			PRINT_ERROR(E_WINAPI);
			p_env->last_err = E_WINAPI;
			return true;
		}
		/* wait for threads, quit in any case*/
		if (p_env->h_clnt_thread) {
			wait_code = WaitForSingleObject(p_env->h_clnt_thread, 60000); // FIXME: more threads
			switch (wait_code) {
			case WAIT_OBJECT_0:
				break;
			case WAIT_TIMEOUT:
				p_env->last_err = E_TIMEOUT;
				break;
			case WAIT_FAILED:
				PRINT_ERROR(E_WINAPI);
				/* fall through */
			default:
				p_env->last_err = E_WINAPI;
				break;
			}
		}
		/* signal main to quit */
		DBG_PRINT("server_quit = true\n");
		return true;
	}
}


int serv_clnt_connect(struct serv_env *p_env)
{
	int res;
	int new_skt;
	struct clnt_args *clnt_args = NULL;
	FD_SET readfs;
	TIMEVAL tv = {0, 1000}; // FIXME:

	// char buffer[100];

	FD_ZERO(&readfs);
	FD_SET(p_env->serv_skt, &readfs);

	/* wait for socket to be signald */
	res = select(p_env->serv_skt + 1, &readfs, NULL, NULL, &tv);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_FAILURE;
	}

	/* socket is signald */
	if (res) {
		
		/* accept incoming connection */
		new_skt = accept(p_env->serv_skt, NULL, NULL);
		if (new_skt == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			return E_FAILURE;
		}
		DBG_PRINT("new connection\n");

		/* alloc mem for handling connection */
		clnt_args = calloc(1, sizeof(*clnt_args));
		if (clnt_args == NULL) {
			PRINT_ERROR(E_STDLIB);
			return E_STDLIB;
		}
		clnt_args->skt = new_skt;
		clnt_args->p_env = p_env;

		// FIXME: this is temporary logic (only single thread infrastructure exists)
		if (p_env->h_clnt_thread) {
			DWORD wait_code = WaitForSingleObject(p_env->h_clnt_thread, 20000);
			switch (wait_code) {
			case WAIT_FAILED:
				PRINT_ERROR(E_WINAPI);
				return E_WINAPI;
			case WAIT_TIMEOUT:
				DBG_PRINT("thread still running.\n");
				if (closesocket(clnt_args->skt) == SOCKET_ERROR) {
					PRINT_ERROR(E_WINSOCK);
					return E_WINSOCK;
				}
				return E_SUCCESS;
			case WAIT_OBJECT_0:
				break;
			}
			if (!CloseHandle(p_env->h_clnt_thread)) {
				PRINT_ERROR(E_WINAPI);
				return E_WINAPI;
			}
		}

		/* handle connection in new thread */
		DBG_PRINT("creating thread\n");
		p_env->h_clnt_thread = CreateThread(NULL, 0, clnt_thread, (LPVOID)clnt_args, 0, NULL);
		if (!p_env->h_clnt_thread) {
			PRINT_ERROR(E_WINAPI);
			if (closesocket(clnt_args->skt) == SOCKET_ERROR) {
				PRINT_ERROR(E_WINSOCK);
				return E_WINSOCK; // FIXME: best solution?
			}
			return E_WINAPI;
		}
		p_env->clnt_cnt++;
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
	DBG_FUNC_STAMP();
	int ret_val = E_SUCCESS;

	if (p_env->olp_stdin.hEvent) {
		if (!CloseHandle(p_env->olp_stdin.hEvent)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_FAILURE;
		}
	}

	if (p_env->h_file_stdin) {
		if (!CloseHandle(p_env->h_file_stdin)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_FAILURE;
		}
	}

	if (p_env->serv_skt != INVALID_SOCKET) {
		if (closesocket(p_env->serv_skt) == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			ret_val = E_FAILURE;
		}
	}

	if (p_env->h_players_smpr) {
		if (!CloseHandle(p_env->h_players_smpr)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_FAILURE;
		}
	}

	if (p_env->h_abort_evt) {
		if (!CloseHandle(p_env->h_abort_evt)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_FAILURE;
		}
	}

	if (p_env->h_clnt_thread) { // FIXME: multiple threads
		if (!CloseHandle(p_env->h_clnt_thread)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_FAILURE;
		}
	}

	if (WSACleanup()) {
		PRINT_ERROR(E_WINSOCK);
		ret_val = E_FAILURE;
	}

	return ret_val;
}

// server send message wrapper
int server_send_msg(struct clnt_args *p_clnt, int type, char *p0, char *p1, char *p2, char *p3)
{
	DBG_FUNC_STAMP();
	struct msg *p_msg = NULL;
	int res;

	/* create message */
	p_msg = new_msg(type, p0, p1, p2, p3);
	if (p_msg == NULL)
		return E_STDLIB;

	/* send message */
	res = send_msg(p_clnt->skt, &p_msg);

	/* free message */
	free_msg(&p_msg);

	return res;
}

// server recv message wrapper
int server_recv_msg(struct clnt_args *p_clnt, struct msg **p_p_msg, int timeout_sec)
{
	DBG_FUNC_STAMP();
	
	TIMEVAL tv;
	int res;

	while (timeout_sec--) { // FIXME: can be better?

		if (server_check_abort(p_clnt->p_env))
			return E_INTERNAL;

		tv.tv_sec  = MSG_TIME_INCERMENT_SEC;
		tv.tv_usec = MSG_TIME_INCERMENT_USEC;
		res = recv_msg(p_p_msg, p_clnt->skt, &tv);
		if (res == E_TIMEOUT)
			continue;
		else
			break;
	}

	return res;
}