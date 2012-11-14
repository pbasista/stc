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
 * SHTI functions implementation.
 * This file contains the implementation of the functions,
 * which are used for the construction of the suffix tree
 * in the memory using the implementation type SHTI.
 */
#include "stree_shti.h"

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
 * starting_position	the starting position in the text of the original
 * 			suffix, which has the prefix corresponding
 * 			to the path from the root to the node "parent".
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * ef_length	the currently effective maximum length of the suffix
 * 		valid for the suffix tree
 * @param
 * stree	the actual suffix tree
 *
 * @return	If the suffix link simulation has been successful,
 * 		this function returns 0.
 * 		If the suffix link's target does not exist yet, we set up
 * 		the parent variable appropriately and return (-1).
 * 		If an error occurs, a positive error number is returned.
 */
int st_shti_simulate_suffix_link_top_down (signed_integral_type grandpa,
		signed_integral_type *parent,
		unsigned_integral_type target_depth,
		size_t starting_position,
		const character_type *text,
		size_t ef_length,
		suffix_tree_shti *stree) {
	/*
	 * The position in the text of the beginning of the new suffix
	 * equals the starting position of the original suffix
	 * incremented by one, as we are moving to the suffix
	 * with the length smaller by one, which ends at the same position.
	 * This position is valid only if we are starting from the root.
	 */
	size_t position = starting_position + 1;
	/* the node from which the suffix link starts */
	signed_integral_type starting_from = (*parent);
	int retval = 0; /* the return value of the descending function */
	if (grandpa <= 0) { /* grandpa is either a leaf or undefined */
		fprintf(stderr,	"Error: grandpa (%d) is not a branching "
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
		position = starting_position + stree->tbranch[grandpa].depth;
		grandpa = stree->tbranch[grandpa].suffix_link;
	}
	(*parent) = 0; /* we invalidate the parent's node */
	/* if we were able to locate the desired depth */
	if ((retval = st_shti_go_down(grandpa, parent, target_depth, &position,
			text, ef_length, stree)) == 0) {
		/* we establish a new suffix link */
		stree->tbranch[starting_from].suffix_link = (*parent);
		return (0); /* suffix link simulation was successful */
	} else if (retval == (-1)) {
		/*
		 * the suffix link's target does not exist yet,
		 * but we found its position
		 */
		return (-1); /* partial success */
	} else { /* something went wrong */
		fprintf(stderr,	"Error: Suffix link simulation "
				"failed permanently!\n");
		return (2); /* a failure */
	}
}

/**
 * A function which inserts suffix starting at the given position in the text
 * into the suffix tree. This function does not take advantage
 * of the suffix links.
 *
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
int st_shti_simple_insert_suffix (size_t starting_position,
		const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
	character_type letter = 0;
	/* everytime, we are starting from the root */
	signed_integral_type parent = 1;
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
	 * the position in the text of the current letter
	 * of the suffix to be inserted (in the beginning, it is set
	 * to the position of the suffix's first letter)
	 */
	size_t position = starting_position;
	size_t tbranch_size = stree->tbranch_size;
	int tmp = 0; /* the return value of the branching function */
	/* if all the branching records are occupied */
	if ((tbranch_size == stree->branching_nodes) &&
			(tbranch_size < length)) {
		/* we need to increase the size of the table tbranch */
		if (st_shti_reallocate(tbranch_size + 1, (size_t)(0),
					text, length, stree) > 0) {
			fprintf(stderr,	"Error: Could not "
					"reallocate memory!\n");
			return (1);
		}
	}
	while ((tmp = st_shti_branch_once(parent, &child, position,
					text, stree)) == 0) {
		/*
		 * while there is a candidate branching edge,
		 * we check it for a complete match
		 */
		if (st_shti_edge_slowscan(parent, child, position,
			&last_match_position, text, length + 1, stree) == 0) {
			/*
			 * The whole edge matches, so we descend down along it.
			 *
			 * We must supply the correct maximum length
			 * of the text valid for the suffix tree. It equals
			 * the number of the "real" characters in the text plus
			 * the terminating character ($).
			 */
			st_shti_edge_descend(&parent, &child, &prev_child,
					&position, length + 1, stree);
			if (parent < 0) { /* parent is a leaf */
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
			letter = text[position];
			if (st_shti_split_edge(&parent, letter, &child,
					&position, last_match_position,
				(unsigned_integral_type)(starting_position),
					text, stree) != 0) {
				fprintf(stderr, "Error: Could not split "
						"the parent->child edge!\n");
				return (3);
			}
			letter = text[position];
			if (st_shti_create_leaf(parent, letter,
				(-(signed_integral_type)(starting_position)),
				text, stree)) {
				fprintf(stderr, "Error: Could not create "
						"a new leaf of the parent!\n");
				return (4);
			}
			return (0); /* we return success */
		}
	}
	if (tmp == 1) {
		/*
		 * The number of parent at the moment of branching
		 * is invalid.
		 */
		fprintf(stderr,	"Error: Insertion of the suffix number "
				"%zu failed!\n", starting_position);
		return (5);
	} else { /* (tmp == 2), which means that there was no matching edge */
		letter = text[starting_position + stree->tbranch[parent].depth];
		/* we need to create a new child of the parent */
		if (st_shti_create_leaf(parent, letter,
			(-(signed_integral_type)(starting_position)),
			text, stree)) {
			fprintf(stderr, "Error: Could not create "
					"a new leaf of the parent!\n");
			return (6);
		}
	}
	return (0); /* we return success */
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
int st_shti_mccreight_insert_suffix (signed_integral_type *parent,
		signed_integral_type *sl_source,
		unsigned_integral_type *sl_targets_depth,
		size_t starting_position,
		const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
	character_type letter = 0;
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
	 * of the suffix to be inserted. In the beginning, it is set
	 * to the position in the text of the suffix's first letter
	 * not belonging to an edge leading to the "parent".
	 */
	size_t position = starting_position +
		stree->tbranch[(*parent)].depth;
	size_t tbranch_size = stree->tbranch_size;
	int tmp = 0; /* the return value of the branching function */
	/* if all the branching records are occupied */
	if ((tbranch_size == stree->branching_nodes) &&
			(tbranch_size < length)) {
		/* we need to increase the size of the table tbranch */
		if (st_shti_reallocate(tbranch_size + 1, (size_t)(0),
					text, length, stree) > 0) {
			fprintf(stderr,	"Error: Could not "
					"reallocate memory!\n");
			return (1);
		}
	}
	while ((tmp = st_shti_branch_once((*parent), &child,
					position, text, stree)) == 0) {
		/*
		 * while there is a candidate branching edge,
		 * we check it for a complete match
		 */
		if (st_shti_edge_slowscan((*parent), child, position,
			&last_match_position, text, length + 1, stree) == 0) {
			/*
			 * The whole edge matches, so we descend down along it.
			 *
			 * We must supply the correct maximum length
			 * of the text valid for the suffix tree. It equals
			 * the number of the "real" characters in the text plus
			 * the terminating character ($).
			 */
			st_shti_edge_descend(parent, &child, &prev_child,
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
			/* we store the current number of parent */
			grandpa = (*parent);
			letter = text[position];
			if (st_shti_split_edge(parent, letter, &child,
					&position, last_match_position,
				(unsigned_integral_type)(starting_position),
					text, stree) != 0) {
				fprintf(stderr, "Error: Could not split "
						"the parent->child edge!\n");
				return (3);
			}
			letter = text[position];
			if (st_shti_create_leaf((*parent), letter,
				(-(signed_integral_type)(starting_position)),
				text, stree)) {
				fprintf(stderr, "Error: Could not create "
						"a new leaf of the parent!\n");
				return (4);
			}
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
					return (5);
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
			if (st_shti_simulate_suffix_link_top_down(grandpa,
					parent, (*sl_targets_depth),
					starting_position, text,
					length + 1, stree) == 0) {
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
		return (6);
	} else { /* (tmp == 2), which means that there was no matching edge */
		letter = text[starting_position +
			stree->tbranch[(*parent)].depth];
		/* we need to create a new child of the parent */
		if (st_shti_create_leaf((*parent), letter,
			(-(signed_integral_type)(starting_position)),
			text, stree)) {
			fprintf(stderr, "Error: Could not create "
					"a new leaf of the parent!\n");
			return (7);
		}
	}
	/* If the parent is a branching node and it is not the root. */
	if ((*parent) > 1) {
		/* we can use a suffix link transition */
		(*parent) = stree->tbranch[(*parent)].suffix_link;
	}
	return (0); /* we return success */
}

/**
 * A function which prolongs a single suffix without taking advantage
 * of the suffix links.
 *
 * @param
 * starting_position	the position in the text of the first character
 * 			of the suffix to be prolonged
 * @param
 * ending_position	the position in the text just after the last character
 * 			of the suffix after being prolonged
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	If a new leaf node has been created during the prolonging
 * 		of the edge, we return 0.
 * 		If the length of the suffix to be prolonged is zero,
 * 		which means that there is nothing to be done,
 * 		this function returns (-1).
 * 		If we reach the ending node of the suffix and find out
 * 		that it is a branching node, this function returns (-2).
 * 		If the suffix to be prolonged ends at a leaf node,
 * 		we need not to do anything and this function returns (-3).
 * 		If an error occurs, a positive error number is returned.
 */
int st_shti_simple_prolong_suffix (size_t starting_position,
		size_t ending_position,
		const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
	character_type letter = 0;
	/* we are always starting from the root */
	signed_integral_type parent = 1;
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
	 * the position in the text of the current letter
	 * of the suffix to be prolonged (in the beginning, it is set
	 * to the position of the suffix's first letter)
	 */
	size_t position = starting_position;
	size_t tbranch_size = stree->tbranch_size;
	int tmp = 0; /* the return value of the branching function */
	int slowscan_return = 0; /* the return value from the "slowscan" */
	/* if all the branching records are occupied */
	if ((tbranch_size == stree->branching_nodes) &&
			(tbranch_size < length)) {
		/* we need to increase the size of the table tbranch */
		if (st_shti_reallocate(tbranch_size + 1, (size_t)(0),
					text, length, stree) > 0) {
			fprintf(stderr,	"Error: Could not "
					"reallocate memory!\n");
			return (1);
		}
	}
	/* if there is nothing to be done */
	if (starting_position == ending_position) {
		return (-1); /* we are (successfully) finished */
	}
	while ((tmp = st_shti_branch_once(parent, &child,
					position, text, stree)) == 0) {
		/*
		 * while there is a candidate branching edge,
		 * we check it for a complete match
		 *
		 * We must supply the correct maximum length of the text
		 * valid for the suffix tree. It equals the last valid
		 * position of the suffix after being prolonged.
		 */
		if ((slowscan_return = st_shti_edge_slowscan(parent, child,
					position, &last_match_position, text,
					ending_position - 1, stree)) == 0) {
			/*
			 * The whole edge matches, so we descend down along it.
			 *
			 * We must supply the correct maximum length
			 * of the text valid for the suffix tree.
			 * It equals the last valid position
			 * of the suffix after being prolonged.
			 */
			st_shti_edge_descend(&parent, &child, &prev_child,
					&position, ending_position - 1, stree);
			if (position == ending_position) {
				/*
				 * we reached the desired depth (and found out
				 * that the suffix is already prolonged)
				 *
				 * It is not possible for "position" to exceed
				 * the "ending_position", because we have
				 * supplied the correct maximum length
				 * of a suffix valid fot the suffix tree.
				 */
				return (-2); /* success */
			}
		} else if (slowscan_return == 2) {
			/*
			 * the edge is longer than necessary,
			 * but its initial letters match
			 *
			 * the current suffix is already present in
			 * the suffix tree and all the following suffixes
			 * need not to be prolonged
			 */
			return (-3);
			abort();
		} else { /* we need to split the edge and insert a new node */
			letter = text[position];
			if (st_shti_split_edge(&parent, letter, &child,
					&position, last_match_position,
				(unsigned_integral_type)(starting_position),
					text, stree) != 0) {
				fprintf(stderr, "Error: Could not split "
						"the parent->child edge!\n");
				return (2);
			}
			letter = text[position];
			if (st_shti_create_leaf(parent, letter,
				(-(signed_integral_type)(starting_position)),
				text, stree)) {
				fprintf(stderr, "Error: Could not create "
						"a new leaf of the parent!\n");
				return (3);
			}
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
		return (4);
	} else { /* (tmp == 2), which means that there was no matching edge */
		letter = text[starting_position + stree->tbranch[parent].depth];
		/* we need to create a new child of the parent */
		if (st_shti_create_leaf(parent, letter,
			(-(signed_integral_type)(starting_position)),
			text, stree)) {
			fprintf(stderr, "Error: Could not create "
					"a new leaf of the parent!\n");
			return (5);
		}
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
int st_shti_ukkonen_prolong_suffix (size_t starting_position,
		size_t ending_position,
		size_t *active_index,
		signed_integral_type *parent,
		signed_integral_type *sl_source,
		unsigned_integral_type *sl_targets_depth,
		const character_type *text,
		suffix_tree_shti *stree) {
	character_type letter = 0;
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
	/* if there is nothing to be done FIXME: Why there is no reall.tion? */
	if (starting_position == ending_position) {
		return (-1); /* we are (successfully) finished */
	}
	while ((tmp = st_shti_branch_once((*parent), &child,
					position, text, stree)) == 0) {
		/*
		 * while there is a candidate branching edge,
		 * we check it for a complete match
		 *
		 * We must supply the correct maximum length of the text
		 * valid for the suffix tree. It equals the last valid
		 * position of the suffix after being prolonged.
		 */
		if ((slowscan_return = st_shti_edge_slowscan((*parent),
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
			st_shti_edge_descend(parent, &child, &prev_child,
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
					stree->tbranch[(*parent)].depth;
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
			/* we store the current number of parent */
			grandpa = (*parent);
			letter = text[position];
			if (st_shti_split_edge(parent, letter, &child,
					&position, last_match_position,
				(unsigned_integral_type)(starting_position),
					text, stree) != 0) {
				fprintf(stderr, "Error: Could not split "
						"the parent->child edge!\n");
				return (1);
			}
			letter = text[position];
			if (st_shti_create_leaf((*parent), letter,
				(-(signed_integral_type)(starting_position)),
				text, stree)) {
				fprintf(stderr, "Error: Could not create "
						"a new leaf of the parent!\n");
				return (2);
			}
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
			if (st_shti_simulate_suffix_link_top_down(grandpa,
					parent, (*sl_targets_depth),
					starting_position, text,
					ending_position - 1, stree) == 0) {
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
		return (4);
	} else { /* (tmp == 2), which means that there was no matching edge */
		letter = text[starting_position +
			stree->tbranch[(*parent)].depth];
		/* we need to create a new child of the parent */
		if (st_shti_create_leaf((*parent), letter,
			(-(signed_integral_type)(starting_position)),
			text, stree)) {
			fprintf(stderr, "Error: Could not create "
					"a new leaf of the parent!\n");
			return (5);
		}
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
 * A function which prolongs suffixes of the intermediate suffix tree T^(n-1)
 * such that it could become the intermediate suffix tree T^n
 * using simple algorithm without utilizing the suffix links.
 *
 * @param
 * starting_position	the position in the text of the first character
 * 			of the first suffix to be prolonged
 * @param
 * ending_position	the position in the text just after the last character
 * 			of the suffixes after being prolonged
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
int st_shti_simple_prolong_suffixes (
		size_t *starting_position,
		size_t ending_position,
		const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
	/* the starting position of the current suffix to be prolonged */
	size_t i = (*starting_position);
	int tmp = 0; /* the return value of the "...prolong_suffix" function */
	while ((tmp = st_shti_simple_prolong_suffix(i, ending_position,
					text, length, stree)) == (-1)) {
		++i;
	}
	/* While there is a need to prolong more suffixes, we do it. */
	while (tmp == 0) {
		++i;
		tmp = st_shti_simple_prolong_suffix(i, ending_position, text,
					length, stree);
	}
	if (tmp > 0) { /* something went wrong */
		fprintf(stderr, "Error: Could not prolong suffixes "
				"to the ending position %zu!\n",
				ending_position);
		return (1);
	}
	(*starting_position) = i;
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
int st_shti_ukkonen_prolong_suffixes (size_t *starting_position,
		size_t ending_position,
		size_t *active_index,
		signed_integral_type *active_node,
		const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
	signed_integral_type sl_source = 0;
	unsigned_integral_type sl_targets_depth = 0;
	size_t maximum_possible_size = stree->branching_nodes +
		ending_position - (*starting_position) - 1;
	int tmp = 0;
	size_t tbranch_size = (size_t)(stree->tbranch_size);
	if ((tbranch_size < maximum_possible_size) &&
			(tbranch_size < length)) {
		if (st_shti_reallocate(maximum_possible_size, (size_t)(0),
					text, length, stree) > 0) {
			fprintf(stderr,	"Error: Could not "
					"reallocate memory!\n");
			return (1);
		}
	}
	while ((tmp = st_shti_ukkonen_prolong_suffix((*starting_position),
					ending_position, active_index,
					active_node, &sl_source,
					&sl_targets_depth, text,
					stree)) == (-1)) {
		++(*starting_position);
	}
	while (tmp == 0) {
		++(*starting_position);
		tmp = st_shti_ukkonen_prolong_suffix((*starting_position),
			ending_position, active_index, active_node, &sl_source,
			&sl_targets_depth, text, stree);
	}
	if (tmp > 0) { /* something went wrong */
		fprintf(stderr, "Error: Could not prolong suffixes "
				"to the ending position %zu!\n",
				ending_position);
		return (2);
	}
	return (0);
}

/* handling functions */

/**
 * A function which creates a suffix tree for the given text
 * of specified length using simple McCreight's algorithm
 * without taking advantage of the suffix links.
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
int st_shti_create_simple_mccreight (const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
	size_t i = 1;
	printf("Creating suffix tree using simple McCreight's "
		"style algorithm\n\n");
	if (st_shti_allocate(length, stree) > 0) {
		fprintf(stderr,	"Allocation error. Exiting.\n");
		return (1);
	}
	for (i = 1; i < length + 2; ++i) {
		if (st_shti_simple_insert_suffix(i, text, length, stree)
				> 0) {
			fprintf(stderr,	"Could not insert suffix number "
					"%zu. Exiting.\n", i);
			return (2);
		}
	}
	printf("\nThe suffix tree has been successfully created.\n");
	st_print_stats(length, stree->edges, stree->branching_nodes,
			(size_t)(0), stree->tedge_size,
			stree->tbranch_size, (size_t)(0), (size_t)(0),
			stree->er_size, stree->br_size, (size_t)(0),
			stree->hs->allocated_size, stree->hs->allocated_size);
	return (0);
}

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
int st_shti_create_mccreight (const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
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
	if (st_shti_allocate(length, stree) > 0) {
		fprintf(stderr,	"Allocation error. Exiting.\n");
		return (1);
	}
	for (i = 1; i < length + 2; ++i) {
		if (st_shti_mccreight_insert_suffix(&starting_node,
					&sl_source, &sl_targets_depth, i,
					text, length, stree) > 0) {
			fprintf(stderr,	"Could not insert suffix number "
					"%zu. Exiting.\n", i);
			return (2);
		}
	}
	printf("\nThe suffix tree has been successfully created.\n");
	st_print_stats(length, stree->edges, stree->branching_nodes,
			(size_t)(0), stree->tedge_size, stree->tbranch_size,
			(size_t)(0), (size_t)(0), stree->er_size,
			stree->br_size, (size_t)(0),
			stree->hs->allocated_size, stree->hs->allocated_size);
	return (0);
}

/**
 * A function which creates a suffix tree for the given text
 * of specified length using simple Ukkonen's algorithm
 * without taking advantage of the suffix links.
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
int st_shti_create_simple_ukkonen (const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
	size_t starting_position = 1;
	size_t i = 0;
	printf("Creating suffix tree using simple Ukkonen's style "
		"algorithm\n\n");
	if (st_shti_allocate(length, stree) > 0) {
		fprintf(stderr,	"Allocation error. Exiting.\n");
		return (1);
	}
	/*
	 * We are starting from 2, because it is the first position just after
	 * the first valid ending position.
	 */
	for (i = 2; i <= length + 2; ++i) {
		if (st_shti_simple_prolong_suffixes(&starting_position, i,
					text, length, stree) != 0) {
			fprintf(stderr,	"Could not create the intermediate "
					"tree number %zu. Exiting.\n", i - 1);
			return (2);
		}
	}
	printf("\nThe suffix tree has been successfully created.\n");
	st_print_stats(length, stree->edges, stree->branching_nodes,
			(size_t)(0), stree->tedge_size, stree->tbranch_size,
			(size_t)(0), (size_t)(0), stree->er_size,
			stree->br_size, (size_t)(0),
			stree->hs->allocated_size, stree->hs->allocated_size);
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
int st_shti_create_ukkonen (const character_type *text,
		size_t length,
		suffix_tree_shti *stree) {
	/* The very first active point is the root. */
	signed_integral_type active_node = 1;
	/* The starting point of the first suffix to be prolonged. */
	size_t active_index = 1;
	/* The starting position of the first suffix to be prolonged. */
	size_t starting_position = 1;
	size_t i = 1;
	printf("Creating suffix tree using Ukkonen's algorithm\n\n");
	if (st_shti_allocate(length, stree) > 0) {
		fprintf(stderr,	"Allocation error. Exiting.\n");
		return (1);
	}
	/*
	 * We are starting from 2, because it is the first position just after
	 * the first valid ending position.
	 */
	for (i = 2; i <= length + 2; ++i) {
		if (st_shti_ukkonen_prolong_suffixes(&starting_position, i,
					&active_index, &active_node, text,
					length, stree) > 0) {
			fprintf(stderr,	"Could not create the intermediate "
					"tree number %zu. Exiting.\n", i - 1);
			return (2);
		}
	}
	printf("\nThe suffix tree has been successfully created.\n");
	st_print_stats(length, stree->edges, stree->branching_nodes,
			(size_t)(0), stree->tedge_size, stree->tbranch_size,
			(size_t)(0), (size_t)(0), stree->er_size,
			stree->br_size, (size_t)(0),
			stree->hs->allocated_size, stree->hs->allocated_size);
	return (0);
}
