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
 * SHTI hashing-related functions declarations.
 * This file contains the declarations of the auxiliary
 * hashing-related functions, which are used by the functions,
 * which construct and maintain the suffix tree over a sliding window
 * using the implementation type SHTI with backward pointers.
 */
#ifndef	SUFFIX_TREE_SLIDING_WINDOW_SHTI_HASH_TABLE_HEADER
#define	SUFFIX_TREE_SLIDING_WINDOW_SHTI_HASH_TABLE_HEADER

#include "stsw_common.h"
#include "stsw_shti_structs.h"

/* auxiliary functions */

int stsw_shti_get_leafs_depth (signed_integral_type leaf,
		size_t *leafs_depth,
		const suffix_tree_sliding_window_shti *stsw);
size_t stsw_shti_get_leafs_sw_offset (signed_integral_type leaf,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);

/* hashing-related supporting functions */

int stsw_shti_er_key_matches (signed_integral_type source_node,
		character_type letter,
		edge_record er,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_er_letter (edge_record er,
		character_type *letter,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_edge_letter (signed_integral_type source_node,
		character_type *letter,
		signed_integral_type target_node,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);

int stsw_shti_ht_rehash (size_t *new_size,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);

/* hashing-related handling functions */

int stsw_shti_ht_insert (signed_integral_type source_node,
		character_type letter,
		signed_integral_type target_node,
		int rehash_allowed,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);
int stsw_shti_ht_delete (signed_integral_type source_node,
		character_type letter,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);
int stsw_shti_ht_lookup (signed_integral_type source_node,
		character_type letter,
		signed_integral_type *target_node,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);

#endif /* SUFFIX_TREE_SLIDING_WINDOW_SHTI_HASH_TABLE_HEADER */
