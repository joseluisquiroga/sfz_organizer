

/*************************************************************

This file is part of gateprog.

gateprog is free software: you can redistribute it and/or modify
it under the terms of the version 3 of the GNU General Public 
License as published by the Free Software Foundation.

gateprog is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gateprog.  If not, see <http://www.gnu.org/licenses/>.

------------------------------------------------------------

Copyright (C) 2020. QUIROGA BELTRAN, Jose Luis.
Id (cedula): 79523732 de Bogota - Colombia.
See https://github.com/gateprog

gateprog is free software thanks to The Glory of Our Lord 
	Yashua Melej Hamashiaj.
Our Resurrected and Living, both in Body and Spirit, 
	Prince of Peace.

------------------------------------------------------------

dbg_util.h

func to print a stack trace.

--------------------------------------------------------------*/

#ifndef ZO_DBG_UTIL_H
#define ZO_DBG_UTIL_H

#include <iostream>
#include <sstream>
#include <fstream>	// used for bj_ofstream
#include <string>

#define NULL_PT NULL
#define zo_null NULL

#define zo_byte uint8_t
#define zo_string std::string

typedef std::ostream zo_ostream;
typedef std::ostringstream zo_ostr_stream;
typedef std::ofstream zo_ofstream;
#define zo_eol std::endl
#define zo_out std::cout
#define zo_err std::cerr
#define zo_dbg std::cout

typedef zo_string::size_type str_pos_t;

template <bool> struct ILLEGAL_USE_OF_OBJECT;
template <> struct ILLEGAL_USE_OF_OBJECT<true>{};
#define OBJECT_COPY_ERROR ILLEGAL_USE_OF_OBJECT<false>()

#define zo_c_decl extern "C"

#define ZO_STRINGIFY(x) #x
#define ZO_TOSTRING(x) ZO_STRINGIFY(x)

#define ZO_MAX_STR_SZ 300

// -----------------

#define ZO_MAX_CALL_STACK_SZ 100
#define ZO_PRT_SIZE_T "%lu"

#define ZO_ABORT_MSG(msg) zo_cstr("ABORTING. '" msg "' at " __FILE__ "(" ZO_TOSTRING(__LINE__) ")")

#define ZO_INFO_STR "Passed " __FILE__ "(" ZO_TOSTRING(__LINE__) ")"

#ifdef __cplusplus
zo_c_decl {
#endif

#ifdef FULL_DEBUG
#	define ZO_DBG(prm) prm
#else
#	define ZO_DBG(prm) /**/ 
#endif

#define ZO_MARK_USED(X)  ((void)(&(X)))

#ifdef __cplusplus
}
#endif

std::string zo_get_stack_trace( const std::string & file, int line );
void zo_ptr_call_stack_trace(FILE* out_fp);
void zo_abort_func(bool prt_stk = false);

#define ZO_STACK_STR zo_get_stack_trace(__FILE__, __LINE__)

const char* zo_get_ptd_log_fnam();
bool zo_call_assert(char* out_fnam, bool is_assert, bool prt_stck, bool vv_ck, 
				const char* file, int line, const char* ck_str, const char* fmt, ...);

#define zo_abort(...) \
{ \
	fprintf(stderr, "\nABORTING.\n"); \
	zo_ptr_call_stack_trace(zo_null); \
	fprintf(stderr, __VA_ARGS__); \
	zo_abort_func(); \
} \

// end_define


//======================================================================
// top_exception

class top_exception {
private:
	top_exception&  operator = (top_exception& other){
		zo_abort("INVALID OPERATOR ON top_exception");
		return (*this);
	}
	
public:
	long		ex_id;
	zo_string 	ex_stk;
	zo_string 	ex_assrt;
	
	top_exception(long the_id = 0, zo_string assrt_str = ""){
		ex_id = the_id;
		ex_stk = ZO_STACK_STR;
		ex_assrt = assrt_str;
	}
	
	~top_exception(){
	}
};

//======================================================================
// mem_exception

typedef enum {
	mex_memout_in_mem_alloc_1,
	mex_memout_in_mem_alloc_2,
	mex_memout_in_mem_sec_re_alloc_1,
	mex_memout_in_mem_re_alloc_1,
	mex_memout_in_mem_re_alloc_2
} mem_ex_cod_t;


class mem_exception : public top_exception {
public:
	mem_exception(long the_id = 0) : top_exception(the_id)
	{}
};


//======================================================================
// dbg macros

#define	ZO_DBG_COND_COMM(cond, comm)	\
	ZO_DBG( \
		if(cond){ \
			zo_ostream& os = zo_dbg; \
			comm; \
			os << '\n'; \
			os.flush(); \
		} \
	) \

//--end_of_def

#define ZO_CK_2(prm, comms1) \
	ZO_DBG_COND_COMM((! (prm)), \
		comms1; \
		os << '\n'; \
		os.flush(); \
		ZO_CK(prm); \
	) \
	
//--end_of_def



#define ZO_CODE(cod) cod
#define ZO_DBG_CODE(cod) ZO_DBG(cod)

#define ZO_CK(vv) ZO_DBG( \
	zo_call_assert(zo_null, true, true, vv, __FILE__, __LINE__, #vv, zo_null))

//	(! vv)?(zo_call_assert(zo_null, true, true, vv, __FILE__, __LINE__, #vv, __VA_ARGS__), 0):(0))
//	zo_call_assert(zo_null, true, true, vv, __FILE__, __LINE__, #vv, __VA_ARGS__))

#define ZO_CK_PRT(vv, ...) ZO_DBG( \
	(! vv)?(zo_call_assert(zo_null, true, true, vv, __FILE__, __LINE__, #vv, __VA_ARGS__), 0):(0))

#define ZO_CK_LOG(vv, ...) ZO_DBG( \
	zo_call_assert(zo_get_ptd_log_fnam(), true, true, vv, __FILE__, __LINE__, #vv, __VA_ARGS__))

#define ZO_COND_LOG(cond, ...) ZO_DBG( \
	zo_call_assert(zo_get_ptd_log_fnam(), false, false, cond, __FILE__, __LINE__, #cond, __VA_ARGS__))

#define ZO_LOG(...) ZO_COND_LOG(true, __VA_ARGS__)

#define ZO_COND_PRT(cond, ...) ZO_DBG( \
	zo_call_assert(zo_null, false, false, cond, __FILE__, __LINE__, #cond, __VA_ARGS__))

#define ZO_PRT(...) ZO_COND_PRT(true, __VA_ARGS__)

#define ZO_PRINTF(...) ZO_DBG(printf(__VA_ARGS__))

#define ZO_PRT_STACK(cond, ...) ZO_DBG( \
	zo_call_assert(zo_null, false, true, cond, __FILE__, __LINE__, #cond, __VA_ARGS__))

#define ZO_LOG_STACK(cond, ...) ZO_DBG( \
	zo_call_assert(zo_get_ptd_log_fnam(), false, true, cond, __FILE__, __LINE__, #cond, __VA_ARGS__))



#endif		// ZO_DBG_UTIL_H


