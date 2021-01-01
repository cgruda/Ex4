/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * common
 *
 * tasks.c
 * 
 * TODO:
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

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "winsock2.h"
#include "tasks.h"

/*
 ==============================================================================
 * GLOBAL VARS
 ==============================================================================
 */

#if DBG_TRACE
char *dbg_trace_mode_2_str[TRACE_MAX] = { // FIXME:
	[TRACE_CLIENT] = "client_",
	[TRACE_THREAD] = "thread_",
	[TRACE_SERVER] = "",
};
#endif

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

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
	case E_INPUT:
		printf("Input Error\n");
		break;
	case E_TIMEOUT:
		printf("Timeout Error\n");
		break;
	case E_FLOW:
		printf("Flow Error\n");
		break;
	default:
		printf("Unknown Error 0x%02X\n", err_val);
	}
}

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

#if DBG_TRACE // FIXME:
char *dbg_trace_get_path(int mode, char *name)
{
	char *path = calloc(100, sizeof(char));
	char *pre  = dbg_trace_mode_2_str[mode];
	if (!path) {
		PRINT_ERROR(E_INTERNAL);
		exit(E_FAILURE);
	}

	memcpy(path, "trace\\", strlen("trace\\"));
	memcpy(path + strlen(path), pre, strlen(pre));
	memcpy(path + strlen(path), name, strlen(name));
	memcpy(path + strlen(path), ".txt", strlen(".txt"));

	return path;
}

void dbg_trace_init(int mode, char *name)
{
	char *path = dbg_trace_get_path(mode, name);

	FILE *fp = fopen(path, "w");
	if (!fp) {
		PRINT_ERROR(E_INTERNAL);
		exit(E_FAILURE);
	}

	fclose(fp);
	free(path);
}

void dbg_trace_log(int mode, char *name, char *str)
{
	char *path = dbg_trace_get_path(mode, name);

	FILE *fp = fopen(path, "a");
	if (!fp) {
		PRINT_ERROR(E_INTERNAL);
		exit(E_FAILURE);
	}

	fprintf(fp, str);

	fclose(fp);
	free(path);
}

#endif // DBG_TRACE