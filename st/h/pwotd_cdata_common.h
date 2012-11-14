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
 * PWOTD common declarations.
 * This file contains the declarations of the auxiliary functions
 * related to the PWOTD algorithm, which are used by the functions,
 * which construct the suffix tree in the memory
 * using the implementation type SLAI.
 */
#ifndef	PWOTD_CONSTRUCTION_DATA_COMMON_HEADER
#define	PWOTD_CONSTRUCTION_DATA_COMMON_HEADER

#include "stree_common.h"

/* struct typedefs */

/**
 * A struct containing the position of the single partition
 * in the table of suffixes.
 */
typedef struct partition_record_pwotd_struct {
	/**
	 * the offset to the table of suffixes
	 * of the first entry of this partition
	 */
	size_t begin_offset;
	/**
	 * the offset to the table of suffixes
	 * of the entry just after the last entry of this partition
	 */
	size_t end_offset;
	/**
	 * the length of the longest common prefix
	 * of all the suffixes in this partition
	 */
	size_t lcp_size;
	/**
	 * the index to the text of the first character
	 * of the longest suffix, which is contained by this partition
	 */
	size_t text_offset;
} partition_record_pwotd;

/**
 * A struct containing the auxiliary information
 * necessary for a single partition
 * to be processed by a PWOTD algorithm.
 */
typedef struct partition_process_record_pwotd_struct {
	/**
	 * the index to the table of partitions
	 * of the partition to be processed
	 */
	size_t index;
	/**
	 * The offset of an entry in the table tnode,
	 * which is reserved for the "pointer"
	 * (an index to the table tnode) to the first child
	 * of the yet unevaluated branching node,
	 * which represents the closest common ancestor node
	 * of all the suffixes in the partition to be processed.
	 *
	 * If this value is zero, it means that the closest
	 * common ancestor node of all suffixes in the partition
	 * to be processed is the root and therefore we should not
	 * set any pointer to its first child. The root's first child
	 * is in fact always present at the beginning of the table tnode.
	 */
	size_t tnode_offset;
	/**
	 * The depth of the closest common ancestor node
	 * of all the suffixes in the partition to be processed.
	 */
	size_t parents_depth;
} partition_process_record_pwotd;

/**
 * The struct containing a single stack entry for the preliminary part
 * of the PWOTD algorithm, which is performed just after
 * the partitioning phase. In this part, the partitions are quickly examined
 * and scheduled for the later, complete evaluation. Also, during this part,
 * the first entries of the table tnode are filled in.
 */
typedef struct partitions_stack_record_pwotd_struct {
	/**
	 * The offset to the table of partitions
	 * marking the first partition in the range of partitions
	 * represented by this partitions stack record.
	 */
	size_t range_begin;
	/**
	 * The offset to the table of partitions
	 * marking the end of the range of partitions
	 * represented by this partitions stack record.
	 */
	size_t range_end;
	/**
	 * The length of the longest common prefix shared by
	 * all the suffixes in all the partitions
	 * represented by this partitions stack record.
	 */
	size_t lcp_size;
	/**
	 * The offset of an entry in the table tnode,
	 * which is reserved for the "pointer"
	 * (an index to the table tnode) to the first child
	 * of the yet unevaluated branching node,
	 * which is the closest common ancestor node
	 * of all the suffixes in the range of partitions
	 * represented by this partitions stack record.
	 */
	size_t tnode_offset;
} partitions_stack_record_pwotd;

/**
 * The struct containing a single stack entry for the main part
 * of the PWOTD algorithm. It holds all the information
 * necessary to evaluate a single branching node.
 */
typedef struct stack_record_pwotd_struct {
	/**
	 * The beginning of the range in the current partition
	 * of the table of suffixes containing all the suffixes
	 * which pass through this still unevaluated branching node.
	 */
	size_t range_begin;
	/**
	 * The end of the range in the current partition
	 * of the table of suffixes containing all the suffixes
	 * which pass through this still unevaluated branching node.
	 */
	size_t range_end;
	/**
	 * The length of the longest common prefix shared by
	 * all the suffixes, which pass through this
	 * still unevaluated branching node.
	 */
	size_t lcp_size;
	/**
	 * The offset of an entry in the table tnode,
	 * which is reserved for the "pointer"
	 * (an index to the table tnode) to the first child
	 * of this still unevaluated branching node.
	 */
	size_t tnode_offset;
} stack_record_pwotd;

/**
 * A struct containing all the auxiliary data structures,
 * which are necessary during the suffix tree construction
 * using the PWOTD algorithm and the implementation type SLAI.
 */
typedef struct pwotd_construction_data_struct {
	/** the size of the data type representing a single suffix */
	size_t s_size;
	/** partition record size */
	size_t pr_size;
	/**
	 * the size of a record containing the auxiliary information
	 * necessary for a single partition to be processed
	 */
	size_t ppr_size;
	/**
	 * the size of the partitions stack record used
	 * in the preliminary part of the PWOTD algorithm
	 */
	size_t psr_size;
	/**
	 * the size of the stack record used
	 * in the main part of the PWOTD algorithm.
	 */
	size_t sr_size;
	/**
	 * the table of suffixes (not the suffix array,
	 * at least not until the successful end of the algorithm)
	 */
	unsigned_integral_type *tsuffixes;
	/** the temporary table of suffixes (again, not the suffix array) */
	unsigned_integral_type *tsuffixes_tmp;
	/** all the partitions into which the table of suffixes is divided */
	partition_record_pwotd *partitions;
	/**
	 * the stack of the partitions,
	 * which need to be further processed (completely evaluated)
	 */
	partition_process_record_pwotd *partitions_tbp;
	/**
	 * the currently used partition of suffixes
	 * (not the suffix array, again)
	 */
	unsigned_integral_type *current_partition;
	/**
	 * The stack of the unevaluated ranges of partitions. It is used
	 * in the preliminary part of the PWOTD algorithm. The memory
	 * for this stack will be allocated on the fly, just before the use,
	 * and it will be deallocated as soon as it is no longer necessary.
	 */
	partitions_stack_record_pwotd *partitions_stack;
	/**
	 * The stack used in the main part of the PWOTD algorithm
	 * to store the information about the yet unevaluated branching nodes.
	 */
	stack_record_pwotd *stack;
	/** the current size of the table of suffixes */
	size_t tsuffixes_size;
	/** the current size of the temporary table of suffixes */
	size_t tsuffixes_tmp_size;
	/** the index of the currently active partition */
	size_t cp_index;
	/** the number of occupied entries in the partitions table */
	size_t partitions_number;
	/** the number of entries in the partitions table */
	size_t partitions_size;
	/**
	 * the amount by which the number of entries in the partitions table
	 * will be increased in case all of them are used
	 */
	size_t partitions_size_increase;
	/**
	 * the current number of entries in the stack of partitions,
	 * which need to be processed
	 */
	size_t partitions_tbp_number;
	/**
	 * the total size of the stack of partitions,
	 * which need to be processed
	 */
	size_t partitions_tbp_size;
	/**
	 * the amount by which the number of entries in the table
	 * of partitions to be processed will be increased
	 * in case all of them are used
	 */
	size_t partitions_tbp_size_increase;
	/**
	 * the current index of the first empty position
	 * in the partitions stack
	 */
	size_t partitions_stack_top;
	/** the current size of the partitions stack */
	size_t partitions_stack_size;
	/**
	 * the amount by which the size of the partitions stack
	 * will be increased in case all of its entries are used
	 */
	size_t partitions_stack_size_increase;
	/** the current index of the first empty position in the stack */
	size_t stack_top;
	/** the current size of the stack */
	size_t stack_size;
	/**
	 * the amount by which the size of the stack will be increased
	 * in case all of its entries are used
	 */
	size_t stack_size_increase;
	/**
	 * The length of a prefix considered
	 * when dividing the suffixes into the partitions.
	 */
	size_t prefix_length;
	/**
	 * the additional number of bytes currently allocated
	 * for this auxiliary data structures
	 * necessary for the suffix tree construction
	 */
	size_t total_memory_allocated;
	/**
	 * the maximum number of bytes allocated for this auxiliary
	 * data structures necessary for the suffix tree construction
	 * at any time since the beginning of the algorithm
	 */
	size_t maximum_memory_allocated;
} pwotd_construction_data;

/* allocation functions */

int pwotd_cdata_allocate (size_t length,
		pwotd_construction_data *cdata);
int pwotd_cdata_tsuffixes_tmp_reallocate (size_t desired_tsuffixes_tmp_size,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_cdata_partitions_reallocate (size_t desired_partitions_size,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_cdata_partitions_tbp_reallocate (size_t desired_partitions_tbp_size,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_cdata_partitions_stack_reallocate (
		size_t desired_partitions_stack_size,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_cdata_stack_reallocate (size_t desired_stack_size,
		size_t length,
		pwotd_construction_data *cdata);
int pwotd_cdata_deallocate (pwotd_construction_data *cdata);

/* supporting functions */

int pwotd_update_memory_usage_stats (size_t deallocated_size,
		size_t allocated_size,
		size_t length,
		pwotd_construction_data *cdata);

/* printing functions */

int pwotd_print_memory_usage_stats (FILE *stream,
		size_t length,
		const pwotd_construction_data *cdata);
int pwotd_print_tsuffixes_range (FILE *stream,
		const char *internal_text_encoding,
		size_t begin_offset,
		size_t end_offset,
		const character_type *text,
		size_t length,
		const pwotd_construction_data *cdata);

/* dumpimg functions */

int pwotd_dump_partitions (FILE *stream,
		const pwotd_construction_data *cdata);
int pwotd_dump_partitions_tbp (FILE *stream,
		const pwotd_construction_data *cdata);
int pwotd_dump_partitions_stack (FILE *stream,
		const pwotd_construction_data *cdata);
int pwotd_dump_stack (FILE *stream,
		const pwotd_construction_data *cdata);

#endif /* PWOTD_CONSTRUCTION_DATA_COMMON_HEADER */
