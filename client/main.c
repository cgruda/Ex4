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

#pragma comment(lib, "ws2_32.lib")

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <process.h>
#include "winsock2.h"
#include "client_tasks.h"

/*
 ==============================================================================
 * MAIN
 ==============================================================================
 */

int main(int argc, char **argv)
{
	struct client_env env = {0};
	int ret_val = E_SUCCESS;
	int res;
	int user_input;

	if (check_input(&env, argc, argv))
		return E_FAILURE;

	/* do-while(0) for easy cleanup */
	do {
		if(client_init(&env))
			break;

		while(1) {
			res = cilent_connect_to_game(&env);
			if (res == S_CONNECT_SUCCESS) {
				break;
			} else if (res== S_CONNECT_FAILURE) {
				UI_PRINT(UI_MENU_CONNECT);
				scanf_s("%d", &user_input);
				if (user_input == 1)
					continue;
				else
					break;
			} else {
				DBG_PRINT("what to do???\n");
				break;
			}
		}

	} while (0);

	if (client_cleanup(&env))
		ret_val = E_FAILURE;

	printf("--- CLIENT EXIT %s ---\n", ret_val ? "FAILURE" : "SUCCESS");
	return ret_val;
}
