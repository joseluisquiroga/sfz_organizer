

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

dbg_util.cpp  

dbg_util funcs.

--------------------------------------------------------------*/

#include <sstream>
#include <execinfo.h>
#include <cstdlib>

#include <string.h>
#include <stdarg.h>

#include "dbg_util.h"

std::string 
zo_get_stack_trace( const std::string & file, int line ){
	std::ostringstream result;
	result << "Call Stack from " << file << ":" << line << "\n";
	const size_t k_max_depth = 100;
	void *stack_addrs[k_max_depth];
	size_t stack_depth;
	char **stack_strings;

	stack_depth = backtrace( stack_addrs, k_max_depth );
	stack_strings = backtrace_symbols( stack_addrs, stack_depth );
	for( size_t i = 1; i < stack_depth; ++i ) {
		//result << "   " << demangle_cxx_name( stack_strings[i] ) << "\n";
		result << "   " << stack_strings[i] << "\n";
	}
	result << "(to see full call names link with -rdynamic option)\n";
	std::free( stack_strings );

	return result.str();
}

void
zo_ptr_call_stack_trace(FILE* out_fp) {
	if(out_fp == zo_null){
		out_fp = stderr;
	}

	void* trace[ZO_MAX_CALL_STACK_SZ];
	memset((uint8_t*)trace, 0, ZO_MAX_CALL_STACK_SZ * sizeof(void*));

	size_t trace_sz = backtrace(trace, ZO_MAX_CALL_STACK_SZ);

	fprintf(out_fp, "trace_size=" ZO_PRT_SIZE_T " \n", trace_sz);

	char **stack_strings = backtrace_symbols(trace, trace_sz);
	fprintf(out_fp, "STACK_STRACE------------------------------\n");
	for( size_t ii = 1; ii < trace_sz; ii++ ) {
		if(ii >= ZO_MAX_CALL_STACK_SZ){ break; }
		fprintf(out_fp, "%s \n", stack_strings[ii]);
	}
	fprintf(out_fp, "------------------------------(to see names link with -rdynamic)\n");
	free( stack_strings );
	fflush(out_fp);
}

void 
zo_abort_func(bool prt_stk){
	if(prt_stk){
		FILE* out_file = stderr;
		fprintf(out_file, "\nABORTING.\n");
		zo_ptr_call_stack_trace(zo_null);
		fprintf(out_file, "\n\n");
	}

	exit(EXIT_FAILURE);
}

const char* zo_get_ptd_log_fnam(){
	return "hlang_log_file.txt";
}

bool 
zo_call_assert(char* out_fnam, bool is_assert, bool prt_stck, bool cond, 
		const char* fnam, int line, const char* ck_str, const char* fmt, ...)
{
	FILE* out_file = stderr;
	if(out_fnam != zo_null){
		out_file = fopen(out_fnam, "a");
		if(out_file == NULL){
			out_file = stderr;
		}
	}

	bool is_asst = (is_assert && ! cond);
	bool is_prt = (! is_assert && cond);
	bool do_prt = is_asst || is_prt;
	if(do_prt){
		flockfile(stdout);
		flockfile(out_file);
		fflush(stdout); 
		fflush(out_file); 

		if(is_assert || prt_stck){
			fprintf(out_file, "\n------------------------------------------------------------------\n");
		}
		//if(MANAGERU_THREAD_ID != 0){
		//	ptd_info_t* inf = mck_get_ptd_info();
		//	fprintf(out_file, "%d:%x --> ", inf->ptd_num, inf->ptd_workeru_id);
		//}
		if(fnam != zo_null){
			fprintf(out_file, "FILE %s(%d): ", fnam, line);
		}
		if(is_assert && (ck_str != zo_null)){
			fprintf(out_file, "ASSERT '%s' FAILED.\n", ck_str);
		} 
		if(prt_stck){
			zo_ptr_call_stack_trace(out_file);
		}

		if(fmt != NULL){
			char prt_buff[ZO_MAX_STR_SZ];
			va_list ap;

			va_start(ap, fmt);
			int size = vsnprintf(prt_buff, ZO_MAX_STR_SZ, fmt, ap);
			va_end(ap);

			prt_buff[ZO_MAX_STR_SZ - 1] = '\0';

			if(size < 0){ 
				fprintf(out_file, "FATAL_ERROR_DURING_ASSERT.\n");
				exit(EXIT_FAILURE);
			}

			fprintf(out_file, "%s", prt_buff);
		}
		if(is_assert || prt_stck){
			fprintf(out_file, "\n------------------------------------------------------------------\n\n");
		}
		fflush(out_file); 

		funlockfile(stdout);
		funlockfile(out_file);
	}

	if((out_fnam != zo_null) && (out_file != stderr)){
		fclose(out_file);
	}

	if(is_assert && ! cond){
		fprintf(out_file, "ASSERT_FAILED.\n");
		exit(EXIT_FAILURE);
	}
	return cond;
}

