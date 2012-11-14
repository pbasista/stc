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
 * SHTI_BP hashing-related functions declarations.
 * This file contains the declarations of the auxiliary
 * hashing-related functions, which are used by the functions,
 * which construct the suffix tree in the memory
 * using the implementation type SHTI with the backward pointers.
 */
#ifndef	SUFFIX_TREE_SHTI_BP_HASH_TABLE_HEADER
#define	SUFFIX_TREE_SHTI_BP_HASH_TABLE_HEADER

#include "stree_shti_bp_structs.h"

/* hashing-related supporting functions */

int stree_shti_bp_er_key_matches (signed_integral_type source_node,
		character_type letter,
		edge_record er,
		const character_type *text,
		const suffix_tree_shti_bp *stree);
int stree_shti_bp_er_letter (edge_record er,
		character_type *letter,
		const character_type *text,
		const suffix_tree_shti_bp *stree);
int stree_shti_bp_edge_letter (signed_integral_type source_node,
		character_type *letter,
		signed_integral_type target_node,
		const character_type *text,
		const suffix_tree_shti_bp *stree);

int stree_shti_bp_ht_rehash (size_t *new_size,
		const character_type *text,
		suffix_tree_shti_bp *stree);

/* hashing-related handling functions */

int stree_shti_bp_ht_insert (signed_integral_type source_node,
		character_type letter,
		signed_integral_type target_node,
		int rehash_allowed,
		const character_type *text,
		suffix_tree_shti_bp *stree);
int stree_shti_bp_ht_delete (signed_integral_type source_node,
		character_type letter,
		const character_type *text,
		suffix_tree_shti_bp *stree);
int stree_shti_bp_ht_lookup (signed_integral_type source_node,
		character_type letter,
		signed_integral_type *target_node,
		const character_type *text,
		const suffix_tree_shti_bp *stree);

#endif /* SUFFIX_TREE_SHTI_BP_HASH_TABLE_HEADER */
