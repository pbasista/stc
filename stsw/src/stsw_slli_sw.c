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
 * SLLI sliding window-related functions implementation.
 * This file contains the implementation of the additional
 * sliding window-related functions, which are used by the functions,
 * which construct and maintain the suffix tree over a sliding window
 * using the implementation type SLLI with backward pointers.
 */
#include "stsw_slli_sw.h"

#include <stdlib.h>

/* error-checking function */

/**
 * A function which checks whether the head positions
 * of all the branching nodes are valid inside the currently active part
 * of the sliding window.
 *
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If all the head positions are valid, this function returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int stsw_slli_check_head_positions (const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_slli *stsw) {
	/* the root does not need to be checked */
	size_t i = 2;
	for (; i < stsw->tbranch_size; ++i) {
		if (stsw->tbranch[i].depth != 0) {
			if (stsw_validate_sw_offset((size_t)
					(stsw->tbranch[i].head_position),
					tfsw) != 0) {
				fprintf(stderr, "check_head_positions:\n"
						"The branching node %zu "
						"has an invalid head "
						"position of %u\n", i,
						stsw->tbranch[i].
						head_position);
				return (1);
			}
		}
	}
	return (0);
}

/* edge label maintenance supporting function */

/**
 * A function which updates the edge labels on the path
 * from the specified branching node to the root.
 *
 * @param
 * parent	The branching node, from which we want
 * 		to start the updating of the edge labels.
 * @param
 * new_head_position	The head position, which will be assigned to the
 * 			provided branching node, if necessary.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully update the branch from the provided
 * 		branching node all the way up to the root,
 * 		this function returns zero (0).
 * 		Otherwise, in case of any error,
 * 		a positive error number is returned.
 */
int stsw_slli_path_update (signed_integral_type parent,
		unsigned_integral_type new_head_position,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw) {
	/* the begin of the currently valid part of the sliding window */
	size_t vp_window_begin = tfsw->ap_window_begin;
	signed_integral_type grandpa = 0;
	/* if the number of the provided branching node is not valid */
	if (parent <= 1) {
		fprintf(stderr,	"stsw_slli_path_update:\n"
				"Error: We can only start the updating "
				"from the non-root, branching node.\n"
				"The provided node number, however, "
				"is %d. Exiting!\n", parent);
		return (1); /* a failure */
	}
	/*
	 * from now on, parent > 1
	 *
	 * The valid sliding window offsets are not only
	 * the ones inside the currently active part
	 * of the sliding window. The offsets
	 * which are at most max_ap_window_size before
	 * the ap_window_begin are valid as well.
	 */
	if (vp_window_begin > tfsw->max_ap_window_size) {
		vp_window_begin -= tfsw->max_ap_window_size;
	} else {
		vp_window_begin = tfsw->total_window_size +
			tfsw->ap_window_begin -
			tfsw->max_ap_window_size;
	}
	/*
	 * Here, it is possible that vp_window_begin ==
	 * tfsw->ap_window_end.
	 */
	if (vp_window_begin < tfsw->ap_window_end) {
		if (stsw->tbranch[parent].head_position < new_head_position) {
			stsw->tbranch[parent].head_position =
				new_head_position;
		} else {
			new_head_position =
				stsw->tbranch[parent].head_position;
		}
	/* vp_window_begin >= tfsw->ap_window_end */
	} else if (vp_window_begin <=
			stsw->tbranch[parent].head_position) {
		if (stsw->tbranch[parent].head_position <
				new_head_position) {
			stsw->tbranch[parent].head_position =
				new_head_position;
		} else if (new_head_position < tfsw->ap_window_end) {
			stsw->tbranch[parent].head_position =
				new_head_position;
		} else {
			new_head_position =
				stsw->tbranch[parent].head_position;
		}
	/* vp_window_begin > stsw->tbranch[parent].head_position */
	} else if (new_head_position < tfsw->ap_window_end) {
		if (stsw->tbranch[parent].head_position <
				new_head_position) {
			stsw->tbranch[parent].head_position =
				new_head_position;
		} else {
			new_head_position =
				stsw->tbranch[parent].head_position;
		}
	} else {
		new_head_position =
			stsw->tbranch[parent].head_position;
	}
	/*
	 * we need not to worry about the credit bits,
	 * because when using this edge label maintenance method,
	 * no credit counters at all are in use
	 */
	grandpa = stsw->tbranch[parent].parent;
	if (grandpa <= 0) {
		fprintf(stderr,	"stsw_slli_path_update:\n"
				"Error: The parent (%d) of the provided\n"
				"branching node (%d) is not a branching "
				"node. Exiting!\n", grandpa, parent);
		return (2); /* a failure */
	/* if the parent of the 'parent' node is not the root */
	} else if (grandpa != 1) {
		/* we recursively call this function on it */
		if (stsw_slli_path_update(grandpa, new_head_position,
					tfsw, stsw) > 0) {
			fprintf(stderr,	"stsw_slli_path_update:\n"
					"Error: The recursive call "
					"to the function\n"
					"stsw_slli_path_update "
					"failed. Exiting!\n");
			return (3); /* a failure */
		}
	}
	return (0); /* success */
}

/* edge label maintenance handling functions */

/**
 * A function which updates the edge labels in the provided suffix tree
 * so that they point only to the currently active part of the sliding window.
 *
 * This type of the edge label maintenance has originally been
 * suggested by M. Senft.
 *
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully update all the edge labels
 * 		and if all the suffixes in the suffix tree end
 * 		at the leaf nodes, 0 is returned.
 * 		If not all the suffixes in the suffix tree
 * 		end at a leaf node, but nevertheless we have updated
 * 		all the branches ending at all the leaves that are currently
 * 		present in the suffix tree, (-1) is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_slli_edge_labels_batch_update (const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw) {
	size_t i = stsw->tleaf_first;
	unsigned_integral_type leafs_sw_offset =
		(unsigned_integral_type)(tfsw->ap_window_begin);
	if (stsw->tleaf_first > stsw->tleaf_last) {
		for (; i <= stsw->tleaf_size; ++i) {
			if (stsw->tleaf[i].parent > 1) {
				if (stsw_slli_path_update(
							stsw->tleaf[i].parent,
							leafs_sw_offset,
							tfsw, stsw) > 0) {
					fprintf(stderr, "stsw_slli_edge_labels"
							"_batch_update:\n"
							"Error: Could not "
							"update the edge "
							"labels\non the path "
							"from the leaf "
							"(-%zu) to the root. "
							"Exiting!\n", i);
					return (1);
				}
			} else if (stsw->tleaf[i].parent == 0) {
				return (-1);
				// FIXME: Is it correct to end right now?
				// Are we sure that all the following leaves
				// are not present in the current suffix tree?
				// Yes, we are, but explain why!
			}
			if (leafs_sw_offset == tfsw->total_window_size) {
				leafs_sw_offset = 1;
			} else {
				++leafs_sw_offset;
			}
		}
		/* here i == tfsw->tleaf_size + 1 */
		i = 1;
	}
	for (; i <= stsw->tleaf_last; ++i) {
		if (stsw->tleaf[i].parent > 1) {
			if (stsw_slli_path_update(stsw->tleaf[i].parent,
						leafs_sw_offset,
						tfsw, stsw) > 0) {
				fprintf(stderr, "stsw_slli_edge_labels"
						"_batch_update:\n"
						"Error: Could not update "
						"the edge labels\n"
						"on the path from the leaf "
						"(-%zu) to the root. "
						"Exiting!\n", i);
				return (2);
			}
		} else if (stsw->tleaf[i].parent == 0) {
			return (-1);
			// FIXME: Is it correct to end right now?
			// Are we sure that all the following leaves
			// are not present in the current suffix tree?
			// Yes, we are, but explain why!
		}
		if (leafs_sw_offset == tfsw->total_window_size) {
			leafs_sw_offset = 1;
		} else {
			++leafs_sw_offset;
		}
	}
	return (0);
}

/**
 * A function which updates the head position and increments
 * the credit counter in the provided branching node.
 * If the credit was previously zero, this function
 * does not do anything else. Otherwise, if the credit
 * was previously one, it is set to zero and this function
 * is recursively called on the provided branching node's parent.
 *
 * @param
 * parent	The branching node, for which we want to increment its credit.
 * @param
 * new_head_position	The head position, which will be assigned to the
 * 			provided branching node if necessary.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If the provided branching node have had its credit incremented,
 * 		this function returns (-1).
 * 		If the provided branching node have had its credit decremented
 * 		and the possible recursive call has finished successfully,
 * 		this function returns (-2).
 * 		Otherwise, in case of any error,
 * 		a positive error number is returned.
 */
int stsw_slli_send_credit (signed_integral_type parent,
		unsigned_integral_type new_head_position,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw) {
	signed_integral_type grandpa = 0;
	/* if the parent is not the non-root branching node */
	if (parent <= 1) {
		fprintf(stderr,	"stsw_slli_send_credit:\n"
				"Error: The credit can only be sent "
				"to a non-root, branching node.\n"
				"The provided node number, however, "
				"is %d. Exiting!\n", parent);
		return (1); /* a failure */
	}
	/* from now on, parent > 1 */
	if (tfsw->ap_window_begin < tfsw->ap_window_end) {
		if (stsw->tbranch[parent].head_position < new_head_position) {
			stsw->tbranch[parent].head_position =
				new_head_position;
		} else {
			new_head_position =
				stsw->tbranch[parent].head_position;
		}
	/* tfsw->ap_window_begin >= tfsw->ap_window_end */
	} else if (tfsw->ap_window_begin <=
			stsw->tbranch[parent].head_position) {
		if (stsw->tbranch[parent].head_position <
				new_head_position) {
			stsw->tbranch[parent].head_position =
				new_head_position;
		} else if (new_head_position < tfsw->ap_window_end) {
			stsw->tbranch[parent].head_position =
				new_head_position;
		} else {
			new_head_position =
				stsw->tbranch[parent].head_position;
		}
	/* tfsw->ap_window_begin > stsw->tbranch[parent].head_position */
	} else if (new_head_position < tfsw->ap_window_end) {
		if (stsw->tbranch[parent].head_position <
				new_head_position) {
			stsw->tbranch[parent].head_position =
				new_head_position;
		} else {
			new_head_position =
				stsw->tbranch[parent].head_position;
		}
	} else {
		new_head_position =
			stsw->tbranch[parent].head_position;
	}
	grandpa = stsw->tbranch[parent].parent;
	if (grandpa == 0) {
		fprintf(stderr,	"stsw_slli_send_credit:\n"
				"Error: The parent of the provided\n"
				"branching node (%d) is zero. Exiting!\n",
				parent);
		return (2); /* a failure */
	/* if the provided branching node's credit counter is zet to zero */
	} else if (grandpa > 0) {
		/* we set it to one */
		stsw->tbranch[parent].parent = -grandpa;
		return (-1); /* success */
	/*
	 * otherwise, the provided branching node's credit counter
	 * is set to one
	 */
	} else { /* grandpa < 0 */
		grandpa = -grandpa;
		/* we set it to zero */
		stsw->tbranch[parent].parent = grandpa;
		/* and if the parent of the 'parent' node is not the root */
		if (grandpa != 1) {
			/* we recursively call this function on it */
			if (stsw_slli_send_credit(grandpa, new_head_position,
						tfsw, stsw) > 0) {
				fprintf(stderr,	"stsw_slli_send_credit:\n"
						"Error: The recursive call "
						"to the function\n"
						"stsw_slli_send_credit "
						"failed. Exiting!\n");
				return (3); /* a failure */
			}
		}
		return (-2); /* success */
	}
}

/* sliding window-related handling function */

/**
 * A function which deletes the longest suffix from the suffix tree
 * while preserving all the shorter suffixes in the suffix tree.
 *
 * @param
 * starting_position	The position in the sliding window of the first
 * 			character of the first suffix to be prolonged
 * 			like in the call to the function
 * 			stsw_slli_ukkonen_prolong_suffixes.
 * 			When this function successfully finishes, it will be
 * 			set to the starting position of the suffix,
 * 			from which the prolonging of the suffixes
 * 			can safely continue.
 * @param
 * active_index		The position in the sliding window from which
 * 			the possible prolonging of the suffixes like in
 * 			the function stsw_slli_ukkonen_prolong_suffixes,
 * 			would actually start.
 * 			When this function successfully finishes, it will be
 * 			set to the position, from which the prolonging
 * 			of the suffixes can safely continue.
 * @param
 * active_node	The node from which the possible prolonging, like in
 * 		the function stsw_slli_ukkonen_prolong_suffixes,
 * 		would actively start.
 * 		Actually this is either the active point or the closest parent
 * 		of the active point.
 * 		When this function successfully finishes, it will be
 * 		set to the node, from which the prolonging
 * 		of the suffixes can safely continue.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully delete the longest suffix
 * 		from the suffix tree, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int stsw_slli_delete_longest_suffix (size_t *starting_position,
		size_t *active_index,
		signed_integral_type *active_node,
		text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw) {
	/* the deepest leaf node */
	signed_integral_type deepest_leaf = -(signed_integral_type)
		(stsw->tleaf_first);
	/* the parent of the deepest leaf node */
	signed_integral_type parent = stsw->tleaf[stsw->tleaf_first].parent;
	/* the grandparent of the deepest leaf node */
	signed_integral_type grandpa = 0; /* will be set later */
	/* another child of the parent of the deepest leaf node */
	signed_integral_type other_child = 0;
	/* the variable used for the branching operations */
	signed_integral_type child = 0;
	/* the variable used for the branching operations */
	signed_integral_type prev_child = 0;
	/* the sliding window offset of the current child node */
	unsigned_integral_type childs_sw_offset = 0;
	/* the depth of the new active point */
	size_t target_depth = 0;
	/*
	 * the lower bound of the number of children
	 * of the parent of the deepest leaf node
	 */
	size_t children_lb = 0;
	/* the return value from the descending function */
	int retval = 0;
	/*
	 * If this variable evaluates to true, it means that the first letter
	 * of the edge leading to the deepest leaf node matches
	 * the edge at the active_index of the sliding window.
	 */
	int initial_edge_letter_matches = 0;
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	character_type letter = L'\0';
#else
	character_type letter = '\0';
#endif
	if (parent <= 0) {
		fprintf(stderr, "Error: Could not delete the leaf (%d) "
				"representing the longest suffix.\n"
				"The number of its parent (%d) "
				"is not valid. Exiting!\n",
				deepest_leaf, parent);
		return (1);
	}
	/*
	 * At first we would like to check whether the current suffix tree
	 * contains the active point. We do it by checking if the active index
	 * has already reached the end of the currently active part
	 * of the sliding window.
	 */
	if ((*active_index) == tfsw->ap_window_end) {
		/*
		 * If it has, it means that the active point is not present
		 * in the current suffix tree. In this case,
		 * we need not to do anything here, because the active node
		 * can not be the parent of the deepest leaf node.
		 */
	} else {
		/*
		 * Otherwise, we need to check whether the active point
		 * is the parent of the deepest leaf node.
		 * Based on that information we decide
		 * whether we will delete the edge leading to the deepest
		 * leaf node or we will just make it shorter.
		 *
		 * At first, we obtain the first letter of the edge
		 * leading to the deepest leaf node.
		 */
		if (stsw_slli_edge_letter(parent, &letter, deepest_leaf,
					tfsw, stsw) > 0) {
			fprintf(stderr, "Error: Could not get the first "
					"letter of an edge "
					"P(%d)--\"?\"-->C(%d)"
					"\nleading to the deepest leaf "
					"node. Exiting!\n",
					parent, deepest_leaf);
			return (2);
		}
		/*
		 * Then we just compare it to the letter
		 * at the active_index of the sliding window.
		 * If these letters match and if parent == (*active_node),
		 * we are sure that the edge leading to the deepest
		 * leaf node needs to be shortened instead of deleted.
		 */
		if (tfsw->text_window[(*active_index)] == letter) {
			initial_edge_letter_matches = 1;
		}
	}
	/*
	 * Now we need to get the previous brother
	 * of the deepest leaf node, if it exists.
	 * Just then we will be able to delete or shorten
	 * the edge leading to the deepest leaf node.
	 */
	child = stsw->tbranch[parent].first_child;
	prev_child = 0;
	if (child == 0) {
		fprintf(stderr,	"stsw_slli_delete_longest_suffix:\n"
				"Error: The alleged parent (%d) "
				"of the deepest leaf node (%d)\n"
				"does not have any children. Exiting!\n",
				parent, deepest_leaf);
		return (3); /* branching failed (no children) */
	}
	/*
	 * while the currently examined child is nonzero
	 * and differs from the deepest leaf
	 */
	while ((child != 0) && (child != deepest_leaf)) {
		++children_lb;
		/* we advance to the next child of the parent node */
		stsw_slli_next_child(&child, &prev_child, stsw);
	}
	/* if we stopped beyond the last child of the parent node */
	if (child == 0) {
		/*
		 * we have examined all the children,
		 * but none seemed to match the deepest leaf node
		 */
		fprintf(stderr,	"stsw_slli_delete_longest_suffix:\n"
				"Error: The deepest leaf node (%d) "
				"is not a child\nof its alleged "
				"parent (%d). Exiting!\n",
				deepest_leaf, parent);
		return (4); /* branching failed (no matching edge) */
	}
	/*
	 * (child != 0) => branching succeeded (matching edge found)
	 *
	 * If there is an implicit node inside an edge leading
	 * to the deepest leaf node, we do not delete this edge.
	 * Instead, we just shorten it.
	 */
	if ((parent == (*active_node)) &&
			(initial_edge_letter_matches == 1)) {
		/*
		 * Here, the 'parent' node is the parent of the deepest
		 * leaf node, child == deepest_leaf
		 * and prev_child is the child of the parent node
		 * just before the deepest_leaf,
		 * or zero if the deepest_leaf is the first child
		 * of the parent node.
		 *
		 * Getting the number of the leaf node, at which the edge
		 * leading to the leaf representing the longest suffix
		 * will end after being shortened.
		 */
		child = (signed_integral_type)(stsw->tleaf_first +
			stsw_get_leafs_depth_order((*starting_position),
					tfsw));
		if (child > (signed_integral_type)
				(stsw->tleaf_size)) {
			child -= (signed_integral_type)
				(stsw->tleaf_size);
		}
		child = -child;
		/*
		 * we replace the current edge leading to the deepest
		 * leaf node with the shorter one
		 */
		if (prev_child == 0) {
			stsw->tbranch[parent].first_child = child;
		} else if (prev_child > 0) {
			stsw->tbranch[prev_child].branch_brother = child;
		} else { /* prev_child < 0 */
			stsw->tleaf[-prev_child].next_brother = child;
		}
		/* setting the next brother of the new leaf node */
		stsw->tleaf[-child].next_brother =
			stsw->tleaf[-deepest_leaf].next_brother;
		/* and the parent of the new leaf node */
		stsw->tleaf[-child].parent = parent;
		/*
		 * and clearing the leaf record
		 * of the formerly deepest leaf node
		 */
		stsw->tleaf[-deepest_leaf].parent = 0;
		stsw->tleaf[-deepest_leaf].next_brother = 0;
		/*
		 * getting the sliding window offset of the first character
		 * of the suffix ending at the current child node
		 */
		childs_sw_offset = (unsigned_integral_type)
			(stsw_slli_get_leafs_sw_offset(child, tfsw, stsw));
		// FIXME: Rename head_position to something new and better!
		/* updating the head position of the parent node */
		if (tfsw->elm_method == 1) { /* batch update by M. Senft */
			/* we don't do anything there */
		/* Fiala's and Greene's method */
		} else if (tfsw->elm_method == 2) {
			if (parent != 1) {
				if (stsw_slli_send_credit(parent,
							childs_sw_offset,
							tfsw, stsw) > 0) {
					fprintf(stderr, "Error: Could not "
							"send a credit "
							"to the parent (%d)\n"
							"of the leaf (%d), "
							"at which the newly "
							"shortened edge ends."
							" Exiting!\n",
							parent, child);
					return (5);
				}
			}
		} else if (tfsw->elm_method == 3) { /* Larsson's method */
			abort(); // FIXME TODO Finish this!
		} else { /* unknown edge label maintenance method */
			fprintf(stderr, "Error: The sliding window has "
					"an unknown value (%d)\n"
					"of the edge label maintenance "
					"method set. Exiting!\n",
					tfsw->elm_method);
			return (6);
		}
		/* we adjust the starting position */
		if ((*starting_position) == tfsw->total_window_size) {
			(*starting_position) = 1;
		} else {
			++(*starting_position);
		}
		/* if the active node is the root */
		if ((*active_node) == 1) {
			/*
			 * we can't use the suffix link transition,
			 * but we have to adjust the active_index
			 */
			if ((*active_index) == tfsw->total_window_size) {
				(*active_index) = 1;
			} else {
				++(*active_index);
			}
		} else { /* the active node is not the root */
			/*
			 * We can use a suffix link transition
			 * and we need not to adjust the active_index,
			 * because its current value is correct
			 * for the next (shorter) suffix.
			 */
			(*active_node) = stsw->tbranch[(*active_node)].
				suffix_link;
		}
		/* (*starting_position) can be equal to tfsw->ap_window_end */
		if ((*starting_position) == tfsw->ap_window_end) {
			/*
			 * In this case, target_depth == 0 and we can skip
			 * the call to the stsw_slli_go_down function.
			 *
			 * We also do not need to adjust the active index
			 * of the next prolonged suffix,
			 * because its current value is correct.
			 */
			goto dls_end;
		} else if ((*starting_position) < tfsw->ap_window_end) {
			target_depth = tfsw->ap_window_end -
				(*starting_position);
		} else { /* (*starting_position) > tfsw->ap_window_end */
			target_depth =
				tfsw->total_window_size +
				tfsw->ap_window_end -
				(*starting_position);
		}
		/*
		 * Here, we need to have the active_index updated.
		 *
		 * if we were able to locate the desired depth
		 */
		if ((retval = stsw_slli_go_down((*active_node), active_node,
						(unsigned_integral_type)
						(target_depth),
						active_index,
						tfsw, stsw)) == 0) {
			/*
			 * The descending down has been successful,
			 * so we need not to do anything here.
			 * The (*active_node) and (*active_index)
			 * have just been set correctly.
			 */
		} else if (retval == (-1)) {
			/*
			 * the new active point does not exist yet,
			 * but we found its position and set
			 * the active_node and active_index
			 * variables appropriately
			 */
		} else { /* something went wrong */
			fprintf(stderr,	"stsw_slli_delete_"
					"longest_suffix:\n");
			fprintf(stderr,	"Error: Can not find "
					"the new active_node. "
					"Exiting!\n");
			return (7); /* a failure */
		}
		/*
		 * The active_node might have changed
		 * (in the stsw_slli_go_down function), so we adjust
		 * the active index of the next prolonged suffix.
		 */
		(*active_index) = (*starting_position) +
			(size_t)(stsw->tbranch[(*active_node)].depth);
		if ((*active_index) > tfsw->total_window_size) {
			(*active_index) -= tfsw->total_window_size;
		}
		goto dls_end;
	/*
	 * the end of the block where we have shortened
	 * the edge leading to the deepest_leaf
	 */
	}
	/*
	 * Here, the 'parent' node is the parent of the deepest
	 * leaf node, child == deepest_leaf
	 * and prev_child is the child of the parent node
	 * just before the deepest_leaf,
	 * or zero if the deepest_leaf is the first child
	 * of the parent node.
	 *
	 * We try to delete the edge leading to the deepest leaf.
	 */
	child = stsw->tleaf[-deepest_leaf].next_brother;
	if (prev_child == 0) {
		stsw->tbranch[parent].first_child = child;
	} else if (prev_child > 0) {
		stsw->tbranch[prev_child].branch_brother = child;
	} else { /* prev_child < 0 */
		stsw->tleaf[-prev_child].next_brother = child;
	}
	/*
	 * The parent of the 'child' node, if it is nonzero,
	 * already is the 'parent' node,
	 * so setting it should not be necessary.
	 *
	 * deleting the deepest leaf by clearing its leaf record
	 */
	stsw->tleaf[-deepest_leaf].parent = 0;
	stsw->tleaf[-deepest_leaf].next_brother = 0;
	/* if parent is the root */
	if (parent == 1) {
		/*
		 * we need not to maintain the edge labels
		 * nor watch for a single child of the root node,
		 * so we are almost finished
		 */
		goto dls_end;
	}
	/*
	 * Now we need to decide whether the parent node
	 * has only one child remaining.
	 *
	 * if the deepest_leaf has been the last child of its parent
	 */
	if (child == 0) {
		other_child = prev_child;
	} else if (child > 0) {
		++children_lb;
		other_child = child;
		if (stsw->tbranch[child].branch_brother != 0) {
			++children_lb;
		}
	} else { /* child < 0 */
		++children_lb;
		other_child = child;
		if (stsw->tleaf[-child].next_brother != 0) {
			++children_lb;
		}
	}
	/*
	 * Here, we can be sure that the parent is not the root.
	 * After the deepest leaf node has been deleted,
	 * we must check if the parent node has only one child remaining.
	 * In that case, the parent node must be deleted as well.
	 *
	 * We check it by examining the lower bound of the number of children
	 * of the parent node. If it is less than two,
	 * we are sure that the parent node has only one child remaining
	 * and that this child is stored in the variable other_child.
	 */
	if (children_lb < 2) {
		/*
		 * now we know that the other_child
		 * is the only remaining child of the parent node
		 *
		 * updating the other_child's sliding window offset
		 * for the possible edge label maintenance
		 */
		if (other_child == 0) {
			fprintf(stderr,	"Error: The only remaining child "
					"of the parent (%d)\nof the deepest "
					"leaf node (%d) is zero. Exiting!\n",
					parent, deepest_leaf);
			return (8);
		} else if (other_child > 0) {
			/* the other_child is a branching node */
			childs_sw_offset =
				stsw->tbranch[other_child].head_position;
		} else { /* other_child < 0 */
			/* the other_child is a leaf */
			childs_sw_offset = (unsigned_integral_type)
				(stsw_slli_get_leafs_sw_offset(other_child,
								tfsw, stsw));
		}
		/*
		 * here we know that the parent node is not the root
		 * and that the other_child is not zero
		 *
		 * We update the parent pointer in the other_child node.
		 */
		if (tfsw->elm_method == 1) { /* batch update by M. Senft */
			/* we don't use any credit bits */
			grandpa = stsw->tbranch[parent].parent;
			/*
			 * We have to update the parent pointer
			 * in the 'other_child' node.
			 */
			if (other_child > 0) {
				/* the other_child is a branching node */
				stsw->tbranch[other_child].parent = grandpa;
			} else { /* other_child < 0 */
				/* the other_child is a leaf */
				stsw->tleaf[-other_child].parent = grandpa;
			}
		/*
		 * we suppose that in all the other edge label
		 * maintenance methods we have to use the credit bits
		 */
		} else { /* the other edge label maintenance methods */
			/*
			 * FIXME: Attention! Is it important whether or not
			 * the deleted branching node
			 * has had its credit set to one?
			 */
			grandpa = abs(stsw->tbranch[parent].parent);
			/*
			 * We have to update the parent pointer
			 * in the 'other_child' node.
			 */
			if (other_child > 0) {
				/*
				 * the other_child is a branching node,
				 * so we must preserve its credit
				 */
				if (stsw->tbranch[other_child].parent > 0) {
					stsw->tbranch[other_child].parent =
						grandpa;
				} else { /* stsw->tbranch[child].parent < 0 */
					stsw->tbranch[other_child].parent =
						-grandpa;
				}
			} else { /* other_child < 0 */
				/* the other_child is a leaf */
				stsw->tleaf[-other_child].parent = grandpa;
			}
		}
		/*
		 * We try to replace the edge leading from the grandpa
		 * to the parent with the direct edge leading
		 * from the grandpa to the other_child.
		 *
		 * At first, we try to find the child of the grandpa,
		 * which preceeds the parent node.
		 */
		if (stsw_slli_get_prev_child(grandpa, parent,
					&child, &prev_child, stsw) > 0) {
			fprintf(stderr,	"stsw_slli_delete_longest_suffix:\n"
					"Error: Could not get the child "
					"of the grandpa (%d)\nof the deepest "
					"leaf node, which preceeds "
					"the parent (%d)\nof the deepest "
					"leaf node (%d). Exiting!\n",
					grandpa, parent, deepest_leaf);
			return (9);
		}
		if (child == 0) {
			/*
			 * we have examined all the children of the grandpa,
			 * but none seemed to match the parent
			 * of the deepest leaf node
			 */
			fprintf(stderr,	"stsw_slli_delete_longest_suffix:\n"
					"Error: The parent (%d) of the "
					"deepest leaf node is not a child\n"
					"of the alleged grandparent (%d) "
					"of the deepest leaf node!\n",
					parent, grandpa);
			return (10); /* branching failed (no matching edge) */
		}
		/*
		 * Here the child == parent is nonzero, which means that
		 * the branching has succeeded (matching edge found).
		 */
		if (prev_child == 0) {
			stsw->tbranch[grandpa].first_child = other_child;
		} else if (prev_child > 0) {
			stsw->tbranch[prev_child].branch_brother = other_child;
		} else { /* prev_child < 0 */
			stsw->tleaf[-prev_child].next_brother = other_child;
		}
		/* We already know that the other_child is nonzero. */
		if (other_child > 0) {
			/* the other_child is a branching node */
			stsw->tbranch[other_child].branch_brother =
				stsw->tbranch[parent].branch_brother;
		} else { /* other_child < 0 */
			/* the other_child is a leaf */
			stsw->tleaf[-other_child].next_brother =
				stsw->tbranch[parent].branch_brother;
		}
		/*
		 * In this case, despite that the description
		 * of the active point might be adjusted,
		 * the active point itself does not change.
		 * That's why we do not need to call
		 * the function stsw_slli_go_down.
		 *
		 * if we are about to delete the active node
		 */
		if ((*active_node) == parent) {
			/*
			 * We have to find the new active node.
			 * At this point, we already know that
			 * the parent is not the root,
			 * so we can safely use its parent, or grandpa.
			 */
			(*active_node) = grandpa;
			/*
			 * We also have to adjust the active index
			 * of the next prolonged suffix.
			 */
			(*active_index) = (*starting_position) +
				(size_t)(stsw->tbranch[(*active_node)].depth);
			if ((*active_index) > tfsw->total_window_size) {
				(*active_index) -= tfsw->total_window_size;
			}
		}
		/*
		 * deleting the branching node 'parent'
		 * by clearing its branching record
		 */
		stsw->tbranch[parent].parent = 0;
		stsw->tbranch[parent].first_child = 0;
		stsw->tbranch[parent].branch_brother = 0;
		stsw->tbranch[parent].depth = 0;
		stsw->tbranch[parent].head_position = 0;
		stsw->tbranch[parent].suffix_link = 0;
		/*
		 * storing the index of the recently deleted branching node
		 * to the table of indices of the deleted branching nodes
		 */
		stsw->tbranch_deleted[stsw->tbdeleted_records] =
			(unsigned_integral_type)(parent);
		/* and incrementing the number of deleted branching nodes */
		++stsw->tbdeleted_records;
		/*
		 * remembering the maximum used size of the table of indices
		 * of the branching records deleted from the table tbranch
		 */
		if (stsw->tbdeleted_records > stsw->max_tbdeleted_index) {
			stsw->max_tbdeleted_index = stsw->tbdeleted_records;
		}
		/*
		 * updating the parent node so that we can send a credit to it
		 * during the following edge label maintenance
		 */
		parent = grandpa;
	} else { /* the parent has more than one child remaining */
		/*
		 * Preparing the sliding window offset of the beginning
		 * of the suffix ending at the deepest leaf node.
		 * This offset will be used for the edge label maintenance
		 * in the case, when the parent node will not be deleted.
		 */
		childs_sw_offset = (unsigned_integral_type)
			(tfsw->ap_window_begin);
	}
	// FIXME: Rename head_position to something new and better!
	/* updating the head position of the parent node */
	if (tfsw->elm_method == 1) { /* batch update by M. Senft */
		/* we don't do anything there */
	/* Fiala's and Greene's method */
	} else if (tfsw->elm_method == 2) {
		if (parent != 1) {
			if (stsw_slli_send_credit(parent, childs_sw_offset,
						tfsw, stsw) > 0) {
				fprintf(stderr, "Error: Could not send "
						"a credit to the former "
						/*
						 * this might either be
						 * the parent
						 * or the grandparent
						 */
						"ancestor (%d)\n"
						"of the deepest leaf "
						"node (-%zu). Exiting!\n",
						parent, stsw->tleaf_first);
				return (11);
			}
		}
	} else if (tfsw->elm_method == 3) { /* Larsson's method */
		abort(); // FIXME TODO Finish this!
	} else { /* unknown edge label maintenance method */
		fprintf(stderr, "Error: The suffix tree has "
				"an unknown value (%d)\n"
				"of the edge label maintenance "
				"method set. Exiting!\n",
				tfsw->elm_method);
		return (12);
	}
dls_end:
	/* decreasing the usable size of the table tleaf */
	if (stsw->tleaf_first == stsw->tleaf_size) {
		stsw->tleaf_first = 1;
	} else {
		++stsw->tleaf_first;
	}
	return (0);
}
