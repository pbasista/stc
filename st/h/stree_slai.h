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
 * SLAI functions declarations.
 * This file contains the declarations of the functions,
 * which are used for the construction of the suffix tree
 * in the memory using the implementation type SLAI.
 */
#ifndef	SUFFIX_TREE_SLAI_HEADER
#define	SUFFIX_TREE_SLAI_HEADER

#include "stree_slai_common.h"

/* handling functions */

int st_slai_create_pwotd (long int desired_prefix_length,
		const character_type *text,
		size_t length,
		suffix_tree_slai *stree);

#endif /* SUFFIX_TREE_SLAI_HEADER */
