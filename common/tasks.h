/**
 * ISP_HW_4_2020
 * Bulls & Cows
 *
 * messages.h
 * 
 * this is the header file of the tasks module
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

// time conversion
#define MS2US                1000
#define SEC2MS               1000
#define MIN2SEC              60

// max port number
#define MAX_PORT             65536

/*
 ==============================================================================
 * ENUMERATIONS
 ==============================================================================
 */

enum err_val {
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
	E_GRACEFUL,
	E_MAX
};

/*
 ==============================================================================
 * MACROS
 ==============================================================================
 */

// print macros
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define DBG_STAMP() printf("[%-14s;%-3d] ", __FILENAME__, __LINE__)

// print error message
#define PRINT_ERROR(err_val)   do {DBG_STAMP(); print_error((err_val));} while (0)

// bit operations
#define BIT(pos) (1 << (pos))
#define SET_BIT(map, pos) ((map) |= BIT(pos))
#define CLR_BIT(map, pos) ((map) &= ~BIT(pos))
#define TEST_BIT(map, pos) (!!((map) & BIT(pos)))

/*
 ==============================================================================
 * DECLARATIONS
 ==============================================================================
 */

/**
 ******************************************************************************
 * @brief print error message to stdin
 * @param err_val (enum err_val)
 ******************************************************************************
 */
void print_error(int err_val);


#endif // __TASKS_H__
