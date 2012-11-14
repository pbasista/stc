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
 * PWOTD common functions implementation.
 * This file contains the implementation of the auxiliary functions
 * related to the PWOTD algorithm, which are used by the functions,
 * which construct the suffix tree in the memory
 * using the implementation type SLAI.
 */
#include "pwotd_cdata_common.h"

#include <errno.h>
#include <iconv.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* allocation functions */

/**
 * A function which allocates the memory for the auxiliary data structures
 * needed by the suffix tree construction.
 *
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	On successful allocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int pwotd_cdata_allocate (size_t length,
		pwotd_construction_data *cdata) {
	/*
	 * The future size of the table of suffixes. It is equal
	 * to the length of the underlying text in the suffix tree
	 * (number of the "real" characters in the text)
	 * increased by two. The first of the additional two entries
	 * is for the unused 0.th entry and the second
	 * is for an entry corresponding to the last nonempty suffix,
	 * consisting only of the terminating character ($).
	 */
	size_t tsuffixes_size = length + 2;
	/*
	 * The future size of the stack. It will be adjusted later.
	 */
	size_t stack_size = 1;
	/*
	 * The temporary variable used for storing the maximum possible number
	 * of branching nodes. It is equal to the overall length of the text,
	 * including the terminating character ($), decreased by one.
	 * The value of this variable will be invalidated
	 * during the computation of the stack_size.
	 */
	size_t max_branching_nodes = length; /* length + 1 - 1 */
	/* the total number of bytes allocated */
	size_t allocated_size = 0;
	/*
	 * we need to fill in the size of data type
	 * representing a single suffix
	 */
	cdata->s_size = sizeof (unsigned_integral_type);
	/* we need to fill in the size of the partition record */
	cdata->pr_size = sizeof (partition_record_pwotd);
	/*
	 * we need to fill in the size of the record
	 * for the partition to be processed
	 */
	cdata->ppr_size = sizeof (partition_process_record_pwotd);
	/*
	 * we need to fill in the size of the partitions stack record
	 * used in the preliminary part of the PWOTD algorithm
	 */
	cdata->psr_size = sizeof (partitions_stack_record_pwotd);
	/*
	 * we need to fill in the size of the stack record
	 * used in the main part of the PWOTD algorithm
	 */
	cdata->sr_size = sizeof (stack_record_pwotd);
	printf("Allocating the memory for the auxiliary data structures\n"
			"necessary for the suffix tree construction:\n\n");
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(cdata->tsuffixes);
	cdata->tsuffixes = NULL;
	printf("Trying to allocate memory for the table of suffixes:\n"
		"%zu cells of %zu bytes (totalling %zu bytes, ",
			tsuffixes_size, cdata->s_size,
			tsuffixes_size * cdata->s_size);
	print_human_readable_size(stdout,
			tsuffixes_size * cdata->s_size);
	printf(").\n");
	cdata->tsuffixes = calloc(tsuffixes_size, cdata->s_size);
	if (cdata->tsuffixes == NULL) {
		perror("calloc(cdata->tsuffixes)");
		/* resetting the errno */
		errno = 0;
		return (1);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += tsuffixes_size * cdata->s_size;
	cdata->tsuffixes_size = tsuffixes_size;
	printf("Successfully allocated!\n");
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(cdata->tsuffixes_tmp);
	/*
	 * We do not allocate the memory for the temporary table
	 * of suffixes here. Instead, we postpone the allocation
	 * until it is necessary either during the partitioning
	 * or just after the main part of the PWOTD algorithm starts
	 * (in case the partitioning phase is omitted).
	 */
	cdata->tsuffixes_tmp = NULL;
	/*
	 * The future size of the temporary table of suffixes
	 * will not be determined here, but its size will be based
	 * on the current purpose of this temporary table of suffixes.
	 */
	cdata->tsuffixes_tmp_size = 0;
	printf("Not allocating the memory "
			"for the temporary table of suffixes\n"
			"right now. The current size:\n"
			"0 cells of %zu bytes (totalling 0 bytes).\n",
			cdata->s_size);
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(cdata->partitions);
	/*
	 * Also, the memory for the partitions will not be allocated yet,
	 * but it will be allocated as soon as the partitioning phase begins.
	 */
	cdata->partitions = NULL;
	/*
	 * The index of the currently active partition is set to zero.
	 * This index is generally valid, but not right now, as the memory
	 * for the table of partitions has not been allocated yet.
	 * That's why it should be considered invalid to use this index
	 * prior to the allocation of the table of partitions!
	 */
	cdata->cp_index = 0;
	/*
	 * The number of entries in the table of partitions
	 * is initially zero.
	 */
	cdata->partitions_number = 0;
	/*
	 * The current size of the table of partitions is set to zero.
	 * It is about to be determined later, but not in this function.
	 */
	cdata->partitions_size = 0;
	/*
	 * The increase step for the size of the table of partitions
	 * will also be determined later, outside of this function,
	 * as soon as the length of the prefix is known
	 * and it is more clear what the final size
	 * of the table of partitions might be.
	 */
	cdata->partitions_size_increase = 0;
	printf("Not allocating the memory for the table of partitions\n"
			"right now. The current size:\n"
			"0 cells of %zu bytes (totalling 0 bytes).\n",
			cdata->pr_size);
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(cdata->partitions_tbp);
	/*
	 * Similarly, the memory for the entries corresponding to
	 * all the partitions, which need to be further processed,
	 * will be allocated only after the partitioning phase.
	 */
	cdata->partitions_tbp = NULL;
	/*
	 * The number of entries in the table of partitions,
	 * which need to be processed is initially zero.
	 */
	cdata->partitions_tbp_number = 0;
	/*
	 * The current size of the table of partitions,
	 * which need to be processed is set to zero.
	 * It is about to be determined later, but not in this function.
	 */
	cdata->partitions_tbp_size = 0;
	/*
	 * The increase step for the size of the table of entries
	 * of the partitions, which need to be processed
	 * will be determined later, outside of this function,
	 * as soon as it will become clear
	 * what the final number of partitions is.
	 */
	cdata->partitions_tbp_size_increase = 0;
	printf("Not allocating the memory for the stack of partitions,\n"
			"which need to be further processed right now. "
			"The current size:\n"
			"0 cells of %zu bytes (totalling 0 bytes).\n",
			cdata->ppr_size);
	/*
	 * We should not free this pointer, because it is
	 * just an auxiliary pointer to the table of suffixes.
	 */
	cdata->current_partition = NULL;
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(cdata->partitions_stack);
	/*
	 * This pointer is also set to NULL for now.
	 * The stack of the unevaluated ranges of partitions will be used
	 * only in the initial part of the PWOTD algorithm, just after
	 * the partitioning phase. It is better to allocate the memory
	 * for it then, because it will be clearer how deep
	 * this stack needs to be.
	 */
	cdata->partitions_stack = NULL;
	/*
	 * The index to the top entry of the partitions stack
	 * is currently set to zero.
	 * This index is generally valid, but not right now, as the memory
	 * for the partitions stack has not been allocated yet.
	 * That's why it should be considered invalid to use this index
	 * prior to the allocation of the partitions stack!
	 */
	cdata->partitions_stack_top = 0;
	/*
	 * as it was mentioned before, the number of entries
	 * in the partitions stack is initially zero
	 */
	cdata->partitions_stack_size = 0;
	/*
	 * The increase step for the size of the partitions stack
	 * is initially supposed to be equal to one half
	 * of the total partitions stack size.
	 * But for now, we can set it to zero.
	 */
	cdata->partitions_stack_size_increase = 0;
	/*
	 * The adjustment of the future size of the stack. We want it
	 * to be equal to the ceiling of base 2 logarithm of the maximum
	 * possible number of branching nodes increased by one.
	 */
	printf("Not allocating the memory for the stack\n"
			"of the unevaluated ranges of partitions right now. "
			"The current size:\n"
			"0 cells of %zu bytes (totalling 0 bytes).\n",
			cdata->psr_size);
	while (max_branching_nodes > 1) {
		++stack_size;
		/* max_branching_nodes / 2 */
		max_branching_nodes = max_branching_nodes >> 1;
	}
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(cdata->stack);
	cdata->stack = NULL;
	printf("Trying to allocate the memory for the stack:\n"
		"%zu cells of %zu bytes (totalling %zu bytes, ",
			stack_size, cdata->sr_size,
			stack_size * cdata->sr_size);
	print_human_readable_size(stdout, stack_size * cdata->sr_size);
	printf(").\n");
	cdata->stack = calloc(stack_size, cdata->sr_size);
	if (cdata->stack == NULL) {
		perror("calloc(cdata->stack)");
		/* resetting the errno */
		errno = 0;
		return (2);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += stack_size * cdata->sr_size;
	/*
	 * The index to the top entry of the stack is currently set to zero.
	 * This index is generally valid, but not right now, as the memory
	 * for the stack has not been allocated yet.
	 * That's why it should be considered invalid to use this index
	 * prior to the allocation of the stack!
	 */
	cdata->stack_top = 0;
	/*
	 * storing the previously computed number of stack entries
	 * in the suffix tree construction data
	 */
	cdata->stack_size = stack_size;
	/*
	 * The increase step for the size of the stack is set
	 * to one half of the total stack size.
	 */
	cdata->stack_size_increase = stack_size >> 1; /* stack_size / 2 */
	cdata->prefix_length = 0;
	cdata->total_memory_allocated = allocated_size;
	cdata->maximum_memory_allocated = allocated_size;
	printf("Successfully allocated!\n\n");
	printf("Total amount of memory currently allocated\n"
			"for the suffix tree construction data: %zu bytes (",
			cdata->total_memory_allocated);
	print_human_readable_size(stdout, cdata->total_memory_allocated);
	/* The meaning: per "real" input character */
	printf(")\nwhich is %.3f bytes per input character.\n\n",
			(double)(cdata->total_memory_allocated) /
			(double)(length));
	return (0);
}

/**
 * A function which reallocates the memory (either increases
 * or decreases its size) for the temporary table of suffixes.
 *
 * @param
 * desired_tsuffixes_tmp_size	The minimum requested size
 * 				of the temporary table of suffixes.
 * 				If this value is zero, we will
 * 				perform the deallocation.
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int pwotd_cdata_tsuffixes_tmp_reallocate (size_t desired_tsuffixes_tmp_size,
		size_t length,
		pwotd_construction_data *cdata) {
	void *tmp_pointer = NULL;
	/*
	 * the future size of the temporary table of suffixes
	 * (the lower bound, which might be later adjusted)
	 */
	size_t tsuffixes_tmp_size = 256;
	size_t allocated_size = 0;
	size_t deallocated_size = 0;
	if (desired_tsuffixes_tmp_size > 0) {
		/*
		 * if the implicitly chosen new size
		 * of the temporary table of suffixes is too small
		 */
		if (tsuffixes_tmp_size < desired_tsuffixes_tmp_size) {
			/* we make it large enough */
			tsuffixes_tmp_size = desired_tsuffixes_tmp_size;
		}
		/*
		 * on the other hand, if the new size of the temporary table
		 * of suffixes is too large, we make it smaller,
		 * but still large enough to hold all the data
		 * that the temporary table of suffixes can possibly hold
		 */
		if (tsuffixes_tmp_size > cdata->tsuffixes_size) {
			tsuffixes_tmp_size = cdata->tsuffixes_size;
		}
		printf("Trying to reallocate the memory\n"
				"for the temporary table of suffixes: "
				"new size:\n%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
				tsuffixes_tmp_size, cdata->s_size,
				tsuffixes_tmp_size * cdata->s_size);
		print_human_readable_size(stdout,
				tsuffixes_tmp_size * cdata->s_size);
		printf(").\n");
		tmp_pointer = realloc(cdata->tsuffixes_tmp,
				tsuffixes_tmp_size * cdata->s_size);
		if (tmp_pointer == NULL) {
			perror("realloc(cdata->tsuffixes_tmp)");
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
			cdata->tsuffixes_tmp = tmp_pointer;
		}
		deallocated_size += cdata->tsuffixes_tmp_size * cdata->s_size;
		allocated_size += tsuffixes_tmp_size * cdata->s_size;
		printf("Successfully reallocated!\n");
		/* we store the new size of the temporary table of suffixes */
		cdata->tsuffixes_tmp_size = tsuffixes_tmp_size;
	/* if the deallocation has been requested */
	} else if (desired_tsuffixes_tmp_size == 0) {
		printf("Trying to deallocate the memory\n"
				"for the temporary table of suffixes: "
				"new size:\n0 cells of %zu bytes "
				"(totalling 0 bytes).\n", cdata->s_size);
		free(cdata->tsuffixes_tmp);
		cdata->tsuffixes_tmp = NULL;
		deallocated_size += cdata->tsuffixes_tmp_size * cdata->s_size;
		printf("Successfully deallocated!\n");
		/* we store the new size of the temporary table of suffixes */
		cdata->tsuffixes_tmp_size = 0;
	}
	pwotd_update_memory_usage_stats(deallocated_size,
			allocated_size, length, cdata);
	return (0);
}

/**
 * A function which reallocates the memory (either increases
 * or decreases its size) for the table of partitions.
 *
 * @param
 * desired_partitions_size	The minimum requested size
 * 				of the table of partitions.
 * 				If this value is zero, we will
 * 				perform the deallocation.
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int pwotd_cdata_partitions_reallocate (size_t desired_partitions_size,
		size_t length,
		pwotd_construction_data *cdata) {
	void *tmp_pointer = NULL;
	/*
	 * the future size of the table of partitions
	 * (the first estimation, which will be further adjusted)
	 * Its size will be based on the character_type_size
	 * and on the desired length of the prefix.
	 */
	size_t partitions_size = cdata->partitions_size +
		cdata->partitions_size_increase;
	/*
	 * The maximum number of partitions equals the total number
	 * of all the suffixes of the text, including the shortest nonempty
	 * suffix, consisting only of the terminating character ($).
	 * This value is equal to the length + 1.
	 */
	size_t maximum_partitions_size = length + 1;
	size_t allocated_size = 0;
	size_t deallocated_size = 0;
	if (desired_partitions_size > 0) {
		/*
		 * if the implicitly chosen new size
		 * of the table of partitions is too small
		 */
		if (partitions_size < desired_partitions_size) {
			/* we make it large enough */
			partitions_size = desired_partitions_size;
			/* and we also adjust the next size increase step */
			cdata->partitions_size_increase =
				/* division by 2 */
				desired_partitions_size >> 1;
		}
		/*
		 * on the other hand, if the new size of the table
		 * of partitions is too large, we make it smaller,
		 * but still large enough to hold all the data
		 * that the table of partitions can possibly hold
		 */
		if (partitions_size > maximum_partitions_size) {
			partitions_size = maximum_partitions_size;
		}
		printf("Trying to reallocate the memory\n"
				"for the table of partitions: "
				"new size:\n%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
				partitions_size, cdata->pr_size,
				partitions_size * cdata->pr_size);
		print_human_readable_size(stdout,
				partitions_size * cdata->pr_size);
		printf(").\n");
		tmp_pointer = realloc(cdata->partitions,
				partitions_size * cdata->pr_size);
		if (tmp_pointer == NULL) {
			perror("realloc(cdata->partitions)");
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
			cdata->partitions = tmp_pointer;
		}
		deallocated_size += cdata->partitions_size * cdata->pr_size;
		allocated_size += partitions_size * cdata->pr_size;
		printf("Successfully reallocated!\n");
		/* we store the new size of the partitions table */
		cdata->partitions_size = partitions_size;
		/*
		 * finally, we adjust the size increase step
		 * for the next resize of the table of partitions
		 */
		if (cdata->partitions_size_increase < 256) {
			/* minimum size increase step */
			cdata->partitions_size_increase = 128;
		} else {
			/* division by 2 */
			cdata->partitions_size_increase =
				cdata->partitions_size_increase >> 1;
		}
	/* if the deallocation has been requested */
	} else if (desired_partitions_size == 0) {
		printf("Trying to deallocate the memory\n"
				"for the table of partitions: "
				"new size:\n0 cells of %zu bytes "
				"(totalling 0 bytes).\n", cdata->pr_size);
		free(cdata->partitions);
		cdata->partitions = NULL;
		deallocated_size += cdata->partitions_size * cdata->pr_size;
		printf("Successfully deallocated!\n");
		/* we store the new size of the partitions table */
		cdata->partitions_size = 0;
	}
	pwotd_update_memory_usage_stats(deallocated_size,
			allocated_size, length, cdata);
	return (0);
}

/**
 * A function which reallocates the memory (either increases
 * or decreases its size) for the table of entries
 * of the partitions to be processed.
 *
 * @param
 * desired_partitions_tbp_size	The minimum requested size
 * 				of the table of entries of the partitions
 * 				to be processed.
 * 				If this value is zero, we will
 * 				perform the deallocation.
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int pwotd_cdata_partitions_tbp_reallocate (size_t desired_partitions_tbp_size,
		size_t length,
		pwotd_construction_data *cdata) {
	void *tmp_pointer = NULL;
	/*
	 * the future size of the table of entries of the partitions
	 * to be processed (the first estimation,
	 * which will be further adjusted)
	 */
	size_t partitions_tbp_size = cdata->partitions_tbp_size +
		cdata->partitions_tbp_size_increase;
	/*
	 * The maximum number of partitions to be processed equals
	 * the total number of all the suffixes of the text,
	 * including the shortest nonempty suffix,
	 * consisting only of the terminating character ($).
	 * This value is equal to the length + 1.
	 */
	size_t maximum_partitions_tbp_size = length + 1;
	size_t allocated_size = 0;
	size_t deallocated_size = 0;
	if (desired_partitions_tbp_size > 0) {
		/*
		 * if the implicitly chosen new size
		 * of the table of entries of the partitions
		 * to be processed is too small
		 */
		if (partitions_tbp_size < desired_partitions_tbp_size) {
			/* we make it large enough */
			partitions_tbp_size = desired_partitions_tbp_size;
			/* and we also adjust the next size increase step */
			cdata->partitions_tbp_size_increase =
				/* division by 2 */
				desired_partitions_tbp_size >> 1;
		}
		/*
		 * on the other hand, if the new size of the table
		 * of entries of the partitions to be processed is too large,
		 * we make it smaller, but still large enough to hold
		 * all the data that the table of entries
		 * of the partitions to be processed can possibly hold
		 */
		if (partitions_tbp_size > maximum_partitions_tbp_size) {
			partitions_tbp_size = maximum_partitions_tbp_size;
		}
		printf("Trying to reallocate the memory "
				"for the table of entries\n"
				"of the partitions to be processed: "
				"new size:\n%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
				partitions_tbp_size, cdata->ppr_size,
				partitions_tbp_size * cdata->ppr_size);
		print_human_readable_size(stdout,
				partitions_tbp_size * cdata->ppr_size);
		printf(").\n");
		tmp_pointer = realloc(cdata->partitions_tbp,
				partitions_tbp_size * cdata->ppr_size);
		if (tmp_pointer == NULL) {
			perror("realloc(cdata->partitions_tbp)");
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
			cdata->partitions_tbp = tmp_pointer;
		}
		deallocated_size +=
			cdata->partitions_tbp_size * cdata->ppr_size;
		allocated_size += partitions_tbp_size * cdata->ppr_size;
		printf("Successfully reallocated!\n");
		/*
		 * we store the new size of the table of entries
		 * of the partitions to be processed
		 */
		cdata->partitions_tbp_size = partitions_tbp_size;
		/*
		 * finally, we adjust the size increase step
		 * for the next resize of the table of entries
		 * of the partitions to be processed
		 */
		if (cdata->partitions_tbp_size_increase < 256) {
			/* minimum size increase step */
			cdata->partitions_tbp_size_increase = 128;
		} else {
			/* division by 2 */
			cdata->partitions_tbp_size_increase =
				cdata->partitions_tbp_size_increase >> 1;
		}
	/* if the deallocation has been requested */
	} else if (desired_partitions_tbp_size == 0) {
		printf("Trying to deallocate the memory "
				"for the table of entries\n"
				"of the partitions to be processed: "
				"new size:\n0 cells of %zu bytes "
				"(totalling 0 bytes).\n", cdata->ppr_size);
		free(cdata->partitions_tbp);
		cdata->partitions_tbp = NULL;
		deallocated_size +=
			cdata->partitions_tbp_size * cdata->ppr_size;
		printf("Successfully deallocated!\n");
		/*
		 * we store the new size of the table of entries
		 * for the partitions to be processed
		 */
		cdata->partitions_tbp_size = 0;
	}
	pwotd_update_memory_usage_stats(deallocated_size,
			allocated_size, length, cdata);
	return (0);
}

/**
 * A function which reallocates the memory (either increases
 * or decreases its size) for the stack of the unevaluated
 * ranges of partitions.
 *
 * @param
 * desired_partitions_stack_size	the minimum requested size
 * 					of the stack of the unevaluated
 * 					ranges of partitions
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int pwotd_cdata_partitions_stack_reallocate (
		size_t desired_partitions_stack_size,
		size_t length,
		pwotd_construction_data *cdata) {
	void *tmp_pointer = NULL;
	/*
	 * the future size of the stack of the unevaluated ranges
	 * of partitions (the first estimation,
	 * which will be further adjusted)
	 */
	size_t partitions_stack_size = cdata->partitions_stack_size +
		cdata->partitions_stack_size_increase;
	/*
	 * The maximum size of the stack of the unevaluated ranges
	 * of partitions equals the total number
	 * of all the characters in the text, including
	 * the terminating character ($), decreased by 2.
	 * This value is equal to the length - 1.
	 *
	 * FIXME: Explain why!
	 */
	size_t maximum_partitions_stack_size = length - 1;
	size_t allocated_size = 0;
	size_t deallocated_size = 0;
	if (desired_partitions_stack_size > 0) {
		/*
		 * if the implicitly chosen new size of the stack
		 * of the unevaluated ranges of partitions is too small
		 */
		if (partitions_stack_size < desired_partitions_stack_size) {
			/* we make it large enough */
			partitions_stack_size = desired_partitions_stack_size;
			/* and we also adjust the next size increase step */
			cdata->partitions_stack_size_increase =
				/* division by 2 */
				desired_partitions_stack_size >> 1;
		}
		/*
		 * on the other hand, if the new size of the stack
		 * of the unevaluated ranges of partitions is too large,
		 * we make it smaller, but still large enough
		 * to hold all the data that this stack can possibly hold
		 */
		if (partitions_stack_size > maximum_partitions_stack_size) {
			partitions_stack_size = maximum_partitions_stack_size;
		}
		printf("Trying to reallocate the memory for the stack\n"
				"of the unevaluated ranges of partitions: "
				"new size:\n%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
				partitions_stack_size, cdata->psr_size,
				partitions_stack_size * cdata->psr_size);
		print_human_readable_size(stdout,
				partitions_stack_size * cdata->psr_size);
		printf(").\n");
		tmp_pointer = realloc(cdata->partitions_stack,
				partitions_stack_size * cdata->psr_size);
		if (tmp_pointer == NULL) {
			perror("realloc(cdata->partitions_stack)");
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
			cdata->partitions_stack = tmp_pointer;
		}
		deallocated_size += cdata->partitions_stack_size *
			cdata->psr_size;
		allocated_size += partitions_stack_size * cdata->psr_size;
		printf("Successfully reallocated!\n");
		/*
		 * we store the new size of the stack
		 * of the unevaluated ranges of partitions
		 */
		cdata->partitions_stack_size = partitions_stack_size;
		/*
		 * finally, we adjust the size increase step
		 * for the next resize of the stack
		 * of the unevaluated ranges of partitions
		 */
		if (cdata->partitions_stack_size_increase < 256) {
			/* minimum increase step */
			cdata->partitions_stack_size_increase = 128;
		} else {
			/* division by 2 */
			cdata->partitions_stack_size_increase =
				cdata->partitions_stack_size_increase >> 1;
		}
	/* if the deallocation has been requested */
	} else if (desired_partitions_stack_size == 0) {
		printf("Trying to deallocate the memory for the stack\n"
				"of the unevaluated ranges of partitions: "
				"new size:\n0 cells of %zu bytes "
				"(totalling 0 bytes).\n", cdata->psr_size);
		free(cdata->partitions_stack);
		cdata->partitions_stack = NULL;
		deallocated_size += cdata->partitions_stack_size *
			cdata->psr_size;
		printf("Successfully deallocated!\n");
		/*
		 * we store the new size of the stack
		 * of the unevaluated ranges of partitions
		 */
		cdata->partitions_stack_size = 0;
	}
	pwotd_update_memory_usage_stats(deallocated_size,
			allocated_size, length, cdata);
	return (0);
}

/**
 * A function which reallocates the memory (either increases
 * or decreases its size) for the stack.
 *
 * @param
 * desired_stack_size	the minimum requested size of the stack
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int pwotd_cdata_stack_reallocate (size_t desired_stack_size,
		size_t length,
		pwotd_construction_data *cdata) {
	void *tmp_pointer = NULL;
	/*
	 * the future size of the stack
	 * (the first estimation, which will be further adjusted)
	 */
	size_t stack_size = cdata->stack_size +
		cdata->stack_size_increase;
	/*
	 * The maximum size of the stack equals the total number
	 * of all the characters in the text, including
	 * the terminating character ($), decreased by 2.
	 * This value is equal to the length - 1.
	 *
	 * FIXME: Explain why!
	 */
	size_t maximum_stack_size = length - 1;
	size_t allocated_size = 0;
	size_t deallocated_size = 0;
	if (desired_stack_size > 0) {
		/*
		 * if the implicitly chosen new size
		 * of the stack is too small
		 */
		if (stack_size < desired_stack_size) {
			/* we make it large enough */
			stack_size = desired_stack_size;
			/* and we also adjust the next size increase step */
			cdata->stack_size_increase =
				desired_stack_size >> 1; /* division by 2 */
		}
		/*
		 * on the other hand, if the new size of the stack
		 * is too large, we make it smaller, but still
		 * large enough to hold all the data
		 * that the stack can possibly hold
		 */
		if (stack_size > maximum_stack_size) {
			stack_size = maximum_stack_size;
		}
		printf("Trying to reallocate the memory "
				"for the stack: new size:\n"
				"%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
				stack_size, cdata->sr_size,
				stack_size * cdata->sr_size);
		print_human_readable_size(stdout,
				stack_size * cdata->sr_size);
		printf(").\n");
		tmp_pointer = realloc(cdata->stack,
				stack_size * cdata->sr_size);
		if (tmp_pointer == NULL) {
			perror("realloc(cdata->stack)");
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
			cdata->stack = tmp_pointer;
		}
		deallocated_size += cdata->stack_size * cdata->sr_size;
		allocated_size += stack_size * cdata->sr_size;
		printf("Successfully reallocated!\n");
		/* we store the new size of the stack */
		cdata->stack_size = stack_size;
		/*
		 * finally, we adjust the size increase step
		 * for the next resize of the stack
		 */
		if (cdata->stack_size_increase < 256) {
			/* minimum increase step */
			cdata->stack_size_increase = 128;
		} else {
			/* division by 2 */
			cdata->stack_size_increase =
				cdata->stack_size_increase >> 1;
		}
	/* if the deallocation has been requested */
	} else if (desired_stack_size == 0) {
		printf("Trying to deallocate the memory "
				"for the stack: new size:\n"
				"0 cells of %zu bytes "
				"(totalling 0 bytes).\n", cdata->sr_size);
		free(cdata->stack);
		cdata->stack = NULL;
		deallocated_size += cdata->stack_size * cdata->sr_size;
		printf("Successfully deallocated!\n");
		/* we store the new size of the stack */
		cdata->stack_size = 0;
	}
	pwotd_update_memory_usage_stats(deallocated_size,
			allocated_size, length, cdata);
	return (0);
}

/**
 * A function which deallocates the memory for the auxiliary data structures
 * needed by the suffix tree construction.
 *
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	On successful deallocation, this function returns 0.
 * 		If there is nothing to deallocate, (-1) is returned.
 * 		Otherwise, if an error occurs, a positive error number
 * 		greater than one is returned.
 */
int pwotd_cdata_deallocate (pwotd_construction_data *cdata) {
	size_t deallocated_size = 0;
	if ((cdata->tsuffixes == NULL) &&
			(cdata->tsuffixes_tmp == NULL) &&
			(cdata->partitions == NULL) &&
			(cdata->partitions_tbp == NULL) &&
			(cdata->partitions_stack == NULL) &&
			(cdata->stack == NULL)) {
		return (-1); /* nothing to deallocate */ }
	printf("Deallocating the suffix tree construction data.\n");
	free(cdata->tsuffixes);
	cdata->tsuffixes = NULL;
	deallocated_size += cdata->tsuffixes_size * cdata->s_size;
	free(cdata->tsuffixes_tmp);
	cdata->tsuffixes_tmp = NULL;
	deallocated_size += cdata->tsuffixes_tmp_size * cdata->s_size;
	free(cdata->partitions);
	cdata->partitions = NULL;
	deallocated_size += cdata->partitions_size * cdata->pr_size;
	free(cdata->partitions_tbp);
	cdata->partitions_tbp = NULL;
	deallocated_size += cdata->partitions_tbp_size * cdata->ppr_size;
	/* resetting the pointer to the current partition to NULL */
	cdata->current_partition = NULL;
	free(cdata->partitions_stack);
	cdata->partitions_stack = NULL;
	deallocated_size += cdata->partitions_stack_size * cdata->psr_size;
	free(cdata->stack);
	cdata->stack = NULL;
	deallocated_size += cdata->stack_size * cdata->sr_size;
	/*
	 * maintaining the suffix tree construction data
	 * constistent with its definition
	 */
	cdata->tsuffixes_size = 0;
	cdata->tsuffixes_tmp_size = 0;
	cdata->cp_index = 0;
	cdata->partitions_number = 0;
	cdata->partitions_size = 0;
	cdata->partitions_size_increase = 0;
	cdata->partitions_tbp_number = 0;
	cdata->partitions_tbp_size = 0;
	cdata->partitions_tbp_size_increase = 0;
	cdata->partitions_stack_top = 0;
	cdata->partitions_stack_size = 0;
	cdata->partitions_stack_size_increase = 0;
	cdata->stack_top = 0;
	cdata->stack_size = 0;
	cdata->stack_size_increase = 0;
	/*
	 * It does not make sense to keep the length
	 * of the partitioning prefix, as all the partitions
	 * and all the suffixes have been deallocated.
	 */
	cdata->prefix_length = 0;
	cdata->total_memory_allocated -= deallocated_size;
	if (cdata->total_memory_allocated > 0) {
		fprintf(stderr, "Error: The total size of the memory "
				"allocated\nafter all the possible "
				"deallocations have been done\n"
				"remains positive: (%zu bytes, ",
				cdata->total_memory_allocated);
		print_human_readable_size(stderr,
				cdata->total_memory_allocated);
		fprintf(stderr, ")\n");
		return (1);
	}
	cdata->maximum_memory_allocated = 0;
	printf("Successfully deallocated!\n");
	return (0);
}

/* supporting functions */

/**
 * A function, which updates the total amount of bytes
 * currently allocated for the suffix tree construction data
 * and the maximum number of bytes ever allocated
 * for the suffix tree construction data.
 *
 * @param
 * deallocated_size	the number of deallocated bytes to consider
 * 			when updating the memory usage statistics
 * 			for the suffix tree construction data
 * @param
 * allocated_size	the number of allocated bytes to consider
 * 			when updating the memory usage statistics
 * 			for the suffix tree construction data
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_update_memory_usage_stats (size_t deallocated_size,
		size_t allocated_size,
		size_t length,
		pwotd_construction_data *cdata) {
	cdata->total_memory_allocated -= deallocated_size;
	cdata->total_memory_allocated += allocated_size;
	/* if the total size of memory currently allocated has changed */
	if (deallocated_size != allocated_size) {
		printf("Total amount of memory currently allocated\n"
			"for the suffix tree construction data: %zu bytes (",
			cdata->total_memory_allocated);
		print_human_readable_size(stdout,
				cdata->total_memory_allocated);
		/* The meaning: per "real" input character */
		printf(")\nwhich is %.3f bytes per input character.\n\n",
				(double)(cdata->total_memory_allocated) /
				(double)(length));
	}
	/* updating the maximum memory size ever allocated, if necessary */
	if (cdata->total_memory_allocated > cdata->maximum_memory_allocated) {
		cdata->maximum_memory_allocated =
			cdata->total_memory_allocated;
	}
	return (0);
}

/* printing functions */

/**
 * A function which prints the current memory usage statistics
 * of the suffix tree construction data into the provided
 * FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the current partition
 * 		of suffixes will be printed
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_print_memory_usage_stats (FILE *stream,
		size_t length,
		const pwotd_construction_data *cdata) {
	fprintf(stream, "\nSuffix tree construction data statistics:\n"
			"-----------------------------------------\n");
	if (cdata->total_memory_allocated > 0) {
		fprintf(stream, "The total current memory usage: "
				"%zu bytes (",
				cdata->total_memory_allocated);
		print_human_readable_size(stream,
				cdata->total_memory_allocated);
		/* The meaning: per "real" input character */
		fprintf(stream, ")\nwhich is %.3f bytes "
				"per input character.\n",
				(double)(cdata->total_memory_allocated) /
				(double)(length));
	}
	if (cdata->maximum_memory_allocated > 0) {
		fprintf(stream, "The total maximum memory usage: "
				"%zu bytes (",
				cdata->maximum_memory_allocated);
		print_human_readable_size(stream,
				cdata->maximum_memory_allocated);
		/* The meaning: per "real" input character */
		fprintf(stream, ")\nwhich is %.3f bytes "
				"per input character.\n",
				(double)(cdata->maximum_memory_allocated) /
				(double)(length));
	}
	fprintf(stream, "\n");
	return (0);
}

/**
 * A function which prints the current partition of suffixes
 * into the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the current partition
 * 		of suffixes will be printed
 * @param
 * internal_text_encoding	The character encoding used in the internal
 * 				representation of the text for the suffixes.
 * @param
 * begin_offset	the offset of the first suffix in the current partition
 * 		of the table of suffixes, which needs to be printed
 * @param
 * end_offset	the offset of the suffix immediately following the last suffix
 * 		in the current partition of the table of suffixes,
 * 		which needs to be printed
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	If this function call is successful, this function returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int pwotd_print_tsuffixes_range (FILE *stream,
		const char *internal_text_encoding,
		size_t begin_offset,
		size_t end_offset,
		const character_type *text,
		size_t length,
		const pwotd_construction_data *cdata) {
	size_t i = 0;
	/*
	 * the index of the character immediately following
	 * the terminating character ($) in the text
	 */
	size_t text_end = length + 2;
	fprintf(stream, "Printing the selected range [%zu, %zu) "
			"of suffixes.\n", begin_offset, end_offset);
	for (i = begin_offset; i < end_offset; ++i) {
		if (st_print_single_suffix(stream, i,
					internal_text_encoding,
					text + cdata->tsuffixes[i],
					text_end - cdata->tsuffixes[i]) > 0) {
			fprintf(stderr, "Printing of the suffix at the "
					"position of %zu failed!\n", i);
			return (1);
		}
	}
	fprintf(stream, "Printing complete!\n");
	return (0);
}

/* dumping functions */

/**
 * A function which dumps the current table of partitions
 * into the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the current table
 * 		of partitions will be dumped
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_dump_partitions (FILE *stream,
		const pwotd_construction_data *cdata) {
	size_t i = 0;
	fprintf(stream, "Printing the information about the current "
			"partitions of suffixes.\n");
	for (i = 0; i < cdata->partitions_number; ++i) {
		fprintf(stream, "%zu:\tbegin_offset: %zu, end_offset: "
				"%zu, lcp_size: %zu, "
				"text_offset: %zu\n", i,
				cdata->partitions[i].begin_offset,
				cdata->partitions[i].end_offset,
				cdata->partitions[i].lcp_size,
				cdata->partitions[i].text_offset);
	}
	fprintf(stream, "Printing complete!\n");
	return (0);
}

/**
 * A function which dumps the current table of partitions to be processed
 * into the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the current table
 * 		of partitions to be processed will be dumped
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_dump_partitions_tbp (FILE *stream,
		const pwotd_construction_data *cdata) {
	size_t i = 0;
	fprintf(stream, "Printing the information about the current\n"
			"table of partitions to be processed.\n");
	for (i = 0; i < cdata->partitions_tbp_number; ++i) {
		fprintf(stream, "%zu:\tindex: %zu, tnode_offset: %zu, "
				"parents_depth: %zu\n",
				i, cdata->partitions_tbp[i].index,
				cdata->partitions_tbp[i].tnode_offset,
				cdata->partitions_tbp[i].parents_depth);
	}
	fprintf(stream, "Printing complete!\n");
	return (0);
}

/**
 * A function which dumps the current content of the partitions stack
 * into the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the current content
 * 		of the partitions stack will be dumped
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_dump_partitions_stack (FILE *stream,
		const pwotd_construction_data *cdata) {
	size_t i = 0;
	fprintf(stream, "Partitions stack dump BEGIN\n");
	for (; i < cdata->partitions_stack_top; ++i) {
		fprintf(stream, "%zu:\trange_begin == %zu\n",
				i, cdata->partitions_stack[i].range_begin);
		fprintf(stream, "\trange_end == %zu\n",
				cdata->partitions_stack[i].range_end);
		fprintf(stream, "\tlcp_size == %zu\n",
				cdata->partitions_stack[i].lcp_size);
		fprintf(stream, "\ttnode_offset == %zu\n",
				cdata->partitions_stack[i].tnode_offset);
	}
	fprintf(stream, "Partitions stack dump END\n");
	return (0);
}

/**
 * A function which dumps the current content of the main stack
 * into the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the current content
 * 		of the main stack will be dumped
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_dump_stack (FILE *stream,
		const pwotd_construction_data *cdata) {
	size_t i = 0;
	fprintf(stream, "Main stack dump BEGIN\n");
	for (; i < cdata->stack_top; ++i) {
		fprintf(stream, "%zu:\trange_begin == %zu\n",
				i, cdata->stack[i].range_begin);
		fprintf(stream, "\trange_end == %zu\n",
				cdata->stack[i].range_end);
		fprintf(stream, "\tlcp_size == %zu\n",
				cdata->stack[i].lcp_size);
		fprintf(stream, "\ttnode_offset == %zu\n",
				cdata->stack[i].tnode_offset);
	}
	fprintf(stream, "Main stack dump END\n");
	return (0);
}
