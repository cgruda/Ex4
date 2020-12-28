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
#define DBG_TRACE      1
#define SERVER		"server"

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





#if DBG_TRACE

enum dbg_trace_mode
{
	C,
	S,
	T,
	DBG_TRACE_MODE_MAX,
};

#define DBG_TRACE_INIT(mode, name) dbg_trace_init(mode, name)
#define DBG_TRACE_LOG(mode, name, str) dbg_trace_log(mode, name, str)

#define DBG_TRACE_STAMP(mode, name) do {							\
		char dbg_trace_stamp_str[100] = {0};						\
		sprintf(dbg_trace_stamp_str, "[%-14s;%-3d] ", __FILENAME__, __LINE__);		\
		DBG_TRACE_LOG(mode, name, dbg_trace_stamp_str);					\
	} while (0)

#define DBG_TRACE_STR(mode, name, str) do {							\
		DBG_TRACE_STAMP(mode, name);							\
		DBG_TRACE_LOG(mode, name, str);							\
		DBG_TRACE_LOG(mode, name, "\n");						\
	} while (0)

#define DBG_TRACE_FUNC(mode, name) do { 							\
		DBG_TRACE_STR(mode, name, __func__);						\
	} while (0)

#define DBG_TRACE_MSG(mode, name, p_msg) do {							\
		char *dbg_trace_msg_str = dbg_trace_msg(p_msg);					\
		DBG_TRACE_LOG(mode, name, dbg_trace_msg_str);					\
		free(dbg_trace_msg_str);							\
	} while (0)

char *dbg_trace_get_path(int mode, char *name);
void dbg_trace_init(int mode, char *name);
void dbg_trace_log(int mode, char *name, char *str);

#endif



#endif // __TASKS_H__
