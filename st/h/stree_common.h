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
 * Suffix tree common declarations.
 * This file contains the common declarations, which are used
 * by the functions for the construction
 * of the suffix tree in the memory.
 */
#ifndef	SUFFIX_TREE_IN_MEMORY_COMMON_HEADER
#define	SUFFIX_TREE_IN_MEMORY_COMMON_HEADER

#include "suffix_tree_common.h"

/* constants */

/* the number of extra characters allocated for the text */

extern const size_t extra_allocated_characters;

/* reading function */

int text_read (const char *file_name,
		const char *file_encoding,
		char **internal_text_encoding,
		character_type **text,
		size_t *length);

/* printing functions */

int st_print_edge (FILE *stream,
		int print_terminating_character,
		signed_integral_type parent,
		signed_integral_type child,
		signed_integral_type childs_suffix_link,
		unsigned_integral_type parents_depth,
		unsigned_integral_type childs_depth,
		size_t log10bn,
		size_t log10l,
		size_t childs_offset,
		const char *internal_character_encoding,
		const character_type *text);
int st_print_single_suffix (FILE *stream,
		size_t suffix_index,
		const char *internal_character_encoding,
		const character_type *suffix,
		size_t suffix_length);
int st_print_stats (size_t length,
		size_t edges,
		size_t branching_nodes,
		size_t la_records,
		size_t tedge_size,
		size_t tbranch_size,
		size_t tnode_size,
		size_t leaf_record_size,
		size_t edge_record_size,
		size_t branch_record_size,
		size_t la_record_size,
		size_t extra_allocated_memory_size,
		size_t extra_used_memory_size);

#endif /* SUFFIX_TREE_IN_MEMORY_COMMON_HEADER */
