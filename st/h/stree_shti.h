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
 * SHTI functions declarations.
 * This file contains the declarations of the functions,
 * which are used for the construction of the suffix tree
 * in the memory using the implementation type SHTI.
 */
#ifndef	SUFFIX_TREE_SHTI_HEADER
#define	SUFFIX_TREE_SHTI_HEADER

#include "stree_shti_common.h"

/* handling functions */

int st_shti_create_simple_mccreight (const character_type *text,
		size_t length,
		suffix_tree_shti *stree);
int st_shti_create_mccreight (const character_type *text,
		size_t length,
		suffix_tree_shti *stree);
int st_shti_create_simple_ukkonen (const character_type *text,
		size_t length,
		suffix_tree_shti *stree);
int st_shti_create_ukkonen (const character_type *text,
		size_t length,
		suffix_tree_shti *stree);

#endif /* SUFFIX_TREE_SHTI_HEADER */
