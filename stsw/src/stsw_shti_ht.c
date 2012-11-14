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
 * SHTI hashing-related functions implementation.
 * This file contains the implementation of the auxiliary
 * hashing-related functions, which are used by the functions,
 * which construct and maintain the suffix tree over a sliding window
 * using the implementation type SHTI with backward pointers.
 */
#include "stsw_shti_ht.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* auxiliary functions */

/**
 * A function which determines the depth
 * of the leaf with the provided number.
 *
 * @param
 * leaf		the leaf, which depth we would like to determine
 * @param
 * leafs_depth	the determined depth of the provided leaf
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If the depth of the provided leaf could be successfully
 * 		determined, this function returns zero.
 * 		Otherwise, in case of nay error,
 * 		a positive error number is returned.
 */
int stsw_shti_get_leafs_depth (signed_integral_type leaf,
		size_t *leafs_depth,
		const suffix_tree_sliding_window_shti *stsw) {
	size_t leafs_number = (size_t)(-leaf);
	/*
	 * FIXME: Check this function for possible errors
	 * and try to simplify it!
	 */
	/* if the currently used part of the table tleaf is continuous */
	if (stsw->tleaf_first <= stsw->tleaf_last) {
		if ((stsw->tleaf_first <= leafs_number) &&
				(leafs_number <= stsw->tleaf_last)) {
			(*leafs_depth) = stsw->tleaf_last - leafs_number + 1;
		} else { /* out of range */
			fprintf(stderr,	"stsw_shti_get_leafs_depth:\n"
					"Error: The provided leaf (%d) "
					"is out of range\nof the current "
					"table tleaf [%zu, %zu].\n",
					leaf, stsw->tleaf_first,
					stsw->tleaf_last);
			return (1);
		}
	/*
	 * otherwise, the currently used part of the table tleaf
	 * is made of two segments
	 */
	} else { /* stsw->tleaf_first > stsw->tleaf_last */
		if ((stsw->tleaf_first <= leafs_number) &&
				(leafs_number <= stsw->tleaf_size)) {
			(*leafs_depth) = stsw->tleaf_size - leafs_number +
				stsw->tleaf_last + 1;
		} else if (leafs_number <= stsw->tleaf_last) {
			(*leafs_depth) = stsw->tleaf_last - leafs_number + 1;
		} else { /* out of range */
			fprintf(stderr,	"stsw_shti_get_leafs_depth:\n"
					"Error: The provided leaf (%d) "
					"is out of range\nof the current "
					"table tleaf [%zu, %zu].\n",
					leaf, stsw->tleaf_first,
					stsw->tleaf_last);
			return (2);
		}
	}
	return (0);
}

/**
 * A function which determines the sliding window offset of the first character
 * of the suffix corresponding to the the leaf with the provided number.
 *
 * @param
 * leaf		the number of leaf, which corresponds to the suffix,
 * 		for which we would like to determine the offset
 * 		of its first character in the sliding window
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	this function returns the determined sliding window offset
 * 		of the first character of the suffix corresponding
 * 		to the provided leaf
 */
size_t stsw_shti_get_leafs_sw_offset (signed_integral_type leaf,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	size_t leafs_number = (size_t)(-leaf);
	size_t depth_order = 0;
	size_t leafs_sw_offset = 0;
	if (stsw->tleaf_first <= stsw->tleaf_last) {
		depth_order = leafs_number - stsw->tleaf_first;
	} else { /* stsw->tleaf_first > stsw->tleaf_last */
		if (stsw->tleaf_first <= leafs_number) {
			depth_order = leafs_number - stsw->tleaf_first;
		} else { /* stsw->tleaf_first > leafs_number */
			depth_order = stsw->tleaf_size - stsw->tleaf_first +
				leafs_number;
		}
	}
	leafs_sw_offset = tfsw->ap_window_begin + depth_order;
	if (leafs_sw_offset > tfsw->total_window_size) {
		leafs_sw_offset -= tfsw->total_window_size;
	}
	return (leafs_sw_offset);
}

/* hashing-related functions */

/**
 * A function which checks whether the provided hash key
 * (the source_node and the letter) matches the source_node
 * and the letter associated with the provided edge record.
 *
 * @param
 * source_node	the first part of the hash key
 * @param
 * letter	the second part of the hash key
 * @param
 * er		the edge record, which contains the source
 * 		and the target nodes of an edge to be examined
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If the provided hash key matches the key part
 * 		of the provided edge record, 1 is returned.
 * 		Otherwise, in case of any error, 0 is returned.
 */
int stsw_shti_er_key_matches (signed_integral_type source_node,
		character_type letter,
		edge_record er,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	/*
	 * the index of the first letter of an edge
	 * associated with the provided edge record
	 */
	size_t er_letter_index = 0;
	/*
	 * the first letter of an edge
	 * associated with the provided edge record
	 */
	character_type er_letter = 0;
	if (source_node != er.source_node) {
		/*
		 * we could not match the first part of the hash key,
		 * so we need not to check the second one
		 */
		return (0);
	}
	/*
	 * We could just call the stsw_shti_er_letter function here,
	 * but we would like this function call to be quick,
	 * so we rather copy the code to avoid another function call overhead.
	 */
	if (er.source_node <= 0) {
		fprintf(stderr, "stsw_shti_er_key_matches:\n"
				"Error: The provided edge_record\n"
				"contains a source node (%d), "
				"which is not a branching node.\n",
				er.source_node);
		return (0);
	}
	if (er.target_node == 0) {
		fprintf(stderr, "stsw_shti_er_key_matches:\n"
				"Error: The provided edge_record\n"
				"contains invalid target node (0).\n");
		return (0);
	} else if (er.target_node > 0) {
		er_letter_index = stsw->tbranch[er.target_node].
			head_position + stsw->tbranch[er.source_node].depth;
	} else { /* er->target_node < 0 */
		er_letter_index = stsw_shti_get_leafs_sw_offset(
				er.target_node, tfsw, stsw) +
			stsw->tbranch[er.source_node].depth;
	}
	if (er_letter_index > tfsw->total_window_size) {
		er_letter_index -= tfsw->total_window_size;
	}
	er_letter = tfsw->text_window[er_letter_index];
	if (letter == er_letter) {
		return (1);
	} else {
		return (0);
	}
}

/**
 * A function which extracts the first letter of the edge
 * represented by the provided edge record.
 *
 * @param
 * er		the edge record, which contains the source
 * 		and the target nodes of an edge to be examined
 * @param
 * letter	the extracted first letter of the edge
 * 		represented by the provided edge record
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully extract the first letter
 * 		of the provided edge record, zero is returned.
 * 		Otherwise, in case of any error,
 * 		a positive error number is returned.
 */
int stsw_shti_er_letter (edge_record er,
		character_type *letter,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	/*
	 * the index of the first letter of the edge
	 * associated with the provided edge record
	 */
	size_t er_letter_index = 0;
	if (er.source_node <= 0) {
		fprintf(stderr, "stsw_shti_er_letter:\n"
				"Error: The provided edge_record\n"
				"contains a source node (%d), "
				"which is not a branching node.\n",
				er.source_node);
		return (1);
	}
	if (er.target_node == 0) {
		fprintf(stderr, "stsw_shti_er_letter:\n"
				"Error: The provided edge_record\n"
				"contains invalid target node (0).\n");
		return (2);
	} else if (er.target_node > 0) {
		er_letter_index = stsw->tbranch[er.target_node].
			head_position + stsw->tbranch[er.source_node].depth;
	} else { /* er->target_node < 0 */
		er_letter_index = stsw_shti_get_leafs_sw_offset(
				er.target_node, tfsw, stsw) +
			stsw->tbranch[er.source_node].depth;
	}
	if (er_letter_index > tfsw->total_window_size) {
		er_letter_index -= tfsw->total_window_size;
	}
	(*letter) = tfsw->text_window[er_letter_index];
	return (0);
}

/**
 * A function which extracts the first letter of the provided edge.
 *
 * @param
 * source_node	the source node of the provided edge
 * @param
 * letter	the extracted first letter of the provided edge
 * @param
 * target_node	the target node of the provided edge
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully extract the first letter
 * 		of the provided edge, zero is returned.
 * 		Otherwise, in case of any error,
 * 		a positive error number is returned.
 */
int stsw_shti_edge_letter (signed_integral_type source_node,
		character_type *letter,
		signed_integral_type target_node,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	/* the index of the first letter of the provided edge */
	size_t edge_letter_index = 0;
	if (source_node <= 0) {
		fprintf(stderr, "stsw_shti_edge_letter:\n"
				"Error: The provided edge\n"
				"contains a source node (%d), "
				"which is not a branching node.\n",
				source_node);
		return (1);
	}
	if (target_node == 0) {
		fprintf(stderr, "stsw_shti_edge_letter:\n"
				"Error: The provided edge\n"
				"contains invalid target node (0).\n");
		return (2);
	} else if (target_node > 0) {
		edge_letter_index = stsw->tbranch[target_node].
			head_position + stsw->tbranch[source_node].depth;
	} else { /* target_node < 0 */
		edge_letter_index = stsw_shti_get_leafs_sw_offset(
				target_node, tfsw, stsw) +
			stsw->tbranch[source_node].depth;
	}
	if (edge_letter_index > tfsw->total_window_size) {
		edge_letter_index -= tfsw->total_window_size;
	}
	(*letter) = tfsw->text_window[edge_letter_index];
	return (0);
}

/**
 * A function which changes the size of the hash table
 * and rehashes all the values currently present in the old hash table
 * to the new hash table using the updated hash functions. Among others,
 * this function also updates the value of the stsw->tedge_size.
 *
 * @param
 * new_size	The desired new size of the hash table.
 * 		When this function successfully finishes, it will be set
 * 		to the actual new size of the hash table.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int stsw_shti_ht_rehash (size_t *new_size,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw) {
	static const size_t max_rehash_attempts = 1024;
	/* the first part of the current hash key */
	signed_integral_type source_node = 0;
	/* the second part of the current hash key */
	character_type letter = 0;
	/* the current value in the hash table */
	signed_integral_type target_node = 0;
	/* the original size of the hash table */
	size_t original_tedge_size = stsw->tedge_size;
	/* the original hash table data */
	edge_record *original_tedge = stsw->tedge;
	size_t original_new_size = (*new_size);
	size_t i = 0;
	size_t attempt_number = 0;
	int rehash_failed = 0;
	/*
	 * The memory pointed to by stsw->tedge is not lost
	 * by the following call(s) to free and calloc,
	 * because it has already been stored as the original_tedge.
	 *
	 * We set it to NULL at first in order to be able to easily allocate
	 * the new memory without worrying about unintentionally freeing
	 * the memory for the current hash table. In fact, we need
	 * what's currently stored in it to create the new hash table
	 * with the same content.
	 */
	stsw->tedge = NULL;
	fprintf(stderr, "The rehashing of the hash table will now start.\n");
	/* we will be trying to rehash the hash table until we succeed */
	do {
		if (attempt_number == max_rehash_attempts) {
			fprintf(stderr, "Error: The maximum number of "
					"attempts to rehash\nthe hash "
					"table (%zu) has been reached!\n"
					"The rehash operation has failed "
					"permanently!\n",
					max_rehash_attempts);
			return (1);
		}
		++attempt_number;
		fprintf(stderr, "Attempt number %zu:\n", attempt_number);
		rehash_failed = 0;
		(*new_size) = original_new_size;
		/*
		 * We update the current hash settings with respect to
		 * the current hash table size.
		 */
		if (hs_update(0, new_size, stsw->hs) != 0) {
			fprintf(stderr, "Error: Can not correctly update "
					"the hash table settings.\n");
			return (2);
		}
		/*
		 * We deallocate the memory used by the previous attemt
		 * to rehash the hash table.
		 *
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(stsw->tedge);
		stsw->tedge = NULL;
		/*
		 * And now we allocate the new, cleared memory
		 * for the hash table itself.
		 * Note that due to the random access to the hash table,
		 * it is essential that the memory is cleared
		 * before begin used. That's why we use calloc
		 * instead of realloc with stsw->tedge set to NULL.
		 * The call to realloc with the first argument
		 * set to NULL is equivalent to malloc,
		 * which does not clear the memory.
		 */
		stsw->tedge = calloc((*new_size), sizeof (edge_record));
		if (stsw->tedge == NULL) {
			perror("calloc(stsw->tedge)");
			/* resetting the errno */
			errno = 0;
			return (3);
		} else {
			/*
			 * Despite that the call to the calloc seems
			 * to have been successful, we reset the errno,
			 * because at least on Mac OS X
			 * it might have changed.
			 */
			errno = 0;
		}
		stsw->tedge_size = (*new_size);
		/*
		 * we reset the number of edges to zero,
		 * as every insertion increases their number
		 */
		stsw->edges = 0;
		for (i = 0; i < original_tedge_size; ++i) {
			/* if the original hash table record is not vacant */
			if (er_vacant(original_tedge[i]) == 0) {
				/*
				 * we have to rehash all the non-vacant
				 * edge records instead of all the non-empty
				 * edge records, because of the possible usage
				 * of the double hashing collision
				 * resolution technique
				 */
				source_node = original_tedge[i].source_node;
				target_node = original_tedge[i].target_node;
				if (stsw_shti_edge_letter(source_node,
							&letter, target_node,
							tfsw, stsw) > 0) {
					fprintf(stderr, "Error: Could not get "
							"the first letter\n"
							"of an edge P(%d)--"
							"\"?\"-->C(%d). "
							"Exiting!\n",
							source_node,
							target_node);
					return (4);
				}
				/*
				 * we insert the same hash table record
				 * to the new hash table at a new position
				 */
				if (stsw_shti_ht_insert(source_node,
							letter, target_node,
							0, tfsw, stsw) != 0) {
					fprintf(stderr, "Error: Insertion "
							"of the edge "
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
					"P(%d)--\"%lc...\"-->C(%d)"
#else
					"P(%d)--\"%c...\"-->C(%d)"
#endif
					" failed permanently!\n"
					"This is very unfortunate, "
					"as we can not continue\n"
					"to rehash the hash table. "
					"We will start over again!\n",
					source_node, letter, target_node);
					rehash_failed = 1;
					break;
				}
			}
		}
	} while (rehash_failed == 1);
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(original_tedge);
	original_tedge = NULL;
	fprintf(stderr, "Current hash table size:\n%zu cells of %zu "
			"bytes (totalling %zu bytes, ",
			stsw->tedge_size, stsw->er_size,
			stsw->tedge_size * stsw->er_size);
	print_human_readable_size(stderr,
			stsw->tedge_size * stsw->er_size);
	fprintf(stderr, ")\nThe rehashing of the hash table is complete.\n");
	return (0);
}

/**
 * A function which tries to perform the "cuckoo" part of the insertion
 * of a record into the hash table.
 *
 * @param
 * call_depth	the current number of nested calls of this function,
 * 		which preceeded this function call and by which we
 * 		determine whether to stop the recursion now
 * 		or not right now.
 * @param
 * original_source_node	the first part of the original hash key
 * @param
 * original_letter	the second part of the original hash key
 * @param
 * current_source_node	the first part of the current hash key
 * @param
 * current_letter	the second part of the current hash key
 * @param
 * current_target_node	the current value to be associated
 * 			with the current key in the hash table
 * @param
 * last_chf_index	the index of the last Cuckoo hash function used
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If this function finishes successfully, 0 is returned.
 * 		Otherwise, a positive error number is returned.
 */
int stsw_shti_cuckoo_ht_insert (size_t call_depth,
		signed_integral_type original_source_node,
		const character_type original_letter,
		signed_integral_type current_source_node,
		character_type current_letter,
		signed_integral_type current_target_node,
		size_t last_chf_index,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw) {
	static const size_t max_call_depth = 1024;
	/* the first part of the new hash key */
	signed_integral_type new_source_node = 0;
	/* the sedond part of the new hash key */
	character_type new_letter = 0;
	/* the value which is associated with the new hash key */
	signed_integral_type new_target_node = 0;
	/* the current number of the Cuckoo hash functions */
	size_t chf_number = stsw->hs->chf_number;
	/*
	 * iteration variable is initialized to the index
	 * of the previous Cuckoo hash function
	 */
	size_t i = (last_chf_index + (chf_number - 1)) % chf_number;
	/* the currently examined hash table position */
	size_t idx = 0;
	++call_depth;
	if (call_depth == max_call_depth) {
		/* the maximum call depth has been reached */
		return (1);
	}
	if ((current_source_node == original_source_node) &&
			(current_letter == original_letter)) {
		/* a loop has been encountered */
		return (2);
	}
	for (; i != last_chf_index; i = (i + (chf_number - 1)) % chf_number) {
		idx = cuckoo_hf(i, current_source_node, current_letter,
				stsw->hs);
		if (er_empty(stsw->tedge[idx]) == 0) {
			/*
			 * there should be no hash table record
			 * with a matching key
			 */
		} else { /* the current hash table record is empty */
			stsw->tedge[idx].source_node = current_source_node;
			stsw->tedge[idx].target_node = current_target_node;
			++(stsw->edges);
			return (0);
		}
	}
	/*
	 * If we got here, it means that all the remaining Cuckoo hashing
	 * functions provided an already occupied hash table record.
	 * This means that we have to make some space using
	 * "cuckoo-like kicking off the nest", or, making a hash table
	 * record empty by removing already present [key, value] pair
	 * and inserting it elsewhere.
	 */
	/* we select the previous Cuckoo hash function */
	i = (last_chf_index + (chf_number - 1)) % chf_number;
	idx = cuckoo_hf(i, current_source_node, current_letter, stsw->hs);
	new_source_node = stsw->tedge[idx].source_node;
	new_target_node = stsw->tedge[idx].target_node;
	if (stsw_shti_er_letter(stsw->tedge[idx],
				&new_letter, tfsw, stsw) > 0) {
		fprintf(stderr, "Error: Could not get the first letter\n"
				"of the edge record [%d, %d]. Exiting!\n",
				stsw->tedge[idx].source_node,
				stsw->tedge[idx].target_node);
		return (3);
	}
	stsw->tedge[idx].source_node = current_source_node;
	stsw->tedge[idx].target_node = current_target_node;
	if (stsw_shti_cuckoo_ht_insert(call_depth, original_source_node,
					original_letter, new_source_node,
					new_letter, new_target_node, i,
					tfsw, stsw) == 0) {
		return (0);
	/* if the recursive call failed permanently */
	} else {
		/*
		 * We need to restore the hash table
		 * to its original state.
		 */
		stsw->tedge[idx].source_node = new_source_node;
		stsw->tedge[idx].target_node = new_target_node;
		return (4); /* and return failure */
	}
}

/**
 * A function which tries to insert a new [key, value] pair
 * into the hash table. If the hash table already contains
 * the record with the matching key, its value part is overwritten.
 *
 * @param
 * source_node	the first part of the hash key
 * @param
 * letter	the second part of the hash key
 * @param
 * target_node	the value to be associated with
 * 		the provided key in the hash table
 * @param
 * rehash_allowed	If this variable is nonzero, the insert operation
 * 			will be allowed to trigger the rehash operation
 * 			of the hash table.
 * 			Otherwise, if an unresolvable hashing collision
 * 			occurrs, this function call will fail
 * 			with the return value of 1.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If this function finishes successfully, 0 is returned.
 * 		If an unresolvable hashing collision occurs while the rehash
 * 		operation is not allowed, one (1) is returned.
 * 		Otherwise, a positive error number greater than one is returned.
 */
int stsw_shti_ht_insert (signed_integral_type source_node,
		/*
		 * It is better to have the letter present as a parameter,
		 * because we then need not to manually retrieve it
		 * using the suffix tree.
		 */
		character_type letter,
		signed_integral_type target_node,
		int rehash_allowed,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw) {
	static const size_t max_insert_attempts = 1024;
	/*
	 * The index of the currently examined place for insertion
	 * or the iteration variable, based on the type
	 * of the currently used collision resolution technique.
	 */
	size_t i = 0;
	/*
	 * the possible future size of the table tedge (the first estimation,
	 * which will be further adjusted)
	 */
	size_t new_tedge_size = stsw->tedge_size +
		stsw->tesize_increase;
	/* the first value of the index i */
	size_t first_i = 0;
	/*
	 * The size of the incremental step used for shifting
	 * along the hash table while looking for an empty place
	 * for insertion.
	 */
	size_t inc = 0;
	/* the currently examined index to the hash table */
	size_t idx = 0;
	/*
	 * the index of the most recently examined
	 * vacant (or empty) record in the hash table
	 */
	size_t last_unused_idx = 0;
	size_t attempt_number = 0;
	int have_unused_idx = 0;
	int insert_failed = 0;
	signed_integral_type new_source_node = 0;
	character_type new_letter = 0;
	signed_integral_type new_target_node = 0;
	/* the current number of the Cuckoo hash functions */
	size_t chf_number = stsw->hs->chf_number;
	if (stsw->hs->crt_type == 1) { /* the Cuckoo hashing */
		/*
		 * We will be trying to insert the new entry
		 * into the hash table either until we succeed
		 * or until we would have to rehash the hash table while
		 * the rehash operation is not allowed in which case
		 * we return failure.
		 */
		do {
			if (attempt_number == max_insert_attempts) {
				fprintf(stderr, "Error: The maximum number "
						"of attempts to insert\n"
						"the new entry into "
						"the hash table (%zu) "
						"has been reached!\n"
						"The insert operation "
						"has failed permanently!\n",
						max_insert_attempts);
				return (1);
			}
			++attempt_number;
			insert_failed = 0;
			have_unused_idx = 0;
			/*
			 * At first, we try all the Cuckoo hash functions
			 * and see if there is any matching key
			 * already present in the hash table.
			 */
			for (i = 0; i < chf_number; ++i) {
				idx = cuckoo_hf(i, source_node, letter,
						stsw->hs);
				/*
				 * if the currently examined
				 * hash table record is occupied
				 */
				if (er_empty(stsw->tedge[idx]) == 0) {
					/*
					 * we check whether the source_node
					 * and the corresponding letter match
					 * this edge record
					 */
					if (stsw_shti_er_key_matches(
						source_node,
						letter, stsw->tedge[idx],
						tfsw, stsw) == 1) {
					/* rewriting the current value */
						stsw->tedge[idx].target_node =
							target_node;
						goto finish;
					}
				} else {
					if (have_unused_idx == 0) {
						have_unused_idx = 1;
						last_unused_idx = idx;
					}
				}
			}
			/*
			 * We have not been able to find any matching key
			 * already present in the hash table.
			 * Now we check whether we have encountered at least
			 * an empty position, which is valid
			 * for the provided key.
			 */
			if (have_unused_idx == 1) {
				idx = last_unused_idx;
				stsw->tedge[idx].source_node = source_node;
				stsw->tedge[idx].target_node = target_node;
				++(stsw->edges);
				break;
			}
			/*
			 * If we got here, it means that all the Cuckoo
			 * hashing functions provided an incompatible
			 * and already occupied hash table record.
			 * This means that we have to make
			 * some space using "cuckoo-like kicking off
			 * the nest", or, making a hash table record empty
			 * by removing already present [key, value] pair
			 * and inserting it elsewhere.
			 *
			 * idx is now the last index used, created by the last
			 * Cuckoo hash function used.
			 */
			new_source_node = stsw->tedge[idx].source_node;
			new_target_node = stsw->tedge[idx].target_node;
			if (stsw_shti_edge_letter(new_source_node,
						&new_letter, new_target_node,
						tfsw, stsw) > 0) {
				fprintf(stderr, "Error: Could not get the "
						"first letter\nof an edge "
						"P(%d)--\"?\"-->C(%d). "
						"Exiting!\n", new_source_node,
						new_target_node);
				return (2);
			}
			/*
			 * we "kick off" the edge record at the position idx
			 * and insert the original [key, value] pair there
			 */
			stsw->tedge[idx].source_node = source_node;
			stsw->tedge[idx].target_node = target_node;
			if (stsw_shti_cuckoo_ht_insert((size_t)(0),
						source_node, letter,
						new_source_node,
						new_letter, new_target_node,
						chf_number - 1, tfsw, stsw)
					== 0) {
				/*
				 * the "cuckoo" part of the Cuckoo hashing
				 * was successful
				 */
				break;
			}
			/*
			 * If we got here, we have not been able
			 * to successfully insert the provided entry
			 * into the hash table. We are left with the only
			 * option: rehash and try again.
			 *
			 * But first, we need
			 * to restore the hash table to its original state.
			 */
			stsw->tedge[idx].source_node = new_source_node;
			stsw->tedge[idx].target_node = new_target_node;
			insert_failed = 1;
			fprintf(stderr, "Warning: The \"cuckoo\" "
					"part of the Cuckoo collision "
					"resolution technique\n"
					"has failed!\nThe current "
					"number of records "
					"in the hash table: %zu\n",
					stsw->edges);
			fprintf(stderr, "The current hash table "
					"size: %zu\n",
					stsw->tedge_size);
			fprintf(stderr, "The hash table load "
					"factor: %f\n",
					(double)(stsw->edges) /
					(double)(stsw->tedge_size));
			if (rehash_allowed != 0) {
				/* if the rehash operation is successful */
				if (stsw_shti_ht_rehash(&new_tedge_size,
							tfsw, stsw) == 0) {
					/*
					 * we adjust the size
					 * increase step for the next resize
					 * of the hash table
					 */
					if (stsw->tesize_increase < 256) {
						/* minimum increase step */
						stsw->tesize_increase = 128;
					} else {
						/* division by 2 */
						stsw->tesize_increase =
						stsw->tesize_increase >> 1;
					}
					/* and are ready to continue */
					continue;
				} else {
					fprintf(stderr, "Error: The rehash "
							"operation of the "
							"hash table failed "
							"permanently!\n");
					return (3);
				}
			} else {
				/* rehash operation is not allowed */
				fprintf(stderr, "Error: The rehash operation "
						"is necessary, but "
						"not allowed!\n");
				return (1);
			}
		} while (insert_failed == 1);
		/*
		 * If we got here, it means that
		 * the insertion has been successful.
		 */
finish:		return (0);
	} else { /* the double hashing */
		i = primary_hf(source_node, letter, stsw->hs);
		first_i = i;
		inc = secondary_hf(source_node, letter, stsw->hs);
		/* the first query has to be done separately */
		if (er_empty(stsw->tedge[i]) == 0) {
			have_unused_idx = 0;
			if (er_vacant(stsw->tedge[i]) == 0) {
				if (stsw_shti_er_key_matches(source_node,
							letter,
							stsw->tedge[i],
							tfsw, stsw) == 1) {
					/* rewriting the current value */
					stsw->tedge[i].source_node =
						source_node;
					stsw->tedge[i].target_node =
						target_node;
					return (0);
				}
			} else {
				have_unused_idx = 1;
				last_unused_idx = i;
			}
			/* here, the parentheses are necessary */
			i = (i + inc) % stsw->tedge_size;
			/*
			 * while there is a non-empty edge record,
			 * which we have not visited yet
			 */
			while ((er_empty(stsw->tedge[i]) == 0) &&
					i != first_i) {
				if (er_vacant(stsw->tedge[i]) == 0) {
					if (stsw_shti_er_key_matches(
						source_node, letter,
						stsw->tedge[i],
						tfsw, stsw) == 1) {
						/*
						 * rewriting
						 * the current value
						 */
						stsw->tedge[i].target_node =
							target_node;
						return (0);
					}
				} else {
					if (have_unused_idx == 0) {
						have_unused_idx = 1;
						last_unused_idx = i;
					}
				}
				/* the parentheses are necessary as well */
				i = (i + inc) % stsw->tedge_size;
			}
			/*
			 * If we got here, it means that we have not found
			 * a matching edge record, but there might still be
			 * a vacant edge record present in the currently
			 * examined sequence of the hash table.
			 */
			if (have_unused_idx == 1) {
				stsw->tedge[last_unused_idx].source_node =
					source_node;
				stsw->tedge[last_unused_idx].target_node =
					target_node;
				++(stsw->edges);
				return (0);
			} else if (i == first_i) {
				/*
				 * We have again reached the initial index,
				 * which means that we have encountered a loop
				 * and will not able to find any suitable
				 * empty hash table record.
				 */
				fprintf(stderr, "Error: The insertion "
						"of the [key, value] pair "
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
						"P(%d)--\"%lc...\"-->C(%d)"
#else
						"P(%d)--\"%c...\"-->C(%d)"
#endif
						" failed permanently!\n"
						"The reason:\n"
						"No appropriate empty slot "
						"in the hash table has been "
						"found.\nThe hash table "
						"is too small!\n",
						source_node, letter,
						target_node);
				return (1);
			}
			/*
			 * If we got here, it means that the currently
			 * examined position in the hash table is empty.
			 * So, we fill it with the desired edge record.
			 */
			stsw->tedge[i].source_node = source_node;
			stsw->tedge[i].target_node = target_node;
			++(stsw->edges);
			return (0);
		}
		/*
		 * If we got here, it means that the first examined position
		 * in the hash table was empty. So, we fill it
		 * with the desired edge record.
		 */
		stsw->tedge[i].source_node = source_node;
		stsw->tedge[i].target_node = target_node;
		++(stsw->edges);
		return (0);
	}
}

/**
 * A function which tries to delete the hash table record
 * associated with the provided key from the hash table.
 *
 * @param
 * source_node	the first part of the hash key
 * @param
 * letter	the second part of the hash key
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If this function successfully deletes
 * 		an edge record from the hash table, 0 is returned.
 * 		If no matching edge record exists in the hash table,
 * 		a positive error number is returned.
 */
int stsw_shti_ht_delete (signed_integral_type source_node,
		character_type letter,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw) {
	/*
	 * The index of the currently examined Cuckoo hash function
	 * or the index of the currently examined place for insertion,
	 * based on the type of the currently used
	 * collision resolution technique.
	 */
	size_t i = 0;
	/* the first value of the index i */
	size_t first_i = 0;
	/*
	 * The size of the incremental step used for shifting
	 * along the hash table while looking for the certain key.
	 */
	size_t inc = 0;
	/* the currently examined index of the hash table */
	size_t idx = 0;
	/* the current number of the Cuckoo hash functions */
	size_t chf_number = stsw->hs->chf_number;
	if (stsw->hs->crt_type == 1) { /* the Cuckoo hashing */
		for (; i < chf_number; ++i) {
			idx = cuckoo_hf(i, source_node, letter, stsw->hs);
			/* if the current edge record is not empty */
			if (er_empty(stsw->tedge[idx]) == 0) {
				if (stsw_shti_er_key_matches(source_node,
					letter, stsw->tedge[idx],
					tfsw, stsw) == 1) {
					/* we have found the required key */
					stsw->tedge[idx].source_node = 0;
					stsw->tedge[idx].target_node = 0;
					--(stsw->edges);
					return (0);
				}
			}
		}
		fprintf(stderr, "Delete: Cuckoo: Not found!\n");
		return (1); /* not found */
	} else { /* the double hashing */
		i = primary_hf(source_node, letter, stsw->hs);
		first_i = i;
		inc = secondary_hf(source_node, letter, stsw->hs);
		/* the first query has to be done separately */
		if (er_empty(stsw->tedge[i]) == 0) {
			if (stsw_shti_er_key_matches(source_node, letter,
				stsw->tedge[i], tfsw, stsw) == 1) {
				/*
				 * We have found the desired key
				 * and its corresponding value.
				 * We can now delete it by making it vacant.
				 */
				stsw->tedge[i].source_node = 0;
				--(stsw->edges);
				return (0);
			}
			/* here, the parentheses are necessary */
			i = (i + inc) % stsw->tedge_size;
			while ((er_empty(stsw->tedge[i]) == 0) &&
					i != first_i) {
				if (stsw_shti_er_key_matches(source_node,
					letter, stsw->tedge[i],
					tfsw, stsw) == 1) {
					/*
					 * We have found the desired key
					 * and its corresponding value.
					 * We can now delete it
					 * by making it vacant.
					 */
					stsw->tedge[i].source_node = 0;
					--(stsw->edges);
					return (0);
				}
				/* the parentheses are necessary as well */
				i = (i + inc) % stsw->tedge_size;
			}
			if (i == first_i) {
				/*
				 * We have again reached the initial index,
				 * which means that we have encountered a loop
				 * and will not able to reach any more
				 * nonvisited hash table records.
				 */
				fprintf(stderr, "Delete: Double: "
						"Not found (loop)!\n");
				return (2); /* not found */
			}
		}
		fprintf(stderr, "Delete: Double: Not found!\n");
		return (3); /* not found */
	}
}

/**
 * A function which tries to lookup the value of the edge record
 * associated with the provided key in the hash table.
 *
 * @param
 * source_node	the first part of the hash key
 * @param
 * letter	the second part of the hash key
 * @param
 * target_node	the value part of the matching edge record, if found
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If the desired edge record has been found, zero is returned.
 * 		If the hash table does not contain an edge record
 * 		with such a key, a positive error number is returned.
 */
int stsw_shti_ht_lookup (signed_integral_type source_node,
		character_type letter,
		signed_integral_type *target_node,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	/*
	 * The index of the currently examined place for insertion
	 * or the iteration variable, based on the type
	 * of the currently used collision resolution technique.
	 */
	size_t i = 0;
	/* the first value of the index i */
	size_t first_i = 0;
	/*
	 * The size of the incremental step used for shifting
	 * along the hash table while looking for the certain key.
	 */
	size_t inc = 0;
	/* the currently examined index to the hash table */
	size_t idx = 0;
	/* the current number of the Cuckoo hash functions */
	size_t chf_number = stsw->hs->chf_number;
	if (stsw->hs->crt_type == 1) { /* the Cuckoo hashing */
		/* we try all the Cuckoo hash functions */
		for (; i < chf_number; ++i) {
			idx = cuckoo_hf(i, source_node, letter, stsw->hs);
			/* if the current edge record is not empty */
			if (er_empty(stsw->tedge[idx]) == 0) {
				if (stsw_shti_er_key_matches(source_node,
					letter, stsw->tedge[idx],
					tfsw, stsw) == 1) {
					/*
					 * we have found the desired edge
					 * record with a matching key
					 */
					(*target_node) =
						stsw->tedge[idx].target_node;
					return (0);
				}
			}
		}
		return (1); /* not found */
	} else { /* the double hashing */
		i = primary_hf(source_node, letter, stsw->hs);
		first_i = i;
		inc = secondary_hf(source_node, letter, stsw->hs);
		/* the first query has to be done separately */
		if (er_empty(stsw->tedge[i]) == 0) {
			if (stsw_shti_er_key_matches(source_node, letter,
				stsw->tedge[i], tfsw, stsw) == 1) {
				/*
				 * we have found the desired key
				 * and its corresponding value
				 */
				(*target_node) = stsw->tedge[i].target_node;
				return (0);
			}
			/* here, the parentheses are necessary */
			i = (i + inc) % stsw->tedge_size;
			while ((er_empty(stsw->tedge[i]) == 0) &&
					i != first_i) {
				if (stsw_shti_er_key_matches(source_node,
					letter, stsw->tedge[i],
					tfsw, stsw) == 1) {
					/*
					 * we have found the desired key
					 * and its corresponding value
					 */
					(*target_node) =
						stsw->tedge[i].target_node;
					return (0);
				}
				/* the parentheses are necessary as well */
				i = (i + inc) % stsw->tedge_size;
			}
			if (i == first_i) {
				/*
				 * We have again reached the initial index,
				 * which means that we have encountered a loop
				 * and will not able to reach any more
				 * nonvisited hash table records.
				 */
				return (2);
			}
		}
		return (3);
	}
}
