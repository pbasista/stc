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
 * SLAI common declarations.
 * This file contains the declarations of the auxiliary functions,
 * which are used by the functions, which construct
 * the suffix tree in the memory using the implementation type SLAI.
 */
#ifndef	SUFFIX_TREE_SLAI_COMMON_HEADER
#define	SUFFIX_TREE_SLAI_COMMON_HEADER

#include "pwotd_cdata.h"

/* constants */

/* the leaf node bit flag */

extern const unsigned_integral_type leaf_node;

/* the rightmost child bit flag */

extern const unsigned_integral_type rightmost_child;

/* struct typedefs */

/**
 * A struct containing the whole suffix tree
 * in the implementation type SLAI.
 */
typedef struct suffix_tree_slai_struct {
	/**
	 * The linear array of simple values which hold all the information
	 * about every node in the suffix tree.
	 *
	 * The branching node is described by two values, of which the first
	 * contains an index to the text of the beginning of the label
	 * of an edge which ends at this branching node.
	 * The second value contains the position in the table tnode
	 * of the first child of this branching node.
	 *
	 * The leaf node is represented by only one value,
	 * which holds an index to the text of the starting position
	 * of the label of an edge which ends at this leaf node.
	 */
	unsigned_integral_type *tnode;
	/**
	 * the number of branching nodes
	 * currently present in the suffix tree
	 */
	size_t branching_nodes;
	/**
	 * the current index of the first empty position
	 * in the linear array (table tnode)
	 */
	size_t tnode_top;
	/** the current size of the linear array (table tnode) */
	size_t tnode_size;
	/**
	 * the amount by which the size of the linear array (table tnode)
	 * will be increased in case all of its entries are used
	 */
	size_t tnode_size_increase;
	/**
	 * the auxiliary data structures used
	 * for the suffix tree construction
	 */
	pwotd_construction_data cdata;
} suffix_tree_slai;

/* allocation functions */

int st_slai_allocate (size_t length,
		suffix_tree_slai *stree);
int st_slai_reallocate (size_t desired_tnode_size,
		size_t length,
		suffix_tree_slai *stree);

/* supporting functions */

int st_slai_dump_tnode (FILE *stream,
		const suffix_tree_slai *stree);

/* handling functions */

int st_slai_process_partition (size_t partition_index,
		size_t tnode_offset,
		size_t parents_depth,
		const character_type *text,
		size_t length,
		suffix_tree_slai *stree);

int st_slai_process_partitions_range (size_t partitions_range_begin,
		size_t partitions_range_end,
		size_t parents_depth,
		size_t tnode_offset,
		const character_type *text,
		size_t length,
		suffix_tree_slai *stree);

int st_slai_output_nodes (size_t lcp_size,
		size_t parents_depth,
		size_t range_begin,
		size_t range_end,
		size_t tnode_offset,
		const character_type *text,
		size_t length,
		suffix_tree_slai *stree);

int st_slai_empty_stack (const character_type *text,
		size_t length,
		suffix_tree_slai *stree);

int st_slai_empty_partitions_stack (const character_type *text,
		size_t length,
		suffix_tree_slai *stree);

int st_slai_traverse (FILE *stream,
		const char *internal_text_encoding,
		int traversal_type,
		const character_type *text,
		size_t length,
		const suffix_tree_slai *stree);
int st_slai_delete (suffix_tree_slai *stree);

#endif /* SUFFIX_TREE_SLAI_COMMON_HEADER */
