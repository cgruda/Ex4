/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * server side
 *
 * client_tasks.h
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#ifndef __GAME_H__
#define __GAME_H__

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <windows.h>
#include <stdbool.h>

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */

#define GAME_MAX_PLAYERS              2
#define PATH_GAME_SESSION            "GameSession.txt"

/*
 ==============================================================================
 * STRUCTURES
 ==============================================================================
 */

struct game {
	int players_cnt;
	bool accept_new_players;
	HANDLE h_play_evt[GAME_MAX_PLAYERS];
	HANDLE h_game_mtx;
};

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

/**
 ******************************************************************************
 * @brief initialize game object
 * @param p_game pointer to game struct
 * @return err_val enum
 ******************************************************************************
 */
int game_init(struct game *p_game);

/**
 ******************************************************************************
 * @brief cleanup game resources
 * @param p_game pointer to game struct
 * @return err_val enum
 ******************************************************************************
 */
int game_cleanup(struct game *p_game);

/**
 ******************************************************************************
 * @brief TODO:
 * @param p_client pointer to client object
 * @return err_val enum
 ******************************************************************************
 */
int game_session_start(struct client *p_client);

/**
 ******************************************************************************
 * @brief TODO:
 * @param p_client pointer to client object
 * @return err_val enum
 ******************************************************************************
 */
int game_sequence(struct client *p_client, char *write_buff, char *read_buff);

/**
 ******************************************************************************
 * @brief TODO:
 * @param p_client pointer to client object
 * @return err_val enum
 ******************************************************************************
 */
int game_session_end(struct client *p_client);

/**
 ******************************************************************************
 * @brief TODO:
 * @param p_client pointer to client object
 * @return err_val enum
 ******************************************************************************
 */
int game_bulls(char *a, char *b);

/**
 ******************************************************************************
 * @brief TODO:
 * @param p_client pointer to client object
 * @return err_val enum
 ******************************************************************************
 */
int game_cows(char *a, char *b);


#endif // __GAME_H__
