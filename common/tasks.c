/**
 * ISP_HW_4_2020
 * Bulls & Cows
 *
 * tasks.c
 * 
 * this is tasks module used for common functions and defnies
 * for server and client.
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "winsock2.h"
#include "tasks.h"

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
