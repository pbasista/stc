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
 * SLLI common functions implementation.
 * This file contains the implementation of the auxiliary functions,
 * which are used by the functions, which construct
 * the suffix tree in the memory using the implementation type SLLI.
 */
#include "stree_slli_common.h"

#include <errno.h>
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
int st_slli_allocate (size_t length,
		suffix_tree_slli *stree) {
	/*
	 * the future size of the table tbranch (the first estimation,
	 * which will be further adjusted)
	 */
	size_t unit_size = (size_t)(1) <<
		(sizeof (size_t) * 8 - 1);
	size_t allocated_size = 0;
	printf("==============================================\n"
		"Trying to allocate memory for the suffix tree:\n\n");
	/* the adjustment of the future size of the table tbranch */
	while (length < unit_size) {
		unit_size = unit_size >> 1; /* unit_size / 2 */
	}
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stree->tleaf);
	stree->tleaf = NULL;
	/* we need to fill in the size of the leaf record */
	stree->lr_size =  sizeof (leaf_record_slli);
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
		return (1);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size = (length + 2) * stree->lr_size;
	printf("Successfully allocated!\n\n");
	free(stree->tbranch);
	stree->tbranch = NULL;
	/* we need to fill in the size of the branch record */
	stree->br_size =  sizeof (branch_record_slli);
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
		return (2);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += (unit_size + 1) * stree->br_size;
	printf("Successfully allocated!\n\n"
		"Total amount of memory initially allocated: %zu bytes (",
			allocated_size);
	print_human_readable_size(stdout, allocated_size);
	/* The meaning: per "real" input character */
	printf(")\nwhich is %.3f bytes per input character.\n\n",
			(double)(allocated_size) / (double)(length));
	/*
	 * This memory is cleared in advance, so we need not to do
	 * the following assignments. But we do, because we would like
	 * to emphasize the correctness of the information stored
	 * for the root in the clean table tbranch.
	 *
	 * stree->tbranch[1] is the root and it has no children
	 * in the beginning
	 */
	stree->tbranch[1].first_child = 0;
	/* it has no brother in the beginning */
	stree->tbranch[1].branch_brother = 0;
	/* its suffix link is undefined (and can never be defined) */
	stree->tbranch[1].suffix_link = 0;
	/* it has the depth of zero (which never changes) */
	stree->tbranch[1].depth = 0;
	/* its head position is zero (by definition) */
	stree->tbranch[1].head_position = 0;
	/* So, in the beginning, we have only one branching node - the root. */
	stree->branching_nodes = 1;
	/*
	 * we store the number of the real branch records available
	 * rather than the number of branch records allocated
	 */
	stree->tbranch_size = unit_size;
	/* The first table tbranch size increase step */
	stree->tbsize_increase = unit_size >> 1; /* unit_size / 2 */
	printf("The memory has been successfully allocated!\n"
		"===========================================\n\n");
	return (0);
}

/**
 * A function which reallocates the memory for the suffix tree
 * to make it larger.
 *
 * @param
 * desired_size	the minimum requested size of the table tbranch
 * 		(in the number of nodes, not including the leading 0.th node)
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int st_slli_reallocate (size_t desired_size,
		size_t length,
		suffix_tree_slli *stree) {
	void *tmp_pointer = NULL;
	size_t new_tbranch_size = stree->tbranch_size +
		stree->tbsize_increase;
	/* if the implicitly chosen new size is too small */
	if (new_tbranch_size < desired_size) {
		/* we make it large enough */
		new_tbranch_size = desired_size;
	}
	/*
	 * on the other hand, if the new size is too large, we make it smaller,
	 * but still large enough to hold all the data the table tbranch
	 * can possibly hold
	 */
	if (new_tbranch_size > length) {
		/*
		 * the maximum number of branching nodes is one less than
		 * the maximum number of leaf nodes (which is length + 1)
		 */
		new_tbranch_size = length;
	}
	printf("Trying to reallocate the memory "
			"for the table tbranch: new size:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
			new_tbranch_size + 1, stree->br_size,
			(new_tbranch_size + 1) * stree->br_size);
	print_human_readable_size(stdout,
			(new_tbranch_size + 1) * stree->br_size);
	printf(").\n");
	/*
	 * The number of actually allocated branch records is increased by one,
	 * because of the 0.th record, which is never used.
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
		stree->tbsize_increase = 128; /* minimum increase step */
	} else {
		/* division by 2 */
		stree->tbsize_increase = stree->tbsize_increase >> 1;
	}
	printf("Successfully reallocated!\n");
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
 * child	a target node of an edge which will be depth-scanned
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
int st_slli_edge_depthscan (signed_integral_type child,
		unsigned_integral_type target_depth,
		size_t ef_length,
		const suffix_tree_slli *stree) {
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
	} else {
		return (1); /* too long edge */
	}
}

/**
 * A function to perform "fastscan" on a given edge with a given letter.
 *
 * Fastscan checks whether the first letter of the provided edge
 * matches the letter at the desired position in the text.
 *
 * @param
 * parent	the node from which the given edge starts
 * @param
 * child	the node where the given edge ends
 * @param
 * position	the position in the text of the letter which will be matched
 * 		against the first letter of a given edge
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If the leading letter of an edge equals the desired letter,
 * 		0 is returned.
 * 		If the leading letter of an edge is smaller
 * 		than the required letter, (-1) is returned.
 * 		If the leading letter of an edge is larger
 * 		than the required letter, 1 is returned.
 * 		If the provided child number is zero, 2 is returned.
 */
int st_slli_edge_fastscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		const character_type *text,
		const suffix_tree_slli *stree) {
	size_t edge_letter_index = 0;
	character_type edge_letter = 0;
	character_type text_letter = text[position];
	if (child == 0) {
		/*
		 * We do not print an error there, because it is valid
		 * for this function to fail this way when it is called
		 * after the last child of the parent has been examined.
		 */
		return (2); /* not a valid child */
	} else if (child > 0) { /* child is a branching node */
		edge_letter_index = stree->tbranch[child].head_position +
			stree->tbranch[parent].depth;
	} else { /* child < 0 => child is a leaf */
		edge_letter_index = (unsigned_integral_type)(-child) +
			stree->tbranch[parent].depth;
	}
	edge_letter = text[edge_letter_index];
	if (edge_letter < text_letter) {
		return (-1); /* an edge with too "small" leading letter */
	} else if (edge_letter == text_letter) {
		return (0); /* the correct edge */
	} else { /* edge_letter > text_letter */
		return (1); /* an edge with too "big" leading letter */
	}
}

/**
 * A function to perform "slowscan" on a given edge starting
 * with a given letter.
 *
 * Slowscan examines all the letters of the given edge and checks
 * whether they match the provided substring in the text.
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
 * 			"position". Moreover, the sign of this parameter
 * 			will indicate the type of the first mismatch, if any.
 * 			If the first mismatching edge label character was
 * 			"larger" than the corresponging character in the text,
 * 			we keep the sign positive.
 * 			Otherwise, we make it negative.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * ef_length	the currently effective maximum length of the suffix
 * 		valid for the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If all the letters of an edge match, 0 is returned.
 * 		If the first mismatching edge letter is smaller
 * 		than the required letter, (-1) is returned.
 * 		If the first mismatching edge letter is larger
 * 		than the required letter, 1 is returned.
 * 		If the number of letters to compare is smaller than the length
 * 		of an edge, but all these letters do match, 2 is returned.
 * 		If an error occures, a positive error number
 * 		greater than 2 is returned.
 */
int st_slli_edge_slowscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		signed_integral_type *last_match_position,
		const character_type *text,
		size_t ef_length,
		const suffix_tree_slli *stree) {
	size_t edge_letter_index = 0;
	size_t edge_letter_index_at_start = 0;
	size_t edge_letter_index_end = 0;
	/*
	 * if this variable evaluates to true, we will be scanning
	 * (and comparing) all the letters of an edge
	 */
	int comparing_all_letters = 1;
	if (child == 0) {
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		/*
		 * As there were no matching characters, we have to set
		 * the "(*last_match_position)" accordingly.
		 */
		(*last_match_position) = 0;
		return (3); /* not a valid child */
	} else if (child > 0) { /* child is a branching node */
		edge_letter_index_at_start = stree->tbranch[child].
			head_position + stree->tbranch[parent].depth;
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
		 * all the letters of an edge
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
			 * to the length of an edge (all the letters match).
			 * In this case, we need not to set the sign,
			 * so we keep it positive.
			 */
			(*last_match_position) =
			(signed_integral_type)(edge_letter_index_end) -
			(signed_integral_type)(edge_letter_index_at_start);
			/*
			 * if we have (successfully) scanned and compared
			 * all the letters of an edge
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
	 * (not the sign, yet)
	 */
	(*last_match_position) = (signed_integral_type)(edge_letter_index -
			edge_letter_index_at_start);
	if (text[edge_letter_index] < text[position]) {
		/*
		 * we have to make the sign
		 * of the "(*last_match_position)" negative
		 */
		(*last_match_position) = (-(*last_match_position));
		/*
		 * partially matching edge with "smaller"
		 * mismatching character
		 */
		return (-1);
	} else { /* text[edge_letter_index] > text[position] */
		/*
		 * We keep the sign of the "(*last_match_position)" positive,
		 * because there is a partially matching edge
		 * with "larger" mismatching character.
		 */
		return (1);
	}
}

/**
 * A function which advances to the next child and assigns
 * the pointers accordingly. It is a quick variant of the function,
 * which means that it does not keep track of previous child.
 *
 * @param
 * child	the node which will be replaced by its next brother
 * 		(or by zero, if it does not have the next brother)
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully advance to the next child,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_slli_quick_next_child (signed_integral_type *child,
		const suffix_tree_slli *stree) {
	if ((*child) == 0) {
		return (1); /* the next child does not exist */
	}
	if ((*child) > 0) {
		(*child) = stree->tbranch[(*child)].branch_brother;
	} else {
		(*child) = stree->tleaf[-(*child)].next_brother;
	}
	return (0);
}

/**
 * A function which advances to the next child and assigns
 * the pointers accordingly.
 *
 * @param
 * child	the node which will be replaced by its next brother
 * 		(or by zero, if it does not have the next brother)
 * @param
 * prev_child	the node which will be replaced by the original value
 * 		of the "child" node
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully advance to the next child,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_slli_next_child (signed_integral_type *child,
		signed_integral_type *prev_child,
		const suffix_tree_slli *stree) {
	if ((*child) == 0) {
		return (1); /* the next child does not exist */
	}
	(*prev_child) = (*child);
	if ((*child) > 0) {
		(*child) = stree->tbranch[(*child)].branch_brother;
	} else {
		(*child) = stree->tleaf[-(*child)].next_brother;
	}
	return (0);
}

/**
 * A function which descends down along an edge. Its only variant is quick,
 * which means that it does not keep track of the previous child.
 *
 * @param
 * parent	the node from which the given edge starts
 * 		(it will be given the value of "child"
 * 		when the function returns)
 * @param
 * child	The node where the given edge ends.
 * 		When the function returns, it will be set to the first child
 * 		of itself (the first child of the "child" node) or to zero
 * 		(if there is no child of the "child" node).
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
int st_slli_quick_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		size_t *position,
		size_t ef_length,
		const suffix_tree_slli *stree) {
	if ((*child) == 0) {
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		return (1); /* invalid number of child */
	} else if ((*child) > 0) {
		(*position) = (*position) - stree->tbranch[(*parent)].depth +
			stree->tbranch[(*child)].depth;
		(*parent) = (*child);
		(*child) = stree->tbranch[(*parent)].first_child;
	} else { /* (*child) < 0 */
		(*position) = (*position) - stree->tbranch[(*parent)].depth +
			(ef_length + 1 - (unsigned_integral_type)(-(*child)));
		(*parent) = (*child);
		(*child) = 0;
	}
	return (0);
}

/**
 * A function which descends down along an edge. Its only variant is quick,
 * which means that it does not keep track of the previous child.
 *
 * @param
 * parent	the node from which the given edge starts
 * 		(it will be given the value of "child"
 * 		when the function returns)
 * @param
 * child	The node where the given edge ends.
 * 		When the function returns, it will be set to the first child
 * 		of itself (the first child of the "child" node) or to zero
 * 		(if there is no child of the "child" node).
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
int st_slli_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		size_t ef_length,
		const suffix_tree_slli *stree) {
	if ((*child) == 0) {
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		return (1); /* invalid number of child */
	} else if ((*child) > 0) {
		(*position) = (*position) - stree->tbranch[(*parent)].depth +
			stree->tbranch[(*child)].depth;
		(*parent) = (*child);
		(*child) = stree->tbranch[(*parent)].first_child;
		(*prev_child) = 0;
	} else { /* (*child) < 0 */
		(*position) = (*position) - stree->tbranch[(*parent)].depth +
			(ef_length + 1 - (unsigned_integral_type)(-(*child)));
		(*parent) = (*child);
		(*child) = 0;
		(*prev_child) = 0;
	}
	return (0);
}

/**
 * A function which examines all the children of a given parent and tries
 * to find a matching edge. It is a quick variant of the function, which means
 * that it does not keep track of previous children.
 *
 * @param
 * parent	the node from which the examination starts
 * @param
 * child	the node which will be set to the first matching edge's end
 * @param
 * position	The position in the text which is just after the character
 * 		which matches the last letter of an edge leading to the parent.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could find an edge with a matching first character,
 * 		0 is returned.
 * 		If the number of parent from which we should do branching
 * 		is invalid, 1 is returned.
 * 		If the parent has no children at all, 2 is returned.
 * 		If the parent has children, but none of them has the matching
 * 		first character, 3 is returned.
 */
int st_slli_quick_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		size_t position,
		const character_type *text,
		const suffix_tree_slli *stree) {
	int tmp = 0; /* here we store the return value of the "fastscan" */
	if (parent <= 0) {
		fprintf(stderr,	"Error: Invalid number of parent (%d)!\n",
				parent);
		return (1); /* branching failed (invalid number of parent) */
	}
	(*child) = stree->tbranch[parent].first_child;
	if ((*child) == 0) {
		fprintf(stderr,	"Warning: There is a branching node "
				"with no children!\n");
		return (2); /* branching failed (no children) */
	}
	while ((tmp = st_slli_edge_fastscan(parent, (*child), position,
					text, stree)) == (-1)) {
		/*
		 * The fastscan was unsuccessful, but the first character
		 * of the edge label was "smaller" than the desired character,
		 * so there is still a chance that there is an edge
		 * leading to some of the next children of the "parent"
		 * which label starts with the desired character.
		 */
		st_slli_quick_next_child(child, stree);
	}
	if (tmp == 0) { /* fastscan succeeded */
		return (0); /* branching succeeded (matching edge found) */
	} else {
		return (3); /* branching failed (no matching edge) */
	}
}

/**
 * A function which examines all the children of a given parent
 * and tries to find a matching edge.
 *
 * @param
 * parent	the node from which the examination starts
 * @param
 * child	the node which will be set to the first matching edge's end
 * @param
 * prev_child	the node which will be set to the end of the last non-matching
 * 		edge leading from the parent. If there is no such edge,
 * 		it will be set to zero.
 * @param
 * position	The position in the text which is just after the character
 * 		which matches the last letter of an edge leading to the parent.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could find an edge with a matching first character,
 * 		0 is returned.
 * 		If the number of parent from which we should do branching
 * 		is invalid, 1 is returned.
 * 		If the parent has no children at all, 2 is returned.
 * 		If the parent has children, but none of them has the matching
 * 		first character, 3 is returned.
 */
int st_slli_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t position,
		const character_type *text,
		const suffix_tree_slli *stree) {
	int tmp = 0; /* here we store the return value of the "fastscan" */
	if (parent <= 0) {
		fprintf(stderr,	"Error: Invalid number of parent (%d)!\n",
				parent);
		return (1); /* branching failed (invalid number of parent) */
	}
	(*child) = stree->tbranch[parent].first_child;
	(*prev_child) = 0;
	if ((*child) == 0) {
		fprintf(stderr,	"Warning: There is a branching node "
				"with no children!\n");
		return (2); /* branching failed (no children) */
	}
	while ((tmp = st_slli_edge_fastscan(parent, (*child), position,
					text, stree)) == (-1)) {
		/*
		 * The fastscan was unsuccessful, but the first character
		 * of the edge label was "smaller" than the desired character,
		 * so there is still a chance that there is an edge
		 * leading to some of the next children of the "parent"
		 * which label starts with the desired character.
		 */
		st_slli_next_child(child, prev_child, stree);
	}
	if (tmp == 0) { /* fastscan succeeded */
		return (0); /* branching succeeded (matching edge found) */
	} else {
		return (3); /* branching failed (no matching edge) */
	}
}

/**
 * A function which tries to reach the desired depth
 * by descending down in the suffix tree.
 *
 * @param
 * grandpa	The node from which we start to descend down.
 * 		The value of this parameter will be changed by descending down
 * 		in the suffix tree, but this change remains
 * 		within this function and is not propagated outside.
 * @param
 * parent	The value of this parameter will be overwritten immediately
 * 		in the beginning of this function. It will then hold
 * 		the end node of an edge which starts from the grandpa
 * 		and matches the text starting at "position".th character.
 * 		After this function returns, it will be set to the end node
 * 		of an edge, which starts from the current "grandpa" and matches
 * 		the text starting at the current "position".th character,
 * 		but which depth exceeds the target depth.
 * @param
 * target_depth	the depth which we are trying to reach
 * 		by descending down in the suffix tree
 * @param
 * position	The position in the text of the beginning of the substring
 * 		which matches an edge leading from the grandpa to the parent
 * 		After this function returns, it will be set to the position
 * 		in the text just after the character which matches
 * 		the last letter of an edge leading
 * 		from the current grandpa to the current parent.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * ef_length	the currently effective maximum length of the suffix
 * 		valid for the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully reach the exact desired depth,
 * 		0 is returned.
 * 		If even the last node of the explored path is too shallow,
 * 		1 is returned.
 * 		If the source node of the last edge explored has the depth
 * 		smaller than the desired depth and the target node
 * 		of the same edge has the depth larger than the desired depth,
 * 		(-1) is returned.
 * 		If the branching fails while the desired depth
 * 		is still not reached, 2 is returned.
 */
int st_slli_go_down (signed_integral_type grandpa,
		signed_integral_type *parent,
		unsigned_integral_type target_depth,
		size_t *position,
		const character_type *text,
		size_t ef_length,
		const suffix_tree_slli *stree) {
	signed_integral_type child = 0; /* the child of the "parent" */
	int tmp = 0; /* here we store the return value of the "depthscan" */
	(*parent) = grandpa;
	/* parent has to be a branching node */
	if (stree->tbranch[(*parent)].depth == target_depth) {
		/*
		 * we need not to descend, because we have
		 * already reached the exact target depth
		 */
		return (0);
	}
	while (st_slli_quick_branch_once((*parent), &child, (*position),
				text, stree) == 0) {
		if ((tmp = st_slli_edge_depthscan(child, target_depth,
						ef_length, stree)) == -1) {
			/*
			 * the edge length is strictly smaller
			 * than the maximum allowed length
			 */
			grandpa = (*parent);
			st_slli_quick_edge_descend(parent, &child,
					position, ef_length, stree);
			if ((*parent) < 0) { /* parent is a leaf */
				fprintf(stderr,	"Warning: By increasing the "
						"depth, we reached a leaf "
						"and we are still "
						"not deep enough. "
						"This should not happen!\n");
				return (1);
			}
		} else if (tmp == 0) { /* the edge of exact length */
			/*
			 * note that we need not to adjust the position,
			 * because it has already been set correctly
			 */
			(*parent) = child;
			return (0);
		} else { /* the target node of an edge is too deep */
			return (-1); /* we are stopping here */
		}
	}
	fprintf(stderr,	"Error: Branching failed while the desired depth\n"
			"still has not been reached!\n");
	return (2);
}

/**
 * A function which creates a new leaf at the specific position
 * in the suffix tree.
 *
 * @param
 * parent	the parent of a newly created leaf
 * @param
 * child	The child of the parent, which will be the next brother
 * 		of a newly created leaf. If "child" is zero, then
 * 		the newly created leaf will not have the next brother.
 * @param
 * prev_child	The child of the parent, whose next or branch brother will be
 * 		the newly created leaf. If "prev_child" is zero, then
 * 		the newly created leaf will be the first child of the parent.
 * @param
 * new_leaf	the number of the newly created leaf
 * 		(the negative value of the starting position of the suffix,
 * 		which will end at this new leaf)
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully create a new leaf, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_slli_create_leaf (signed_integral_type parent,
		signed_integral_type child,
		signed_integral_type prev_child,
		signed_integral_type new_leaf,
		suffix_tree_slli *stree) {
	if (parent <= 0) {
		fprintf(stderr,	"Error: Could not create a new child "
				"of a non-branching node number %d!\n",
				parent);
		return (1); /* invalid number of parent */
	}
	if (prev_child == 0) {
		stree->tbranch[parent].first_child = new_leaf;
	} else if (prev_child > 0) {
		stree->tbranch[prev_child].branch_brother = new_leaf;
	} else { /* prev_child < 0 */
		stree->tleaf[(-prev_child)].next_brother = new_leaf;
	}
	stree->tleaf[-new_leaf].next_brother = child;
	return (0);
}

/**
 * A function which splits the edge from parent to child in the suffix tree
 * into two edges at the specific position while creating
 * a new branching node.
 *
 * @param
 * parent	The source node of an edge which will be split.
 * 		When the function returns, the "parent" will be set to
 * 		the newly created branching node.
 * @param
 * child	The target node of an edge, which will be split.
 * 		When the function returns, the "child" will be set either
 * 		to zero or it retains its value in compliance
 * 		with the mismatch character.
 * @param
 * prev_child	The previous (in terms of parent's children ordering) child
 * 		of the "parent" (the one preceding the "child").
 * 		If "prev_child" is zero, then the "child" is the first child
 * 		of the "parent".
 * 		When the function returns, the "prev_child" will be set either
 * 		to zero or the "child", in compliance with the mismatch
 * 		character.
 * @param
 * position	The position in the text of the beginning of a substring
 * 		which partially matches the characters of an edge
 * 		from the "parent" to the "child".
 * 		When the function returns, the position will be set
 * 		to the character in the text just after
 * 		the last matching character.
 * @param
 * last_match_position	The number of leading characters of an edge
 * 			from the "parent" to the "child", which match the text
 * 			starting at the position given
 * 			in the input parameter "position".
 * @param
 * new_head_position	The head position which will be assigned to the
 * 			newly created branching node between the "parent"
 * 			and the "child".
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully split the edge and create
 * 		a new branching node, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_slli_split_edge (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		signed_integral_type last_match_position,
		unsigned_integral_type new_head_position,
		suffix_tree_slli *stree) {
	signed_integral_type new_branching_node = 0;
	/*
	 * If this variable evaluates to true, it means that the newly created
	 * branching node will have the "child" node as its first child.
	 */
	int child_first = 0;
	if ((*parent) <= 0) {
		fprintf(stderr,	"Error: Invalid number of parent (%d)!\n",
				(*parent));
		return (1); /* invalid number of parent */
	}
	if ((*child) == 0) {
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		return (2); /* invalid number of child */
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
	/*
	 * the negative value of the last_match_position means that
	 * the mismatching character in the text was "larger"
	 * than the corresponding character in the edge's label
	 */
	} else if (last_match_position < 0) {
		/*
		 * in this case, the original child node will be
		 * the first child of the newly created branching node
		 */
		child_first = 1;
		/* we make the last matching position positive */
		last_match_position = (-last_match_position);
	}
	if ((*prev_child) == 0) {
		stree->tbranch[(*parent)].first_child = new_branching_node;
	} else if ((*prev_child) > 0) {
		stree->tbranch[(*prev_child)].branch_brother =
			new_branching_node;
	} else { /* (*prev_child) < 0 */
		stree->tleaf[-(*prev_child)].next_brother = new_branching_node;
	}
	stree->tbranch[new_branching_node].first_child = (*child);
	/* the following value will be set right away */
	stree->tbranch[new_branching_node].branch_brother = 0;
	/* the following value will be set later */
	stree->tbranch[new_branching_node].suffix_link = 0;
	stree->tbranch[new_branching_node].depth =
		stree->tbranch[(*parent)].depth +
		(unsigned_integral_type)(last_match_position);
	stree->tbranch[new_branching_node].head_position = new_head_position;
	if ((*child) > 0) {
		stree->tbranch[new_branching_node].branch_brother =
			stree->tbranch[(*child)].branch_brother;
		stree->tbranch[(*child)].branch_brother = 0;
	} else { /* (*child) < 0 */
		stree->tbranch[new_branching_node].branch_brother =
			stree->tleaf[-(*child)].next_brother;
		stree->tleaf[-(*child)].next_brother = 0;
	}
	(*parent) = new_branching_node;
	/*
	 * Now we adjust the "child" and the "prev_child" variables
	 * for the later creation of a new leaf.
	 */
	if (child_first == 1) {
		(*prev_child) = (*child);
		(*child) = 0;
	} else {
		(*prev_child) = 0;
	}
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
int st_slli_simple_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length,
		const suffix_tree_slli *stree) {
	signed_integral_type child = 0;
	unsigned_integral_type parents_depth = 0;
	unsigned_integral_type childs_depth = 0;
	size_t childs_offset = 0;
	if (starting_node <= 0) {
		fprintf(stderr,	"Error: The traveral has to start from "
				"a branching node, but the starting node "
				"is %d!", starting_node);
		return (1);
	}
	child = stree->tbranch[starting_node].first_child;
	parents_depth = stree->tbranch[starting_node].depth;
	while (child != 0) {
		if (child > 0) {
			childs_depth = stree->tbranch[child].depth;
			childs_offset = stree->tbranch[child].head_position;
			st_print_edge(stream, 0,
					(signed_integral_type)(0),
					(signed_integral_type)(0),
					(signed_integral_type)(0),
					parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, internal_text_encoding,
					text);
			st_slli_simple_traverse_from(stream, child,
					log10bn, log10l,
					internal_text_encoding, text, length,
					stree);
			/*
			 * there would be some overhead if we called
			 * the next child function, so we do it simply
			 */
			child = stree->tbranch[child].branch_brother;
		} else {
			childs_depth = (unsigned_integral_type)((length + 2)
				- (size_t)(-child));
			childs_offset = (unsigned_integral_type)(-child);
			st_print_edge(stream, 1,
					(signed_integral_type)(0),
					child, (signed_integral_type)(0),
					parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, internal_text_encoding,
					text);
			/*
			 * there would be some overhead if we called
			 * the next child function, so we do it simply
			 */
			child = stree->tleaf[-child].next_brother;
		}
	}
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
int st_slli_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length,
		const suffix_tree_slli *stree) {
	signed_integral_type child = 0;
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
	child = stree->tbranch[starting_node].first_child;
	parents_depth = stree->tbranch[starting_node].depth;
	while (child != 0) {
		if (child > 0) {
			childs_suffix_link = stree->tbranch[child].suffix_link;
			childs_depth = stree->tbranch[child].depth;
			childs_offset = stree->tbranch[child].head_position;
			st_print_edge(stream, 0, starting_node, child,
					childs_suffix_link, parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, internal_text_encoding,
					text);
			st_slli_traverse_from(stream, child, log10bn, log10l,
					internal_text_encoding, text, length,
					stree);
			/*
			 * there would be some overhead if we called
			 * the next child function, so we do it simply
			 */
			child = stree->tbranch[child].branch_brother;
		} else {
			childs_depth = (unsigned_integral_type)((length + 2)
				- (size_t)(-child));
			childs_offset = (unsigned_integral_type)(-child);
			st_print_edge(stream, 1, starting_node, child,
					0, parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, internal_text_encoding,
					text);
			/*
			 * there would be some overhead if we called
			 * the next child function, so we do it simply
			 */
			child = stree->tleaf[-child].next_brother;
		}
	}
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
int st_slli_traverse (FILE *stream,
		const char *internal_text_encoding,
		int traversal_type,
		const character_type *text,
		size_t length,
		const suffix_tree_slli *stree) {
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
		if (st_slli_traverse_from(stream, start_node,
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
		if (st_slli_simple_traverse_from(stream, start_node,
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

/** A function which deallocates the memory used by the suffix tree.
 *
 * @param
 * stree	the actual suffix tree to be "deleted" (in more detail:
 * 		all the data it contains will be erased
 * 		and the memory it occupies will be deallocated)
 *
 * @return	This function always returns zero.
 */
int st_slli_delete (suffix_tree_slli *stree) {
	printf("Deleting the suffix tree\n");
	free(stree->tleaf);
	stree->tleaf = NULL;
	free(stree->tbranch);
	stree->tbranch = NULL;
	/*
	 * maintaining the suffix tree struct
	 * constistent with its definition
	 */
	stree->tbranch_size = 0;
	stree->branching_nodes = 0;
	printf("Successfully deleted!\n");
	return (0);
}
