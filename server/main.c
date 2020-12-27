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
#include <stdio.h>
#include "server_tasks.h"
#include "tasks.h"


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
	struct serv_env env = {0};
	int ret_val = E_FAILURE;

	if (check_input(&env, argc, argv))
		return E_FAILURE;

	/* do-while(0) for easy cleanup */
	do { // TODO: fix
		if (server_init(&env))
			break;

		/* execution loop */
		while (!server_quit(&env)) {
			
			if(serv_clnt_connect(&env))
				break;
		}

		ret_val = E_SUCCESS;

	} while (0);

	if (server_cleanup(&env))
		ret_val = E_FAILURE;

	printf("--- SERVER EXIT %s ---\n", ret_val ? "FAILURE" : "SUCCESS");
	return ret_val;
}
