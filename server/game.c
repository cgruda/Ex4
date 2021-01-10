/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * server program
 *
 * game.c
 * 
 * game module handles game tasks, and communication
 * between thread over the GameSession.txt file.
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

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
#include "tasks.h"

/*
 ==============================================================================
 * FUNCTION DEFENITIONS
 ==============================================================================
 */

int game_init(struct game *p_game)
{
	/* event thread 0 uses to notify thread 1 */
	p_game->h_play_evt[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!p_game->h_play_evt[0]) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* event thread1 uses to notify thread 0 */
	p_game->h_play_evt[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!p_game->h_play_evt[1]) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* mutex to sync game session update */
	p_game->h_game_mtx = CreateMutex(NULL, FALSE, NULL);
	if (p_game->h_game_mtx == NULL) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* delete game session file incase it exists */
	if (!DeleteFileA(PATH_GAME_SESSION)) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
			PRINT_ERROR(E_WINAPI);
			return E_WINAPI;
		}
	}

	p_game->players_cnt = 0;
	p_game->accept_new_players = true;

	return E_SUCCESS;
}

int game_cleanup(struct game *p_game)
{
	int ret_val = E_SUCCESS;

	if (p_game->h_game_mtx) {
		if (!CloseHandle(p_game->h_game_mtx)) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	if (p_game->h_play_evt[0]) {
		if (!CloseHandle(p_game->h_play_evt[0])) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}
	
	if (p_game->h_play_evt[1]) {
		if (!CloseHandle(p_game->h_play_evt[1])) {
			PRINT_ERROR(E_WINAPI);
			ret_val = E_WINAPI;
		}
	}

	return ret_val;
}

int game_lock(struct game *p_game)
{
	DWORD wait_code;

	wait_code = WaitForSingleObject(p_game->h_game_mtx, 5000);
	switch (wait_code) {
	case WAIT_FAILED:
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	case WAIT_OBJECT_0:
		return E_SUCCESS;
	case WAIT_TIMEOUT:
		PRINT_ERROR(E_INTERNAL);
		return E_TIMEOUT;
	default:
		return E_FAILURE;
	}
}

int game_release(struct game *p_game)
{
	if (!ReleaseMutex(p_game->h_game_mtx)) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	return E_SUCCESS;
}

int destroy_game(struct client *p_client)
{
	struct game *p_game = &p_client->p_env->game;

	/* make sure my event is down */
	if (!ResetEvent(*(p_client->p_h_play_evt))) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* delete game session file */
	if (!DeleteFileA(PATH_GAME_SESSION)) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* update game */
	p_game->players_cnt = 0;
	p_game->accept_new_players = true;
	
	/* update client */
	p_client->p_h_play_evt = NULL;
	p_client->playing = false;
	p_client->opp_pos = 0;

	/* remove opponent username */
	if (p_client->opp_username)
		free(p_client->opp_username);
	p_client->opp_username = NULL;

	return E_SUCCESS;
}

int leave_game(struct client *p_client)
{
	struct game *p_game = &p_client->p_env->game;

	/* make sure my event is down */
	if (!ResetEvent(*(p_client->p_h_play_evt))) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* update client */
	p_client->p_h_play_evt = NULL;
	p_client->playing = false;
	p_client->opp_pos = 0;

	/* remove opponent username */
	if (p_client->opp_username)
		free(p_client->opp_username);
	p_client->opp_username = NULL;

	/* update game */
	p_game->players_cnt--;

	return E_SUCCESS;
}

int join_game(struct client *p_client)
{
	struct game *p_game = &p_client->p_env->game;

	/* update client */
	p_client->opp_pos = 0;
	p_client->p_h_play_evt = &p_game->h_play_evt[1];
	p_client->playing = true;

	/* update game */
	p_game->players_cnt++;
	p_game->accept_new_players = false;

	return E_SUCCESS;
}

int create_game(struct client *p_client)
{
	struct game *p_game = &p_client->p_env->game;
	HANDLE h_file = NULL;

	/* create session file */
	h_file = CreateFileA(PATH_GAME_SESSION,     /* GameSession path    */
			     0,                     /* no access needed    */
			     0,                     /* no share            */
			     NULL,                  /* default security    */
			     CREATE_ALWAYS,         /* create file         */
			     FILE_ATTRIBUTE_NORMAL, /* asynchronous read   */
			     NULL);                 /* no template         */
	if (h_file == INVALID_HANDLE_VALUE) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* close handle to session file */
	if (!CloseHandle(h_file)) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* update game */
	p_game->players_cnt++;

	/* update client */
	p_client->opp_pos = 1;
	p_client->p_h_play_evt = &p_game->h_play_evt[0];
	p_client->playing = true;

	return E_SUCCESS;
}

int game_session_start(struct client *p_client)
{
	struct game *p_game = &p_client->p_env->game;
	int res;
	bool other_players_available = false;

	/* check if there are more clients connected
	 * if none - no need to start session. added
	 * after reading question in forum specifiying
	 * this scenario */
	res = server_lock(p_client->p_env);
	if (res != E_SUCCESS)
		return res;
	
	if ((p_client->p_env->thread_bitmap & ~(THREAD_BITMAP_INIT_MASK | BIT(p_client->id))))
		other_players_available = true;
	
	res = server_release(p_client->p_env);
	if (res != E_SUCCESS)
		return res;

	if (!other_players_available)
		return E_TIMEOUT;

	/* aquaire game lock */
	res = game_lock(p_game);
	if (res != E_SUCCESS)
		return res;

	/* create or join game */
	if (p_game->accept_new_players) {
		if (p_game->players_cnt == 0)
			res = create_game(p_client);
		else
			res = join_game(p_client);
	} else {
		/* this may occur when player disconnected but
		 * second player still didnt call session_end,
		 * act as if the are no opponents since this
		 * case was not defined */
		res = E_TIMEOUT;
	}

	/* release game lock */
	res |= game_release(p_game);
	if (res != E_SUCCESS)
		return res;

	return res;
}

int game_session_end(struct client *p_client)
{
	int res;
	struct game *p_game = &p_client->p_env->game;

	/* make sure player is in game */
	if (!p_client->playing)
		return E_SUCCESS;

	/* lock game */
	res = game_lock(p_game);
	if (res != E_SUCCESS)
		return res;

	/* leave or destroy game */
	switch (p_game->players_cnt) {
	case 1:
		res = destroy_game(p_client);
		break;
	case 2:
		res = leave_game(p_client);
		break;
	default:
		break;
	}

	/* release game */
	res |= game_release(p_game);

	return res;
}

int game_session_write(struct client *p_client, char *data)
{
	HANDLE h_file = NULL;
	int res;

	/* do-while(0) for easy cleanup */
	do {
		/* create file for write */
		h_file = CreateFileA(PATH_GAME_SESSION,
				     GENERIC_WRITE,
				     FILE_SHARE_READ | FILE_SHARE_WRITE,
				     NULL,
				     OPEN_EXISTING,
				     FILE_ATTRIBUTE_NORMAL,
				     NULL);
		if (h_file == INVALID_HANDLE_VALUE) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		/* write data */
		res = WriteFile(h_file, data, (int)strlen(data) + 1, NULL, NULL);
		if (!res) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		/* indicate write done */
		if (!SetEvent(*(p_client->p_h_play_evt))) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
		}

		res = E_SUCCESS;

	} while (0);
	
	/* cleanup */
	if (h_file) {
		if (!CloseHandle(h_file)) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
		}
	}

	return res;
}

int game_session_read(struct client *p_client, char *buffer)
{
	HANDLE h_file = NULL;
	int res;
	buffer[0] = 0;

	/* dp-while(0) for easy cleanup */
	do {
		/* open file */
		h_file = CreateFileA(PATH_GAME_SESSION,
				     GENERIC_READ,
				     FILE_SHARE_READ | FILE_SHARE_WRITE,
				     NULL,
				     OPEN_EXISTING,
				     FILE_ATTRIBUTE_NORMAL,
				     NULL);
		if (h_file == INVALID_HANDLE_VALUE) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		/* read data */
		res = ReadFile(h_file, buffer, 23, NULL, NULL);
		if (!res) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		res = E_SUCCESS;

	} while (0);

	/* close file */
	if (h_file) {
		if (!CloseHandle(h_file)) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
		}
	}

	return res;
}

int game_sequence(struct client *p_client, char *write_buff, char *read_buff, int timeout_sec)
{
	struct game *p_game = &p_client->p_env->game;
	HANDLE *h_evt = &p_game->h_play_evt[p_client->opp_pos];
	DWORD wait_code;
	int res;

	/* acquire file lock */
	res = game_lock(p_game);
	if (res != E_SUCCESS)
		return res;

	/* early indication that opponent quit */
	if (p_client->playing      &&
	    p_client->opp_username &&
	    p_game->players_cnt != GAME_MAX_PLAYERS) {
		res = game_release(p_game);
		if (res != E_SUCCESS)
			return res;
		return E_TIMEOUT;
	}

	/* check if opponent wrote data */
	wait_code = WaitForSingleObject(*h_evt, 0);
	switch (wait_code) {
	case WAIT_OBJECT_0:
		/* read then write */
		res = game_session_read(p_client, read_buff);
		if (res != E_SUCCESS)
			break;
		res = game_session_write(p_client, write_buff);
		break;
	case WAIT_TIMEOUT:
		/* write, wait, then read */
		res = game_session_write(p_client, write_buff);
		if (res != E_SUCCESS)
			break;
		res = game_release(p_game);
		if (res != E_SUCCESS)
			break;
		wait_code = WaitForSingleObject(*h_evt, timeout_sec * SEC2MS);
		if (wait_code == WAIT_TIMEOUT) {
			res = E_TIMEOUT;
			break;
		} else if (wait_code == WAIT_OBJECT_0) {
			res = game_lock(p_game);
			if (res != E_SUCCESS)
				break;
			res = game_session_read(p_client, read_buff);
			if (res != E_SUCCESS)
				break;
			break;
		} /* else - fall through */
	default:
		PRINT_ERROR(E_WINAPI);
		res = E_WINAPI;
	}

	/* free game lock */
	if (res != E_TIMEOUT) {
		res |= game_release(p_game);
	}

	return res;
}

int game_bulls(char *a, char *b)
{
	int bulls = 0;

	for (int i = 0; i < 4; i++)
		if (a[i] == b[i])
			bulls++;

	return bulls;
}

int game_cows(char *a, char *b)
{
	int cows = 0;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j)
				continue;
			if (a[i] == b[j]) {
				cows++;
				break;
			}
		}
	}

	return cows;
}
