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
 * SLLI functions implementation.
 * This file contains the implementation of the functions,
 * which use both the classic or the minimized branching
 * suffix link simulation technique
 * These functions are used for the construction and maintenance
 * of the suffix tree over a sliding window
 * using the implementation type SLLI with backward pointers.
 */
#include "stsw_slli.h"

#ifdef STSW_USE_PTHREAD

#include <errno.h> /* for pthread-related error handling */

#endif

#include <stdio.h>
#include <stdlib.h>

/* supporting functions */

/**
 * A function which simulates the suffix link
 * using a classic top-down approach.
 *
 * @param
 * grandpa	the parent of the "parent" node
 * @param
 * parent	The node for which we need to find its suffix link.
 * 		When this function successfully returns, it will be set
 * 		either to the suffix link's target itself (if it already
 * 		exists) or to the parent of the new suffix link's target
 * 		(if the suffix link's target itself does not exist yet).
 * @param
 * target_depth	the depth of the suffix link's target
 * @param
 * starting_position	the starting position in the sliding window
 * 			of the original suffix,
 * 			which has the prefix corresponding
 * 			to the path from the root to the node "parent".
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If the suffix link simulation has been successful,
 * 		this function returns 0.
 * 		If the suffix link's target does not exist yet, we set up
 * 		the parent variable appropriately and return (-1).
 * 		If an error occurs, a positive error number is returned.
 */
int stsw_slli_simulate_suffix_link_top_down (signed_integral_type grandpa,
		signed_integral_type *parent,
		unsigned_integral_type target_depth,
		size_t starting_position,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw) {
	/*
	 * The position in the sliding window of the beginning
	 * of the next suffix equals the starting position
	 * of the original suffix incremented by one,
	 * because the next suffix's length is smaller by one
	 * while it ends at the same position.
	 *
	 * This position can be directly used
	 * only if the grandpa is the root (and its depth is zero).
	 * Otherwise, we have to increment it
	 * by the depth of the grandpa.
	 */
	size_t position = starting_position + 1;
	/* the node from which the suffix link starts */
	signed_integral_type starting_from = (*parent);
	int retval = 0; /* the return value of the descending function */
	if (position > tfsw->total_window_size) {
		/* we are sure that the value could overflow just by one */
		position = 1;
	}
	if (grandpa <= 0) { /* grandpa is either a leaf or undefined */
		fprintf(stderr,	"stsw_slli_simulate_suffix_link_top_down:\n"
				"Error: grandpa (%d) is not a branching "
				"node, which is unacceptable!\n", grandpa);
		return (1); /* grandpa is not a branching node */
	} else if (grandpa > 1) { /* grandpa is not the root */
		/*
		 * We can use a suffix link transition from the grandpa.
		 * But before we do that, we set the position,
		 * because we then need not to increment it by one,
		 * as the suffix link points to a node which depth
		 * is one less than the depth of its source node.
		 */
		position = starting_position + stsw->tbranch[grandpa].depth;
		if (position > tfsw->total_window_size) {
			position -= tfsw->total_window_size;
		}
		grandpa = stsw->tbranch[grandpa].suffix_link;
	}
	(*parent) = 0; /* we invalidate the parent's node */
	/* if we were able to locate the desired depth */
	if ((retval = stsw_slli_go_down(grandpa, parent, target_depth,
					&position, tfsw, stsw)) == 0) {
		/* we establish a new suffix link */
		stsw->tbranch[starting_from].suffix_link = (*parent);
		return (0); /* suffix link simulation was successful */
	} else if (retval == (-1)) {
		/*
		 * the suffix link's target does not exist yet,
		 * but we found its position
		 */
		return (-1); /* partial success */
	} else { /* something went wrong */
		fprintf(stderr,	"stsw_slli_simulate_suffix_link_top_down:\n"
				"Error: Suffix link simulation "
				"failed permanently!\n");
		return (2); /* a failure */
	}
}

/**
 * A function which simulates the suffix link
 * using the bottom-up approach.
 *
 * @param
 * parent	The node for which we need to find its suffix link.
 * 		When this function successfully returns, it will be set
 * 		either to the suffix link's target itself (if it already
 * 		exists) or to the parent of the new suffix link's target
 * 		(if the suffix link's target itself does not exist yet).
 * @param
 * child	The most recent child of the "parent" node (not the newly
 * 		created leaf node!).
 * 		During this function, it will be set to one of the suffix
 * 		link target's children, even if the suffix link's target
 * 		itself does not exist yet. The chosen child will be
 * 		the only such one, which corresponds to the substring,
 * 		which matches the substring from the root to the original
 * 		child node. The changes made to this variable are, of course,
 * 		not propagated outside of this function.
 * @param
 * target_depth	the depth of the suffix link's target
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If the suffix link simulation has been successful,
 * 		this function returns 0.
 * 		If the suffix link's target does not exist yet, we set up
 * 		the parent variable appropriately and return (-1).
 * 		If an error occurs, a positive error number is returned.
 */
int stsw_slli_simulate_suffix_link_bottom_up (signed_integral_type *parent,
		signed_integral_type child,
		unsigned_integral_type target_depth,
		suffix_tree_sliding_window_slli *stsw) {
	/* the node from which the suffix link starts */
	signed_integral_type starting_from = (*parent);
	int retval = 0; /* the return value of the ascending function */
	if (child == 0) { /* child number is undefined */
		fprintf(stderr,	"stsw_slli_simulate_suffix_link_bottom_up:\n"
				"Error: Invalid number of child (0)!\n");
		return (1); /* child number is invalid */
	} else if (child > 0) { /* child is a branching node */
		/*
		 * We can use a suffix link transition
		 * from the branching node.
		 */
		child = stsw->tbranch[child].suffix_link;
	} else { /* child < 0 (child is a leaf) */
		/*
		 * We can use a suffix link transition
		 * from the leaf node.
		 */
		if ((size_t)(-child) == stsw->tleaf_size) {
			child = (-1);
		} else {
			child = child - 1;
		}
	}
	(*parent) = 0; /* we invalidate the parent's node */
	/* if we were able to locate the desired depth */
	if ((retval = stsw_slli_go_up(parent, &child, target_depth,
					stsw)) == 0) {
		/* we establish a new suffix link */
		stsw->tbranch[starting_from].suffix_link = (*parent);
		return (0);
	/*
	 * We could not find the suffix link's target,
	 * because it does not exist yet. But we found its position!
	 */
	} else if (retval == (-1)) {
		/* suffix link simulation was partially successful */
		return (-1);
	} else { /* something went wrong */
		fprintf(stderr,	"stsw_slli_simulate_suffix_link_bottom_up:\n"
				"Error: Suffix link simulation "
				"failed permanently!\n");
		return (2); /* a failure */
	}
}

/**
 * A function which prolongs a single suffix. This function utilizes
 * the suffix links in compliance with the Ukkonen's algorithm
 * to speed up the construction.
 *
 * @param
 * variation	the desired algorithm variation to use
 * @param
 * starting_position	the position in the sliging window of the first
 * 			character of the suffix to be prolonged
 * @param
 * ending_position	the position in the sliding window just after
 * 			the last character of the suffix after being prolonged
 * @param
 * active_index		The position in the sliding window from which we start
 * 			to prolong the suffix.
 * 			The text starting at this position will be matched
 * 			against the edges leading from the current active_node.
 * 			When the function returns, the (*active_index) is set
 * 			to the position in the sliding window,
 * 			from which we can safely start to prolong
 * 			the following (shorter) suffix.
 * @param
 * active_node	A branching node from which the prolonging starts.
 * 		When the function returns, the (*active_node) is set to the
 * 		node, from which we can safely start the prolonging
 * 		of the next, shorter, suffix.
 * @param
 * sl_source	The source node of the suffix link, which needs
 * 		to be completed. When the function returns, the (*sl_source)
 * 		is set to the new node, which needs to be equipped
 * 		with the suffix link, if there is any.
 * @param
 * sl_targets_depth	The suffix link target's depth.
 * 			When the function returns, the (*sl_targets_depth)
 * 			is set to the new suffix link target's depth, if any.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If a new leaf node has been created during the prolonging
 * 		of the edge, we return 0.
 * 		If the length of the suffix to be prolonged is zero,
 * 		which means that there is nothing to be done,
 * 		this function returns (-1).
 * 		If the suffix to be prolonged ends at a leaf node,
 * 		we need not to do anything and this function returns (-2).
 * 		If we reach the ending node of the suffix and find out
 * 		that it is a branching node, this function returns (-3).
 * 		If the suffix to be prolonged ends at some implicit node
 * 		inside an edge, we need not to do anything and this function
 * 		returns (-4).
 * 		If an error occurs, a positive error number is returned.
 */
int stsw_slli_ukkonen_prolong_suffix (const int variation,
		size_t starting_position,
		size_t ending_position,
		size_t *active_index,
		signed_integral_type *active_node,
		signed_integral_type *sl_source,
		unsigned_integral_type *sl_targets_depth,
		text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw) {
	/* the starting letter of the currently examined edge */
	character_type letter = 0;
	signed_integral_type new_leaf = 0;
	/*
	 * a variable where we will store the immediate parent
	 * of the active_node
	 */
	signed_integral_type grandpa = 0;
	signed_integral_type child = 0;
	signed_integral_type prev_child = 0;
	/* the target node of an edge, which will be split */
	signed_integral_type latest_child = 0;
	/*
	 * The position in the sliding window of the last character
	 * on the current active_node->child edge, which matches a substring
	 * of the suffix starting at the position "starting_position".
	 *
	 * Its sign indicates the type of the following character's mismatch.
	 */
	signed_integral_type last_match_position = 0;
	/*
	 * The position in the sliding window of the current letter
	 * of the suffix to be prolonged. In the beginning, it is set
	 * to the position in the sliding window from which we start
	 * to prolong the suffix. It is the position corresponding to
	 * the first letter of an edge leading from the active_node
	 * to the correct child.
	 */
	size_t position = (*active_index);
	/* The desired length of the suffix after being prolonged. */
	size_t desired_length = 0;
	/*
	 * the variable used to store the temporarily computed positions
	 * at various places around this function
	 */
	size_t tmp_position = 0;
	int tmp = 0; /* the return value of the branching function */
	/* the return value of the suffix link simulation function */
	int sls_retval = 0;
	int slowscan_retval = 0; /* the return value from the "slowscan" */
	/* if there is nothing to be done */
	if (starting_position == ending_position) {
		/*
		 * Note that returning is correct here, because even if
		 * we were prolonging the longest possible suffix
		 * in the suffix tree, it would still be at most as long
		 * as the maximum length of the active part
		 * of the sliding window can be. And this length is required
		 * to be strictly smaller than the total length
		 * of the sliding window.
		 */
		return (-1); /* we are (successfully) finished */
	/* setting the desired length of the suffix after being prolonged */
	} else if (starting_position < ending_position) {
		desired_length = ending_position - starting_position;
	} else { /* starting_position > ending_position */
		desired_length += tfsw->total_window_size + ending_position -
			starting_position;
	}
	/*
	 * while there is an edge with the matching first character
	 * leading from the current active_node
	 */
	while ((tmp = stsw_slli_branch_once((*active_node), &child,
					&prev_child, position,
					tfsw, stsw)) == 0) {
		/*
		 * while there is a candidate branching edge,
		 * we check it for a complete match
		 */
		if ((slowscan_retval = stsw_slli_edge_slowscan((*active_node),
				child, position, &last_match_position, tfsw,
				stsw)) == 0) {
			/*
			 * The whole edge matches.
			 *
			 * We store the current number of the active_node.
			 */
			grandpa = (*active_node);
			/*
			 * We are going to descend down along
			 * the active_node->child edge.
			 */
			stsw_slli_edge_descend(active_node, &child,
					&prev_child, &position,
					tfsw, stsw);
			if ((*active_node) < 0) {
				/*
				 * We have descended down to a leaf,
				 * which means that we are done.
				 *
				 * The edge leading to this leaf will be
				 * implicitly prolonged when the length
				 * of the active part of the sliding window
				 * increases.
				 *
				 * We restore the old value of (*active_node).
				 */
				(*active_node) = grandpa;
				/* if the active_node is not the root */
				if ((*active_node) != 1) {
					/*
					 * We use a suffix link to adjust
					 * the active_node variable
					 * for the next suffix.
					 */
					(*active_node) = stsw->tbranch[
						(*active_node)].suffix_link;
				}
				/*
				 * Similarly, we adjust the starting position
				 * for the next suffix according to
				 * the new value of the (*active_node).
				 *
				 * We have to increase it by one, because
				 * the next suffix will be shorter
				 * by one character.
				 */
				(*active_index) = starting_position + 1 +
				(size_t)(stsw->tbranch[(*active_node)].depth);
				if ((*active_index) >
						tfsw->total_window_size) {
					(*active_index) -=
						tfsw->total_window_size;
				}
				return (-2); /* success */
			} else { /* here, (*active_node) >= 0 */
				/*
				 * calculating the position
				 * in the sliding window
				 * of the last character
				 * of the grandpa->active_node edge
				 */
				tmp_position = starting_position +
					stsw->tbranch[(*active_node)].depth;
				if (tmp_position > tfsw->total_window_size) {
					tmp_position -=
						tfsw->total_window_size;
				}
				if (tmp_position == ending_position) {
					/*
					 * We have descended to the exact
					 * maximum depth.
					 *
					 * We adjust the active_index
					 * for the next prolonged suffix.
					 *
					 * We take advantage of the fact that
					 * the next suffix will be one
					 * character shorter and therefore
					 * will start at a position increased
					 * by one, compared to the current
					 * suffix.
					 * Moreover, the current value
					 * of the active_node's depth before
					 * the suffix link transition
					 * is the value of the new
					 * active_node's depth after
					 * the suffix link transition,
					 * increased by one.
					 * So, we can postpone the suffix
					 * link transition and use the current
					 * value of the active_node's
					 * depth instead of incrementing
					 * the new value of the active_node's
					 * depth by one.
					 */
					(*active_index) = starting_position +
						(size_t)(stsw->tbranch[
							(*active_node)].depth);
					if ((*active_index) > tfsw->
							total_window_size) {
						(*active_index) -=
						tfsw->total_window_size;
					}
					/*
					 * the current suffix is already
					 * explicitly present in the suffix
					 * tree and all the following suffixes
					 * need not to be prolonged
					 */
					return (-3);
				}
			}
		} else if (slowscan_retval == 2) {
			/*
			 * the edge is longer than necessary,
			 * but its initial letters match
			 *
			 * We adjust the starting position
			 * for the next prolonged suffix.
			 *
			 * We take advantage of the fact that
			 * the next suffix will be one character
			 * shorter and therefore will start
			 * at a position increased by one,
			 * compared to the current suffix.
			 * Moreover, the current value of active_node's
			 * depth before suffix link transition
			 * is the value of the new active_node's depth,
			 * after the suffix link transition,
			 * increased by one. So, we can postpone
			 * the suffix link transition and use
			 * the current value of the active_node's depth
			 * instead of incrementing the new value
			 * of the active_node's depth by one.
			 */
			(*active_index) = starting_position +
				(size_t)(stsw->tbranch[(*active_node)].depth);
			if ((*active_index) > tfsw->total_window_size) {
				(*active_index) -= tfsw->total_window_size;
			}
			/*
			 * the current suffix is already present in
			 * the suffix tree and all the following suffixes
			 * need not to be prolonged
			 */
			return (-4);
		} else { /* we need to split the edge and insert a new node */
			/* we store the current number of the active_node */
			grandpa = (*active_node);
			/* we store the current number of child */
			latest_child = child;
			letter = tfsw->text_window[position];
			if (stsw_slli_split_edge(active_node, &child,
						&prev_child, &position,
						last_match_position,
						(unsigned_integral_type)
						(starting_position),
						tfsw, stsw) != 0) {
				fprintf(stderr, "Error: Could not split the "
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
						"P(%d)--\"%lc...\"->C(%d)"
#else
						"P(%d)--\"%c...\"->C(%d)"
#endif
						" edge!\n", (*active_node),
						letter, child);
				return (1);
			}
			/* the position has just changed */
			letter = tfsw->text_window[position];
			new_leaf = (signed_integral_type)(stsw->tleaf_first +
				stsw_get_leafs_depth_order(starting_position,
						tfsw));
			if (new_leaf > (signed_integral_type)
					(stsw->tleaf_size)) {
				new_leaf -= (signed_integral_type)
					(stsw->tleaf_size);
			}
			if (stsw_slli_create_leaf((*active_node), child,
						prev_child, -new_leaf,
						stsw) > 0) {
				fprintf(stderr, "Error: Could not create "
						"the new leaf edge "
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
						"P(%d)--\"%lc...\"->C(%d)"
#else
						"P(%d)--\"%c...\"->C(%d)"
#endif
						". Exiting!\n",
						(*active_node), letter,
						-new_leaf);
				return (2);
			}
			/* if there is a suffix link to be filled in */
			if ((*sl_source) != 0) {
				tmp_position = starting_position +
					(*sl_targets_depth);
				if (tmp_position > tfsw->total_window_size) {
					tmp_position -=
						tfsw->total_window_size;
				}
				if (position == tmp_position) {
					/*
					 * we have reached the desired depth
					 * of the suffix link's target,
					 *
					 * so we complete this suffix link
					 */
					stsw->tbranch[(*sl_source)].
						suffix_link = (*active_node);
				} else {
					/*
					 * the current depth is not the depth
					 * of the suffix link's target
					 */
					fprintf(stderr,	"Error! The last "
							"opportunity to "
							"set the target\n"
							"of a suffix link "
							"missed! (suffix "
							"link source = %d)\n",
							(*sl_source));
					return (3);
				}
			}
			/*
			 * in case suffix link simulation fails, we need
			 * to remember the new source of a suffix link
			 */
			(*sl_source) = (*active_node);
			/* as well as the depth of a suffix link's target */
			(*sl_targets_depth) =
				stsw->tbranch[(*active_node)].depth - 1;
			/*
			 * if the desired algorithm variation
			 * is the minimized branching
			 */
			if (variation == 1) {
				/*
				 * if the suffix link simulation
				 * was successful
				 */
				if ((sls_retval =
				stsw_slli_simulate_suffix_link_bottom_up(
							active_node,
							latest_child,
							(*sl_targets_depth),
							stsw)) == 0) {
					/*
					 * we can invalidate
					 * the remembered values
					 */
					(*sl_source) = 0;
					(*sl_targets_depth) = 0;
				/* if it has encountered an error */
				} else if (sls_retval > 0) {
					fprintf(stderr,
					"stsw_slli_ukkonen_prolong_suffix:\n"
					"Error: The suffix link simulation "
					"has encountered an error. "
					"Exiting!\n");
					return (4);
				}
			} else {
				/*
				 * if the suffix link simulation
				 * was successful
				 */
				if ((sls_retval =
				stsw_slli_simulate_suffix_link_top_down(
							grandpa, active_node,
							(*sl_targets_depth),
					starting_position, tfsw, stsw)) == 0) {
					/*
					 * we can invalidate
					 * the remembered values
					 */
					(*sl_source) = 0;
					(*sl_targets_depth) = 0;
				/* if it has encountered an error */
				} else if (sls_retval > 0) {
					fprintf(stderr,
					"stsw_slli_ukkonen_prolong_suffix:\n"
					"Error: The suffix link simulation "
					"has encountered an error. "
					"Exiting!\n");
					return (5);
				}
			}
			/*
			 * We adjust the starting position
			 * of the next prolonged suffix.
			 */
			(*active_index) = starting_position + 1 +
				(size_t)(stsw->tbranch[(*active_node)].depth);
			if ((*active_index) > tfsw->total_window_size) {
				(*active_index) -= tfsw->total_window_size;
			}
			return (0); /* we return success */
		}
	}
	if (tmp == 1) {
		fprintf(stderr,	"Error: The (*active_node) at the moment "
				"of branching (%d)\nis not valid. Exiting!\n",
				(*active_node));
		return (6);
	} else if (tmp == 2) { /* the parent has no children at all */
		/* we need to create the first child of the parent */
		fprintf(stderr,	"Information: Creating the first child "
				"of the active_node.\n"
				"This should not happen more than once!\n");
		new_leaf = (signed_integral_type)(stsw->tleaf_first +
			stsw_get_leafs_depth_order(starting_position, tfsw));
		if (new_leaf > (signed_integral_type)(stsw->tleaf_size)) {
			new_leaf -= (signed_integral_type)(stsw->tleaf_size);
		}
		stsw->tbranch[(*active_node)].first_child = -new_leaf;
		stsw->tleaf[new_leaf].parent = (*active_node);
		stsw->tleaf[new_leaf].next_brother = 0;
		/*
		 * since we have just created the first child of the root,
		 * no edge label maintenance is necessary
		 */
	} else { /* (tmp == 3), which means that there was no matching edge */
		letter = tfsw->text_window[position];
		new_leaf = (signed_integral_type)(stsw->tleaf_first +
			stsw_get_leafs_depth_order(starting_position, tfsw));
		if (new_leaf > (signed_integral_type)(stsw->tleaf_size)) {
			new_leaf -= (signed_integral_type)(stsw->tleaf_size);
		}
		/* we need to create a new child of the active_node */
		if (stsw_slli_create_leaf((*active_node), child, prev_child,
					-new_leaf, stsw)) {
			fprintf(stderr, "Error: Could not create "
					"the new leaf edge "
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
					"P(%d)--\"%lc...\"->C(%d)"
#else
					"P(%d)--\"%c...\"->C(%d)"
#endif
					". Exiting!\n",
					(*active_node), letter,
					-new_leaf);
			return (7);
		}
		// FIXME: Rename head_position to something new and better!
		/* updating the head position of the active_node */
		if (tfsw->elm_method == 1) { /* batch update by M. Senft */
			/* we don't do anything there */
		} else {
			/*
			 * Both the remaining edge label maintenance methods
			 * (Fiala's and Greene's and Larsson's) take the same
			 * actions here, so we need not to distinguish
			 * between them.
			 */
			if ((*active_node) != 1) {
				if (stsw_slli_send_credit((*active_node),
						(unsigned_integral_type)
						(starting_position),
						tfsw, stsw) > 0) {
					fprintf(stderr, "Error: Could not "
							"send a credit "
							"to the active node,\n"
							"which is the parent "
							"(%d) of the newly "
							"created leaf node "
							"(%d). Exiting!\n",
							(*active_node),
							-new_leaf);
					return (8);
				}
			}
		}
	}
	/* If the active_node is a branching node and it is not the root. */
	if ((*active_node) > 1) {
		/* we can use a suffix link transition */
		(*active_node) = stsw->tbranch[(*active_node)].suffix_link;
	}
	/* We adjust the starting position of the next prolonged suffix. */
	(*active_index) = starting_position + 1 +
		(size_t)(stsw->tbranch[(*active_node)].depth);
	if ((*active_index) > tfsw->total_window_size) {
		(*active_index) -= tfsw->total_window_size;
	}
	return (0);
}

/**
 * A function to prolong all the suffixes in the intermediate suffix tree
 * by one character.
 * This function utilizes the suffix links in compliance with
 * the Ukkonen's algorithm to speed up the construction.
 *
 * @param
 * variation	the desired algorithm variation to use
 * @param
 * starting_position	The position in the sliding window of the first
 * 			character of the first suffix to be prolonged.
 * 			Note that despite we are prolonging all the suffixes
 * 			currently present in the suffix tree, we don't have to
 * 			prolong all of them explicitly. The starting_position
 * 			should point to the beginning of the longest suffix,
 * 			which we might have to prolong explicitly.
 * @param
 * ending_position	the position in the sliding window of the character
 * 			just after the last character of the suffixes
 * 			after being prolonged
 * @param
 * active_index		The position in the sliding window from which we start
 * 			to prolong the first suffix.
 * 			The text starting at this position will be matched
 * 			against the edges leading from the current active_node.
 * @param
 * active_node	The node from which the prolonging actively starts.
 * 		Actually this is either the active point or the closest parent
 * 		of the active point. After the function returns,
 * 		it is set to the next active point.
 * 		The active point is the position in the suffix tree,
 * 		which corresponds to the last character
 * 		of the longest substring, which occurs only as a suffix
 * 		in the current suffix tree and does not end
 * 		at the leaf node. FIXME: Is this correct?
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If the prolonging of the suffixes has been successful,
 * 		this function returns 0.
 * 		If an error occurs, a nonzero error number is returned.
 */
int stsw_slli_ukkonen_prolong_suffixes (const int variation,
		size_t *starting_position,
		size_t ending_position,
		size_t *active_index,
		signed_integral_type *active_node,
		text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw) {
	signed_integral_type sl_source = 0;
	unsigned_integral_type sl_targets_depth = 0;
	int tmp = 0;
	/* increasing the usable size of the table tleaf */
	if (stsw->tleaf_last == stsw->tleaf_size) {
		stsw->tleaf_last = 1;
	} else {
		++stsw->tleaf_last;
	}
	/* if the depth of the suffix to be prolonged is zero */
	if ((tmp = stsw_slli_ukkonen_prolong_suffix(variation,
					(*starting_position),
					ending_position, active_index,
					active_node, &sl_source,
					&sl_targets_depth, tfsw,
					stsw)) == (-1)) {
		/*
		 * we adjust the starting_position for the next call
		 * of the function stsw_slli_ukkonen_prolong_suffix
		 */
		++(*starting_position);
		if ((*starting_position) > tfsw->total_window_size) {
			/*
			 * we are sure that this value
			 * has just overflown precisely by one
			 */
			(*starting_position) = 1;
		}
		/*
		 * The active_node does not need to be updated here,
		 * because it is required to be the root.
		 * The active_index also does not need to be updated here,
		 * because it currently points to the first character
		 * after the currently active part of the sliding window.
		 */
	}
	/*
	 * while a new leaf node has been created during
	 * the call to the stsw_slli_ukkonen_prolong_suffix
	 */
	while (tmp == 0) {
		++(*starting_position);
		if ((*starting_position) > tfsw->total_window_size) {
			/*
			 * we are sure that this value
			 * has just overflown precisely by one
			 */
			(*starting_position) = 1;
		}
		tmp = stsw_slli_ukkonen_prolong_suffix(variation,
				(*starting_position), ending_position,
				active_index, active_node, &sl_source,
				&sl_targets_depth, tfsw, stsw);
	}
	if (tmp > 0) { /* something went wrong */
		fprintf(stderr, "Error: Could not prolong the suffix\n"
				"starting at the position %zu to end "
				"at the position %zu. Exiting!\n",
				(*starting_position), ending_position);
		return (1);
	}
	return (0);
}

/* handling functions */

/**
 * A function which creates and maintains the suffix tree over
 * a sliding window, which moves through the whole input text,
 * using the Ukkonen's algorithm with the variation of
 * bottom-up suffix link simulation (minimized branching).
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written (if requested)
 * @param
 * benchmark	the requested benchmark to perform
 * @param
 * variation	the desired algorithm variation to use
 * @param
 * traversal_type	the type of the suffix tree traversal,
 * 			which will be performed (if requested)
 * @param
 * verbosity_level	The requested level of verbosity of the information
 * 			collected and displayed to the user
 * 			during the suffix tree construction and maintenance.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the suffix tree which will be created and maintained
 *
 * @return	If this function successfully creates and maintains
 * 		the suffix tree for all the positions of the sliding window,
 * 		it returns 0.
 * 		If an error occurs, a nonzero error number is returned.
 */
int stsw_slli_create_ukkonen (FILE *stream,
		const int benchmark,
		const int variation,
		const int traversal_type,
		const int verbosity_level,
		text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_slli *stsw) {
	/*
	 * The active node is the node from which the prolonging
	 * of the suffixes actively starts.
	 *
	 * The very first active node is the root.
	 */
	signed_integral_type active_node = 1;
	/*
	 * The active index is the index in the sliding window
	 * of the character just after the last character,
	 * which matches the path from the root
	 * to the current active node.
	 *
	 * The very first active node is the root and therefore
	 * its corresponding active index is the first index
	 * of the sliding window.
	 */
	size_t active_index = 1;
	/*
	 * The starting position of the current suffix to be prolonged.
	 *
	 * The first suffix we are going to prolong will surely start
	 * at the first index of the sliding window.
	 */
	size_t starting_position = 1;
	/*
	 * The position just after the last character of the current suffixes
	 * after being prolonged to their maximum length allowed
	 * by the sliding window.
	 */
	size_t ending_position = 0;
	/*
	 * The position in the sliding window of the first character
	 * of the currently active part of the sliding window.
	 * This variable is only used in the final part
	 * of the suffix tree construction algorithm,
	 * where the longest suffixes are iteratively deleted
	 * from the suffix tree.
	 */
	size_t back_position = 0;
	/*
	 * The index of a block, which is currently processed.
	 * We start by processing the first (0.th block).
	 */
	size_t block_to_process = 0;
	/* the total number of blocks processed so far */
	size_t blocks_processed = 0;
	/*
	 * The position in the sliding window just after the last character
	 * of the current suffixes after being prolonged.
	 *
	 * We are starting from 2, because it is the first position just after
	 * the first valid last character of the suffix after being prolonged.
	 */
	size_t i = 2;
	/*
	 * The text index of the character, which is at the beginning
	 * of the currently active part of the sliding window.
	 *
	 * The text index is the index of a character in the whole text
	 * that has already been processed in the sliding window.
	 * It starts at 1 and continues up to the number
	 * of processed characters.
	 */
	size_t ap_window_begin_text_idx = 0;
	/*
	 * The number of characters in the final block read.
	 * This variable is used in both the pthread and non-pthread parts
	 * of this function.
	 */
	size_t final_block_characters = 0;
	/* the number of the last block which has been read */
	size_t final_block_number = 0;
	/*
	 * The return value from the pthread_create function
	 * and from the block reading function.
	 */
	int retval = 0;
	/*
	 * the statistics-related variables
	 *
	 * the number of observations performed to collect the statistics
	 */
	size_t observations = 0;
	/* the minimum number of branching nodes observed */
	size_t min_branching_nodes = 0;
	/*
	 * the text index of the beginning of the active part
	 * of the sliding window (or ap_bti),
	 * at the moment when there has been the minimum number
	 * of branching nodes observed
	 */
	size_t min_brn_ap_bti = 0;
	/* the average number of branching nodes observed */
	double avg_branching_nodes = 0;
	/* the maximum number of branching nodes observed */
	size_t max_branching_nodes = 0;
	/*
	 * the text index of the beginning of the active part
	 * of the sliding window (or ap_bti),
	 * at the moment when there has been the maximum number
	 * of branching nodes observed
	 */
	size_t max_brn_ap_bti = 0;
/* POSIX threads-related variables */
#ifdef STSW_USE_PTHREAD
	/* The index of a block, which will be made available for reading. */
	size_t block_to_release = 0;
	/* The return value from this function. */
	int function_retval = 0;
	/* The return value from the reading thread function. */
	void *thread_retval = NULL;
	/*
	 * The data shared between the main thread and the reading thread.
	 *
	 * Despite that we do not initialize this struct right here,
	 * we do it immediately afterwards,
	 * so it will never be used uninitialized.
	 */
	shared_data sd;
	/*
	 * The mutex attributes.
	 *
	 * We do not initialize it right here,
	 * but we do it immediately afterwards,
	 * so it will never be used uninitialized.
	 */
	pthread_mutexattr_t mutex_attributes;
	/* initialization of the mutex attributes */
	pthread_mutexattr_init(&mutex_attributes);
	pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_NORMAL);
	/* initialization of the mutex itself */
	pthread_mutex_init(&sd.mx, &mutex_attributes);
	/* initialization of the condition variable */
	pthread_cond_init(&sd.cv, NULL);
	/* initialization of the shared_data struct */
	sd.tfsw = tfsw;
	sd.reading_finished = 0;
	sd.final_block_characters = 0;
	sd.final_block_number = 0;
#else /* non POSIX threads-related variables */
	size_t blocks_read = 0;
	size_t characters_read = 0;
	size_t bytes_read = 0;
#endif
	if (variation == 0) {
		printf("Creating and maintaining the suffix tree "
				"over a sliding window\n"
				"using the SL implementation technique "
				"and the Ukkonen's algorithm\n\n");
	} else if (variation == 1) {
		printf("Creating and maintaining the suffix tree "
				"over a sliding window\n"
				"using the SL implementation technique "
				"and the Ukkonen's algorithm\n"
				"with the variation of "
				"the minimized branching\n\n");
	} else {
		fprintf(stderr, "stsw_slli_create_ukkonen:\n"
				"Unknown value for the selected "
				"algorithm variation (%d). "
				"Exiting!\n", variation);
		return (1);
	}
	if (stsw_slli_allocate(verbosity_level, tfsw, stsw) > 0) {
		fprintf(stderr, "stsw_slli_create_ukkonen:\n"
				"Allocation error. Exiting.\n");
		return (2);
	}
	/*
	 * We suppose that the suffix tree will be used extensively,
	 * utilizing the maximum possible number of branching nodes
	 * at some time. That's why we just allocate
	 * all the required memory now - to avoid checking for overflows
	 * during the suffix tree construction and maintenance.
	 */
	if (stsw_slli_reallocate(verbosity_level, tfsw->sw_block_size *
				tfsw->ap_scale_factor - 1, tfsw, stsw) > 0) {
		fprintf(stderr, "stsw_slli_create_ukkonen:\n"
				"Reallocation error. Exiting.\n");
		return (3);
	}
#ifdef STSW_USE_PTHREAD /* the pthread part */
	pthread_t reader; /* the reading thread */
	// FIXME: What if not even all the blocks in the active part
	// of the sliding window will be read? What about stats then?
	// That should be handled by the number of observations.
	// At least one observation should always be performed -
	// before the final part.
	if ((retval = pthread_create(&reader, NULL,
				&reading_thread_function, &sd)) != 0) {
		fprintf(stderr, "stsw_slli_create_ukkonen:\n");
		if (errno != 0) {
			perror("pthread_create inside");
		}
		errno = retval; /* retval != 0 */
		perror("pthread_create");
		/* resetting the errno */
		errno = 0;
		return (4);
	}
	/* FIXME: Return values from the pthread functions? */
	if (verbosity_level > 0) {
		printf("The first part:\n"
				"populating the active part "
				"of the sliding window ...\n");
	}
	/*
	 * The first part.
	 *
	 * We have to process the first a few blocks separately, until
	 * the active part of the sliding window reaches its maximum size,
	 * or until there are no more blocks to be read.
	 * Here, we will not be adjusting the index of the most recently
	 * processed block, nor the block flags.
	 * During this part, we will just prolong the suffixes
	 * previously present in the suffix tree and we will
	 * not be deleting the longest suffixes.
	 * The reason is that at first, we would like to fill in
	 * all the space available for the suffix tree so that
	 * it will contain all the suffixes from the active part
	 * of the sliding window of the maximum possible size.
	 */
	/*
	 * block_to_process == 0 here, but we need it to be equal to zero
	 * just after the first increment. That's why we decrement it now
	 * and we rely on the wrap-around underflow and overflow arithmetic
	 * of the data type size_t. This should be pretty portable.
	 */
	--block_to_process;
	while (++block_to_process < tfsw->ap_scale_factor) {
		/*
		 * The check for an overflow of the block index
		 * is not necessary here, because the total number
		 * of blocks is required to be strictly higher
		 * than the maximum number of blocks forming
		 * the active part of the sliding window.
		 */
		/* the start of the critical section */
		pthread_mutex_lock(&sd.mx);
		/* while the current block is not ready to be processed */
		while (tfsw->sw_block_flags[block_to_process] !=
				BLOCK_STATUS_READ_AND_UNPROCESSED) {
			/* if the reading thread has not finished yet */
			if (sd.reading_finished == 0) {
				/*
				 * we wait for it to signal
				 * a block status change
				 */
				pthread_cond_wait(&sd.cv, &sd.mx);
			} else {
				/*
				 * Otherwise we know that no more blocks
				 * will be read, so we can safely unlock
				 * the mutex and jump
				 * to the thread joining part.
				 */
				pthread_mutex_unlock(&sd.mx);
				goto thread_joining;
			}
		}
		/*
		 * we are still inside the critical section
		 *
		 * The current block is now ready to be processed!
		 */
		/* if no more blocks will be read */
		if (sd.reading_finished == 1) {
			/*
			 * We know that the reading thread has either
			 * already finished or it will finish soon.
			 * So, we can afford to keep the mutex locked
			 * for a little longer without much worrying.
			 *
			 * if the current block is the last block
			 */
			if (block_to_process == sd.final_block_number) {
				/* we just process it */
				ending_position = block_to_process *
					tfsw->sw_block_size +
					/*
					 * the text in the sliding window
					 * starts at the index of 1
					 */
					sd.final_block_characters + 1;
			/*
			 * otherwise, there are more blocks
			 * ready to be processed
			 */
			} else {
				/*
				 * but right now, we process
				 * the current block only
				 */
				ending_position = (block_to_process + 1) *
					/*
					 * the text in the sliding window
					 * starts at the index of 1
					 */
					tfsw->sw_block_size + 1;
			}
		} else { /* the reading has not finished yet */
			/* we process the current block only */
			ending_position = (block_to_process + 1) *
				/*
				 * the text in the sliding window
				 * starts at the index of 1
				 */
				tfsw->sw_block_size + 1;
		}
		pthread_mutex_unlock(&sd.mx);
		/*
		 * the end of the critical section
		 *
		 * Now we have the current block ready to be processed
		 * and we know that its status can not change,
		 * because this is the only processing thread.
		 * So, we can end the critical section here
		 * and continue to process the block normally.
		 */
		for (; i <= ending_position; ++i) {
			/*
			 * In this initial part,
			 * we are not deleting the longest suffix.
			 */
			++tfsw->ap_window_end;
			/*
			 * we have to increase the size
			 * of the sliding window as well
			 */
			++tfsw->ap_window_size;
			if (stsw_slli_ukkonen_prolong_suffixes(variation,
						&starting_position, i,
						&active_index, &active_node,
						tfsw, stsw) > 0) {
				fprintf(stderr,	"Could not prolong "
						"the suffixes to end "
						"at the position %zu. "
						"Exiting.\n", i);
				/* the start of the critical section */
				pthread_mutex_lock(&sd.mx);
				/*
				 * There was an error, so we need to terminate
				 * the reading thread, if it is still running.
				 * But at first, we have to wake it up,
				 * because it might be sleeping.
				 * We also have to raise the reading_finished
				 * flag to force the reading thread
				 * to terminate immediately.
				 */
				sd.reading_finished = 1;
				pthread_cond_signal(&sd.cv);
				pthread_mutex_unlock(&sd.mx);
				/*
				 * the end of the critical section
				 *
				 * we need to join with the reading thread
				 * at first and just then return failure
				 */
				function_retval = 5;
				goto thread_joining;
			}
		}
	}
	/* if the traversal type of benchmark has been requested */
	if (benchmark == 2) {
		fprintf(stream, "Initial suffix tree traversal:\n");
		fprintf(stream, "The suffix tree is built over the initial "
			"%zu characters\nof the input text.\n\n",
			stsw->tleaf_last - stsw->tleaf_first + 1);
		/*
		 * we traverse the suffix tree here,
		 * because it has just grown to its maximum size
		 */
		if (stsw_slli_traverse(verbosity_level, stream, traversal_type,
					tfsw, stsw) != 0) {
			fprintf(stderr, "Error: The initial suffix tree "
					"traversal has failed. Exiting!\n");
			/* the start of the critical section */
			pthread_mutex_lock(&sd.mx);
			/*
			 * There was an error, so we need to terminate
			 * the reading thread, if it is still running.
			 * But at first, we have to wake it up,
			 * because it might be sleeping.
			 * We also have to raise the reading_finished
			 * flag to force the reading thread
			 * to terminate immediately.
			 */
			sd.reading_finished = 1;
			pthread_cond_signal(&sd.cv);
			pthread_mutex_unlock(&sd.mx);
			/*
			 * the end of the critical sectiom
			 *
			 * we need to join with the reading thread
			 * at first and just then return failure
			 */
			function_retval = 6;
			goto thread_joining;
		}
	}
	if (verbosity_level > 1) {
		/*
		 * we initialize the text index of the beginning
		 * of the currently active part of the sliding window
		 */
		ap_window_begin_text_idx = 1;
		/* also, we initialize the number of observations */
		observations = 1;
		/* as well as all the statistical variables */
		min_branching_nodes = stsw->branching_nodes;
		min_brn_ap_bti = ap_window_begin_text_idx;
		avg_branching_nodes = (double)(stsw->branching_nodes);
		max_branching_nodes = stsw->branching_nodes;
		max_brn_ap_bti = ap_window_begin_text_idx;
	}
	/*
	 * Here block_to_process == tfsw->ap_scale_factor,
	 * so we have to decrement it, because in the beginning
	 * of the next part it will be incremented again.
	 * We can also be sure that block_to_process >= 1,
	 * because tfsw->ap_scale_factor >= 1.
	 */
	--block_to_process;
	if (verbosity_level > 0) {
		printf("Main part:\n"
				"maintaining the suffix tree "
				"over the sliding window ...\n");
	}
	/*
	 * The main part.
	 *
	 * This is where the processing of the other blocks takes place.
	 * Here, we will be finally adjusting the index
	 * of the most recently processed block and the block flags.
	 * During this part, we will be deleting the longest suffixes
	 * currently present in the suffix tree as well as prolonging
	 * the suffixes currently present in the suffix tree.
	 * The reason is that we would like to keep the smallest possible
	 * size of the suffix tree, while still containing all the suffixes
	 * from the active part of the sliding window in the suffix tree.
	 */
	while (1) {
		++block_to_process;
		if (block_to_process == tfsw->sw_blocks) {
			block_to_process = 0;
		}
		/* the start of the critical section */
		pthread_mutex_lock(&sd.mx);
		/* while the current block is not ready to be processed */
		while (tfsw->sw_block_flags[block_to_process] !=
				BLOCK_STATUS_READ_AND_UNPROCESSED) {
			/* if the reading thread has not finished yet */
			if (sd.reading_finished == 0) {
				/*
				 * we wait for it to signal
				 * a block status change
				 */
				pthread_cond_wait(&sd.cv, &sd.mx);
			} else {
				/*
				 * Otherwise we know that no more blocks
				 * will be read, so we can safely unlock
				 * the mutex and jump
				 * to the thread joining part.
				 */
				pthread_mutex_unlock(&sd.mx);
				goto thread_joining;
			}
		}
		/*
		 * we are still inside the critical section
		 *
		 * The current block is now ready to be processed!
		 */
		/* if no more blocks will be read */
		if (sd.reading_finished == 1) {
			/*
			 * We know that the reading thread has either
			 * already finished or it will finish soon.
			 * So, we can afford to keep the mutex locked
			 * for a little longer without much worrying.
			 *
			 * if the current block is the last block
			 */
			if (block_to_process == sd.final_block_number) {
				/* we just process it */
				ending_position = block_to_process *
					tfsw->sw_block_size +
					/*
					 * the text in the sliding window
					 * starts at the index of 1
					 */
					sd.final_block_characters + 1;
			/*
			 * otherwise, there are more blocks
			 * ready to be processed
			 */
			} else {
				/*
				 * but right now, we process
				 * the current block only
				 */
				ending_position = (block_to_process + 1) *
					/*
					 * the text in the sliding window
					 * starts at the index of 1
					 */
					tfsw->sw_block_size + 1;
			}
		} else { /* the reading has not finished yet */
			/* we process the current block only */
			ending_position = (block_to_process + 1) *
				/*
				 * the text in the sliding window
				 * starts at the index of 1
				 */
				tfsw->sw_block_size + 1;
		}
		pthread_mutex_unlock(&sd.mx);
		/*
		 * the end of the critical section
		 *
		 * Now we have the current block ready to be processed
		 * and we know that its status can not change,
		 * because this is the only processing thread.
		 * So, we can end the critical section here
		 * and continue to process the block normally.
		 */
		for (; i < ending_position; ++i) {
			/* at first, we have to delete the longest suffix */
			if (stsw_slli_delete_longest_suffix(&starting_position,
						&active_index, &active_node,
						tfsw, stsw) > 0) {
				fprintf(stderr,	"Could not delete "
						"the longest suffix\n"
						"starting at the position %zu "
						"and ending at the position "
						"%zu. Exiting.\n",
						tfsw->ap_window_begin,
						tfsw->ap_window_end);
				/* the start of the critical section */
				pthread_mutex_lock(&sd.mx);
				/*
				 * There was an error, so we need to terminate
				 * the reading thread, if it is still running.
				 * But at first, we have to wake it up,
				 * because it might be sleeping.
				 * We also have to raise the reading_finished
				 * flag to force the reading thread
				 * to terminate immediately.
				 */
				sd.reading_finished = 1;
				pthread_cond_signal(&sd.cv);
				pthread_mutex_unlock(&sd.mx);
				/*
				 * the end of the critical section
				 *
				 * we need to join with the reading thread
				 * at first and just then return failure
				 */
				function_retval = 7;
				goto thread_joining;
			}
			++tfsw->ap_window_begin;
			/* and then we can prolong the current suffixes */
			++tfsw->ap_window_end;
			/*
			 * The size of the sliding window is decreased at first
			 * and immediately afterwards it is increased again.
			 * So, it does not change here and we don't
			 * have to worry about adjusting it at all.
			 */
			if (stsw_slli_ukkonen_prolong_suffixes(variation,
						&starting_position, i,
						&active_index, &active_node,
						tfsw, stsw) > 0) {
				fprintf(stderr,	"Could not prolong "
						"the suffixes to end "
						"at the position %zu. "
						"Exiting.\n", i);
				/* the start of the critical section */
				pthread_mutex_lock(&sd.mx);
				/*
				 * There was an error, so we need to terminate
				 * the reading thread, if it is still running.
				 * But at first, we have to wake it up,
				 * because it might be sleeping.
				 * We also have to raise the reading_finished
				 * flag to force the reading thread
				 * to terminate immediately.
				 */
				sd.reading_finished = 1;
				pthread_cond_signal(&sd.cv);
				pthread_mutex_unlock(&sd.mx);
				/*
				 * the end of the critical section
				 *
				 * we need to join with the reading thread
				 * at first and just then return failure
				 */
				function_retval = 8;
				goto thread_joining;
			}
			/*
			 * the size of the sliding window
			 * is decremented and incremented here,
			 * so it does not change
			 */
			if (verbosity_level > 1) {
				/*
				 * we increment the text index
				 * of the beginning of the currently
				 * active part of the sliding window
				 */
				++ap_window_begin_text_idx;
				/*
				 * we also increment
				 * the number of observations
				 */
				++observations;
				/*
				 * and finally we try to update
				 * all the statistical variables
				 */
				if (stsw->branching_nodes <
						min_branching_nodes) {
					min_branching_nodes =
						stsw->branching_nodes;
					min_brn_ap_bti =
						ap_window_begin_text_idx;
				}
				avg_branching_nodes +=
					(double)(stsw->branching_nodes);
				if (stsw->branching_nodes >
						max_branching_nodes) {
					max_branching_nodes =
						stsw->branching_nodes;
					max_brn_ap_bti =
						ap_window_begin_text_idx;
				}
			}
		}
		/* i == ending_position here */
		if (i > tfsw->total_window_size) {
			i = 1; /* this variable can only overflow by one */
		}
		/* at first, we have to delete the longest suffix */
		if (stsw_slli_delete_longest_suffix(&starting_position,
					&active_index, &active_node,
					tfsw, stsw) > 0) {
			fprintf(stderr,	"Could not delete "
					"the longest suffix\n"
					"starting at the position %zu "
					"and ending at the position "
					"%zu. Exiting.\n",
					tfsw->ap_window_begin,
					tfsw->ap_window_end);
			/* the start of the critical section */
			pthread_mutex_lock(&sd.mx);
			/*
			 * There was an error, so we need to terminate
			 * the reading thread, if it is still running.
			 * But at first, we have to wake it up,
			 * because it might be sleeping.
			 * We also have to raise the reading_finished
			 * flag to force the reading thread
			 * to terminate immediately.
			 */
			sd.reading_finished = 1;
			pthread_cond_signal(&sd.cv);
			pthread_mutex_unlock(&sd.mx);
			/*
			 * the end of the critical section
			 *
			 * we need to join with the reading thread
			 * at first and just then return failure
			 */
			function_retval = 9;
			goto thread_joining;
		}
		++tfsw->ap_window_begin;
		if (tfsw->ap_window_begin > tfsw->total_window_size) {
			/* this variable can only overflow by one */
			tfsw->ap_window_begin = 1;
		}
		/* and then we can prolong the current suffixes */
		++tfsw->ap_window_end;
		if (tfsw->ap_window_end > tfsw->total_window_size) {
			/* this variable can only overflow by one */
			tfsw->ap_window_end = 1;
		}
		/*
		 * The size of the sliding window is decreased at first
		 * and immediately afterwards it is increased again.
		 * So, it does not change here and we don't
		 * have to worry about adjusting it at all.
		 */
		if (stsw_slli_ukkonen_prolong_suffixes(variation,
					&starting_position, i,
					&active_index, &active_node,
					tfsw, stsw) > 0) {
			fprintf(stderr,	"Could not prolong "
					"the suffixes to end "
					"at the position %zu. "
					"Exiting.\n", i);
			/* the start of the critical section */
			pthread_mutex_lock(&sd.mx);
			/*
			 * There was an error, so we need to terminate
			 * the reading thread, if it is still running.
			 * But at first, we have to wake it up,
			 * because it might be sleeping.
			 * We also have to raise the reading_finished
			 * flag to force the reading thread
			 * to terminate immediately.
			 */
			sd.reading_finished = 1;
			pthread_cond_signal(&sd.cv);
			pthread_mutex_unlock(&sd.mx);
			/*
			 * the end of the critical section
			 *
			 * we need to join with the reading thread
			 * at first and just then return failure
			 */
			function_retval = 10;
			goto thread_joining;
		}
		/*
		 * the size of the sliding window
		 * is decremented and incremented here,
		 * so it does not change
		 */
		if (verbosity_level > 1) {
			/*
			 * we increment the text index
			 * of the beginning of the currently
			 * active part of the sliding window
			 */
			++ap_window_begin_text_idx;
			/*
			 * we also increment
			 * the number of observations
			 */
			++observations;
			/*
			 * and finally we try to update
			 * all the statistical variables
			 */
			if (stsw->branching_nodes <
					min_branching_nodes) {
				min_branching_nodes =
					stsw->branching_nodes;
				min_brn_ap_bti =
					ap_window_begin_text_idx;
			}
			avg_branching_nodes +=
				(double)(stsw->branching_nodes);
			if (stsw->branching_nodes >
					max_branching_nodes) {
				max_branching_nodes =
					stsw->branching_nodes;
				max_brn_ap_bti =
					ap_window_begin_text_idx;
			}
		}
		/* adjusting the value of i for the next iteration */
		++i;
		/*
		 * The index of the most recently processed block
		 * needs not to be changed inside the critical section,
		 * because its value is checked by this thread only.
		 */
		if (block_to_process >= tfsw->ap_scale_factor) {
			tfsw->sw_mrp_block = block_to_process -
				tfsw->ap_scale_factor;
		} else { /* block_to_process < tfsw->ap_scale_factor */
			tfsw->sw_mrp_block = tfsw->sw_blocks -
				tfsw->ap_scale_factor + block_to_process;
		}
		/*
		 * If the desired edge label maintenance method
		 * is the batch update by M. Senft, we have to release
		 * the blocks for reading in two steps.
		 * At first, we mark the block as still in use.
		 * This is done here.
		 * Then, when it is clear that no edge labels
		 * are referencing the certain blocks in question,
		 * we release them completely so that they will be
		 * available for reading.
		 */
		if (tfsw->elm_method == 1) {
			/*
			 * We set the appropriate block flag.
			 * Note that we can safely do it outside of
			 * the critical section, because despite that
			 * we are modifying the shared data,
			 * the modifications we are making
			 * do not influence the behaviour
			 * of the reading thread.
			 */
			tfsw->sw_block_flags[tfsw->sw_mrp_block] =
				BLOCK_STATUS_STILL_IN_USE;
		} else {
			/* the start of the critical section */
			pthread_mutex_lock(&sd.mx);
			/* we set the appropriate block flag */
			tfsw->sw_block_flags[tfsw->sw_mrp_block] =
				BLOCK_STATUS_UNKNOWN;
			/*
			 * We signal that the status of the most recently
			 * processed block has changed.
			 */
			pthread_cond_signal(&sd.cv);
			pthread_mutex_unlock(&sd.mx);
			/* the end of the critical section */
		}
		++blocks_processed;
		if (verbosity_level > 0) {
			fprintf(stderr, "\rThe block number %zu has just been "
					"processed (%zu blocks in total).",
					tfsw->sw_mrp_block,
					blocks_processed);
		}
		/* if the traversal type of benchmark has been requested */
		if (benchmark == 2) {
			fprintf(stream, "\nIntermediate suffix tree "
					"traversal:\n");
			fprintf(stream, "The suffix tree is built over "
					"the %zu characters,\n"
					"starting at the %zu.th "
					"character of the input "
					"text.\n\n",
					tfsw->max_ap_window_size,
					ap_window_begin_text_idx);
			/*
			 * we traverse the suffix tree
			 * each time we process the whole block
			 */
			if (stsw_slli_traverse(verbosity_level, stream,
						traversal_type,
						tfsw, stsw) != 0) {
				fprintf(stderr, "Error: The intermediate "
						"suffix tree traversal "
						"has failed. Exiting!\n");
				/* the start of the critical section */
				pthread_mutex_lock(&sd.mx);
				/*
				 * There was an error, so we need to terminate
				 * the reading thread, if it is still running.
				 * But at first, we have to wake it up,
				 * because it might be sleeping.
				 * We also have to raise the reading_finished
				 * flag to force the reading thread
				 * to terminate immediately.
				 */
				sd.reading_finished = 1;
				pthread_cond_signal(&sd.cv);
				pthread_mutex_unlock(&sd.mx);
				/*
				 * the end of the critical section
				 *
				 * we need to join with the reading thread
				 * at first and just then return failure
				 */
				function_retval = 11;
				goto thread_joining;
			}
		}
		/*
		 * If the desired edge label maintenance method
		 * is the batch update by M. Senft and if the total number
		 * of blocks processed so far is divisible by the active part
		 * scale factor, we have to perform the previously mentioned
		 * batch update of the edge labels right now.
		 */
		if ((tfsw->elm_method == 1) &&
				(blocks_processed %
				(tfsw->ap_scale_factor) == 0)) {
			if (verbosity_level > 1) {
				fprintf(stderr, "\nPerforming the batch "
						"update of the edge "
						"labels ...");
			}
			if (stsw_slli_edge_labels_batch_update(tfsw,
						stsw) > 0) {
				fprintf(stderr, "Error: The batch update "
						"of the edge labels "
						"has failed. Exiting!\n");
				/* the start of the critical section */
				pthread_mutex_lock(&sd.mx);
				/*
				 * There was an error, so we need to terminate
				 * the reading thread, if it is still running.
				 * But at first, we have to wake it up,
				 * because it might be sleeping.
				 * We also have to raise the reading_finished
				 * flag to force the reading thread
				 * to terminate immediately.
				 */
				sd.reading_finished = 1;
				pthread_cond_signal(&sd.cv);
				pthread_mutex_unlock(&sd.mx);
				/*
				 * the end of the critical section
				 *
				 * we need to join with the reading thread
				 * at first and just then return failure
				 */
				function_retval = 12;
				goto thread_joining;
			}
			if (verbosity_level > 1) {
				fprintf(stderr, " Success!\n");
			}
			/*
			 * Computing the index of the block,
			 * which will be made available for reading.
			 */
			block_to_release = tfsw->sw_mrp_block + 1;
			if (block_to_release == tfsw->sw_blocks) {
				block_to_release = 0;
			}
			if (block_to_release > tfsw->ap_scale_factor) {
				block_to_release = block_to_release -
					tfsw->ap_scale_factor;
			} else {
				/*
				 * block_to_release <=
				 * tfsw->ap_scale_factor
				 */
				block_to_release = tfsw->sw_blocks +
					block_to_release -
					tfsw->ap_scale_factor;
			}
			/* the start of the critical section */
			pthread_mutex_lock(&sd.mx);
			/* we set the appropriate block flags */
			if (block_to_release <= tfsw->sw_mrp_block) {
				for (; block_to_release <= tfsw->sw_mrp_block;
						++block_to_release) {
					tfsw->sw_block_flags[
						block_to_release] =
						BLOCK_STATUS_UNKNOWN;
				}
			} else { /* block_to_release > tfsw->sw_mrp_block */
				for (; block_to_release < tfsw->sw_blocks;
						++block_to_release) {
					tfsw->sw_block_flags[
						block_to_release] =
						BLOCK_STATUS_UNKNOWN;
				}
				/* here, block_to_release == tfsw->sw_blocks */
				for (block_to_release = 0;
						block_to_release <=
						tfsw->sw_mrp_block;
						++block_to_release) {
					tfsw->sw_block_flags[
						block_to_release] =
						BLOCK_STATUS_UNKNOWN;
				}
			}
			/*
			 * We signal that the statuses of the blocks,
			 * which have previously been still in use has changed.
			 */
			pthread_cond_signal(&sd.cv);
			pthread_mutex_unlock(&sd.mx);
		}
	}
thread_joining:
	if (verbosity_level > 1) {
		printf("Thread joining begins\n");
	}
	/* we try to join with the reading thread */
	pthread_join(reader, &thread_retval);
	/* deallocation of the mutex attributes */
	pthread_mutexattr_destroy(&mutex_attributes);
	/* deallocation of the mutex itself */
	pthread_mutex_destroy(&sd.mx);
	/* deallocation of the condition variable */
	pthread_cond_destroy(&sd.cv);
	/* now we need to check for possible errors and return if necessary */
	if (thread_retval != 0) {
		fprintf(stderr, "Error: The reading thread "
				"has finished unsuccessfully!\n");
		return (13);
	}
	if (function_retval != 0) {
		fprintf(stderr, "Error: The processing thread "
				"has encountered errors!\n");
		return (function_retval);
	}
	/*
	 * We store the number of characters in the final block
	 * into a variable, which is also accessible
	 * from the common part of this function (i.e. the part,
	 * which is common for both the pthread and non-pthread
	 * implementation types).
	 */
	final_block_characters = sd.final_block_characters;
	/*
	 * we also have to make the final block number
	 * available for non-pthread part of this function
	 */
	final_block_number = sd.final_block_number;
#else /* non-pthread part */
	/*
	 * At first, we try to read all the blocks in the active part
	 * of the sliding window.
	 */
	if ((retval = text_file_read_blocks(tfsw->ap_scale_factor,
					&blocks_read, &characters_read,
					&bytes_read, tfsw)) > 1) {
		fprintf(stderr, "stsw_slli_create_ukkonen:\n"
				"file reading error (1)\n");
		return (14);
	} else if (retval == 0) { /* the reading has been successful */
		/*
		 * we adjust the ending_position
		 * for the last of the blocks read
		 */
		ending_position = blocks_read *
			/*
			 * the text in the sliding window
			 * starts at the index of 1
			 */
			tfsw->sw_block_size + 1;
	/*
	 * if the reading of the initial blocks
	 * has been partially successful
	 * (meaning that the underlying text was too short
	 * to fill in the entire active part
	 * of the sliding window)
	 */
	} else if (retval == 1) {
		/* we check if the input file was empty */
		if (blocks_read == 0) {
			/* in which case we print an error message */
			fprintf(stderr, "stsw_slli_create_ukkonen:\n"
					"Error: The input file is empty!\n");
			return (15); /* and return failure */
		}
		/*
		 * we need to store the number of characters
		 * read into the final block
		 */
		final_block_characters = characters_read -
			(blocks_read - 1) * tfsw->sw_block_size;
		/*
		 * we adjust the ending_position
		 * for the last of the blocks read
		 */
		ending_position =
			/*
			 * the text in the sliding window
			 * starts at the index of 1
			 */
			characters_read + 1;
	} else { /* retval < 0 */
		fprintf(stderr, "stsw_slli_create_ukkonen:\n"
				"Error: Unknown return value (%d)\n"
				"from the text_file_read_blocks "
				"function. Exiting!\n",
				retval);
		return (16);
	}
	/* setting the block flags (block_to_process == 0 here) */
	for (; block_to_process < blocks_read;
			++block_to_process) {
		tfsw->sw_block_flags[block_to_process] =
			BLOCK_STATUS_READ_AND_UNPROCESSED;
	}
	if (verbosity_level > 0) {
		printf("The first part:\n"
				"populating the active part "
				"of the sliding window ...\n");
	}
	/*
	 * The first part.
	 *
	 * We have to process the first a few blocks separately, until
	 * the active part of the sliding window reaches its maximum size,
	 * or until there are no more blocks read.
	 * Here, we will not be adjusting the index of the most recently
	 * processed block, nor the block flags.
	 * During this part, we will just prolong the suffixes
	 * previously present in the suffix tree and we will
	 * not be deleting the longest suffixes.
	 * The reason is that at first, we would like to fill in
	 * all the space available for the suffix tree so that
	 * it will contain all the suffixes from the active part
	 * of the sliding window of the maximum possible size.
	 */
	/*
	 * We need not to set the starting_position or i here.
	 * The starting_position is already equal to one
	 * and i is already equal to two here, because these variables
	 * haven't been changed since their definition.
	 */
	/* Now we have the recently read blocks ready to be processed. */
	for (; i <= ending_position; ++i) {
		/*
		 * In this initial part,
		 * we are not deleting the longest suffix.
		 */
		++tfsw->ap_window_end;
		/*
		 * we have to increase the size
		 * of the sliding window as well
		 */
		++tfsw->ap_window_size;
		if (stsw_slli_ukkonen_prolong_suffixes(variation,
					&starting_position, i,
					&active_index, &active_node,
					tfsw, stsw) > 0) {
			fprintf(stderr,	"Could not prolong "
					"the suffixes to end "
					"at the position %zu. "
					"Exiting.\n", i);
			return (17);
		}
	}
	if (verbosity_level > 1) {
		/*
		 * we initialize the text index of the beginning
		 * of the currently active part of the sliding window
		 */
		ap_window_begin_text_idx = 1;
		/* also, we initialize the number of observations */
		observations = 1;
		/* as well as all the statistical variables */
		min_branching_nodes = stsw->branching_nodes;
		min_brn_ap_bti = ap_window_begin_text_idx;
		avg_branching_nodes = (double)(stsw->branching_nodes);
		max_branching_nodes = stsw->branching_nodes;
		max_brn_ap_bti = ap_window_begin_text_idx;
	}
	/*
	 * checking the return value from the latest call
	 * to the text_file_read_blocks function
	 */
	if (retval == 1) { /* if no more blocks will be read */
		/*
		 * we can safely skip the main part
		 * and jump directly to the final part
		 */
		goto final_part;
	}
	/* if the traversal type of benchmark has been requested */
	if (benchmark == 2) {
		fprintf(stream, "Initial suffix tree traversal:\n");
		fprintf(stream, "The suffix tree is built over the initial "
			"%zu characters\nof the input text.\n\n",
			stsw->tleaf_last - stsw->tleaf_first + 1);
		/*
		 * we traverse the suffix tree here,
		 * because it has just grown to its maximum size
		 */
		if (stsw_slli_traverse(verbosity_level, stream, traversal_type,
					tfsw, stsw) != 0) {
			fprintf(stderr, "Error: The initial suffix tree "
					"traversal has failed. Exiting!\n");
			return (18);
		}
	}
	/*
	 * Here block_to_process == blocks_read,
	 * so we have to decrement it, because in the beginning
	 * of the next part it will be incremented again.
	 * We can also be sure that block_to_process >= 1,
	 * because if no blocks have been read,
	 * we would have already returned.
	 */
	--block_to_process;
	if (verbosity_level > 0) {
		printf("Main part:\n"
				"maintaining the suffix tree "
				"over the sliding window ...\n");
	}
	/*
	 * The main part.
	 *
	 * This is where the processing of the other blocks takes place.
	 * Here, we will be finally adjusting the index
	 * of the most recently processed block and the block flags.
	 * During this part, we will be deleting the longest suffixes
	 * currently present in the suffix tree as well as prolonging
	 * the suffixes currently present in the suffix tree.
	 * The reason is that we would like to keep the smallest possible
	 * size of the suffix tree, while still containing all the suffixes
	 * from the active part of the sliding window in the suffix tree.
	 */
	while (1) {
		++block_to_process;
		if (block_to_process == tfsw->sw_blocks) {
			block_to_process = 0;
		}
		/*
		 * we know that the current block is not ready
		 * to be processed, so we read it at first
		 */
		if ((retval = text_file_read_blocks((size_t)(1),
					&blocks_read, &characters_read,
					&bytes_read, tfsw)) > 1) {
			fprintf(stderr, "stsw_slli_create_ukkonen:\n"
					"file reading error (2)\n");
			return (19);
		} else if (retval == 0) { /* the whole block has been read */
			/*
			 * We adjust the ending_position
			 * for the current block.
			 */
			ending_position = (block_to_process + 1) *
				/*
				 * the text in the sliding window
				 * starts at the index of 1
				 */
				tfsw->sw_block_size + 1;
		/*
		 * if the reading of the current block
		 * has been partially successful
		 * (meaning that the remaining part of the underlying text
		 * was too short to fill in the entire block
		 * of the sliding window)
		 */
		} else if (retval == 1) {
			/* if no characters at all have been read */
			if (characters_read == 0) {
				/* we can finish this part */
				break;
			}
			/*
			 * we need to store the number of characters
			 * in the final block
			 */
			final_block_characters = characters_read;
			/*
			 * We adjust the ending_position
			 * for the current block.
			 */
			ending_position = block_to_process *
				tfsw->sw_block_size +
				/*
				 * the text in the sliding window
				 * starts at the index of 1
				 */
				final_block_characters + 1;
		} else { /* retval < 0 */
			fprintf(stderr, "stsw_slli_create_ukkonen:\n"
					"Error: Unknown return value (%d) "
					"from the text_file_read_blocks "
					"function.\n", retval);
			return (20);
		}
		/*
		 * Setting the appropriate block flag.
		 * The tfsw->sw_mrr_block variable is updated
		 * inside the text_file_read_blocks function.
		 */
		tfsw->sw_block_flags[tfsw->sw_mrr_block] =
			BLOCK_STATUS_READ_AND_UNPROCESSED;
		/*
		 * The current block is now ready to be processed!
		 *
		 * We need not to set the starting_position or i here.
		 * They already have their correct values set
		 * from the previous calls of the functions
		 * stsw_slli_delete_longest_suffix
		 * and stsw_slli_ukkonen_prolong_suffixes.
		 */
		for (; i < ending_position; ++i) {
			/* at first, we have to delete the longest suffix */
			if (stsw_slli_delete_longest_suffix(&starting_position,
						&active_index, &active_node,
						tfsw, stsw) > 0) {
				fprintf(stderr,	"Could not delete "
						"the longest suffix\n"
						"starting at the position %zu "
						"and ending at the position "
						"%zu. Exiting.\n",
						tfsw->ap_window_begin,
						tfsw->ap_window_end);
				return (21);
			}
			++tfsw->ap_window_begin;
			/* and then we can prolong the current suffixes */
			++tfsw->ap_window_end;
			/*
			 * The size of the sliding window is decreased at first
			 * and immediately afterwards it is increased again.
			 * So, it does not change here and we don't
			 * have to worry about adjusting it at all.
			 */
			if (stsw_slli_ukkonen_prolong_suffixes(variation,
						&starting_position, i,
						&active_index, &active_node,
						tfsw, stsw) > 0) {
				fprintf(stderr,	"Could not prolong "
						"the suffixes to end "
						"at the position %zu. "
						"Exiting.\n", i);
				return (22);
			}
			/*
			 * the size of the sliding window
			 * is decremented and incremented here,
			 * so it does not change
			 */
			if (verbosity_level > 1) {
				/*
				 * we increment the text index
				 * of the beginning of the currently
				 * active part of the sliding window
				 */
				++ap_window_begin_text_idx;
				/*
				 * we also increment
				 * the number of observations
				 */
				++observations;
				/*
				 * and finally we try to update
				 * all the statistical variables
				 */
				if (stsw->branching_nodes <
						min_branching_nodes) {
					min_branching_nodes =
						stsw->branching_nodes;
					min_brn_ap_bti =
						ap_window_begin_text_idx;
				}
				avg_branching_nodes +=
					(double)(stsw->branching_nodes);
				if (stsw->branching_nodes >
						max_branching_nodes) {
					max_branching_nodes =
						stsw->branching_nodes;
					max_brn_ap_bti =
						ap_window_begin_text_idx;
				}
			}
		}
		/* i == ending_position here */
		if (i > tfsw->total_window_size) {
			i = 1; /* this variable can only overflow by one */
		}
		/* at first, we have to delete the longest suffix */
		if (stsw_slli_delete_longest_suffix(&starting_position,
					&active_index, &active_node,
					tfsw, stsw) > 0) {
			fprintf(stderr,	"Could not delete "
					"the longest suffix\n"
					"starting at the position %zu "
					"and ending at the position "
					"%zu. Exiting.\n",
					tfsw->ap_window_begin,
					tfsw->ap_window_end);
			return (23);
		}
		++tfsw->ap_window_begin;
		if (tfsw->ap_window_begin > tfsw->total_window_size) {
			/* this variable can only overflow by one */
			tfsw->ap_window_begin = 1;
		}
		/* and then we can prolong the current suffixes */
		++tfsw->ap_window_end;
		if (tfsw->ap_window_end > tfsw->total_window_size) {
			/* this variable can only overflow by one */
			tfsw->ap_window_end = 1;
		}
		/*
		 * The size of the sliding window is decreased at first
		 * and immediately afterwards it is increased again.
		 * So, it does not change here and we don't
		 * have to worry about adjusting it at all.
		 */
		if (stsw_slli_ukkonen_prolong_suffixes(variation,
					&starting_position, i,
					&active_index, &active_node,
					tfsw, stsw) > 0) {
			fprintf(stderr,	"Could not prolong "
					"the suffixes to end "
					"at the position %zu. "
					"Exiting.\n", i);
			return (24);
		}
		/*
		 * the size of the sliding window
		 * is decremented and incremented here,
		 * so it does not change
		 */
		if (verbosity_level > 1) {
			/*
			 * we increment the text index
			 * of the beginning of the currently
			 * active part of the sliding window
			 */
			++ap_window_begin_text_idx;
			/*
			 * we also increment
			 * the number of observations
			 */
			++observations;
			/*
			 * and finally we try to update
			 * all the statistical variables
			 */
			if (stsw->branching_nodes <
					min_branching_nodes) {
				min_branching_nodes =
					stsw->branching_nodes;
				min_brn_ap_bti =
					ap_window_begin_text_idx;
			}
			avg_branching_nodes +=
				(double)(stsw->branching_nodes);
			if (stsw->branching_nodes >
					max_branching_nodes) {
				max_branching_nodes =
					stsw->branching_nodes;
				max_brn_ap_bti =
					ap_window_begin_text_idx;
			}
		}
		/* adjusting the value of i for the next iteration */
		++i;
		/*
		 * FIXME: Fix the ambiguity between
		 * the variable block_to_process
		 * and the advertised number of block
		 * which has just been processed.
		 */
		/* we change the index of the most recently processed block */
		if (block_to_process >= tfsw->ap_scale_factor) {
			tfsw->sw_mrp_block = block_to_process -
				tfsw->ap_scale_factor;
		} else { /* block_to_process < tfsw->ap_scale_factor */
			tfsw->sw_mrp_block = tfsw->sw_blocks -
				tfsw->ap_scale_factor + block_to_process;
		}
		/* and finally we set the appropriate block flag */
		tfsw->sw_block_flags[tfsw->sw_mrp_block] =
			BLOCK_STATUS_UNKNOWN;
		++blocks_processed;
		if (verbosity_level > 0) {
			fprintf(stderr, "\rThe block number %zu has just been "
					"processed (%zu blocks in total).",
					tfsw->sw_mrp_block,
					blocks_processed);
		}
		/* if the traversal type of benchmark has been requested */
		if (benchmark == 2) {
			fprintf(stream, "\nIntermediate suffix tree "
					"traversal:\n");
			fprintf(stream, "The suffix tree is built over "
					"the %zu characters,\n"
					"starting at the %zu.th "
					"character of the input "
					"text.\n\n",
					tfsw->max_ap_window_size,
					ap_window_begin_text_idx);
			/*
			 * we traverse the suffix tree
			 * each time we process the whole block
			 */
			if (stsw_slli_traverse(verbosity_level, stream,
						traversal_type,
						tfsw, stsw) != 0) {
				fprintf(stderr, "Error: The intermediate "
						"suffix tree traversal "
						"has failed. Exiting!\n");
				return (25);
			}
		}
		/*
		 * If the desired edge label maintenance method
		 * is the batch update by M. Senft and if the total number
		 * of blocks processed so far is divisible by the active part
		 * scale factor, we have to perform the previously mentioned
		 * batch update of the edge labels right now.
		 */
		if ((tfsw->elm_method == 1) &&
				(blocks_processed %
				(tfsw->ap_scale_factor) == 0)) {
			if (verbosity_level > 1) {
				fprintf(stderr, "\nPerforming the batch "
						"update of the edge "
						"labels ...");
			}
			if (stsw_slli_edge_labels_batch_update(tfsw,
						stsw) > 0) {
				fprintf(stderr, "Error: The batch update "
						"of the edge labels "
						"has failed. Exiting!\n");
				return (26);
			}
			if (verbosity_level > 1) {
				fprintf(stderr, " Success!\n");
			}
		}
	}
	/*
	 * the last block which has been read
	 * is also the last block which has been processed
	 */
	final_block_number = block_to_process;
final_part:
#endif
	/*
	 * Here, we have to adjust the block_to_process, so that it will refer
	 * to the block, which has been most recently processed.
	 * If no blocks have been processed so far,
	 * it means that the tfsw->sw_mrp_block is left at its default value
	 * of tfsw->sw_blocks - 1, which is very convenient in this case.
	 * The reason is that block_to_process will be incremented
	 * immediately at the beginning of the final part.
	 */
	block_to_process = tfsw->sw_mrp_block;
	/*
	 * The final part.
	 *
	 * This is where the processing of the last blocks takes place.
	 * During this part, we will be deleting the longest suffixes
	 * currently present in the suffix tree, but we will not be
	 * prolonging the suffixes currently present in the suffix tree.
	 * The reason is that we would like to iteratively decrease
	 * the size of the sliding window by moving its beginning
	 * towards its end.
	 * Finally, we should end up with the empty sliding window
	 * as well as the empty suffix tree.
	 *
	 * If we have the POSIX threads enabled,
	 * the reading thread must have already finished,
	 * so we can have the final part common for both
	 * pthread and non-pthread implementations.
	 */
	if (verbosity_level > 0) {
		printf("Final part:\n"
				"shrinking the active part "
				"of the sliding window ...\n");
	}
	back_position = (block_to_process + 1) * tfsw->sw_block_size + 1;
	if (back_position > tfsw->total_window_size) {
		back_position = 1; /* this variable can only overflow by one */
	}
	i = back_position;
	while (1) {
		++block_to_process;
		if (block_to_process == tfsw->sw_blocks) {
			block_to_process = 0;
		}
		/* if the current block is not ready to be processed */
		if (tfsw->sw_block_flags[block_to_process] !=
				BLOCK_STATUS_READ_AND_UNPROCESSED) {
			/*
			 * We know that the reading thread has already
			 * finished, so it means that no more blocks
			 * will be read, because the previously
			 * processed block was the last block read.
			 * That's why we can safely end this part.
			 */
			break;
		}
		/*
		 * The current block is now ready to be processed!
		 *
		 * if the current block is the last block
		 */
		if (block_to_process == final_block_number) {
			/* we just process it */
			back_position += final_block_characters;
		/* otherwise, there are more blocks ready to be processed */
		} else {
			/* but right now, we process the current block only */
			back_position += tfsw->sw_block_size;
		}
		/*
		 * We need not to set the starting_position here.
		 * It already has the correct value set
		 * from the previous calls of the functions
		 * stsw_slli_delete_longest_suffix
		 * and stsw_slli_ukkonen_prolong_suffixes.
		 */
		for (; i < back_position; ++i) {
			/* at first, we have to delete the longest suffix */
			if (stsw_slli_delete_longest_suffix(&starting_position,
						&active_index, &active_node,
						tfsw, stsw) > 0) {
				fprintf(stderr,	"Could not delete "
						"the longest suffix\n"
						"starting at the position %zu "
						"and ending at the position "
						"%zu. Exiting.\n",
						tfsw->ap_window_begin,
						tfsw->ap_window_end);
						/*
						 * FIXME: Fix the deletion
						 * block logic and the error
						 * message to use the "real"
						 * block boundaries!
						 */
				return (27);
			}
			++tfsw->ap_window_begin;
			/*
			 * we have to decrease the size
			 * of the sliding window as well
			 */
			--tfsw->ap_window_size;
			/* we are not prolonging any suffixes here */
		}
		/* i == back_position here */
		if (i > tfsw->total_window_size) {
			i = 1; /* this variable can only overflow by one */
		}
		if (back_position > tfsw->total_window_size) {
			/* also this variable can only overflow by one */
			back_position = 1;
		}
		if (tfsw->ap_window_begin > tfsw->total_window_size) {
			/*
			 * Watch out! Unlike the other variables of similar
			 * purpose in this function, this variable
			 * at this place can overflow by more than one!
			 */
			tfsw->ap_window_begin -= tfsw->total_window_size;
		}
		/* we update the index of the most recently processed block */
		tfsw->sw_mrp_block = block_to_process;
		/*
		 * FIXME: Watch out! The block boundaries here
		 * do not correspond to the block boundaries
		 * in the other parts of this function.
		 * This is not a problem while the block alignment
		 * is not assumed.
		 * However, it would be nice to have it fixed...
		 */
		/*
		 * Finally, we update the block flag,
		 * without the need to enter the critical section.
		 */
		tfsw->sw_block_flags[tfsw->sw_mrp_block] =
			BLOCK_STATUS_UNKNOWN;
		++blocks_processed;
		if (verbosity_level > 0) {
			fprintf(stderr, "\rThe block number %zu has just been "
					"processed (%zu blocks in total).",
					tfsw->sw_mrp_block,
					blocks_processed);
		}
	}
	if (verbosity_level > 0) {
		printf("\n");
	}
	printf("\nThe suffix tree has been successfully created "
			"and maintained.\n");
	/*
	 * FIXME: Fill in the stats when only one block
	 * has been processed in the pthread implementation type!
	 */
	avg_branching_nodes /= (double)(observations);
	stsw_print_stats(verbosity_level, tfsw->characters_read,
			tfsw->max_ap_window_size,
			/*
			 * the following zero values correspond
			 * to the edge statistics, which are unavailable
			 * when using the SL implementation technique
			 */
			(size_t)(0), (size_t)(0), (double)(0),
			(size_t)(0), (size_t)(0),
			min_branching_nodes, min_brn_ap_bti,
			avg_branching_nodes, max_branching_nodes,
			max_brn_ap_bti, (size_t)(0),
			stsw->tbranch_size,
			stsw->lr_size, (size_t)(0), stsw->br_size,
			/*
			 * the size of the additional
			 * memory allocated
			 */
			stsw->tbdeleted_size *
			sizeof (unsigned_integral_type),
			/*
			 * the size of the additional
			 * memory used
			 */
			(stsw->max_tbdeleted_index + 1) *
			sizeof (unsigned_integral_type));
	return (0);
}
