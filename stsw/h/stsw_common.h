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
 * Suffix tree over a sliding window common declarations.
 * This file contains the common declarations, which are used
 * by the functions for the construction and maintenance
 * of the suffix tree over a sliding window.
 */
#ifndef	SUFFIX_TREE_SLIDING_WINDOW_COMMON_HEADER
#define	SUFFIX_TREE_SLIDING_WINDOW_COMMON_HEADER

#include "suffix_tree_common.h"

/* if we are on the Apple platform (Mac OS X, for example) */
#ifdef	__APPLE__

#ifndef _POSIX_C_SOURCE

/**
 * We need to define this macro by hand
 * to enable the POSIX threads on the Apple platform
 */
#define	_POSIX_C_SOURCE 199506L

#endif

#endif

/*
 * if the the macro _POSIX_C_SOURCE is defined,
 * either by the compiler or explicitly
 */
#ifdef	_POSIX_C_SOURCE
/*
 * We need to check if the supported POSIX features
 * conform at least to the IEEE Std 1003.1c-1995
 */

/*
 * The "minus zero" is usually used when we are not sure
 * whether the specific macro is defined. It ensures
 * that the value in parentheses will be expanded
 * to a non-blank expression in any case.
 * Here, we don't have to use it, because we already know
 * that the macro _POSIX_C_SOURCE is defined.
 */
#if	(_POSIX_C_SOURCE - 0) >= 199506L

/* we can use the POSIX threads */
#define	STSW_USE_PTHREAD
#include <pthread.h>

#endif

#endif

/* constants */

/* sliding window block status flags */

extern const int BLOCK_STATUS_UNKNOWN;
extern const int BLOCK_STATUS_READ_AND_UNPROCESSED;
extern const int BLOCK_STATUS_STILL_IN_USE;

/* struct typedefs */

/**
 * A struct containing the sliding window over the text coming
 * from the input file and all the necessary information
 * for the sliding window maintenance.
 * The sliding window is divided into blocks. They are read
 * and processed one at a time.
 * 'ap_scale_factor' blocks together form the active part
 * of the sliding window.
 * It is the portion of the sliding window, which currently has
 * the suffix tree constructed over its characters.
 * 'sw_scale_factor' blocks together form the entire sliding window.
 */
typedef struct text_file_sliding_window_struct {
	/**
	 * the total number of bytes
	 * currently allocated for the sliding window
	 */
	size_t bytes_allocated;
	/**
	 * the additional number of bytes
	 * currently allocated for this struct
	 */
	size_t additional_bytes_allocated;
	/**
	 * The total number of blocks read so far
	 * to freshen the content of the sliding window.
	 * If the most recently read block contains less
	 * newly replaced characters than its size,
	 * the number of blocks read is still incremented by one.
	 */
	size_t blocks_read;
	/**
	 * The total number of bytes, which have been read
	 * from the input file and converted to characters so far.
	 * These characters have been continuously used to replace
	 * the characters in the oldest blocks of the sliding window.
	 */
	size_t bytes_read;
	/**
	 * the total number of characters, which have been read
	 * from the input file into the sliding window so far
	 */
	size_t characters_read;
	/**
	 * the current size of the entire sliding window,
	 * not including the unused 0.th character
	 */
	size_t total_window_size;
	/** the maximum size of the active part of the sliding window */
	size_t max_ap_window_size;
	/** the current size of the active part of the sliding window */
	size_t ap_window_size;
	/**
	 * the current offset of the first character
	 * in the currently active part of the sliding window
	 */
	size_t ap_window_begin;
	/**
	 * the current offset of the character just after the last character
	 * in the currently active part of the sliding window
	 *
	 * Note that we are talking about a character offset
	 * inside the sliding window. This implies that in case
	 * the active part of the sliding window currently ends
	 * at the last character of the sliding window,
	 * the appropriate ending character of the active part
	 * of the sliding window would be the first character of
	 * the sliding window.
	 */
	size_t ap_window_end;
	/**
	 * the maximum size of the active part of the sliding window
	 * as a multiple of the size of a single block in the sliding window
	 */
	size_t ap_scale_factor;
	/**
	 * the total size of the sliding window
	 * as a multiple of the size of a single block in the sliding window
	 */
	size_t sw_scale_factor;
	/**
	 * The current number of blocks forming the sliding window.
	 * The characters in the sliding window will be renewed by
	 * one block at a time to avoid large I/O overhead.
	 */
	size_t sw_blocks;
	/**
	 * The index of a block in the current sliding window,
	 * which has been most recently processed. It means
	 * that the sliding window has moved so that
	 * it no longer contains any part of this block.
	 * If the most recently processed block contains less
	 * newly replaced characters than its size,
	 * the index of the most recently processed block
	 * is still incremented by one.
	 * If none of the blocks in the current sliding window
	 * have been processed yet, then the correct value
	 * for this variable is the index of the last block
	 * in the sliding window, which is @ref sw_blocks - 1.
	 */
	size_t sw_mrp_block;
	/**
	 * The index of a block in the current sliding window,
	 * which has been most recently read.
	 * If the most recently read block contains less
	 * newly replaced characters than its size,
	 * the index of the most recently read block
	 * is still incremented by one.
	 * If none of the blocks in the current sliding window
	 * have been read yet, then the correct value
	 * for this variable is the index of the last block
	 * in the sliding window, which is @ref sw_blocks - 1.
	 */
	size_t sw_mrr_block;
	/** the number of characters that fit into a single block */
	size_t sw_block_size;
	/**
	 * The flags for each block, which indicate whether or not
	 * it has already been read but not yet processed.
	 * If the threading is enabled, it is necessary
	 * that the access to this member is protected by the mutex.
	 */
	int *sw_block_flags;
	/** the characters in the current sliding window */
	character_type *text_window;
	/** the name of the input file */
	const char *file_name;
	/** the character encoding of the input file */
	const char *fromcode;
	/**
	 * The character encoding used in the memory representation
	 * of the sliding window.
	 * It needs to be a fixed length encoding.
	 */
	const char *tocode;
	/**
	 * The buffer, which will be used for character conversion
	 * while reading from the input file.
	 */
	char *conversion_buffer;
	/** the current size of the conversion buffer */
	size_t conversion_buffer_size;
	/** the iconv input buffer */
	char *inbuf;
	/** the iconv output buffer */
	char *outbuf;
	/** the number of bytes left in the iconv input buffer */
	size_t inbytesleft;
	/** the number of bytes left in the iconv output buffer */
	size_t outbytesleft;
	/** the desired method of the edge label maintenance to use */
	int elm_method;
	/** the read-only file descriptor associated with the input file */
	int fd;
	/** the conversion descriptor used by the iconv */
	iconv_t cd; /* iconv_t is just a typedef for void* */
} text_file_sliding_window;

#ifdef	STSW_USE_PTHREAD
/**
 * A struct containing the data shared between the main thread,
 * which performs the suffix tree construction itself
 * and the auxiliary thread, which reads the input file
 * and replaces the already expired characters
 * in the oldest blocks of the sliding window with the new ones.
 */
typedef struct shared_data_struct {
	/** the mutex */
	pthread_mutex_t mx;
	/** the condition variable */
	pthread_cond_t cv;
	/** the pointer to the sliding window */
	text_file_sliding_window *tfsw;
	/**
	 * if this variable evaluates to true, it means that
	 * the reading thread has already finished the reading
	 * of the input file
	 */
	int reading_finished;
	/**
	 * The number of characters, which have been read and replaced
	 * in the most recently read block of the sliding window.
	 * This variable is used only after the reading has finished
	 * and its purpose is to provide the information about the number
	 * of valid characters in the final, most recently read block
	 * of the sliding window to the main thread.
	 */
	size_t final_block_characters;
	/**
	 * The number of the most recently read block of the sliding window.
	 * This variable is used only after the reading has finished
	 * and its purpose is to provide the information about the number
	 * of the final, the most recently read block of the sliding window
	 * to the main thread.
	 */
	size_t final_block_number;
} shared_data;

/* auxiliary functions */

/* thread related auxiliary function */

void *reading_thread_function (void *arg);
#endif

size_t stsw_get_leafs_depth_order (size_t sw_offset,
		const text_file_sliding_window *tfsw);
int stsw_validate_sw_offset (size_t sw_offset,
		const text_file_sliding_window *tfsw);

/* reading related functions */

int text_file_open (const int verbosity_level,
		const char *file_name,
		const char *input_file_encoding,
		char **internal_character_encoding,
		size_t desired_sw_block_size,
		size_t desired_ap_scale_factor,
		size_t desired_sw_scale_factor,
		int desired_elm_method,
		text_file_sliding_window *tfsw);
int text_file_close (const int verbosity_level,
		text_file_sliding_window *tfsw);
int text_file_read_blocks (size_t blocks_to_read,
		size_t *blocks_read,
		size_t *characters_read,
		size_t *bytes_read,
		text_file_sliding_window *tfsw);

/* printing functions */

int stsw_print_edge (FILE *stream,
		int print_terminating_character,
		signed_integral_type parent,
		signed_integral_type child,
		signed_integral_type childs_suffix_link,
		unsigned_integral_type parents_depth,
		unsigned_integral_type childs_depth,
		size_t log10bn,
		size_t log10l,
		size_t childs_offset,
		const text_file_sliding_window *tfsw);
int stsw_print_sliding_window (FILE *stream,
		text_file_sliding_window *tfsw);
int stsw_print_stats (const int verbosity_level,
		size_t length,
		size_t max_ap_window_size,
		size_t min_edges,
		size_t min_edges_ap_begin,
		double avg_edges,
		size_t max_edges,
		size_t max_edges_ap_begin,
		size_t min_branching_nodes,
		size_t min_brn_ap_begin,
		double avg_branching_nodes,
		size_t max_branching_nodes,
		size_t max_brn_ap_begin,
		size_t tedge_size,
		size_t tbranch_size,
		size_t leaf_record_size,
		size_t edge_record_size,
		size_t branch_record_size,
		size_t extra_allocated_memory_size,
		size_t extra_used_memory_size);

#endif /* SUFFIX_TREE_SLIDING_WINDOW_COMMON_HEADER */
