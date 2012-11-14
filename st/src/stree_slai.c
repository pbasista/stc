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
 * SLAI functions implementation.
 * This file contains the implementation of the functions,
 * which are used for the construction
 * of the suffix tree in the memory
 * using the implementation type SLAI.
 */
#include "stree_slai.h"

#include <stdio.h>

/* supporting functions */

/* handling functions */

/**
 * A function which creates a suffix tree for the given text
 * of specified length using
 * the Partition and Write Only Top Down (PWOTD) algorithm.
 *
 * @param
 * desired_prefix_length	The desired length of the prefix
 * 				used for dividing the suffixes
 * 				into the partitions. The special value of (-1)
 * 				means that the calling function does not give
 * 				any preference on the prefix length and that
 * 				we are free to determine it here in this
 * 				function based on the length of the text.
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
int st_slai_create_pwotd (long int desired_prefix_length,
		const character_type *text,
		size_t length,
		suffix_tree_slai *stree) {
	/*
	 * The prefix length is chosen based on the text length.
	 * If the text is not very long
	 * (meaning: no more than approximately one million (2^20) symbols),
	 * we assume that all the data structures for the text of this length
	 * fit in memory and we skip the partitioning part entirely.
	 * On the other hand, if the text is longer,
	 * we choose the prefix length for the partitioning by taking
	 * the ceiling of base 32 logarithm of the number of characters
	 * in the text divided by approximately one million (2^20).
	 * This will be checked for later.
	 */
	size_t prefix_length = 0;
	/*
	 * the variable used for storing the overall length
	 * of the text, including the terminating character ($)
	 */
	size_t text_length = length + 1;
	/*
	 * The temporary variable used for storing the overall length
	 * of the text, including the terminating character ($).
	 * The value of this variable will be invalidated
	 * during the computation of the prefix_length.
	 */
	size_t tmp_text_length = text_length;
	size_t extra_allocated_memory_size = 0;
	size_t extra_used_memory_size = 0;
	partition_process_record_pwotd *ppr = NULL;
	printf("Creating the suffix tree using the PWOTD algorithm\n\n");
	/* we have to count also the terminating character ($) */
	if (tmp_text_length > 1048576) { /* 2^20 */
		prefix_length = 1;
		/* tmp_text_length / (2 ^ 25) */
		tmp_text_length = tmp_text_length >> 25;
		while (tmp_text_length > 0) {
			++prefix_length;
			/* tmp_text_length / (2 ^ 5) */
			tmp_text_length = tmp_text_length >> 5;
		}
	}
	/* if there is a user / caller preference on the prefix length */
	if (desired_prefix_length >= 0) {
		printf("Abandoning the automatically determined "
				"prefix length: %zu\n", prefix_length);
		/* we have to meet it */
		prefix_length = (size_t)(desired_prefix_length);
	}
	printf("The selected prefix length: %zu\n\n",
			prefix_length);
	if (st_slai_allocate(length, stree) > 0) {
		fprintf(stderr,	"Suffix tree allocation error. Exiting.\n");
		return (1);
	}
	/*
	 * in the expression &(stree->cdata), we do not have to use
	 * the parentheses, because the member selection via pointer (->)
	 * has greater priority than the address operator (&)
	 */
	if (pwotd_cdata_allocate(length, &stree->cdata) > 0) {
		fprintf(stderr,	"Auxiliary data structures "
				"allocation error. Exiting.\n");
		return (2);
	}
	/* we have to initialize the table of suffixes at first */
	pwotd_initialize_suffixes(&stree->cdata);
	/*
	 * if the determined size of the prefix length is positive,
	 * we can proceed and start the partitioning phase
	 */
	if (prefix_length > 0) {
		if (pwotd_partition_suffixes(prefix_length, text,
					length, &stree->cdata) > 0) {
			fprintf(stderr,	"Error: Could not perform "
					"the partitioning phase! Exiting.\n");
			return (3);
		}
	} else { /* otherwise, we create just a single partition */
		/*
		 * but we need to allocate the memory
		 * for the temporary table of suffixes and
		 * for the partitions at first
		 */
		if (pwotd_cdata_tsuffixes_tmp_reallocate(text_length,
					length, &stree->cdata) > 0) {
			fprintf(stderr,	"Error: Could not allocate "
					"the memory for the temporary "
					"table of suffixes! Exiting.\n");
			return (4);
		}
		if (pwotd_cdata_partitions_reallocate((size_t)(1),
					length, &stree->cdata) > 0) {
			fprintf(stderr,	"Error: Could not allocate "
					"the memory for the table "
					"of partitions! Exiting.\n");
			return (5);
		}
		/*
		 * Despite the fact that the initial value
		 * of the prefix_length stored inside the construction data
		 * is zero, we update it anyway. We want to emphasize
		 * that its current value is correct.
		 */
		stree->cdata.prefix_length = 0;
		printf("Creating a single partition!\n");
		pwotd_insert_partition((size_t)(1), text_length + 1,
				(size_t)(0), length, &stree->cdata);
		printf("The partition has been created!\n");
	}
	/*
	 * We have to allocate the memory for the table of partitions
	 * to be processed. We can safely assume that the number of records
	 * for the partitions to be processed will be at most as high
	 * as the number of partitions itself.
	 */
	if (pwotd_cdata_partitions_tbp_reallocate(
				stree->cdata.partitions_number,
				length, &stree->cdata) > 0) {
		fprintf(stderr, "Error: st_slai_create_pwotd:\n"
				"Could not reallocate the memory\n"
				"for the table of partitions "
				"to be processed.\n");
		return (6);
	}
	/*
	 * FIXME: Try to come up with something like
	 * alphabet size * prefix_length
	 */
	if (pwotd_cdata_partitions_stack_reallocate(
				256 * stree->cdata.prefix_length,
				length, &stree->cdata) > 0) {
		fprintf(stderr, "Error: Could not reallocate "
				"the memory for the partitions "
				"stack. Exiting.\n");
		return (7);
	}
	st_slai_process_partitions_range((size_t)(0),
			stree->cdata.partitions_number,
			(size_t)(0), (size_t)(0),
			text, length, stree);
	if (st_slai_empty_partitions_stack(text, length, stree) > 0) {
		fprintf(stderr, "Error: Could not successfully "
				"empty the partitions stack "
				"Exiting.\n");
		return (8);
	}
	/*
	 * And now we should just take the partitions one by one
	 * and process them by evaluating all the unevaluated
	 * branching nodes inside them
	 */
	while (stree->cdata.partitions_tbp_number > 0) {
		--stree->cdata.partitions_tbp_number;
		ppr = stree->cdata.partitions_tbp +
			stree->cdata.partitions_tbp_number;
		st_slai_process_partition(ppr->index, ppr->tnode_offset,
				ppr->parents_depth, text, length, stree);
	}
	pwotd_print_memory_usage_stats(stdout, length, &stree->cdata);
	extra_allocated_memory_size = stree->cdata.maximum_memory_allocated;
	/*
	 * FIXME: Long shot: the amount of memory used is NOT equal
	 * to the total amount of memory currently allocated
	 */
	extra_used_memory_size = stree->cdata.total_memory_allocated;
	if (pwotd_cdata_deallocate(&stree->cdata) > 0) {
		fprintf(stderr,	"Deallocation error. Exiting.\n");
		return (9);
	}
	printf("\nThe suffix tree has been successfully created.\n");
	st_print_stats(length, (size_t)(0), stree->branching_nodes,
			stree->tnode_top - 1, (size_t)(0), (size_t)(0),
			stree->tnode_size, (size_t)(0), (size_t)(0),
			(size_t)(0), sizeof (unsigned_integral_type),
			extra_allocated_memory_size,
			extra_used_memory_size);
	return (0);
}
