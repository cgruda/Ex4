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
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "server_tasks.h"
#include "message.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */
int my_atoi(char *str, int *p_result)
{
	if (!str || !p_result)
		return E_FAILURE;

	if ((*str != '+') && (*str != '-') && (!isdigit(*str)))
		return E_FAILURE;

	while (*(str++))
		if (!isdigit(*str))
			return E_FAILURE;

	*p_result = strtol(str, NULL, 10);

	if (errno == ERANGE)
		return E_FAILURE;

	return E_SUCCESS;
}

void print_usage()
{
	printf("\nusage:\n\tserver.exe <server ip> <port>\n\n");
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
	case E_MESSAGE:
		printf("Message Error\n");
		break;
	default:
		printf("Unknown Error\n");
	}
}

int check_input(struct server_env *p_env, int argc, char** argv)
{
	DBG_PRINT("check_input\n");
	struct args *p_args = &p_env->args;
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
	p_args->server_ip = argv[1];

	/* check port number */
	port = strtol(argv[2], NULL, 10);
	if (!((port > 0) && (port < MAX_PORT))) {
		printf("\n%s is not a valid port", argv[2]);
		ret_val = E_FAILURE;
	}
	p_args->server_port = port;

	if (ret_val != E_SUCCESS)
		print_usage();

	return ret_val;
}

int server_init(struct server_env *p_env)
{
	DBG_PRINT("server_init\n");
	WSADATA	wsa_data;
	int res;

	/* create file handle for stdin */
	p_env->h_file_stdin = CreateFileA("CONIN$",
					  GENERIC_READ,
					  FILE_SHARE_READ,
					  NULL,
					  OPEN_EXISTING,
					  FILE_FLAG_OVERLAPPED,
					  NULL);
	if (p_env->h_file_stdin == INVALID_HANDLE_VALUE) {
		PRINT_ERROR(E_WINAPI);
		return E_FAILURE;
	}

	/* create event handle for overlapped stdin read */
	p_env->olp_stdin.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (p_env->olp_stdin.hEvent == NULL) {
		PRINT_ERROR(E_WINAPI);
		return E_FAILURE;
	}

	/* 
	 * read stdin asynchronously for exit command.
	 * asynchronous read will always return false.
	 * error is detected by checking WSA last error.
	 * io_penfing designates i/o pending completion
	 * and thus not treated as an error.
	 */ 
	res = ReadFile(p_env->h_file_stdin, p_env->buffer, 4, NULL, &p_env->olp_stdin);
	if ((res) || (WSAGetLastError() != ERROR_IO_PENDING)) {
		PRINT_ERROR(E_WINAPI);
		return E_FAILURE;
	}

	/* WinSockApi startup */
	res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (res) {
		printf("Error: WSAStartup returnd with code 0x%X\n", res);
		return E_FAILURE;
	}

	/* create socket */
	p_env->serv_skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (p_env->serv_skt == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_FAILURE;
	}

	/* set server address */
	p_env->server.sin_family      = AF_INET;
	p_env->server.sin_addr.s_addr = inet_addr(p_env->args.server_ip);
	p_env->server.sin_port        = htons(p_env->args.server_port);

	/* bind socket to ip and port */
	res = bind(p_env->serv_skt, (PSOCKADDR)&p_env->server, sizeof(SOCKADDR));
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_FAILURE;
	}

	/* place socket in listening state */
	res = listen(p_env->serv_skt, SOMAXCONN);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_FAILURE;
	}
	
	return E_SUCCESS;
}

bool server_quit(struct server_env *p_env)
{
	DWORD wait_code;
	int res;

	/* check stdin input event */
	wait_code = WaitForSingleObject(p_env->olp_stdin.hEvent, 1);
	switch (wait_code) {
	case WAIT_TIMEOUT:
		return false;
	case WAIT_OBJECT_0:
		break;
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
		/* fall through */
	case WAIT_ABANDONED:
		/* fall through */
	default:
		return true;
	}

	/*
	 * reset read event and start a new asynchronous read
	 * or return true if stdin input was "exit" command
	 */
	if (strncmp(p_env->buffer, "exit", 5)) {
		if (!ResetEvent(p_env->olp_stdin.hEvent)) {
			PRINT_ERROR(E_WINAPI);
			return true;
		}
		res = ReadFile(p_env->h_file_stdin, p_env->buffer, 4, NULL, &p_env->olp_stdin);
		if ((res) || (WSAGetLastError() != ERROR_IO_PENDING)) {
			PRINT_ERROR(E_WINAPI);
			return true;
		}
		return false;
	} else {
		DBG_PRINT("server_quit = true\n");
		return true;
	}
}





// int server_accept_client(struct server_env *p_env, int clnt_skt)
// {
// 	p_env->h_clnt_thread = CreateThread(NULL, 0, client_thread, (LPVOID)&clnt_skt, 0, NULL);
// 	if (!p_env->h_clnt_thread) {
// 		PRINT_ERROR(E_WINAPI);
// 		return E_FAILURE;
// 	}

	


// }


// DWORD WINAPI client_thread(LPVOID param)
// {
// 	ExitThread(E_SUCCESS);
// }


int server_accept_client(struct server_env *p_env)
{
	int res;
	int clnt_skt;
	FD_SET readfs;
	// char buffer[100];

	TIMEVAL tv = {1, 0};
	FD_ZERO(&readfs);
	FD_SET(p_env->serv_skt, &readfs);

	/* wait for socket to be signald */
	res = select(0, &readfs, NULL, NULL, &tv);
	if (res == SOCKET_ERROR) {
		PRINT_ERROR(E_WINSOCK);
		return E_FAILURE;
	}

	/* socket signald - accept client */
	if (res) {
		clnt_skt = accept(p_env->serv_skt, NULL, NULL);
		if (clnt_skt == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			return E_FAILURE;
		}
		DBG_PRINT("accepted new client\n");

		// dbg
		struct msg dbg_msg;
		dbg_msg.param_cnt = 2;
		dbg_msg.type = MSG_SERVER_WIN;
		dbg_msg.param_lst[0] = "test";
		dbg_msg.param_lst[1] = "1234";

		res = send_msg(clnt_skt, &dbg_msg);


		// memset(buffer, 0, 100);
		// sprintf_s(buffer, 100, "this is the server");
		// res = send(clnt_skt, buffer, strlen(buffer), 0);
		// if (res == SOCKET_ERROR) {
		// 	PRINT_ERROR(E_WINSOCK);
		// 	return E_FAILURE;
		// }

		// dbg
		res = closesocket(clnt_skt);
		if (res == SOCKET_ERROR) {
			PRINT_ERROR(E_WINSOCK);
			return E_FAILURE;
		}
	}

	return E_SUCCESS;
}

int server_cleanup(struct server_env *p_env)
{
	DBG_PRINT("server_cleanup\n");
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

	if (WSACleanup()) {
		PRINT_ERROR(E_WINSOCK);
		ret_val = E_FAILURE;
	}

	return ret_val;
}
