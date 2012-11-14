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
 * PWOTD declarations.
 * This file contains the declarations of the functions
 * related to the PWOTD algorithm, which are used by the functions,
 * which construct the suffix tree in the memory
 * using the implementation type SLAI.
 */
#ifndef	PWOTD_CONSTRUCTION_DATA_HEADER
#define	PWOTD_CONSTRUCTION_DATA_HEADER

#include "pwotd_cdata_common.h"

/* supporting functions */

int pwotd_determine_lcp (size_t *lcp_size,
		size_t range_begin,
		size_t range_end,
		const character_type *text,
		size_t length,
		const pwotd_construction_data *cdata);
int pwotd_determine_partitions_range_lcp (size_t *lcp_size,
		size_t range_begin,
		size_t range_end,
		const character_type *text,
		size_t length,
		const pwotd_construction_data *cdata);
int pwotd_determine_partitions_range_smallest_text_offset (
		size_t *text_offset,
		size_t range_begin,
		size_t range_end,
		const pwotd_construction_data *cdata);

/* handling functions */

int pwotd_initialize_suffixes (pwotd_construction_data *cdata);
int pwotd_partition_suffixes (size_t prefix_length,
		const character_type *text,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_insert_partition_tbp (size_t new_index,
		size_t new_tnode_offset,
		size_t new_parents_depth,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_partitions_stack_push (size_t new_lcp_size,
		size_t new_offset,
		size_t new_size,
		size_t new_tnode_offset,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_sort_suffixes (size_t prefix_offset,
		size_t pts_begin,
		size_t pts_end,
		const character_type *text,
		pwotd_construction_data *cdata);
int pwotd_insert_partition (size_t begin_offset,
		size_t end_offset,
		size_t lcp_size,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_select_partition (size_t partition_index,
		pwotd_construction_data *cdata);

#endif /* PWOTD_CONSTRUCTION_DATA_HEADER */
