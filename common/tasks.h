/**
 * ISP_HW_4_2020
 * Bulls & Cows
 * client program
 *
 * messages.h
 * 
 * this is the header file of the messages module
 * 
 * by: Chaim Gruda
 *     Nir Beiber
 */

#ifndef __TASKS_H__
#define __TASKS_H__

/*
 ==============================================================================
 * INCLUDES
 ==============================================================================
 */

#include <stdio.h>
#include <string.h>

/*
 ==============================================================================
 * DEFINES
 ==============================================================================
 */
// debug prints enable
#define DBG_ENABLE     1

// for debug use
#define __FILENAME__   (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// times
#define MS2US                1000
#define SEC2MS               1000
#define MIN2SEC              60

#define MAX_PORT             65536

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

enum err_val
{
	E_SUCCESS = 0,
	E_FAILURE,
	E_INTERNAL,
	E_TIMEOUT,
	E_MESSAGE,
	E_INPUT,
	E_STDLIB,
	E_WINAPI,
	E_WINSOCK,
	E_FLOW,
	E_MAX
};

/*
 ==============================================================================
 * MACROS
 ==============================================================================
 */
// debug stamp [file;line]
#define DBG_STAMP()     printf("[%-14s;%-3d] ", __FILENAME__, __LINE__)


// for debuging
#if DBG_ENABLE
#define DBG_PRINT(...)  do {DBG_STAMP(); printf(__VA_ARGS__);} while (0)
#define DBG_FUNC_STAMP()  do {DBG_STAMP(); printf("$$$ %s\n", __func__);} while (0)
#else
#define DBG_PRINT(...)
#endif

// print error message
#define PRINT_ERROR(err_val)   do {DBG_STAMP(); print_error((err_val));} while (0)

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

/**
 ******************************************************************************
 * @brief TODO:
 * @param 
 * @return 
 ******************************************************************************
 */
int my_atoi(char *str, int *p_result);

/**
 ******************************************************************************
 * @brief print error message to stdin
 * @param err_val (enum err_val)
 ******************************************************************************
 */
void print_error(int err_val);

#endif // __TASKS_H__
