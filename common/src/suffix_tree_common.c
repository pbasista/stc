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
 * The implementation of the very common suffix tree functions.
 * This file contains the implementation of the very common functions,
 * which are used by the other functions for the construction
 * and maintenance of both the in-memory suffix tree
 * as well as of the suffix tree over a sliding window.
 */
#include "suffix_tree_common.h"

#include <errno.h>
#include <iconv.h>
#include <limits.h>
#include <stdio.h>

/* constants */

/**
 * the terminating character ($)
 *
 * we would like it to be greater than any other regular character
 */
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
const character_type terminating_character = WCHAR_MAX;
#else
const character_type terminating_character = CHAR_MAX;
#endif

/**
 * The size of the data type 'character_type'.
 */
const size_t character_type_size = sizeof (character_type);

/**
 * The constant representing the detailed traversal type.
 * This type of suffix tree traversal prints
 * the branching node numbers, the leaf numbers and possibly
 * the suffix link targets, if they are present.
 */
const int tt_detailed = 1;

/**
 * The constant representing the simple traversal type.
 * This type of suffix tree traversal omits
 * the branching node numbers as well as the suffix link targets,
 * but prints the leaf numbers.
 */
const int tt_simple = 2;

/* common helping functions */

/**
 * A function which prints the byte count in human readable format.
 *
 * @param
 * stream	the FILE * type stream to which the human readable size
 * 		will be printed
 * @param
 * size		the actual size that we would like to print
 * 		in the human readable format
 *
 * @return	this function always returns zero (0)
 */
int print_human_readable_size (FILE *stream, size_t size) {
	static const char *prefixes = " KMGTPEZY";
	size_t order = 0;
	size_t hr_size = size; /* human readable size */
	size_t hr_fraction = 0; /* the fraction used in human readable size */
	double double_fraction = 0;
	char prefix = '\0';
	while (hr_size > 1023) {
		hr_size = hr_size >> 10; /* hr_size / 1024 */
		++order;
	}
	prefix = prefixes[order];
	order = (size_t)(1) << (10 * order);
	hr_fraction = size % order;
	double_fraction = (double)(hr_fraction);
	while (order > 1024) {
		/* shortening the fractional part */
		double_fraction = double_fraction / 1024;
		order = order >> 10; /* order / 1024 */
	}
	/* 1024 to 1000 correction and rounding */
	hr_fraction = (size_t)(0.5 + double_fraction * 0.9765625);
	if (hr_fraction == 1000) {
		++hr_size;
		hr_fraction = 0;
	}
	if (prefix == ' ') {
		/* hr_fraction == 0 */
		fprintf(stream, "%zu B", hr_size);
	} else {
		fprintf(stream, "%zu.%03zu %ciB", hr_size, hr_fraction,
				prefix);
	}
	return (0);
}

/**
 * A function which prints the number of milliseconds in human readable format.
 *
 * @param
 * stream	the (FILE *) type stream to which the human readable time
 * 		will be printed
 * @param
 * time		the number of milliseconds that we would like to print
 * 		in the human readable format
 *
 * @return	this function always returns zero (0)
 */
int print_human_readable_time (FILE *stream, size_t time) {
	size_t hr_ms = time % 1000; /* human readable number of milliseconds */
	size_t hr_s = time / 1000; /* human readable number of seconds */
	size_t hr_m = hr_s / 60; /* human readable number of minutes */
	size_t hr_h = hr_m / 60; /* human readable number of hours */
	hr_m = hr_m % 60;
	hr_s = hr_s % 60;
	if (hr_h != 0) {
		fprintf(stream, "%zu hours, ", hr_h);
	}
	if (hr_m != 0) {
		fprintf(stream, "%zu minutes, ", hr_m);
	}
	if (hr_s != 0) {
		hr_ms = (hr_ms + 5) / 10; /* lowering the precision a little */
		fprintf(stream, "%zu.%02zu seconds (%zu ms)", hr_s, hr_ms,
				time);
	} else {
		fprintf(stream, "%zu milliseconds (%zu ms)", hr_ms, time);
	}
	return (0);
}
