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
 * SLLI_BP functions declarations.
 * This file contains the declarations of the functions,
 * which are used for the construction of the suffix tree
 * in the memory using the implementation type SLLI
 * with the backward pointers.
 */
#ifndef	SUFFIX_TREE_SLLI_BP_HEADER
#define	SUFFIX_TREE_SLLI_BP_HEADER

#include "stree_slli_bp_common.h"

/* handling functions */

int st_slli_bp_create_mccreight (const character_type *text,
		size_t length,
		suffix_tree_slli_bp *stree);
int st_slli_bp_create_ukkonen (const character_type *text,
		size_t length,
		suffix_tree_slli_bp *stree);

#endif /* SUFFIX_TREE_SLLI_BP_HEADER */
