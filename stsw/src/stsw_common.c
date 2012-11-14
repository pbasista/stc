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
 * Suffix tree over a sliding window common functions implementation.
 * This file contains the implementation of the common functions,
 * which are used for the construction and maintenance
 * of the suffix tree over a sliding window.
 */
#include "stsw_common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* constants */

/* sliding window block status flags */

/**
 * A flag indicating that the respective block in the sliding window
 * has unknown read status and unknown processed status.
 */
const int BLOCK_STATUS_UNKNOWN = 0;

/**
 * A flag indicating that the respective block in the sliding window
 * has already been read and it has not been completely processed yet.
 * Even if the number of the valid characters in the particular block
 * is smaller than the total size of the block, it can still
 * be given this flag legitimately. It suffices that there are no more
 * characters to be read from the input file
 * and that the processing of this block has not yet finished.
 */
const int BLOCK_STATUS_READ_AND_UNPROCESSED = 1;

/**
 * A flag indicating that the respective block in the sliding window
 * has already been processed, but can not be rewritten
 * with the new characters right now.
 * Usually such a block is still at least partially in use.
 */
const int BLOCK_STATUS_STILL_IN_USE = 2;

/* auxiliary functions */

/**
 * A function which reads the next part of the previously specified input file
 * into the previously specified part of the provided sliding window.
 * This function does not perform the checking nor adjusting
 * of the sliding window block flags or the index of the most recently read
 * block. It is user's responsibility to check
 * and appropriately change these flags and the index if necessary.
 * Also, this function does not update the total number of blocks,
 * characters or bytes read in the text_file_sliding_window struct.
 * Again, it is user's responsibility to adjust these values accordingly,
 * based on this function's output parameters.
 *
 * @param
 * characters_read	When this function successfully finishes,
 * 			this variable will be replaced with the total
 * 			number of characters read from the input file
 * 			and converted during this function call.
 * @param
 * bytes_read	When this function successfully finishes,
 * 		this variable will be replaced with the total number of bytes
 * 		read from the input file in this function call.
 * 		Note that this variable contains the number of bytes read,
 * 		which does not necessarily have to be the same
 * 		as the number of bytes read and converted.
 * 		For example, when there are no bytes left in the output buffer,
 * 		but there are still some bytes left in the input (conversion)
 * 		buffer, it means that not all of the bytes
 * 		which have already been read have also already been used
 * 		for the conversion to the characters. These read but
 * 		yet unused bytes will be utilized later,
 * 		in the next call of this function.
 * @param
 * tfsw		the sliding window which will be updated
 * 		with the new characters coming
 * 		from the previously specified input file
 *
 * @return	If the reading was successful, this function returns 0.
 * 		If the reading was partially successful (it had to stop,
 * 		because the input file does not have enough
 * 		unread bytes left), (-1) is returned.
 * 		Otherwise, a positive error number is returned.
 */
int text_file_read_part (size_t *characters_read,
		size_t *bytes_read,
		text_file_sliding_window *tfsw) {
	/*
	 * the variable used to remember the number of bytes left
	 * in the output buffer at the beginning of the iconv conversion
	 * and to determine the number of bytes converted
	 */
	size_t outbytesleft_at_start = 0;
	/* the return value of the iconv */
	size_t retval = 0;
	/* the number of bytes read by one function call to read() */
	ssize_t current_bytes_read = 0;
	/*
	 * number of unused bytes in the input buffer
	 * used in the last call to the iconv function
	 */
	size_t unused_input_bytes = 0;
	/* at first, we clear the provided output parameters */
	(*bytes_read) = 0;
	(*characters_read) = 0;
	/*
	 * while we have not freshened all the characters
	 * in the first part of the desired blocks
	 */
	while (tfsw->outbytesleft > 0) {
		/*
		 * we remember the current number of bytes,
		 * which remain to be read
		 */
		outbytesleft_at_start = tfsw->outbytesleft;
		/*
		 * we try to use iconv to convert the characters
		 * in the input buffer to the characters in the output buffer
		 *
		 * note that we do not have to use
		 * the parentheses like this: &(tfsw->inbuf),
		 * because the member selection via pointer (->)
		 * has greater priority than the address operator (&)
		 *
		 * The first call to this function has tfsw->inbuf == NULL
		 * and tfsw->inbytesleft == 0 while tfsw->outbuf != NULL
		 * and tfsw->outbytesleft != 0. This is not an error,
		 * because in this case iconv sets the conversion descriptor
		 * to the initial state and stores a corresponding state
		 * shift sequence at tfsw->outbuf.
		 * Since all the internal character encodings
		 * (ASCII, UCS-2LE, UCS-4LE) are state-independent,
		 * no state shift sequence is output to tfsw->outbuf.
		 */
		retval = iconv(tfsw->cd, &tfsw->inbuf, &tfsw->inbytesleft,
				&tfsw->outbuf, &tfsw->outbytesleft);
		/*
		 * FIXME:
		 * We don't know why, but there are segmentation faults
		 * when converting from inbuf to outbuf such that exactly
		 * one of them is exhausted and the other is not.
		 * The segmentation faults occur in the function iconv here.
		 * Unlikely, but it is also possible
		 * that it is a bug in iconv.
		 * FIXME: Resolve this!
		 */
		/* resetting the number of unused bytes */
		unused_input_bytes = 0;
		/* if the iconv has encountered an error */
		if (retval == (size_t)(-1)) {
			/*
			 * The most common case when iconv can return (-1)
			 * and set errno and it is not considered an error
			 * is when the output buffer is fully exhausted
			 * (E2BIG). This means that all the characters
			 * expected to be read in this part have already
			 * been read. Similarly, if iconv has encountered
			 * an incomplete multi-byte sequence at the end
			 * of the input buffer (EINVAL), it is not really
			 * an error as well.
			 * In both of these cases we simply move the unused
			 * part of the input buffer to its beginning
			 * for later processing.
			 */
			if ((errno == E2BIG) || (errno == EINVAL)) {
				/*
				 * An incomplete multi-byte sequence
				 * has been encountered at the end
				 * of the input buffer. We move it
				 * to the beginning of the input buffer
				 * for later processing.
				 */
				memmove(tfsw->conversion_buffer,
						tfsw->inbuf,
						tfsw->inbytesleft);
				/* correcting the number of unused bytes */
				unused_input_bytes = tfsw->inbytesleft;
				/* resetting the errno */
				errno = 0;
			} else {
				perror("text_file_read_part: iconv");
				/* resetting the errno */
				errno = 0;
				return (1);
			}
		} else if (retval > 0) {
			fprintf(stderr,	"text_file_read_part: iconv "
					"converted %zu characters\n"
					"in a nonreversible way!\n",
					retval);
			return (2);
		}
		/*
		 * Now we add the number of characters,
		 * which have just been converted from the conversion buffer
		 * to the total number of characters read so far.
		 */
		(*characters_read) += (outbytesleft_at_start -
				tfsw->outbytesleft) / character_type_size;
		current_bytes_read = read(tfsw->fd, tfsw->conversion_buffer +
				unused_input_bytes,
				tfsw->conversion_buffer_size -
				unused_input_bytes);
		/* we check whether the read has encountered an error */
		if (current_bytes_read == (-1)) {
			perror("text_file_read_part: read");
			/* resetting the errno */
			errno = 0;
			return (3);
		/* if we have reached the end of the input file */
		} else if ((current_bytes_read == 0) &&
			/*
			 * and there are no unused bytes left
			 * in the input buffer
			 */
				(unused_input_bytes == (size_t)(0))) {
			/*
			 * we don't need to increment the total number
			 * of bytes read so far
			 */
			return (-1); /* partial success */
		}
		/*
		 * we set the iconv input buffer to the beginning
		 * of the buffer which has recently been read
		 */
		tfsw->inbuf = tfsw->conversion_buffer;
		/* the maximum number of input bytes to process */
		tfsw->inbytesleft = unused_input_bytes +
			(size_t)(current_bytes_read);
		/* we increment the total number of bytes read so far */
		(*bytes_read) += (size_t)(current_bytes_read);
	}
	return (0);
}

#ifdef	STSW_USE_PTHREAD
/* thread related auxiliary function */

/**
 * A function, which is executed by the auxiliary thread
 * and which reads the input file and replaces the characters
 * in the oldest blocks of the sliding window.
 * This funtion reads one block at a time.
 *
 * @param
 * arg		The void * type of the pointer to the shared_data struct,
 * 		which holds all the data necessary for this thread's operation
 * 		and for the synchronization with the main thread.
 *
 * @return	If the whole input file has been successfully read
 * 		and all the parts of the sliding window have been
 * 		successfully overwritten, this function returns NULL.
 * 		Otherwise, if an error occurs,
 * 		a positive error number type-cast to (void*) is returned.
 */
void *reading_thread_function (void *arg) {
	/* an index of the currently read block */
	size_t block_to_read = 0;
	size_t blocks_read = 0;
	size_t characters_read = 0;
	size_t bytes_read = 0;
	/* the return value from the function text_file_read_blocks */
	int retval = 0;
	shared_data *sd = arg;
	while (1) {
		block_to_read = sd->tfsw->sw_mrr_block + 1;
		if (block_to_read == sd->tfsw->sw_blocks) {
			block_to_read = 0;
		}
		/* start of the first critical section */
		pthread_mutex_lock(&sd->mx);
		/*
		 * the block, which we are going to refresh,
		 * must not be already read but not yet completely processed
		 */
		while ((sd->tfsw->sw_block_flags[block_to_read] !=
				BLOCK_STATUS_UNKNOWN) &&
				(sd->reading_finished == 0)) {
			/* if this condition is not met, we have to wait */
			pthread_cond_wait(&sd->cv, &sd->mx);
		}
		/*
		 * if some other thread has set this variable to nonzero,
		 * we have to finish this thread immediately,
		 * because some special situation might just have occurred
		 */
		if (sd->reading_finished != 0) {
			/*
			 * Signaling might not be necessary,
			 * because the shared data has changed
			 * in the main thread,
			 * not in the reading thread.
			 * FIXME: Remove the signaling?
			 */
			pthread_cond_signal(&sd->cv);
			pthread_mutex_unlock(&sd->mx);
			fprintf(stderr, "The main thread has requested\n"
					"that the reading thread "
					"stops immediately!\n");
			return ((void *)(1));
		}
		/*
		 * From now on, the block has the correct flags set.
		 * We can safely unlock the mutex immediately, because we know
		 * that no other thread can invalidate this condition.
		 * This assumption is true,
		 * because there is only one reading thread.
		 */
		/* FIXME: The signaling is not necessary here, right? */
		pthread_mutex_unlock(&sd->mx);
		/* the end of the first critical section */
		if ((retval = text_file_read_blocks((size_t)(1),
						&blocks_read,
						&characters_read,
						&bytes_read, sd->tfsw)) > 1) {
			/* start of the second critical section */
			pthread_mutex_lock(&sd->mx);
			/*
			 * we have to raise a flag indicating
			 * that the reading has (unsuccessfully) finished
			 */
			sd->reading_finished = 1;
			/*
			 * Signaling is necessary here,
			 * because we have changed the shared data.
			 */
			pthread_cond_signal(&sd->cv);
			pthread_mutex_unlock(&sd->mx);
			fprintf(stderr, "reading_thread_function: "
					"file reading error\n");
			return ((void *)(2));
		}
		/* start of the third critical section */
		pthread_mutex_lock(&sd->mx);
		/*
		 * under the protection of the mutex, we set
		 * the appropriate flag for the most recently read block
		 */
		sd->tfsw->sw_block_flags[block_to_read] =
			BLOCK_STATUS_READ_AND_UNPROCESSED;
		/* if there are no more unread bytes left in the input file */
		if (retval == 1) {
			sd->reading_finished = 1;
			sd->final_block_characters = characters_read;
			/* FIXME: Explain why this is correct! */
			sd->final_block_number = block_to_read;
			pthread_cond_signal(&sd->cv);
			pthread_mutex_unlock(&sd->mx);
			printf("The whole input file has been read.\n");
			break;
		} else { /* retval == 0 */
			pthread_cond_signal(&sd->cv);
			pthread_mutex_unlock(&sd->mx);
		}
		/* the end of the second critical section */
	}
	return (NULL);
}
#endif

/**
 * A function which determines the sliding window offset of the first character
 * of the suffix corresponding to the the leaf with the provided number.
 * Important:
 * It is not advisable to call this function, unless the suffix tree itself
 * is not available. There are suffix-tree-implementation-aware versions
 * of this function, which are slightly faster.
 *
 * @param
 * child	the number of child which corresponds to the suffix,
 * 		for which we would like to determine the offset
 * 		of its first character in the sliding window
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 *
 * @return	this function returns the determined sliding window offset
 * 		of the first character of the suffix corresponding
 * 		to the provided child
 */
size_t stsw_get_leafs_sw_offset (signed_integral_type child,
		const text_file_sliding_window *tfsw) {
	size_t ap_window_begin_sw_block = (tfsw->ap_window_begin - 1) /
		tfsw->sw_block_size;
	size_t ap_window_begin_sw_block_offset = (tfsw->ap_window_begin - 1) %
		tfsw->sw_block_size;
	size_t ap_window_end_sw_block = (tfsw->ap_window_end - 1) /
		tfsw->sw_block_size;
	size_t ap_window_end_sw_block_offset = (tfsw->ap_window_end - 1) %
		tfsw->sw_block_size;
	size_t ap_window_begin_ap_block = 0;
	size_t childs_number = (size_t)(-child);
	size_t childs_ap_block = (childs_number - 1) / tfsw->sw_block_size;
	size_t childs_ap_block_offset = (childs_number - 1) %
		tfsw->sw_block_size;
	size_t childs_sw_block = 0;
	size_t childs_sw_offset = 0;
	size_t sw_read_loops = (tfsw->blocks_read - 1) / tfsw->sw_blocks;
	if (ap_window_end_sw_block_offset == 0) {
		if (ap_window_end_sw_block == 0) {
			ap_window_end_sw_block = tfsw->sw_blocks - 1;
		} else {
			--ap_window_end_sw_block;
		}
	}
	if (tfsw->sw_mrr_block < ap_window_end_sw_block) {
		--sw_read_loops;
	} else if (ap_window_end_sw_block < ap_window_begin_sw_block) {
		--sw_read_loops;
	}
	ap_window_begin_ap_block = (sw_read_loops * tfsw->sw_blocks +
			ap_window_begin_sw_block) % tfsw->ap_scale_factor;
	if (childs_ap_block < ap_window_begin_ap_block) {
		childs_ap_block += tfsw->ap_scale_factor;
	} else if (childs_ap_block == ap_window_begin_ap_block) {
		if (childs_ap_block_offset < ap_window_begin_sw_block_offset) {
			childs_ap_block += tfsw->ap_scale_factor;
		}
	}
	childs_sw_block = (ap_window_begin_sw_block + childs_ap_block -
		ap_window_begin_ap_block) % tfsw->sw_blocks;
	childs_sw_offset = childs_sw_block * tfsw->sw_block_size +
		childs_ap_block_offset + 1;
	return (childs_sw_offset);
}

/**
 * a function which determines the depth order of a leaf, which corresponds
 * to the suffix beginning at the provided position in the sliding window.
 * the depth order of a leaf is its depth relative to
 * the maximum possible depth in the current suffix tree, starting from zero.
 * the leaf with the depth order of zero has the maximum possible depth
 * of a suffix in the current suffix tree.
 *
 * @param
 * sw_offset	the position in the sliding window of the first character
 * 		of the suffix, which corresponds to the leaf,
 * 		which depth order we would like to determine.
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 *
 * @return	this function returns the determined depth order of a leaf,
 * 		which corresponds to the suffix beginning at the provided
 * 		position in the sliding window
 */
size_t stsw_get_leafs_depth_order (size_t sw_offset,
		const text_file_sliding_window *tfsw) {
	size_t depth_order = 0;
	if (tfsw->ap_window_begin <= sw_offset) {
		depth_order = sw_offset - tfsw->ap_window_begin;
	} else { /* tfsw->ap_window_begin > sw_offset */
		depth_order = tfsw->total_window_size + sw_offset -
			tfsw->ap_window_begin;
	}
	return (depth_order);
}

/**
 * A function which checks whether the provided offset in the sliding window
 * is or has recently been within the currently active part
 * of the sliding window and therefore is valid.
 *
 * @param
 * sw_offset	the position in the sliding window, which should be validated
 * @param
 * tfsw		the actual sliding window containing the text
 * 		currently used by the suffix tree
 *
 * @return	If the provided sliding window offset is valid,
 * 		this function returns zero.
 * 		Otherwise, a positive error number is returned.
 */
int stsw_validate_sw_offset (size_t sw_offset,
		const text_file_sliding_window *tfsw) {
	/* the begin of the currently valid part of the sliding window */
	size_t vp_window_begin = tfsw->ap_window_begin;
	/*
	 * Here, we need to identify the edge label maintenance method,
	 * which is currently in use. This method determines
	 * the range of the valid sliding window offsets.
	 */
	if (tfsw->elm_method == 1) { /* batch update by M. Senft */
		/*
		 * When using this edge label maintenance method,
		 * the valid sliding window offsets are not only
		 * the ones inside the currently active part
		 * of the sliding window. The offsets
		 * which are at most max_ap_window_size before
		 * the ap_window_begin are valid as well.
		 */
		if (vp_window_begin > tfsw->max_ap_window_size) {
			vp_window_begin -= tfsw->max_ap_window_size;
		} else {
			vp_window_begin = tfsw->total_window_size +
				tfsw->ap_window_begin -
				tfsw->max_ap_window_size;
		}
		/*
		 * Here, it is possible that vp_window_begin ==
		 * tfsw->ap_window_end. In this case, all the offsets
		 * in the sliding window are valid
		 */
		if (vp_window_begin == tfsw->ap_window_end) {
			return (0);
		}
	}
	/*
	 * we now know that vp_window_begin !=
	 * tfsw->ap_window_end
	 */
	if (vp_window_begin < tfsw->ap_window_end) {
		if ((sw_offset < vp_window_begin) ||
				(tfsw->ap_window_end <= sw_offset)) {
			fprintf(stderr, "stsw_validate_sw_offset:\n"
					"The offset %zu is not within "
					"the currently valid "
					"part\nof the sliding "
					"window [%zu, %zu).\n",
					sw_offset,
					vp_window_begin,
					tfsw->ap_window_end);
			return (1);
		}
	} else { /* vp_window_begin > tfsw->ap_window_end */
		if ((sw_offset < vp_window_begin) &&
				(tfsw->ap_window_end <= sw_offset)) {
			fprintf(stderr, "stsw_validate_sw_offset:\n"
					"The offset %zu is not within "
					"the currently valid "
					"part\nof the sliding "
					"window [%zu, %zu).\n",
					sw_offset,
					vp_window_begin,
					tfsw->ap_window_end);
			return (2);
		}
	}
	return (0);
}

/* functions to hande the reading */

/**
 * A function which opens the file descriptor and creates the iconv
 * conversion descriptor for later reading from the specified file.
 *
 * @param
 * verbosity_level	the desired level of messaging verbosity
 * @param
 * file_name	the name of the input file from which the text will be read
 * @param
 * input_file_encoding	The character encoding used in the input file.
 * 			Its default value is UTF-8.
 * @param
 * internal_character_encoding	The character encoding used in the internal
 * 				representation of the text in memory.
 * 				Its default value depends on the size
 * 				of the data type @ref character_type.
 * 				When this function finishes successfully,
 * 				the string representing the currently used
 * 				internal character encoding will be stored
 * 				at the (*internal_character_encoding).
 * @param
 * desired_sw_block_size	The desired size of a single block
 * 				in the sliding window.
 * 				Its default value is 8388608 characters (C)
 * 				or 8 MiC.
 * @param
 * desired_ap_scale_factor	The desired maximum size of the active part
 * 				of the sliding window as a multiple
 * 				of the size of a single block
 * 				in the sliding window.
 * 				The minimum allowed value, as well as
 * 				the default value is 1.
 * @param
 * desired_sw_scale_factor	The desired total size of the sliding window
 * 				as a multiple of the size of a single block
 * 				in the sliding window.
 * 				This value must always be strictly higher than
 * 				the 'ap_scale_factor'. Therefore, the minimum
 * 				allowed value is 'ap_scale_factor'
 * 				increased by 1.
 * 				The default value is 'ap_scale_factor' times 2
 * 				when the edge label maintenance method
 * 				is batch update by M. Senft.
 * 				Otherwise, if the edge label maintenance
 * 				method is different, the default value is
 * 				'ap_scale_factor' increased by 2.
 * @param
 * desired_elm_method	The desired edge label maintenance method to use.
 * 			The default value is 1 for the batch update
 * 			by M. Senft.
 * @param
 * tfsw		When this function finishes successfully, this variable
 * 		will be initialized as a new sliding window for the text
 * 		coming from the desired input file.
 *
 * @return	If this function call is successful, it returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int text_file_open (const int verbosity_level,
		const char *file_name,
		/*
		 * We expect the file_name to consist of standard (short)
		 * characters only.
		 */
		const char *input_file_encoding,
		char **internal_character_encoding,
		size_t desired_sw_block_size,
		size_t desired_ap_scale_factor,
		size_t desired_sw_scale_factor,
		int desired_elm_method,
		text_file_sliding_window *tfsw) {
	/* the default size of a single block in the sliding window */
	size_t sw_block_size = 8388608; /* 8 MiC (8 mebi characters) */
	/* the default value of the active part scale factor */
	size_t ap_scale_factor = 1;
	/* the default value of the sliding window scale factor */
	size_t sw_scale_factor = ap_scale_factor + 2;
	/* the default value of the edge label maintenance method */
	int elm_method = 1; /* batch update by M. Senft */
	tfsw->file_name = file_name;
	/* By default, we suppose that the input file encoding is UTF-8. */
	tfsw->fromcode = "UTF-8";
	/*
	 * The character encoding used in the memory representation.
	 * It will be determined later according to the caller's preference
	 * or the size of the character_type.
	 */
	tfsw->tocode = NULL;
	/* we try to open the input file for reading */
	tfsw->fd = open(file_name, O_RDONLY);
	if (tfsw->fd == -1) {
		perror("text_file_open: open");
		/* resetting the errno */
		errno = 0;
		return (1);
	}
	if (input_file_encoding != NULL) {
		/*
		 * If the input file character encoding was supplied,
		 * we set it accordingly.
		 */
		tfsw->fromcode = input_file_encoding;
	}
	/*
	 * We check the current size of the character_type
	 * and decide which internal character encoding to use.
	 */
	if (character_type_size == 1) {
		/*
		 * we can not use Unicode, so by default we stick
		 * to the basic ASCII encoding
		 */
		tfsw->tocode = "ASCII";
	} else if ((character_type_size > 1) &&
			(character_type_size < 4)) {
		/*
		 * We can use limited Unicode (Basic Multilingual Plane,
		 * or BMP only). We prefer UCS-2 to UTF-16, because we would
		 * not like to deal with the byte order marks (BOM).
		 */
		/* we suppose we are on the little endian architecture */
		tfsw->tocode = "UCS-2LE";
	} else { /* character_type_size >= 4 */
		/*
		 * We can use full Unicode (all the code points). We prefer
		 * UCS-4 to UTF-32, because we would not like to deal
		 * with the byte order marks (BOM).
		 */
		/* again, we suppose the little endian architecture */
		tfsw->tocode = "UCS-4LE";
	}
	/*
	 * if the internal character encoding was not supplied by the calling
	 * function, we keep the recently determined encoding instead
	 */
	if ((**internal_character_encoding) == '\0') {
		/*
		 * we can safely skip the length test here,
		 * because we know exactly for which strings
		 * it is possible to be pointed to by tfsw->tocode
		 *
		 * publishing the choice of the internal character
		 * encoding outside of this function
		 */
		strcpy((*internal_character_encoding), tfsw->tocode);
	} else { /* the caller has specified the internal character encoding */
		fprintf(stderr,	"Warning:\n========\nWe can not check "
				"whether the provided internal character "
				"encoding ('%s')\nis a single-byte encoding, "
				"variable length encoding or a multi-byte "
				"encoding.\nThe fixed internal character "
				"size is %zu byte(s).\nIf this encoding "
				"has been chosen inappropriately,\n"
				"you might experience wrong interpretation "
				"of characters\nin either of these cases!\n\n",
				(*internal_character_encoding),
				character_type_size);
		tfsw->tocode = (*internal_character_encoding);
	}
	/* we create the desired conversion descriptor */
	if ((tfsw->cd = iconv_open(tfsw->tocode, tfsw->fromcode)) ==
			(iconv_t)(-1)) {
		perror("text_file_open: iconv_open");
		/* resetting the errno */
		errno = 0;
		return (2);
	}
	/*
	 * if the caller did not provide any preference
	 * on the edge label maintenance method
	 */
	if (desired_elm_method == 0) {
		/*
		 * we can use the default size
		 * and therefore we do nothing here
		 */
	/*
	 * otherwise, provided that the caller's choice is valid,
	 * we respect it
	 */
	} else if ((desired_elm_method > 0) && (desired_elm_method < 4)) {
		elm_method = desired_elm_method;
	} else { /* the caller's choice is not valid */
		fprintf(stderr, "text_file_open: The desired value of "
				"the edge label maintenance method (%d)\n"
				"is not valid. Exiting!\n",
				desired_elm_method);
		return (3);
	}
	/*
	 * if the caller did not provide any preference
	 * on the size of a single block in the sliding window
	 */
	if (desired_sw_block_size == 0) {
		/*
		 * we can use the default size
		 * and therefore we do nothing here
		 */
	} else { /* otherwise, we respect the caller's choice */
		sw_block_size = desired_sw_block_size;
	}
	/*
	 * if the caller did not provide any preference
	 * on the active part scale factor
	 */
	if (desired_ap_scale_factor == 0) {
		/*
		 * we can use the default size
		 * and therefore we do nothing here
		 */
	} else { /* otherwise, we respect the caller's choice */
		ap_scale_factor = desired_ap_scale_factor;
	}
	/*
	 * if the caller did not provide any preference
	 * on the sliding window scale factor
	 */
	if (desired_sw_scale_factor == 0) {
		/*
		 * We can use the default value, but we have to evaluate
		 * this expression again, because the 'ap_scale_factor'
		 * might have changed and we also have to take into account
		 * the current edge label maintenance method.
		 */
		if (elm_method == 1) {
			/*
			 * in case we are supposed to use the batch update
			 * as the edge label maintenance method,
			 * we need the sliding window scale factor
			 * to be at least as large as twice the active part
			 * scale factor
			 */
			sw_scale_factor = ap_scale_factor * 2;
			/* check for an overflow */
			if (sw_scale_factor < ap_scale_factor) {
				fprintf(stderr, "text_file_open: The value "
						"of sw_scale_factor (%zu) "
						"has overflown!\n",
						sw_scale_factor);
				return (4);
			}
		} else {
			sw_scale_factor = ap_scale_factor + 2;
			/* check for an overflow */
			if (sw_scale_factor < ap_scale_factor) {
				fprintf(stderr, "text_file_open: The value "
						"of sw_scale_factor (%zu) "
						"has overflown!\n",
						sw_scale_factor);
				return (5);
			}
		}
	} else if (desired_sw_scale_factor <= ap_scale_factor) {
		/*
		 * if the desired sliding window scale factor is too small,
		 * we adjust its size and print the warning message
		 */
		if (elm_method == 1) {
			sw_scale_factor = ap_scale_factor * 2;
		} else {
			sw_scale_factor = ap_scale_factor + 1;
		}
		/* check for an overflow */
		if (sw_scale_factor == (size_t)(0)) {
			fprintf(stderr, "text_file_open: The value of "
					"sw_scale_factor (%zu) has "
					"overflown!\n", sw_scale_factor);
			return (6);
		}
		fprintf(stderr, "text_file_open: The desired sliding window "
				"scale factor (%zu)\nis too small! "
				"Adjusting its size to %zu.\n",
				desired_sw_scale_factor, sw_scale_factor);
	} else { /* otherwise, we try to respect the caller's choice */
		if (elm_method == 1) {
			if (desired_sw_scale_factor < ap_scale_factor * 2) {
				sw_scale_factor = ap_scale_factor * 2;
				/* check for an overflow */
				if (sw_scale_factor < ap_scale_factor) {
					fprintf(stderr, "text_file_open: "
							"The value of "
							"sw_scale_factor "
							"(%zu) has "
							"overflown!\n",
							sw_scale_factor);
					return (7);
				}
				fprintf(stderr, "text_file_open: The desired "
						"sliding window scale factor "
						"(%zu)\nis too small! "
						"Adjusting its size to %zu.\n",
						desired_sw_scale_factor,
						sw_scale_factor);
			} else {
				sw_scale_factor = desired_sw_scale_factor;
			}
		} else {
			sw_scale_factor = desired_sw_scale_factor;
		}
	}
	/*
	 * we reset the total number of bytes
	 * allocated for the sliding window
	 */
	tfsw->bytes_allocated = 0;
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(tfsw->text_window);
	tfsw->text_window = NULL;
	/*
	 * We want to allocate all the memory necessary
	 * for all parts of the sliding window. Additional memory
	 * is allocated for the unused 0.th character.
	 */
	tfsw->text_window = calloc(sw_block_size * sw_scale_factor + 1,
			character_type_size);
	if (tfsw->text_window == NULL) {
		perror("text_file_open: calloc(text_window)");
		/* resetting the errno */
		errno = 0;
		return (8);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	tfsw->bytes_allocated +=
		(sw_block_size * sw_scale_factor + 1) *
		character_type_size;
	/*
	 * we store the number of usable characters rather than
	 * the number of allocated characters
	 */
	tfsw->total_window_size = sw_block_size * sw_scale_factor;
	tfsw->max_ap_window_size = sw_block_size * ap_scale_factor;
	/*
	 * we reset the additional number of bytes
	 * allocated for the sliding window
	 */
	tfsw->additional_bytes_allocated = 0;
	/*
	 * we set the size of the conversion buffer to the number
	 * of bytes that a single block in the sliding window occupies
	 */
	tfsw->conversion_buffer_size = sw_block_size * character_type_size;
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(tfsw->conversion_buffer);
	tfsw->conversion_buffer = NULL;
	/* we allocate the memory for the conversion buffer */
	tfsw->conversion_buffer = malloc(tfsw->conversion_buffer_size);
	if (tfsw->conversion_buffer == NULL) {
		perror("text_file_open: malloc(conversion_buffer)");
		/* resetting the errno */
		errno = 0;
		return (9);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	tfsw->additional_bytes_allocated += tfsw->conversion_buffer_size;
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(tfsw->sw_block_flags);
	tfsw->sw_block_flags = NULL;
	/* we allocate the memory for the sliding window block flags */
	tfsw->sw_block_flags = calloc(sw_scale_factor, sizeof (int));
	if (tfsw->sw_block_flags == NULL) {
		perror("text_file_open: calloc(sw_block_flags)");
		/* resetting the errno */
		errno = 0;
		return (10);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	tfsw->additional_bytes_allocated += sw_scale_factor * sizeof (int);
	tfsw->bytes_allocated += tfsw->additional_bytes_allocated;
	/*
	 * initializing the remaining members
	 * of the text_file_sliding_window struct
	 */
	tfsw->blocks_read = 0;
	tfsw->bytes_read = 0;
	tfsw->characters_read = 0;
	/* the current size of the active part of the sliding window is zero */
	tfsw->ap_window_size = 0;
	tfsw->ap_window_begin = 1;
	tfsw->ap_window_end = 1;
	tfsw->ap_scale_factor = ap_scale_factor;
	tfsw->sw_scale_factor = sw_scale_factor;
	/*
	 * For the better clarity, we want to have another variable,
	 * which explicitly holds the number of blocks in the sliding window.
	 */
	tfsw->sw_blocks = sw_scale_factor;
	/*
	 * The indices of the most recently read block and the most recently
	 * processed block are set to the last block of the sliding window.
	 * It is a simple way to make the next reading or processing
	 * functions to start at the first block of the sliding window.
	 * This approach, however, posesses one serious disadvantage.
	 * It is the inability to tell whether the reading or processing
	 * of the sliding window have already started or it haven't.
	 * This needs to be monitored by the user's own means.
	 */
	tfsw->sw_mrp_block = sw_scale_factor - 1;
	tfsw->sw_mrr_block = sw_scale_factor - 1;
	tfsw->sw_block_size = sw_block_size;
	tfsw->inbuf = NULL;
	tfsw->outbuf = NULL;
	tfsw->inbytesleft = 0;
	tfsw->outbytesleft = 0;
	tfsw->elm_method = elm_method;
	/*
	 * we do not intend to use tfsw->text_window[0],
	 * that's why we fill it with "blank" (space) character
	 */
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	tfsw->text_window[0] = L' ';
#else
	tfsw->text_window[0] = ' ';
#endif
	printf("The file '%s' has been opened successfully!\n",
			tfsw->file_name);
	printf("Selected input file encoding: '%s'\n", tfsw->fromcode);
	printf("Selected internal character encoding: '%s'\n\n", tfsw->tocode);
	if (verbosity_level > 0) {
		printf("The block size: %zu cells of %zu bytes (",
				tfsw->sw_block_size, character_type_size);
		print_human_readable_size(stdout,
				tfsw->sw_block_size * character_type_size);
		printf(")\n");
	}
	if (verbosity_level > 1) {
		printf("The maximum size of the active part "
				"of the sliding window:\n"
				"%zu cells of %zu bytes (",
				tfsw->max_ap_window_size,
				character_type_size);
		print_human_readable_size(stdout,
				tfsw->max_ap_window_size *
				character_type_size);
		printf(")\nThe total sliding window size: "
				"%zu cells of %zu bytes (",
				tfsw->total_window_size,
				character_type_size);
		print_human_readable_size(stdout,
				tfsw->total_window_size *
				character_type_size);
		printf(")\n");
	}
	if (verbosity_level > 0) {
		printf("Active part scale factor: %zu\n", ap_scale_factor);
		printf("Sliding window scale factor: %zu\n", sw_scale_factor);
	}
	if (verbosity_level > 1) {
		printf("Additional amount of memory allocated\n"
				"for the sliding window: "
				"%zu bytes (",
				tfsw->additional_bytes_allocated);
		print_human_readable_size(stdout,
				tfsw->additional_bytes_allocated);
		printf(")\n");
	}
	if (verbosity_level > 0) {
		printf("\n");
	}
	return (0);
}

/**
 * A function which closes the file descriptor and the iconv conversion
 * descriptor associated with the input file.
 * It also deallocates all the additional memory allocated
 * for the sliding window.
 *
 * @param
 * verbosity_level	the desired level of messaging verbosity
 * @param
 * tfsw	the text file sliding window, which needs to be closed
 *
 * @return	If this function call is successful, it returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int text_file_close (const int verbosity_level,
		text_file_sliding_window *tfsw) {
	const size_t bytes_allocated = tfsw->bytes_allocated;
	if (verbosity_level > 0) {
		printf("\nTrying to close the text file '%s'\n"
				"and clean the sliding window "
				"associated with it.\n", tfsw->file_name);
	}
	/*
	 * At first, we free the memory for the conversion buffer.
	 *
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(tfsw->conversion_buffer);
	tfsw->conversion_buffer = NULL;
	tfsw->additional_bytes_allocated -= tfsw->conversion_buffer_size;
	tfsw->bytes_allocated -= tfsw->conversion_buffer_size;
	/* then we free the memory for the sliding window itself */
	free(tfsw->text_window);
	tfsw->text_window = NULL;
	tfsw->bytes_allocated -= (tfsw->total_window_size + 1) *
		character_type_size;
	/* then we free the memory for the block flags */
	free(tfsw->sw_block_flags);
	tfsw->sw_block_flags = NULL;
	tfsw->additional_bytes_allocated -=
		tfsw->sw_scale_factor * sizeof (int);
	tfsw->bytes_allocated -= tfsw->sw_scale_factor * sizeof (int);
	if (tfsw->additional_bytes_allocated > 0) {
		fprintf(stderr, "text_file_close: After the deallocations,\n"
				"additional_bytes_allocated (%zu) "
				"remains positive!\n",
				tfsw->additional_bytes_allocated);
		return (1);
	}
	if (tfsw->bytes_allocated > 0) {
		fprintf(stderr, "text_file_close: After the deallocations,\n"
				"bytes_allocated (%zu) remains positive!\n",
				tfsw->bytes_allocated);
		return (2);
	}
	/* next, we delete the conversion descriptor used by the iconv */
	if (iconv_close(tfsw->cd) == (-1)) {
		perror("text_file_close: iconv_close");
		/* resetting the errno */
		errno = 0;
		return (3);
	}
	tfsw->cd = NULL;
	/*
	 * and finally we close the file descriptor
	 * used for reading the input file
	 */
	if (close(tfsw->fd) == -1) {
		perror("text_file_close: close");
		/* resetting the errno */
		errno = 0;
		return (4);
	}
	tfsw->fd = 0;
	if (verbosity_level > 0) {
		printf("Successfully closed!\n\n");
	}
	printf("The sliding window statistics:\n"
		"------------------------------\n");
	printf("Block size: %zu characters of %zu bytes\n"
			"            (totalling %zu bytes, ",
			tfsw->sw_block_size, character_type_size,
			tfsw->sw_block_size * character_type_size);
	print_human_readable_size(stdout,
			tfsw->sw_block_size * character_type_size);
	printf(")\nActive part scale factor: %zu\n", tfsw->ap_scale_factor);
	printf("Sliding window scale factor: %zu\n", tfsw->sw_scale_factor);
	if (verbosity_level > 1) {
		printf("Maximum size of the active part:\n"
				"%zu characters of %zu bytes "
				"(totalling %zu bytes, ",
				tfsw->max_ap_window_size, character_type_size,
				tfsw->max_ap_window_size * character_type_size);
		print_human_readable_size(stdout,
				tfsw->max_ap_window_size * character_type_size);
		printf(")\nTotal size of the sliding window:\n"
				"%zu characters of %zu bytes "
				"(totalling %zu bytes, ",
				tfsw->total_window_size, character_type_size,
				tfsw->total_window_size * character_type_size);
		print_human_readable_size(stdout,
				tfsw->total_window_size * character_type_size);
		printf(")\n");
	}
	if (verbosity_level > 0) {
		printf("\nThe total amount of memory allocated\n"
				"for the sliding window: %zu bytes (",
				bytes_allocated);
		print_human_readable_size(stdout,
				bytes_allocated);
		printf(")\n");
	}
	if (verbosity_level > 1) {
		printf("\nTotal number of blocks read: %zu\n",
				tfsw->blocks_read);
		printf("Total disk space read for the text: %zu bytes (",
				tfsw->bytes_read);
		print_human_readable_size(stdout, tfsw->bytes_read);
		printf(")\n");
	}
	if (verbosity_level > 0) {
		printf("Total number of characters obtained "
				"from the input file: %zu\n",
				tfsw->characters_read);
		printf("Average character size: %2.3f bytes\n",
				(double)(tfsw->bytes_read) /
				(double)(tfsw->characters_read));
	}
	if (verbosity_level > 1) {
		printf("\nThe active part can hold "
				"%2.2f%% of the text\n", 100 *
				(double)(tfsw->max_ap_window_size) /
				(double)(tfsw->characters_read));
		printf("The whole sliding window can hold "
				"%2.2f%% of the text\n", 100 *
				(double)(tfsw->total_window_size) /
				(double)(tfsw->characters_read));
	}
	/*
	 * we should, of course, also reset the other variables
	 * to their initial values
	 */
	tfsw->blocks_read = 0;
	tfsw->bytes_read = 0;
	tfsw->characters_read = 0;
	tfsw->total_window_size = 0;
	tfsw->max_ap_window_size = 0;
	tfsw->ap_window_size = 0;
	tfsw->ap_window_begin = 0;
	tfsw->ap_window_end = 0;
	tfsw->ap_scale_factor = 0;
	tfsw->sw_scale_factor = 0;
	tfsw->sw_blocks = 0;
	tfsw->sw_mrp_block = 0;
	tfsw->sw_mrr_block = 0;
	tfsw->sw_block_size = 0;
	tfsw->file_name = NULL;
	tfsw->fromcode = NULL;
	tfsw->tocode = NULL;
	tfsw->conversion_buffer_size = 0;
	tfsw->inbuf = NULL;
	tfsw->outbuf = NULL;
	tfsw->inbytesleft = 0;
	tfsw->outbytesleft = 0;
	return (0);
}

/**
 * A function which tries to read the desired number of blocks
 * from the previously specified input file
 * into the oldest part of the provided sliding window.
 * This function adjusts the total numbers of blocks,
 * characters and bytes read, which are stored
 * in the text_file_sliding_window struct.
 * It also adjusts the index of the most recently read block.
 * However, it does not set the appropriate sliding window block flags,
 * because, it could lead to race conditions.
 * In threaded environment, these flags need to be changed exclusively
 * under the protection of the mutex.
 *
 * @param
 * blocks_to_read	the desired number of blocks to read
 * @param
 * blocks_read	When this function successfully finishes,
 * 		this variable will be set to the final number of blocks,
 * 		which have been entirely read during this function call.
 * 		If the input file has been completely read during
 * 		this function call then the block containing
 * 		the last characters from the input file
 * 		is also considered to be the entirely read block,
 * 		regardless of how many characters inside it
 * 		have actually been replaced.
 * @param
 * characters_read	when this function successfully finishes,
 * 			this variable will be set to the final
 * 			number of characters in the sliding window,
 * 			which have in fact been successfully replaced
 * 			during this function call
 * @param
 * bytes_read	When this function successfully finishes,
 * 		this variable will be set to the final number of bytes,
 * 		which have been read during this function call.
 * 		Note that due to the buffering, the final number of bytes
 * 		read during this function call might be different
 * 		than the final number of bytes converted to the characters
 * 		during this function call.
 * @param
 * tfsw		the sliding window which will be updated
 * 		with the new characters coming
 * 		from the previously specified input file
 *
 * @return	If the reading was successful, this function returns 0.
 * 		If the reading was partially successful (e.g. it had to stop,
 * 		because the input file does not have enough unread bytes left),
 * 		one (1) is returned.
 * 		Otherwise, a positive error number greater than one is returned.
 */
int text_file_read_blocks (size_t blocks_to_read,
		size_t *blocks_read,
		size_t *characters_read,
		size_t *bytes_read,
		text_file_sliding_window *tfsw) {
	/* the return value of the function text_file_read_part */
	int retval = 0;
	/* the index of the first block, which we will try to read */
	size_t starting_block = 0;
	/*
	 * the variable used to store the index of a block,
	 * which is just after the last block, which we will try to read
	 */
	size_t ending_block = 0;
	/*
	 * the number of blocks read during one call
	 * to the text_file_read_part function
	 */
	size_t partial_blocks_read = 0;
	/*
	 * the number of characters read and converted during one call
	 * to the text_file_read_part function
	 */
	size_t partial_characters_converted = 0;
	/*
	 * the number of bytes read during one call
	 * to the text_file_read_part function
	 */
	size_t partial_bytes_read = 0;
	/*
	 * an auxiliary index of the block, used when computing
	 * the total number of blocks read during this function call
	 */
	size_t current_block = 0;
	/* initializing the values of the output variables to zeros */
	(*blocks_read) = 0;
	(*characters_read) = 0;
	(*bytes_read) = 0;
	/* if the desired number of blocks to read is too small */
	if (blocks_to_read == 0) {
		fprintf(stderr, "text_file_read_blocks: The provided number\n"
				"of blocks to read must be positive!\n");
		return (2);
	/* if the desired number of blocks to read is too high */
	} else if (blocks_to_read > tfsw->sw_blocks) {
		fprintf(stderr, "text_file_read_blocks: The provided number\n"
				"of blocks to read (%zu)\n"
				"is higher than the total "
				"number of blocks (%zu)!\n",
				blocks_to_read, tfsw->sw_blocks);
		return (3);
	}
	/* the blocks are numbered starting from zero */
	starting_block = tfsw->sw_mrr_block + 1;
	if (starting_block == tfsw->sw_blocks) {
		starting_block = 0;
	}
	ending_block = starting_block + blocks_to_read;
	if (ending_block == tfsw->sw_blocks) {
		ending_block = 0;
	}
	/*
	 * The typedef to char* is necessary,
	 * because the tfsw->text_window might be of the type wchar_t*
	 */
	tfsw->outbuf = (char *)(tfsw->text_window + 1 +
			starting_block * tfsw->sw_block_size *
			character_type_size);
	/* filling in the maximum number of bytes to write in the first part */
	if (starting_block < ending_block) {
		tfsw->outbytesleft = (ending_block - starting_block) *
			tfsw->sw_block_size * character_type_size;
	} else { /* starting_block >= ending_block */
		tfsw->outbytesleft = (tfsw->sw_blocks - starting_block) *
			tfsw->sw_block_size * character_type_size;
	}
	/*
	 * The first part
	 *
	 * Reading the blocks from the oldest block up to the last block
	 * of the sliding window or until the ending block or until
	 * the end of the text, whichever comes first.
	 *
	 * the first part is always necessary
	 */
	if ((retval = text_file_read_part(
				&partial_characters_converted,
				&partial_bytes_read, tfsw)) == (-1)) {
		tfsw->characters_read += partial_characters_converted;
		tfsw->bytes_read += partial_bytes_read;
		(*characters_read) = partial_characters_converted;
		(*bytes_read) = partial_bytes_read;
		if (partial_characters_converted == 0) {
			partial_blocks_read = 0;
			/*
			 * as no blocks have been read, we need not
			 * to adjust the index
			 * of the most recently read block
			 */
		} else { /* partial_characters_converted > 0 */
			/* adjusting the number of blocks read */
			for (current_block = starting_block;
					current_block <
					tfsw->sw_blocks;
					++current_block) {
				if (partial_characters_converted >
						tfsw->sw_block_size) {
					partial_characters_converted -=
						tfsw->sw_block_size;
					++partial_blocks_read;
				} else {
					/*
					 * the last successfully
					 * read block, which may not
					 * be entirely utilized
					 */
					++partial_blocks_read;
					break;
				}
			}
			tfsw->sw_mrr_block = current_block;
		}
		tfsw->blocks_read += partial_blocks_read;
		(*blocks_read) = partial_blocks_read;
		return (1); /* partial success */
	} else if (retval > 0) { /* and retval != (-1) */
		fprintf(stderr,	"text_file_read: the first call "
				"to the function text_file_read_part "
				"failed!\n");
		return (4);
	} /* the end of the first part */
	/* setting the current number of blocks read */
	if (starting_block < ending_block) {
		(*blocks_read) = ending_block - starting_block;
		/*
		 * adjusting the index of the current
		 * and the most recently read block
		 */
		current_block = ending_block - 1;
	} else { /* starting_block >= ending_block */
		(*blocks_read) = tfsw->sw_blocks - starting_block;
		/*
		 * adjusting the index of the current
		 * and the most recently read block
		 */
		current_block = tfsw->sw_blocks - 1;
	}
	/* setting the current number of characters and bytes read */
	(*characters_read) = partial_characters_converted;
	(*bytes_read) = partial_bytes_read;
	/*
	 * The second part
	 *
	 * Reading the blocks from the beginning
	 * of the sliding window until the ending block.
	 *
	 * we at first check whether the second part is necessary
	 */
	if (starting_block < ending_block) {
		/* the second part is not necessary */
	} else if (ending_block == 0) {
		/*
		 * again, the second part is not necessary
		 * (for slightly different reason)
		 */
	} else { /* the second part is necessary */
		/*
		 * The typedef to char* is necessary,
		 * because the tfsw->text_window
		 * might be of the type wchar_t*
		 */
		tfsw->outbuf = (char *)(tfsw->text_window + 1);
		/*
		 * at this point, tfsw->outbytesleft is zero, so we set it
		 * to the maximum number of bytes to write in this part
		 */
		tfsw->outbytesleft = ending_block * tfsw->sw_block_size *
			character_type_size;
		if ((retval = text_file_read_part(
					&partial_characters_converted,
					&partial_bytes_read, tfsw)) == (-1)) {
			(*characters_read) += partial_characters_converted;
			(*bytes_read) += partial_bytes_read;
			tfsw->characters_read += (*characters_read);
			tfsw->bytes_read += (*bytes_read);
			if (partial_characters_converted == 0) {
				partial_blocks_read = 0;
				tfsw->sw_mrr_block = tfsw->sw_blocks - 1;
			} else { /* partial_characters_converted > 0 */
				/* adjusting the number of blocks read */
				for (current_block = 0;
						current_block < ending_block;
						++current_block) {
					if (partial_characters_converted >
							tfsw->sw_block_size) {
						partial_characters_converted -=
							tfsw->sw_block_size;
						++partial_blocks_read;
					} else {
						/*
						 * the last successfully
						 * read block, which may not
						 * be entirely utilized
						 */
						++partial_blocks_read;
						break;
					}
				}
				tfsw->sw_mrr_block = current_block;
			}
			(*blocks_read) += partial_blocks_read;
			tfsw->blocks_read += (*blocks_read);
			return (1); /* partial success */
		} else if (retval > 0) { /* and retval != (-1) */
			fprintf(stderr,	"text_file_read: the second call "
					"to the function text_file_read_part "
					"failed!\n");
			return (5);
		}
		/*
		 * adjusting the index of the current
		 * and the most recently read block
		 */
		current_block = ending_block - 1;
		/*
		 * setting the current number of blocks
		 * read during the second part
		 */
		partial_blocks_read = ending_block;
		/*
		 * updating the current numbers
		 * of blocks, characters and bytes read
		 */
		(*blocks_read) += partial_blocks_read;
		(*characters_read) += partial_characters_converted;
		(*bytes_read) += partial_bytes_read;
	} /* the end of the second part */
	/*
	 * updating the current numbers
	 * of blocks, characters and bytes read,
	 * which are stored inside the text_file_sliding_window struct
	 */
	tfsw->blocks_read += (*blocks_read);
	tfsw->characters_read += (*characters_read);
	tfsw->bytes_read += (*bytes_read);
	/* setting the currently valid number of the most recently read block */
	tfsw->sw_mrr_block = current_block;
	/*
	 * At this point, we just check whether there are some bytes left
	 * in the iconv output buffer.
	 */
	if (tfsw->outbytesleft > 0) {
		fprintf(stderr,	"text_file_read: There are still %zu bytes "
				"left\nin the iconv output buffer!\n",
				tfsw->outbytesleft);
		return (6);
	}
	/*
	 * If there are still some bytes left in the iconv input buffer inbuf,
	 * it is not a very big deal. We just leave them as they are,
	 * available for the next call of this function.
	 */
	return (0); /* success */
}

/* printing functions */

/**
 * A function which prints the edge from parent to the child
 * within a suffix tree over a sliding window in a human readable format.
 *
 * @param
 * stream	the FILE * type stream to which the traversal progress
 * 		will be written
 * @param
 * print_terminating_character	If this variable evaluates to true,
 * 				this function will print the dollar sign ($)
 * 				instead of the last non-NULL character
 * 				at the provided edge.
 * @param
 * parent	the node where the given edge starts
 * @param
 * child	the node where the given edge ends
 * @param
 * childs_suffix_link	the suffix link starting from the child
 * @param
 * parents_depth	the depth of the parent node
 * @param
 * childs_depth		the depth of the child node
 * @param
 * log10bn	A ceiling of base 10 logarithm of the number
 * 		of branching nodes. It will be used for printing alignment.
 * @param
 * log10l	A ceiling of base 10 logarithm of the number
 * 		of leaves. It will be used for printing alignment.
 * @param
 * childs_offset	The position in the sliding window of the first letter
 * 			of the leftmost "branching occurrence" of the string
 * 			composed of the letters on the path
 * 			from the root to the child.
 * @param
 * tfsw		the actual sliding window containing
 * 		the characters of the edge to be printed
 *
 * @return	If we could successfully print the edge, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
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
		const text_file_sliding_window *tfsw) {
	/*
	 * The format string buffer. Its length should never exceed 128.
	 * All its characters (not only the first one) are initialized
	 * to zero automatically.
	 */
	static char fs_buffer[128] = {0};
	/*
	 * The text string buffer. Its length should never exceed 512.
	 * It has space for at least 128 4-byte UTF-8 characters,
	 * which should be sufficient for our purposes, as the maximum
	 * number of bytes used by any single UTF-8 character is 4.
	 * All its characters (not only the first one) are initialized
	 * to zero automatically.
	 */
	static char text_buffer[512] = {0};
	/*
	 * By default, we suppose that the current locale supports
	 * the character encoding UTF-8 and that's why we print
	 * the underlying text of the suffix tree using this encoding.
	 */
	static char *tocode = "UTF-8";
	/* the length of the text which forms the parent->child edge label */
	size_t text_length = childs_depth - parents_depth;
	/* the conversion descriptor used by the iconv */
	iconv_t cd = NULL; /* iconv_t is just a typedef for void* */
	/* the variables used by the iconv */
	char *inbuf = NULL;
	char *outbuf = NULL;
	size_t inbuf_offset = 0;
	size_t incharsleft = 0;
	size_t inbytesleft = 0;
	size_t outbytesleft = 0;
	/* the return value of the iconv */
	size_t retval = 0;
	if (childs_depth < parents_depth) {
		fprintf(stderr,	"Error: Something went wrong.\n"
				"The child (%d) has the depth of %u,\n"
				"but its parent (%d) has the depth "
				"of %u,\nwhich should never happen!\n",
				child, childs_depth, parent, parents_depth);
		fprintf(stderr,	"The traversal of this branch "
				"is terminated here.\n");
		return (1);
	}
	/* at first, we can safely print the parent */
	if (parent == 0) {
		fprintf(stream, "P(?)[%u]", parents_depth);
	} else {
		fprintf(stream, "P(%0*d)[%u]", (int)(log10bn), parent,
				parents_depth);
	}
	/* we create the desired conversion descriptor */
	if ((cd = iconv_open(tocode, tfsw->tocode)) ==
			(iconv_t)(-1)) {
		perror("stsw_print_edge: iconv_open");
		/* resetting the errno */
		errno = 0;
		return (2);
	}
	/* we print the edge label */
	if (text_length < 33) {
		inbuf_offset = childs_offset + parents_depth;
		if (inbuf_offset > tfsw->total_window_size) {
			inbuf_offset -= tfsw->total_window_size;
		}
		if (print_terminating_character == 0) {
			sprintf(fs_buffer, "--\"%%s\"(%zu)-->", text_length);
			incharsleft = text_length;
		} else {
			/*
			 * we do not want the terminating character
			 * to be printed from the memory as is, so
			 * we print the dollar sign ($) instead
			 */
			sprintf(fs_buffer, "--\"%%s$\"(%zu)-->", text_length);
			incharsleft = text_length - 1;
		}
		if (inbuf_offset + incharsleft - 1 >
				tfsw->total_window_size) {
			inbytesleft = (tfsw->total_window_size -
				inbuf_offset + 1) * character_type_size;
		} else {
			inbytesleft = incharsleft * character_type_size;
		}
		inbuf = (char *)(tfsw->text_window + inbuf_offset);
		outbuf = text_buffer;
		outbytesleft = 511;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("stsw_print_edge: iconv 1");
			/* resetting the errno */
			errno = 0;
			return (3);
		}
		if (inbuf_offset + incharsleft - 1 >
				tfsw->total_window_size) {
			inbytesleft = (inbuf_offset + incharsleft -
					tfsw->total_window_size - 1) *
				character_type_size;
			inbuf = (char *)(tfsw->text_window + 1);
			retval = iconv(cd, &inbuf, &inbytesleft,
					&outbuf, &outbytesleft);
			/* if the iconv has encountered an error */
			if (inbytesleft > 0 || retval != 0) {
				perror("stsw_print_edge: iconv 2");
				/* resetting the errno */
				errno = 0;
				return (4);
			}
		}
		/* we make sure that the string is safely printable */
		(*outbuf) = 0;
		fprintf(stream, fs_buffer, text_buffer);
	} else { /* text_length >= 33 */
		inbuf_offset = childs_offset + parents_depth;
		if (inbuf_offset > tfsw->total_window_size) {
			inbuf_offset -= tfsw->total_window_size;
		}
		incharsleft = 15;
		if (inbuf_offset + incharsleft - 1 >
				tfsw->total_window_size) {
			inbytesleft = (tfsw->total_window_size -
				inbuf_offset + 1) * character_type_size;
		} else {
			inbytesleft = incharsleft * character_type_size;
		}
		inbuf = (char *)(tfsw->text_window + inbuf_offset);
		outbuf = text_buffer;
		outbytesleft = 255;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("stsw_print_edge: iconv 3");
			/* resetting the errno */
			errno = 0;
			return (5);
		}
		if (inbuf_offset + incharsleft - 1 >
				tfsw->total_window_size) {
			inbytesleft = (inbuf_offset + incharsleft -
					tfsw->total_window_size - 1) *
				character_type_size;
			inbuf = (char *)(tfsw->text_window + 1);
			retval = iconv(cd, &inbuf, &inbytesleft,
					&outbuf, &outbytesleft);
			/* if the iconv has encountered an error */
			if (inbytesleft > 0 || retval != 0) {
				perror("stsw_print_edge: iconv 4");
				/* resetting the errno */
				errno = 0;
				return (6);
			}
		}
		/* we make sure that the string is safely printable */
		(*outbuf) = 0;
		inbuf_offset = childs_offset + childs_depth - 15;
		if (inbuf_offset > tfsw->total_window_size) {
			inbuf_offset -= tfsw->total_window_size;
		}
		if (print_terminating_character == 0) {
			sprintf(fs_buffer, "--\"%%s...%%s\"(%zu)-->",
					text_length);
			incharsleft = 15;
		} else {
			/*
			 * we do not want the terminating character
			 * to be printed from the memory as is, so
			 * we print the dollar sign ($) instead
			 */
			sprintf(fs_buffer, "--\"%%s...%%s$\"(%zu)-->",
					text_length);
			incharsleft = 14;
		}
		if (inbuf_offset + incharsleft - 1 >
				tfsw->total_window_size) {
			inbytesleft = (tfsw->total_window_size -
				inbuf_offset + 1) * character_type_size;
		} else {
			inbytesleft = incharsleft * character_type_size;
		}
		inbuf = (char *)(tfsw->text_window + inbuf_offset);
		outbuf = text_buffer + 256;
		outbytesleft = 255;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("stsw_print_edge: iconv 5");
			/* resetting the errno */
			errno = 0;
			return (7);
		}
		if (inbuf_offset + incharsleft - 1 >
				tfsw->total_window_size) {
			inbytesleft = (inbuf_offset + incharsleft -
					tfsw->total_window_size - 1) *
				character_type_size;
			inbuf = (char *)(tfsw->text_window + 1);
			retval = iconv(cd, &inbuf, &inbytesleft,
					&outbuf, &outbytesleft);
			/* if the iconv has encountered an error */
			if (inbytesleft > 0 || retval != 0) {
				perror("stsw_print_edge: iconv 6");
				/* resetting the errno */
				errno = 0;
				return (8);
			}
		}
		/* we make sure that the string is safely printable */
		(*outbuf) = 0;
		fprintf(stream, fs_buffer, text_buffer, text_buffer + 256);
	}
	/* now we can safely print the child */
	if (child == 0) {
		fprintf(stream, "C(?)[%u]", childs_depth);
	} else if (child > 0) {
		fprintf(stream, "C(%0*d)[%u]", (int)(log10bn), child,
				childs_depth);
	} else { /* child < 0 => child is a leaf */
		fprintf(stream, "C(%0*d)[%u]", (int)(log10l), child,
				childs_depth);
	}
	/* and finally, we can optionally print the suffix link */
	if (childs_suffix_link == 0) {
		fprintf(stream, "\n");
	} else {
		fprintf(stream, "{%0*d}\n", (int)(log10bn),
				childs_suffix_link);
	}
	/* we delete the conversion descriptor used by the iconv */
	if (iconv_close(cd) == (-1)) {
		perror("stsw_print_edge: iconv_close");
		/* resetting the errno */
		errno = 0;
		return (9);
	}
	if (errno != 0) {
		perror("stsw_print_edge: printf");
		/* resetting the errno */
		errno = 0;
		return (10);
	}
	return (0);
}

/**
 * A function which prints the provided sliding window
 * into the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the provided
 * 		sliding window will be printed
 * @param
 * tfsw		the actual sliding window to be printed
 *
 * @return	If this function call is successful, this function returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int stsw_print_sliding_window (FILE *stream,
		text_file_sliding_window *tfsw) {
	/*
	 * The text string buffer. Its length should never exceed 512.
	 * It has space for at least 128 4-byte UTF-8 characters,
	 * which should be sufficient for our purposes, as the maximum
	 * number of bytes used by any single UTF-8 character is 4.
	 * All its characters (not only the first one) are initialized
	 * to zero automatically.
	 */
	static char text_buffer[512] = {0};
	/*
	 * By default, we suppose that the current locale supports
	 * the character encoding UTF-8 and that's why we print
	 * the underlying text of the suffix tree using this encoding.
	 */
	static char *tocode = "UTF-8";
	/* the conversion descriptor used by the iconv */
	iconv_t cd = NULL; /* iconv_t is just a typedef for void* */
	/* the variables used by the iconv */
	char *inbuf = NULL;
	char *outbuf = NULL;
	size_t inbytesleft = 0;
	size_t outbytesleft = 0;
	/* the return value of the iconv */
	size_t retval = 0;
	/* we create the desired conversion descriptor */
	if ((cd = iconv_open(tocode, tfsw->tocode)) ==
			(iconv_t)(-1)) {
		perror("stsw_print_sliding_window: iconv_open");
		return (1);
	}
	/*
	 * The typedef to char* is necessary,
	 * because the tfsw->text_window might be of the type wchar_t*
	 */
	inbuf = (char *)(tfsw->text_window);
	inbytesleft = tfsw->total_window_size * character_type_size;
	/* while there are unconverted bytes in the input buffer */
	while (inbytesleft > 0) {
		outbuf = text_buffer;
		/*
		 * the last character of the buffer is reserved
		 * for the terminating NULL character, which is
		 * never changed after the initialization
		 */
		outbytesleft = 511;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (retval == (size_t)(-1)) {
			/*
			 * the only allowed case when iconv can return
			 * an error is when the output buffer
			 * is fully exhausted
			 */
			if (errno != E2BIG) {
				perror("stsw_print_sliding_window: iconv");
				return (2);
			}
			/* resetting the errno */
			errno = 0;
			/* we can safely print the whole output buffer */
			fprintf(stream, "%s", text_buffer);
		} else if (retval > 0) {
			fprintf(stderr, "stsw_print_sliding_window: iconv "
					"has converted\n%zu characters "
					"in a nonreversible way!", retval);
			return (3);
		} else { /* the output buffer has not been entirely utilized */
			/*
			 * we make sure we print only
			 * the recently read part of the output buffer
			 */
			(*outbuf) = 0;
			fprintf(stream, "%s", text_buffer);
		}
	}
	/* we delete the conversion descriptor used by the iconv */
	if (iconv_close(cd) == (-1)) {
		perror("stsw_print_sliding_window: iconv_close");
		/* resetting the errno */
		errno = 0;
		return (4);
	}
	if (errno != 0) {
		perror("stsw_print_sliding_window: fprintf");
		/* resetting the errno */
		errno = 0;
		return (5);
	}
	return (0);
}

/**
 * A function which prints the memory usage statistics
 * for the suffix tree over a sliding window.
 *
 * @param
 * verbosity_level	the desired level of messaging verbosity
 * @param
 * length	the total number of characters in the text underlying
 * 		the sliding window used by the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * max_ap_window_size	the maximum size of the active part
 * 			of the sliding window
 * @param
 * min_edges		the minimum number of edges in the suffix tree,
 * 			during the maximum utilization of the sliding window
 * @param
 * min_edges_ap_bti	The text index of the beginning of the active part
 * 			of the sliding window (or ap_bti),
 * 			at the moment when there has been the minimum number
 * 			of edges used while the utilization
 * 			of the sliding window has been the highest.
 *
 * 			The text index is the index of a character
 * 			in the whole text that has already been processed
 * 			in the sliding window.
 * 			It starts at 1 and continues up to the number
 * 			of processed characters.
 * @param
 * avg_edges		the average number of edges in the suffix tree,
 * 			during the maximum utilization of the sliding window
 * @param
 * max_edges		the maximum number of edges in the suffix tree,
 * 			during the maximum utilization of the sliding window
 * @param
 * max_edges_ap_bti	The text index of the beginning of the active part
 * 			of the sliding window (or ap_bti),
 * 			at the moment when there has been the maximum number
 * 			of edges used while the utilization
 * 			of the sliding window has been the highest.
 * @param
 * min_branching_nodes	the minimum number of branching nodes
 * 			in the suffix tree, during the maximum utilization
 * 			of the sliding window
 * @param
 * min_brn_ap_bti	The text index of the beginning of the active part
 * 			of the sliding window (or ap_bti),
 * 			at the moment when there has been the minimum number
 * 			of branching nodes used while the utilization
 * 			of the sliding window has been the highest.
 * @param
 * avg_branching_nodes	the average number of branching nodes
 * 			in the suffix tree, during the maximum utilization
 * 			of the sliding window
 * @param
 * max_branching_nodes	the maximum number of branching nodes
 * 			in the suffix tree, during the maximum utilization
 * 			of the sliding window
 * @param
 * max_brn_ap_bti	The text index of the beginning of the active part
 * 			of the sliding window (or ap_bti),
 * 			at the moment when there has been the maximum number
 * 			of branching nodes used while the utilization
 * 			of the sliding window has been the highest.
 * @param
 * tedge_size		the current size of the table tedge
 * @param
 * tbranch_size		the current size of the table tbranch
 * @param
 * leaf_record_size	the size of the leaf record
 * 			in the current implementation of the suffix tree
 * @param
 * edge_record_size	the size of the edge record
 * 			in the current implementation of the suffix tree
 * @param
 * branch_record_size	the size of the branch record
 * 			in the current implementation of the suffix tree
 * @param
 * extra_allocated_memory_size	the size of the additional memory,
 * 				which has been already allocated
 * 				in the current implementation
 * 				of the suffix tree
 * @param
 * extra_used_memory_size	the size of the additional memory,
 * 				which has been already allocated
 * 				in the current implementation
 * 				of the suffix tree
 *
 * @return	this function always returns zero (0)
 */
int stsw_print_stats (const int verbosity_level,
		size_t length,
		size_t max_ap_window_size,
		size_t min_edges,
		size_t min_edges_ap_bti,
		double avg_edges,
		size_t max_edges,
		size_t max_edges_ap_bti,
		size_t min_branching_nodes,
		size_t min_brn_ap_bti,
		double avg_branching_nodes,
		size_t max_branching_nodes,
		size_t max_brn_ap_bti,
		size_t tedge_size,
		size_t tbranch_size,
		size_t leaf_record_size,
		size_t edge_record_size,
		size_t branch_record_size,
		size_t extra_allocated_memory_size,
		size_t extra_used_memory_size) {
	size_t allocated_size = extra_allocated_memory_size;
	size_t used_size = extra_used_memory_size;
	/*
	 * adjusting the number of memory cells allocated for the leaves
	 * by including the unused first cell (tleaf[0])
	 */
	allocated_size += (max_ap_window_size + 1) * leaf_record_size;
	/* adding the memory allocated for the table tedge */
	allocated_size += tedge_size * edge_record_size;
	/* we have to include the leading unused branching node tbranch[0] */
	allocated_size += (tbranch_size + 1) * branch_record_size;
	/* adjusting the number of memory cells used by the table tleaf */
	if (length > max_ap_window_size) {
		used_size += max_ap_window_size * leaf_record_size;
	} else {
		used_size += length * leaf_record_size;
	}
	/* adding the average memory used by the table tedge */
	used_size += (size_t)(avg_edges + 0.5) * edge_record_size;
	/* adding the average memory used by the table tbranch */
	used_size += (size_t)(avg_branching_nodes + 0.5) * branch_record_size;
	printf("\nSuffix tree statistics:\n-----------------------\n");
	if (verbosity_level > 1) {
		if (leaf_record_size > 0) {
			printf("Leaf record size: %zu bytes\n",
					leaf_record_size);
		}
		if (edge_record_size > 0) {
			printf("Edge record size: %zu bytes\n",
					edge_record_size);
		}
		if (branch_record_size > 0) {
			printf("Branch record size: %zu bytes\n\n",
					branch_record_size);
		}
	}
	if (verbosity_level > 0) {
		printf("Total length of the text (number of the "
				"\"real\" characters): %zu\n\n", length);
		printf("Size of the table tleaf:\n"
				"%zu cells of %zu bytes "
				"(totalling %zu bytes, ",
			(max_ap_window_size + 1), leaf_record_size,
			(max_ap_window_size + 1) * leaf_record_size);
		print_human_readable_size(stdout,
			(max_ap_window_size + 1) * leaf_record_size);
		printf(").\n\n");
	}
	if (verbosity_level > 1) {
		if (min_edges > 0) {
			printf("\nMinimum number of edges: %zu "
					"at the text position %zu\n",
					min_edges, min_edges_ap_bti);
		}
		if (avg_edges > 0) {
			printf("Average number of edges: %.2f\n", avg_edges);
		}
		if (max_edges > 0) {
			printf("Maximum number of edges: %zu "
					"at the text position %zu\n\n",
					max_edges, max_edges_ap_bti);
		}
	}
	if ((verbosity_level > 0) && (tedge_size > 0)) {
		printf("Size of the table tedge:\n"
			"%zu cells of %zu bytes "
			"(totalling %zu bytes, ",
			tedge_size, edge_record_size,
			tedge_size * edge_record_size);
		print_human_readable_size(stdout,
			tedge_size * edge_record_size);
		printf(").\n");
		if ((verbosity_level > 1) && (avg_edges > 0)) {
			printf("Average edges load factor: %2.2f%%\n\n",
					100 * avg_edges /
					(double)(tedge_size));
		}
	}
	if (verbosity_level > 1) {
		if (min_branching_nodes > 0) {
			printf("Minimum number of branching nodes: %zu "
					"at the text position %zu\n",
					min_branching_nodes, min_brn_ap_bti);
		}
		if (avg_branching_nodes > 0) {
			printf("Average number of branching nodes: %.2f\n",
					avg_branching_nodes);
		}
		if (max_branching_nodes > 0) {
			printf("Maximum number of branching nodes: %zu "
					"at the text position %zu\n\n",
					max_branching_nodes, max_brn_ap_bti);
		}
	}
	if ((verbosity_level > 0) && (tbranch_size > 0)) {
		printf("Size of the table tbranch:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
			tbranch_size, branch_record_size,
			tbranch_size * branch_record_size);
		print_human_readable_size(stdout,
			tbranch_size * branch_record_size);
		if ((verbosity_level > 1) && (avg_branching_nodes > 0)) {
			printf("\nAverage branching nodes "
					"load factor: %2.2f%%\n\n", 100 *
					avg_branching_nodes /
					(double)(tbranch_size));
		} else {
			printf(").\n\n");
		}
	}
	if (verbosity_level > 1) {
		if (extra_allocated_memory_size > 0) {
			printf("Extra allocated memory size: %zu bytes (",
					extra_allocated_memory_size);
			print_human_readable_size(stdout,
					extra_allocated_memory_size);
			printf(").\n");
		}
		if (extra_used_memory_size > 0) {
			printf("Extra used memory size: %zu bytes (",
					extra_used_memory_size);
			print_human_readable_size(stdout,
					extra_used_memory_size);
			printf(").\n");
		}
	}
	printf("Total memory allocated: %zu bytes (",
			allocated_size);
	print_human_readable_size(stdout, allocated_size);
	printf(")\nwhich is %.3f bytes per active "
			"sliding window character.\n",
			(double)(allocated_size) /
			(double)(max_ap_window_size));
	printf("Total memory used: %zu bytes (", used_size);
	print_human_readable_size(stdout, used_size);
	printf(")\nwhich is %.3f bytes per active "
			"sliding window character.\n",
			(double)(used_size) /
			(double)(max_ap_window_size));
	printf("Memory load factor: %2.2Lf%%\n\n", 100 *
			(long double)(used_size) /
			(long double)(allocated_size));
	return (0);
}
