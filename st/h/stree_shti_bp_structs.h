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
 * SHTI_BP basic structs declarations.
 * This file contains the declarations of the basic structs,
 * which are used by the functions, which construct
 * the suffix tree in the memory using the implementation type SHTI
 * with the backward pointers.
 */
#ifndef	SUFFIX_TREE_SHTI_BP_STRUCTS_HEADER
#define	SUFFIX_TREE_SHTI_BP_STRUCTS_HEADER

#include "suffix_tree_hash_table_common.h"
#include "stree_common.h"

/* struct typedefs */

/* suffix tree structs */

/**
 * A struct containing the information about a single leaf node
 * in the implementation type SHTI with backward pointers.
 */
typedef struct leaf_record_shti_bp_struct {
	/** the parent of this leaf node */
	signed_integral_type parent;
} leaf_record_shti_bp;

/**
 * A struct containing the information about a single branching node
 * in the implementation type SHTI with backward pointers.
 */
typedef struct branch_record_shti_bp_struct {
	/** the parent of this branching node */
	signed_integral_type parent;
	/** the depth in the suffix tree of this branching node */
	unsigned_integral_type depth;
	/** the head position of this branching node */
	unsigned_integral_type head_position;
	/** the target of a suffix link from this branching node */
	signed_integral_type suffix_link;
} branch_record_shti_bp;

/**
 * A struct containing the whole suffix tree
 * in the implementation type SHTI with backward pointers.
 */
typedef struct suffix_tree_shti_bp_struct {
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
	leaf_record_shti_bp *tleaf;
	/** the array of edge structs, used as a hash table */
	edge_record *tedge;
	/** the array of branching structs for the branching nodes */
	branch_record_shti_bp *tbranch;
	/** the current number of edges */
	size_t edges;
	/** the current size of the edge table */
	size_t tedge_size;
	/**
	 * the amount by which the size of the edge table will be increased
	 * in case its load factor exceeds the maximum allowed value
	 */
	size_t tesize_increase;
	/** the number of currently used branching nodes */
	size_t branching_nodes;
	/** the current number of available branching records */
	size_t tbranch_size;
	/**
	 * the amount by which the number of branching records
	 * will be increased in case all of them are used
	 */
	size_t tbsize_increase;
} suffix_tree_shti_bp;

#endif /* SUFFIX_TREE_SHTI_BP_STRUCTS_HEADER */
