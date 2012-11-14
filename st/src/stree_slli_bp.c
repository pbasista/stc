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
 * SLLI_BP functions implementation.
 * This file contains the implementation of the functions,
 * which are used for the construction
 * of the suffix tree in the memory
 * using the implementation type SLLI with the backward pointers.
 */
#include "stree_slli_bp.h"

#include <stdio.h>
#include <stdlib.h>

/* supporting functions */

/**
 * A function which simulates the suffix link
 * using a novel bottom-up approach.
 *
 * @param
 * parent	The node for which we need to find its suffix link.
 * 		When this function successfully returns, it will be set
 * 		either to the suffix link's target itself (if it already
 * 		exists) or to the parent of the new suffix link's target
 * 		(if the suffix link's target itself does not exist yet).
 * @param
 * child	the most recent child of the "parent" node (not the newly
 * 		created leaf node!)
 * 		During this function, it will be set to one of the suffix
 * 		link target's children, even if the suffix link's target
 * 		itself does not exist yet. The chosen child will be
 * 		the only such one, which is present on the path
 * 		from the original child node to the root. The changes made
 * 		to this variable are not propagated outside of this function.
 * @param
 * target_depth	the depth of the suffix link's target
 * @param
 * stree	the actual suffix tree
 *
 * @return	If the suffix link simulation has been successful,
 * 		this function returns 0.
 * 		If the suffix link's target does not exist yet, we set up
 * 		the parent and the child variables appropriately
 * 		and return (-1).
 * 		If an error occurs, a positive error number is returned.
 */
int st_slli_bp_simulate_suffix_link_bottom_up (signed_integral_type *parent,
		signed_integral_type child,
		unsigned_integral_type target_depth,
		suffix_tree_slli_bp *stree) {
	/* the node from which the suffix link starts */
	signed_integral_type starting_from = (*parent);
	int tmp = 0; /* the return value of the ascending function */
	if (child == 0) { /* child number is undefined */
		fprintf(stderr,	"Error: Invalid number of child (0)!\n");
		return (1); /* child number is invalid */
	} else if (child > 0) { /* child is a branching node */
		/*
		 * We can use a suffix link transition
		 * from the branching node.
		 */
		child = stree->tbranch[child].suffix_link;
	} else { /* child < 0 (child is a leaf) */
		/*
		 * We can use a suffix link transition
		 * from the leaf node.
		 */
		child = child - 1;
	}
	(*parent) = 0; /* we invalidate the parent's node */
	/* if we were able to locate the desired depth */
	if ((tmp = st_slli_bp_go_up(parent, &child, target_depth,
					stree)) == 0) {
		/* we establish a new suffix link */
		stree->tbranch[starting_from].suffix_link = (*parent);
		return (0);
	/*
	 * we could not find the suffix link's target,
	 * because it does not exist yet
	 */
	} else if (tmp == (-1)) {
		/* suffix link simulation was partially successful */
		return (-1);
	} else { /* something went wrong */
		fprintf(stderr,	"Error: Suffix link simulation "
				"failed permanently!\n");
		return (2); /* a failure */
	}
}

/**
 * A function which inserts suffix starting at the given position of the text
 * into the suffix tree. This function utilizes the suffix links
 * in compliance with the McCreight's algorithm in order to
 * make the construction faster.
 *
 * @param
 * parent	A branching node from which the insertion starts.
 * 		When the function returns, the (*parent) is set to the
 * 		node, from which we can safely start the insertion
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
 * starting_position	the position in the text of the first character
 * 			of the suffix to be inserted
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	If the insertion of the suffix has been successful,
 * 		this function returns 0.
 * 		If an error occurs, a nonzero error number is returned.
 */
int st_slli_bp_mccreight_insert_suffix (signed_integral_type *parent,
		signed_integral_type *sl_source,
		unsigned_integral_type *sl_targets_depth,
		size_t starting_position,
		const character_type *text,
		size_t length,
		suffix_tree_slli_bp *stree) {
	signed_integral_type latest_child = 0;
	signed_integral_type child = 0;
	signed_integral_type prev_child = 0;
	/*
	 * The position of the last character on the current
	 * parent->child edge, which matches a substring of a suffix starting
	 * at the position "starting_position".
	 *
	 * Its sign indicates the type of the following character's mismatch.
	 */
	signed_integral_type last_match_position = 0;
	/*
	 * The position in the text of the current letter
	 * of the suffix to be inserted. In the beginning, it is set
	 * to the position in the text of the suffix's first letter
	 * not belonging to an edge leading to the "parent".
	 */
	size_t position = starting_position +
		stree->tbranch[(*parent)].depth;
	size_t tbranch_size = (size_t)(stree->tbranch_size);
	int tmp = 0; /* the return value of the branching function */
	/* if all the branching records are occupied */
	if ((tbranch_size == stree->branching_nodes) &&
			(tbranch_size < length)) {
		/* we need to increase the size of the table tbranch */
		if (st_slli_bp_reallocate((size_t)(0), length, stree) > 0) {
			fprintf(stderr,	"Error: Could not "
					"reallocate memory!\n");
			return (1);
		}
	}
	while ((tmp = st_slli_bp_branch_once((*parent), &child, &prev_child,
					position, text, stree)) == 0) {
		/*
		 * while there is a candidate branching edge,
		 * we check it for a complete match
		 */
		if (st_slli_bp_edge_slowscan((*parent), child, position,
			&last_match_position, text, length + 1, stree) == 0) {
			/*
			 * The whole edge matches, so we descend down along it.
			 *
			 * We must supply the correct maximum length
			 * of the text valid for the suffix tree. It equals
			 * the number of the "real" characters in the text plus
			 * the terminating character ($).
			 */
			st_slli_bp_edge_descend(parent, &child, &prev_child,
					&position, length + 1, stree);
			if ((*parent) < 0) { /* parent is a leaf */
				fprintf(stderr,	"Error: The newly inserted "
						"suffix already has the "
						"corresponding leaf node\n"
						"present in the "
						"suffix tree!\n");
				/*
				 * the function call was unsuccessful,
				 * because it did not insert anything new
				 * into the suffix tree.
				 */
				return (2);
			}
		} else { /* we need to split the edge and insert a new node */
			/* we store the current number of child */
			latest_child = child;
			st_slli_bp_split_edge(parent, &child, &prev_child,
					&position, last_match_position,
				(unsigned_integral_type)(starting_position),
					stree);
			st_slli_bp_create_leaf((*parent), child, prev_child,
				(-(signed_integral_type)(starting_position)),
				stree);
			/* if there is a suffix link to be filled in */
			if ((*sl_source) != 0) {
				if (position == starting_position +
						(*sl_targets_depth)) {
					/*
					 * we have reached the desired depth
					 * of the suffix link's target,
					 *
					 * so we complete this suffix link
					 */
					stree->tbranch[(*sl_source)].
						suffix_link = (*parent);
				} else {
					/*
					 * the current depth is not the depth
					 * of the suffix link's target
					 */
					fprintf(stderr,	"Error! The last "
							"opportunity to "
							"set the target "
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
			(*sl_source) = (*parent);
			/* as well as the depth of a suffix link's target */
			(*sl_targets_depth) =
				stree->tbranch[(*parent)].depth - 1;
			/* if the suffix link simulation was successful */
			if (st_slli_bp_simulate_suffix_link_bottom_up(parent,
					latest_child, (*sl_targets_depth),
					stree) == 0) {
				/* we can invalidate the remembered values */
				(*sl_source) = 0; /* invalidation */
				(*sl_targets_depth) = 0; /* invalidation */
			}
			return (0); /* we return success */
		}
	}
	if (tmp == 1) {
		/*
		 * the number of parent at the moment of branching
		 * is invalid
		 */
		fprintf(stderr,	"Error: Insertion of the suffix number "
				"%zu failed!\n", starting_position);
		return (4);
	} else if (tmp == 2) { /* the parent has no children at all */
		/* we need to create the first child of the parent */
		fprintf(stderr,	"Information: Creating the first child "
				"of the parent.\nThis should not happen "
				"more than once!\n");
		stree->tbranch[(*parent)].first_child =
			(-(signed_integral_type)(starting_position));
		stree->tleaf[starting_position].parent = (*parent);
		stree->tleaf[starting_position].next_brother = 0;
	} else { /* (tmp == 3), which means that there was no matching edge */
		/* we need to create a new child of the parent */
		st_slli_bp_create_leaf((*parent), child, prev_child,
				(-(signed_integral_type)(starting_position)),
				stree);
	}
	/* If the parent is a branching node and it is not the root. */
	if ((*parent) > 1) {
		/* we can use a suffix link transition */
		(*parent) = stree->tbranch[(*parent)].suffix_link;
	}
	return (0); /* we return success */
}

/**
 * A function which prolongs a single suffix. This function utilizes
 * the suffix links in compliance with the Ukkonen's algorithm
 * to speed up the construction.
 *
 * @param
 * starting_position	the position in the text of the first character
 * 			of the suffix to be prolonged
 * @param
 * ending_position	the position in the text just after the last character
 * 			of the suffix after being prolonged
 * @param
 * active_index		The position in the text from which we start
 * 			to prolong the suffix.
 * 			When the function returns, the (*active_index) is set
 * 			to the position in the text, from which we can start
 * 			to prolong the following (shorter) suffix.
 * @param
 * parent	A branching node from which the prolonging starts.
 * 		When the function returns, the (*parent) is set to the
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
 * text		the actual underlying text of the suffix tree
 * @param
 * stree	the actual suffix tree
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
int st_slli_bp_ukkonen_prolong_suffix (size_t starting_position,
		size_t ending_position,
		size_t *active_index,
		signed_integral_type *parent,
		signed_integral_type *sl_source,
		unsigned_integral_type *sl_targets_depth,
		const character_type *text,
		suffix_tree_slli_bp *stree) {
	signed_integral_type latest_child = 0;
	/*
	 * a variable where we will store the immediate parent
	 * of the "parent" node
	 */
	signed_integral_type grandpa = 0;
	signed_integral_type child = 0;
	signed_integral_type prev_child = 0;
	/*
	 * The position of the last character on the current
	 * parent->child edge, which matches a substring of a suffix starting
	 * at the position "starting_position".
	 *
	 * Its sign indicates the type of the following character's mismatch.
	 */
	signed_integral_type last_match_position = 0;
	/*
	 * The position in the text of the current letter
	 * of the suffix to be prolonged. In the beginning, it is set
	 * to the position in the text from which we start
	 * to prolong the suffix. It is the position corresponding to
	 * the first letter of an edge leading from the "active_node"
	 * to the correct child.
	 */
	size_t position = (*active_index);
	int tmp = 0; /* the return value of the branching function */
	int slowscan_return = 0; /* the return value from the "slowscan" */
	/* if there is nothing to be done */
	if (starting_position == ending_position) {
		return (-1); /* we are (successfully) finished */
	}
	while ((tmp = st_slli_bp_branch_once((*parent), &child, &prev_child,
					position, text, stree)) == 0) {
		/*
		 * while there is a candidate branching edge,
		 * we check it for a complete match
		 *
		 * We must supply the correct maximum length of the text
		 * valid for the suffix tree. It equals the last valid
		 * position of the suffix after being prolonged.
		 */
		if ((slowscan_return = st_slli_bp_edge_slowscan((*parent),
				child, position, &last_match_position, text,
				ending_position - 1, stree)) == 0) {
			/*
			 * The whole edge matches.
			 *
			 * We store the current number of parent.
			 */
			grandpa = (*parent);
			/*
			 * We are going to descend down along
			 * the parent->child edge.
			 * We must supply the correct maximum length
			 * of the text valid for the suffix tree.
			 * It equals the last valid position
			 * of the suffix after being prolonged.
			 */
			st_slli_bp_edge_descend(parent, &child, &prev_child,
					&position, ending_position - 1, stree);
			if ((*parent) < 0) {
				/*
				 * We have descended down to a leaf,
				 * which means that we are done.
				 *
				 * The edge leading to this leaf will be
				 * implicitly prolonged when the maximum
				 * length of the text increases.
				 *
				 * We restore the old value of (*parent).
				 */
				(*parent) = grandpa;
				/* if parent is not the root */
				if ((*parent) != 1) {
					/*
					 * We use a suffix link to adjust
					 * the parent variable
					 * for the next suffix.
					 */
					(*parent) = stree->tbranch[(*parent)].
						suffix_link;
				}
				/*
				 * Similarly, we adjust the starting position
				 * for the next suffix according to
				 * the new value of (*parent).
				 *
				 * We have to increase it by one, because
				 * the next suffix will be shorter
				 * by one character.
				 */
				(*active_index) = starting_position + 1 +
				(size_t)(stree->tbranch[(*parent)].depth);
				return (-2); /* success */
			} else if (starting_position +
					stree->tbranch[(*parent)].depth ==
					ending_position) {
				/*
				 * We have descended to the exact
				 * maximum depth.
				 *
				 * We adjust the active position
				 * for the next prolonged suffix.
				 */
				(*active_index) = starting_position +
					stree->tbranch[(*parent)].depth;
				/*
				 * the current suffix is already explicitly
				 * present in the suffix tree and all the
				 * following suffixes need not to be prolonged
				 */
				return (-3);
			}
		} else if (slowscan_return == 2) {
			/*
			 * the edge is longer than necessary,
			 * but its initial letters match
			 *
			 * We adjust the starting position
			 * for the next prolonged suffix.
			 */
			(*active_index) = starting_position +
				stree->tbranch[(*parent)].depth;
			/*
			 * the current suffix is already present in
			 * the suffix tree and all the following suffixes
			 * need not to be prolonged
			 */
			return (-4);
		} else { /* we need to split the edge and insert a new node */
			/* we store the current number of child */
			latest_child = child;
			st_slli_bp_split_edge(parent, &child, &prev_child,
					&position, last_match_position,
				(unsigned_integral_type)(starting_position),
					stree);
			st_slli_bp_create_leaf((*parent), child, prev_child,
				(-(signed_integral_type)(starting_position)),
				stree);
			/* if there is a suffix link to be filled in */
			if ((*sl_source) != 0) {
				if (position == starting_position +
						(*sl_targets_depth)) {
					/*
					 * we have reached the desired depth
					 * of the suffix link's target,
					 *
					 * so we complete this suffix link
					 */
					stree->tbranch[(*sl_source)].
						suffix_link = (*parent);
				} else {
					/*
					 * the current depth is not the depth
					 * of the suffix link's target
					 */
					fprintf(stderr,	"Error! The last "
							"opportunity to "
							"set the target "
							"of a suffix link "
							"missed! (suffix "
							"link source = %d)\n",
							(*sl_source));
					return (1);
				}
			}
			/*
			 * in case suffix link simulation fails, we need
			 * to remember the new source of a suffix link
			 */
			(*sl_source) = (*parent);
			/* as well as the depth of a suffix link's target */
			(*sl_targets_depth) =
				stree->tbranch[(*parent)].depth - 1;
			/* if the suffix link simulation was successful */
			if (st_slli_bp_simulate_suffix_link_bottom_up(parent,
					latest_child, (*sl_targets_depth),
					stree) == 0) {
				/* we can invalidate the remembered values */
				(*sl_source) = 0; /* invalidation */
				(*sl_targets_depth) = 0; /* invalidation */
			}
			/*
			 * We adjust the starting position
			 * of the next prolonged suffix.
			 */
			(*active_index) = starting_position + 1 +
				stree->tbranch[(*parent)].depth;
			return (0); /* we return success */
		}
	}
	if (tmp == 1) {
		/*
		 * the number of parent at the moment of branching
		 * is invalid
		 */
		fprintf(stderr,	"Error: Prolonging of the suffix number "
				"%zu to the length of %zu failed!\n",
				starting_position,
				ending_position - starting_position);
		return (2);
	} else if (tmp == 2) { /* the parent has no children at all */
		/* we need to create the first child of the parent */
		fprintf(stderr,	"Information: Creating the first child "
				"of the parent.\nThis should not happen "
				"more than once!\n");
		stree->tbranch[(*parent)].first_child =
			(-(signed_integral_type)(starting_position));
		stree->tleaf[starting_position].parent = (*parent);
		stree->tleaf[starting_position].next_brother = 0;
	} else { /* (tmp == 3), which means that there was no matching edge */
		/* we need to create a new child of the parent */
		st_slli_bp_create_leaf((*parent), child, prev_child,
				(-(signed_integral_type)(starting_position)),
				stree);
	}
	/* If the parent is a branching node and it is not the root. */
	if ((*parent) > 1) {
		/* we can use a suffix link transition */
		(*parent) = stree->tbranch[(*parent)].suffix_link;
	}
	/*
	 * We adjust the starting position of the next prolonged suffix.
	 */
	(*active_index) = starting_position + 1 +
		stree->tbranch[(*parent)].depth;
	return (0);
}

/**
 * A function to prolong all the suffixes in the intermediate suffix tree
 * T^(n-1) such that it could become the intermediate suffix tree T^n
 * This function utilizes the suffix links in compliance with
 * the Ukkonen's algorithm to speed up the construction.
 *
 * @param
 * starting_position	the position in the text of the first character
 * 			of the first suffix to be prolonged
 * @param
 * ending_position	the position in the text just after the last character
 * 			of the suffixes after being prolonged
 * @param
 * active_index	A position in the text of the character just after
 * 		the last letter of a string which matches the text of an edge
 * 		leading to the current active node.
 * @param
 * active_node	A node from which the construction actively starts.
 * 		Actually this is either the active point or the closest parent
 * 		of the active point. After the function returns,
 * 		it is set to the next active point.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	If the prolonging of the suffixes has been successful,
 * 		this function returns 0.
 * 		If an error occurs, a nonzero error number is returned.
 */
int st_slli_bp_ukkonen_prolong_suffixes (size_t *starting_position,
		size_t ending_position,
		size_t *active_index,
		signed_integral_type *active_node,
		const character_type *text,
		size_t length,
		suffix_tree_slli_bp *stree) {
	signed_integral_type sl_source = 0;
	unsigned_integral_type sl_targets_depth = 0;
	size_t maximum_possible_size = stree->branching_nodes +
		ending_position - (*starting_position) - 1;
	int tmp = 0;
	size_t tbranch_size = (size_t)(stree->tbranch_size);
	if ((tbranch_size < maximum_possible_size) &&
			(tbranch_size < length)) {
		if (st_slli_bp_reallocate(maximum_possible_size,
					length, stree) > 0) {
			fprintf(stderr,	"Error: Could not "
					"reallocate memory!\n");
			return (1);
		}
	}
	while ((tmp = st_slli_bp_ukkonen_prolong_suffix((*starting_position),
					ending_position, active_index,
					active_node, &sl_source,
					&sl_targets_depth, text,
					stree)) == (-1)) {
		++(*starting_position);
	}
	while (tmp == 0) {
		++(*starting_position);
		tmp = st_slli_bp_ukkonen_prolong_suffix((*starting_position),
			ending_position, active_index, active_node, &sl_source,
			&sl_targets_depth, text, stree);
	}
	if (tmp > 0) { /* something went wrong */
		fprintf(stderr,	"Error: Could not prolong suffixes "
				"to the ending position %zu!\n",
				ending_position);
		return (2);
	}
	return (0);
}

/* handling functions */

/**
 * A function which creates a suffix tree for the given text
 * of specified length using McCreight's algorithm.
 *
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the suffix tree which will be created
 *
 * @return	If this function has successfully created the suffix tree,
 * 		it returns 0.
 * 		If an error occurs, a nonzero error number is returned.
 */
int st_slli_bp_create_mccreight (const character_type *text,
		size_t length,
		suffix_tree_slli_bp *stree) {
	/* in the beginning, we are starting from the root */
	signed_integral_type starting_node = 1;
	/*
	 * the source of the suffix link, which needs to be linked
	 * to its target
	 */
	signed_integral_type sl_source = 0;
	/* the depth of the target node of the suffix link */
	unsigned_integral_type sl_targets_depth = 0;
	size_t i = 1;
	printf("Creating suffix tree using McCright's algorithm\n\n");
	if (st_slli_bp_allocate(length, stree) > 0) {
		fprintf(stderr,	"Allocation error. Exiting.\n");
		return (1);
	}
	for (i = 1; i < length + 2; ++i) {
		if (st_slli_bp_mccreight_insert_suffix(&starting_node,
					&sl_source, &sl_targets_depth, i,
					text, length, stree) > 0) {
			fprintf(stderr,	"Could not insert suffix number "
					"%zu. Exiting.\n", i);
			return (2);
		}
	}
	printf("\nThe suffix tree has been successfully created.\n");
	st_print_stats(length, (size_t)(0), stree->branching_nodes,
			(size_t)(0), (size_t)(0), stree->tbranch_size,
			(size_t)(0), stree->lr_size, (size_t)(0),
			stree->br_size, (size_t)(0),
			(size_t)(0), (size_t)(0));
	return (0);
}

/**
 * A function which creates a suffix tree for the given text
 * of specified length using Ukkonen's algorithm.
 *
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the suffix tree which will be created
 *
 * @return	If this function has successfully created the suffix tree,
 * 		it returns 0.
 * 		If an error occurs, a nonzero error number is returned.
 */
int st_slli_bp_create_ukkonen (const character_type *text,
		size_t length,
		suffix_tree_slli_bp *stree) {
	/* The very first active point is the root. */
	signed_integral_type active_node = 1;
	/* The starting point of the first suffix to be prolonged. */
	size_t active_index = 1;
	/* The starting position of the first suffix to be prolonged. */
	size_t starting_position = 1;
	size_t i = 1;
	printf("Creating suffix tree using Ukkonen's algorithm\n\n");
	if (st_slli_bp_allocate(length, stree) > 0) {
		fprintf(stderr,	"Allocation error. Exiting.\n");
		return (1);
	}
	/*
	 * We are starting from 2, because it is the first position just after
	 * the first valid ending position.
	 */
	for (i = 2; i <= length + 2; ++i) {
		if (st_slli_bp_ukkonen_prolong_suffixes(&starting_position, i,
					&active_index, &active_node, text,
					length, stree) > 0) {
			fprintf(stderr,	"Could not create the intermediate "
					"tree number %zu. Exiting.\n", i - 1);
			return (2);
		}
	}
	printf("\nThe suffix tree has been successfully created.\n");
	st_print_stats(length, (size_t)(0), stree->branching_nodes,
			(size_t)(0), (size_t)(0), stree->tbranch_size,
			(size_t)(0), stree->lr_size, (size_t)(0),
			stree->br_size, (size_t)(0),
			(size_t)(0), (size_t)(0));
	return (0);
}
