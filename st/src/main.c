/*
 * Copyright 2012 Peter Bašista
 *
 * This file is part of the stc.
 * 
 * stc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * The benchmarks of the construction algorithms for the suffix tree.
 * This file contains the implementation of the benchmarks,
 * which create and maintain the suffix tree in the memory.
 */

/* feature test macros */

/** This macro is necessary for the function srandom. */
#define	_BSD_SOURCE

/**
 * This macro is necessary for the function getopt
 * and variables optarg and optind.
 */
#define	_POSIX_C_SOURCE 2

#ifdef	__APPLE__
/**
 * This macro is necessary for the ru_maxrss member
 * of the struct getrusage under Mac OS
 */
#define	_DARWIN_C_SOURCE
#endif

#include "stree.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* for getrusage */
#include <sys/time.h>
#include <sys/resource.h>

/* Doxygen main page documentation */

/**
 * @mainpage
 *
 * @section Introduction
 * This project implementats some of the basic suffix tree construction
 * algorithms. It is possible to use it for benchmarking the time
 * and space utilization of the implemented algorithms.
 *
 * There are several main types of suffix tree construction algorithms.
 * This project aims to implement some algorithms of the following two types.
 *
 * \li	Construction of the whole suffix tree in the memory (program @ref st)
 * \li	Construction and maintaining of the suffix tree only for the part
 * of the text, called the sliding window (program @ref stsw)
 *
 * @page st Suffix tree
 *
 * @section Description
 *
 * The implemented suffix tree construction algorithms are:
 *
 * \li	simple McCreight's algorithm without the use of the suffix links
 * \li	complete McCreight's algorithm utilizing the suffix links
 * \li	simple Ukkonen's algorithm without the use of the suffix links
 * \li	complete Ukkonen's algorithm utilizing the suffix links
 * \li	Partition and Write Only Top Down (PWOTD) algorithm
 * 	by S. Tata, R. A. Hankins and J. M. Patel
 * 	without the use of the suffix links
 *
 * Both the McCreight's and Ukkonen's algorithms utilizing the suffix links
 * are implemented in two variations:
 *
 * \li	standard variation with top-down suffix link simulation
 * \li	the variation by M. Senft and T. Dvořák
 * 	with bottom-up suffix link simulation
 * 	(also referred to as "the minimized branching" variation)
 *
 * Every variation of McCreight's and Ukkonen's algorithm is implemented
 * using both of the following implementation techniques by S. Kurtz:
 *
 * \li	Simple Linked List Implementation (SL)
 * \li	Simple Hash Table Implementation (SH)
 *
 * The PWOTD algorithm is implemented using the implementation technique
 * first described by R. Giegerich, S. Kurtz and J. Stoye, which we further
 * refer to as the Simple Linear Array Implementation (LA).
 *
 * @section Usage
 *
 * This program can be executed like this:
 *
@verbatim
./st -t <type> -a <algorithm>[variation] -b <benchmark> [options] filename
@endverbatim
 *
 * which effectively results in
 * performing benchmark <tt>&lt;benchmark&gt;</tt>
 * on the suffix tree for the text from file @c 'filename'
 * using the implementation type <tt>&lt;type&gt;</tt>
 * and construction algorithm <tt>&lt;algorithm&gt;</tt>
 * with variation <tt>[variation]</tt>.
 *
 * The available implementation types are:
 *
 * \li	@c SL	simple linked list (S. Kurtz)
 * \li	@c SH	simple hash table (S. Kurtz)
 * \li	@c LA	simple linear array (R. Giegerich, S. Kurtz and J. Stoye)
 *
 * The available construction algorithms are:
 *
 * \li	@c A	simple McCreight's style
 * \li	@c M	McCreight's
 * \li	@c B	simple Ukkonen's style
 * \li	@c U	Ukkonen's
 * \li	@c P	Partition and Write Only Top Down (PWOTD)
 *
 * The available algorithm variations are:
 * \li	{empty}	default variation
 * \li	@c B	minimized branching (bottom-up suffix link simulation)
 *
 * The available benchmarks are:
 * \li	@c C	create and delete the suffix tree
 * \li	@c T	create, traverse and delete the suffix tree
 *
 * Additional available options are:
 *
 * \li	<tt>-p &lt;number&gt;</tt>
 * 		Forces the PWOTD algorithm to use the specified @c number
 * 		of prefix characters to divide the suffixes
 * 		into the partitions.
 * \li	<tt>-r &lt;CRT&gt;</tt>
 * 		Forces the simple hash table implementation type to use
 * 		the specified collision resolution technique @c CRT.
 * 		The default value is @c C for the Cuckoo hashing.
 * 		Alternatively, you can use @c D for the double hashing.
 * \li	<tt>-c &lt;number&gt;</tt>
 * 		Forces the Cuckoo hashing collision resolution technique
 * 		to use the specified @c number of hash functions.
 * 		The default value is 8.
 * \li	@c -s	Enables simple traversal logs, which have the same format
 * 		for all the algorithms and implementation techniques.
 * \li	<tt>-d &lt;dump_filename&gt;</tt>
 * 		If the traverse benchmark is selected,
 * 		the log from the traversal of the suffix tree
 * 		will be printed to the file @c 'dump_filename'
 * 		instead of the standard output.
 * \li	<tt>-e &lt;file_encoding&gt;</tt>
 * 		Specifies the character encoding of the input file
 * 		@c 'filename'. The default value is @c UTF-8.
 * 		The valid encodings are all those supported by the iconv.
 * \li	<tt>-i &lt;internal_encoding&gt;</tt>
 * 		Specifies the internal text tencoding to use. The default
 * 		value depends on the size of the @ref character_type.
 */

/* helping function */

/**
 * A function, which prints the short usage text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_short_usage (const char *argv0) {
	printf("Usage:\t%s\t-t <type> ", argv0);
	printf("-a <algorithm>[variation] -b <benchmark> [options]\n"
		"\t\tfilename\n\n"
		"This will perform the benchmark <benchmark> "
		"on the suffix tree\nfor the text from the file "
		"'filename' using the implementation type <type>\n"
		"and the construction algorithm <algorithm> "
		"with variation [variation]\n\n");
	return (0);
}

/**
 * A function, which prints the help text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_help (const char *argv0) {
	print_short_usage(argv0);
	printf("Available implementation types are:\n"
		"SL\tsimple linked list (S. Kurtz)\n"
		"SH\tsimple hash table (S. Kurtz)\n"
		"LA\tsimple linear array (R. Giegerich, "
		"S. Kurtz and J. Stoye)\n\n"
		"Available construction algorithms are:\n"
		"A\tsimple McCreight's style\n"
		"M\tMcCreight's\n"
		"B\tsimple Ukkonen's style\n"
		"U\tUkkonen's\n"
		"P\tPartition and Write Only Top Down (PWOTD)\n\n"
		"Available algorithm variations are:\n"
		"{empty}\tdefault variation\n"
		"B\tminimized branching (bottom-up "
		"suffix link simulation)\n\n");
	/*
	 * dividing the string literal into more parts, just to fit
	 * in the length limit of 509 characters,
	 * imposed by the requirements for ISO C90 compilers
	 */
	printf("Available benchmarks are:\n"
		"C\tcreate and delete the suffix tree\n"
		"T\tcreate, traverse and delete the suffix tree\n\n"
		"Additional options:\n"
		"-p <number>\t\tForces the PWOTD algorithm to use\n"
		"\t\t\tthe specified <number> of prefix characters\n"
		"\t\t\tto divide the suffixes into the partitions.\n"
		"-r <CRT>\t\tForces the simple hash table implementation\n"
		"\t\t\ttype to use the specified collision resolution\n"
		"\t\t\ttechnique <CRT>. The default value is C\n"
		"\t\t\tfor the Cuckoo hashing. Alternatively,\n"
		"\t\t\tyou can use D for the double hashing.\n"
		"-c <number>\t\tForces the Cuckoo hashing collision\n"
		"\t\t\tresolution technique to use the specified number\n"
		"\t\t\tof hash functions. The default value is 8.\n");
	printf("-s\t\t\tEnables simple traversal logs,\n"
		"\t\t\twhich have the same format for all the algorithms\n"
		"\t\t\tand implementation techniques.\n"
		"-d <dump_filename>\tIf the traverse benchmark is selected,\n"
		"\t\t\tthe log from the traversal of the suffix tree\n"
		"\t\t\twill be printed to the file 'dump_filename'\n"
		"\t\t\tinstead of to the standard output.\n"
		"-e <file_encoding>\tSpecifies the character encoding\n"
		"\t\t\tof the input file 'filename'. The default value\n"
		"\t\t\tis UTF-8. The valid encodings are all those\n"
		"\t\t\tsupported by the iconv.\n"
		"-i <internal_encoding>\tSpecifies the internal text "
		"encoding to use.\n\t\t\tThe default value depends "
		"on the size of the\n\t\t\t\"character_type\".\n");
	return (0);
}

/**
 * A function, which prints the full usage text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_usage (const char *argv0) {
	print_short_usage(argv0);
	printf("For the list of available parameter values\n"
			"and additional options, run: %s -h\n",
			argv0);
	return (0);
}

/* benchmarking functions */

/**
 * A function, which tries to run the specified SLLI based benchmark
 * of the desired construction algorithm for the suffix tree.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written (if requested)
 * @param
 * algorithm	the desired construction algorithm to use
 * @param
 * benchmark	the requested benchmark to use
 * @param
 * traversal_type	the type of the suffix tree traversal,
 * 			which will be performed (if requested)
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 *
 * @return	If the SL implementation technique is not compatible
 * 		with the selected algorithm, one (1) is returned.
 * 		Otherwise, zero (0) is returned.
 */
int benchmark_slli (FILE *stream,
		int algorithm,
		int benchmark,
		int traversal_type,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length) {
	suffix_tree_slli stree = {.lr_size = 0};
	switch (algorithm) {
		case 1:
			st_slli_create_simple_mccreight(text, length, &stree);
			break;
		case 2:
			st_slli_create_mccreight(text, length, &stree);
			break;
		case 3:
			st_slli_create_simple_ukkonen(text, length, &stree);
			break;
		case 4:
			st_slli_create_ukkonen(text, length, &stree);
			break;
		case 5:
			fprintf(stderr, "The selected implementation "
					"technique (SL)\n"
					"is not compatible with "
					"the desired algorithm (PWOTD)\n");
			return (1);
	}
	if (benchmark == 2) {
		st_slli_traverse(stream, internal_text_encoding,
				traversal_type, text, length, &stree);
	}
	st_slli_delete(&stree);
	return (0);
}

/**
 * A function, which tries to run the specified SHTI based benchmark
 * of the desired construction algorithm for the suffix tree.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written (if requested)
 * @param
 * algorithm	the desired construction algorithm to use
 * @param
 * benchmark	the requested benchmark to use
 * @param
 * traversal_type	the type of the suffix tree traversal,
 * 			which will be performed (if requested)
 * @param
 * crt_type	the desired type of the collision resolution technique to use
 * @param
 * chf_number	the desired number of the Cuckoo hash functions
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 *
 * @return	If the SH implementation technique is not compatible
 * 		with the selected algorithm, one (1) is returned.
 * 		Otherwise, zero (0) is returned.
 */
int benchmark_shti (FILE *stream,
		int algorithm,
		int benchmark,
		int traversal_type,
		int crt_type,
		size_t chf_number,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length) {
	suffix_tree_shti stree = {.hs_size = 0};
	stree.crt_type = crt_type;
	stree.chf_number = chf_number;
	switch (algorithm) {
		case 1:
			st_shti_create_simple_mccreight(text, length, &stree);
			break;
		case 2:
			st_shti_create_mccreight(text, length, &stree);
			break;
		case 3:
			st_shti_create_simple_ukkonen(text, length, &stree);
			break;
		case 4:
			st_shti_create_ukkonen(text, length, &stree);
			break;
		case 5:
			fprintf(stderr, "The selected implementation "
					"technique (SH)\n"
					"is not compatible with "
					"the desired algorithm (PWOTD)\n");
			return (1);
	}
	if (benchmark == 2) {
		st_shti_traverse(stream, internal_text_encoding,
				traversal_type, text, length, &stree);
	}
	st_shti_delete(&stree);
	return (0);
}

/**
 * A function, which tries to run the specified SLAI based benchmark
 * of the desired construction algorithm for the suffix tree.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written (if requested)
 * @param
 * algorithm	the desired construction algorithm to use
 * @param
 * benchmark	the requested benchmark to use
 * @param
 * prefix_length	the length of the prefix, which will be considered
 * 			when dividing the suffixes into the partitions
 * @param
 * traversal_type	the type of the suffix tree traversal,
 * 			which will be performed (if requested)
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 *
 * @return	If the LA implementation technique is not compatible
 * 		with the selected algorithm, one (1) is returned.
 * 		Otherwise, zero (0) is returned.
 */
int benchmark_slai (FILE *stream,
		int algorithm,
		int benchmark,
		long int prefix_length,
		int traversal_type,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length) {
	char *algorithm_names[4] = {NULL};
	suffix_tree_slai stree = {.tnode = NULL};
	algorithm_names[0] = "simple McCreight's style";
	algorithm_names[1] = "McCreight's";
	algorithm_names[2] = "simple Ukkonen's style";
	algorithm_names[3] = "Ukkonen's";
	switch (algorithm) {
		case 1:
		case 2:
		case 3:
		case 4:
			fprintf(stderr, "The selected implementation "
					"technique (LA)\n"
					"is not compatible with "
					"the desired algorithm (%s)\n",
					algorithm_names[algorithm - 1]);
			return (1);
		case 5:
			st_slai_create_pwotd(prefix_length,
					text, length, &stree);
			break;
	}
	if (benchmark == 2) {
		st_slai_traverse(stream, internal_text_encoding,
				traversal_type, text, length, &stree);
	}
	st_slai_delete(&stree);
	return (0);
}

/**
 * A function, which tries to run the specified SLLI_BP based benchmark
 * of the desired construction algorithm for the suffix tree.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written (if requested)
 * @param
 * algorithm	the desired construction algorithm to use
 * @param
 * benchmark	the requested benchmark to use
 * @param
 * traversal_type	the type of the suffix tree traversal,
 * 			which will be performed (if requested)
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 *
 * @return	If the benchmark could be successfully started,
 * 		zero (0) is returned.
 * 		Otherwise, a positive error number is returned.
 */
int benchmark_slli_bp (FILE *stream,
		int algorithm,
		int benchmark,
		int traversal_type,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length) {
	suffix_tree_slli_bp stree = {.lr_size = 0};
	switch (algorithm) {
		case 1:
			fprintf(stderr, "The selected implementation "
					"technique (SL) "
					"and algorithm variation (B)\n"
					"are not compatible with "
					"the desired algorithm "
					"(simple McCreight's style)\n");
			return (1);
		case 2:
			st_slli_bp_create_mccreight(text, length, &stree);
			break;
		case 3:
			fprintf(stderr, "The selected implementation "
					"technique (SL) "
					"and algorithm variation (B)\n"
					"are not compatible with "
					"the desired algorithm "
					"(simple Ukkonen's style)\n");
			return (2);
		case 4:
			st_slli_bp_create_ukkonen(text, length, &stree);
			break;
		case 5:
			fprintf(stderr, "The selected implementation "
					"technique (SL) "
					"and algorithm variation (B)\n"
					"are not compatible with "
					"the desired algorithm (PWOTD)\n");
			return (3);
	}
	if (benchmark == 2) {
		st_slli_bp_traverse(stream, internal_text_encoding,
				traversal_type, text, length, &stree);
	}
	st_slli_bp_delete(&stree);
	return (0);
}

/**
 * A function, which tries to run the specified SHTI_BP based benchmark
 * of the desired construction algorithm for the suffix tree.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written (if requested)
 * @param
 * algorithm	the desired construction algorithm to use
 * @param
 * benchmark	the requested benchmark to use
 * @param
 * traversal_type	the type of the suffix tree traversal,
 * 			which will be performed (if requested)
 * @param
 * crt_type	the desired type of the collision resolution technique to use
 * @param
 * chf_number	the desired number of the Cuckoo hash functions
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 *
 * @return	If the benchmark could be successfully started,
 * 		zero (0) is returned.
 * 		Otherwise, a positive error number is returned.
 */
int benchmark_shti_bp (FILE *stream,
		int algorithm,
		int benchmark,
		int traversal_type,
		int crt_type,
		size_t chf_number,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length) {
	suffix_tree_shti_bp stree = {.hs_size = 0};
	stree.crt_type = crt_type;
	stree.chf_number = chf_number;
	switch (algorithm) {
		case 1:
			fprintf(stderr, "The selected implementation "
					"technique (SH) "
					"and algorithm variation (B)\n"
					"are not compatible with "
					"the desired algorithm "
					"(simple McCreight's style)\n");
			return (1);
		case 2:
			st_shti_bp_create_mccreight(text, length, &stree);
			break;
		case 3:
			fprintf(stderr, "The selected implementation "
					"technique (SH) "
					"and algorithm variation (B)\n"
					"are not compatible with "
					"the desired algorithm "
					"(simple Ukkonen's style)\n");
			return (2);
		case 4:
			st_shti_bp_create_ukkonen(text, length, &stree);
			break;
		case 5:
			fprintf(stderr, "The selected implementation "
					"technique (SH) "
					"and algorithm variation (B)\n"
					"are not compatible with "
					"the desired algorithm (PWOTD)\n");
			return (3);
	}
	if (benchmark == 2) {
		st_shti_bp_traverse(stream, internal_text_encoding,
				traversal_type, text, length, &stree);
	}
	st_shti_bp_delete(&stree);
	return (0);
}

/* the main function */

/**
 * The main function.
 * It executes the desired benchmark of the specified
 * construction algorithm for the suffix tree.
 *
 * @param
 * argc		the argument count, or the number of program arguments
 * 		(including the argv[0])
 * @param
 * argv		the argument vector itself, or an array of argument strings
 *
 * @return	If the benchmark is successful or was not requested,
 * 		this function returns EXIT_SUCCESS.
 * 		Otherwise, this function returns EXIT_FAILURE.
 */
int main (int argc, char **argv) {
	/*
	 * According to the C specification, the non-listed members
	 * of the struct are initialized to "zero-like" values
	 * automatically when only some of the first elements
	 * are present in the initialization.
	 *
	 * But still, at least gcc produces
	 * an unpleasant warning message here.
	 *
	 * That's why we use the designated initializers,
	 * an alternative which does not produce gcc warnings.
	 */
	struct rusage resource_usage_struct = {.ru_maxrss = 0};
	/* the maximum resident set size */
	size_t maximum_rss_size = 0;
	char c = '\0';
	char *endptr = NULL;
	int getopt_retval = 0;
	int type = 0;
	int algorithm = 0;
	int variation = 0;
	int benchmark = 0;
	/*
	 * the desired collision resolution technique used when hashing
	 * available values:	1 - Cuckoo hashing
	 * 			2 - double hashing
	 */
	int crt_type = 0;
	/* the desired number of Cuckoo hash functions */
	size_t chf_number = 0;
	/*
	 * the default value of (-1) means that the prefix length
	 * will be determined automatically based on the text length
	 */
	long int prefix_length = (-1);
	/* by default, we would like the traversal to be detailed */
	int traversal_type = tt_detailed;
	/*
	 * The pointer to the identification string
	 * of the internal text encoding.
	 */
	char *internal_text_encoding = NULL;
	/*
	 * The pointer to the program argument representing the
	 * identification string of the desired internal text encoding.
	 */
	char *internal_text_encoding_arg = NULL;
	/* By default, we suppose that the input file encoding is UTF-8 */
	char *input_file_encoding = "UTF-8";
	char *input_filename = NULL;
	char *dump_filename = NULL;
	char *algorithm_names[5] = {NULL};
	character_type *text = NULL;
	FILE *stream = stdout;
	size_t length = 0;
	algorithm_names[0] = NULL;
	algorithm_names[1] = "simple McCreight's style";
	algorithm_names[2] = "McCreight's";
	algorithm_names[3] = "simple Ukkonen's style";
	algorithm_names[4] = "Ukkonen's";
	printf("Benchmark of the suffix tree construction algorithms\n\n");
	printf("Compile-time options:\n"
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
			"character_type is wchar_t\n"
#else
			"character_type is char\n"
#endif
	"\n");
	if (argc == 1) {
		print_usage(argv[0]);
		return (EXIT_SUCCESS);
	}
	/* parsing the command line options */
	while ((getopt_retval = getopt(argc, argv, "t:a:b:p:r:c:sd:e:i:h")) !=
			(-1)) {
		c = (char)(getopt_retval);
		switch (c) {
			case 't':
				if ((optarg[0] == 'S') &&
						(optarg[1] == 'L')) {
					type = 1;
				} else if ((optarg[0] == 'S') &&
						(optarg[1] == 'H')) {
					type = 2;
				} else if ((optarg[0] == 'L') &&
						(optarg[1] == 'A')) {
					type = 3;
				} else {
					fprintf(stderr, "Unrecognized "
						"argument for the -t "
						"parameter!\n\n");
					print_usage(argv[0]);
					return (EXIT_FAILURE);
				}
				break;
			case 'a':
				if (optarg[0] == 'A') {
					algorithm = 1;
				} else if (optarg[0] == 'M') {
					algorithm = 2;
				} else if (optarg[0] == 'B') {
					algorithm = 3;
				} else if (optarg[0] == 'U') {
					algorithm = 4;
				} else if (optarg[0] == 'P') {
					algorithm = 5;
				} else {
					fprintf(stderr, "Unrecognized "
						"argument for the -a "
						"parameter!\n\n");
					print_usage(argv[0]);
					return (EXIT_FAILURE);
				}
				if (optarg[1] == 0) {
					variation = 0;
				} else if ((optarg[1] == 'B') &&
						(optarg[2] == 0)) {
					variation = 1;
				} else {
					fprintf(stderr, "Unrecognized "
						"argument for the -a "
						"parameter!\n\n");
					print_usage(argv[0]);
					return (EXIT_FAILURE);
				}
				break;
			case 'b':
				if (optarg[0] == 'C') {
					benchmark = 1;
				} else if (optarg[0] == 'T') {
					benchmark = 2;
				} else {
					fprintf(stderr, "Unrecognized "
						"argument for the -b "
						"parameter!\n\n");
					print_usage(argv[0]);
					return (EXIT_FAILURE);
				}
				break;
			case 'p':
				prefix_length = strtol(optarg, &endptr, 0);
				if ((*endptr) != '\0') {
					fprintf(stderr, "Unrecognized "
						"argument for the -p "
						"parameter!\n\n");
					return (EXIT_FAILURE);
				}
				if (errno != 0) {
					perror("strtol(prefix_length)");
					/* resetting the errno */
					errno = 0;
					return (EXIT_FAILURE);
				}
				break;
			case 'r':
				if (optarg[0] == 'C') {
					crt_type = 1;
				} else if (optarg[0] == 'D') {
					crt_type = 2;
				} else {
					fprintf(stderr, "Unrecognized "
						"argument for the -r "
						"parameter!\n\n");
					return (EXIT_FAILURE);
				}
				break;
			case 'c':
				chf_number = strtoul(optarg, &endptr, 0);
				if ((*endptr) != '\0') {
					fprintf(stderr, "Unrecognized "
						"argument for the -c "
						"parameter!\n\n");
					return (EXIT_FAILURE);
				}
				if (errno != 0) {
					perror("strtoul(chf_number)");
					/* resetting the errno */
					errno = 0;
					return (EXIT_FAILURE);
				}
				break;
			case 's':
				traversal_type = tt_simple;
				break;
			case 'd':
				dump_filename = optarg;
				break;
			case 'e':
				input_file_encoding = optarg;
				break;
			case 'i':
				internal_text_encoding_arg = optarg;
				break;
			case 'h':
				print_help(argv[0]);
				return (EXIT_SUCCESS);
			case '?':
				return (EXIT_FAILURE);
		}
	}
	if (optind == argc) {
		fprintf(stderr, "Missing the 'filename' parameter!\n\n");
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	input_filename = argv[optind];
	if (optind + 1 < argc) {
		fprintf(stderr, "Too many parameters!\n\n");
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	/* command line options parsing complete */
	if (type == 0) {
		fprintf(stderr, "The -t parameter is mandatory "
				"and it was not specified!\n\n");
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	if (algorithm == 0) {
		fprintf(stderr, "The -a parameter is mandatory "
				"and it was not specified!\n\n");
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	if (benchmark == 0) {
		fprintf(stderr, "The -b parameter is mandatory "
				"and it was not specified!\n\n");
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	if (type == 3) {
		if (algorithm != 5) {
			fprintf(stderr, "Error: The selected implementation "
					"type (LA)\n"
					"does not support the desired "
					"algorithm (%s)!\n",
					algorithm_names[algorithm]);
			return (EXIT_FAILURE);
		}
	}
	if (algorithm == 5) {
		if (type == 1) {
			fprintf(stderr, "Error: The selected implementation "
					"type (SL)\n"
					"does not support the desired "
					"algorithm (PWOTD)!\n");
			return (EXIT_FAILURE);
		} else if (type == 2) {
			fprintf(stderr, "Error: The selected implementation "
					"type (SH)\n"
					"does not support the desired "
					"algorithm (PWOTD)!\n");
			return (EXIT_FAILURE);
		}
	}
	if (variation == 1) {
		if ((algorithm == 1) ||
				(algorithm == 3) ||
				(algorithm == 5)) {
			fprintf(stderr, "Error: The selected algorithm "
					"(%s)\n"
					"does not support the desired "
					"algorithm variation (B)!\n",
					algorithm_names[algorithm]);
			return (EXIT_FAILURE);
		}
	}
	if ((dump_filename != NULL) && (benchmark != 2)) {
		fprintf(stderr, "The -d parameter "
				"can only be used with the traverse (T) "
				"type of benchmark!\n");
		return (EXIT_FAILURE);
	}
	if ((traversal_type != tt_detailed) && (benchmark != 2)) {
		fprintf(stderr, "The -s parameter "
				"can only be used with the traverse (T) "
				"type of benchmark!\n");
		return (EXIT_FAILURE);
	}
	if ((type != 2) && (crt_type != 0)) {
		fprintf(stderr, "The -r parameter "
				"can only be used with the SH "
				"implementation type!\n");
		return (EXIT_FAILURE);
	}
	if ((type != 2) && (chf_number != 0)) {
		fprintf(stderr, "The -c parameter "
				"can only be used with the SH "
				"implementation type!\n");
		return (EXIT_FAILURE);
	}
	/*
	 * The Cuckoo hashing collision resolution technique
	 * is supposed to be represented by crt_type == 1.
	 * Otherwise the following conditional expression is incorrect.
	 */
	if ((type == 2) && (crt_type != 1) && (chf_number != 0)) {
		fprintf(stderr, "The -c parameter "
				"can only be used with the SH "
				"implementation type\nwhen the collision "
				"resolution technique is set "
				"to the Cuckoo hashing!\n");
		return (EXIT_FAILURE);
	}
	if ((type != 3) && (prefix_length != (-1))) {
		fprintf(stderr, "The -p parameter "
				"can only be used with the LA "
				"implementation type!\n");
		return (EXIT_FAILURE);
	}
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	if ((type == 2) && (benchmark == 2)) {
		fprintf(stderr, "Warning:\n"
				"========\n"
				"This program is compiled "
				"with the support for Unicode characters.\n"
				"You have chosen a hash table based "
				"implementation type (SH)\n"
				"and a benchmark, which traverses "
				"the suffix tree.\nThis combination "
				"is not recommended, because it will "
				"most likely result in\nhanging the program "
				"during the suffix tree traversal!\n\n");
	}
#endif
	/*
	 * we need not to free the internal_text_encoding,
	 * because it is a local variable, which has been initialized to NULL
	 */
	internal_text_encoding = calloc((size_t)(64), (size_t)(1));
	if (internal_text_encoding == NULL) {
		perror("calloc(internal_text_encoding)");
		/* resetting the errno */
		errno = 0;
		return (EXIT_FAILURE);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	if (internal_text_encoding_arg != NULL) {
		/*
		 * We test if the argument representing the internal text
		 * encoding fits into the provided buffer. Its length
		 * might not be more than 63 bytes, because the last byte
		 * is reserved for the terminating NULL character.
		 */
		if (strlen(internal_text_encoding_arg) > (size_t)(63)) {
			fprintf(stderr, "The argument for the -i parameter "
					"is too long.\nIts maximum length "
					"is 63 bytes (characters).\n");
			/*
			 * strlen returns the text length NOT including
			 * the terminating NULL character
			 */
			return (EXIT_FAILURE);
		}
		strcpy(internal_text_encoding, internal_text_encoding_arg);
	}
	if (text_read(input_filename, input_file_encoding,
				&internal_text_encoding,
				&text, &length) > 0) {
		return (EXIT_FAILURE);
	}
	if (dump_filename != NULL) {
		/* if we got here, benchmark must be set to 2 */
		stream = fopen(dump_filename, "w");
		if (stream == NULL) {
			perror("fopen(stream)");
			return (EXIT_FAILURE);
		}
	}
	/* random number generator initialization */
	srandom((unsigned int)(time(NULL)));
	if (variation == 0) {
		switch (type) {
			case 1:
				benchmark_slli(stream, algorithm, benchmark,
						traversal_type,
						internal_text_encoding,
						text, length);
				break;
			case 2:
				benchmark_shti(stream, algorithm, benchmark,
						traversal_type,
						crt_type, chf_number,
						internal_text_encoding,
						text, length);
				break;
			case 3:
				benchmark_slai(stream, algorithm, benchmark,
						prefix_length, traversal_type,
						internal_text_encoding,
						text, length);
				break;
		}
	} else {
		switch (type) {
			case 1:
				benchmark_slli_bp(stream, algorithm, benchmark,
						traversal_type,
						internal_text_encoding,
						text, length);
				break;
			case 2:
				benchmark_shti_bp(stream, algorithm, benchmark,
						traversal_type,
						crt_type, chf_number,
						internal_text_encoding,
						text, length);
				break;
			case 3:
				fprintf(stderr, "Error: The selected "
						"implementation technique "
						" (LA)\ndoes not support "
						"the desired algorithm "
						"variation (B)!\n");
				break;
		}
	}
	getrusage(RUSAGE_SELF, &resource_usage_struct);
	printf("\nFinal CPU and memory statistics:\n"
			"--------------------------------\n");
	printf("Total benchmark CPU user time: ");
	print_human_readable_time(stdout, (size_t)
			/* seconds to milliseconds */
			(resource_usage_struct.ru_utime.tv_sec * 1000
			/* microseconds to milliseconds */
			+ resource_usage_struct.ru_utime.tv_usec / 1000));
	printf("\nTotal benchmark CPU system time: ");
	print_human_readable_time(stdout, (size_t)
			/* seconds to milliseconds */
			(resource_usage_struct.ru_stime.tv_sec * 1000
			/* microseconds to milliseconds */
			+ resource_usage_struct.ru_stime.tv_usec / 1000));
	/*
	 * The ru_maxrss member of the struct rusage
	 * (usually) has its value in kilobytes.
	 * One notable exception is the Apple platform
	 * (Mac OS X, for example), where the ru_maxrss is in bytes.
	 * So, we need to take that into account!
	 */
#ifdef	__APPLE__
	maximum_rss_size = (size_t)(resource_usage_struct.ru_maxrss);
#else
	maximum_rss_size = (size_t)(resource_usage_struct.ru_maxrss) * 1024;
#endif
	printf("\nThe maximum resident set size: %zu bytes (",
			maximum_rss_size);
	print_human_readable_size(stdout, maximum_rss_size);
	printf(")\n");
	if (dump_filename != NULL) {
		if (fclose(stream) == EOF) {
			perror("fclose(stream)");
			return (EXIT_FAILURE);
		}
	}
	free(internal_text_encoding);
	internal_text_encoding = NULL;
	printf("\nTrying to free the memory allocated for the text\n");
	free(text);
	text = NULL;
	printf("Successfully freed!\n");
	return (EXIT_SUCCESS);
}
