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
 * SLLI declarations.
 * This file contains the declarations of the functions,
 * which are used for the construction and maintenance
 * of the suffix tree over a sliding window
 * using the implementation type SLLI with backward pointers.
 */
#ifndef	SUFFIX_TREE_SLIDING_WINDOW_SLLI_HEADER
#define	SUFFIX_TREE_SLIDING_WINDOW_SLLI_HEADER

#include "stsw_slli_sw.h"

/* handling functions */

int stsw_slli_create_ukkonen (FILE *stream,
		const int benchmark,
		const int variation,
		const int traversal_type,
		const int collect_stats,
		text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw);

#endif /* SUFFIX_TREE_SLIDING_WINDOW_SLLI_HEADER */
