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

#pragma comment(lib, "ws2_32.lib")

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
