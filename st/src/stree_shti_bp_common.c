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
 * SHTI_BP common functions implementation.
 * This file contains the implementation of the auxiliary functions,
 * which are used by the functions, which construct
 * the suffix tree in the memory using the implementation type SHTI
 * with the backward pointers.
 */
#include "stree_shti_bp_common.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* allocation functions */

/**
 * A function which allocates memory for the suffix tree.
 *
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	On successful allocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int st_shti_bp_allocate (size_t length,
		suffix_tree_shti_bp *stree) {
	/*
	 * the future size of the table tbranch (the first estimation,
	 * which will be further adjusted)
	 */
	size_t unit_size = (size_t)(1) <<
		(sizeof (size_t) * 8 - 1);
	size_t allocated_size = 0;
	printf("==============================================\n"
		"Trying to allocate memory for the suffix tree:\n\n");
	/* we need to fill in the size of the hash settings */
	stree->hs_size =  sizeof (hash_settings);
	/* we need to fill in the size of the leaf record */
	stree->lr_size =  sizeof (leaf_record_shti_bp);
	/* we need to fill in the size of the edge record */
	stree->er_size =  sizeof (edge_record);
	/* we need to fill in the size of the branch record */
	stree->br_size =  sizeof (branch_record_shti_bp);
	if ((hs_deallocate(stree->hs)) > 0) {
		fprintf(stderr, "Error: Could not deallocate "
				"the hash settings!\n");
		return (1);
	}
	printf("Trying to allocate memory for the hash settings:\n"
		"%zu cells of 1 bytes (totalling %zu bytes, ",
		stree->hs_size, stree->hs_size);
	print_human_readable_size(stdout,
			stree->hs_size);
	printf(").\n");
	/*
	 * We allocate and clear the memory for the hash settings.
	 * To achieve it, we simply use calloc instead of malloc.
	 */
	stree->hs = calloc(stree->hs_size, (size_t)(1));
	if (stree->hs == NULL) {
		perror("calloc(stree->hs)");
		/* resetting the errno */
		errno = 0;
		return (2);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size = stree->hs_size;
	printf("Successfully allocated!\n\n");
	/* filling in some of the desired values for the hash settings */
	stree->hs->crt_type = stree->crt_type;
	stree->hs->chf_number = stree->chf_number;
	/*
	 * The number of edges will be at least the same as the number
	 * of leaves, which is "length + 1". It will also be at most
	 * the same as the maximum number of edges in a tree which has
	 * 2 * "length + 1" - 1 vertices, which is equal to 2 * "length".
	 * To be sure that all the edges fit in the hash table,
	 * we want it to have at least 2 * "length" records.
	 * We have to make this assignment before we call
	 * the hs_update function, because this function call
	 * can possibly increase the desired value.
	 */
	stree->tedge_size = 2 * length;
	/* we update the hash table size and hash settings */
	if (hs_update(0, &(stree->tedge_size), stree->hs) != 0) {
		fprintf(stderr, "Error: Can not correctly update "
				"the hash settings.\n");
		return (3);
	}
	/* the adjustment of the future size of the table tbranch */
	while (length < unit_size) {
		unit_size = unit_size >> 1; /* unit_size / 2 */
	}
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stree->tbranch);
	stree->tbranch = NULL;
	printf("Trying to allocate memory for tbranch:\n"
		"%zu cells of %zu bytes (totalling %zu bytes, ",
			unit_size + 1, stree->br_size,
			(unit_size + 1) * stree->br_size);
	print_human_readable_size(stdout,
			(unit_size + 1) * stree->br_size);
	printf(").\n");
	/*
	 * The number of actually allocated branch records is increased by one,
	 * because of the 0.th record, which is never used.
	 */
	stree->tbranch = calloc(unit_size + 1, stree->br_size);
	if (stree->tbranch == NULL) {
		perror("calloc(tbranch)");
		/* resetting the errno */
		errno = 0;
		return (4);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += (unit_size + 1) * stree->br_size;
	printf("Successfully allocated!\n\n");
	/*
	 * This memory is cleared in advance, so we need not to do
	 * the following assignments. But we do, because we would like
	 * to emphasize the correctness of the information stored
	 * for the root in the clean table tbranch.
	 *
	 * stree->tbranch[1] is the root and it has no parent
	 */
	stree->tbranch[1].parent = 0;
	/* its depth is zero (and it never changes) */
	stree->tbranch[1].depth = 0;
	/* its head position is zero (by definition) */
	stree->tbranch[1].head_position = 0;
	/* its suffix link is undefined (and can never be defined) */
	stree->tbranch[1].suffix_link = 0;
	/* So, in the beginning, we have only one branching node - the root. */
	stree->branching_nodes = 1;
	/*
	 * we store the number of the real branch records available
	 * rather than the number of branch records allocated
	 */
	stree->tbranch_size = unit_size;
	/* The first table tbranch size increase step */
	stree->tbsize_increase = unit_size >> 1; /* unit_size / 2 */
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stree->tedge);
	stree->tedge = NULL;
	printf("Trying to allocate memory for tedge:\n"
		"%zu cells of %zu bytes (totalling %zu bytes, ",
			stree->tedge_size, stree->er_size,
			stree->tedge_size * stree->er_size);
	print_human_readable_size(stdout,
			stree->tedge_size * stree->er_size);
	printf(").\n");
	/*
	 * The number of edge records has already been determined,
	 * so we just use it.
	 */
	stree->tedge = calloc(stree->tedge_size, stree->er_size);
	if (stree->tedge == NULL) {
		perror("calloc(tedge)");
		/* resetting the errno */
		errno = 0;
		return (5);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += stree->tedge_size * stree->er_size;
	printf("Successfully allocated!\n\n");
	/* the number of all edges currently present in the hash table */
	stree->edges = 0;
	/* stree->tedge_size / 2 */
	stree->tesize_increase = stree->tedge_size >> 1;
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stree->tleaf);
	stree->tleaf = NULL;
	printf("Trying to allocate memory for tleaf:\n"
		"%zu cells of %zu bytes (totalling %zu bytes, ",
			length + 2, stree->lr_size,
			(length + 2) * stree->lr_size);
	print_human_readable_size(stdout,
			(length + 2) * stree->lr_size);
	printf(").\n");
	/*
	 * The number of actually allocated leaf records is increased by two,
	 * compared to the number of the "real" characters in the text.
	 * It is because of the 0.th record (which is never used)
	 * and one extra record, representing the shortest non-empty suffix
	 * and consisting only of the terminating character ($).
	 */
	stree->tleaf = calloc((length + 2), stree->lr_size);
	if (stree->tleaf == NULL) {
		perror("calloc(tleaf)");
		/* resetting the errno */
		errno = 0;
		return (6);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += (length + 2) * stree->lr_size;
	printf("Successfully allocated!\n\n");
	printf("Total amount of memory initially allocated: %zu bytes (",
			allocated_size);
	print_human_readable_size(stdout, allocated_size);
	/* The meaning: per "real" input character */
	printf(")\nwhich is %.3f bytes per input character.\n\n",
			(double)(allocated_size) / (double)(length));
	printf("The memory has been successfully allocated!\n"
		"===========================================\n\n");
	return (0);
}

/**
 * A function which reallocates the memory for the suffix tree
 * to make it larger.
 *
 * @param
 * desired_tbranch_size	the minimum requested size of the table tbranch
 * 			(in the number of nodes, not including
 * 			the leading 0.th node)
 * 			If zero is given, no reallocation
 * 			of the table tbranch is performed.
 * @param
 * desired_tedge_size	the minimum requested size of the hash table
 * 			If zero is given, no reallocation
 * 			of the hash table is performed.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int st_shti_bp_reallocate (size_t desired_tbranch_size,
		size_t desired_tedge_size,
		const character_type *text,
		size_t length,
		suffix_tree_shti_bp *stree) {
	void *tmp_pointer = NULL;
	/*
	 * the future size of the table tbranch (the first estimation,
	 * which will be further adjusted)
	 */
	size_t new_tbranch_size = stree->tbranch_size +
		stree->tbsize_increase;
	/*
	 * the future size of the table tedge (the first estimation,
	 * which will be further adjusted)
	 */
	size_t new_tedge_size = stree->tedge_size +
		stree->tesize_increase;
	if (desired_tbranch_size > 0) {
		/*
		 * if the implicitly chosen new size of the table tbranch
		 * is too small
		 */
		if (new_tbranch_size < desired_tbranch_size) {
			/* we make it large enough */
			new_tbranch_size = desired_tbranch_size;
		}
		/*
		 * on the other hand, if the new size of the table tbranch
		 * is too large, we make it smaller, but still large enough
		 * to hold all the data the table tbranch can possibly hold
		 */
		if (new_tbranch_size > length) {
			/*
			 * the maximum number of branching nodes is one less
			 * than the maximum number of leaf nodes
			 * (which is length + 1)
			 */
			new_tbranch_size = length;
		}
		printf("Trying to reallocate the memory "
				"for the table tbranch: new size:\n"
				"%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
				new_tbranch_size + 1, stree->br_size,
				(new_tbranch_size + 1) * stree->br_size);
		print_human_readable_size(stdout,
				(new_tbranch_size + 1) * stree->br_size);
		printf(").\n");
		/*
		 * The number of actually allocated branch records
		 * is increased by one, because of the 0.th record,
		 * which is never used.
		 */
		tmp_pointer = realloc(stree->tbranch,
				(new_tbranch_size + 1) * stree->br_size);
		if (tmp_pointer == NULL) {
			perror("realloc(tbranch)");
			/* resetting the errno */
			errno = 0;
			return (1);
		} else {
			/*
			 * Despite that the call to the realloc seems
			 * to have been successful, we reset the errno,
			 * because at least on Mac OS X
			 * it might have changed.
			 */
			errno = 0;
			stree->tbranch = tmp_pointer;
		}
		printf("Successfully reallocated!\n");
		/*
		 * we store the number of the real branch records available
		 * rather than the number of branch records allocated
		 */
		stree->tbranch_size = new_tbranch_size;
		/*
		 * finally, we adjust the size increase step
		 * for the next resize of the table tbranch
		 */
		if (stree->tbsize_increase < 256) {
			/* minimum increase step */
			stree->tbsize_increase = 128;
		} else {
			/* division by 2 */
			stree->tbsize_increase = stree->tbsize_increase >> 1;
		}
	}
	if (desired_tedge_size > 0) {
		/*
		 * if the implicitly chosen new size of the hash table
		 * is too small
		 */
		if (new_tedge_size < desired_tedge_size) {
			/* we make it large enough */
			new_tedge_size = desired_tedge_size;
		}
		/*
		 * The new size of the hash table can never be "too large",
		 * because the hash table is not meant to be fully occupied,
		 * so if it is larger, it just means that we can expect less
		 * hash collisions.
		 */
		printf("Trying to reallocate the memory "
				"for the hash table: desired new size:\n"
				"%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
				new_tedge_size, stree->er_size,
				new_tedge_size * stree->er_size);
		print_human_readable_size(stdout,
				new_tedge_size * stree->er_size);
		printf(").\n");
		/*
		 * The hash table size is supposed to meet certain conditions
		 * (e.g. it should be a prime). That's why the new_tedge_size
		 * is just a recommendation on the new hash table size,
		 * which will further be adjusted by the function
		 * stree_shti_bp_ht_rehash. This function also updates
		 * the tedge_size entry of the suffix tree struct.
		 */
		if (stree_shti_bp_ht_rehash(&new_tedge_size, text, stree)
				== 0) {
			printf("Successfully reallocated! Current size:\n"
					"%zu cells of %zu bytes "
					"(totalling %zu bytes, ",
					stree->tedge_size, stree->er_size,
					stree->tedge_size * stree->er_size);
			print_human_readable_size(stdout,
					stree->tedge_size * stree->er_size);
			printf(").\n");
		} else {
			fprintf(stderr, "Error: The reallocation of the "
					"hash table has failed!\n");
			return (2);
		}
		/*
		 * finally, we adjust the size increase step
		 * for the next resize of the hash table
		 */
		if (stree->tesize_increase < 256) {
			/* minimum increase step */
			stree->tesize_increase = 128;
		} else {
			/* division by 2 */
			stree->tesize_increase = stree->tesize_increase >> 1;
		}
	}
	return (0);
}

/* supporting functions */

/**
 * A function to perform "depthscan" on a given edge with a given letter.
 *
 * Depthscan just checks whether the provided target node of an edge
 * is deep enough in the suffix tree.
 *
 * @param
 * child	the target node of an edge which will be depth-scanned
 * @param
 * target_depth	the depth which we will try to reach
 * @param
 * ef_length	the currently effective maximum length of the suffix
 * 		valid for the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If the depth of the target node of an edge equals
 * 		the desired depth, 0 is returned.
 * 		If the depth of the target node of an edge is smaller than
 * 		the desired depth, (-1) is returned.
 * 		If the depth of the target node of an edge is larger than
 * 		the desired depth, 1 is returned.
 * 		If an error occures, a positive error number
 * 		greater than 1 is returned.
 */
int st_shti_bp_edge_depthscan (signed_integral_type child,
		unsigned_integral_type target_depth,
		size_t ef_length,
		const suffix_tree_shti_bp *stree) {
	unsigned_integral_type childs_depth = 0;
	if (child == 0) {
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		return (2); /* not a valid child */
	} else if (child > 0) { /* child is a branching node */
		childs_depth = stree->tbranch[child].depth;
	} else { /* child < 0 => child is a leaf */
		/*
		 * The depth of a leaf is the total length of the suffix,
		 * which starts at the root and ends at this leaf.
		 */
		childs_depth = (unsigned_integral_type)(ef_length + 1
				- (size_t)(-child));
	}
	if (childs_depth < target_depth) {
		return (-1); /* an edge shorter than required */
	} else if (childs_depth == target_depth) {
		return (0); /* an edge of exact length */
	} else { /* childs_depth > target_depth */
		return (1); /* too long edge */
	}
}

/**
 * A function to perform "slowscan" on a given edge starting
 * at the given position in the text.
 *
 * Slowscan examines all the letters of the given edge and checks
 * whether they match the provided substring of the text.
 *
 * @param
 * parent	the node from which the given edge starts
 * @param
 * child	the node where the given edge ends
 * @param
 * position	the position in the text of the letter which will be matched
 * 		against the first letter of a given edge
 * @param
 * last_match_position	Upon returning from this function,
 * 			the value of (*last_match_position) will be overwritten
 * 			with the number of characters from the edge label
 * 			which match the text starting at the position
 * 			"position". However, the sign of this parameter
 * 			will NOT indicate the type of the first mismatch,
 * 			if any, as it did with the implementation type SL.
 * 			The reason is simple: the implementation type SH
 * 			can not utilize this information.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * ef_length	the currently effective maximum length of the suffix
 * 		valid for the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If all the letters of the provided edge match, 0 is returned.
 * 		If only some of the first edge letters match, 1 is returned.
 * 		If the number of letters to compare is smaller than the length
 * 		of the edge, but all these letters do match, 2 is returned.
 * 		If an error occures, a positive error number
 * 		greater than 2 is returned.
 */
int st_shti_bp_edge_slowscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		signed_integral_type *last_match_position,
		const character_type *text,
		size_t ef_length,
		const suffix_tree_shti_bp *stree) {
	size_t edge_letter_index = 0;
	size_t edge_letter_index_at_start = 0;
	/*
	 * the position in the text just after the last letter
	 * of the provided edge
	 */
	size_t edge_letter_index_end = 0;
	/*
	 * if this variable evaluates to true, we will be scanning
	 * (and comparing) all the letters of an edge
	 */
	int comparing_all_letters = 1;
	/* we suppose that the parent node of this edge is a branching node */
	if (child == 0) {
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		/*
		 * As there were no matching characters, we have to set
		 * the "(*last_match_position)" accordingly.
		 */
		(*last_match_position) = 0;
		return (3); /* not a valid child */
	} else if (child > 0) { /* child is a branching node */
		edge_letter_index_at_start =
			stree->tbranch[child].head_position +
			stree->tbranch[parent].depth;
		edge_letter_index_end = stree->tbranch[child].head_position +
			stree->tbranch[child].depth;
	} else { /* child < 0 => child is a leaf */
		edge_letter_index_at_start = (unsigned_integral_type)(-child) +
			stree->tbranch[parent].depth;
		/* one character after the last character of the text */
		edge_letter_index_end = ef_length + 1;
	}
	/*
	 * if the edge length is greater than the number of remaining
	 * characters in the text, we have to adjust the number
	 * of edge label characters to be compared
	 */
	if (ef_length < position + edge_letter_index_end -
			edge_letter_index_at_start - 1) {
		/*
		 * we do it by decreasing the position
		 * where the comparison stops
		 */
		edge_letter_index_end = edge_letter_index_at_start +
			ef_length + 1 - position;
		/*
		 * we have to remember that we will not scan
		 * all the letters of the provided edge
		 */
		comparing_all_letters = 0;
	}
	edge_letter_index = edge_letter_index_at_start;
	/* while the comparison is successful */
	while (text[edge_letter_index] == text[position]) {
		++edge_letter_index; /* we increment the edge letter index */
		++position; /* as well as the position in the text */
		/* we check whether we should not end the comparison now */
		if (edge_letter_index == edge_letter_index_end) {
			/*
			 * We set the number of matching edge letters
			 * to the length of the edge (all the letters match).
			 */
			(*last_match_position) =
			(signed_integral_type)(edge_letter_index_end) -
			(signed_integral_type)(edge_letter_index_at_start);
			/*
			 * if we have (successfully) scanned and compared
			 * _all_ the letters of the edge
			 */
			if (comparing_all_letters == 1) {
				/*
				 * The slowscan succeeded (we have found
				 * the desired edge). This is actually
				 * the only case when the slowscan can succeed.
				 */
				return (0);
			} else {
				/*
				 * Slowscan partially succeeded,
				 * but we had to stop comparing the letters
				 * before the end of an edge.
				 */
				return (2);
			}
		}
	}
	/*
	 * If we got here, it means that we have not yet compared
	 * all the letters of an edge and we also have just encountered
	 * a character mismatch!
	 *
	 * we store the number of successfully matched characters
	 */
	(*last_match_position) = (signed_integral_type)(edge_letter_index -
			edge_letter_index_at_start);
	return (1); /* success */
}

/**
 * A function which advances to the next child and assigns
 * the pointers accordingly. It is a quick variant of the function,
 * which means that it does not keep track of previous child.
 *
 * @param
 * parent	the node from which the current edge starts
 * @param
 * child	the node which will be replaced by its next brother
 * 		(or by zero, if it does not have the next brother)
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully advance to the next child,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_quick_next_child (signed_integral_type parent,
		signed_integral_type *child,
		const character_type *text,
		const suffix_tree_shti_bp *stree) {
	int retval = 0;
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	character_type letter = WCHAR_MIN;
#else
	character_type letter = CHAR_MIN;
#endif
	if (parent <= 0) {
		fprintf(stderr, "Error: The provided parent (%u) "
				"is not a branching node!\n", parent);
		return (1);
	}
	if ((*child) == 0) {
		/*
		 * We are expected to return the first child,
		 * so we have to start with the first lookup separately,
		 * because we are expected to test the first letter as well.
		 */
		if (stree_shti_bp_ht_lookup(parent, letter, child,
					text, stree) == 0) {
			/* We have successfully found the first child! */
			return (0);
		}
	} else if ((*child) > 0) {
		letter = text[stree->tbranch[(*child)].head_position +
			stree->tbranch[parent].depth];
	} else { /* (*child) < 0 */
		letter = text[(unsigned_integral_type)(-(*child)) +
			stree->tbranch[parent].depth];
	}
	if (letter == terminating_character) {
		/*
		 * We have already reached the last child
		 * of the current parent.
		 */
		return (2);
	}
	(*child) = 0; /* resetting the current number of child */
	do {
		++letter;
		retval = stree_shti_bp_ht_lookup(parent, letter, child,
				text, stree);
	} while ((letter < terminating_character) && (retval > 0));
	if (retval == 0) {
		/* We have successfully found the next child! */
		return (0);
	} else {
		/* We were unable to find any next child. */
		return (3);
	}
}

/**
 * A function which advances to the next child and assigns
 * the pointers accordingly.
 *
 * @param
 * parent	the node from which the current edge starts
 * @param
 * child	the node which will be replaced by its next brother
 * 		(or by zero, if it does not have the next brother)
 * @param
 * prev_child	the node which will be replaced by the original value
 * 		of the "child" node
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully advance to the next child,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_next_child (signed_integral_type parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		const character_type *text,
		const suffix_tree_shti_bp *stree) {
	int retval = 0;
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	character_type letter = WCHAR_MIN;
#else
	character_type letter = CHAR_MIN;
#endif
	if (parent <= 0) {
		fprintf(stderr, "Error: The provided parent (%u) "
				"is not a branching node!\n", parent);
		return (1);
	}
	if ((*child) == 0) {
		/*
		 * We are expected to return the first child,
		 * so we have to start with the first lookup separately,
		 * because we are expected to test the first letter as well.
		 */
		if (stree_shti_bp_ht_lookup(parent, letter, child,
					text, stree) == 0) {
			/* We have successfully found the first child! */
			return (0);
		}
	} else if ((*child) > 0) {
		letter = text[stree->tbranch[(*child)].head_position +
			stree->tbranch[parent].depth];
	} else { /* (*child) < 0 */
		letter = text[(unsigned_integral_type)(-(*child)) +
			stree->tbranch[parent].depth];
	}
	if (letter == terminating_character) {
		/*
		 * We have already reached the last child
		 * of the current parent.
		 */
		return (2);
	}
	/* saving the current number of child as the previous child */
	(*prev_child) = (*child);
	(*child) = 0; /* resetting the current number of child */
	do {
		++letter;
		retval = stree_shti_bp_ht_lookup(parent, letter, child,
				text, stree);
	} while ((letter < terminating_character) && (retval > 0));
	if (retval == 0) {
		/* We have successfully found the next child! */
		return (0);
	} else {
		/* We were unable to find any next child. */
		return (3);
	}
}

/**
 * A function which ascends up along the path leading to the root.
 *
 * @param
 * parent	The node, which is the parent of the "child" node.
 * 		Unless it is the root, it will be given the value of its parent
 * 		when this function returns.
 * @param
 * child	The node, which is a child of the "parent" node.
 * 		When this function successfully returns,
 * 		it will be set to the "parent".
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully descend down along an edge,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_edge_climb (signed_integral_type *parent,
		signed_integral_type *child,
		const suffix_tree_shti_bp *stree) {
	/* if the parent is either a leaf, undefined or the root */
	if ((*parent) < 2) {
		fprintf(stderr,	"Error: Invalid number of parent (%d)!\n",
				(*parent));
		return (1); /* ascending failed (invalid number of parent) */
	}
	(*child) = (*parent);
	(*parent) = stree->tbranch[(*parent)].parent;
	return (0);
}

/**
 * A function which descends down along an edge.
 *
 * @param
 * parent	the node from which the given edge starts
 * 		(it will be given the value of "child"
 * 		when the function returns)
 * @param
 * child	The node where the given edge ends.
 * 		When the function returns, it will be set to zero.
 * @param
 * prev_child	the node which will be set to zero
 * 		if this function successfully returns.
 * @param
 * position	The position in the text of the substring which matches
 * 		the letters of the given edge. When this function returns,
 * 		it will be set to the first letter after the substring
 * 		which matches this very same edge.
 * @param
 * ef_length	the currently effective maximum length of the suffix
 * 		valid for the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully descend down along an edge,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		size_t ef_length,
		const suffix_tree_shti_bp *stree) {
	/* we suppose that the parent node of this edge is a branching node */
	if ((*child) == 0) {
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		return (1); /* invalid number of child */
	} else if ((*child) > 0) {
		(*position) = (*position) - stree->tbranch[(*parent)].depth +
			stree->tbranch[(*child)].depth;
	} else { /* (*child) < 0 */
		(*position) = (*position) - stree->tbranch[(*parent)].depth +
			(ef_length + 1 - (unsigned_integral_type)(-(*child)));
	}
	/* saving the current number of child as the next parent */
	(*parent) = (*child);
	(*child) = 0;
	(*prev_child) = 0;
	return (0);
}

/**
 * A function which selects the target node of an edge, which starts
 * at the provided parent node and begins with the provided letter
 * (if such an edge exists).
 *
 * @param
 * parent	the node from which the desired edge should start
 * @param
 * child	the node which will be set to the matching edge's end
 * @param
 * position	The position in the text which is just after the string
 * 		which matches the label of an edge leading to the parent.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could find an edge with the matching first character,
 * 		0 is returned.
 * 		If the number of parent from which we should do branching
 * 		is invalid, 1 is returned.
 * 		If no matching edge was found, 2 is returned.
 */
int st_shti_bp_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		size_t position,
		const character_type *text,
		const suffix_tree_shti_bp *stree) {
	character_type letter = 0;
	if (parent <= 0) {
		fprintf(stderr,	"Error: Invalid number of parent (%d)!\n",
				parent);
		return (1); /* branching failed (invalid number of parent) */
	}
	letter = text[position];
	if (stree_shti_bp_ht_lookup(parent, letter, child, text, stree) == 0) {
		/* lookup (or "fastscan") succeeded */
		return (0); /* branching succeeded (matching edge found) */
	} else {
		/* lookup (or "fastscan") was unsuccessful */
		return (2); /* branching failed (no matching edge) */
	}
}

/**
 * A function which tries to reach the desired depth
 * by ascending up in the suffix tree.
 *
 * @param
 * parent	The value of this parameter will be overwritten immediately
 * 		in the beginning of this function.
 * 		After this function returns, it will be set to the parent node
 * 		of the current "child" node. If this function returns
 * 		successfully, its depth will be exactly the target_depth.
 * @param
 * child	The node from which we start to ascend up.
 * 		When this function returns, it will be set
 * 		to the the most shallow node on the path towards the root,
 * 		which depth is strictly larger than the desired depth.
 * @param
 * target_depth	the depth which we are trying to reach
 * 		by ascending up in the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully reach the exact desired depth,
 * 		0 is returned.
 * 		If we could not reach exactly the desired depth, but we have
 * 		successfully reached an edge, which parent is too shallow
 * 		and which child is too deep, (-1) is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_go_up (signed_integral_type *parent,
		signed_integral_type *child,
		unsigned_integral_type target_depth,
		const suffix_tree_shti_bp *stree) {
	unsigned_integral_type parents_depth = 0;
	if ((*child) == 0) { /* child number is invalid */
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		return (1); /* invalid number of child */
	} else if ((*child) > 0) { /* child is a branching node */
		/* we set the parent of this child */
		(*parent) = stree->tbranch[(*child)].parent;
	} else { /* child < 0 (child is a leaf ) */
		/* we set the parent of this child */
		(*parent) = stree->tleaf[-(*child)].parent;
	}
	/* from now on, the parent is expected to be a branching node */
	do { /* we check the current depth and act appropriately */
		parents_depth = stree->tbranch[(*parent)].depth;
		if (parents_depth == target_depth) {
			/* we have found the node with the desired depth */
			return (0);
		} else if (parents_depth < target_depth) {
			/*
			 * We have climbed too high and now the parent's depth
			 * is too small.
			 */
			return (-1);
		} /* else we continue to ascend */
	/* While we can ascend up, we do it */
	} while (st_shti_bp_edge_climb(parent, child, stree) == 0);
	/*
	 * If we got here, it means that we could not climb further,
	 * because we either reached the root node
	 * or because of some error occurred. This way or another,
	 * it means that this function call was unsuccessful.
	 */
	fprintf(stderr,	"Error: We could not climb higher, but we still\n"
			"has not reached the desired depth!\n");
	return (2);
}

/**
 * A function which creates a new leaf at the specific position
 * in the suffix tree.
 *
 * Warning: This function differs from its counterpart
 * 'st_shti_create_leaf'.
 *
 * @param
 * parent	the parent of a newly created leaf
 * @param
 * letter	the first letter of an edge leading from the parent
 * 		to the newly created leaf
 * @param
 * new_leaf	the number of the newly created leaf
 * 		(the negative value of the starting position of the suffix,
 * 		which will end at this new leaf)
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully create a new leaf, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_create_leaf (signed_integral_type parent,
		character_type letter,
		signed_integral_type new_leaf,
		const character_type *text,
		suffix_tree_shti_bp *stree) {
	if (parent <= 0) {
		fprintf(stderr,	"Error: Could not create a new child "
				"of a non-branching node number %d!\n",
				parent);
		return (1); /* invalid number of parent */
	}
	if (stree_shti_bp_ht_insert(parent, letter, new_leaf, 1, text, stree)
			!= 0) {
		fprintf(stderr,	"Error: Could not insert the new leaf node\n"
				"into the hash table!\n");
		return (2);
	}
	/* we set the parent of the new leaf node */
	stree->tleaf[-new_leaf].parent = parent;
	return (0);
}

/**
 * A function which splits the edge from the provided parent
 * to the provided child in the suffix tree into two edges
 * at the specific position while creating a new branching node.
 *
 * Warning: This function differs from its counterpart
 * 'st_shti_split_edge'.
 *
 * @param
 * parent	The source node of an edge which will be split.
 * 		When the function returns, it will be set to
 * 		the newly created branching node.
 * @param
 * letter	The first letter of an edge leading from the parent
 * 		to the child.
 * @param
 * child	The target node of an edge, which will be split.
 * 		When the function returns, it will be set to zero.
 * @param
 * position	The position in the text of the beginning of a substring
 * 		which partially matches the characters of an edge
 * 		from the "parent" to the "child".
 * 		When the function returns, it will be set to the position
 * 		in the text just after the last character, which matches
 * 		the edge from the parent to the newly created branching node.
 * @param
 * last_match_position	The number of leading characters of an edge
 * 			from the "parent" to the "child", which match the text
 * 			starting at the position given
 * 			in the input parameter "position".
 * @param
 * new_head_position	The head position which will be assigned to the
 * 			newly created branching node between the "parent"
 * 			and the "child" nodes.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully split the edge and create
 * 		a new branching node, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_split_edge (signed_integral_type *parent,
		character_type letter,
		signed_integral_type *child,
		size_t *position,
		signed_integral_type last_match_position,
		unsigned_integral_type new_head_position,
		const character_type *text,
		suffix_tree_shti_bp *stree) {
	signed_integral_type new_branching_node = 0;
	unsigned_integral_type childs_head_position = 0;
	if ((*parent) <= 0) {
		fprintf(stderr,	"Error: Invalid number of parent (%d)!\n",
				(*parent));
		return (1); /* invalid number of parent */
	}
	if ((*child) == 0) {
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		return (2); /* invalid number of child */
	} else if ((*child) > 0) { /* child is a branching node */
		childs_head_position = stree->tbranch[(*child)].head_position;
	} else { /* (*child) < 0 => child is a leaf */
		childs_head_position = (unsigned_integral_type)(-(*child));
	}
	new_branching_node = (signed_integral_type)(++stree->branching_nodes);
	if (last_match_position == 0) {
		/*
		 * Either we don't know the mismatch position or
		 * it is at the beginning of an edge.
		 * In both cases, we can not split an edge nor create
		 * a new branching node. So, in fact, it is a failure.
		 */
		return (3);
	}
	/* the following value will be set later */
	stree->tbranch[new_branching_node].suffix_link = 0;
	/*
	 * This is where the implementation with backward pointers differs
	 * from the original implementation
	 */
	stree->tbranch[new_branching_node].parent = (*parent);
	stree->tbranch[new_branching_node].depth =
		stree->tbranch[(*parent)].depth +
		(unsigned_integral_type)(last_match_position);
	stree->tbranch[new_branching_node].head_position = new_head_position;
	/* correcting the old edge to end at the new_branching_node */
	if (stree_shti_bp_ht_insert((*parent), letter,
				new_branching_node, 1, text, stree) != 0) {
		fprintf(stderr,	"Error: Could not correct the old edge "
				"to end at the new_branching_node!\n");
		return (4);
	}
	letter = text[childs_head_position +
		stree->tbranch[new_branching_node].depth];
	/*
	 * creating the new edge from the new_branching_node
	 * to the original child
	 */
	if (stree_shti_bp_ht_insert(new_branching_node, letter,
				(*child), 1, text, stree) != 0) {
		fprintf(stderr,	"Error: Could not insert the new edge "
				"starting at the new_branching_node\n"
				"into the hash table!\n");
		return (5);
	}
	/* another difference from the original implementation */
	if ((*child) > 0) { /* child is a branching node */
		stree->tbranch[(*child)].parent = new_branching_node;
	} else { /* (*child) < 0 => child is a leaf */
		stree->tleaf[-(*child)].parent = new_branching_node;
	}
	(*parent) = new_branching_node;
	(*child) = 0;
	/* adjusting the position in the text */
	(*position) = (*position) +
		(unsigned_integral_type)(last_match_position);
	return (0);
}

/**
 * A function which traverses and prints the suffix tree
 * while starting from the given node.
 * This function performs a simple traversal.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written
 * @param
 * starting_node	the node from which the traversing starts
 * @param
 * log10bn	A ceiling of base 10 logarithm of the number
 * 		of branching nodes. It will be used for printing alignment.
 * @param
 * log10l	A ceiling of base 10 logarithm of the number
 * 		of leaves. It will be used for printing alignment.
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree which will be traversed
 *
 * @return	If we could successfully traverse and print the suffix tree
 * 		and if we could start from the given node, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_simple_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length,
		const suffix_tree_shti_bp *stree) {
	signed_integral_type child = 0;
	/* the parent of the child as stored in the child node */
	signed_integral_type childs_parent = 0;
	unsigned_integral_type parents_depth = 0;
	unsigned_integral_type childs_depth = 0;
	size_t childs_offset = 0;
	if (starting_node <= 0) {
		fprintf(stderr,	"Error: The traveral has to start from "
				"a branching node, but the starting node "
				"is %d!", starting_node);
		return (1);
	}
	/* getting the first child of the starting_node */
	if (st_shti_bp_quick_next_child(starting_node, &child, text, stree)
			!= 0) {
		fprintf(stderr,	"Error: The traversal of the current branch "
				"is not possible,\nbecause we were not able "
				"to advance to the next child of the parent "
				"(%d)!\n", starting_node);
		return (2);
	}
	parents_depth = stree->tbranch[starting_node].depth;
	do {
		if (child > 0) {
			childs_depth = stree->tbranch[child].depth;
			childs_offset = stree->tbranch[child].head_position;
			childs_parent = stree->tbranch[child].parent;
			if (childs_parent != starting_node) {
				fprintf(stderr,	"Error: Something went wrong."
						"\nChild (%d) has a different "
						"parent (%d)\nfrom what is "
						"stored inside its branching "
						"record (%d).\n", child,
						starting_node, childs_parent);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (3);
			}
			st_print_edge(stream, 0,
					(signed_integral_type)(0),
					(signed_integral_type)(0),
					(signed_integral_type)(0),
					parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, internal_text_encoding,
					text);
			st_shti_bp_simple_traverse_from(stream, child,
					log10bn, log10l,
					internal_text_encoding, text, length,
					stree);
		} else { /* child < 0 */
			childs_depth = (unsigned_integral_type)
				((length + 2) - (size_t)(-child));
			childs_offset = (unsigned_integral_type)(-child);
			childs_parent = stree->tleaf[-child].parent;
			if (childs_parent != starting_node) {
				fprintf(stderr,	"Error: Something went wrong."
						"\nChild (%d) has a different "
						"parent (%d)\nfrom what is "
						"stored inside its leaf "
						"record (%d).\n", child,
						starting_node, childs_parent);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (4);
			}
			st_print_edge(stream, 1,
					(signed_integral_type)(0),
					child, (signed_integral_type)(0),
					parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, internal_text_encoding,
					text);
		}
		/*
		 * there is quite a large overhead to move to the next child
		 * of the starting_node, but it is unevitable, because of
		 * the nature of the hash table representation.
		 */
	} while (st_shti_bp_quick_next_child(starting_node, &child, text, stree)
			== 0);
	return (0);
}

/**
 * A function which traverses and prints the suffix tree
 * while starting from the given node.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written
 * @param
 * starting_node	the node from which the traversing starts
 * @param
 * log10bn	A ceiling of base 10 logarithm of the number
 * 		of branching nodes. It will be used for printing alignment.
 * @param
 * log10l	A ceiling of base 10 logarithm of the number
 * 		of leaves. It will be used for printing alignment.
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree which will be traversed
 *
 * @return	If we could successfully traverse and print the suffix tree
 * 		and if we could start from the given node, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_shti_bp_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length,
		const suffix_tree_shti_bp *stree) {
	signed_integral_type child = 0;
	/* the parent of the child as stored in the child node */
	signed_integral_type childs_parent = 0;
	signed_integral_type childs_suffix_link = 0;
	unsigned_integral_type parents_depth = 0;
	unsigned_integral_type childs_depth = 0;
	size_t childs_offset = 0;
	if (starting_node <= 0) {
		fprintf(stderr,	"Error: The traveral has to start from "
				"a branching node, but the starting node "
				"is %d!", starting_node);
		return (1);
	}
	/* getting the first child of the starting_node */
	if (st_shti_bp_quick_next_child(starting_node, &child, text, stree)
			!= 0) {
		fprintf(stderr,	"Error: The traversal of the current branch "
				"is not possible,\nbecause we were not able "
				"to advance to the next child of the parent "
				"(%d)!\n", starting_node);
		return (2);
	}
	parents_depth = stree->tbranch[starting_node].depth;
	do {
		if (child > 0) {
			childs_suffix_link = stree->tbranch[child].suffix_link;
			childs_depth = stree->tbranch[child].depth;
			childs_offset = stree->tbranch[child].head_position;
			childs_parent = stree->tbranch[child].parent;
			if (childs_parent != starting_node) {
				fprintf(stderr,	"Error: Something went wrong."
						"\nChild (%d) has a different "
						"parent (%d)\nfrom what is "
						"stored inside its branching "
						"record (%d).\n", child,
						starting_node, childs_parent);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (3);
			}
			st_print_edge(stream, 0, starting_node, child,
					childs_suffix_link, parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, internal_text_encoding,
					text);
			st_shti_bp_traverse_from(stream, child, log10bn, log10l,
					internal_text_encoding, text, length,
					stree);
		} else { /* child < 0 */
			childs_depth = (unsigned_integral_type)
				((length + 2) - (size_t)(-child));
			childs_offset = (unsigned_integral_type)(-child);
			childs_parent = stree->tleaf[-child].parent;
			if (childs_parent != starting_node) {
				fprintf(stderr,	"Error: Something went wrong."
						"\nChild (%d) has a different "
						"parent (%d)\nfrom what is "
						"stored inside its leaf "
						"record (%d).\n", child,
						starting_node, childs_parent);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (4);
			}
			st_print_edge(stream, 1, starting_node, child,
					0, parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, internal_text_encoding,
					text);
		}
		/*
		 * there is quite a large overhead to move to the next child
		 * of the starting_node, but it is unevitable, because of
		 * the nature of the hash table representation.
		 */
	} while (st_shti_bp_quick_next_child(starting_node, &child, text, stree)
			== 0);
	return (0);
}

/* handling functions */

/**
 * A function which traverses the suffix tree while printing its edges.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * traversal_type	the type of the traversal to perform
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree to be traversed
 *
 * @return	If the traversal was successful, zero is returned.
 * 		Otherwise, a positive error number is returned.
 */
int st_shti_bp_traverse (FILE *stream,
		const char *internal_text_encoding,
		int traversal_type,
		const character_type *text,
		size_t length,
		const suffix_tree_shti_bp *stree) {
	signed_integral_type start_node = 1; /* the root */
	/* the current number of branching nodes in the suffix tree */
	size_t branching_nodes = stree->branching_nodes;
	/*
	 * The current number of leaves in the suffix tree.
	 * We have to include the suffix consisting of
	 * the terminating character ($) only.
	 */
	size_t leaves = length + 1;
	/*
	 * a ceiling of base 10 logarithm of the number of branching nodes
	 * (it will be used for printing alignment)
	 */
	size_t log10bn = 1;
	/*
	 * a ceiling of base 10 logarithm of the number of leaves
	 * (it will be used for printing alignment)
	 */
	size_t log10l = 1;
	/*
	 * adjusting the ceiling of base 10 logarithm
	 * of the number of branching nodes
	 */
	while (branching_nodes > 9) {
		++log10bn;
		branching_nodes = branching_nodes / 10;
	}
	/*
	 * adjusting the ceiling of base 10 logarithm
	 * of the number of leaves
	 */
	while (leaves > 9) {
		++log10l;
		leaves = leaves / 10;
	}
	printf("Traversing the suffix tree\n\n");
	if (stream != stdout) {
		printf("The traversal log is being dumped "
				"to the specified file.\n");
	}
	if (traversal_type == tt_detailed) {
		fprintf(stream, "Suffix tree traversal BEGIN\n");
		if (st_shti_bp_traverse_from(stream, start_node,
					log10bn, log10l,
					internal_text_encoding,
					text, length, stree) > 0) {
			fprintf(stderr, "Error: The traversal "
					"from the branching node\n"
					"was unsuccessful. "
					"Exiting!\n");
			return (1);
		}
		fprintf(stream, "Suffix tree traversal END\n");
	} else if (traversal_type == tt_simple) {
		fprintf(stream, "Simple suffix tree traversal BEGIN\n");
		if (st_shti_bp_simple_traverse_from(stream, start_node,
					log10bn, log10l,
					internal_text_encoding,
					text, length, stree) > 0) {
			fprintf(stderr, "Error: The traversal "
					"from the branching node\n"
					"was unsuccessful. "
					"Exiting!\n");
			return (2);
		}
		fprintf(stream, "Simple suffix tree traversal END\n");
	} else {
		fprintf(stderr, "Error: Unknown traversal type (%d) "
				"Exiting!\n", traversal_type);
		return (3);
	}
	if (stream != stdout) {
		printf("Dump complete.\n");
	}
	printf("\nTraversing completed\n\n");
	return (0);
}

/**
 * A function which deallocates the memory used by the suffix tree.
 *
 * @param
 * stree	the actual suffix tree to be "deleted" (in more detail:
 * 		all the data it contains will be erased and all the
 * 		dynamically allocated memory it occupies will be deallocated,
 * 		but the suffix tree struct itself will remain intact,
 * 		because we can not be sure it was dynamically allocated
 * 		and that's why we can not deallocate it)
 *
 * @return	On successful deallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int st_shti_bp_delete (suffix_tree_shti_bp *stree) {
	printf("Deleting the suffix tree\n");
	if (hs_deallocate(stree->hs) > 0) {
		fprintf(stderr,	"Error: The hash settings could not be "
				"properly deallocated!\n");
		return (1);
	}
	free(stree->tbranch);
	stree->tbranch = NULL;
	free(stree->tedge);
	stree->tedge = NULL;
	free(stree->tleaf);
	stree->tleaf = NULL;
	/*
	 * maintaining the suffix tree struct
	 * constistent with its definition
	 */
	stree->branching_nodes = 0;
	stree->tbranch_size = 0;
	stree->edges = 0;
	stree->tedge_size = 0;
	/*
	 * The other entries of the suffix_tree_shti_bp struct
	 * need not to be reset to zero.
	 */
	printf("Successfully deleted!\n");
	return (0);
}
