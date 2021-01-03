/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * server program
 *
 * game.h
 * 
 * game module handles game tasks, and communication
 * between thread over the GameSession.txt file.
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
#define GAME_OPP_WAIT_TIME_MS         29000 // FIXME:
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
 * @brief initialize game struct
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
 * @brief start game session:
 *        first thread to call function creates the game
 *        second thread to call function joins the game
 * @param p_client pointer to client struct
 * @return E_SUCCESS on success
 ******************************************************************************
 */
int game_session_start(struct client *p_client);

/**
 ******************************************************************************
 * @brief end a game session:
 *        first thread to call this function leaves the game
 *        second thread to call this function destroys the game
 * @param p_client pointer to client struct
 * @return E_SUCCESS on success
 ******************************************************************************
 */
int game_session_end(struct client *p_client);

/**
 ******************************************************************************
 * @brief sync threads, and allow passing data between them
 * @param p_client pointer to client struct
 * @param write_buff data to be written in GameSession file
 * @param read_buff data that has been read from GameSession file
 * @return E_SUCCESS on success
 *         E_TIMEOUT if sync with secind thread is lost
 *         other err_vals on error
 ******************************************************************************
 */
int game_sequence(struct client *p_client, char *write_buff, char *read_buff);

/**
 ******************************************************************************
 * @brief count bulls by comparing 2 strings
 * @param a string
 * @param b string
 * @return number of bulls
 ******************************************************************************
 */
int game_bulls(char *a, char *b);

/**
 ******************************************************************************
 * @brief count cows by comparing 2 strings
 * @param a string
 * @param b string
 * @return number of cows
 ******************************************************************************
 */
int game_cows(char *a, char *b);


#endif // __GAME_H__
