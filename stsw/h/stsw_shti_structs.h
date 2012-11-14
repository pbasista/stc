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
 * SHTI basic structs declarations.
 * This file contains the declarations of the basic structs,
 * which are used by the functions, which construct and maintain
 * the suffix tree over a sliding window
 * using the implementation type SHTI with backward pointers.
 */
#ifndef	SUFFIX_TREE_SLIDING_WINDOW_SHTI_STRUCTS_HEADER
#define	SUFFIX_TREE_SLIDING_WINDOW_SHTI_STRUCTS_HEADER

#include "suffix_tree_hash_table_common.h"

/* struct typedefs */

/* suffix tree structs */

/**
 * A struct containing the information about a single leaf node
 * in the implementation type SHTI with backward pointers.
 */
typedef struct leaf_record_shti_struct {
	/**
	 * The parent of this leaf node.
	 * As the parent is always a branching node,
	 * it is clear that its sign is positive.
	 * By using the signed integral type, we need not
	 * to worry about the type-casts, because all the variables
	 * containing the suffix tree nodes are of the same data type.
	 */
	signed_integral_type parent;
} leaf_record_shti;

/**
 * A struct containing the information about a single branching node
 * in the implementation type SHTI with backward pointers.
 */
typedef struct branch_record_shti_struct {
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
	/** the depth in the suffix tree of this branching node */
	unsigned_integral_type depth;
	/** the head position of this branching node */
	unsigned_integral_type head_position;
	/** the target of a suffix link from this branching node */
	signed_integral_type suffix_link;
	/**
	 * The number of children of this branching node.
	 * The maximum number of children a single branching node can have
	 * is at most the same as the maximum number of different characters
	 * that are present in the suffix tree at the same time.
	 * And that is always at most the same as the maximum allowed
	 * alphabet size, which is one less than the size of the data type
	 * character_type. That's why it is enough to use this data type
	 * for the number of children of a single branching node.
	 */
	character_type children;
} branch_record_shti;

/**
 * A struct containing the whole suffix tree
 * in the implementation type SHTI with backward pointers.
 */
typedef struct suffix_tree_sliding_window_shti_struct {
	/**
	 * the additional number of bytes
	 * currently allocated for this struct
	 */
	size_t additional_bytes_allocated;
	/** hash settings size */
	size_t hs_size;
	/** leaf record size */
	size_t lr_size;
	/** edge record size */
	size_t er_size;
	/** branch record size */
	size_t br_size;
	/** the desired type of the collision resolution technique to use */
	int crt_type;
	/** the desired number of the Cuckoo hash functions */
	size_t chf_number;
	/** hash settings */
	hash_settings *hs;
	/** the array of leaf structs for the leaf nodes */
	leaf_record_shti *tleaf;
	/** the array of branching structs for the branching nodes */
	branch_record_shti *tbranch;
	/**
	 * the array of indices of branching records
	 * deleted from the table tbranch and currently vacant
	 */
	unsigned_integral_type *tbranch_deleted;
	/** the array of edge structs, used as a hash table */
	edge_record *tedge;
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
	/** the current number of edges */
	size_t edges;
	/** the current size of the edge table */
	size_t tedge_size;
	/**
	 * the amount by which the size of the edge table will be increased
	 * in case its load factor exceeds the maximum allowed value
	 */
	size_t tesize_increase;
} suffix_tree_sliding_window_shti;

#endif /* SUFFIX_TREE_SLIDING_WINDOW_SHTI_STRUCTS_HEADER */
