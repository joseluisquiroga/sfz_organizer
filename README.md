# sfz_organizer
A command line program to copy and move sfz soundfonts keeping sample references consistent.

# To build:

```
sudo apt-get install git g++
git https://github.com/joseluisquiroga/sfz_organizer.git
cd sfz_organizer
make
```

# Description


sfz_organizer [OPTION] ... [FILE] ...  
The purpose of sfz_organizer is to move and copy sfz soundfonts while preserving consistency of any referenced sample within them.  

## Options:  
==========  
  
	-l --list  
		Just print what it would do without actualy doing it.  
	-m --move  
		move action. Rest of parameters will decide what and how.  
		If the --to directory is not under of the --from directory the action becomes a --copy.  
	-c --copy  
		copy action. Rest of parameters will decide what and how.  
	-p --purge  
		purge action.  
		moves all non utf8 files with '.sfz' ext to subdir "purged" in the --to directory.  
		moves all sfz soundfonts with no valid sample references under the --from directory, to subdir "purged" in the --to directory.  
		moves all samples with no sample references in sfz soundfonts under the --from directory, to subdir "purged" in the --to directory.  
		Works on the whole --from directory (as if --recursive  and no files were selected).  
	-a --add_sfz_ext  
		add_sfz_ext action. Add the extention ".sfz" to selected files. Rest of parameters will decide what and how.  
		Only does it to utf8 encoded files checking the first Kbyte and only if it does not end with the '.sfz' extension already.  
	-N --normalize  
		normalize action. moves all selected sfz sounfonts and samples (not directories) so that they do not have odd (non alphanumeric) characters.  
		All non alphanumeric characters are replaced with _ (underscore).  
		Conflicts are resolved before writing files by a counter just before each file extension, that increments until there is no conflict. 
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
	--only_samples  
		Only select files without '.sfz' extension.   
	--skip_normalize  
		Do not normalize names. Not recomended because it deactivates conflict solving.  
	--help   
	--version   

## Notes:  
========  
  
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
  
11. The most common error is: 'Cannot open file ...' because of lack of permissions. Give the user write permissions.  
  
12. The name "purged" is reserved. A subdirectory under --from called "purged" is always ignored.  
  

## Examples:  
============  
  
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
  

	
	
# Status
sfz_organizer 0.1

# Warning.
In sfz_v2 files the 'default_path' opcode is commented and included in the sample paths.

# Missing Features.
No #include opcode.



