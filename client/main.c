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

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <stdio.h>
#include "client_tasks.h"
#include "client_flow.h"

/*
 ==============================================================================
 * MAIN
 ==============================================================================
 */

int main(int argc, char **argv)
{
	struct client_env env = {0};
	int ret_val, state;

	/* check program input arguments */
	if (check_input(&env, argc, argv) != E_SUCCESS)
		return E_FAILURE;

	/* initialize client resources */
	state = client_init(&env);

	/* client execution loop */
	while (state != STATE_EXIT)
	{
		state = (*clnt_flow[state])(&env);
	}

	/* free client resources */
	ret_val = client_cleanup(&env);

	/* exit program */
	printf("--- CLIENT EXIT 0x%x ---\n", ret_val);
	return ret_val;
}
