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
#define _CRT_SECURE_NO_WARNINGS // FIXME:

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

	/* check program input */
	ret_val = check_input(&env, argc, argv);
	if (ret_val != E_SUCCESS)
		return ret_val;

	/* initalize resources */
	ret_val = server_init(&env);

	if (ret_val == E_SUCCESS) {

		/* execution loop */
		while (1) {
			
			/* TCP connection entrance */
			if(serv_clnt_connect(&env) != E_SUCCESS)
				break;

			/* check if any TCP connections ended */
			if (server_check_thread_status(&env, 0) != E_SUCCESS)
				break;

			/* check ig "exit" command given */
			if (server_quit(&env))
				break;
		}

		/* make sure all threads end nicely */
		env.last_err = server_destroy_clients(&env);
	}
	
	/* free server resources */
	ret_val = server_cleanup(&env);

	printf("--- SERVER EXIT 0x%x ---\n", ret_val);
	return ret_val;
}
