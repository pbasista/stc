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
 * The benchmarks of the construction algorithms for the suffix tree
 * over a sliding window. This file contains the implementation
 * of the benchmarks, which create and maintain
 * the suffix tree over a sliding window.
 */

/*
 * This file needs to be included in advance of the other include files
 * as well as before any changes to the feature test macros are made,
 * because some of the files it includes might rely on the initial values
 * of the feature test macros.
 */
#include "stsw.h"

/* feature test macros */

#ifndef _BSD_SOURCE

/** This macro is necessary for the function srandom. */
#define	_BSD_SOURCE

#endif

/* if this macro is either undefined or its value is too small */
#if (_POSIX_C_SOURCE - 0) < 2

#undef _POSIX_C_SOURCE

/**
 * This macro is necessary for the function getopt
 * and variables optarg and optind.
 */
#define	_POSIX_C_SOURCE 2

#endif

#ifdef	__APPLE__

#ifndef _DARWIN_C_SOURCE

/**
 * This macro is necessary for the ru_maxrss member
 * of the struct getrusage under Mac OS
 */
#define	_DARWIN_C_SOURCE

#endif

#endif

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
 * @page stsw Suffix tree over a sliding window
 *
 * @section Description
 *
 * The implemented suffix tree construction algorithms are:
 *
 * \li	complete Ukkonen's algorithm utilizing the suffix links
 *
 * Ukkonen's algorithm utilizing the suffix links
 * is implemented in two variations:
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
 * @section Usage
 *
 * This program can be executed like this:
 *
@verbatim
./stsw -t <type> -a <algorithm>[variation] -b <benchmark> [options] filename
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
 *
 * The available construction algorithms are:
 *
 * \li	@c U	Ukkonen's
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
 * \li	<tt>-r &lt;CRT&gt;</tt>
 * 		Forces the simple hash table implementation type to use
 * 		the specified collision resolution technique @c CRT.
 * 		The default value is @c C for the Cuckoo hashing.
 * 		Alternatively, you can use @c D for the double hashing.
 * \li	<tt>-c &lt;number&gt;</tt>
 * 		Forces the Cuckoo hashing collision resolution technique
 * 		to use the specified @c number of hash functions.
 * 		The default value is 8.
 * \li	<tt>-m &lt;method&gt;</tt>
 * 		Forces the edge label maintenance method to use.
 * 		Available values are:
 * 		<ul><li>@c B	batch update by M. Senft</li>
 * 		<li>@c F	Fiala's and Greene's method</li></ul>
 * 		The default method is batch update by M. Senft.
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
 * \li	<tt>-k &lt;sw_block_size&gt;</tt>
 * 		Specifies the desired size of a single block
 * 		in the sliding window in the number of characters.
 * 		The default value is 8388608 characters (C), or 8 MiC.
 * \li	<tt>-A &lt;ap_scale_factor&gt;</tt>
 * 		Specifies the desired size of the active part of the sliding
 * 		window as a multiple of the size of a single block
 * 		in the sliding window, also known as the active part
 * 		scale factor.
 * 		The minimum allowed value, as well as
 * 		the default value is @c 1.
 * \li	<tt>-S &lt;sw_scale_factor&gt;</tt>
 * 		Specifies the desired total sliding window size as a multiple
 * 		of the size of a single block in the sliding window,
 * 		also known as the sliding window scale factor.
 * 		The minimum allowed value is <tt>ap_scale_factor + 1</tt>.
 * 		The default value is <tt>ap_scale_factor + 2</tt>.
 * \li	@c -v	The requested verbosity level. Available values are:
 * 		<ul><li>@c 0	low verbosity</li>
 * 		<li>@c 1	medium verbosity</li>
 * 		<li>@c 2	high verbosity.</li></ul>
 * 		The default verbosity level is low.
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
		"SH\tsimple hash table (S. Kurtz)\n\n"
		"Available construction algorithm is:\n"
		"U\tUkkonen's\n\n"
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
		"-r <CRT>\t\tForces the simple hash table implementation\n"
		"\t\t\ttype to use the specified collision resolution\n"
		"\t\t\ttechnique <CRT>. The default value is C\n"
		"\t\t\tfor the Cuckoo hashing. Alternatively,\n"
		"\t\t\tyou can use D for the double hashing.\n"
		"-c <number>\t\tForces the Cuckoo hashing collision\n"
		"\t\t\tresolution technique to use the specified number\n"
		"\t\t\tof hash functions. The default value is 8.\n"
		"-m <method>\t\tForces the edge label maintenance method\n"
		"\t\t\tto use. Available values are:\n"
		"\t\t\tB\tbatch update by M. Senft\n"
		"\t\t\tF\tFiala's and Greene's method\n"
		"\t\t\tThe default method is batch update by M. Senft.\n");
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
		"-i <internal_encoding>\tSpecifies the internal text\n"
		"\t\t\tencoding to use. The default value depends\n"
		"\t\t\ton the size of the \"character_type\".\n");
	printf("-k <size>\t\tSpecifies the desired <size> of a single block\n"
		"\t\t\tin the sliding window (in the number of characters).\n"
		"\t\t\tThe default value is "
		"8388608 characters (C), or 8 MiC.\n"
		"-A <factor>\t\tSpecifies the desired size of the active "
		"part\n\t\t\tof the sliding window as a multiple "
		"of the size\n\t\t\tof a single block in the sliding window,\n"
		"\t\t\talso known as the active part scale <factor>.\n"
		"\t\t\tThe minimum allowed value,\n"
		"\t\t\tas well as the default value is 1.\n"
		"-S <factor>\t\tSpecifies the desired total sliding window "
		"size\n\t\t\tas a multiple of the size of a single block\n"
		"\t\t\tin the sliding window, also known as\n"
		"\t\t\tthe sliding window scale <factor>.\n"
		"\t\t\tIts value needs to be strictly higher than\n"
		"\t\t\tthe active part scale factor.\n"
		"-v\t\t\tThe requested verbosity level. "
		"Available values are:\n"
		"\t\t\t0\tlow verbosity\n"
		"\t\t\t1\tmedium verbosity\n"
		"\t\t\t2\thigh verbosity\n"
		"\t\t\tThe default verbosity level is low.\n");
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
 * of the desired construction algorithm for the suffix tree
 * over a sliding window.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written (if requested)
 * @param
 * algorithm	the desired construction algorithm to use
 * @param
 * variation	the desired algorithm variation to use
 * @param
 * benchmark	the requested benchmark to use
 * @param
 * traversal_type	the type of the suffix tree traversal,
 * 			which will be performed (if requested)
 * @param
 * requested_verbosity_level	The requested level of verbosity
 * 				of the information collected
 * 				and displayed to the user during
 * 				the suffix tree construction
 * 				and maintenance.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 *
 * @return	If the benchmark could be successfully started,
 * 		zero (0) is returned.
 * 		Otherwise, a positive error number is returned.
 */
int benchmark_slli (FILE *stream,
		const int algorithm,
		const int variation,
		const int benchmark,
		const int traversal_type,
		const int requested_verbosity_level,
		text_file_sliding_window *tfsw) {
	suffix_tree_sliding_window_slli stsw = {.branching_nodes =
		(size_t)(0)};
	switch (algorithm) {
		case 1:
			if ((variation == 0) || (variation == 1)) {
				stsw_slli_create_ukkonen(stream, benchmark,
						variation, traversal_type,
						requested_verbosity_level,
						tfsw, &stsw);
			} else {
				fprintf(stderr, "Unknown value for "
						"the selected algorithm "
						"variation (%d)\n",
						variation);
				return (1);
			}
			break;
		default:
			fprintf(stderr, "Unknown value for the selected "
					"algorithm (%d)\n", algorithm);
			return (2);
	}
	stsw_slli_delete(requested_verbosity_level, &stsw);
	return (0);
}

/**
 * A function, which tries to run the specified SHTI based benchmark
 * of the desired construction algorithm for the suffix tree
 * over a sliding window.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written (if requested)
 * @param
 * algorithm	the desired construction algorithm to use
 * @param
 * variation	the desired algorithm variation to use
 * @param
 * benchmark	the requested benchmark to use
 * @param
 * traversal_type	the type of the suffix tree traversal,
 * 			which will be performed (if requested)
 * @param
 * requested_verbosity_level	The requested level of verbosity
 * 				of the information collected
 * 				and displayed to the user during
 * 				the suffix tree construction
 * 				and maintenance.
 * @param
 * crt_type	the desired type of the collision resolution technique to use
 * @param
 * chf_number	the desired number of the Cuckoo hash functions
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 *
 * @return	If the benchmark could be successfully started,
 * 		zero (0) is returned.
 * 		Otherwise, a positive error number is returned.
 */
int benchmark_shti (FILE *stream,
		const int algorithm,
		const int variation,
		const int benchmark,
		const int traversal_type,
		const int requested_verbosity_level,
		const int crt_type,
		const size_t chf_number,
		text_file_sliding_window *tfsw) {
	suffix_tree_sliding_window_shti stsw = {.crt_type = crt_type,
		.chf_number = chf_number};
	switch (algorithm) {
		case 1:
			if ((variation == 0) || (variation == 1)) {
				stsw_shti_create_ukkonen(stream, benchmark,
						variation, traversal_type,
						requested_verbosity_level,
						tfsw, &stsw);
			} else {
				fprintf(stderr, "Unknown value for "
						"the selected algorithm "
						"variation (%d)\n",
						variation);
				return (1);
			}
			break;
		default:
			fprintf(stderr, "Unknown value for the selected "
					"algorithm (%d)\n", algorithm);
			return (2);
	}
	stsw_shti_delete(requested_verbosity_level, &stsw);
	return (0);
}

/* the main function */

/**
 * The main function.
 * It executes the desired benchmark of the specified
 * construction algorithm for the suffix tree over a sliding window.
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
	 * According to the C specification (6.7.8 Initialization,
	 * paragraph 21), the non-listed members
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
	text_file_sliding_window tfsw = {.blocks_read = 0};
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
	 * The requested level of verbosity of the information collected
	 * and displayed to the user during the suffix tree construction
	 * and maintenance.
	 * The available values are:
	 * 	0 - low verbosity (the default)
	 * 		no statistics are collected
	 * 		very little information is displayed
	 * 	1 - medium verbosity
	 * 		no statistics are collected
	 * 		more information is displayed
	 * 	2 - high verbosity
	 * 		some statistics are collected
	 * 		more information including the statistics are displayed
	 * By default, we do not want to collect any statistics,
	 * because it slows down the suffix tree construction
	 * and maintenance itself. We also do not want to display
	 * more information than what can safely fit on the standard,
	 * 80 by 25 characters terminal.
	 * That's why we have chosen the low verbosity
	 * to be the default verbosity level.
	 */
	long int verbosity_level = 0;
	/*
	 * the desired collision resolution technique used when hashing
	 * available values:	1 - Cuckoo hashing
	 * 			2 - double hashing
	 */
	int crt_type = 0;
	/*
	 * the desired edge label maintenance method to use
	 * available values:	1 - Batch update by M. Senft
	 * 			2 - Fiala's and Greene's method
	 * 			3 - Larsson's method
	 */
	int elm_method = 0;
	/* the desired number of Cuckoo hash functions */
	size_t chf_number = 0;
	/* the desired size of a single block in the sliding window */
	size_t sw_block_size = 0;
	/* the desired active part scale factor */
	size_t ap_scale_factor = 0;
	/* the desired sliding window scale factor */
	size_t sw_scale_factor = 0;
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
	FILE *stream = stdout;
	printf("Benchmark of the suffix tree construction algorithms,\n"
			"which use the sliding window.\n\n");
	printf("Compile-time options:\n"
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
			"character_type is wchar_t\n"
#else
			"character_type is char\n"
#endif
#ifdef	STSW_USE_PTHREAD
			"POSIX threads are enabled\n"
#else
			"POSIX threads are disabled\n"
#endif
	"\n");
	if (argc == 1) {
		print_usage(argv[0]);
		return (EXIT_SUCCESS);
	}
	/* parsing the command line options */
	while ((getopt_retval = getopt(argc, argv,
					"t:a:b:r:c:m:sd:e:i:k:A:S:v:h")) !=
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
				} else {
					fprintf(stderr, "Unrecognized "
						"argument for the -t "
						"parameter!\n\n");
					print_usage(argv[0]);
					return (EXIT_FAILURE);
				}
				break;
			case 'a':
				if (optarg[0] == 'U') {
					algorithm = 1;
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
					return (EXIT_FAILURE);
				}
				break;
			case 'm':
				if (optarg[0] == 'B') {
					elm_method = 1;
				} else if (optarg[0] == 'F') {
					elm_method = 2;
				} else if (optarg[0] == 'L') {
					elm_method = 3;
				} else {
					fprintf(stderr, "Unrecognized "
						"argument for the -m "
						"parameter!\n\n");
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
			case 'k':
				sw_block_size = strtoul(optarg, &endptr, 0);
				if ((*endptr) != '\0') {
					fprintf(stderr, "Unrecognized "
						"argument for the -k "
						"parameter!\n\n");
					return (EXIT_FAILURE);
				}
				if (errno != 0) {
					perror("strtoul(sw_block_size)");
					return (EXIT_FAILURE);
				}
				break;
			case 'A':
				ap_scale_factor = strtoul(optarg, &endptr, 0);
				if ((*endptr) != '\0') {
					fprintf(stderr, "Unrecognized "
						"argument for the -A "
						"parameter!\n\n");
					return (EXIT_FAILURE);
				}
				if (errno != 0) {
					perror("strtoul(ap_scale_factor)");
					return (EXIT_FAILURE);
				}
				break;
			case 'S':
				sw_scale_factor = strtoul(optarg, &endptr, 0);
				if ((*endptr) != '\0') {
					fprintf(stderr, "Unrecognized "
						"argument for the -S "
						"parameter!\n\n");
					return (EXIT_FAILURE);
				}
				if (errno != 0) {
					perror("strtoul(sw_scale_factor)");
					return (EXIT_FAILURE);
				}
				break;
			case 'v':
				verbosity_level = strtol(optarg, &endptr, 0);
				if ((*endptr) != '\0') {
					fprintf(stderr, "Unrecognized "
						"argument for the -v "
						"parameter!\n\n");
					return (EXIT_FAILURE);
				}
				if (errno != 0) {
					perror("strtol(verbosity_level)");
					return (EXIT_FAILURE);
				}
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
	if (dump_filename != NULL) {
		/* if we got here, benchmark must be set to 2 */
		stream = fopen(dump_filename, "w");
		if (stream == NULL) {
			perror("fopen(stream)");
			return (EXIT_FAILURE);
		}
	}
	if (text_file_open((const int)(verbosity_level),
				input_filename, input_file_encoding,
				&internal_text_encoding,
				sw_block_size, ap_scale_factor,
				sw_scale_factor, elm_method, &tfsw) > 0) {
		fprintf(stderr, "text_file_open: The function call "
				"has failed!\n");
		return (EXIT_FAILURE);
	}
	/* random number generator initialization */
	srandom((unsigned int)(time(NULL)));
	if (type == 1) {
		benchmark_slli(stream, algorithm, variation,
				benchmark, traversal_type,
				(const int)(verbosity_level), &tfsw);
	} else if (type == 2) {
		benchmark_shti(stream, algorithm, variation,
				benchmark, traversal_type,
				(const int)(verbosity_level),
				crt_type, chf_number, &tfsw);
	} else {
		fprintf(stderr, "Error: Unknown implementation type (%d)\n",
				type);
		return (EXIT_FAILURE);
	}
	if (text_file_close((const int)(verbosity_level), &tfsw) > 0) {
		fprintf(stderr, "text_file_close: The function call "
				"has failed!\n");
		return (EXIT_FAILURE);
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
	return (EXIT_SUCCESS);
}
