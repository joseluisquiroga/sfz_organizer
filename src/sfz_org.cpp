

#include <stdio.h>

#include <algorithm> 
#include <cctype>
#include <locale>
#include <regex>
#include <vector>
#include <array>

#include <chrono>

#include "is_utf8.h"
#include "sfz_org.h"

void
print_version(const zo_str_vec& args){
	fprintf(stdout, "%s 0.1\n", args[0].c_str());
}

class zo_err_cat : public std::error_category
{
	public:
	virtual const char *name() const noexcept override {
		return "zo_err";
	}
	
	virtual std::string message(int val) const override {
		std::string msg = "zo_err" + std::to_string(val);
		return msg;
	}
};

const std::error_category& zo_err_category(){
  static zo_err_cat instance;
  return instance;
}

std::regex ZO_INCR_PATTERN{R"(_c([[:digit:]]*)\.)"};
std::regex ZO_INCR_DOT_PATTERN{R"(\.)"};

int sfz_organizer_main(int argc, char* argv[]){
	zo_str_vec args{argv, argv + argc};
	zo_orga org;
	org.organizer_main(args);
	return 0;
}

void
normalize_name(zo_string& nm){
	std::size_t pos = nm.rfind('.');
	for(char& ch : nm){
		if(! isalnum(ch)){
			ch = '_';
		}
	}
	if(pos != std::string::npos){
		nm[pos] = '.';
	}
}

void
set_num_name(zo_string& nm, long val){
	zo_string vstr = std::to_string(val);
	std::smatch incr_matches;
	if(regex_search(nm, incr_matches, ZO_INCR_PATTERN)){
		ZO_CK(incr_matches.size() > 1);
		zo_string m0 = incr_matches[0];
		zo_string pfx = incr_matches.prefix().str();
		zo_string sfx = incr_matches.suffix().str();
		nm = pfx + "_c" + vstr + "." + sfx;
		return;
	} 
	if(regex_search(nm, incr_matches, ZO_INCR_DOT_PATTERN)){
		zo_string pfx = incr_matches.prefix().str();
		zo_string sfx = incr_matches.suffix().str();
		nm = pfx + "_c" + vstr + "." + sfx;
		return;
	}
	nm += "_c" + vstr + ".";
	return;
}


zo_string ZO_SFZ_EXT = ".sfz";
zo_string ZO_SAMPLE_STR = "sample";
zo_string ZO_SAMPLE_LINE_PATTERN_STR = R"(sample\s*=)";
zo_string ZO_OPCODE_PATTERN_STR = R"((\w*)\s*=)";

constexpr long ZO_BUFFER_SZ = 1024;
unsigned char ZO_BUFFER[ZO_BUFFER_SZ];

std::regex ZO_SAMPLE_LINE_PATTERN{ZO_SAMPLE_LINE_PATTERN_STR.c_str()};
std::regex ZO_OPCODE_PATTERN{ZO_OPCODE_PATTERN_STR.c_str()};

zo_path 
find_relative(const zo_path& pth, const zo_path& base, std::error_code& ec, bool do_checks = true){
	if(do_checks){
		if(! pth.is_absolute()){
			fprintf(stdout, "sfz_pth_not_absolute:'%s'\n", pth.c_str());
			ec = make_zo_err(sfz_pth_not_absolute);
			return "";
		}
		if(! fs::exists(pth)){
			fprintf(stdout, "sfz_pth_not_exists:'%s'\n", pth.c_str());
			ec = make_zo_err(sfz_pth_not_exists);
			return "";
		}
		if(! fs::is_directory(pth)){
			fprintf(stdout, "sfz_pth_not_directory:'%s'\n", pth.c_str());
			ec = make_zo_err(sfz_pth_not_directory);
			return "";
		}

		if(! base.is_absolute()){
			fprintf(stdout, "sfz_base_not_absolute:'%s'\n", base.c_str());
			ec = make_zo_err(sfz_base_not_absolute);
			return "";
		}
		if(! fs::exists(base)){
			fprintf(stdout, "sfz_base_not_exists:'%s'\n", base.c_str());
			ec = make_zo_err(sfz_base_not_exists);
			return "";
		}
		if(! fs::is_directory(base)){
			fprintf(stdout, "sfz_base_not_directory:'%s'\n", base.c_str());
			ec = make_zo_err(sfz_base_not_directory);
			return "";
		}
	}

	if(pth.root_path() != base.root_path()){
		return pth;
	}
	
	//fprintf(stdout, "pth:%s\n", pth.c_str()); // dbg_prt
	//fprintf(stdout, "base:%s\n", base.c_str()); // dbg_prt

	zo_path result;

	auto itr_path = pth.begin();
	auto itr_bs = base.begin();
	while(itr_path != pth.end() && itr_bs != base.end() && ((*itr_path) == (*itr_bs)) ) {
		++itr_path;
		++itr_bs;
	}

	while(itr_bs != base.end() ) {
		result /= "..";
		++itr_bs;
	}

	while( itr_path != pth.end() ) {
		result /= *itr_path;
		++itr_path;
	}

	//fprintf(stdout, "result:%s\n", result.c_str()); // dbg_prt
	return result;
}

const zo_string&
zo_ref::get_orig(){
	ZO_CK(sref != zo_null);
	return sref->get_orig();
}

const zo_string& 
zo_ref::get_next(){
	ZO_CK(sref != zo_null);
	return sref->get_next();
}

const zo_string
get_rel_sample_path(zo_path& sf_pth, zo_path& rf_pth, std::error_code& ec, bool do_checks){
	//fprintf(stdout, "sf_pth:%s\n", sf_pth.c_str()); // dbg_prt
	//fprintf(stdout, "rf_pth:%s\n", rf_pth.c_str()); // dbg_prt
	
	zo_path sf_pnt = sf_pth.parent_path();
	zo_path rf_pnt = rf_pth.parent_path();
	
	zo_path rel_dir = find_relative(rf_pnt, sf_pnt, ec, do_checks);
	zo_path rel_pth = rel_dir / rf_pth.filename();
	return rel_pth;
}

const zo_string
zo_ref::get_orig_rel(){
	ZO_CK(owner != zo_null);
	ZO_CK(sref != zo_null);
	zo_path sf_pth = owner->get_orig();
	zo_path rf_pth = get_orig();
	
	std::error_code ec;
	zo_string rpth = get_rel_sample_path(sf_pth, rf_pth, ec, true);
	ZO_CK(! ec);
	return rpth;
}

const zo_string
zo_ref::get_next_rel(){
	ZO_CK(owner != zo_null);
	ZO_CK(sref != zo_null);
	zo_path sf_pth = owner->get_next();
	if(sf_pth.empty()){
		sf_pth = owner->get_orig();
	}
	zo_path rf_pth = get_next();
	if(rf_pth.empty()){
		rf_pth = get_orig();
	}
	std::error_code ec;
	return get_rel_sample_path(sf_pth, rf_pth, ec, false);
}


// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

zo_path
fix_ref_path(const zo_string& rpth, const zo_path& fpth, std::error_code& ec){
	zo_path pnt = fpth.parent_path();
	zo_string fx = rpth;
	std::replace(fx.begin(), fx.end(), '\\', '/');
	zo_path fxp = fs::canonical(fs::absolute(fx, pnt), ec);
	return fxp;
}

bool
is_text_file(zo_path pth){
	std::ifstream istm;
	istm.open(pth.c_str(), std::ios::binary);
	if(! istm.good() || ! istm.is_open()){
		throw sfz_exception(sfz_cannot_open, pth);
	}
	
	istm.read((char*)ZO_BUFFER, ZO_BUFFER_SZ);

	long tot_read = istm.gcount();
	
	std::string msg;
	int faulty_bytes = 0;
	size_t pos = is_utf8(ZO_BUFFER, tot_read, msg, faulty_bytes);
	
	return (pos == 0);
}

void
zo_sfont::get_samples(zo_orga& org){
	zo_sfont_pt fl = this;
	std::ifstream istm;
	zo_path fl_orig = fl->get_orig();
	istm.open(fl_orig.c_str(), std::ios::binary);
	if(! istm.good() || ! istm.is_open()){
		throw sfz_exception(sfz_cannot_open, fl_orig);
	}
	
	tot_spl_ref = 0;
	
	std::smatch sample_matches;
	std::smatch opcode_matches;
	long lnum = 0;
	zo_string ln;
	for(;getline(istm, ln);){
		lnum++;
		std::size_t pos_spl = ln.find(ZO_SAMPLE_STR);
		bool has_spl = (pos_spl != std::string::npos);
		if(! has_spl){
			continue;
		}
		if(! regex_search(ln, sample_matches, ZO_SAMPLE_LINE_PATTERN)){
			continue;
		}
		//fprintf(stdout, "> %ld:%s\n", lnum, ln.c_str()); // dbg_prt
		zo_string lprefix = "";
		zo_string lref = "";
		zo_string lsuffix = "";
		
		zo_string nxt_pfx = "";
		zo_string opcod = ln;
		bool found_sample_opcode = false;
		while(regex_search(opcod, opcode_matches, ZO_OPCODE_PATTERN)){
			if(found_sample_opcode && nxt_pfx.empty()){
				ZO_CK(opcode_matches.size() > 0);
				nxt_pfx = opcode_matches.prefix().str();
				//fprintf(stdout, "pfx:%s\n", nxt_pfx.c_str()); // dbg_prt
				zo_string m0 = opcode_matches[0];
				lsuffix = m0 + opcode_matches.suffix().str();
				break;
			}
			for(long aa = 0; aa < (long)opcode_matches.size(); aa++){
				zo_string m0 = opcode_matches[aa];
				//fprintf(stdout, "+%ld:%s\n", aa, m0.c_str()); // dbg_prt
				if(m0 == ZO_SAMPLE_STR){
					found_sample_opcode = true;
					lprefix = opcode_matches.prefix().str();
				}
			}
			opcod = opcode_matches.suffix().str();
			//fprintf(stdout, "sfx:%s\n", opcod.c_str()); // dbg_prt
		}

		lref = (! nxt_pfx.empty())?(nxt_pfx):(opcod);
		trim(lref);
		//fprintf(stdout, "lref:'%s'\n", lref.c_str()); // dbg_prt
		
		auto ec = std::error_code{};
		zo_path fx_ref = fix_ref_path(lref, fl_orig, ec);

		//fprintf(stdout, "lprefix:'%s'\n", lprefix.c_str()); // dbg_prt
		//fprintf(stdout, "fx_ref:'%s'\n", fx_ref.c_str()); // dbg_prt
		//fprintf(stdout, "lsuffix:'%s'\n", lsuffix.c_str()); // dbg_prt
		
		bool is_nw = false;
		zo_sample_pt spl = zo_null;
		if(ec){
			spl = org.bad_spl;
		} else {
			spl = org.get_read_sample(fx_ref, is_nw);
			ZO_CK(spl->get_orig() == fx_ref);
			org.get_selected_sample(fx_ref, spl, is_nw, org.samples_too);
		}
		ZO_CK(spl != zo_null);
		tot_spl_ref++;
		
		spl->all_bk_ref[fl_orig] = fl;
		
		auto nw_ref = make_ref_pt(fl, lnum, spl);
		fl->all_ref.push_back(nw_ref);
		
		if(ec){
			nw_ref->bad_pth = lref;
			fprintf(stderr, "bad_ref:'%s' in file %s\n", lref.c_str(), fl_orig.c_str());
		} else {
			nw_ref->prefix = lprefix;
			nw_ref->suffix = lsuffix;
		}
	}
	
}

bool
is_hidden(const zo_string& pth){
	return (! pth.empty() && (pth[0] == '.'));
}

bool
has_sfz_ext(const zo_path& pth){
	zo_string fnam = pth.filename();
	long pos = fnam.length() - ZO_SFZ_EXT.length();
	if(pos < 0){ return false; }
	int is_sfz = fnam.compare(pos, ZO_SFZ_EXT.length(), ZO_SFZ_EXT);
	return (is_sfz == 0);
}

void 
zo_orga::read_file(const zo_path& pth, const zo_ftype ft, const bool only_with_ref){
	zo_orga& org = *this;
	ZO_CK(! only_with_ref || (ft == zo_ftype::soundfont));
	if(! fs::exists(pth)){
		return;
	}
	auto apth = zo_path{fs::canonical(pth)};
	bool is_nw = false;
	bool is_sfz = has_sfz_ext(pth);
	
	auto igt = all_to_ignore.find(apth);
	if(igt != all_to_ignore.end()){
		std::cout << "IGNORING " << apth << "\n";
		return;
	}
	
	bool is_hdn = is_hidden(apth.filename());
	if(is_hdn && ! hidden_too){
		std::cout << "Hidden_file_ignored " << apth << "\n";
		return;
	}

	if(! regex_str.empty()){
		zo_string nm = apth.filename();
		std::smatch fname_matches;
		if(! regex_search(nm, fname_matches, select_rx)){
			std::cout << "File_not_matching_regex_ignored " << apth << "\n";
			return;
		}
	}
	
	bool adding_ext = (oper == zo_action::add_sfz);
	if(adding_ext){
		zo_sfont_pt sf = get_read_soundfont(apth, is_nw);
		if(! is_sfz){
			bool is_txt = is_text_file(apth);
			if(is_txt){
				get_selected_soundfont(apth, sf, is_nw);
			}
		}
		return;
	}
	bool purging = (oper == zo_action::purge);
	bool normal_sfz = true;
	
	if((ft == zo_ftype::soundfont) && is_sfz){
		zo_sfont_pt sf = get_read_soundfont(apth, is_nw);
		sf->can_move = ! only_with_ref;
		sf->cmd_sel = ! only_with_ref;
		if(purging){
			sf->is_txt = is_text_file(apth);
			normal_sfz = sf->is_txt;
		}
		if(is_nw && normal_sfz){
			sf->get_samples(org);
		}
		bool has_ref = ! sf->all_ref.empty();
		if(! only_with_ref || has_ref){
			ZO_CK(! ((oper == zo_action::copy) && only_with_ref));
			get_selected_soundfont(apth, sf, is_nw);
		}
		return;
	}
	if((ft == zo_ftype::sample) && ! is_sfz){
		zo_sample_pt sp = get_read_sample(apth, is_nw);
		sp->cmd_sel = true;
		ZO_CK(sp != zo_null);
		get_selected_sample(apth, sp, is_nw, true);
		return;
	}
}


void 
zo_orga::read_dir_files(zo_path pth_dir, const zo_ftype ft, const bool only_with_ref){
	auto igt = all_to_ignore.find(pth_dir);
	if(igt != all_to_ignore.end()){
		std::cout << "IGNORING " << pth_dir << "\n";
		return;
	}
	bool is_hdn = is_hidden(pth_dir.filename());
	if(is_hdn && ! hidden_too){
		std::cout << "Hidden_directory_ignored " << pth_dir << "\n";
		return;
	}
	
	if(fs::is_symlink(pth_dir)){
		if(! follw_symlk){
			std::cout << "Symlink_ignored " << pth_dir << "\n";
			return;
		}
		pth_dir = fs::read_symlink(pth_dir);
	}
	std::cout << "ENTERING_DIR " << pth_dir << "\n";
	if(fs::exists(pth_dir) && fs::is_directory(pth_dir)){
		for (const auto& entry : fs::directory_iterator(pth_dir)){
			zo_path pth = entry.path();
			auto st = entry.status();
			if(fs::is_directory(st)){
				read_dir_files(entry, ft, only_with_ref);
			} else if(fs::is_regular_file(st)){
				read_file(pth, ft, only_with_ref);
			} 
		}
	}
}

void 
fill_files(const zo_path& pth_dir, zo_str_vec& names){
	ZO_CK(fs::exists(pth_dir));
	ZO_CK(fs::is_directory(pth_dir));
	for (const auto& entry : fs::directory_iterator(pth_dir)){
		zo_path pth = entry.path();
		names.push_back(pth);
	}
}

void 
zo_orga::read_files(const zo_str_vec& all_pth, const zo_ftype ft){
	for(auto nm : all_pth){
		zo_path f_pth = nm;
		if(! fs::exists(f_pth)){
			continue;
		}
		if(fs::is_directory(f_pth)){
			if(! recursive){
				continue;
			}
			read_dir_files(f_pth, ft, false);
		} else if(fs::is_regular_file(f_pth)){
			read_file(f_pth, ft, false);
		} 
	}
}

void 
zo_orga::read_selected(){
	if(f_names.empty() && ! gave_names){
		fill_files(dir_from, f_names);
	}
	
	bool adding_ext = (oper == zo_action::add_sfz);
	if(! only_sfz && ! adding_ext){
		read_files(f_names, zo_ftype::sample);
	}
	
	read_files(f_names, zo_ftype::soundfont);
	
	if(adding_ext){
		return;
	}
	
	long tot_spl = (long)all_selected_spl.size();
	fprintf(stdout, "tot_selected_samples = %ld\n", tot_spl);
	
	bool is_cp = (oper == zo_action::copy);
	if(! is_cp && (tot_spl > 0)){
		ZO_CK(! base_pth.empty());
		read_dir_files(base_pth, zo_ftype::soundfont, true);
	}
}

int test_fs(int argc, char* argv[]){
	//fs::path pth1 = fs::path("/home/jose/Video");
	fs::path pth1 = fs::current_path();

    std::cout << "Current path is " << pth1 << '\n'
              << "Canonical path for " << pth1 << " is " << fs::canonical(pth1) << '\n';
              
	std::cout << "exists() = " << fs::exists(pth1) << "\n"
		<< "root_name() = " << pth1.root_name() << "\n"
		<< "root_path() = " << pth1.root_path() << "\n"
		<< "rel_path() = " << pth1.relative_path() << "\n"
		<< "parent_path() = " << pth1.parent_path() << "\n"
		<< "filename() = " << pth1.filename() << "\n"
		<< "stem() = " << pth1.stem() << "\n"
		<< "extension() = " << pth1.extension() << "\n";   
	 
	int ii = 0;    
	for (const auto& part : pth1){
		std::cout << "path part: " << ii++ << " = " << part << "\n";		
	}
	
	zo_orga org;
	org.read_dir_files(pth1, zo_ftype::soundfont, false);
	
	return 0;
}

int test_rx(int argc, char* argv[]){
	if(argc < 2){
		fprintf(stdout, "%s <f_nam>\n", argv[0]);
		return 0;
	}
	
	zo_string f_nam = argv[1];
	
	fprintf(stdout, "reading %s\n", f_nam.c_str());
	
	auto fpth = zo_path{fs::canonical(f_nam)};
	//auto dpth = zo_path{fs::canonical(".")};
	auto fl = make_sfont_pt(fpth);
	zo_orga org;
	
	fl->get_samples(org);
	for(zo_ref_pt ii : fl->all_ref){ 
		zo_path orig = ii->get_orig();
		//ZO_CK(orig.is_absolute());
		//zo_path cano = fs::canonical(orig);
		fprintf(stdout, "\t'%s'\n\t'%s'\n\t'%s'\n\t'%s'\n=======================\n", 
				ii->prefix.c_str(), ii->get_orig().c_str(), ii->get_orig_rel().c_str(), ii->suffix.c_str()); 
	}

	auto ff = std::make_shared<zo_sfont>(fpth);
	std::vector<std::shared_ptr<zo_sfont> > vv2;
	vv2.push_back(ff);
	
	return 0;
}

void
print_help(const zo_str_vec& args){
	const zo_string& prg_nm = args[0];
	fprintf(stdout, 
	R"help(%s [OPTION] ... [FILE] ...
	The purpose of %s is to move and copy sfz soundfonts while preserving consistency of any referenced sample within them.
	
	Options:
	=======
	
	-l --list
		Just print what it would do without actualy doing it.
	-m --move
		move action. Rest of parameters will decide what and how.
		If the --to directory is not under of the --from directory the action becomes a --copy.
	-c --copy
		copy action. Rest of parameters will decide what and how.
	-p --purge
		purge action. Rest of parameters will decide what and how.
		moves all non utf8 files with '.sfz' ext to --to directory.
		moves all sfz soundfonts with no valid sample references under the --from directory, to the --to directory.
		moves all samples with no sample references in sfz soundfonts under the --from directory, to --to directory.
		Works on the whole --from directory (as if --recursive  and no files were selected). 
	-a --add_sfz_ext
		add_sfz_ext action. Add the extention ".sfz" to selected files. Rest of parameters will decide what and how.
		Only does it to utf8 encoded files checking the first Kbyte and only if it does not end with the '.sfz' extension already.
	-N --normalize
		normalize action. moves all selected sfz sounfonts and samples (not directories) so that they do not have odd (non alphanumeric) characters. 
		All non alphanumeric characters are replaced with _ (underscore). 
		Conflicts are resolved before renaming by a counter, at the begining of the name, that increments until there is no conflict.
		Use --match and --substitute toghether with --move if more control is desired.
	-i --ignore <file>
		Do not select this file or anything under it if it is a directory.
	-d --hidden
		Read also hidden files.
		Any file or directory name that starts with '.' is considdered hidden and by default it is not read.
	-f --from <source_dir>
		Source directory. 
		Consistency of action is only kept up to this directory, meaning:
			All sfz soundfonts and samples (referenced files by sfz files) subject to change MUST be under this directory.
		If no --from option is given the --from directory will be the current directory.
	-t --to <dest_dir>
		Destination directory. 
		If no --to option is given the --to directory will be the same as the --from directory.
	-A --samples_too
		Copy samples too.
		In a  --copy, if the --to directory is under of the --from directory copy the samples too. By default it will only change the references in
		copied sfz soundfonts to the existing samples (to keep reference consistency).
	-K --keep
		keep the destination file when it already exists.
	-P --replace
		replace the destination file when it already exists.
	-E --regex <regex_to_match>
		match regex to select files. 
		All selected filenames (not paths) are filtered by this option. Only filenames matching this option will be selected.
		Used as the third parameter to standard function 'std::regex_search' with default flags.
	-M --match <regex_to_match>
		match regex used for substitution in selected filenames. All selected filenames (not paths) will be affected by this option.
		If no --match is given the regex will be '(.*)'
		If no --substitute is given this options is ignored.
		Used as the second parameter to standard function 'std::regex_replace' with default flags.
	-S --substitute <expression_to_substitute>
		substitute expression used to determine the result filename of action. All selected filenames (not paths) will be affected by this option.
		Used as the third parameter to standard function 'std::regex_replace' with default flags.
	-r --recursive
		Select files in all subdirectories under selected directories.
		By default all files are selected including samples. 
		Use --only_sfz and --regex to further filter selected files.
	-o --only_sfz
		Only select files with '.sfz' extension. 
	-L --follow_symlinks
		Follow symlinks when reading directories.
	-F --force_action
		Execute action solving all conflicts with different names.
	--skip_normalize
		Do not normalize names. Not recomended because it deactivates conflict solving.
	--help 
	--version 
	
	Notes:
	=====
	
	1. All files under --from and --to directories SHOULD be either:
		- a sfz soundfont. A text file (utf8) with '.sfz' extension.
		- a sample. A file without '.sfz' extension.
		- a directory.
		
		Use --purge and --add_sfz_ext options to ensure this conditions.
		Behaviour is undefined for files that cannot be classified according to these rules.
		
	2. The target for --copy or --move actions is determined in the following way:
		If a --to directory is given it will be the target. All selected files will be copied or moved to that directoy.
		If a --substitute option is given, target filenames will be determined by that option.
		If just ONE file is selected in the command line the --to will be the target.
		If no --to, no --substitute, no --recursive are given and MORE than one file is selected in the command line, the LAST one will be the target.
		
	3. A file is subject to change if it is selected or if it has a reference to a selected file. i.e. a sfz soundfont file that has a reference to a selected sample.
	
	4. All changed references in sfz soundfonts (the 'sample' opcodes) will be canonized:
		- The line will start by the opcode (the 'sample' opcode).
		- The line will have ony one opcode (the 'sample' opcode).
		- The directory separator will be '/'.
	
		Other opcodes that were in the same line will be in separate lines.
		Only the 'sample' opcodes will be canonized.
	
	5. If one or more samples are selected then ALL sfz files under --from directory are read (to find posible references to the selected samples).
	
	6. A file (or directory) is selected:
		If it is listed explicitly in the [FILE] ... portion of the command line or,
		If the option --recursive is given and it is under one of the selected directories.
		
	7. If no files are selected then ALL files in the current directory are selected.
	
	8. By default all sfz soundfont and sample names are normalized during ANY action.
	
	9. The default policy is --keep.
	
	10. If more than one action (--move, --copy, --purge, --add_sfz_ext, --normalize) is given, only the last one will be executed.
	
	Examples:
	=========
	
	1. sfz_organizer -N -r -l
	Just list what it would do to normalize all names recursively in the current directory.

	2. sfz_organizer --purge -r -l
	Just list what it would do to purge all names recursively in the current directory.
	
	3. sfz_organizer -c aa.sfz sub_dir/bb.sfz
	Copy 'aa.sfz' into 'sub_dir' with name 'bb.sfz' and reusing samples under the current directory. 'sub_dir' must already exist.
	
	4. sfz_organizer -m samples/stick* samp2/
	Move all samples that start with 'stick' in directory 'samples' to directory 'samp2'. 'samp2' must already exist.
	
	5. sfz_organizer -c gong.sfz copies/gong.sfz -A
	Copy 'gong.sfz' into 'copies' with name 'gong.sfz' and copy all samples used by 'gong.sfz' to an equivalent directory tree under 'copies'.
	
	6. sfz_organizer -c -M "SJO_" -S "LOL_" *.sfz lol/
	Copy all files ending in '.sfz' in the current directory into directory 'lol' and change any filename that has string 'SJO_' for string 'LOL_'.
		
	7. sfz_organizer --move String*.sfz ../separate
	Changes to a copy. Copy all files with form 'String*.sfz' in the current directory into directory '../separate'. It also copies all samples 
	used by them keeping an equivalent tree structure. 
		
	8. sfz_organizer -c -E "percu" -r --only_sfz -t ../per2/ -l | grep CONFLICT
	Print all conflicts that would happen if a copy of all sfz soundfonts that match regex "percu" recursively under current directory into 
	directory '../per2' is attempted.

	)help", prg_nm.c_str(), prg_nm.c_str());
	fprintf(stdout, "\n");
}

void
zo_orga::ignore_purged_dir(){
	zo_path pth = "purged";
	if(fs::exists(pth)){
		std::error_code ec;
		auto apth = zo_path{fs::canonical(pth, ec)};
		if(! ec){
			all_to_ignore.insert(apth);
		}
	}
}

bool
zo_orga::get_args(const zo_str_vec& args){
	ZO_CK(! args.empty());
	
	if(args.size() < 2){
		print_help(args);
		return false;
	}
	
	ignore_purged_dir();
	
	last_pth = "";
	gave_names = false;
	
	bool is_fst = true;
	auto it = args.begin();
	for(; it != args.end(); it++){
		auto ar = *it;
		fprintf(stdout, ">'%s'\n", ar.c_str());
		if(is_fst){
			is_fst = false;
			continue;
		}
		
		if((ar == "-l") || (ar == "--list")){
			just_list = true;
		}
		else if((ar == "-m") || (ar == "--move")){
			oper = zo_action::move;
		}
		else if((ar == "-c") || (ar == "--copy")){
			oper = zo_action::copy;
		}
		else if((ar == "-p") || (ar == "--purge")){
			oper = zo_action::purge;
		}
		else if((ar == "-a") || (ar == "--add_sfz_ext")){
			oper = zo_action::add_sfz;
		}
		else if((ar == "-N") || (ar == "--normalize")){
			oper = zo_action::normalize;
		}
		else if((ar == "-i") || (ar == "--ignore")){
			it++; if(it == args.end()){ break; }
			zo_path pth = *it;
			if(fs::exists(pth)){
				std::error_code ec;
				auto apth = zo_path{fs::canonical(pth, ec)};
				if(! ec){
					all_to_ignore.insert(apth);
				}
			}
		}
		else if((ar == "-f") || (ar == "--from")){
			it++; if(it == args.end()){ break; }
			dir_from = *it;
		}
		else if((ar == "-t") || (ar == "--to")){
			it++; if(it == args.end()){ break; }
			dir_to = *it;
		}
		else if((ar == "-d") || (ar == "--hidden")){
			hidden_too = true;
		}
		else if((ar == "-A") || (ar == "--samples_too")){
			samples_too = true;
		}
		else if((ar == "-K") || (ar == "--keep")){
			pol = zo_policy::keep;
		}
		else if((ar == "-P") || (ar == "--replace")){
			pol = zo_policy::replace;
		}
		else if((ar == "-E") || (ar == "--regex")){
			it++; if(it == args.end()){ break; }
			regex_str = *it;
		}
		else if((ar == "-M") || (ar == "--match")){
			it++; if(it == args.end()){ break; }
			match_str = *it;
		}
		else if((ar == "-S") || (ar == "--substitute")){
			it++; if(it == args.end()){ break; }
			subst_str = *it;
		}
		else if((ar == "-r") || (ar == "--recursive")){
			recursive = true;
		}
		else if((ar == "-o") || (ar == "--only_sfz")){
			only_sfz = true;
		}
		else if((ar == "-L") || (ar == "--follow_symlinks")){
			follw_symlk = true;
		}
		else if((ar == "-F") || (ar == "--force_action")){
			force_action = true;
		}
		else if(ar == "--skip_normalize"){
			skip_normalize = true;
		}
		else if(ar == "--help"){
			print_help(args);
			return false;
		}
		else if(ar == "--version"){
			print_version(args);
			return false;
		}
		else{
			fprintf(stdout, ":'%s'\n", ar.c_str());
			last_pth = ar;
			if(fs::exists(last_pth)){
				f_names.push_back(last_pth);
			}
			gave_names = true;
		}
	}
	
	if(oper == zo_action::purge){
		f_names.clear();
		recursive = true;
	}
	if(oper == zo_action::nothing){
		just_list = true;
	}
	
	bool had_dir_to = ! dir_to.empty();
	if(dir_from.empty()){
		dir_from = ".";
	}
	if(dir_to.empty()){
		dir_to = dir_from;
	}
	
	if(oper == zo_action::copy){
		bool ok = calc_target(had_dir_to);
		if(! ok){
			fprintf(stdout, "Invalid target\n");
			return false;
		}
	}
	if(oper == zo_action::move){
		bool ok = calc_target(had_dir_to);
		if(! ok){
			fprintf(stdout, "Invalid target\n");
			return false;
		}
	}
	
	if(! fs::exists(dir_from)){
		fprintf(stderr, "directory %s given in option --from does not exist\n", dir_from.c_str());
		return false;
	}
	if(! fs::exists(dir_to)){
		fprintf(stderr, "directory %s given in option --to does not exist\n", dir_to.c_str());
		return false;
	}
	dir_from = fs::canonical(dir_from);
	dir_to = fs::canonical(dir_to);
	
	fprintf(stdout, "Using dir from '%s'\n", dir_from.c_str());
	fprintf(stdout, "Using dir to '%s'\n", dir_to.c_str());
	
	base_pth = dir_from;
	tmp_pth = base_pth / tmp_nam;
	fs::create_directories(tmp_pth.parent_path());
	fprintf(stdout, "Using temp file path '%s'\n", tmp_pth.c_str());
	fprintf(stdout, "Using target name '%s'\n", target.c_str());
	
	if(! regex_str.empty()){
		select_rx = regex_str;
		fprintf(stdout, "Using regex_str '%s'\n", regex_str.c_str());
	}
	
	if(! subst_str.empty()){
		match_rx = match_str;
		fprintf(stdout, "Using match_str '%s'\n", match_str.c_str());
		fprintf(stdout, "Using subst_str '%s'\n", subst_str.c_str());
	}
	
	zo_string dto = dir_to;
	bool is_under = (dto.rfind(dir_from, 0) == 0);
	if(! is_under && (oper == zo_action::move)){
		fprintf(stdout, "Dir 'to' is not under dir 'from' CHANGING action to copy\n");
		oper = zo_action::copy;
	}
	if(oper == zo_action::copy){
		samples_too = samples_too || ! is_under;
	}

	fprintf(stdout, "Using target %s\n", target.c_str());
	if(samples_too){
		fprintf(stdout, "Copying samples_too\n");
	}
	if(gave_names){
		fprintf(stdout, "gave_names\n");
	}
	
	zo_string ac_str = get_action_str(oper);
	if(just_list){
		fprintf(stdout, "simulating action: %s\n", ac_str.c_str());
	} else {
		fprintf(stdout, "executing action: %s\n", ac_str.c_str());
	}
	
	return true;
}

void
zo_fname::print_actions(zo_orga& org, bool only_orig){
	bool purging = (org.oper == zo_action::purge);
	if(nxt_pth.empty()){
		if(! purging){
			fprintf(stdout, "SKIP '%s'\n", orig_pth.c_str());
		}
		return;
	}
	if(only_orig){
		fprintf(stdout, "'%s'\n", orig_pth.c_str());
		return;
	}
	if((orig_pth != nxt_pth) && fs::exists(nxt_pth) && (org.pol == zo_policy::keep)){
		fprintf(stdout, "KEEPING_EXISTING_FILE '%s'\n", nxt_pth.c_str());
		return;
	}
	
	bool is_mv = org.is_move();
	zo_string act = (is_mv)?("MOVE_FILE"):("COPY_FILE");
	zo_string cfl = (is_confl)?(" THAT_WAS_IN_CONFLICT"):("");
	fprintf(stdout, "%s%s '%s' to '%s'\n", act.c_str(), cfl.c_str(), orig_pth.c_str(), nxt_pth.c_str());
	
}

void
zo_ref::print_actions(zo_orga& org){
	ZO_CK(org.oper != zo_action::purge);
	fprintf(stdout, "----------\n");
	if(! bad_pth.empty()){
		fprintf(stdout, "BAD_REFERENCE_SKIPPED '%s'\n", bad_pth.c_str());
		return;
	}
	fprintf(stdout, "original_sample: %s\n", get_orig().c_str());
	fprintf(stdout, "original_rel_sample: %s\n", get_orig_rel().c_str());
	if(is_same()){
		fprintf(stdout, "KEEPING_ORIG_LINE %ld. zo_ref::is_same()\n", num_line);
		return;
	}
	fprintf(stdout, "replace_line %ld with:\n", num_line);
	if(! prefix.empty()){
		fprintf(stdout, "%s\n", prefix.c_str());
	}
	fprintf(stdout, "sample=%s\n", get_next_rel().c_str());
	if(! suffix.empty()){
		fprintf(stdout, "%s\n", suffix.c_str());
	}
}

void
print_separator_line(const char* sep){
	for(int aa = 0; aa < 100; aa++){ 
		fprintf(stdout, "%s", sep); 
	} 
	fprintf(stdout, "\n");  
}

void
zo_sfont::print_actions(zo_orga& org){
	if(is_same()){
		fprintf(stdout, "UNCHANGED SOUNDFONT '%s' (skipping)\n", get_orig().c_str());
		return;
	}
	bool purging = (org.oper == zo_action::purge);
	if(! purging || ! fpth.nxt_pth.empty()){
		print_separator_line("=");
	}
	fpth.print_actions(org, false);
	
	if(purging){
		if(! is_txt){
			fprintf(stdout, "NON_UTF8\n");
		} else
		if(tot_spl_ref == 0){
			fprintf(stdout, "NO_VALID_SAMPLES_FOUND\n");
		}
		return;
	}
	for(auto rf: all_ref){
		rf->print_actions(org);
	}
}

void
zo_sample::print_actions(zo_orga& org){
	if(is_same()){
		fprintf(stdout, "UNCHANGED SAMPLE '%s' (skipping)\n", get_orig().c_str());
		return;
	}
	bool purging = (org.oper == zo_action::purge);
	if(! purging || ! fpth.nxt_pth.empty()){
		print_separator_line("+");
	}
	fpth.print_actions(org, false);
	
	if(purging){
		if(all_bk_ref.empty()){
			fprintf(stdout, "NO_REFERENCES_IN_SFZ_SOUNDFONTS\n");
		}
		return;
	}
	for(const auto& sfe : all_bk_ref){
		zo_sfont_pt sf = sfe.second;
		fprintf(stdout, "FOUND IN----------\n");
		sf->fpth.print_actions(org, true);
	}
	
}

void
zo_dir::print_actions(zo_orga& org){
	if(! all_selected_sfz.empty()){
		print_separator_line("%");
		fprintf(stdout, "ALL_SELECTED SOUNDFONTS\n");  
		print_separator_line("%");
		for(const auto& sfe : all_selected_sfz){
			zo_sfont_pt sf = sfe.second;
			sf->print_actions(org);
		}
	}
	if(! all_selected_spl.empty()){
		print_separator_line("#");
		fprintf(stdout, "ALL_SELECTED SAMPLES\n");  
		print_separator_line("#");
		for(const auto& sme : all_selected_spl){
			zo_sample_pt sm = sme.second;
			sm->print_actions(org);
		}
	}
		
	ZO_CK(bad_spl != zo_null);
	if(! bad_spl->all_bk_ref.empty()){
		print_separator_line("!");
		fprintf(stdout, "ALL_WITH_BAD_REFERENCES\n");  
		print_separator_line("!");
		for(const auto& sfe : bad_spl->all_bk_ref){
			zo_sfont_pt sf = sfe.second;
			fprintf(stdout, "FOUND_BAD_REFERENCES_IN:\n");  
			sf->fpth.print_actions(org, true);
		}
	}
}

void
zo_sfont::prepare_normalize(zo_orga& org){
	ZO_CK(fpth.nxt_pth.empty());
	fpth.calc_next(org, cmd_sel);
}

void
zo_sample::prepare_normalize(zo_orga& org){
	ZO_CK(fpth.nxt_pth.empty());
	fpth.calc_next(org, cmd_sel);
}

void
zo_orga::prepare_normalize(){
	zo_orga& org = *this; 
	for(const auto& sfe : all_selected_sfz){
		zo_sfont_pt sf = sfe.second;
		sf->prepare_normalize(org);
	}
	for(const auto& sme : all_selected_spl){
		zo_sample_pt sm = sme.second;
		sm->prepare_normalize(org);
	}
}

int
test_fix(int argc, char* argv[]){
	zo_str_vec args{argv, argv + argc};
	
	if(args.size() < 2){
		fprintf(stdout, "%s <str>\n", args[0].c_str());
		return -1;
	}

	fprintf(stdout, "%s\n", args[1].c_str());
	
	zo_orga dd;
	if(args.size() > 2){
		long num_fix = std::stol(args[2]);
		for(int aa = 0; aa < num_fix; aa++){
			zo_fname fpth;
			fpth.orig_pth = args[1];
			ZO_CK(fpth.nxt_pth.empty());
			fpth.calc_next(dd, false);
			fprintf(stdout, "nxt=%s\n", fpth.nxt_pth.c_str());
		}
	}
	
	fprintf(stdout, "-------------\n");
	zo_fname fpth2;
	ZO_CK(fpth2.nxt_pth.empty());
	fpth2.orig_pth = args[1];
	fpth2.calc_next(dd, false);
	fprintf(stdout, "fixed=%s\n", fpth2.nxt_pth.c_str());

	return 0;
}

int main(int argc, char* argv[]){
	return sfz_organizer_main(argc, argv);
	//return test_fix(argc, argv);
	//return test_fs(argc, argv);
	//return test_rx(argc, argv);
	// create_directories
}

void 
zo_sfont::do_actions(zo_orga& org){
	ZO_CK(! did_it);
	if(did_it){ return; }
	did_it = true;
	
	if(is_same()){
		fprintf(stdout, "UNCHANGED SOUNDFONT '%s' (skipping)\n", get_orig().c_str());
		return;
	}
	zo_path nxt = get_next();
	if(nxt.empty()){
		fprintf(stdout, "SKIPPING SOUNDFONT '%s'\n", get_orig().c_str());
		return;
	}
	fs::create_directories(nxt.parent_path());
	
	if((get_orig() != nxt) && fs::exists(nxt) && (org.pol == zo_policy::keep)){
		fprintf(stdout, "KEEPING_EXISTING_FILE '%s'\n", nxt.c_str());
		return;
	}
	
	bool is_mv = org.is_move();
	zo_path tmp = org.get_temp_path();
	prepare_tmp_file(tmp);
	if(is_mv){
		fs::remove(get_orig());
	}
	fs::rename(tmp, nxt);
}

void 
zo_sample::do_actions(zo_orga& org){
	ZO_CK(! did_it);
	if(did_it){ return; }
	did_it = true;
	
	if(is_same()){
		fprintf(stdout, "UNCHANGED SAMPLE '%s' (skipping)\n", get_orig().c_str());
		return;
	}
	zo_path nxt = get_next();
	if(nxt.empty()){
		fprintf(stdout, "SKIPPING SAMPLE '%s'\n", get_orig().c_str());
		return;
	}
	fs::create_directories(nxt.parent_path());
	
	if((get_orig() != nxt) && fs::exists(nxt) && (org.pol == zo_policy::keep)){
		fprintf(stdout, "KEEPING_EXISTING_FILE '%s'\n", nxt.c_str());
		return;
	}
	
	bool is_mv = org.is_move();
	if(is_mv){
		fs::rename(get_orig(), nxt);
		return;
	}
	fs::copy(get_orig(), nxt);
}

void 
zo_dir::do_actions(zo_orga& org){
	for(const auto& sfe : all_selected_sfz){
		zo_sfont_pt sf = sfe.second;
		sf->do_actions(org);
	}
	for(const auto& sme : all_selected_spl){
		zo_sample_pt sm = sme.second;
		sm->do_actions(org);
	}
}

void
zo_sfont::prepare_tmp_file(const zo_path& tmp_pth){
	if(all_ref.empty()){
		fprintf(stdout, "JUST_COPY_FILE. all_ref.empty(). %s\n", get_orig().c_str()); // dbg_prt
		fs::copy(get_orig(), tmp_pth);
		return;
	}
	
	std::ifstream src;
	src.open(get_orig().c_str(), std::ios::binary);
	if(! src.good() || ! src.is_open()){
		throw sfz_exception(sfz_cannot_open, get_orig());
	}
	std::ofstream dst;
	zo_string tmp = tmp_pth;
	dst.open(tmp.c_str(), std::ios::binary);
	if(! dst.good() || ! dst.is_open()){
		throw sfz_exception(sfz_cannot_open, tmp);
	}
	
	auto it = all_ref.begin();
	
	std::smatch sample_matches;
	long lnum = 0;
	zo_string ln;
	for(;getline(src, ln);){
		if(it == all_ref.end()){
			dst << ln << '\n';
			continue;
		}
		lnum++;
		std::size_t pos_spl = ln.find(ZO_SAMPLE_STR);
		bool has_spl = (pos_spl != std::string::npos);
		if(! has_spl){
			dst << ln << '\n';
			continue;
		}
		if(! regex_search(ln, sample_matches, ZO_SAMPLE_LINE_PATTERN)){
			dst << ln << '\n';
			continue;
		}
		zo_ref_pt rf = *it;
		ZO_CK(rf != zo_null);
		if(rf->num_line > lnum){
			dst << ln << '\n';
			continue;
		}
		if(rf->num_line < lnum){
			throw sfz_exception(sfz_read_1_and_2_differ, get_orig());
		}
			
		it++;
		rf->write_ref(dst, ln);
	}
	
	//return 0;
}

void 
zo_ref::write_ref(std::ofstream& dst, const zo_string& ln){
	if(! bad_pth.empty()){
		//fprintf(stdout, "KEEP_LINE.! bad_pth.empty()_during %s\n", ln.c_str()); // dbg_prt
		dst << ln << '\n';
		return;
	}
	if(is_same()){
		fprintf(stdout, "KEEP_LINE. zo_ref::is_same()_during %s\n", ln.c_str()); // dbg_prt
		dst << ln << '\n';
		return;
	} 
	if(! prefix.empty()){
		//fprintf(stdout, "WRITING_PREFIX. %s\n", prefix.c_str()); // dbg_prt
		dst << prefix << '\n';
	}
	zo_string nx_rel = get_next_rel();
	//fprintf(stdout, "WRITING_SAMPLE. %s\n", nx_rel.c_str()); // dbg_prt
	dst << "sample=" << nx_rel << '\n';
	if(! suffix.empty()){
		//fprintf(stdout, "WRITING_SUFIX. %s\n", suffix.c_str()); // dbg_prt
		dst << suffix << '\n';
	}
}

void
zo_sfont::prepare_add_sfz_ext(zo_orga& org){
	ZO_CK(fpth.nxt_pth.empty());
	fpth.calc_next(org, cmd_sel);
}

void
zo_orga::prepare_add_sfz_ext(){
	zo_orga& org = *this;
	for(const auto& sfe : all_selected_sfz){
		zo_sfont_pt sf = sfe.second;
		sf->prepare_add_sfz_ext(org);
	}
}

const zo_path
get_from_rel_path(zo_path& pth, zo_path& dir_from){
	zo_path dir_pth = pth.parent_path();
	
	std::error_code ec;
	zo_path rel_dir = find_relative(dir_pth, dir_from, ec);
	zo_path rel_pth = rel_dir / pth.filename();
	ZO_CK(! ec);
	return rel_pth;
}

void
zo_sfont::prepare_purge(zo_orga& org){
	ZO_CK(fpth.nxt_pth.empty());
	if(! is_txt || (tot_spl_ref == 0)){
		fpth.calc_next(org, cmd_sel);
		return;
	}
}

void
zo_sample::prepare_purge(zo_orga& org){
	ZO_CK(fpth.nxt_pth.empty());
	if(all_bk_ref.empty()){
		fpth.calc_next(org, cmd_sel);
		return;
	}
}

void
zo_orga::prepare_purge(){
	zo_orga& org = *this;
	for(const auto& sfe : all_selected_sfz){
		zo_sfont_pt sf = sfe.second;
		sf->prepare_purge(org);
	}
	for(const auto& sme : all_selected_spl){
		zo_sample_pt sm = sme.second;
		sm->prepare_purge(org);
	}
}

bool
zo_orga::calc_target(bool had_dir_to){
	bool lst_is_dir = fs::is_directory(last_pth);
	bool lst_exists = fs::exists(last_pth);
	target = "";
	if(had_dir_to){
		return true;
	}
	bool lst_is_good_dir = (lst_exists && lst_is_dir && ! recursive);
	if(! subst_str.empty() && ! lst_is_good_dir){
		return true;
	}
	if(f_names.empty()){
		return true;
	}
	ZO_CK(! f_names.empty());

	if(last_pth.empty()){
		return true;
	}
	
	if(! lst_exists){
		ZO_CK(last_pth != f_names.back());
		ZO_CK(! lst_is_dir);

		zo_path pnt = last_pth.parent_path();
		if(pnt.empty()){
			pnt = ".";
		}
		if(! fs::is_directory(pnt)){
			fprintf(stdout, "The target's directory '%s' does NOT exist\n", pnt.c_str());
			return false;
		}
		
		ZO_CK(! had_dir_to);
		dir_to = pnt;
		
		zo_path nm = last_pth.filename();
		target = nm;
		return true;
	}
	
	ZO_CK(last_pth == f_names.back());
	
	last_pth = fs::canonical(last_pth);
	f_names.pop_back();
	
	if(! lst_is_dir){
		fprintf(stdout, "Target '%s' exists and is NOT directory\n", last_pth.c_str());
		return false;
	}
	
	ZO_CK(! had_dir_to);
	dir_to = last_pth;
	
	return true;
}

void
zo_sfont::prepare_copy_or_move(zo_orga& org){
	ZO_CK(fpth.nxt_pth.empty());
	fpth.calc_next(org, cmd_sel, can_move);
}

void
zo_sample::prepare_copy_or_move(zo_orga& org){
	ZO_CK(fpth.nxt_pth.empty());
	fpth.calc_next(org, cmd_sel);
}

void
zo_orga::prepare_copy_or_move(){
	zo_orga& org = *this;
	for(const auto& sfe : all_selected_sfz){
		zo_sfont_pt sf = sfe.second;
		sf->prepare_copy_or_move(org);
	}
	for(const auto& sme : all_selected_spl){
		zo_sample_pt sm = sme.second;
		sm->prepare_copy_or_move(org);
	}
}

void
zo_orga::organizer_main(const zo_str_vec& args){
	if(! get_args(args)){
		fprintf(stderr, "Doing_nothing\n");
		return;
	}
	fprintf(stderr, "Starting\n");
	
	zo_orga& org = *this;
	read_selected();
	
	if(oper == zo_action::normalize){
		prepare_normalize();
	}
	if(oper == zo_action::add_sfz){
		prepare_add_sfz_ext();
	}
	if(oper == zo_action::purge){
		prepare_purge();
	}
	if(oper == zo_action::copy){
		prepare_copy_or_move();
	}
	if(oper == zo_action::move){
		prepare_copy_or_move();
	}
	
	if((tot_conflict > 0) && ! force_action){
		just_list = true;
	}
	
	if(just_list){
		fprintf(stderr, "Just_printing_actions\n");
		print_actions(org);
		if(tot_conflict > 0){
			fprintf(stderr, "Found %ld conflicts. Use --force_action to execute\n", tot_conflict);
		}
		fprintf(stderr, "Doing_nothing.\n");
		return;
	}
	
	ZO_CK(oper != zo_action::nothing);
	do_actions(org);
}

void
zo_fname::calc_next(zo_orga& org, bool cmd_sel, bool can_mv){
	ZO_CK(nxt_pth.empty());
	std::error_code ec;
	zo_path pth = orig_pth;
	zo_path dir_pth = pth.parent_path();
	zo_path rel_dir = find_relative(dir_pth, org.dir_from, ec);
	ZO_CK(! ec);
	
	zo_string nm = pth.filename();
	
	zo_path dr_to = org.dir_to;
	if((org.oper == zo_action::copy) || (org.oper == zo_action::move)){
		if(cmd_sel){
			rel_dir = "";
		}
		if(! can_mv){
			dr_to = org.dir_from;
		}
		if(! org.target.empty() && can_mv && cmd_sel){
			nm = org.target;
		}
	}
	if(! org.skip_normalize){
		normalize_name(nm);
	}
	
	if(org.oper == zo_action::add_sfz){
		ZO_CK(! has_sfz_ext(nm));
		nm = nm + ".sfz";
	}
	if(org.oper == zo_action::purge){
		dr_to = dr_to / "purged";
	}
	
	if(! org.subst_str.empty() && can_mv){
		nm = std::regex_replace(nm, org.match_rx, org.subst_str);
	}
	
	zo_path nx_pth = dr_to / rel_dir / nm;
	
	zo_last_confl_pt the_cfl = zo_null;
	auto it = org.all_unique_nxt.find(nx_pth);
	if(it != org.all_unique_nxt.end()){
		org.tot_conflict++;
		is_confl = true;
		if(org.skip_normalize){
			return;
		}
		
		the_cfl = it->second;
		ZO_CK(the_cfl != zo_null);
		the_cfl->val++;
		set_num_name(nm, the_cfl->val);
		
		nx_pth = dr_to / rel_dir / nm;
		
		it = org.all_unique_nxt.find(nx_pth);
	}
	ZO_CK(it == org.all_unique_nxt.end());
	
	if(the_cfl == zo_null){
		the_cfl = make_last_confl_pt();
		ZO_CK(the_cfl->val == 0);
	}
	org.all_unique_nxt[nx_pth] = the_cfl;
	nxt_pth = nx_pth;
	//fprintf(stdout, "calc_next. %s->%s\n", orig_pth.c_str(), nxt_pth.c_str()); // dbg_prt
}

bool
zo_sfont::is_same(){
	bool same_nm = fpth.is_same();
	bool all_sm = true;
	for(zo_ref_pt ii : all_ref){ 
		ZO_CK(ii != zo_null);
		all_sm = all_sm && ii->is_same();
	}
	return (all_sm && same_nm);
}

zo_fname& 
zo_ref::sf_name(){
	ZO_CK(owner != zo_null);
	return owner->fpth;
}

bool
zo_ref::is_same(){
	bool sm = prefix.empty() && suffix.empty() && bad_pth.empty();
	ZO_CK(sref != zo_null);
	return (sm && sref->is_same() && sf_name().is_same());
}
