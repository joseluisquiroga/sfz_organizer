

/*************************************************************

This file is part of sfz_organizer.

sfz_organizer is free software: you can redistribute it and/or modify
it under the terms of the version 3 of the GNU General Public 
License as published by the Free Software Foundation.

sfz_organizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with sfz_organizer.  If not, see <http://www.gnu.org/licenses/>.

------------------------------------------------------------

Copyright (C) 2020. QUIROGA BELTRAN, Jose Luis.
Id (cedula): 79523732 de Bogota - Colombia.
See https://github.com/open-soundfonts/sfz_organizer

sfz_organizer is free software thanks to The Glory of Our Lord 
	Yashua Melej Hamashiaj.
Our Resurrected and Living, both in Body and Spirit, 
	Prince of Peace.

------------------------------------------------------------

sfz_org.h


--------------------------------------------------------------*/

#ifndef SFZ_ORG_H
#define SFZ_ORG_H

#include "dbg_util.h"

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include <vector>
#include <map>
#include <set>

typedef enum {
	sfz_cannot_open,
	sfz_read_1_and_2_differ,
	sfz_pth_not_absolute,
	sfz_pth_not_exists,
	sfz_pth_not_directory,
	sfz_base_not_absolute,
	sfz_base_not_exists,
	sfz_base_not_directory,
} sfz_ex_cod_t;


const std::error_category& zo_err_category();

inline std::error_code make_zo_err(int val){
  return std::error_code(val, zo_err_category());
}


class sfz_exception : public top_exception {
public:
	zo_string f_nm;
	sfz_exception(long the_id = 0, zo_string ff = "unknow_file") : top_exception(the_id)
	{
		f_nm = ff;
	}
};

using zo_str_vec = std::vector<std::string>;

//======================================================================
// zo_ref

#define ZO_INVALID_LINE_NUM -1

using zo_path = fs::path;

class zo_ref;
class zo_sfont;
class zo_sample;
class zo_dir;
class zo_orga;

//using zo_ref_pt = std::shared_ptr<zo_ref>;
//using zo_sfont_pt = std::shared_ptr<zo_sfont>;
//using zo_sample_pt = std::shared_ptr<zo_sample>;

enum class zo_action {
	nothing,
	move,
	copy,
	purge,
	normalize,
	add_sfz
};

zo_string get_action_str(zo_action ac){
	switch(ac){
		case zo_action::nothing:
			return "nothing";
		case zo_action::move:
			return "move";
		case zo_action::copy:
			return "copy";
		case zo_action::purge:
			return "purge";
		case zo_action::normalize:
			return "normalize";
		case zo_action::add_sfz:
			return "add_sfz";
		default:
			return "invalid_action";
	}
	return "invalid_action";
}

enum class zo_policy {
	replace,
	keep
};

enum class zo_ftype {
	soundfont,
	sample
};

inline
bool
is_move_oper(zo_action act){
	switch(act){
		case zo_action::add_sfz: 
		case zo_action::normalize: 
		case zo_action::purge: 
		case zo_action::move: 
			return true;
		case zo_action::copy: 
			return false;
		default:
			return false;
	}
	return false;
}

using zo_ref_pt = zo_ref*;
using zo_sfont_pt = zo_sfont*;
using zo_sample_pt = zo_sample*;

class zo_fname {
public:
	zo_string 	orig_pth{""};
	zo_string 	nxt_pth{""};
	bool 		is_confl{false};

	void print_actions(zo_orga& org, bool only_orig);
	void keep_same(){
		nxt_pth = orig_pth;
	}
	void calc_next(zo_orga& org, bool cmd_sel, bool can_mv = true);
	
	bool is_same(){
		return (nxt_pth.empty() || (orig_pth == nxt_pth));
	}
};

class zo_ref {
	zo_ref(zo_ref& rr) = delete;
	zo_ref(zo_ref&& rr) = delete;
	zo_ref& operator = (const zo_ref& rr) = delete;
	zo_ref& operator = (zo_ref&& rr) = delete;

public:
	zo_sfont_pt		owner = zo_null;
	long			num_line{ZO_INVALID_LINE_NUM};
	zo_string 		prefix{""};
	zo_sample_pt	sref = zo_null;
	zo_string 		suffix{""};
	zo_string 		bad_pth{""};
	
	zo_ref(zo_sfont_pt fl, long ln_num, zo_sample_pt rf){
		ZO_CK(fl != zo_null);
		ZO_CK(rf != zo_null);
		
		owner = fl;
		num_line = ln_num;
		sref = rf;
	}
	
	~zo_ref(){
		num_line = ZO_INVALID_LINE_NUM;
		owner = zo_null;
		sref = zo_null;
		//fprintf(stdout, "Calling ~zo_ref\n");
	}
	
	zo_fname& sf_name();
	
	const zo_string& get_orig();
	const zo_string& get_next();
	const zo_string get_orig_rel();
	const zo_string get_next_rel();
	
	bool is_same();
	
	void write_ref(std::ofstream& dst, const zo_string& ln);
	void print_actions(zo_orga& org);
};

using zo_ref_vec = std::vector<zo_ref_pt>;

class zo_sfont {
	zo_sfont(zo_sfont& rr) = delete;
	zo_sfont(zo_sfont&& rr) = delete;
	zo_sfont& operator = (const zo_sfont& rr) = delete;
	zo_sfont& operator = (zo_sfont&& rr) = delete;
	
public:
	zo_fname	fpth;
	bool 		did_it{false};
	zo_ref_vec	all_ref;
	
	bool 		is_txt{false};
	long 		tot_spl_ref{0};

	bool 		can_move{false};
	bool 		cmd_sel{false};
	
	zo_sfont(const zo_path& fl){
		ZO_CK(fl.is_absolute());
		fpth.orig_pth = zo_path{fl};
	}
	
	~zo_sfont(){
		//fprintf(stdout, "Calling ~zo_sfont\n");
	}

	const zo_string& get_orig(){
		return fpth.orig_pth;
	}
	
	const zo_string& get_next(){
		return fpth.nxt_pth;
	}
	
	bool is_same();
	
	void get_samples(zo_orga& org);
	
	void print_actions(zo_orga& org);	
	void do_actions(zo_orga& org);
	void prepare_normalize(zo_orga& org);
	void prepare_tmp_file(const zo_path& tmp_pth);
	
	void prepare_add_sfz_ext(zo_orga& org);
	void prepare_purge(zo_orga& org);
	void prepare_copy_or_move(zo_orga& org);
	
};

using zo_ptsfont_vec = std::vector<zo_sfont_pt>;
using zo_sfont_map = std::map<zo_string, zo_sfont_pt>;
using zo_sample_map = std::map<zo_string, zo_sample_pt>;
using zo_file_set = std::set<zo_string>;

class zo_sample {
	zo_sample(zo_sample& rr) = delete;
	zo_sample(zo_sample&& rr) = delete;
	zo_sample& operator = (const zo_sample& rr) = delete;
	zo_sample& operator = (zo_sample&& rr) = delete;
	
public:
	zo_fname		fpth;
	bool 			did_it{false};
	zo_sfont_map	all_bk_ref;

	bool 		cmd_sel{false};
	
	zo_sample(zo_path fl){
		fpth.orig_pth = zo_path{fl};
	}
	
	~zo_sample(){
		//fprintf(stdout, "Calling ~zo_sample\n");
	}
	
	const zo_string& get_orig(){
		return fpth.orig_pth;
	}
	
	const zo_string& get_next(){
		return fpth.nxt_pth;
	}

	bool is_same(){
		return fpth.is_same();
	}
	
	void print_actions(zo_orga& org);
	void do_actions(zo_orga& org);
	void prepare_normalize(zo_orga& org);
	void prepare_purge(zo_orga& org);
	void prepare_copy_or_move(zo_orga& org);
};


inline 
zo_ref_pt
make_ref_pt(zo_sfont_pt fl, long lnum, zo_sample_pt spl){
	return new zo_ref(fl, lnum, spl);
	//return std::make_shared<zo_ref>(fl, lnum, spl);
}

inline 
zo_sfont_pt
make_sfont_pt(const zo_path& pth){
	return new zo_sfont(pth);
	//return std::make_shared<zo_sfont>(pth);
}

inline 
zo_sample_pt
make_sample_pt(const zo_path& pth){
	return new zo_sample(pth);
	//return std::make_shared<zo_sample>(pth);
}


class zo_last_confl {
public:
	long 	val{0};
};

using zo_last_confl_pt = zo_last_confl*;
using zo_conflict_map = std::map<zo_string, zo_last_confl_pt>;

inline 
zo_last_confl_pt
make_last_confl_pt(){
	return new zo_last_confl;
}


class zo_dir {
	zo_dir(zo_dir& rr) = delete;
	zo_dir(zo_dir&& rr) = delete;
	zo_dir& operator = (const zo_dir& rr) = delete;
	zo_dir& operator = (zo_dir&& rr) = delete;

public:
	zo_path 			base_pth{""};
	
	zo_file_set			all_to_ignore;
	
	zo_sfont_map 		all_read_sfz;
	zo_sample_map 		all_read_spl;
	
	zo_sfont_map 		all_selected_sfz;
	zo_sample_map 		all_selected_spl;
	
	zo_conflict_map		all_unique_nxt;
	long 				tot_conflict{0};

	zo_sample_pt 		bad_spl{zo_null};
	
	zo_dir(){
		bad_spl = make_sample_pt("");
	}
	
	~zo_dir(){
		//fprintf(stdout, "Calling ~zo_dir\n");
	}
	
	zo_sfont_pt get_read_soundfont(const zo_path& pth, bool& is_nw){
		is_nw = false;
		ZO_CK(pth.is_absolute());
		ZO_CK(fs::exists(pth));
		auto it = all_read_sfz.find(pth);
		if(it != all_read_sfz.end()){
			zo_sfont_pt sfz = it->second;
			ZO_CK(sfz != zo_null);
			//std::cout << "already added " << pth << "\n";
			return sfz;
		}
  
		std::cout << "reading " << pth << "\n";
		is_nw = true;
		zo_sfont_pt nw_sfz = make_sfont_pt(pth);
		all_read_sfz[pth] = nw_sfz;
		return nw_sfz;
	}
	
	zo_sample_pt get_read_sample(const zo_path& pth, bool& is_nw){
		is_nw = false;
		ZO_CK(pth.is_absolute());
		ZO_CK(fs::exists(pth));
		auto it = all_read_spl.find(pth);
		if(it != all_read_spl.end()){
			zo_sample_pt spl = it->second;
			ZO_CK(spl != zo_null);
			//std::cout << "already added " << pth << "\n";
			return spl;
		}
  
		//std::cout << "reading " << pth << "\n";
		is_nw = true;
		zo_sample_pt nw_spl = make_sample_pt(pth);
		all_read_spl[pth] = nw_spl;
		return nw_spl;
	}
	
	zo_sfont_pt get_selected_soundfont(const zo_string& pth, zo_sfont_pt sf, bool& is_nw){
		ZO_CK(sf != zo_null);
		is_nw = false;
		auto it = all_selected_sfz.find(pth);
		if(it != all_selected_sfz.end()){
			zo_sfont_pt sfz = it->second;
			ZO_CK(sfz != zo_null);
			return sfz;
		}
  
		std::cout << ">>>SELECTING " << pth << "\n";
		is_nw = true;
		all_selected_sfz[pth] = sf;
		return sf;
	}
	
	zo_sample_pt get_selected_sample(const zo_string& pth, zo_sample_pt sp, bool& is_nw, bool add_it){
		ZO_CK(sp != zo_null);
		is_nw = false;
		auto it = all_selected_spl.find(pth);
		if(it != all_selected_spl.end()){
			zo_sample_pt spl = it->second;
			ZO_CK(spl != zo_null);
			return spl;
		}
		if(! add_it){
			return zo_null;
		}
  
		std::cout << ">>>SELECTING " << pth << "\n";
		is_nw = true;
		all_selected_spl[pth] = sp;
		return sp;
	}
	
	void print_actions(zo_orga& org);
	void do_actions(zo_orga& org);
};

class zo_orga : public zo_dir {
	zo_orga(zo_orga& rr) = delete;
	zo_orga(zo_orga&& rr) = delete;
	zo_orga& operator = (const zo_orga& rr) = delete;
	zo_orga& operator = (zo_orga&& rr) = delete;
	
public:
	bool just_list = false; // list option
	bool recursive = false; // recursive option
	bool only_sfz = false; // recursive option
	bool follw_symlk = false;
	bool samples_too = false;
	bool hidden_too = false;
	bool skip_normalize = false;
	bool force_action = false;
	
	zo_policy pol{zo_policy::keep}; // replace | keep options
	
	zo_path  dir_from{""};	// from option
	zo_path  dir_to{""};	// to option
	zo_string regex_str{""};	// regex option
	zo_string match_str{"(.*)"};	// match option
	zo_string subst_str{""}; // substitute option
	zo_action oper{zo_action::nothing};

	std::regex select_rx;
	std::regex match_rx;
	
	zo_string tmp_nam{".temp_sfz_organizer_file"};
	zo_path tmp_pth{""};

	zo_str_vec f_names;
	
	zo_path last_pth{""};
	bool gave_names{false};

	zo_path target{""};
	
	zo_orga(){
	}
	
	~zo_orga(){
		//fprintf(stdout, "Calling ~zo_orga\n");
	}
	
	void ignore_purged_dir();
	
	bool get_args(const zo_str_vec& args);
	
	bool is_move(){
		return is_move_oper(oper);
	}
	
	void read_file(const zo_path& pth, const zo_ftype ft, const bool only_with_ref);
	void read_dir_files(zo_path pth_dir, const zo_ftype ft, const bool only_with_ref);
	
	void read_files(const zo_str_vec& all_pth, const zo_ftype ft);
	void read_selected();
	
	const zo_path& get_temp_path(){
		ZO_CK(! tmp_pth.empty());
		return tmp_pth;
	}
	
	bool calc_target(bool had_dir_to);

	void prepare_normalize();
	void prepare_add_sfz_ext();
	void prepare_purge();
	void prepare_copy_or_move();
	void organizer_main(const zo_str_vec& args);
};

#endif		// SFZ_ORG_H


