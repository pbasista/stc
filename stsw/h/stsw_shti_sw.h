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
 * SHTI sliding window-related functions declarations.
 * This file contains the declarations of the additional
 * sliding window-related functions, which are used by the functions,
 * which construct and maintain the suffix tree over a sliding window
 * using the implementation type SHTI with backward pointers.
 */
#ifndef	SUFFIX_TREE_SLIDING_WINDOW_SHTI_SLIDING_WINDOW_HEADER
#define	SUFFIX_TREE_SLIDING_WINDOW_SHTI_SLIDING_WINDOW_HEADER

#include "stsw_shti_common.h"

/* error-checking function */

int stsw_shti_check_head_positions (const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw);

/* edge label maintenance functions */

int stsw_shti_edge_labels_batch_update (const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);
int stsw_shti_send_credit (signed_integral_type parent,
		unsigned_integral_type new_head_position,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);

/* sliding window-related handling function */

int stsw_shti_delete_longest_suffix (size_t *starting_position,
		size_t *active_index,
		signed_integral_type *active_node,
		text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw);

#endif /* SUFFIX_TREE_SLIDING_WINDOW_SHTI_SLIDING_WINDOW_HEADER */
