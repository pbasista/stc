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
 * SLLI common declarations.
 * This file contains the declarations of the auxiliary functions,
 * which are used by the functions, which construct
 * the suffix tree in the memory using the implementation type SLLI.
 */
#ifndef	SUFFIX_TREE_SLLI_COMMON_HEADER
#define	SUFFIX_TREE_SLLI_COMMON_HEADER

#include "stree_common.h"

/* struct typedefs */

/* suffix tree structs */

/**
 * A struct containing the information about a single leaf node
 * in the implementation type SLLI.
 */
typedef struct leaf_record_slli_struct {
	/** the next brother of this leaf node */
	signed_integral_type next_brother;
} leaf_record_slli;

/**
 * A struct containing the information about a single branching node
 * in the implementation type SLLI.
 */
typedef struct branch_record_slli_struct {
	/** the first child of this branching node */
	signed_integral_type first_child;
	/** the next brother of this branching node */
	signed_integral_type branch_brother;
	/** the target of a suffix link from this branching node */
	signed_integral_type suffix_link;
	/** the depth in the suffix tree of this branching node */
	unsigned_integral_type depth;
	/** the head position of this branching node */
	unsigned_integral_type head_position;
} branch_record_slli;

/**
 * A struct containing the whole suffix tree
 * in the implementation type SLLI.
 */
typedef struct suffix_tree_slli_struct {
	/** leaf record size */
	size_t lr_size;
	/** branch record size */
	size_t br_size;
	/** the array of leaf structs for the leaf nodes */
	leaf_record_slli *tleaf;
	/** the array of branching structs for the branching nodes */
	branch_record_slli *tbranch;
	/** the number of currently used branching nodes */
	size_t branching_nodes;
	/** the current number of available branching records */
	size_t tbranch_size;
	/**
	 * the amount by which the number of branching records
	 * will be increased in case all of them are used
	 */
	size_t tbsize_increase;
} suffix_tree_slli;

/* allocation functions */

int st_slli_allocate (size_t length,
		suffix_tree_slli *stree);
int st_slli_reallocate (size_t desired_size,
		size_t length,
		suffix_tree_slli *stree);

/* supporting functions */

int st_slli_edge_depthscan (signed_integral_type child,
		unsigned_integral_type target_depth,
		size_t ef_length,
		const suffix_tree_slli *stree);
int st_slli_edge_fastscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		const character_type *text,
		const suffix_tree_slli *stree);
int st_slli_edge_slowscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		signed_integral_type *last_match_position,
		const character_type *text,
		size_t length,
		const suffix_tree_slli *stree);

int st_slli_quick_next_child (signed_integral_type *child,
		const suffix_tree_slli *stree);
int st_slli_next_child (signed_integral_type *child,
		signed_integral_type *prev_child,
		const suffix_tree_slli *stree);
int st_slli_quick_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		size_t *position,
		size_t ef_length,
		const suffix_tree_slli *stree);
int st_slli_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		size_t ef_length,
		const suffix_tree_slli *stree);
int st_slli_quick_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		size_t position,
		const character_type *text,
		const suffix_tree_slli *stree);
int st_slli_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t position,
		const character_type *text,
		const suffix_tree_slli *stree);

int st_slli_go_down (signed_integral_type grandpa,
		signed_integral_type *parent,
		unsigned_integral_type target_depth,
		size_t *position,
		const character_type *text,
		size_t ef_length,
		const suffix_tree_slli *stree);
int st_slli_create_leaf (signed_integral_type parent,
		signed_integral_type child,
		signed_integral_type prev_child,
		signed_integral_type new_leaf,
		suffix_tree_slli *stree);
int st_slli_split_edge (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		signed_integral_type last_match_position,
		unsigned_integral_type new_head_position,
		suffix_tree_slli *stree);

/* handling functions */

int st_slli_traverse (FILE *stream,
		const char *internal_text_encoding,
		int traversal_type,
		const character_type *text,
		size_t length,
		const suffix_tree_slli *stree);
int st_slli_delete (suffix_tree_slli *stree);

#endif /* SUFFIX_TREE_SLLI_COMMON_HEADER */
