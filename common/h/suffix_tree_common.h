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
 * The very common suffix tree declarations.
 * This file contains the very common declarations, which are used
 * by the functions for the construction and maintenance
 * of both the in-memory suffix tree
 * as well as of the suffix tree over a sliding window.
 */
#ifndef	SUFFIX_TREE_VERY_COMMON_HEADER
#define	SUFFIX_TREE_VERY_COMMON_HEADER

#include <iconv.h>
#include <stdio.h>

/*
 * By default, we want to have standard (short) characters.
 *
 * If you want to use alphabets of large sizes
 * (up to the number of characters in Unicode),
 * please, define the following macro:
 *
 * #define	SUFFIX_TREE_TEXT_WIDE_CHAR
 *
 * Note that when using wide characters for the internal representation
 * of the text, the memory usage will usually increase significantly
 * compared to the situation when standard characters are used.
 * Moreover, the hash table based implementation technique will perform
 * very poorly in suffix tree traversal, because of very large
 * character ranges, which need to be scanned in order to determine
 * all the children of a single branching node.
 */

/* #define	SUFFIX_TREE_TEXT_WIDE_CHAR */

#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
#include <wchar.h>
#endif

/* simple typedefs */

/** the character type typedef */

#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
/** (wide character) */
typedef wchar_t character_type;
#else
/** (standard character) */
typedef char character_type;
#endif

/**
 * The typedef for a type used mainly as an index to the text
 * or the hash table. It is also used for the length, depth,
 * head position, size and possibly some other nonnegative values.
 */
typedef unsigned int unsigned_integral_type;

/**
 * The typedef for a type used almost exclusively as an identification
 * of the nodes in the suffix tree. The branching nodes are represented
 * by positive numbers while the leaf nodes are represented
 * by negative numbers. The value of zero is invalid and is used
 * to indicate a nonexisting node.
 */
typedef int signed_integral_type;

/* constants */

/* the terminating character ($) */

extern const character_type terminating_character;

/* the size of the data type 'character_type' */

extern const size_t character_type_size;

/* the detailed traversal type */

extern const int tt_detailed;

/* the simple traversal type */

extern const int tt_simple;

/* common helping functions */

int print_human_readable_size (FILE *stream, size_t size);
int print_human_readable_time (FILE *stream, size_t time);

#endif /* SUFFIX_TREE_VERY_COMMON_HEADER */
