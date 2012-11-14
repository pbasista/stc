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
 * The very common hash table-related declarations.
 * This file contains the very common declarations,
 * which are used by the functions which use the hash table
 * in both the in-memory suffix tree construction
 * as well as in the suffix tree over a sliding window
 * construction and maintenance.
 */
#ifndef	SUFFIX_TREE_HASH_TABLE_COMMON_HEADER
#define	SUFFIX_TREE_HASH_TABLE_COMMON_HEADER

#include "suffix_tree_common.h"

/* struct typedefs */

/**
 * A struct which stores the hash settings.
 */
typedef struct hash_settings_struct {
	/** the type of the collision resolution technique to use */
	int crt_type;
	/** the number of different values for the primary hash function */
	unsigned_integral_type phf_max;
	/** the number of different values for the secondary hash function */
	unsigned_integral_type shf_max;
	/** the number of the Cuckoo hash functions */
	size_t chf_number;
	/** the next prime following the size of the universum */
	unsigned_integral_type npu_size;
	/** the "a" parameters chosen for the Cuckoo hash functions */
	unsigned_integral_type *chf_as;
	/** the "b" parameters chosen for the Cuckoo hash functions */
	unsigned_integral_type *chf_bs;
	/** the starting offsets of the Cuckoo hashing partitions */
	size_t *cp_offsets;
	/** the sizes of the Cuckoo hashing partitions */
	size_t *cp_sizes;
	/** the total size of the memory allocated for the hash settings */
	size_t allocated_size;
} hash_settings;

/**
 * A struct containing the information about a single edge
 * in every implementation type.
 */
typedef struct edge_record_struct {
	/** the source node of this edge */
	signed_integral_type source_node;
	/** the target node of this edge */
	signed_integral_type target_node;
} edge_record;

/* hashing-related supporting functions */

int hs_update (const int verbosity_level,
		size_t *new_size,
		hash_settings *hs);
int hs_deallocate (hash_settings *hs);

int er_empty (const edge_record er);
int er_vacant (const edge_record er);

int ht_dump (FILE *stream,
		unsigned_integral_type tedge_size,
		const edge_record *tedge);

size_t primary_hf (signed_integral_type source_node,
		character_type letter,
		const hash_settings *hs);
size_t secondary_hf (signed_integral_type source_node,
		character_type letter,
		const hash_settings *hs);
size_t cuckoo_hf (size_t index,
		signed_integral_type source_node,
		character_type letter,
		const hash_settings *hs);

#endif /* SUFFIX_TREE_HASH_TABLE_COMMON_HEADER */
