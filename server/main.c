/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client side
 *
 * main.c
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")
// include libraries


#include "winsock2.h"
#include <stdint.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <process.h>
#include "server_tasks.h"

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

/*
 ==============================================================================
 * MAIN
 ==============================================================================
 */

int main(int argc, char **argv)
{
	struct server_env env = {0};
	int ret_val = E_FAILURE;

	if (check_input(&env, argc, argv))
		return E_FAILURE;

	int count = 0;
	
	// do-while(0) for easy cleanup
	do {
		if (server_init(&env))
			break;

		// execution loop
		while (count < 100000) {
			if (server_exit_test(&env))
				break;
			count++;
		}
		
		ret_val = E_SUCCESS;

	} while (0);

	if (server_cleanup(&env))
		ret_val = E_FAILURE;

	printf("BYBY BIRDY count=%d\n", count);
	return ret_val;
}
