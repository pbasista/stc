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
 * SLAI common functions implementation.
 * This file contains the implementation of the auxiliary functions,
 * which are used by the functions, which construct
 * the suffix tree in the memory using the implementation type SLAI.
 */
#include "stree_slai_common.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* constants */

/**
 * The bit mask representing the leaf node in the linear array.
 * We use the most significant bit for this leaf node flag.
 */
const unsigned_integral_type leaf_node =
			(unsigned_integral_type)(1) <<
			(sizeof (unsigned_integral_type) * 8 - 1);

/**
 * The bit mask representing the rightmost child in the linear array.
 * We use the second most significant bit for this rightmost child flag.
 */
const unsigned_integral_type rightmost_child =
			(unsigned_integral_type)(1) <<
			(sizeof (unsigned_integral_type) * 8 - 2);

/* allocation functions */

/**
 * A function which allocates the memory for the suffix tree.
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
int st_slai_allocate (size_t length, suffix_tree_slai *stree) {
	/*
	 * the future size of the simple linear array
	 * (just the temporary value, which will later be used to
	 * estimate this initial size more precisely)
	 */
	size_t tnode_size = (size_t)(1) << (sizeof (size_t) * 8 - 1);
	size_t allocated_size = 0;
	printf("==================================================\n"
		"Trying to allocate the memory for the suffix tree:\n\n");
	/*
	 * The adjustment of the future size of the simple linear array.
	 *
	 * We would like it to be equal to the smallest power of two,
	 * which is at least as high as the length of the text,
	 * including the terminating character ($).
	 */
	while (length < tnode_size) {
		tnode_size = tnode_size >> 1; /* tnode_size / 2 */
	}
	tnode_size = tnode_size << 1; /* tnode_size * 2 */
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stree->tnode);
	stree->tnode = NULL;
	printf("Trying to allocate memory for tnode:\n"
		"%zu cells of %zu bytes (totalling %zu bytes, ",
			tnode_size, sizeof (unsigned_integral_type),
			tnode_size * sizeof (unsigned_integral_type));
	print_human_readable_size(stdout, tnode_size *
			sizeof (unsigned_integral_type));
	printf(").\n");
	stree->tnode = calloc(tnode_size, sizeof (unsigned_integral_type));
	if (stree->tnode == NULL) {
		perror("calloc(tnode)");
		/* resetting the errno */
		errno = 0;
		return (1);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += tnode_size * sizeof (unsigned_integral_type);
	printf("Successfully allocated!\n\n");
	printf("Total amount of memory initially allocated: %zu bytes (",
			allocated_size);
	print_human_readable_size(stdout, allocated_size);
	/* The meaning: per "real" input character */
	printf(")\nwhich is %.3f bytes per input character.\n\n",
			(double)(allocated_size) / (double)(length));
	/*
	 * There is one branching node, which will not occupy a single record
	 * of the simple linear array - the root. That's why
	 * we have to begin with one as the number of branching nodes.
	 */
	stree->branching_nodes = 1;
	/*
	 * This memory is cleared in advance, so we need not to do
	 * the following assignment. But we do, because we would like
	 * to emphasize the correctness of the initial zero value.
	 */
	stree->tnode_top = 0;
	stree->tnode_size = tnode_size;
	stree->tnode_size_increase = tnode_size >> 1; /* tnode_size / 2 */
	printf("The memory has been successfully allocated!\n"
		"===========================================\n\n");
	return (0);
}

/**
 * A function which reallocates the memory for the suffix tree
 * to make it larger.
 *
 * @param
 * desired_tnode_size	the minimum requested size of the table tnode
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int st_slai_reallocate (size_t desired_tnode_size,
		size_t length,
		suffix_tree_slai *stree) {
	void *tmp_pointer = NULL;
	size_t new_tnode_size = stree->tnode_size +
		stree->tnode_size_increase;
	/* if the implicitly chosen new size is too small */
	if (new_tnode_size < desired_tnode_size) {
		/* we make it large enough */
		new_tnode_size = desired_tnode_size;
		/*
		 * adjusting the next size increase step
		 * to desired_tnode_size / 2
		 */
		stree->tnode_size_increase = desired_tnode_size >> 1;
	}
	/*
	 * on the other hand, if the new size is too large, we make it smaller,
	 * but still large enough to hold all the data
	 * that the simple linear array can possibly hold
	 */
	if (new_tnode_size > (3 * length - 1)) {
		/*
		 * the maximum number of simple linear array records equals
		 * the number of leaf nodes (which is length + 1) plus
		 * two times the maximum number of branching nodes
		 * (which is length) minus two (there is no entry for the root)
		 */
		new_tnode_size = 3 * length - 1;
	}
	printf("Trying to reallocate the memory "
			"for the table tnode: new size:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
			new_tnode_size, sizeof (unsigned_integral_type),
			new_tnode_size * sizeof (unsigned_integral_type));
	print_human_readable_size(stdout, new_tnode_size *
			sizeof (unsigned_integral_type));
	printf(").\n");
	tmp_pointer = realloc(stree->tnode,
			new_tnode_size * sizeof (unsigned_integral_type));
	if (tmp_pointer == NULL) {
		perror("realloc(tnode)");
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
		stree->tnode = tmp_pointer;
	}
	/* we store the number of simple linear array records allocated */
	stree->tnode_size = new_tnode_size;
	/*
	 * finally, we adjust the size increase step
	 * for the next resize of the table tnode
	 */
	if (stree->tnode_size_increase < 256) {
		stree->tnode_size_increase = 128; /* minimum increase step */
	} else {
		/* division by 2 */
		stree->tnode_size_increase = stree->tnode_size_increase >> 1;
	}
	printf("Successfully reallocated!\n");
	return (0);
}

/* supporting functions */

/**
 * A function which tries to compute the longest common prefix
 * of all the children of a certain branching node (which is equal
 * to its depth) by examining all its respective children.
 *
 * The value of the branching node's depth is adjusted
 * with respect to the depth of its parent. This means that the determined
 * depth of the branching node is equal to the length of the edge label
 * of an edge leading into the branching node in question.
 *
 * @param
 * clean_parents_text_idx	the offset in the text of the beginning
 * 				of a label of an edge, which leads
 * 				to the branching node, for which
 * 				we would like to compute its depth
 * @param
 * first_node_offset	the offset in the table tnode of the first node
 * 			to be considered when computing the longest
 * 			common prefix of the incoming edge labels
 * @param
 * childrens_lcp_size	the value of this parameter will be overwritten
 * 			with the current value of the adjusted length
 * 			of the longest common prefix of the specified
 * 			range of nodes.
 * @param
 * stree	the actual suffix tree
 *
 * @return	This function always returns zero (0).
 */
int st_slai_compute_childrens_lcp (
		unsigned_integral_type clean_parents_text_idx,
		size_t first_node_offset,
		size_t *childrens_lcp_size,
		const suffix_tree_slai *stree) {
	size_t current_offset = first_node_offset;
	unsigned_integral_type current_text_idx =
		stree->tnode[current_offset];
	unsigned_integral_type clean_current_text_idx =
		(current_text_idx & ~rightmost_child & ~leaf_node);
	unsigned_integral_type min_index_difference =
		clean_current_text_idx - clean_parents_text_idx;
	/* while we have not already encountered the rightmost child */
	while ((current_text_idx & rightmost_child) == 0) {
		if ((current_text_idx & leaf_node) > 0) {
			++current_offset;
		} else { /* the previous child was a branching node */
			current_offset += 2;
		}
		current_text_idx = stree->tnode[current_offset];
		/*
		 * we clear the possible rightmost_child flag
		 * and possible leaf_node flag
		 */
		clean_current_text_idx =
			(current_text_idx & ~rightmost_child & ~leaf_node);
		if (clean_current_text_idx - clean_parents_text_idx <
				min_index_difference) {
			min_index_difference =
				clean_current_text_idx -
				clean_parents_text_idx;
		}
	}
	(*childrens_lcp_size) = min_index_difference;
	return (0);
}

/**
 * A function which prints a single entry of the simple linear array
 * (table tnode) to the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the table tnode
 * 		will be printed
 * @param
 * offset	The offset of an entry in the table tnode,
 * 		which will be printed.
 * 		If this entry corresponds to a branching node,
 * 		we expect that the provided offset points
 * 		to the first part of the branching node entry.
 * 		In this case, the offset will be incremented by one
 * 		upon the return from this function.
 * @param
 * stree	the actual suffix tree of which a single entry
 * 		in the table tnode will be printed
 *
 * @return	This function always returns zero (0).
 */
int st_slai_print_tnode_entry (FILE *stream,
		size_t *offset,
		const suffix_tree_slai *stree) {
	unsigned_integral_type text_idx = 0;
	/*
	 * we check whether the entry at the specified offset
	 * belongs to a leaf node
	 */
	if ((stree->tnode[(*offset)] & leaf_node) > 0) {
		/* we check whether it is the rightmost child of its parent */
		if ((stree->tnode[(*offset)] & rightmost_child) > 0) {
			text_idx = stree->tnode[(*offset)] ^
				leaf_node ^ rightmost_child;
			fprintf(stream, "(%zu)L[%u]R", (*offset), text_idx);
		} else {
			text_idx = stree->tnode[(*offset)] ^
				leaf_node;
			fprintf(stream, "(%zu)L[%u]", (*offset), text_idx);
		}
	} else { /* otherwise, it is a branching node */
		/* we check whether it is the rightmost child of its parent */
		if ((stree->tnode[(*offset)] & rightmost_child) > 0) {
			text_idx = stree->tnode[(*offset)] ^ rightmost_child;
			++(*offset);
			fprintf(stream, "(%zu,%zu)B[%u,%u]R", (*offset) - 1,
					(*offset), text_idx,
					stree->tnode[(*offset)]);
		} else {
			text_idx = stree->tnode[(*offset)];
			++(*offset);
			fprintf(stream, "(%zu,%zu)B[%u,%u]", (*offset) - 1,
					(*offset), text_idx,
					stree->tnode[(*offset)]);
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
 * starting_offset	the offset of a node in the simple linear array
 * 			(table tnode), from which the traversal starts
 * @param
 * parents_depth	the depth of a parent, from which
 * 			this traversal starts
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
int st_slai_traverse_from (FILE *stream,
		size_t starting_offset,
		unsigned_integral_type parents_depth,
		size_t log10bn,
		size_t log10l,
		const char *internal_text_encoding,
		const character_type *text,
		size_t length,
		const suffix_tree_slai *stree) {
	unsigned_integral_type current_text_idx = 0;
	unsigned_integral_type clean_current_text_idx = 0;
	unsigned_integral_type childs_depth = 0;
	signed_integral_type child = 0;
	size_t childs_offset = 0;
	size_t childrens_lcp_size = 0;
	size_t current_offset = starting_offset;
	/* we print all the children of the current parent node */
	do {
		current_text_idx = stree->tnode[current_offset];
		/* we clear the possible rightmost_child flag */
		clean_current_text_idx =
			(current_text_idx & ~rightmost_child);
		/* if the current node is a leaf node */
		if ((current_text_idx & leaf_node) > 0) {
			clean_current_text_idx ^= leaf_node;
			childs_depth = parents_depth +
				(unsigned_integral_type)(length + 2) -
				(unsigned_integral_type)
				(clean_current_text_idx);
			childs_offset = clean_current_text_idx -
				parents_depth;
			child = -(signed_integral_type)(childs_offset);
			if (st_print_edge(stream, 1,
					(signed_integral_type)(0),
					child, (signed_integral_type)(0),
					parents_depth, childs_depth,
					log10bn, log10l,
					childs_offset, internal_text_encoding,
					text) > 0) {
				fprintf(stderr, "Error: Could not "
						"print an edge!\n");
				return (1);
			}
			++current_offset;
		} else { /* otherwise it is a branching node */
			++current_offset;
			st_slai_compute_childrens_lcp(clean_current_text_idx,
				/*
				 * the first child
				 * of the current branching node
				 */
				(size_t)(stree->tnode[current_offset]),
				&childrens_lcp_size, stree);
			childs_depth = parents_depth +
				(unsigned_integral_type)(childrens_lcp_size);
			childs_offset = clean_current_text_idx -
				parents_depth;
			child = 0;
			if (st_print_edge(stream, 0,
					(signed_integral_type)(0),
					(signed_integral_type)(0),
					(signed_integral_type)(0),
					parents_depth, childs_depth,
					log10bn, log10l,
					childs_offset, internal_text_encoding,
					text) > 0) {
				fprintf(stderr, "Error: Could not "
						"print an edge!\n");
				return (2);
			}
			if (st_slai_traverse_from(stream,
			/* the first child of the current branching node */
				(size_t)(stree->tnode[current_offset]),
				childs_depth, log10bn, log10l,
				internal_text_encoding, text, length,
				stree) > 0) {
				fprintf(stderr, "Error: The traversal "
						"from the branching node\n"
						"was unsuccessful. "
						"Exiting!\n");
				return (3);
			}
			++current_offset;
		}
	} while ((current_text_idx & rightmost_child) == 0);
	return (0);
}

/**
 * A function which dumps the current content of the simple linear array
 * (table tnode) to the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the table tnode
 * 		will be dumped
 * @param
 * stree	the actual suffix tree, of which the table tnode
 * 		will be dumped
 *
 * @return	This function always returns zero (0).
 */
int st_slai_dump_tnode (FILE *stream, const suffix_tree_slai *stree) {
	size_t i = 0;
	fprintf(stream, "Table tnode dump BEGIN\n");
	st_slai_print_tnode_entry(stream, &i, stree);
	for (++i; i < stree->tnode_top; ++i) {
		fprintf(stream, ", ");
		st_slai_print_tnode_entry(stream, &i, stree);
	}
	fprintf(stream, "\nTable tnode dump END\n");
	return (0);
}

/* handling functions */

/**
 * A function which tries to evaluate the specified partition
 * during the main phase of the PWOTD algorithm.
 *
 * @param
 * partition_index	The index of a partition,
 * 			which is about to be evaluated.
 * @param
 * tnode_offset	The offset of an entry in the table tnode, which is reserved
 * 		for the "pointer" (an index to the table tnode) to the first
 * 		child of the yet unevaluated branching node, which represents
 * 		the closest common ancestor node of all the suffixes
 * 		in all the partitions in the provided range.
 * @param
 * parents_depth	The depth of a parent node of all the suffixes
 * 			in all the partitions in the provided range.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully evaluate the desired partition,
 * 		zero is returned.
 * 		Otherwise, a positive error number is returned.
 */
int st_slai_process_partition (size_t partition_index,
		size_t tnode_offset,
		size_t parents_depth,
		const character_type *text,
		size_t length,
		suffix_tree_slai *stree) {
	/*
	 * we output this message to the stderr,
	 * just to be able to easily overwrite the previous line
	 */
	fprintf(stderr, "Processing the partition with index of %-5zu\r",
	/*
	 * right padding the partition number with spaces,
	 * useful to overwrite possibly longer previous numbers
	 */
			partition_index);
	/* we make the current partition active */
	if (pwotd_select_partition(partition_index, &stree->cdata) > 0) {
		fprintf(stderr,	"Error: Could not select the "
				"partition to make it active! "
				"Exiting.\n");
		return (1);
	}
	/*
	 * we have to sort the suffixes in the current partition
	 * according to their now initial letter (at the offset
	 * of their lcp_size), because they have not been sorted
	 * on this position yet.
	 *
	 * We can safely skip sorting the suffixes in this partition
	 * at the offsets between prefix_length and lcp_size,
	 * because if lcp_size > prefix_length, it means that
	 * the first lcp_size initial characters are identical
	 * for all the suffixes in this partition!
	 */
	pwotd_sort_suffixes(stree->cdata.partitions[partition_index].lcp_size,
		(size_t)(0), stree->cdata.partitions[partition_index].
		end_offset - stree->cdata.partitions[partition_index].
		begin_offset, text, &stree->cdata);
	if (st_slai_output_nodes(stree->cdata.partitions[partition_index].
			lcp_size,
			parents_depth,
			(size_t)(0),
			stree->cdata.partitions[partition_index].end_offset -
			stree->cdata.partitions[partition_index].begin_offset,
			tnode_offset,
			text, length, stree) > 0) {
		fprintf(stderr,	"Error: Could not successfully output "
				"the nodes. Exiting.\n");
		return (2);
	}
	if (st_slai_empty_stack(text, length, stree) > 0) {
		fprintf(stderr,	"Error: Could not successfully empty "
				"the stack. Exiting.\n");
		return (3);
	}
	return (0);
}

/**
 * A function which tries to quickly examine and process the partitions
 * in the provided range of partitions and output some nodes directly
 * into the table tnode, while scheduling the partitions containing
 * more than one suffix for later evaluation during the main phase
 * of the PWOTD algorithm.
 *
 * @param
 * partitions_range_begin	The beginning of the provided
 * 				range of partitions.
 * @param
 * partitions_range_end	The end of the provided range of partitions.
 * @param
 * parents_depth	The depth of the parent node of all the suffixes
 * 			in all the partitions in the provided range.
 * @param
 * tnode_offset	The offset of an entry in the table tnode, which is reserved
 * 		for the "pointer" (an index to the table tnode) to the first
 * 		child of the yet unevaluated branching node, which represents
 * 		the closest common ancestor node of all the suffixes
 * 		in all the partitions in the provided range.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree
 *
 * @return	If we could successfully process all the partitions
 * 		in the specified range, zero is returned.
 * 		Otherwise, a positive error number is returned.
 */
int st_slai_process_partitions_range (size_t partitions_range_begin,
		size_t partitions_range_end,
		size_t parents_depth,
		size_t tnode_offset,
		const character_type *text,
		size_t length,
		suffix_tree_slai *stree) {
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	character_type old_letter = L'\0';
	character_type new_letter = L'\0';
#else
	character_type old_letter = '\0';
	character_type new_letter = '\0';
#endif
	size_t current_partition_index = 0;
	size_t lcp_size = 0;
	size_t text_offset = 0;
	size_t new_partitions_range_begin = partitions_range_begin;
	if ((stree->tnode_size - stree->tnode_top) <
			(partitions_range_end - partitions_range_begin) * 2) {
		if (st_slai_reallocate(stree->tnode_top +
					(partitions_range_end -
					partitions_range_begin) * 2,
					length, stree) > 0) {
			fprintf(stderr, "Error: Could not reallocate "
					"the memory for the table tnode. "
					"Exiting.\n");
			return (1);
		}
	}
	/* if the number of partitions in the provided range is exactly one */
	if (partitions_range_begin + 1 == partitions_range_end) {
		/* we check if there is only one suffix in this partition */
		if (stree->cdata.partitions[new_partitions_range_begin].
				begin_offset + 1 == stree->cdata.partitions[
				new_partitions_range_begin].end_offset) {
			/*
			 * in that case, we just output a single leaf node
			 * into the table tnode
			 */
			if (tnode_offset != 0) {
				stree->tnode[tnode_offset] =
					(unsigned_integral_type)
					(stree->tnode_top);
			}
			stree->tnode[stree->tnode_top] =
				(unsigned_integral_type)
				((stree->cdata.partitions[
				new_partitions_range_begin].
				text_offset + parents_depth) |
				leaf_node | rightmost_child);
			++stree->tnode_top;
		} else {
			/*
			 * otherwise, we schedule the current partition
			 * for the later complete evaluation
			 */
			pwotd_insert_partition_tbp(partitions_range_begin,
				tnode_offset, parents_depth,
				length, &stree->cdata);
		}
		return (0);
	} else {
		/* the partitions range of length greater than 1 */
		old_letter = text[stree->cdata.
			partitions[partitions_range_begin].
			text_offset + parents_depth];
		/* we will sequentially examine each partition */
		for (current_partition_index = partitions_range_begin + 1;
				current_partition_index <
				partitions_range_end;
				++current_partition_index) {
			new_letter =
				text[stree->cdata.partitions[
				current_partition_index].
				text_offset + parents_depth];
			/* if the initial letter has changed */
			if (old_letter != new_letter) {
				/*
				 * we have to at least partially evaluate
				 * the current sub-range of partitions
				 *
				 * if the size of this sub-range is just one
				 */
				if (new_partitions_range_begin + 1 ==
						current_partition_index) {
						if (
					/*
					 * the partitions sub-range
					 * of length 1
					 */
					new_partitions_range_begin ==
					partitions_range_begin) {
					if (tnode_offset != 0) {
					stree->tnode[tnode_offset] =
					(unsigned_integral_type)
					(stree->tnode_top);
							}
						}
					/*
					 * we check if there is only one
					 * suffix in this partition
					 */
					if (stree->cdata.partitions[
						new_partitions_range_begin].
						begin_offset + 1 ==
						stree->cdata.partitions[
						new_partitions_range_begin].
						end_offset) {
						stree->tnode[
							stree->tnode_top] =
						(unsigned_integral_type)
						((stree->cdata.partitions[
						new_partitions_range_begin].
						text_offset +
						parents_depth) |
						leaf_node);
						++stree->tnode_top;
					} else {
						stree->tnode[
							stree->tnode_top] =
						(unsigned_integral_type)
						(stree->cdata.partitions[
						new_partitions_range_begin].
						text_offset +
						parents_depth);
						++stree->tnode_top;
						pwotd_insert_partition_tbp(
						new_partitions_range_begin,
							stree->tnode_top,
						stree->cdata.partitions[
						new_partitions_range_begin].
						/*
						 * The depth of the closest
						 * common ancestor
						 * of all the suffixes
						 * in a single partition
						 * is equal to the length
						 * of their longest
						 * common prefix.
						 */
						lcp_size,
							length, &stree->cdata);
						++stree->tnode_top;
						++stree->branching_nodes;
					}
				} else {
					/*
					 * the partitions sub-range
					 * of length greater than 1
					 */
					if (new_partitions_range_begin ==
						partitions_range_begin) {
						if (tnode_offset != 0) {
						stree->tnode[tnode_offset] =
						(unsigned_integral_type)
						(stree->tnode_top);
						}
					}
			pwotd_determine_partitions_range_smallest_text_offset(
						&text_offset,
						new_partitions_range_begin,
						current_partition_index,
						&stree->cdata);
					stree->tnode[stree->tnode_top] =
						(unsigned_integral_type)
						(text_offset + parents_depth);
					++stree->tnode_top;
					lcp_size = 0;
					pwotd_determine_partitions_range_lcp(
						&lcp_size,
						new_partitions_range_begin,
						current_partition_index,
						text, length, &stree->cdata);
					pwotd_partitions_stack_push(
						new_partitions_range_begin,
						current_partition_index,
						lcp_size,
						stree->tnode_top,
						length, &stree->cdata);
					++stree->tnode_top;
					++stree->branching_nodes;
				}
				new_partitions_range_begin =
					current_partition_index;
				old_letter = new_letter;
			}
		}
		/* the final part of the partitions range */
		if (new_partitions_range_begin == partitions_range_begin) {
			if (tnode_offset != 0) {
				stree->tnode[tnode_offset] =
					(unsigned_integral_type)
					(stree->tnode_top);
			}
		}
		if (new_partitions_range_begin + 1 == partitions_range_end) {
			/*
			 * the final partitions sub-range
			 * of length 1
			 *
			 * we check if there is only one suffix
			 * in this partition
			 */
			if (stree->cdata.partitions[
					new_partitions_range_begin].
					begin_offset + 1 ==
					stree->cdata.partitions[
					new_partitions_range_begin].
					end_offset) {
				stree->tnode[stree->tnode_top] =
					(unsigned_integral_type)
					((stree->cdata.partitions[
					new_partitions_range_begin].
					text_offset + parents_depth) |
					leaf_node | rightmost_child);
				++stree->tnode_top;
			} else {
				stree->tnode[stree->tnode_top] =
					(unsigned_integral_type)
					((stree->cdata.partitions[
					new_partitions_range_begin].
					text_offset + parents_depth) |
					rightmost_child);
				++stree->tnode_top;
				pwotd_insert_partition_tbp(
						new_partitions_range_begin,
						stree->tnode_top,
						stree->cdata.partitions[
						new_partitions_range_begin].
						/*
						 * The depth of the closest
						 * common ancestor
						 * of all the suffixes
						 * in a single partition
						 * is equal to the length
						 * of their longest
						 * common prefix.
						 */
						lcp_size,
						length, &stree->cdata);
				++stree->tnode_top;
				++stree->branching_nodes;
			}
			return (0);
		} else {
			/*
			 * the final partitions sub-range
			 * of length greater than 1
			 */
			pwotd_determine_partitions_range_smallest_text_offset(
						&text_offset,
						new_partitions_range_begin,
						partitions_range_end,
						&stree->cdata);
			stree->tnode[stree->tnode_top] =
				(unsigned_integral_type)((text_offset +
							parents_depth) |
						rightmost_child);
			++stree->tnode_top;
			lcp_size = 0;
			pwotd_determine_partitions_range_lcp(
				&lcp_size,
				new_partitions_range_begin,
				partitions_range_end,
				text, length, &stree->cdata);
			pwotd_partitions_stack_push(
					new_partitions_range_begin,
					partitions_range_end,
					lcp_size,
					stree->tnode_top,
					length, &stree->cdata);
			++stree->tnode_top;
			++stree->branching_nodes;
		}
		return (0);
	}
}

/**
 * A function which outputs the nodes in the currently active part (range)
 * of the current partition of the table of suffixes into the linear array
 * and pushes the unevaluated branching nodes onto the stack.
 *
 * @param
 * lcp_size	The length of the longest common prefix shared by
 * 		all the suffixes in the provided range.
 * @param
 * parents_depth	The depth of the closest common ancestor
 * 			of all the suffixes in the provided range.
 * @param
 * range_begin	The beginning of the range in the current partition
 * 		of suffixes, from which we will output the nodes
 * 		into the linear array and onto the stack.
 * @param
 * range_end	The end of the range in the current partition
 * 		of suffixes, from which we will output the nodes
 * 		into the linear array and onto the stack.
 * @param
 * tnode_offset	The offset of an entry in the table tnode,
 * 		which is reserved for the "pointer" (an index
 * 		to the table tnode) to the first child of the
 * 		yet unevaluated branching node, which is the parent node
 * 		of all the suffixes in the provided range of suffixes.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree to be traversed
 *
 * @return	If we could successfully output all the nodes
 * 		in the desired range into the linear array, zero is returned.
 * 		Otherwise, a positive error number is returned.
 */
int st_slai_output_nodes (size_t lcp_size,
		size_t parents_depth,
		size_t range_begin,
		size_t range_end,
		size_t tnode_offset,
		const character_type *text,
		size_t length,
		suffix_tree_slai *stree) {
	size_t i = 0;
	/*
	 * The index in the text of the beginning of the current edge label
	 * leading to the node (either a branching node or a leaf node).
	 */
	size_t text_idx = stree->cdata.current_partition[range_begin] +
		lcp_size;
	/*
	 * The index in the text of the beginning of the edge label leading
	 * to the current branching node, which is part
	 * of the longest suffix available here.
	 */
	size_t min_text_idx = stree->cdata.current_partition[range_begin] +
		parents_depth;
	size_t node_begin = range_begin;
	character_type c = text[text_idx];
	/*
	 * At first, we check whether the current size of the table tnode
	 * is large enough to hold all the possible nodes
	 * which could be contained in the provided range
	 * of the table of suffixes.
	 */
	if ((stree->tnode_size - stree->tnode_top) <
			(range_end - range_begin)) {
		if (st_slai_reallocate(stree->tnode_top +
					range_end - range_begin,
					length, stree) > 0) {
			fprintf(stderr, "Error: Could not reallocate "
					"the memory for the table tnode. "
					"Exiting.\n");
			return (1);
		}
	}
	/*
	 * Next, we check whether the current size of the stack is large
	 * enough to hold all the possible entries which could arise
	 * in the provided range of the table of suffixes.
	 */
	if ((stree->cdata.stack_size - stree->cdata.stack_top) <
			(range_end - range_begin) / 2) {
		if (pwotd_cdata_stack_reallocate(
					/*
					 * we would like to be sure that
					 * all the possible stack entries
					 * that could rise in this function
					 * call do fit into the stack
					 */
					stree->cdata.stack_top +
					(range_end - range_begin) / 2,
					length, &stree->cdata) > 0) {
			fprintf(stderr, "Error: Could not reallocate the "
					"memory for the stack. Exiting.\n");
			return (2);
		}
	}
	/*
	 * Then we check if it is necessary to establish
	 * a link to the first node of this partition.
	 * (we suppose there will be at least one node in this partition)
	 */
	if (tnode_offset != 0) {
		/* and if it is necessary, we establish it */
		stree->tnode[tnode_offset] = (unsigned_integral_type)
			(stree->tnode_top);
	}
	/* we iteratively examine all the suffixes in the provided range */
	for (i = range_begin + 1; i < range_end; ++i) {
		text_idx = stree->cdata.current_partition[i] + lcp_size;
		/* if we have found a new node boundary */
		if (c != text[text_idx]) {
			/* if we have encountered a leaf node */
			if (node_begin + 1 == i) {
				/*
				 * we mark it as a leaf and simply output it
				 * to the linear array
				 */
				stree->tnode[stree->tnode_top] =
					(unsigned_integral_type)
					(min_text_idx ^ leaf_node);
				++stree->tnode_top;
			} else { /* we have encountered a branching node */
				/*
				 * we reserve the space for it
				 * in the linear array
				 */
				stree->tnode[stree->tnode_top] =
					(unsigned_integral_type)
					(min_text_idx);
				/*
				 * We need to advance by 2,
				 * because the branching node occupies
				 * two entries in the linear array.
				 * But the second increment is postponed
				 * until we write the current tnode_offset
				 * to the stack entry.
				 */
				++stree->tnode_top;
				/*
				 * and we push this branching node
				 * onto the stack
				 */
				stree->cdata.stack[stree->cdata.stack_top].
					range_begin = node_begin;
				stree->cdata.stack[stree->cdata.stack_top].
					range_end = i;
				stree->cdata.stack[stree->cdata.stack_top].
					lcp_size = lcp_size + 1;
				stree->cdata.stack[stree->cdata.stack_top].
					tnode_offset = stree->tnode_top;
				++stree->cdata.stack_top;
				/*
				 * and now we can safely perform
				 * the second increment
				 */
				++stree->tnode_top;
				/*
				 * finally, we increment
				 * the current number of branching nodes
				 */
				++stree->branching_nodes;
			}
			/*
			 * we reset the minimum index to the beginning
			 * of the edge label leading
			 * to the current branching node
			 */
			min_text_idx = stree->cdata.current_partition[i] +
				parents_depth;
			/*
			 * and we also reset the index
			 * to the current partition of the table of suffixes
			 * of the first suffix corresponding to the next node
			 */
			node_begin = i;
		}
		/* we remember the new character to be compared */
		c = text[text_idx];
		/*
		 * we need to keep the minimum text index of the starting offset
		 * of the label of an edge, which leads into the parent node
		 * of all the suffixes in the provided range
		 */
		if (min_text_idx > text_idx - lcp_size + parents_depth) {
			min_text_idx = text_idx - lcp_size + parents_depth;
		}
	}
	--i; /* the last processed value of i */
	/* if the last node is a leaf */
	if (node_begin == i) {
		/*
		 * we mark it as a leaf and simply output it
		 * to the linear array
		 */
		stree->tnode[stree->tnode_top] =
			(unsigned_integral_type)
			(min_text_idx ^ leaf_node);
		/* we mark this entry as the rightmost child of its parent */
		stree->tnode[stree->tnode_top] ^= rightmost_child;
		++stree->tnode_top;
	} else { /* the last node is a branching node */
		/*
		 * we reserve the space for it
		 * in the linear array
		 */
		stree->tnode[stree->tnode_top] =
			(unsigned_integral_type)
			(min_text_idx);
		/*
		 * We have to mark this entry in the linear array of nodes
		 * as the rightmost child of its parent.
		 */
		stree->tnode[stree->tnode_top] ^= rightmost_child;
		/*
		 * We need to advance by 2,
		 * because the branching node occupies
		 * two entries in the linear array.
		 * But the second increment is postponed
		 * until we write the current tnode_offset
		 * to the stack entry.
		 */
		++stree->tnode_top;
		/*
		 * and we push this branching node
		 * onto the stack
		 */
		stree->cdata.stack[stree->cdata.stack_top].
			range_begin = node_begin;
		stree->cdata.stack[stree->cdata.stack_top].
			range_end = i + 1;
		stree->cdata.stack[stree->cdata.stack_top].
			lcp_size = lcp_size + 1;
		stree->cdata.stack[stree->cdata.stack_top].
			tnode_offset = stree->tnode_top;
		++stree->cdata.stack_top;
		/*
		 * and now we can safely perform
		 * the second increment
		 */
		++stree->tnode_top;
		/*
		 * finally, we increment
		 * the current number of branching nodes
		 */
		++stree->branching_nodes;
	}
	return (0);
}

/**
 * A function which tries to empty the main stack by taking its entries
 * and evaluating them one by one.
 *
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree to be traversed
 *
 * @return	If we could successfully empty the whole main stack,
 * 		zero is returned.
 * 		Otherwise, a positive error number is returned.
 */
int st_slai_empty_stack (const character_type *text,
		size_t length,
		suffix_tree_slai *stree) {
	/*
	 * the beginning of the currently processed range
	 * in the current partition of the table of suffixes
	 */
	size_t range_begin = 0;
	/*
	 * the end of the currently processed range
	 * in the current partition of the table of suffixes
	 */
	size_t range_end = 0;
	/*
	 * The length of the longest common prefix of all suffixes
	 * in the currently processed range of the current partition
	 * of the table of suffixes.
	 */
	size_t lcp_size = 0;
	/*
	 * The offset of an entry in the table tnode, which is reserved
	 * for the "pointer" (an index to the table tnode) to the first child
	 * of the still unevaluated branching node, which is the parent node
	 * of all the suffixes in the currently processed range
	 * of the current partition of the table of suffixes.
	 */
	size_t tnode_offset = 0;
	while (stree->cdata.stack_top > 0) {
		/* popping the top entry of the stack */
		--stree->cdata.stack_top;
		range_begin = stree->cdata.stack[stree->cdata.stack_top].
			range_begin;
		range_end = stree->cdata.stack[stree->cdata.stack_top].
			range_end;
		lcp_size = stree->cdata.stack[stree->cdata.stack_top].
			lcp_size;
		tnode_offset = stree->cdata.stack[stree->cdata.stack_top].
			tnode_offset;
		/* adjusting the length of the longest common prefix */
		pwotd_determine_lcp(&lcp_size, range_begin, range_end,
				text, length, &stree->cdata);
		pwotd_sort_suffixes(lcp_size, range_begin, range_end,
				text, &stree->cdata);
		if (st_slai_output_nodes(lcp_size, lcp_size, range_begin,
				range_end, tnode_offset,
				text, length, stree) > 0) {
			fprintf(stderr,	"Error: Could not successfully "
					"output the nodes. Exiting.\n");
			return (1);
		}
	}
	return (0);
}

/** FIXME: Complete the description!
 * A function which tries to empty the partitions stack by taking its entries
 * and evaluating them one by one.
 *
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * stree	the actual suffix tree to be traversed
 *
 * @return	If we could successfully empty the whole partitions stack,
 * 		zero is returned.
 * 		Otherwise, a positive error number is returned.
 */
int st_slai_empty_partitions_stack (const character_type *text,
		size_t length,
		suffix_tree_slai *stree) {
	/* the beginning of the currently processed range of partitions */
	size_t range_begin = 0;
	/* the end of the currently processed range of partitions */
	size_t range_end = 0;
	/*
	 * The length of the longest common prefix of all the suffixes in all
	 * the partitions in the currently processed range of partitions.
	 */
	size_t lcp_size = 0;
	/*
	 * The offset of an entry in the table tnode, which is reserved
	 * for the "pointer" (an index to the table tnode) to the first child
	 * of the still unevaluated branching node, which is the parent node
	 * of all the suffixes in all the partitions in the currently
	 * processed range of partitions.
	 */
	size_t tnode_offset = 0;
	while (stree->cdata.partitions_stack_top > 0) {
		/* popping the top entry of the stack */
		--stree->cdata.partitions_stack_top;
		range_begin = stree->cdata.partitions_stack[
			stree->cdata.partitions_stack_top].range_begin;
		range_end = stree->cdata.partitions_stack[
			stree->cdata.partitions_stack_top].range_end;
		lcp_size = stree->cdata.partitions_stack[
			stree->cdata.partitions_stack_top].lcp_size;
		tnode_offset = stree->cdata.partitions_stack[
			stree->cdata.partitions_stack_top].tnode_offset;
		/*
		 * there is no need to update the lcp_size
		 * FIXME: Really? Explain why!
		 */
		if (st_slai_process_partitions_range(range_begin, range_end,
				lcp_size, tnode_offset,
				text, length, stree) > 0) {
			fprintf(stderr,	"Error: Could not successfully "
					"process the partitions range. "
					"Exiting.\n");
			return (1);
		}
	}
	return (0);
}

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
int st_slai_traverse (FILE *stream,
		const char *internal_text_encoding,
		int traversal_type,
		const character_type *text,
		size_t length,
		const suffix_tree_slai *stree) {
	/* the starting offset of the first child of the root */
	size_t starting_offset = 0;
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
	if (traversal_type == tt_detailed) {
		fprintf(stderr, "Warning: The detailed traversal type "
				"is not supported\nwhen using the SLAI "
				"implementation technique.\nFalling "
				"back to the simple traversal type!\n\n");
		traversal_type = tt_simple;
	} else if (traversal_type != tt_simple) {
		fprintf(stderr, "Error: Unknown traversal type (%d) "
				"Exiting!\n", traversal_type);
		return (1);
	}
	printf("Traversing the suffix tree\n\n");
	if (stream != stdout) {
		printf("The traversal log is being dumped "
				"to the specified file.\n");
	}
	fprintf(stream, "Simple suffix tree traversal BEGIN\n");
	if (st_slai_traverse_from(stream, starting_offset,
		(unsigned_integral_type)(0), log10bn, log10l,
		internal_text_encoding, text, length, stree) > 0) {
		fprintf(stderr, "Error: The traversal "
				"from the branching node\n"
				"was unsuccessful. "
				"Exiting!\n");
		return (2);
	}
	fprintf(stream, "Simple suffix tree traversal END\n");
	if (stream != stdout) {
		printf("Dump complete.\n");
	}
	printf("\nTraversing completed\n\n");
	return (0);
}

/**
 * A function which deallocates the memory used by the suffix tree.
 *
 * But, it will not deallocate the additional memory
 * used by the suffix tree construction data.
 * It needs to be deallocated separately!
 *
 * @param
 * stree	the actual suffix tree to be "deleted" (in more detail:
 * 		all the data it contains will be erased and all the
 * 		dynamically allocated memory it occupies will be deallocated,
 * 		but the suffix tree struct itself will remain intact,
 * 		because we can not be sure it was dynamically allocated
 * 		and that's why we can not deallocate it)
 *
 * @return	This function always returns zero.
 */
int st_slai_delete (suffix_tree_slai *stree) {
	printf("Deleting the suffix tree\n");
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stree->tnode);
	stree->tnode = NULL;
	stree->branching_nodes = 0;
	stree->tnode_top = 0;
	stree->tnode_size = 0;
	/*
	 * The other entries of the suffix_tree_slai struct
	 * need not to be reset to zero.
	 */
	printf("Successfully deleted!\n");
	return (0);
}
