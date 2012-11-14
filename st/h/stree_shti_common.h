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
 * which are used by the functions, which construct the suffix tree
 * in the memory using the implementation type SHTI.
 */
#ifndef	SUFFIX_TREE_SHTI_COMMON_HEADER
#define	SUFFIX_TREE_SHTI_COMMON_HEADER

#include "stree_shti_ht.h"

/* allocation functions */

int st_shti_allocate (size_t length,
		suffix_tree_shti *stree);
int st_shti_reallocate (size_t desired_tbranch_size,
		size_t desired_tedge_size,
		const character_type *text,
		size_t length,
		suffix_tree_shti *stree);

/* supporting functions */

int st_shti_edge_depthscan (signed_integral_type child,
		unsigned_integral_type target_depth,
		size_t ef_length,
		const suffix_tree_shti *stree);
int st_shti_edge_slowscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		signed_integral_type *last_match_position,
		const character_type *text,
		size_t length,
		const suffix_tree_shti *stree);

int st_shti_quick_next_child (signed_integral_type parent,
		signed_integral_type *child,
		const character_type *text,
		const suffix_tree_shti *stree);
int st_shti_next_child (signed_integral_type parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		const character_type *text,
		const suffix_tree_shti *stree);
int st_shti_quick_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		size_t *position,
		size_t ef_length,
		const suffix_tree_shti *stree);
int st_shti_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		size_t ef_length,
		const suffix_tree_shti *stree);
int st_shti_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		size_t position,
		const character_type *text,
		const suffix_tree_shti *stree);

int st_shti_go_down (signed_integral_type grandpa,
		signed_integral_type *parent,
		unsigned_integral_type target_depth,
		size_t *position,
		const character_type *text,
		size_t ef_length,
		const suffix_tree_shti *stree);
int st_shti_create_leaf (signed_integral_type parent,
		character_type letter,
		signed_integral_type new_leaf,
		const character_type *text,
		suffix_tree_shti *stree);
int st_shti_split_edge (signed_integral_type *parent,
		character_type letter,
		signed_integral_type *child,
		size_t *position,
		signed_integral_type last_match_position,
		unsigned_integral_type new_head_position,
		const character_type *text,
		suffix_tree_shti *stree);

/* handling functions */

int st_shti_traverse (FILE *stream,
		const char *internal_text_encoding,
		int traversal_type,
		const character_type *text,
		size_t length,
		const suffix_tree_shti *stree);
int st_shti_delete (suffix_tree_shti *stree);

#endif /* SUFFIX_TREE_SHTI_COMMON_HEADER */
