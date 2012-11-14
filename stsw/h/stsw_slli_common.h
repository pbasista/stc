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
 * which are used by the functions, which construct and maintain
 * the suffix tree over a sliding window
 * using the implementation type SLLI with backward pointers.
 */
#ifndef	SUFFIX_TREE_SLIDING_WINDOW_SLLI_COMMON_HEADER
#define	SUFFIX_TREE_SLIDING_WINDOW_SLLI_COMMON_HEADER

#include "stsw_common.h"

/* struct typedefs */

/* suffix tree structs */

/**
 * A struct containing the information about a single leaf node
 * in the implementation type SLLI with backward pointers.
 */
typedef struct leaf_record_slli_struct {
	/**
	 * The parent of this leaf node.
	 * As the parent is always a branching node,
	 * it is clear that its sign is positive.
	 * By using the signed integral type, we need not
	 * to worry about the type-casts, because all the variables
	 * containing the suffix tree nodes are of the same data type.
	 */
	signed_integral_type parent;
	/** the next brother of this leaf node */
	signed_integral_type next_brother;
} leaf_record_slli;

/**
 * A struct containing the information about a single branching node
 * in the implementation type SLLI with backward pointers.
 */
typedef struct branch_record_slli_struct {
	/**
	 * The parent of this branching node.
	 * As the parent is always a branching node,
	 * it is clear that its sign is positive. That would imply
	 * that there is no point in having a signed integral type
	 * for the parent. But there is another reason why we use it.
	 * At first, by using the signed integral type,
	 * we need not to worry about the type-casts,
	 * because all the variables containing the suffix tree nodes
	 * are of the same data type.
	 * But more importantly, the unused sign enables us to store
	 * additional one bit of information per node. And we take
	 * a great advantage of it when performing the edge label maintenance.
	 * We will use this extra bit as a credit counter for each node.
	 */
	signed_integral_type parent;
	/** the first child of this branching node */
	signed_integral_type first_child;
	/** the next brother of this branching node */
	signed_integral_type branch_brother;
	/** the depth in the suffix tree of this branching node */
	unsigned_integral_type depth;
	/** the head position of this branching node */
	unsigned_integral_type head_position;
	/** the target of a suffix link from this branching node */
	signed_integral_type suffix_link;
} branch_record_slli;

/**
 * A struct containing the whole suffix tree
 * in the implementation type SLLI with backward pointers.
 */
typedef struct suffix_tree_sliding_window_slli_struct {
	/** leaf record size */
	size_t lr_size;
	/** branch record size */
	size_t br_size;
	/** the array of leaf structs for the leaf nodes */
	leaf_record_slli *tleaf;
	/** the array of branching structs for the branching nodes */
	branch_record_slli *tbranch;
	/**
	 * the array of indices of branching records
	 * deleted from the table tbranch and currently vacant
	 */
	unsigned_integral_type *tbranch_deleted;
	/** the number of available leaf records */
	size_t tleaf_size;
	/**
	 * the index to the table tleaf of the first (the deepest) leaf
	 * in the current sliding window
	 */
	size_t tleaf_first;
	/**
	 * the index to the table tleaf of the last (the shallowest) leaf
	 * in the current sliding window
	 */
	size_t tleaf_last;
	/** the number of currently used branching nodes */
	size_t branching_nodes;
	/** the current number of available branching records */
	size_t tbranch_size;
	/**
	 * the amount by which the number of branching records
	 * will be increased in case all of them are used
	 */
	size_t tbsize_increase;
	/**
	 * the current number of branching records
	 * deleted from the table tbranch and currently vacant
	 */
	size_t tbdeleted_records;
	/**
	 * the maximum index of the branching record
	 * deleted at some point from the table tbranch
	 */
	size_t max_tbdeleted_index;
	/**
	 * the current size of the array of indices of the branching records
	 * deleted from the table tbranch and currently vacant
	 */
	size_t tbdeleted_size;
} suffix_tree_sliding_window_slli;

/* allocation functions */

int stsw_slli_allocate (const int verbosity_level,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw);
int stsw_slli_reallocate (const int verbosity_level,
		size_t desired_tbranch_size,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw);

/* supporting functions */

int stsw_slli_get_leafs_depth (signed_integral_type leaf,
		size_t *leafs_depth,
		const suffix_tree_sliding_window_slli *stsw);
size_t stsw_slli_get_leafs_sw_offset (signed_integral_type leaf,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_edge_letter (signed_integral_type source_node,
		character_type *letter,
		signed_integral_type target_node,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_edge_depthscan (signed_integral_type child,
		unsigned_integral_type target_depth,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_edge_fastscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_edge_slowscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		signed_integral_type *last_match_position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);

int stsw_slli_quick_next_child (signed_integral_type *child,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_next_child (signed_integral_type *child,
		signed_integral_type *prev_child,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_edge_climb (signed_integral_type *parent,
		signed_integral_type *child,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_quick_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		size_t position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_get_prev_child (signed_integral_type parent,
		signed_integral_type desired_child,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		const suffix_tree_sliding_window_slli *stsw);

int stsw_slli_go_down (signed_integral_type grandpa,
		signed_integral_type *parent,
		unsigned_integral_type target_depth,
		size_t *position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_go_up (signed_integral_type *parent,
		signed_integral_type *child,
		unsigned_integral_type target_depth,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_create_leaf (signed_integral_type parent,
		signed_integral_type child,
		signed_integral_type prev_child,
		signed_integral_type new_leaf,
		suffix_tree_sliding_window_slli *stsw);
int stsw_slli_split_edge (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		signed_integral_type last_match_position,
		unsigned_integral_type new_head_position,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw);

int stsw_slli_simple_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);

/* handling functions */

int stsw_slli_traverse (const int verbosity_level,
		FILE *stream,
		int traversal_type,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw);
int stsw_slli_delete (const int verbosity_level,
		suffix_tree_sliding_window_slli *stsw);

#endif /* SUFFIX_TREE_SLIDING_WINDOW_SLLI_COMMON_HEADER */
