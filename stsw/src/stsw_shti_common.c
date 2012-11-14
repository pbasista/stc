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
 * SHTI common functions implementation.
 * This file contains the implementation of the auxiliary functions,
 * which are used by the functions, which construct and maintain
 * the suffix tree over a sliding window
 * using the implementation type SHTI with backward pointers.
 */
#include "stsw_shti_common.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* allocation functions */

/**
 * A function which allocates the memory for the suffix tree
 * over a sliding window implemented using the SH implementation technique.
 *
 * @param
 * verbosity_level	the desired level of messaging verbosity
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	On successful allocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int stsw_shti_allocate (const int verbosity_level,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw) {
	/*
	 * the future size of the table tbranch (the first estimation,
	 * which will be further adjusted)
	 */
	size_t unit_size = (size_t)(1) <<
		(sizeof (size_t) * 8 - 1);
	size_t allocated_size = 0;
	if (verbosity_level > 1) {
		printf("==============================================\n"
			"Trying to allocate memory for the suffix tree:\n\n");
	}
	/* we need to fill in the size of the hash settings */
	stsw->hs_size = sizeof (hash_settings);
	/* we need to fill in the size of the leaf record */
	stsw->lr_size = sizeof (leaf_record_shti);
	/* we need to fill in the size of the edge record */
	stsw->er_size = sizeof (edge_record);
	/* we need to fill in the size of the branch record */
	stsw->br_size = sizeof (branch_record_shti);
	/*
	 * The adjustment of the future size of the table tbranch.
	 * We take advantage of the fact that the maximum number of branching
	 * nodes is by one less than the maximum number of leaves,
	 * which is 'tfsw->max_ap_window_size'
	 */
	while (tfsw->max_ap_window_size <= unit_size) {
		unit_size = unit_size >> 1; /* unit_size / 2 */
	}
	/* the unit size should always be positive */
	if (unit_size == 0) {
		fprintf(stderr, "Warning: The unit_size has been decreased to "
				"zero!\nAdjusting its value back to one!\n\n");
		unit_size = 1;
	}
	if ((hs_deallocate(stsw->hs)) > 0) {
		fprintf(stderr, "Error: Could not deallocate "
				"the hash settings!\n");
		return (1);
	}
	if (verbosity_level > 1) {
		printf("Trying to allocate memory for the hash settings:\n"
			"%zu cells of 1 bytes (totalling %zu bytes, ",
			stsw->hs_size, stsw->hs_size);
		print_human_readable_size(stdout, stsw->hs_size);
		printf(").\n");
	}
	/*
	 * We allocate and clear the memory for the hash settings.
	 * To achieve it, we simply use calloc instead of malloc.
	 */
	stsw->hs = calloc(stsw->hs_size, (size_t)(1));
	if (stsw->hs == NULL) {
		perror("calloc(stsw->hs)");
		/* resetting the errno */
		errno = 0;
		return (2);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size = stsw->hs_size;
	if (verbosity_level > 1) {
		printf("Successfully allocated!\n\n");
	}
	/* filling in some of the desired values for the hash settings */
	stsw->hs->crt_type = stsw->crt_type;
	stsw->hs->chf_number = stsw->chf_number;
	/*
	 * The maximum number of leaves in the suffix tree
	 * is "tfsw->max_ap_window_size".
	 * The estimated maximum number of edges is at least as high
	 * as the maximum number of all vertices
	 * (which is 2 * tfsw->max_ap_window_size - 1) decreased by one.
	 * That is 2 * tfsw->max_ap_window_size - 2, which is equal to
	 * 2 * (tfsw->max_ap_window_size - 1).
	 * To be sure that all the edges fit in the hash table,
	 * we want it to have at least this many records.
	 * We have to make this assignment before we call
	 * the hs_update function, because this function call
	 * can possibly increase the desired value.
	 */
	stsw->tedge_size = 2 * (tfsw->max_ap_window_size - 1);
	/*
	 * Moreover, to increase the effectivity of the double hashing,
	 * in case this collision resolution technique has been chosen,
	 * we would like to increase the size of the hash table even more.
	 */
	if (stsw->crt_type == 2) {
		/*
		 * This, however, is not a robust solution,
		 * because in real usage, the deletions
		 * will continuously make more and more
		 * vacant edge records in the hash table,
		 * which will result in lookups getting
		 * slower and slower over time anyway.
		 */
		stsw->tedge_size *= 2;
	}
	/* we update the hash table size and hash settings */
	if (hs_update(verbosity_level, &(stsw->tedge_size), stsw->hs) != 0) {
		fprintf(stderr, "Error: Can not correctly update "
				"the hash settings.\n");
		return (3);
	}
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stsw->tleaf);
	stsw->tleaf = NULL;
	if (verbosity_level > 1) {
		printf("Trying to allocate memory for tleaf:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
				tfsw->max_ap_window_size + 1, stsw->lr_size,
				(tfsw->max_ap_window_size + 1) *
				stsw->lr_size);
		print_human_readable_size(stdout,
				(tfsw->max_ap_window_size + 1) *
				stsw->lr_size);
		printf(").\n");
	}
	/*
	 * The number of actually allocated leaf records is increased by one,
	 * because of the 0.th record, which is never used.
	 */
	stsw->tleaf = calloc(tfsw->max_ap_window_size + 1, stsw->lr_size);
	if (stsw->tleaf == NULL) {
		perror("calloc(tleaf)");
		/* resetting the errno */
		errno = 0;
		return (4);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += (tfsw->max_ap_window_size + 1) * stsw->lr_size;
	if (verbosity_level > 1) {
		printf("Successfully allocated!\n\n");
	}
	/*
	 * we store the number of the real leaf records available
	 * rather than the number of leaf records allocated
	 */
	stsw->tleaf_size = tfsw->max_ap_window_size;
	/* the first leaf should start at the index of 1 */
	stsw->tleaf_first = 1;
	/*
	 * The table tleaf is empty at the beginning and this is indicated
	 * by setting the index of the last leaf to zero. It also makes it
	 * easy to increase the usable size of the table tleaf to one
	 * by just incrementing this variable.
	 */
	stsw->tleaf_last = 0;
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stsw->tbranch);
	stsw->tbranch = NULL;
	if (verbosity_level > 1) {
		printf("Trying to allocate memory for tbranch:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
				unit_size + 1, stsw->br_size,
				(unit_size + 1) * stsw->br_size);
		print_human_readable_size(stdout,
				(unit_size + 1) * stsw->br_size);
		printf(").\n");
	}
	/*
	 * The number of actually allocated branch records is increased by one,
	 * because of the 0.th record, which is never used.
	 */
	stsw->tbranch = calloc(unit_size + 1, stsw->br_size);
	if (stsw->tbranch == NULL) {
		perror("calloc(tbranch)");
		/* resetting the errno */
		errno = 0;
		return (5);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += (unit_size + 1) * stsw->br_size;
	if (verbosity_level > 1) {
		printf("Successfully allocated!\n\n");
	}
	/*
	 * This memory is cleared in advance, so we need not to do
	 * the following assignments. But we do, because we would like
	 * to emphasize the correctness of the information stored
	 * for the root in the clean table tbranch.
	 *
	 * stsw->tbranch[1] is the root and therefore has no parent
	 * (it can not have any).
	 * Moreover, no credit counter is necessary for the root,
	 * so the zero value here does not violate the design
	 * of the credit bits stored inside the parent pointers.
	 */
	stsw->tbranch[1].parent = 0;
	/* it has the depth of zero (which never changes) */
	stsw->tbranch[1].depth = 0;
	/* its head position is zero (by definition) */
	stsw->tbranch[1].head_position = 0;
	/* its suffix link is undefined (and can never be defined) */
	stsw->tbranch[1].suffix_link = 0;
	/* it has zero children in the beginning */
	stsw->tbranch[1].children = 0;
	/* So, in the beginning, we have only one branching node - the root. */
	stsw->branching_nodes = 1;
	/*
	 * we store the number of the real branch records available
	 * rather than the number of branch records allocated
	 */
	stsw->tbranch_size = unit_size;
	/* The first table tbranch size increase step */
	stsw->tbsize_increase = unit_size >> 1; /* unit_size / 2 */
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stsw->tbranch_deleted);
	stsw->tbranch_deleted = NULL;
	if (verbosity_level > 1) {
		printf("Trying to allocate memory for the array of indices\n"
				"of the branching records deleted from "
				"the table tbranch and currently vacant:\n"
				"%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
				unit_size, sizeof (unsigned_integral_type),
				unit_size * sizeof (unsigned_integral_type));
		print_human_readable_size(stdout,
				unit_size * sizeof (unsigned_integral_type));
		printf(").\n");
	}
	/*
	 * By default, we assume that at some point all the branching nodes
	 * will be deleted. That's why the size of this array needs to be
	 * at least as large as the maximum number of branching nodes.
	 */
	stsw->tbranch_deleted = calloc(unit_size,
			sizeof (unsigned_integral_type));
	if (stsw->tbranch_deleted == NULL) {
		perror("calloc(tbranch_deleted)");
		/* resetting the errno */
		errno = 0;
		return (6);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += unit_size * sizeof (unsigned_integral_type);
	if (verbosity_level > 1) {
		printf("Successfully allocated!\n\n");
	}
	/* Initially, there are no deleted branching records. */
	stsw->tbdeleted_records = 0;
	/*
	 * And the maximum index of the deleted
	 * branching record is also zero.
	 */
	stsw->max_tbdeleted_index = 0;
	/*
	 * the default size of the array of indices of the branching records
	 * deleted from the table tbranch and currently vacant is unit_size
	 */
	stsw->tbdeleted_size = unit_size;
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(stsw->tedge);
	stsw->tedge = NULL;
	if (verbosity_level > 1) {
		printf("Trying to allocate memory for tedge:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
				stsw->tedge_size, stsw->er_size,
				stsw->tedge_size * stsw->er_size);
		print_human_readable_size(stdout,
				stsw->tedge_size * stsw->er_size);
		printf(").\n");
	}
	/*
	 * The number of edge records has already been determined,
	 * so we just use it.
	 */
	stsw->tedge = calloc(stsw->tedge_size, stsw->er_size);
	if (stsw->tedge == NULL) {
		perror("calloc(tedge)");
		/* resetting the errno */
		errno = 0;
		return (7);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	allocated_size += stsw->tedge_size * stsw->er_size;
	if (verbosity_level > 1) {
		printf("Successfully allocated!\n\n");
	}
	/* the number of all edges currently present in the hash table */
	stsw->edges = 0;
	/* stsw->tedge_size / 2 */
	stsw->tesize_increase = stsw->tedge_size >> 1;
	if (verbosity_level > 0) {
		printf("Total amount of memory initially allocated: "
				"%zu bytes (", allocated_size);
		print_human_readable_size(stdout, allocated_size);
		printf(")\nwhich is %.3f bytes per each character "
				"of the active part.\n\n",
				(double)(allocated_size) /
				(double)(tfsw->max_ap_window_size));
	}
	if (verbosity_level > 1) {
		printf("The memory has been successfully allocated!\n"
			"===========================================\n\n");
	}
	return (0);
}

/**
 * A function which reallocates the memory for the suffix tree
 * to make it larger.
 *
 * @param
 * verbosity_level	the desired level of messaging verbosity
 * @param
 * desired_tbranch_size	The minimum requested size of the table tbranch
 * 			(in the number of nodes, not including
 * 			the leading 0.th node).
 * 			If zero is given, no reallocation
 * 			of the table tbranch is performed.
 * @param
 * desired_tedge_size	The minimum requested size of the hash table.
 * 			If zero is given, no reallocation
 * 			of the hash table is performed.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	On successful reallocation, this function returns 0.
 * 		If an error occurs, a positive error number is returned.
 */
int stsw_shti_reallocate (const int verbosity_level,
		size_t desired_tbranch_size,
		size_t desired_tedge_size,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw) {
	void *tmp_pointer = NULL;
	/*
	 * the future size of the table tbranch (the first estimation,
	 * which will be further adjusted)
	 */
	size_t new_tbranch_size = stsw->tbranch_size +
		stsw->tbsize_increase;
	/*
	 * the future size of the table tedge (the first estimation,
	 * which will be further adjusted)
	 */
	size_t new_tedge_size = stsw->tedge_size +
		stsw->tesize_increase;
	if (desired_tbranch_size > 0) {
		/*
		 * if the implicitly chosen new size of the table tbranch
		 * is too small
		 */
		if (new_tbranch_size < desired_tbranch_size) {
			/* we make it large enough */
			new_tbranch_size = desired_tbranch_size;
		}
		/*
		 * on the other hand, if the new size of the table tbranch
		 * is too large, we make it smaller, but still large enough
		 * to hold all the data the table tbranch can possibly hold
		 */
		if (new_tbranch_size >= tfsw->max_ap_window_size) {
			/*
			 * the maximum number of branching nodes is one less
			 * than the maximum number of leaf nodes (which is
			 * equal to the maximum size of the active part
			 * of the sliding window)
			 */
			new_tbranch_size = tfsw->max_ap_window_size - 1;
		}
		/* if we will be shrinking the table tbranch */
		if (new_tbranch_size < stsw->tbranch_size) {
			/*
			 * we need to check whether the array of deleted nodes
			 * from the table tbranch is empty
			 */
			if (stsw->tbdeleted_records > 0) {
				/*
				 * If it is not, we can not guarantee that
				 * the reallocation will not shrink
				 * the table tbranch so that some
				 * of the indices in the table
				 * tbranch_deleted will become invalid.
				 * That's why the reallocation in this case
				 * will not be allowed.
				 */
				fprintf(stderr, "The reallocation of the "
						"table tbranch to a smaller "
						"size\nis not allowed, "
						"because the table "
						"tbranch_deleted "
						"is not empty.\n");
				return (1);
			}
		}
		if (verbosity_level > 0) {
			printf("Trying to reallocate the table tbranch:\n");
		}
		if (verbosity_level > 1) {
			printf("new size:\n%zu cells of %zu bytes "
					"(totalling %zu bytes, ",
					new_tbranch_size + 1, stsw->br_size,
					(new_tbranch_size + 1) *
					stsw->br_size);
			print_human_readable_size(stdout,
					(new_tbranch_size + 1) *
					stsw->br_size);
			printf(").\n");
		}
		/*
		 * The number of actually allocated branch records
		 * is increased by one, because of the 0.th record,
		 * which is never used.
		 */
		tmp_pointer = realloc(stsw->tbranch,
				(new_tbranch_size + 1) * stsw->br_size);
		if (tmp_pointer == NULL) {
			perror("realloc(tbranch)");
			/* resetting the errno */
			errno = 0;
			return (2);
		} else {
			/*
			 * Despite that the call to the realloc seems
			 * to have been successful, we reset the errno,
			 * because at least on Mac OS X
			 * it might have changed.
			 */
			errno = 0;
			stsw->tbranch = tmp_pointer;
		}
		/*
		 * we store the number of the real branch records available
		 * rather than the number of branch records allocated
		 */
		stsw->tbranch_size = new_tbranch_size;
		/*
		 * finally, we adjust the size increase step
		 * for the next resize of the table tbranch
		 */
		if (stsw->tbsize_increase < 256) {
			/* minimum size increase step */
			stsw->tbsize_increase = 128;
		} else {
			/* division by 2 */
			stsw->tbsize_increase = stsw->tbsize_increase >> 1;
		}
		if (verbosity_level > 0) {
			printf("Successfully reallocated!\n"
					"Trying to reallocate the memory "
					"for the array of indices\n"
					"of the branching records deleted "
					"from the table tbranch "
					"and currently vacant:\n");
		}
		if (verbosity_level > 1) {
			printf("new size:\n%zu cells of %zu bytes "
					"(totalling %zu bytes, ",
					new_tbranch_size,
					sizeof (unsigned_integral_type),
					new_tbranch_size *
					sizeof (unsigned_integral_type));
			print_human_readable_size(stdout,
					new_tbranch_size *
					sizeof (unsigned_integral_type));
			printf(").\n");
		}
		tmp_pointer = realloc(stsw->tbranch_deleted,
			new_tbranch_size * sizeof (unsigned_integral_type));
		if (tmp_pointer == NULL) {
			perror("realloc(tbranch_deleted)");
			/* resetting the errno */
			errno = 0;
			return (3);
		} else {
			/*
			 * Despite that the call to the realloc seems
			 * to have been successful, we reset the errno,
			 * because at least on Mac OS X
			 * it might have changed.
			 */
			errno = 0;
			stsw->tbranch_deleted = tmp_pointer;
		}
		/*
		 * we store the new size of the array of indices
		 * of the branching records
		 * deleted from the table tbranch and currently vacant
		 */
		stsw->tbdeleted_size = new_tbranch_size;
		if (verbosity_level > 0) {
			printf("Successfully reallocated!\n");
		}
	}
	if (desired_tedge_size > 0) {
		/*
		 * if the implicitly chosen new size of the hash table
		 * is too small
		 */
		if (new_tedge_size < desired_tedge_size) {
			/* we make it large enough */
			new_tedge_size = desired_tedge_size;
		}
		/*
		 * The new size of the hash table can never be "too large",
		 * because the hash table is not meant to be fully occupied,
		 * so if it is larger, it just means that we can expect less
		 * hash collisions.
		 */
		if (verbosity_level > 0) {
			printf("Trying to reallocate the table tedge:\n");
		}
		if (verbosity_level > 1) {
			printf("desired new size:\n%zu cells of %zu bytes "
					"(totalling %zu bytes, ",
					new_tedge_size, stsw->er_size,
					new_tedge_size * stsw->er_size);
			print_human_readable_size(stdout,
					new_tedge_size * stsw->er_size);
			printf(").\n");
		}
		/*
		 * The hash table size is supposed to meet certain conditions
		 * (e.g. it should be a prime). That's why the new_tedge_size
		 * is just a recommendation on the new hash table size,
		 * which will further be adjusted by the function
		 * stsw_shti_ht_rehash. This function also updates
		 * the tedge_size entry of the suffix tree struct.
		 */
		if (stsw_shti_ht_rehash(&new_tedge_size, tfsw, stsw)
				== 0) {
			/*
			 * finally, we adjust the size increase step
			 * for the next resize of the hash table
			 */
			if (stsw->tesize_increase < 256) {
				/* minimum size increase step */
				stsw->tesize_increase = 128;
			} else {
				/* division by 2 */
				stsw->tesize_increase =
					stsw->tesize_increase >> 1;
			}
			if (verbosity_level > 0) {
				printf("Successfully reallocated!\n");
			}
			if (verbosity_level > 1) {
				printf("current size:\n"
						"%zu cells of %zu bytes "
						"(totalling %zu bytes, ",
					stsw->tedge_size, stsw->er_size,
					stsw->tedge_size * stsw->er_size);
				print_human_readable_size(stdout,
					stsw->tedge_size * stsw->er_size);
				printf(").\n");
			}
		} else {
			fprintf(stderr, "Error: The reallocation of the "
					"hash table has failed!\n");
			return (4);
		}
	}
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
 * child	the target node of an edge which will be depth-scanned
 * @param
 * target_depth	the depth which we will try to reach
 * @param
 * stsw		the actual suffix tree
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
int stsw_shti_edge_depthscan (signed_integral_type child,
		unsigned_integral_type target_depth,
		const suffix_tree_sliding_window_shti *stsw) {
	unsigned_integral_type childs_depth = 0;
	if (child == 0) {
		fprintf(stderr,	"stsw_shti_edge_depthscan:\n"
				"Error: Invalid number of child (0)!\n");
		return (2); /* not a valid child */
	} else if (child > 0) { /* child is a branching node */
		childs_depth = stsw->tbranch[child].depth;
	} else { /* child < 0 => child is a leaf */
		/*
		 * The depth of a leaf is the total length of the suffix,
		 * which starts at the root and ends at this leaf.
		 */
		if (stsw_shti_get_leafs_depth(child, (size_t *)
					(&childs_depth), stsw) > 0) {
			fprintf(stderr,	"stsw_shti_edge_depthscan:\n"
					"Error: Could not get "
					"the depth of the child "
					"(%d). Exiting!\n", child);
			return (3);
		}
	}
	if (childs_depth < target_depth) {
		return (-1); /* an edge shorter than required */
	} else if (childs_depth == target_depth) {
		return (0); /* an edge of exact length */
	} else { /* childs_depth > target_depth */
		return (1); /* too long edge */
	}
}

/**
 * A function to perform "slowscan" on the given edge
 * starting with the letter at the specified position in the sliding window.
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
 * 			"position". However, the sign of this parameter
 * 			will NOT indicate the type of the first mismatch,
 * 			if any, as it did with the implementation type SL.
 * 			The reason is simple: the implementation type SH
 * 			can not utilize this information.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If all the letters of the provided edge match, 0 is returned.
 * 		If only some of the first edge letters match, 1 is returned.
 * 		If the number of letters to compare is smaller than the length
 * 		of the edge, but all these letters do match, 2 is returned.
 * 		If an error occures, a positive error number
 * 		greater than 2 is returned.
 */
int stsw_shti_edge_slowscan (signed_integral_type parent,
		signed_integral_type child,
		size_t position,
		signed_integral_type *last_match_position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	size_t remaining_edge_letters = 0;
	size_t remaining_sw_characters = 0;
	size_t edge_letter_index = 0;
	size_t edge_letter_index_at_start = 0;
	/*
	 * the position in the text just after the last letter
	 * of the provided edge
	 */
	size_t edge_letter_index_end = 0;
	/*
	 * if this variable evaluates to true, we will be scanning
	 * (and comparing) all the letters of an edge
	 */
	int comparing_all_letters = 1;
	/* we suppose that the parent node of this edge is a branching node */
	if (child == 0) {
		fprintf(stderr,	"stsw_shti_edge_slowscan:\n"
				"Error: Invalid number of child (0)!\n");
		/*
		 * As there were no matching characters, we have to set
		 * the "(*last_match_position)" accordingly.
		 */
		(*last_match_position) = 0;
		return (3); /* not a valid child */
	} else if (child > 0) { /* child is a branching node */
		edge_letter_index_at_start = stsw->tbranch[child].
			head_position + stsw->tbranch[parent].depth;
		if (edge_letter_index_at_start > tfsw->total_window_size) {
			edge_letter_index_at_start -= tfsw->total_window_size;
		}
		edge_letter_index_end = stsw->tbranch[child].head_position +
			stsw->tbranch[child].depth;
		if (edge_letter_index_end > tfsw->total_window_size) {
			edge_letter_index_end -= tfsw->total_window_size;
		}
	} else { /* child < 0 => child is a leaf */
		edge_letter_index_at_start = (unsigned_integral_type)
			(stsw_shti_get_leafs_sw_offset(child, tfsw, stsw)) +
			stsw->tbranch[parent].depth;
		if (edge_letter_index_at_start > tfsw->total_window_size) {
			edge_letter_index_at_start -= tfsw->total_window_size;
		}
		/*
		 * one character after the last character of the currently
		 * active part of the sliding window
		 */
		edge_letter_index_end = tfsw->ap_window_end;
	}
	remaining_edge_letters = edge_letter_index_end -
		edge_letter_index_at_start;
	if (edge_letter_index_end <= edge_letter_index_at_start) {
		remaining_edge_letters += tfsw->total_window_size;
	}
	remaining_sw_characters = tfsw->ap_window_end - position;
	if (tfsw->ap_window_end <= position) {
		remaining_sw_characters += tfsw->total_window_size;
	}
	/*
	 * if the edge length is greater than the number of remaining
	 * characters in the sliding window, we have to adjust the number
	 * of edge label characters to be compared
	 */
	if (remaining_edge_letters > remaining_sw_characters) {
		/*
		 * we do it by decreasing the position
		 * where the comparison stops
		 */
		edge_letter_index_end = edge_letter_index_at_start +
			remaining_sw_characters;
		if (edge_letter_index_end > tfsw->total_window_size) {
			edge_letter_index_end -= tfsw->total_window_size;
		}
		/*
		 * we have to remember that we will not scan
		 * all the letters of the provided edge
		 */
		comparing_all_letters = 0;
	}
	edge_letter_index = edge_letter_index_at_start;
	/* while the comparison is successful */
	while (tfsw->text_window[edge_letter_index] ==
			tfsw->text_window[position]) {
		/* we increment the edge letter index */
		if (edge_letter_index == tfsw->total_window_size) {
			edge_letter_index = 1;
		} else {
			++edge_letter_index;
		}
		/* as well as the position in the text */
		if (position == tfsw->total_window_size) {
			position = 1;
		} else {
			++position;
		}
		/* we check whether we should not end the comparison now */
		if (edge_letter_index == edge_letter_index_end) {
			/*
			 * We set the number of matching edge letters
			 * to the length of the edge (all the letters match).
			 */
			(*last_match_position) = (signed_integral_type)
				(remaining_edge_letters);
			/*
			 * if we have (successfully) scanned and compared
			 * all the letters of the edge
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
	 */
	(*last_match_position) = (signed_integral_type)(edge_letter_index -
			edge_letter_index_at_start);
	if (edge_letter_index <= edge_letter_index_at_start) {
		(*last_match_position) += (signed_integral_type)
			(tfsw->total_window_size);
	}
	return (1); /* success */
}

/**
 * A function which advances to the next child and assigns
 * the pointers accordingly. It is a quick variant of the function,
 * which means that it does not keep track of previous child.
 *
 * @param
 * parent	the node from which the current edge starts
 * @param
 * child	the node which will be replaced by its next brother
 * 		(or by zero, if it does not have the next brother)
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully advance to the next child,
 * 		0 is returned.
 * 		If there is no next child or in case of any error,
 * 		a positive error number is returned.
 */
int stsw_shti_quick_next_child (signed_integral_type parent,
		signed_integral_type *child,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	size_t letter_offset = 0;
	int retval = 0;
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	character_type letter = WCHAR_MIN;
#else
	character_type letter = CHAR_MIN;
#endif
	if (parent <= 0) {
		fprintf(stderr,	"stsw_shti_quick_next_child:\n"
				"Error: The provided parent (%u) "
				"is not a branching node!\n", parent);
		return (1);
	}
	if ((*child) == 0) {
		/*
		 * We are expected to return the first child,
		 * so we have to start with the first lookup separately,
		 * because we are expected to test the first letter as well.
		 */
		if (stsw_shti_ht_lookup(parent, letter, child,
					tfsw, stsw) == 0) {
			/* We have successfully found the first child! */
			return (0);
		}
	} else if ((*child) > 0) {
		letter_offset = stsw->tbranch[(*child)].head_position +
			stsw->tbranch[parent].depth;
		if (letter_offset > tfsw->total_window_size) {
			letter_offset -= tfsw->total_window_size;
		}
		letter = tfsw->text_window[letter_offset];
	} else { /* (*child) < 0 */
		letter_offset = stsw_shti_get_leafs_sw_offset((*child),
				tfsw, stsw) + stsw->tbranch[parent].depth;
		if (letter_offset > tfsw->total_window_size) {
			letter_offset -= tfsw->total_window_size;
		}
		letter = tfsw->text_window[letter_offset];
	}
	if (letter == terminating_character) {
		/*
		 * We have already reached the last child
		 * of the current parent.
		 */
		return (2);
	}
	(*child) = 0; /* resetting the current number of child */
	do {
		++letter;
		retval = stsw_shti_ht_lookup(parent, letter, child,
				tfsw, stsw);
	} while ((letter < terminating_character) && (retval > 0));
	if (retval == 0) {
		/* We have successfully found the next child! */
		return (0);
	} else {
		/* We were unable to find any next child. */
		return (3);
	}
}

/**
 * A function which advances to the next child and assigns
 * the pointers accordingly.
 *
 * @param
 * parent	the node from which the current edge starts
 * @param
 * child	the node which will be replaced by its next brother
 * 		(or by zero, if it does not have the next brother)
 * @param
 * prev_child	the node which will be replaced by the original value
 * 		of the "child" node
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully advance to the next child,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_next_child (signed_integral_type parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	size_t letter_offset = 0;
	int retval = 0;
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	character_type letter = WCHAR_MIN;
#else
	character_type letter = CHAR_MIN;
#endif
	if (parent <= 0) {
		fprintf(stderr,	"stsw_shti_next_child:\n"
				"Error: The provided parent (%u) "
				"is not a branching node!\n", parent);
		return (1);
	}
	if ((*child) == 0) {
		/*
		 * We are expected to return the first child,
		 * so we have to start with the first lookup separately,
		 * because we are expected to test the first letter as well.
		 */
		if (stsw_shti_ht_lookup(parent, letter, child,
					tfsw, stsw) == 0) {
			/* We have successfully found the first child! */
			return (0);
		}
	} else if ((*child) > 0) {
		letter_offset = stsw->tbranch[(*child)].head_position +
			stsw->tbranch[parent].depth;
		if (letter_offset > tfsw->total_window_size) {
			letter_offset -= tfsw->total_window_size;
		}
		letter = tfsw->text_window[letter_offset];
	} else { /* (*child) < 0 */
		letter_offset = stsw_shti_get_leafs_sw_offset((*child),
				tfsw, stsw) + stsw->tbranch[parent].depth;
		if (letter_offset > tfsw->total_window_size) {
			letter_offset -= tfsw->total_window_size;
		}
		letter = tfsw->text_window[letter_offset];
	}
	if (letter == terminating_character) {
		/*
		 * We have already reached the last child
		 * of the current parent.
		 */
		return (2);
	}
	/* saving the current number of child as the previous child */
	(*prev_child) = (*child);
	(*child) = 0; /* resetting the current number of child */
	do {
		++letter;
		retval = stsw_shti_ht_lookup(parent, letter, child,
				tfsw, stsw);
	} while ((letter < terminating_character) && (retval > 0));
	if (retval == 0) {
		/* We have successfully found the next child! */
		return (0);
	} else {
		/* We were unable to find any next child. */
		return (3);
	}
}

/**
 * A function which ascends up along the path leading to the root.
 *
 * @param
 * parent	The node, which is the parent of the "child" node.
 * 		Unless it is the root, it will be given the value of its parent
 * 		when this function returns.
 * @param
 * child	The node, which is a child of the "parent" node.
 * 		When this function successfully returns,
 * 		it will be set to the "parent".
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully descend down along an edge,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_edge_climb (signed_integral_type *parent,
		signed_integral_type *child,
		const suffix_tree_sliding_window_shti *stsw) {
	/* if the parent is either a leaf, undefined or the root */
	if ((*parent) < 2) {
		fprintf(stderr,	"stsw_shti_edge_climb:\n"
				"Error: Invalid number of parent (%d)!\n",
				(*parent));
		return (1); /* ascending failed (invalid number of parent) */
	}
	(*child) = (*parent);
	/* from now on we suppose that parent is a branching node */
	(*parent) = abs(stsw->tbranch[(*parent)].parent);
	return (0);
}

/**
 * A function which descends down along an edge.
 * It is a quick variant of this function,
 * which means that it does not keep track of the previous child.
 *
 * @param
 * parent	the node from which the given edge starts
 * 		(it will be given the value of "child"
 * 		when the function returns)
 * @param
 * child	The node where the given edge ends.
 * 		When the function returns, it will be set to zero,
 * 		regardless of the presence of the child of the "child" node.
 * @param
 * position	The position in the text of the substring which matches
 * 		the letters of the given edge. When this function returns,
 * 		it will be set to the first letter after the substring
 * 		which matches this very same edge.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully descend down along an edge,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_quick_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		size_t *position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	size_t childs_depth = 0;
	/* we suppose that the parent node of this edge is a branching node */
	if ((*child) == 0) {
		fprintf(stderr,	"stsw_shti_quick_edge_descend:\n"
				"Error: Invalid number of child (0)!\n");
		return (1); /* invalid number of child */
	} else if ((*child) > 0) {
		(*position) = (*position) + stsw->tbranch[(*child)].depth -
			stsw->tbranch[(*parent)].depth;
		if ((*position) > tfsw->total_window_size) {
			(*position) -= tfsw->total_window_size;
		}
	} else { /* (*child) < 0 */
		if (stsw_shti_get_leafs_depth((*child),
					&childs_depth, stsw) > 0) {
			fprintf(stderr,	"stsw_shti_quick_edge_descend:\n"
					"Error: Could not get "
					"the depth of the child "
					"(%d). Exiting!\n", (*child));
			return (2);
		}
		(*position) = (*position) + childs_depth -
			stsw->tbranch[(*parent)].depth;
		if ((*position) > tfsw->total_window_size) {
			(*position) -= tfsw->total_window_size;
		}
	}
	/* saving the current number of child as the next parent */
	(*parent) = (*child);
	(*child) = 0;
	return (0);
}

/**
 * A function which descends down along an edge.
 *
 * @param
 * parent	the node from which the given edge starts
 * 		(it will be given the value of "child"
 * 		when the function returns)
 * @param
 * child	The node where the given edge ends.
 * 		When the function returns, it will be set to zero,
 * 		regardless of the presence of the child of the "child" node.
 * @param
 * prev_child	the node which will be set to zero
 * 		if this function successfully returns.
 * @param
 * position	The position in the text of the substring which matches
 * 		the letters of the given edge. When this function returns,
 * 		it will be set to the first letter after the substring
 * 		which matches this very same edge.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully descend down along an edge,
 * 		0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_edge_descend (signed_integral_type *parent,
		signed_integral_type *child,
		signed_integral_type *prev_child,
		size_t *position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	size_t childs_depth = 0;
	/* we suppose that the parent node of this edge is a branching node */
	if ((*child) == 0) {
		fprintf(stderr,	"stsw_shti_edge_descend:\n"
				"Error: Invalid number of child (0)!\n");
		return (1); /* invalid number of child */
	} else if ((*child) > 0) {
		(*position) = (*position) + stsw->tbranch[(*child)].depth -
			stsw->tbranch[(*parent)].depth;
		if ((*position) > tfsw->total_window_size) {
			(*position) -= tfsw->total_window_size;
		}
	} else { /* (*child) < 0 */
		if (stsw_shti_get_leafs_depth((*child),
					&childs_depth, stsw) > 0) {
			fprintf(stderr,	"stsw_shti_edge_descend:\n"
					"Error: Could not get "
					"the depth of the child "
					"(%d). Exiting!\n", (*child));
			return (2);
		}
		(*position) = (*position) + childs_depth -
			stsw->tbranch[(*parent)].depth;
		if ((*position) > tfsw->total_window_size) {
			(*position) -= tfsw->total_window_size;
		}
	}
	/* saving the current number of child as the next parent */
	(*parent) = (*child);
	(*child) = 0;
	(*prev_child) = 0;
	return (0);
}

/**
 * A function which selects the target node of an edge, which starts
 * at the provided parent node and begins with the provided letter
 * (if such an edge exists).
 *
 * @param
 * parent	the node from which the desired edge should start
 * @param
 * child	the node which will be set to the first matching edge's end
 * @param
 * position	The position in the sliding window of the first
 * 		character of the string, which we would like
 * 		to match against the edges leading from
 * 		the provided parent node.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could find an edge with the matching first character,
 * 		0 is returned.
 * 		If the number of parent from which we should do branching
 * 		is invalid, 1 is returned.
 * 		If no matching edge was found, 2 is returned.
 */
int stsw_shti_branch_once (signed_integral_type parent,
		signed_integral_type *child,
		size_t position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	character_type letter = L'\0';
#else
	character_type letter = '\0';
#endif
	if (parent <= 0) {
		fprintf(stderr,	"stsw_shti_branch_once:\n"
				"Error: Invalid number of parent (%d)!\n",
				parent);
		return (1); /* branching failed (invalid number of parent) */
	}
	letter = tfsw->text_window[position];
	if (stsw_shti_ht_lookup(parent, letter, child, tfsw, stsw) == 0) {
		/* lookup (or "fastscan") succeeded */
		return (0); /* branching succeeded (matching edge found) */
	} else {
		/* lookup (or "fastscan") was unsuccessful */
		return (2); /* branching failed (no matching edge) */
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
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully reach the exact desired depth,
 * 		0 is returned.
 * 		If the source node of the last edge explored has the depth
 * 		smaller than the desired depth and the target node
 * 		of the same edge has the depth larger than the desired depth,
 * 		(-1) is returned.
 * 		If even the last node of the explored path is too shallow,
 * 		1 (one) is returned.
 * 		If the branching fails while the desired depth
 * 		is still not reached, 2 (two) is returned.
 */
int stsw_shti_go_down (signed_integral_type grandpa,
		signed_integral_type *parent,
		unsigned_integral_type target_depth,
		size_t *position,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	signed_integral_type child = 0; /* the child of the "parent" */
	int tmp = 0; /* here we store the return value of the "depthscan" */
	(*parent) = grandpa;
	/* parent has to be a branching node */
	if (stsw->tbranch[(*parent)].depth == target_depth) {
		/*
		 * we need not to descend, because we have
		 * already reached the exact target depth
		 */
		return (0);
	}
	while (stsw_shti_branch_once((*parent), &child, (*position),
				tfsw, stsw) == 0) {
		if ((tmp = stsw_shti_edge_depthscan(child, target_depth,
						stsw)) == -1) {
			/*
			 * the edge length is strictly smaller
			 * than the maximum allowed length
			 */
			grandpa = (*parent);
			stsw_shti_quick_edge_descend(parent, &child,
					position, tfsw, stsw);
			if ((*parent) < 0) { /* parent is a leaf */
				fprintf(stderr,	"stsw_shti_go_down:\n"
						"Warning: By increasing the "
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
		} else {
			/*
			 * We can be sure that the child node is nonzero
			 * and therefore tmp == 1 here. That means that
			 * the target node of an edge is too deep.
			 */
			return (-1); /* we are stopping here */
		}
	}
	fprintf(stderr,	"stsw_shti_go_down:\n"
			"Error: Branching failed while the desired depth\n"
			"still has not been reached!\n");
	return (2);
}

/**
 * A function which tries to reach the desired depth
 * by ascending up in the suffix tree.
 *
 * @param
 * parent	The value of this parameter will be overwritten immediately
 * 		in the beginning of this function.
 * 		After this function returns, it will be set to the parent node
 * 		of the current "child" node. If this function returns
 * 		successfully, its depth will be exactly the target_depth.
 * @param
 * child	The node from which we start to ascend up.
 * 		When this function returns, it will be set
 * 		to the the most shallow node on the path towards the root,
 * 		which depth is strictly larger than the desired depth.
 * @param
 * target_depth	the depth which we are trying to reach
 * 		by ascending up in the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully reach the exact desired depth,
 * 		0 is returned.
 * 		If we could not reach exactly the desired depth, but we have
 * 		successfully reached an edge, which parent is too shallow
 * 		and which child is too deep, (-1) is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_go_up (signed_integral_type *parent,
		signed_integral_type *child,
		unsigned_integral_type target_depth,
		const suffix_tree_sliding_window_shti *stsw) {
	unsigned_integral_type parents_depth = 0;
	if ((*child) == 0) { /* child number is invalid */
		fprintf(stderr,	"stsw_shti_go_up:\n"
				"Error: Invalid number of child (0)!\n");
		return (1); /* invalid number of child */
	} else if ((*child) > 0) { /* child is a branching node */
		/* we set the parent of this child */
		(*parent) = abs(stsw->tbranch[(*child)].parent);
	} else { /* child < 0 (child is a leaf ) */
		/* we set the parent of this child */
		(*parent) = stsw->tleaf[-(*child)].parent;
	}
	/* from now on, the parent is expected to be a branching node */
	do { /* we check the current depth and act appropriately */
		parents_depth = stsw->tbranch[(*parent)].depth;
		if (parents_depth == target_depth) {
			/* we have found the node with the desired depth */
			return (0);
		} else if (parents_depth < target_depth) {
			/*
			 * We have climbed too high and now the parent's depth
			 * is too small.
			 */
			return (-1);
		} /* else we continue to ascend */
	/* While we can ascend up, we do it */
	} while (stsw_shti_edge_climb(parent, child, stsw) == 0);
	/*
	 * If we got here, it means that we could not climb further,
	 * because we either reached the root node
	 * or because of some error occurred. This way or another,
	 * it means that this function call was unsuccessful.
	 */
	fprintf(stderr,	"stsw_shti_go_up:\n"
			"Error: We could not climb higher,\n"
			"but we still has not reached the desired depth!\n");
	return (2);
}

/**
 * A function which creates a new leaf at the specific position
 * in the suffix tree.
 *
 * @param
 * parent	the parent of a newly created leaf
 * @param
 * letter	the first letter of an edge leading from the parent
 * 		to the newly created leaf
 * @param
 * new_leaf	the number of the newly created leaf
 * 		(the negative value of the starting position of the suffix,
 * 		which will end at this new leaf)
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully create a new leaf, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_create_leaf (signed_integral_type parent,
		character_type letter,
		signed_integral_type new_leaf,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw) {
	if (parent <= 0) {
		fprintf(stderr,	"stsw_shti_create_leaf:\n"
				"Error: Could not create a new child "
				"of a non-branching node number %d!\n",
				parent);
		return (1); /* invalid number of parent */
	}
	/* rehash operation is initially allowed */
	if (stsw_shti_ht_insert(parent, letter, new_leaf, 1,
			tfsw, stsw) != 0) {
		fprintf(stderr,	"stsw_shti_create_leaf:\n"
				"Error: Could not insert the new leaf node\n"
				"into the hash table!\n");
		return (2);
	}
	/* we set the parent of the new leaf node */
	stsw->tleaf[-new_leaf].parent = parent;
	/*
	 * we increment the number of children
	 * of the parent of the new leaf node
	 */
	++stsw->tbranch[parent].children;
	return (0);
}

/**
 * A function which splits the edge from the provided parent
 * to the provided child in the suffix tree into two edges
 * at the specified position while creating a new branching node.
 *
 * @param
 * parent	The source node of an edge which will be split.
 * 		When the function returns, the "parent" will be set to
 * 		the newly created branching node.
 * @param
 * letter	The first letter of an edge leading from the parent
 * 		to the child.
 * @param
 * child	The target node of an edge, which will be split.
 * 		When the function returns, it will be set to zero.
 * @param
 * position	The position in the sliding window of the beginning
 * 		of a substring, which partially matches
 * 		the characters of an edge from the "parent" to the "child".
 * 		When the function returns, it will be set to the position
 * 		in the sliding window just after the last character of an edge
 * 		from the parent to the newly created branching node.
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
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree
 *
 * @return	If we could successfully split the edge and create
 * 		a new branching node, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_split_edge (signed_integral_type *parent,
		character_type letter,
		signed_integral_type *child,
		size_t *position,
		signed_integral_type last_match_position,
		unsigned_integral_type new_head_position,
		const text_file_sliding_window *tfsw,
		suffix_tree_sliding_window_shti *stsw) {
	size_t letter_offset = 0;
	signed_integral_type new_branching_node = 0;
	unsigned_integral_type childs_head_position = 0;
	if ((*parent) <= 0) {
		fprintf(stderr,	"stsw_shti_split_edge:\n"
				"Error: Invalid number of parent (%d)!\n",
				(*parent));
		return (1); /* invalid number of parent */
	}
	if ((*child) == 0) {
		fprintf(stderr,	"stsw_shti_split_edge:\n"
				"Error: Invalid number of child (0)!\n");
		return (2); /* invalid number of child */
	} else if ((*child) > 0) { /* child is a branching node */
		childs_head_position = stsw->tbranch[(*child)].head_position;
	} else { /* (*child) < 0 => child is a leaf */
		childs_head_position = (unsigned_integral_type)
			(stsw_shti_get_leafs_sw_offset((*child), tfsw, stsw));
	}
	/* if there are no vacant positions in the table tbranch */
	if (stsw->tbdeleted_records == 0) {
		/* we insert the new node at the end of the table tbranch */
		new_branching_node =
			(signed_integral_type)(++stsw->branching_nodes);
	} else {
		/*
		 * otherwise, we use the position of the record
		 * most recently deleted from the table tbranch
		 */
		--stsw->tbdeleted_records;
		new_branching_node = (signed_integral_type)
			(stsw->tbranch_deleted[stsw->tbdeleted_records]);
		/*
		 * invalidating the last entry in the table of indices
		 * of branching records deleted from the table tbranch
		 */
		stsw->tbranch_deleted[stsw->tbdeleted_records] = 0;
	}
	if (last_match_position == 0) {
		/*
		 * Either we don't know the mismatch position or
		 * it is at the beginning of an edge.
		 * In both cases, we can not split an edge nor create
		 * a new branching node. So, in fact, it is a failure.
		 */
		return (3);
	}
	/*
	 * This is where the implementation with backward pointers differs
	 * from the original implementation.
	 *
	 * Here, we need to identify the edge label maintenance method,
	 * which is currently in use. This method determines
	 * the default value of the credit counter for the newly created
	 * branching node.
	 */
	if (tfsw->elm_method == 1) { /* batch update by M. Senft */
		/*
		 * By default, when using this edge label maintenance method,
		 * we can completely avoid using any credit counters.
		 * That's why the new branching nodes
		 * are created with the credit set to zero.
		 */
		stsw->tbranch[new_branching_node].parent = (*parent);
	} else {
		/*
		 * By default, when using the other edge label maintenance
		 * methods, we are expected to send a credit from each
		 * newly created leaf node to its immediate parent.
		 * As we already know that the reason for splitting the edge
		 * is that a new leaf node is going to be added, we can create
		 * the new branching nodes directly with the credit
		 * set to one and completely ignore the next requirement
		 * to send the credit.
		 */
		stsw->tbranch[new_branching_node].parent = -(*parent);
	}
	/* the following value will be set later */
	stsw->tbranch[new_branching_node].suffix_link = 0;
	stsw->tbranch[new_branching_node].depth =
		stsw->tbranch[(*parent)].depth +
		(unsigned_integral_type)(last_match_position);
	stsw->tbranch[new_branching_node].head_position = new_head_position;
	/* correcting the old edge to end at the new_branching_node */
	if (stsw_shti_ht_insert((*parent), letter,
				new_branching_node, 1, tfsw, stsw) != 0) {
		fprintf(stderr,	"Error: Could not correct the old edge "
				"to end at the new_branching_node!\n");
		return (4);
	}
	letter_offset = childs_head_position +
			stsw->tbranch[new_branching_node].depth;
	if (letter_offset > tfsw->total_window_size) {
		letter_offset -= tfsw->total_window_size;
	}
	letter = tfsw->text_window[letter_offset];
	/*
	 * creating the new edge from the new_branching_node
	 * to the original child
	 */
	if (stsw_shti_ht_insert(new_branching_node, letter,
				(*child), 1, tfsw, stsw) != 0) {
		fprintf(stderr,	"stsw_shti_split_edge:\n"
				"Error: Could not insert the new edge "
				"starting at the new_branching_node\n"
				"into the hash table!\n");
		return (5);
	}
	/* we set the number of children of the new branching node */
	stsw->tbranch[new_branching_node].children = 1;
	/*
	 * a difference from the implementation
	 * without the backward pointers
	 */
	if ((*child) > 0) { /* child is a branching node */
		if (stsw->tbranch[(*child)].parent == 0) {
			/*
			 * If the (*child) == 1, which means that
			 * the child is the root, this would be
			 * perfectly correct.
			 * But since the child can only be a target node
			 * of an edge, it can not be the root
			 * and therefore it has to have a parent.
			 */
			fprintf(stderr,	"stsw_shti_split_edge:\n"
					"Error: The parent of the provided "
					"child node (%d) is zero!\n",
					(*child));
			return (6);
		/* we must preserve the child's credit */
		} else if (stsw->tbranch[(*child)].parent > 0) {
			stsw->tbranch[(*child)].parent = new_branching_node;
		} else { /* stsw->tbranch[(*child)].parent < 0 */
			stsw->tbranch[(*child)].parent = -new_branching_node;
		}
	} else { /* (*child) < 0 => child is a leaf */
		stsw->tleaf[-(*child)].parent = new_branching_node;
	}
	(*parent) = new_branching_node;
	(*child) = 0;
	/* adjusting the position in the text */
	(*position) = (*position) +
		(unsigned_integral_type)(last_match_position);
	/* a difference from the original in-memory implementation */
	if ((*position) > tfsw->total_window_size) {
		(*position) -= tfsw->total_window_size;
	}
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
 * starting_node	the node from which the traversal starts
 * @param
 * log10bn	A ceiling of base 10 logarithm of the number
 * 		of branching nodes. It will be used for printing alignment.
 * @param
 * log10l	A ceiling of base 10 logarithm of the number
 * 		of leaves. It will be used for printing alignment.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree which will be traversed
 *
 * @return	If we could successfully traverse and print the suffix tree
 * 		and if we could start from the given node, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_simple_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	signed_integral_type child = 0;
	/* the parent of the child as stored in the child node */
	signed_integral_type childs_parent = 0;
	unsigned_integral_type parents_depth = 0;
	unsigned_integral_type childs_depth = 0;
	size_t childs_offset = 0;
	if (starting_node <= 0) {
		fprintf(stderr,	"Error: The traveral has to start from "
				"a branching node, but the starting node "
				"is %d!", starting_node);
		return (1);
	}
	/* getting the first child of the starting_node */
	if (stsw_shti_quick_next_child(starting_node, &child,
			tfsw, stsw) != 0) {
		fprintf(stderr,	"Error: The traversal of the current branch "
				"is not possible,\nbecause we were not able "
				"to advance to the next child of the parent "
				"(%d)!\n", starting_node);
		return (2);
	}
	parents_depth = stsw->tbranch[starting_node].depth;
	do {
		if (child > 0) {
			childs_depth = stsw->tbranch[child].depth;
			childs_offset = stsw->tbranch[child].head_position;
			if (stsw_validate_sw_offset(childs_offset,
						tfsw) != 0) {
				return (3);
			}
			childs_parent = abs(stsw->tbranch[child].parent);
			if (childs_parent != starting_node) {
				fprintf(stderr,	"Error: Something went wrong."
						"\nChild (%d) has a different "
						"parent (%d)\nfrom what is "
						"stored inside its branching "
						"record (%d).\n", child,
						starting_node, childs_parent);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (4);
			}
			stsw_print_edge(stream, 0,
					(signed_integral_type)(0),
					(signed_integral_type)(0),
					(signed_integral_type)(0),
					parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, tfsw);
			stsw_shti_simple_traverse_from(stream, child,
					log10bn, log10l, tfsw, stsw);
		} else { /* child < 0 */
			if (stsw_shti_get_leafs_depth(child, (size_t *)
						(&childs_depth), stsw) > 0) {
				fprintf(stderr,	"stsw_shti_simple_"
						"traverse_from:\n"
						"Error: Could not get "
						"the depth of the child "
						"(%d).\n", child);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (5);
			}
			childs_offset = stsw_shti_get_leafs_sw_offset(child,
					tfsw, stsw);
			childs_parent = stsw->tleaf[-child].parent;
			if (childs_parent != starting_node) {
				fprintf(stderr,	"Error: Something went wrong."
						"\nChild (%d) has a different "
						"parent (%d)\nfrom what is "
						"stored inside its leaf "
						"record (%d).\n", child,
						starting_node, childs_parent);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (6);
			}
			stsw_print_edge(stream, 0,
					(signed_integral_type)(0),
					child, (signed_integral_type)(0),
					parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, tfsw);
		}
		/*
		 * there is quite a large overhead to move to the next child
		 * of the starting_node, but it is unevitable, because of
		 * the nature of the hash table representation.
		 */
	} while (stsw_shti_quick_next_child(starting_node, &child,
			tfsw, stsw) == 0);
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
 * starting_node	the node from which the traversal starts
 * @param
 * log10bn	A ceiling of base 10 logarithm of the number
 * 		of branching nodes. It will be used for printing alignment.
 * @param
 * log10l	A ceiling of base 10 logarithm of the number
 * 		of leaves. It will be used for printing alignment.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree which will be traversed
 *
 * @return	If we could successfully traverse and print the suffix tree
 * 		and if we could start from the given node, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int stsw_shti_traverse_from (FILE *stream,
		signed_integral_type starting_node,
		size_t log10bn,
		size_t log10l,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	signed_integral_type child = 0;
	/* the parent of the child as stored in the child node */
	signed_integral_type childs_parent = 0;
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
	/* getting the first child of the starting_node */
	if (stsw_shti_quick_next_child(starting_node, &child,
			tfsw, stsw) != 0) {
		fprintf(stderr,	"Error: The traversal of the current branch "
				"is not possible,\nbecause we were not able "
				"to advance to the next child of the parent "
				"(%d)!\n", starting_node);
		return (2);
	}
	parents_depth = stsw->tbranch[starting_node].depth;
	do {
		if (child > 0) {
			childs_suffix_link = stsw->tbranch[child].suffix_link;
			childs_depth = stsw->tbranch[child].depth;
			childs_offset = stsw->tbranch[child].head_position;
			if (stsw_validate_sw_offset(childs_offset,
						tfsw) != 0) {
				return (3);
			}
			childs_parent = abs(stsw->tbranch[child].parent);
			if (childs_parent != starting_node) {
				fprintf(stderr,	"Error: Something went wrong."
						"\nChild (%d) has a different "
						"parent (%d)\nfrom what is "
						"stored inside its branching "
						"record (%d).\n", child,
						starting_node, childs_parent);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (4);
			}
			stsw_print_edge(stream, 0, starting_node, child,
					childs_suffix_link, parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, tfsw);
			stsw_shti_traverse_from(stream, child, log10bn, log10l,
					tfsw, stsw);
		} else { /* child < 0 */
			if (stsw_shti_get_leafs_depth(child, (size_t *)
						(&childs_depth), stsw) > 0) {
				fprintf(stderr,	"stsw_shti_"
						"traverse_from:\n"
						"Error: Could not get "
						"the depth of the child "
						"(%d).\n", child);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (5);
			}
			childs_offset = stsw_shti_get_leafs_sw_offset(child,
					tfsw, stsw);
			childs_parent = stsw->tleaf[-child].parent;
			if (childs_parent != starting_node) {
				fprintf(stderr,	"Error: Something went wrong."
						"\nChild (%d) has a different "
						"parent (%d)\nfrom what is "
						"stored inside its leaf "
						"record (%d).\n", child,
						starting_node, childs_parent);
				fprintf(stderr,	"The traversal of this branch "
						"is terminated here.\n");
				return (6);
			}
			stsw_print_edge(stream, 0, starting_node, child,
					0, parents_depth,
					childs_depth, log10bn, log10l,
					childs_offset, tfsw);
		}
		/*
		 * there is quite a large overhead to move to the next child
		 * of the starting_node, but it is unevitable, because of
		 * the nature of the hash table representation.
		 */
	} while (stsw_shti_quick_next_child(starting_node, &child,
			tfsw, stsw) == 0);
	return (0);
}

/* handling functions */

/**
 * A function which traverses the suffix tree while printing its edges.
 *
 * @param
 * verbosity_level	the desired level of messaging verbosity
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written
 * @param
 * traversal_type	the type of the traversal to perform
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 * @param
 * stsw		the actual suffix tree to be traversed
 *
 * @return	If the traversal was successful, zero is returned.
 * 		Otherwise, a positive error number is returned.
 */
int stsw_shti_traverse (const int verbosity_level,
		FILE *stream,
		int traversal_type,
		const text_file_sliding_window *tfsw,
		const suffix_tree_sliding_window_shti *stsw) {
	signed_integral_type start_node = 1; /* the root */
	/* the current number of branching nodes in the suffix tree */
	size_t branching_nodes = stsw->branching_nodes;
	/* the current number of leaves in the suffix tree */
	size_t leaves = tfsw->ap_window_size;
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
	if (verbosity_level > 2) {
		printf("Traversing the suffix tree\n\n");
		if (stream != stdout) {
			printf("The traversal log is being dumped "
					"to the specified file.\n");
		}
	}
	if (traversal_type == tt_detailed) {
		fprintf(stream, "Suffix tree traversal BEGIN\n");
		if (stsw_shti_traverse_from(stream, start_node,
					log10bn, log10l,
					tfsw, stsw) > 0) {
			fprintf(stderr, "Error: The traversal "
					"from the branching node\n"
					"was unsuccessful. "
					"Exiting!\n");
			return (1);
		}
		fprintf(stream, "Suffix tree traversal END\n");
	} else if (traversal_type == tt_simple) {
		fprintf(stream, "Simple suffix tree traversal BEGIN\n");
		if (stsw_shti_simple_traverse_from(stream, start_node,
					log10bn, log10l,
					tfsw, stsw) > 0) {
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
	if (verbosity_level > 2) {
		if (stream != stdout) {
			printf("Dump complete.\n");
		}
		printf("\nTraversing completed\n\n");
	}
	return (0);
}

/**
 * A function which deallocates the memory used by the suffix tree.
 *
 * @param
 * verbosity_level	the desired level of messaging verbosity
 * @param
 * stsw		the actual suffix tree to be "deleted" (in more detail:
 * 		all the data it contains will be erased and all the
 * 		additionally allocated memory it occupies will be deallocated)
 *
 * @return	On successful deallocation, this function returns 0.
 * 		If there is nothing to deallocate (the memory
 * 		for the suffix tree has not yet been allocated
 * 		or it has been already deallocated), this function returns 1.
 * 		If an error occurs, a positive error number
 * 		greater than 1 is returned.
 */
int stsw_shti_delete (const int verbosity_level,
		suffix_tree_sliding_window_shti *stsw) {
	if ((stsw->hs == NULL) &&
			(stsw->tbranch == NULL) &&
			(stsw->tedge == NULL)) {
		return (1); /* nothing to delete */
	}
	if (verbosity_level > 0) {
		printf("Deleting the suffix tree\n");
	}
	if (hs_deallocate(stsw->hs) > 0) {
		fprintf(stderr,	"Error: The hash settings could not be "
				"properly deallocated!\n");
		return (2);
	}
	free(stsw->tedge);
	stsw->tedge = NULL;
	free(stsw->tbranch_deleted);
	stsw->tbranch_deleted = NULL;
	free(stsw->tbranch);
	stsw->tbranch = NULL;
	free(stsw->tleaf);
	stsw->tleaf = NULL;
	/*
	 * maintaining the suffix tree struct
	 * constistent with its definition
	 */
	stsw->edges = 0;
	stsw->tedge_size = 0;
	stsw->tbdeleted_records = 0;
	stsw->max_tbdeleted_index = 0;
	stsw->tbdeleted_size = 0;
	stsw->branching_nodes = 0;
	stsw->tbranch_size = 0;
	stsw->tleaf_size = 0;
	stsw->tleaf_first = 1;
	stsw->tleaf_last = 0;
	/*
	 * The other entries of the suffix_tree_sliding_window_shti struct
	 * need not to be reset to zero.
	 */
	if (verbosity_level > 0) {
		printf("Successfully deleted!\n");
	}
	return (0);
}
