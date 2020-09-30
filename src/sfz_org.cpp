

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


zo_string ZO_INC_PATTERN_STR = "^f([[:digit:]]*)_";
std::regex ZO_INC_PATTERN{ZO_INC_PATTERN_STR.c_str()};

int sfz_organizer_main(int argc, char* argv[]){
	zo_str_vec args{argv, argv + argc};
	zo_orga org;
	org.organizer_main(args);
	return 0;
}

void
fix_name(zo_string& nm){
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
inc_name(zo_string& nm){
	std::smatch sample_matches;
	if(regex_search(nm, sample_matches, ZO_INC_PATTERN)){
		ZO_CK(sample_matches.size() > 1);
		zo_string m0 = sample_matches[0];
		long val = std::stol(sample_matches[1]);
		nm = "f" + std::to_string(val + 1) + "_" + nm.substr(m0.size(), nm.size() - m0.size());
		return;
	} 
	nm = "f1_" + nm;
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
relative(const zo_path& pth, const zo_path& base, std::error_code& ec){
	if(! pth.is_absolute()){
		return "";
	}
	if(! fs::exists(pth)){
		return "";
	}
	if(! fs::is_directory(pth)){
		return "";
	}

	if(! base.is_absolute()){
		return "";
	}
	if(! fs::exists(base)){
		return "";
	}
	if(! fs::is_directory(base)){
		return "";
	}

	if(pth.root_path() != base.root_path()){
		return pth;
	}
	
	//fprintf(stdout, "base:%s\n", base.c_str());

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

	return result;
}

const zo_string&
zo_ref::get_orig(){
	ZO_CK(fref != zo_null);
	return fref->get_orig();
}

const zo_string& 
zo_ref::get_next(){
	ZO_CK(fref != zo_null);
	return fref->get_next();
}

const zo_string
zo_ref::get_orig_rel(){
	ZO_CK(owner != zo_null);
	ZO_CK(fref != zo_null);
	zo_path sf_pth = owner->get_orig();
	zo_path rf_pth = get_orig();
	
	zo_path sf_pnt = sf_pth.parent_path();
	zo_path rf_pnt = rf_pth.parent_path();
	
	std::error_code ec;
	zo_path rel_pth = find_relative(rf_pnt, sf_pnt, ec) / rf_pth.filename();
	ZO_CK(! ec);
	return rel_pth;
}

const zo_string
zo_ref::get_next_rel(){
	ZO_CK(owner != zo_null);
	ZO_CK(fref != zo_null);
	zo_path sf_pth = owner->get_next();
	zo_path rf_pth = get_next();
	
	zo_path sf_pnt = sf_pth.parent_path();
	zo_path rf_pnt = rf_pth.parent_path();
	
	std::error_code ec;
	zo_path rel_pth = find_relative(rf_pnt, sf_pnt, ec) / rf_pth.filename();
	ZO_CK(! ec);
	return rel_pth;
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

int
sfz_get_samples(zo_sfont_pt fl, zo_dir& dir){
	std::ifstream istm;
	istm.open(fl->get_orig().c_str(), std::ios::binary);
	if(! istm.good() || ! istm.is_open()){
		throw sfz_exception(sfz_cannot_open, fl->get_orig());
	}
	
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
		//fprintf(stdout, "> %ld:%s\n", lnum, ln.c_str());
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
				//fprintf(stdout, "pfx:%s\n", nxt_pfx.c_str());
				zo_string m0 = opcode_matches[0];
				lsuffix = m0 + opcode_matches.suffix().str();
				break;
			}
			for(long aa = 0; aa < (long)opcode_matches.size(); aa++){
				zo_string m0 = opcode_matches[aa];
				//fprintf(stdout, "+%ld:%s\n", aa, m0.c_str());
				if(m0 == ZO_SAMPLE_STR){
					found_sample_opcode = true;
					lprefix = opcode_matches.prefix().str();
				}
			}
			opcod = opcode_matches.suffix().str();
			//fprintf(stdout, "sfx:%s\n", opcod.c_str());
		}

		lref = (! nxt_pfx.empty())?(nxt_pfx):(opcod);
		trim(lref);
		//fprintf(stdout, "sample_f_nam:'%s'\n", lref.c_str());
		
		auto ec = std::error_code{};
		zo_path fx_ref = fix_ref_path(lref, fl->get_orig(), ec);
		
		if(ec){
			dir.bad_spl->all_bk_ref.push_back(fl);
			continue;
		}
		
		//fprintf(stdout, "rpth:'%s'\n", rpth.c_str());
		//fprintf(stdout, "pnt:'%s'\n", pnt.c_str());
		
		bool is_nw = false;
		zo_sample_pt spl = dir.get_read_sample(fx_ref, is_nw);
		ZO_CK(spl != zo_null);		
		ZO_CK(spl->get_orig() == fx_ref);
		
		spl->all_bk_ref.push_back(fl);
		
		zo_sample_pt sspl = dir.get_selected_sample(fx_ref, spl, is_nw, false);
		if(sspl == zo_null){
			continue;
		}
		ZO_CK(sspl == spl);

		auto nw_ref = make_ref_pt(fl, lnum, spl);
		nw_ref->prefix = lprefix;
		nw_ref->suffix = lsuffix;
		fl->all_ref.push_back(nw_ref);
		
		if(ec){
			nw_ref->bad_pth = lref;
			fprintf(stderr, "bad_ref:'%s' in file %s\n", lref.c_str(), fl->get_orig().c_str());
		}
	}
	
	return 0;
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
read_file(const zo_path& pth, zo_dir& dir, const zo_ftype ft, const bool only_with_ref){
	ZO_CK(! only_with_ref || (ft == zo_ftype::soundfont));
	if(! fs::exists(pth)){
		return;
	}
	auto apth = zo_path{fs::canonical(pth)};
	bool is_nw = false;
	bool is_sfz = has_sfz_ext(pth);
	if((ft == zo_ftype::soundfont) && is_sfz){
		zo_sfont_pt sf = dir.get_read_soundfont(apth, is_nw);
		if(is_nw){
			sfz_get_samples(sf, dir);
		}
		bool has_ref = ! sf->all_ref.empty();
		if(! only_with_ref || has_ref){
			dir.get_selected_soundfont(apth, sf, is_nw);
		}
	}
	if((ft == zo_ftype::sample) && ! is_sfz){
		zo_sample_pt sp = dir.get_read_sample(apth, is_nw);
		ZO_CK(sp != zo_null);
		sp->selected = true;
		dir.tot_selected_spl++;
		dir.get_selected_sample(apth, sp, is_nw, true);
	}
}


void 
read_dir_files(zo_path pth_dir, zo_dir& dir, const zo_ftype ft, const bool only_with_ref, const bool follw_symlk){
	if(fs::is_symlink(pth_dir)){
		pth_dir = fs::read_symlink(pth_dir);
	}
	if(fs::exists(pth_dir) && fs::is_directory(pth_dir)){
		for (const auto& entry : fs::directory_iterator(pth_dir)){
			zo_path pth = entry.path();
			auto st = entry.status();
			if(fs::is_directory(st)){
				if(follw_symlk || ! fs::is_symlink(pth)){
					read_dir_files(entry, dir, ft, only_with_ref, follw_symlk);
				}
			} else if(fs::is_regular_file(st)){
				read_file(pth, dir, ft, only_with_ref);
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
read_files(const zo_str_vec& all_pth, const zo_ftype ft, zo_dir& dir, bool is_recu, bool follw_symlk){
	for(auto nm : all_pth){
		zo_path f_pth = nm;
		if(! fs::exists(f_pth)){
			continue;
		}
		if(fs::is_directory(f_pth)){
			if(! is_recu){
				continue;
			}
			read_dir_files(f_pth, dir, ft, false, follw_symlk);
		} else if(fs::is_regular_file(f_pth)){
			read_file(f_pth, dir, ft, false);
		} 
	}
}

void 
zo_orga::read_selected(zo_dir& dir, bool only_sfz, bool follw_symlk){
	if(f_names.empty()){
		fill_files(dir_from, f_names);
	}
	if(! only_sfz){
		read_files(f_names, zo_ftype::sample, dir, recursive, follw_symlk);
	}
	read_files(f_names, zo_ftype::soundfont, dir, recursive, follw_symlk);
	//fprintf(stdout, "basic reading Ok\n");
	fprintf(stdout, "tot_selected_samples = %ld\n", dir.tot_selected_spl);
	if(dir.tot_selected_spl > 0){
		ZO_CK(! dir.base_pth.empty());
		read_dir_files(dir.base_pth, dir, zo_ftype::soundfont, true, follw_symlk);
	}
	//fprintf(stdout, "recu reading Ok\n");
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
	
	zo_dir dir;
	read_dir_files(pth1, dir, zo_ftype::soundfont, false, false);
	
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
	zo_dir dir;
	
	sfz_get_samples(fl, dir);
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
	-l --list
		Just print what it would do without actualy doing it.
	-n --move
		move action. Rest of parameters will decide what and how.
	-c --copy
		copy action. Rest of parameters will decide what and how.
	-p --purge
		purge action. Rest of parameters will decide what and how.
		moves all non utf8 files with '.sfz' ext to --to directory.
		moves all files with '.sfz' extension and no sample references to --to directory.
		moves all non-referenced samples (files without '.sfz' extension) to --to directory.
	-a --add_sfz_ext
		add_sfz_ext action. Add the extention ".sfz" to selected files. Rest of parameters will decide what and how.
		Only does it to utf8 encoded files checking the first Kbyte and only if it does not end with the '.sfz' extension already.
	-x --fix
		fix action. moves all selected sfz sounfonts and samples (not directories) so that they do not have odd (non alphanumeric) characters. 
		All non alphanumeric characters are replaced with _ (underscore). Conflicts are resolved before renaming by a counter, at the begining of the name, that increments until there is no conflict.
		Use --match and --substitute toghether with --move if more control is desired.
	-f --from <source_dir>
		Source directory. All sfz soundfonts and samples (referenced files by sfz files) subject of action MUST be under this directory.
		If no --to option is given the --to directory will be the same as this.
	-t --to <dest_dir>
		Destination directory. If no --from option is given the source directory will be the same as this.
		If the --to directory is not under of the --from directory a --move of sfz soundfonts will only copy any referenced sample.
		If the --to directory is under of the --from directory a --copy of sfz soundfonts will not copy any referenced sample, only preserve consistency, unless the --samples_too option is given.
	-S --samples_too
		Copy samples too.
	-k --keep
		keep the destination file when it already exists.
	-e --replace
		replace the destination file when it already exists.
	-m --match <regex_to_match>
		match regex to use in action. All selected files with names (not paths) matching will be used as subject of action.
	-s --substitute <expression_to_substitute>
		substitute expression used to determine the file name result of action.
	-r --recursive
		do action for all subdirectories under selected directories.
	-o --only_sfz
		Only select files with '.sfz' extension.
	-w --follow_symlinks
		Follow symlinks when reading directories.
	--help 
	--version 
	
	Notes:
	1. All files under --from and --to directories SHOULD be either:
		- a sfz soundfont. A text file (utf8) with '.sfz' extension.
		- a sample. A file without '.sfz' extension.
		- a directory.
		
		Use --purge and --add_sfz_ext options to ensure this conditions.
		Behaviour is undefined for files that cannot be classified according to these rules.
	2. If no --from neither --to directories are given the current directory is taken as both.
	3. A file is subject of action if it is selected or if it has a reference to a selected file. i.e. a sfz soundfont file that has a reference to a selected sample.
	4. If one or more samples are selected then all sfz files under --from directory are read (to find posible references to the selected samples).
	5. A file (or directory) is selected if it is listed explicitly in the [FILE] ... portion of the command line or if the option --recursive is given and it is under one of the selected directories.
	6. If no files are selected then ALL files in the current directory are selected.
	7. The default policy is --keep.
		
	)help", prg_nm.c_str(), prg_nm.c_str());
}

bool
zo_orga::get_args(const zo_str_vec& args){
	ZO_CK(! args.empty());
	
	if(args.size() < 2){
		print_help(args);
		return false;
	}
	
	auto it = args.begin();
	for(; it != args.end(); it++){
		auto ar = *it;
		fprintf(stdout, ">'%s'\n", ar.c_str());
		if((ar == "-l") || (ar == "--list")){
			just_list = true;
		}
		else if((ar == "-n") || (ar == "--move")){
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
		else if((ar == "-x") || (ar == "--fix")){
			oper = zo_action::fix;
		}
		else if((ar == "-f") || (ar == "--from")){
			it++; if(it == args.end()){ break; }
			dir_from = *it;
		}
		else if((ar == "-t") || (ar == "--to")){
			it++; if(it == args.end()){ break; }
			dir_to = *it;
		}
		else if((ar == "-S") || (ar == "--samples_too")){
			samples_too = true;
		}
		else if((ar == "-k") || (ar == "--keep")){
			pol = zo_policy::keep;
		}
		else if((ar == "-e") || (ar == "--replace")){
			pol = zo_policy::replace;
		}
		else if((ar == "-m") || (ar == "--match")){
			it++; if(it == args.end()){ break; }
			match_str = *it;
		}
		else if((ar == "-s") || (ar == "--substitute")){
			it++; if(it == args.end()){ break; }
			subst_str = *it;
		}
		else if((ar == "-r") || (ar == "--recursive")){
			recursive = true;
		}
		else if((ar == "-o") || (ar == "--only_sfz")){
			only_sfz = true;
		}
		else if((ar == "-w") || (ar == "--follow_symlinks")){
			follw_symlk = true;
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
			zo_path pth = ar;
			if(fs::exists(pth)){
				f_names.push_back(ar);
			}
		}
	}
	if(dir_from.empty() && ! dir_to.empty()){
		dir_from = dir_to;
	}
	if(! dir_from.empty() && dir_to.empty()){
		dir_to = dir_from;
	}
	if(dir_from.empty() && dir_to.empty()){
		dir_to = ".";
		dir_from = ".";
	}
	if(! fs::exists(dir_from)){
		fprintf(stderr, "directory %s given in option --from does not exist\n", dir_from.c_str());
		return false;
	}
	if(! fs::exists(dir_to)){
		fprintf(stderr, "directory %s given in option --to does not exist\n", dir_to.c_str());
		return false;
	}
	dir_from = fs::canonical(dir_to);
	dir_to = fs::canonical(dir_to);
	
	fprintf(stdout, "Using dir from %s\n", dir_from.c_str());
	fprintf(stdout, "Using dir to %s\n", dir_to.c_str());
	
	base_pth = dir_from;
	tmp_pth = base_pth / tmp_nam;
	fs::create_directories(tmp_pth.parent_path());
	
	if(oper == zo_action::nothing){
		just_list = true;
	}
	
	return true;
}

void
zo_fname::print_actions(zo_orga& org){
	if(nxt_pth.empty()){
		fprintf(stdout, "SKIP '%s'\n", orig_pth.c_str());
		return;
	}
	bool is_mv = org.is_move();
	zo_string act = (is_mv)?("move"):("copy");
	fprintf(stdout, "'%s' %s to: \n\t '%s'\n", orig_pth.c_str(), act.c_str(), nxt_pth.c_str());
}

void
zo_ref::print_actions(zo_orga& org){
	fprintf(stdout, "----------\n");
	if(! bad_pth.empty()){
		fprintf(stdout, "BAD_REFERENCE_SKIPPED '%s'\n", bad_pth.c_str());
		return;
	}
	fprintf(stdout, "original_sample: %s\n", get_orig().c_str());
	fprintf(stdout, "original_rel_sample: %s\n", get_orig_rel().c_str());
	if(get_next().empty()){
		fprintf(stdout, "KEEPING_ORIG_LINE %ld\n", num_line);
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
zo_sfont::print_actions(zo_orga& org){
	fpth.print_actions(org);
	for(auto rf: all_ref){
		rf->print_actions(org);
	}
}

void
zo_sample::print_actions(zo_orga& org){
	fpth.print_actions(org);
	for(zo_sfont_pt sf: all_bk_ref){
		fprintf(stdout, "FOUND IN----------\n");
		sf->fpth.print_actions(org);
	}
	
}

void
zo_dir::print_actions(zo_orga& org){
	for(const auto& sfe : all_selected_sfz){
		zo_sfont_pt sf = sfe.second;
		sf->print_actions(org);
	}
	for(const auto& sme : all_selected_spl){
		zo_sample_pt sm = sme.second;
		sm->print_actions(org);
	}
}

zo_path
fix_path(const zo_string& pth_str, bool is_fst){
	zo_path pth = pth_str;
	zo_string nm = pth.filename();
	if(is_fst){
		fix_name(nm);
	} else {
		inc_name(nm);
	}
	zo_path nx_pth = pth.parent_path() / nm;
	return nx_pth;
}

void
zo_fname::fix_next(){
	if(nxt_pth.empty()){
		nxt_pth = fix_path(orig_pth, true);
		return;
	}
	nxt_pth = fix_path(nxt_pth, false);
}

void
zo_sfont::prepare_fix(zo_dir& dir){
	zo_sfont_pt sf = this;
	ZO_CK(fpth.nxt_pth.empty());
	fpth.fix_next();
	bool is_nw = false;
	zo_sfont_pt nx_sf = dir.get_next_soundfont(fpth.nxt_pth, sf, is_nw);
	ZO_MARK_USED(nx_sf);
	while(! is_nw){
		fpth.fix_next();
		nx_sf = dir.get_next_soundfont(fpth.nxt_pth, sf, is_nw);
	}
	ZO_CK(nx_sf == sf);
}

void
zo_sample::prepare_fix(zo_dir& dir){
	zo_sample_pt sp = this;
	ZO_CK(fpth.nxt_pth.empty());
	fpth.fix_next();
	bool is_nw = false;
	zo_sample_pt nx_sp = dir.get_next_sample(fpth.nxt_pth, sp, is_nw);
	ZO_MARK_USED(nx_sp);
	while(! is_nw){
		fpth.fix_next();
		nx_sp = dir.get_next_sample(fpth.nxt_pth, sp, is_nw);
	}
	ZO_CK(nx_sp == sp);
}

void
zo_orga::prepare_fix(){
	zo_dir& dir = *this; 
	for(const auto& sfe : all_selected_sfz){
		zo_sfont_pt sf = sfe.second;
		sf->prepare_fix(dir);
	}
	for(const auto& sme : all_selected_spl){
		zo_sample_pt sm = sme.second;
		sm->prepare_fix(dir);
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
	
	zo_string nm = args[1];
	zo_path fx1 = fix_path(nm, true);
	fprintf(stdout, "%s\n", fx1.c_str());

	zo_string unm = args[1];
	zo_path fx2 = fix_path(unm, false);
	fprintf(stdout, "%s\n", fx2.c_str());
	
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
	if(get_next().empty()){
		return;
	}
	zo_path nxt = get_next();
	fs::create_directories(nxt.parent_path());
	fpth.nxt_pth = "";
	ZO_CK(get_next().empty());
	
	bool is_mv = org.is_move();
	zo_path tmp = org.get_temp_path();
	prepare_tmp_file(tmp);
	fs::rename(tmp, nxt);
	if(is_mv){
		fs::remove(get_orig());
		return;
	}
}

void 
zo_sample::do_actions(zo_orga& org){
	if(get_next().empty()){
		return;
	}
	zo_path nxt = get_next();
	fs::create_directories(nxt.parent_path());
	fpth.nxt_pth = "";
	ZO_CK(get_next().empty());
	
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
		ZO_CK(it != all_ref.end());
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
		rf->print_lines(dst, ln);
	}
	
	//return 0;
}

void 
zo_ref::print_lines(std::ofstream& dst, const zo_string& ln){
	if(! bad_pth.empty()){
		dst << ln << '\n';
		return;
	}
	if(get_next().empty()){
		dst << ln << '\n';
		return;
	} 
	if(! prefix.empty()){
		dst << prefix << '\n';
	}
	dst << "sample=" << get_next_rel() << '\n';
	if(! suffix.empty()){
		dst << suffix << '\n';
	}
}

void
zo_orga::organizer_main(const zo_str_vec& args){
	fprintf(stderr, "organizer_main\n");
	if(! get_args(args)){
		fprintf(stderr, "not enough args\n");
		return;
	}
	std::cout << "Starting\n";
	if(just_list){
		std::cout << "just_list\n";
	}
	
	zo_orga& org = *this;
	read_selected();
	
	if(oper == zo_action::fix){
		prepare_fix();
	}
	
	if(just_list){
		std::cout << "just_list\n";
		print_actions(org);
		return;
	}
	
	ZO_CK(oper != zo_action::nothing);
	do_actions(org);
}

