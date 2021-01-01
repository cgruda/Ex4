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

#define _CRT_SECURE_NO_WARNINGS		// FIXME:

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
	p_game->h_play_evt[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!p_game->h_play_evt[0]) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

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

int destroy_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	struct game *p_game = &p_clnt->p_env->game;

	if (!ResetEvent(*(p_clnt->play_evt))) {
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
	p_clnt->play_evt = NULL;
	p_clnt->playing = false;
	p_clnt->op_pos = 0; // FIXME:

	return E_SUCCESS;
}

int leave_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	struct game *p_game = &p_clnt->p_env->game;

	if (!ResetEvent(*(p_clnt->play_evt))) {
		PRINT_ERROR(E_WINAPI);
		return E_WINAPI;
	}

	/* update client */
	p_clnt->play_evt = NULL;
	p_clnt->playing = false;
	p_clnt->op_pos = 0; // FIXME:

	/* update game */
	p_game->players_cnt--;

	return E_SUCCESS;
}

int join_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	struct game *p_game = &p_clnt->p_env->game;

	/* update client */
	p_clnt->op_pos = 0;
	p_clnt->play_evt = &p_game->h_play_evt[1];
	p_clnt->playing = true;

	/* update game */
	p_game->players_cnt++;
	p_game->accept_new_players = false;

	return E_SUCCESS;
}

int create_game(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	struct game *p_game = &p_clnt->p_env->game;
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
	p_clnt->op_pos = 1;
	p_clnt->play_evt = &p_game->h_play_evt[0];
	p_clnt->playing = true;

	return E_SUCCESS;
}

int game_session_start(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	struct game *p_game = &p_clnt->p_env->game;
	// DWORD wait_code;
	int res;

	/* aquaire game mutex */
	res = game_lock(p_game);
	if (res != E_SUCCESS)
		return res;

	if (p_game->accept_new_players) {
		if (p_game->players_cnt == 0)
			res = create_game(p_clnt);
		else
			res = join_game(p_clnt);
	} else {
		res = E_FAILURE;
	}

	res |= game_release(p_game);
	if (res != E_SUCCESS)
		return res;

	return res;
}

int game_session_end(struct client *p_clnt)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	if (!p_clnt->playing)
		return E_SUCCESS;

	int res;
	struct game *p_game = &p_clnt->p_env->game;

	res = game_lock(p_game);

	/* leave or destroy game */
	switch (p_game->players_cnt) {
	case 1:
		res = destroy_game(p_clnt);
		break;
	case 2:
		res = leave_game(p_clnt);
		break;
	default:
		break;
	}

	res |= game_release(p_game);

	return res;
}

int game_session_write(struct client *p_clnt, char *data)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	HANDLE h_file = NULL;
	int res;
	int buffer_size;
	char *buffer = NULL;

	do {
		h_file = CreateFileA(PATH_GAME_SESSION,
				     GENERIC_WRITE,
				     0,
				     NULL,
				     OPEN_EXISTING,
				     FILE_ATTRIBUTE_NORMAL,
				     NULL);
		if (h_file == INVALID_HANDLE_VALUE) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		buffer_size = strlen(data) + 1;
		buffer = calloc(buffer_size, 1);
		if (!buffer) {
			PRINT_ERROR(E_STDLIB);
			res = E_STDLIB;
			break;
		}

		memcpy(buffer, data, strlen(data));

		/* write */
		res = WriteFile(h_file, buffer, buffer_size, NULL, NULL);
		if (!res) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		/* indicate write done */
		if (!SetEvent(*(p_clnt->play_evt))) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		res = E_SUCCESS;

	} while (0);
	
	/* cleanup */
	if (buffer)
		free(buffer);
	if (h_file) {
		if (!CloseHandle(h_file)) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
		}
	}

	DBG_TRACE_STR(T, p_clnt->username, "write: %s", data);

	return res;
}

int game_session_read(struct client *p_clnt, char *buffer)
{
	DBG_TRACE_FUNC(T, p_clnt->username);

	HANDLE h_file = NULL;
	int res;
	buffer[0] = 0;

	do {
		h_file = CreateFileA(PATH_GAME_SESSION,
				     GENERIC_READ,
				     0,
				     NULL,
				     OPEN_EXISTING,
				     FILE_ATTRIBUTE_NORMAL,
				     NULL);
		if (h_file == INVALID_HANDLE_VALUE) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		res = ReadFile(h_file, buffer, 23, NULL, NULL);
		if (!res) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
			break;
		}

		res = E_SUCCESS;

	} while (0);

	if (h_file) {
		if (!CloseHandle(h_file)) {
			PRINT_ERROR(E_WINAPI);
			res = E_WINAPI;
		}
	}

	DBG_TRACE_STR(T, p_clnt->username, "read: %s", buffer);
	return res;
}

int game_sequence(struct client *p_clnt, char *write_buff, char *read_buff)
{
	DBG_TRACE_FUNC(T, p_clnt->username);
	struct game *p_game = &p_clnt->p_env->game;
	HANDLE *h_evt = &p_game->h_play_evt[p_clnt->op_pos];
	DWORD wait_code;
	int res;

	/* acquire file lock */
	res = game_lock(p_game);
	if (res != E_SUCCESS)
		return res;

	DBG_TRACE_STR(T, p_clnt->username, "LOCK");

	/* check if opponent wrote data */
	wait_code = WaitForSingleObject(*h_evt, 0);
	switch (wait_code) {
	case WAIT_OBJECT_0:
		/* read then write */
		res = game_session_read(p_clnt, read_buff);
		if (res != E_SUCCESS)
			break;
		res = game_session_write(p_clnt, write_buff);
		break;
	case WAIT_TIMEOUT:
		/* write, wait, then read */
		res = game_session_write(p_clnt, write_buff);
		if (res != E_SUCCESS)
			break;
		res = game_release(p_game);
		if (res != E_SUCCESS)
			break;
		DBG_TRACE_STR(T, p_clnt->username, "RELEASE");
		wait_code = WaitForSingleObject(*h_evt, 25000); // FIXME:
		if (wait_code == WAIT_TIMEOUT) {
			res = E_TIMEOUT;
			break;
		} else if (wait_code == WAIT_OBJECT_0) {
			res = game_lock(p_game);
			if (res != E_SUCCESS)
				break;
			DBG_TRACE_STR(T, p_clnt->username, "LOCK");
			res = game_session_read(p_clnt, read_buff);
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
		DBG_TRACE_STR(T, p_clnt->username, "RELEASE");
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
