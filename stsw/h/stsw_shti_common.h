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
 * SHTI common functions declarations.
 * This file contains the declarations of the auxiliary functions,
 * which are used by the functions, which construct and maintain
 * the suffix tree over a sliding window
 * using the implementation type SHTI with backward pointers.
 */
#ifndef	SUFFIX_TREE_SLIDING_WINDOW_SHTI_COMMON_HEADER
#define	SUFFIX_TREE_SLIDING_WINDOW_SHTI_COMMON_HEADER

#include "stsw_shti_ht.h"

/* allocation functions */

int stsw_shti_allocate (const int verbosity_level,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);
int stsw_shti_reallocate (const int verbosity_level,
		size_t desired_tbranch_size,
		size_t desired_tedge_size,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);

/* supporting functions */

int stsw_shti_edge_depthscan (signed_integral_type child,
		unsigned_integral_type target_depth,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_edge_slowscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		signed_integral_type *last_match_position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);

int stsw_shti_quick_next_child (signed_integral_type parent,
		signed_integral_type *child,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_next_child (signed_integral_type parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_edge_climb (signed_integral_type *parent,
		signed_integral_type *child,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_quick_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		size_t *position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);

int stsw_shti_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		size_t position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);

int stsw_shti_go_down (signed_integral_type grandpa,
		signed_integral_type *parent,
		unsigned_integral_type target_depth,
		size_t *position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_go_up (signed_integral_type *parent,
		signed_integral_type *child,
		unsigned_integral_type target_depth,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_create_leaf (signed_integral_type parent,
		character_type letter,
		signed_integral_type new_leaf,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);
int stsw_shti_split_edge (signed_integral_type *parent,
		character_type letter,
		signed_integral_type *child,
		size_t *position,
		signed_integral_type last_match_position,
		unsigned_integral_type new_head_position,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);

int stsw_shti_simple_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);

/* handling functions */

int stsw_shti_traverse (const int verbosity_level,
		FILE *stream,
		int traversal_type,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);
int stsw_shti_delete (const int verbosity_level,
		suffix_tree_sliding_window_shti *stsw);

#endif /* SUFFIX_TREE_SLIDING_WINDOW_SHTI_COMMON_HEADER */
