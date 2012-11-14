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
 * PWOTD functions implementation.
 * This file contains the implementation of the functions
 * related to the PWOTD algorithm, which are used by the functions,
 * which construct the suffix tree in the memory
 * using the implementation type SLAI.
 */
#include "pwotd_cdata.h"

#include <iconv.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* local functions */

/**
 * A function which orders the specified part of the table of suffixes
 * on a specified prefix character and its specified byte
 * according to the provided number of occurrences of the respective bytes.
 * It will use the temporary table of suffixes to store
 * the intermediate ordering result, from where
 * it will be copied back to the table of suffixes.
 *
 * @param
 * occurrences_size	the size of the table containing the numbers
 * 			of occurrences of the particular bytes
 * @param
 * occurrences	the table of occurrences of the particular bytes
 * @param
 * shift_size	the number of bits by which it is necessary to shift
 * 		each of the examined character values to obtain
 * 		the desired byte, according to which we will try
 * 		to order the table of suffixes.
 * @param
 * tsuffixes_part	The part of the table of suffixes
 * 			which will be ordered. Here, we expect
 * 			to obtain the pointer to the first of the entries
 * 			in the table of suffixes, which need to be ordered.
 * @param
 * tsuffixes_part_size	the size of the specified part
 * 			of the table of suffixes, which needs to be ordered
 * @param
 * tsuffixes_tmp	The temporary table of suffixes,
 * 			which will be used for storing the already
 * 			ordered part of the table of suffixes.
 * 			We suppose it is at least as large
 * 			as the part of the table of suffixes,
 * 			which needs to be ordered.
 * @param
 * prefix_character_offset	the current offset of the prefix character
 * 				(starting from zero), according to which
 * 				we will try to order the provided part
 * 				of the table of suffixes
 * @param
 * text		the actual underlying text of the suffix tree
 *
 * @return	This function always returns zero (0).
 */
int order_suffixes (size_t occurrences_size,
		size_t *occurrences,
		size_t shift_size,
		unsigned_integral_type *tsuffixes_part,
		size_t tsuffixes_part_size,
		unsigned_integral_type *tsuffixes_tmp,
		size_t prefix_character_offset,
		const character_type *text) {
	size_t i = 0;
	/*
	 * The variable used to store the cumulative sum
	 * of the byte partitions' starting offsets.
	 * We initially set it to 0, because we have to ensure
	 * that the first partition begins at the offset of 0,
	 * which is the first valid offset.
	 */
	size_t sum = 0;
	/*
	 * very similarly, this variable is used in addition
	 * to the 'sum' variable and stores its
	 * most recently used value
	 */
	size_t oldsum = 0;
	/* the currently processed byte of the currently processed character */
	unsigned char cbyte = 0;
	/*
	 * At first, we transform the table of occurrences of particular
	 * bytes into the table of cumulative sums representing
	 * the starting offsets of the respective partitions.
	 */
	for (i = 0; i < occurrences_size; ++i) {
		sum += occurrences[i];
		occurrences[i] = oldsum;
		oldsum = sum;
	}
	/*
	 * if we need not to use the bitwise shifting operation,
	 * because the size of the shift is zero,
	 * we can abandon it completely to make the code a little bit faster
	 */
	if (shift_size == 0) {
		for (i = 0; i < tsuffixes_part_size; ++i) {
			/*
			 * according to the conversion conventions,
			 * this is equivalent to:
			 * cbyte = (text[tsuffixes_part[i] +
			 * 		prefix_character_offset]) % 256;
			 */
			cbyte = (unsigned char)(text[tsuffixes_part[i] +
					prefix_character_offset]);
			tsuffixes_tmp[occurrences[cbyte]] = tsuffixes_part[i];
			++occurrences[cbyte];
		}
	} else { /* otherwise, we have to use it */
		for (i = 0; i < tsuffixes_part_size; ++i) {
			/*
			 * according to the conversion conventions,
			 * this is equivalent to:
			 * cbyte = (text[tsuffixes_part[i] +
			 * 		prefix_character_offset] >>
			 * 		shift_size) % 256;
			 */
			cbyte = (unsigned char)(text[tsuffixes_part[i] +
					prefix_character_offset] >>
					shift_size);
			tsuffixes_tmp[occurrences[cbyte]] = tsuffixes_part[i];
			++occurrences[cbyte];
		}
	}
	/*
	 * Finally, we just copy the ordered suffixes from the temporary
	 * table of suffixes back to its original location
	 * in the main table of suffixes.
	 */
	for (i = 0; i < tsuffixes_part_size; ++i) {
		tsuffixes_part[i] = tsuffixes_tmp[i];
	}
	return (0);
}

/**
 * A function which dumps the provided array of byte occurrences
 * used when ordering the table of suffixes
 * into the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the array
 * 		of byte occurrences will be dumped
 * @param
 * occurrences_size	the size of the array
 * 			of byte occurrences to be dumped
 * @param
 * occurrences	the array of byte occurrences to be dumped
 *
 * @return	This function always returns zero (0).
 */
int pwotd_dump_occurrences (FILE *stream,
		size_t occurrences_size,
		const size_t *occurrences) {
	size_t i = 0;
	while (occurrences[i] == 0) {
		++i;
	}
	fprintf(stream, "(%zu)%zu", i, occurrences[i]);
	for (++i; i < occurrences_size; ++i) {
		if (occurrences[i] > 0) {
			fprintf(stream, ", (%zu)%zu", i, occurrences[i]);
		}
	}
	fprintf(stream, "\n");
	return (0);
}

/* supporting functions */

/**
 * A function which updates the length
 * of the longest common prefix of all the suffixes
 * in the provided range of the current partition of suffixes.
 *
 * @param
 * lcp_size	The last known value of the longest common prefix
 * 		of all the suffixes in the provided range.
 * 		When this function successfully returns,
 * 		this parameter will contain the current length
 * 		of the longest common prefix shared by
 * 		all the suffixes in the provided range.
 * @param
 * range_begin	The beginning of the range in the current partition
 * 		of suffixes, from which we will try to determine
 * 		the longest common prefix of the suffixes.
 * @param
 * range_end	The end of the range in the current partition
 * 		of suffixes, from which we will try to determine
 * 		the longest common prefix of the suffixes.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_determine_lcp (size_t *lcp_size,
		size_t range_begin,
		size_t range_end,
		const character_type *text,
		size_t length,
		const pwotd_construction_data *cdata) {
	size_t i = range_begin + 1;
	size_t j = 0;
	size_t k = 0;
	size_t old_lcp_size = (*lcp_size);
	unsigned_integral_type first_text_idx =
		cdata->current_partition[range_begin];
	unsigned_integral_type text_idx = 0;
	size_t first_text_idx_end = length + 2;
	size_t text_idx_end = length + 2;
	/* if the provided range contains just one suffix */
	if (i == range_end) {
		(*lcp_size) = text_idx_end - first_text_idx;
		return (0);
	}
	/*
	 * otherwise, we have to compare all the suffixes in the provided range
	 * of the current partition of suffixes in order to
	 * determine their longest common prefix
	 */
	for (; i < range_end; ++i) {
		text_idx = cdata->current_partition[i];
		j = first_text_idx + old_lcp_size;
		k = text_idx + old_lcp_size;
		/*
		 * if the currently examined part of the suffix
		 * is shorter than the current longest common prefix,
		 * we have to shorten it accordingly
		 */
		if (text_idx_end - j > first_text_idx_end - k) {
			text_idx_end = j + first_text_idx_end - k;
		}
		for (; j < text_idx_end; ++j, ++k) {
			if (text[j] != text[k]) {
				break;
			}
		}
		text_idx_end = j;
	}
	(*lcp_size) = text_idx_end - first_text_idx;
	return (0);
}

/**
 * A function which updates the length of the longest common prefix
 * of all the suffixes in the provided range of the table of suffixes.
 *
 * @param
 * lcp_size	The last known value of the longest common prefix
 * 		of all the suffixes in the provided range
 * 		of the table of suffixes.
 * 		When this function successfully returns,
 * 		this parameter will contain the current length
 * 		of the longest common prefix shared by
 * 		all the suffixes in the provided range
 * 		of the table of suffixes.
 * @param
 * range_begin	The beginning of the range in the table of suffixes,
 * 		from which we will try to determine
 * 		the longest common prefix of the suffixes.
 * @param
 * range_end	The end of the range in the table of suffixes,
 * 		from which we will try to determine
 * 		the longest common prefix of the suffixes.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_determine_tsuffixes_range_lcp (size_t *lcp_size,
		size_t range_begin,
		size_t range_end,
		const character_type *text,
		size_t length,
		const pwotd_construction_data *cdata) {
	size_t i = range_begin + 1;
	size_t j = 0;
	size_t k = 0;
	size_t old_lcp_size = (*lcp_size);
	unsigned_integral_type first_text_idx =
		cdata->tsuffixes[range_begin];
	unsigned_integral_type text_idx = 0;
	size_t first_text_idx_end = length + 2;
	size_t text_idx_end = length + 2;
	/* if the provided range contains just one suffix */
	if (i == range_end) {
		(*lcp_size) = text_idx_end - first_text_idx;
		return (0);
	}
	/*
	 * otherwise, we have to compare all the suffixes in the provided range
	 * of the table of suffixes in order to
	 * determine their longest common prefix
	 */
	for (; i < range_end; ++i) {
		text_idx = cdata->tsuffixes[i];
		j = first_text_idx + old_lcp_size;
		k = text_idx + old_lcp_size;
		/*
		 * if the currently examined part of the suffix
		 * is shorter than the current longest common prefix,
		 * we have to shorten it accordingly
		 */
		if (text_idx_end - j > first_text_idx_end - k) {
			text_idx_end = j + first_text_idx_end - k;
		}
		for (; j < text_idx_end; ++j, ++k) {
			if (text[j] != text[k]) {
				break;
			}
		}
		text_idx_end = j;
	}
	(*lcp_size) = text_idx_end - first_text_idx;
	return (0);
}

/**
 * A function which updates the length of the longest common prefix
 * of all the suffixes in all the partitions
 * in the provided range of partitions.
 *
 * When this function is called, we suppose that the length
 * of the longest common prefix has already been determined
 * for at least the first partition in the provided range.
 *
 * @param
 * lcp_size	The last known value of the longest common prefix
 * 		of all the suffixes in the provided range of partitions.
 * 		When this function successfully returns,
 * 		this parameter will contain the current length
 * 		of the longest common prefix shared by
 * 		all the suffixes in the provided range of partitions.
 * @param
 * range_begin	The beginning of the range in the table of partitions,
 * 		for which we will try to determine their longest common prefix.
 * @param
 * range_end	The end of the range in the table of partitions,
 * 		for which we will try to determine their longest common prefix.
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_determine_partitions_range_lcp (size_t *lcp_size,
		size_t range_begin,
		size_t range_end,
		const character_type *text,
		size_t length,
		const pwotd_construction_data *cdata) {
	size_t i = range_begin + 1;
	size_t j = 0;
	size_t k = 0;
	size_t old_lcp_size = (*lcp_size);
	size_t first_text_idx =
		cdata->partitions[range_begin].text_offset;
	size_t text_idx = 0;
	size_t first_text_idx_end = length + 2;
	size_t text_idx_end = length + 2;
	/* if the provided range contains just one partition */
	if (i == range_end) {
		/*
		 * then the desired length of the longest common prefix
		 * is equal to the longest common prefix
		 * of all the suffixes in this partition
		 */
		(*lcp_size) = cdata->partitions[range_begin].lcp_size;
		return (0);
	}
	/*
	 * Otherwise, we would have to compare all the suffixes
	 * of all the partitions in the provided range. But fortunately,
	 * since the maximum length of the longest common suffix in this case
	 * is equal to the prefix_length, it is sufficient to take just
	 * one arbitrarily chosen suffix from each partition and compare
	 * these with each other.
	 */
	for (; i < range_end; ++i) {
		text_idx = cdata->partitions[i].text_offset;
		j = first_text_idx + old_lcp_size;
		k = text_idx + old_lcp_size;
		if (text_idx_end - j > first_text_idx_end - k) {
			text_idx_end = j + first_text_idx_end - k;
		}
		for (; j < text_idx_end; ++j, ++k) {
			if (text[j] != text[k]) {
				break;
			}
		}
		text_idx_end = j;
	}
	(*lcp_size) = text_idx_end - first_text_idx;
	return (0);
}

/**
 * A function which computes the smallest starting text offset
 * of all the suffixes in the provided range of the table of partitions.
 *
 * We suppose that the smallest starting text offsets for all the partitions
 * in the provided range have already been computed.
 *
 * @param
 * text_offset	When this function successfully returns, this parameter
 * 		will contain the smallest starting text offset of all
 * 		the suffixes in all the partitions in the provided range.
 * @param
 * range_begin	The beginning of the range in the table of partitions,
 * 		for which we will try to determine their
 * 		smallest starting text_offset.
 * @param
 * range_end	The end of the range in the table of partitions,
 * 		for which we will try to determine their
 * 		smallest starting text_offset.
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_determine_partitions_range_smallest_text_offset (
		size_t *text_offset,
		size_t range_begin,
		size_t range_end,
		const pwotd_construction_data *cdata) {
	size_t partition_index = range_begin;
	(*text_offset) = cdata->partitions[range_begin].text_offset;
	for (; partition_index < range_end; ++partition_index) {
		if (cdata->partitions[partition_index].text_offset <
				(*text_offset)) {
			(*text_offset) =
				cdata->partitions[partition_index].
				text_offset;
		}
	}
	return (0);
}

/* handling functions */

/**
 * A function which prepares the table of suffixes by initializing it
 * with all the suffixes sorted by their length
 * from the longest to the shortest.
 *
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero.
 */
int pwotd_initialize_suffixes (pwotd_construction_data *cdata) {
	size_t i = 0;
	/*
	 * We start from 1, as the 0.th position
	 * of the table of suffixes is unused.
	 */
	for (i = 1; i < cdata->tsuffixes_size; ++i) {
		cdata->tsuffixes[i] = (unsigned_integral_type)(i);
	}
	return (0);
}

/**
 * A function which divides the current suffixes into several partitions,
 * based on the prefix_length initial letters.
 *
 * We suppose that the table of suffixes has already been initialized.
 *
 * @param
 * prefix_length	the number of initial characters,
 * 			which will be considered when dividing
 * 			the suffixes into the partitions
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	If the partitioning is successful, this function returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int pwotd_partition_suffixes (size_t prefix_length,
		const character_type *text,
		size_t length,
		pwotd_construction_data *cdata) {
	size_t i = 0;
	/* the offset of the current prefix character */
	size_t cpco = 0;
	/*
	 * the index of the currently processed byte
	 * in the current prefix character
	 */
	size_t bi = 0;
	/* the index used for the text */
	size_t ti = 0;
	/*
	 * the index to the text of the character immediately following
	 * the terminating character ($)
	 */
	size_t text_end = length + 2;
	/*
	 * The size of the data type 'character_type' containing
	 * the individual letters of the text.
	 */
	size_t character_type_size = sizeof (character_type);
	/* the currently processed byte of the currently processed character */
	unsigned char cbyte = 0;
	/*
	 * the number of bits by which it is necessary to shift the currently
	 * processed character to obtain the currently processed byte
	 */
	size_t shift_size = 0;
	/*
	 * The offset in the table of suffixes,
	 * where the currently examined partition starts.
	 * The starting offset of the first partition is equal to 1,
	 * so we can set it right here.
	 */
	size_t partition_start = 1;
	/*
	 * The size of the largest partition created.
	 * It will be determined later.
	 */
	size_t maximum_partition_size = 0;
	/* FIXME: Try to make the estimation more accurate and if possible
	 * avoid constants!
	 * The estimated final size of the table of partitions.
	 * Its value should be based on the size
	 * of the data type character_type and on the length
	 * of the prefix. However, we just set it to the maximum number
	 * of possible values of a single byte character
	 * and if necessary, we will further increase it later.
	 */
	size_t expected_partitions_size = 256;
	/*
	 * The size of the longest common prefix of all the suffixes
	 * sharing the same partition. It will be computed
	 * for all the partitions.
	 */
	size_t lcp_size = 0;
	/*
	 * the array used by the counting sort containing the number
	 * of occurrences of every byte
	 *
	 * Despite the static variables are initialized to zero automatically,
	 * we do it explicitly to emphasize the correctness of the zero values.
	 *
	 * As usual, the non-listed members of the array
	 * are initialized to zero automatically.
	 */
	static size_t occurrences[256] = {0};
	if (prefix_length == 0) {
		fprintf(stderr, "Error: The provided prefix length (0) "
				"is invalid!\n");
		return (1);
	} else if (prefix_length > length + 1) {
		fprintf(stderr, "Warning: The provided prefix length (%zu) "
				"is too long!\nShortened to the length of "
				"the longest suffix (%zu).\n", prefix_length,
				length + 1);
		prefix_length = length + 1;
	}
	/* storing the desired prefix length inside the construction data */
	cdata->prefix_length = prefix_length;
	printf("Starting the partitioning using the prefix length of %zu\n",
			prefix_length);
	/*
	 * At first, we try to allocate the memory for the temporary
	 * table of suffixes and for the table of partitions.
	 * The size of the temporary table of suffixes must be equal
	 * to the size of the main table of suffixes,
	 * because it will be used for sorting the main table of suffixes.
	 * The reason is that in the "worst" case, there could be a partition
	 * with length very close to the total number of suffixes.
	 */
	if (pwotd_cdata_tsuffixes_tmp_reallocate(cdata->tsuffixes_size,
				length, cdata) > 0) {
		fprintf(stderr, "Error: pwotd_partition_suffixes:\n"
				"Could not reallocate the memory "
				"for the temporary table of suffixes!\n");
		return (2);
	}
	if (pwotd_cdata_partitions_reallocate(expected_partitions_size,
				length, cdata) > 0) {
		fprintf(stderr, "Error: pwotd_partition_suffixes:\n"
				"Could not reallocate the memory "
				"for the table of partitions!\n");
		return (3);
	}
	/*
	 * Then we make sure that the table of partitions is empty.
	 * However, we do not deallocate the memory for this table,
	 * we just set the number of partitions to zero.
	 */
	cdata->partitions_number = 0;
	/*
	 * we start sorting from the least significant (last)
	 * letter of the prefix
	 */
	for (cpco = prefix_length; cpco > 0; ) {
		--cpco;
		/*
		 * resetting the number of occurrences
		 * of each byte to zero
		 */
		for (i = 0; i < 256; ++i) {
			occurrences[i] = 0;
		}
		/*
		 * We start with the iteration for the least significant byte,
		 * which does not require the bitwise shift operation.
		 *
		 * The text starts at the index of 1, so we need
		 * to remember it when using the prefix letter offset.
		 */
		for (ti = 1 + cpco; ti < text_end; ++ti) {
			/*
			 * according to the conversion conventions,
			 * this is equivalent to: cbyte = text[ti] % 256;
			 */
			cbyte = (unsigned char)(text[ti]);
			/*
			 * we do not have to use the parentheses like this:
			 * ++(occurrences[cbyte]), because the prefix
			 * increment operator has lower priority
			 * than the square brackets operator
			 */
			++occurrences[cbyte];
		}
		shift_size = 0;
		/*
		 * Here, we suppose that the ordering proceeds from
		 * the largest prefix character offset to the shortest
		 * prefix character offset (0) and that in the beginning,
		 * all the suffixes of the text are present in the table
		 * of suffixes and are ordered according to their length
		 * such that the shortest suffixes are present
		 * at the end of the table.
		 */
		order_suffixes((size_t)(256), occurrences, shift_size,
				cdata->tsuffixes + 1,
				cdata->tsuffixes_size - cpco - 1,
				cdata->tsuffixes_tmp, cpco, text);
		/*
		 * Here, it is important to start from 1,
		 * because the iteration for zero has already been done.
		 */
		for (bi = 1; bi < character_type_size; ++bi) {
			/* resetting the occurrences of each byte to zero */
			for (i = 0; i < 256; ++i) {
				occurrences[i] = 0;
			}
			/* adjusting the shift size */
			shift_size += 8;
			/*
			 * The text starts at the index of 1, so we need
			 * to remember it when using the prefix letter offset.
			 */
			for (ti = 1 + cpco; ti < text_end; ++ti) {
				/*
				 * according to the conversion conventions,
				 * this is equivalent to:
				 * cbyte = (text[ti] >> shift_size) % 256;
				 */
				cbyte = (unsigned char)(text[ti] >>
						shift_size);
				/*
				 * again, we do not have to use
				 * the parentheses, because the prefix
				 * increment operator has lower priority
				 * than the square brackets operator
				 */
				++occurrences[cbyte];
			}
			/*
			 * Here, we suppose that the ordering proceeds
			 * from the largest prefix character offset
			 * to the shortest prefix character offset (0)
			 * and that in the beginning, all the suffixes
			 * of the text are present in the table of suffixes
			 * and are ordered according to their length
			 * such that the shortest suffixes
			 * are present at the end of the table.
			 */
			order_suffixes((size_t)(256), occurrences,
				shift_size, cdata->tsuffixes + 1,
				cdata->tsuffixes_size - cpco - 1,
				cdata->tsuffixes_tmp, cpco, text);
		}
	}
	/*
	 * Finally, we just scan the partially ordered array of suffixes
	 * and determine the partition boundaries by scanning
	 * the first prefix_length characters of each suffix.
	 *
	 * We start from 2, as the first suffix can not be compared
	 * to its preceding suffix, simply because it does not exist.
	 */
	for (i = 2; i < cdata->tsuffixes_size; ++i) {
		/*
		 * We suppose that each suffix has the different length.
		 * This enables us to create a new partition immediately if
		 * the first suffix in the partition is not long enough to have
		 * all the 'prefix_length' initial characters compared.
		 *
		 * FIXME: Can't we move this part outside the loop?
		 */
		if (cdata->tsuffixes[partition_start] + prefix_length >
				text_end) {
			/*
			 * the current partition contains just one suffix,
			 * so the length of its longest common prefix
			 * is equal to the length of this suffix
			 */
			lcp_size = text_end - cdata->tsuffixes[
				partition_start];
			pwotd_insert_partition(partition_start,
					i, lcp_size, length, cdata);
			/*
			 * checking whether we have just encountered
			 * the largest partition so far
			 */
			if (i - partition_start > maximum_partition_size) {
				maximum_partition_size = i - partition_start;
			}
			partition_start = i;
		/*
		 * It also enables us to create two new partitions if
		 * the current suffix is not long enough to have
		 * all the 'prefix_length' initial characters compared.
		 */
		} else if (cdata->tsuffixes[i] + prefix_length > text_end) {
			/*
			 * we can be sure that the longest common prefix
			 * of all the suffixes in the current partition
			 * is at lest as long as the prefix_length
			 */
			lcp_size = prefix_length;
			pwotd_determine_tsuffixes_range_lcp(&lcp_size,
					partition_start, i,
					text, length, cdata);
			pwotd_insert_partition(partition_start,
					i, lcp_size, length, cdata);
			/*
			 * checking whether we have just encountered
			 * the largest partition so far
			 */
			if (i - partition_start > maximum_partition_size) {
				maximum_partition_size = i - partition_start;
			}
			partition_start = i;
			++i;
			/*
			 * the second partition we are about to create
			 * contains just one suffix, so the length
			 * of its longest common prefix is equal
			 * to the length of this suffix
			 */
			lcp_size = text_end - cdata->tsuffixes[partition_start];
			pwotd_insert_partition(partition_start,
					i, lcp_size, length, cdata);
			/*
			 * We do not need to check for the possibly larger
			 * maximum_partition_size here, because the size
			 * of this partition is 1, which is surely at most
			 * as large as the size of the partition
			 * created just a few lines earlier.
			 */
			partition_start = i;
		/*
		 * If both suffixes are long enough, we can proceed
		 * and compare all the 'prefix_length' initial characters.
		 */
		} else {
			/*
			 * We start the comparison of the two adjacent suffixes
			 * from the last letter of the considered prefix,
			 * because we suppose it is more probable that
			 * the two adjacent suffixes differ
			 * at the less significant position than
			 * at the more significant position.
			 */
			/* the iteration starts at prefix_length - 1 */
			for (cpco = prefix_length; cpco > 0; ) {
				--cpco;
				/*
				 * if we find a mismatch, we create
				 * a new partition
				 */
				if (text[cdata->tsuffixes[i] + cpco] !=
					text[cdata->tsuffixes[i - 1] + cpco]) {
					/*
					 * we can safely assume that
					 * the longest common prefix
					 * of all the suffixes
					 * in the current partition
					 * is at lest as long
					 * as the prefix_length
					 */
					lcp_size = prefix_length;
					pwotd_determine_tsuffixes_range_lcp(
							&lcp_size,
							partition_start, i,
							text, length, cdata);
					pwotd_insert_partition(partition_start,
							i, lcp_size,
							length, cdata);
					/*
					 * checking whether
					 * we have just encountered
					 * the largest partition so far
					 */
					if (i - partition_start >
						maximum_partition_size) {
						maximum_partition_size =
							i - partition_start;
					}
					partition_start = i;
					break;
				}
			}
		}
	}
	/*
	 * We have to check whether the last suffix doesn't need
	 * to be contained in an extra partition.
	 *
	 * FIXME: Check this out! What if the last partition
	 * contained two or more suffixes?
	 */
	if (partition_start == cdata->tsuffixes_size - 1) {
		lcp_size = 0;
		pwotd_determine_tsuffixes_range_lcp(&lcp_size,
				partition_start, cdata->tsuffixes_size,
				text, length, cdata);
		pwotd_insert_partition(partition_start,
				cdata->tsuffixes_size, lcp_size,
				length, cdata);
		if (maximum_partition_size == 0) {
			maximum_partition_size = 1;
		}
	}
	/*
	 * we will now try to decrease the size of the temporary table
	 * of suffixes to the size of the largest partition
	 */
	if (pwotd_cdata_tsuffixes_tmp_reallocate(maximum_partition_size,
				length, cdata) > 0) {
		fprintf(stderr, "Error: pwotd_partition_suffixes:\n"
				"Could not reallocate the memory for "
				"the temporary table of suffixes!\n");
		return (4);
	}
	printf("The partitioning has been successfully completed!\n");
	printf("Number of partitions created: %zu.\n\n",
			cdata->partitions_number);
	return (0);
}

/**
 * A function which inserts the new partition into the table of partitions
 * to be processed.
 *
 * @param
 * new_index	an index to the table of partitions of the new partition,
 * 		which needs to be processed
 * @param
 * new_tnode_offset	The offset of an entry in the table tnode,
 * 			which is reserved for the "pointer"
 * 			(an index to the table tnode) to the first child
 * 			of the yet unevaluated branching node,
 * 			which represents the closest common ancestor node
 * 			of all the suffixes in the new partition
 * 			to be processed.
 * @param
 * new_parents_depth	the depth of a node, which is the closest common
 * 			ancestor node of all the suffixes in the partition,
 * 			which is about to be inserted into the table
 * 			of partitions, which need to be further processed
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	If the insertion of the new partition entry is successful,
 * 		and does not require the reallocation,
 * 		this function returns 0.
 * 		In case the reallocation of the table of the entries
 * 		of the partitions to be processed is necessary and succeeds,
 * 		this function returns (-1).
 * 		Otherwise, a positive error number is returned.
 */
int pwotd_insert_partition_tbp (size_t new_index,
		size_t new_tnode_offset,
		size_t new_parents_depth,
		size_t length,
		pwotd_construction_data *cdata) {
	int return_value = 0;
	/* if the stack of entries of the partitions to be processed is full */
	if (cdata->partitions_tbp_number == cdata->partitions_tbp_size) {
		/* we want at least to double its current size */
		if (pwotd_cdata_partitions_tbp_reallocate(
			cdata->partitions_tbp_size * 2, length, cdata) > 0) {
			fprintf(stderr, "Error: pwotd_insert_partition_tbp:\n"
				"Could not reallocate the memory for "
				"the stack of entries\nof the partitions "
				"to be processed!\n");
			return (1);
		}
		return_value = (-1);

	}
	cdata->partitions_tbp[cdata->partitions_tbp_number].index = new_index;
	cdata->partitions_tbp[cdata->partitions_tbp_number].tnode_offset =
		new_tnode_offset;
	cdata->partitions_tbp[cdata->partitions_tbp_number].parents_depth =
		new_parents_depth;
	/*
	 * we do not have to use the parentheses like this:
	 * ++(cdata->partitions_tbp_number), because the prefix
	 * increment operator has lower priority
	 * than the member selection via pointer
	 */
	++cdata->partitions_tbp_number;
	return (return_value);
}

/**
 * A function which pushes the specified entry onto the stack
 * of the unevaluated partition ranges.
 *
 * @param
 * new_range_begin	The offset of the first partition in the range
 * 			of partitions represented by this partitions stack
 * 			record.
 * @param
 * new_range_end	The offset of the partition just after the last
 * 			partition in the range of partitions represented by
 * 			this partitions stack record.
 * @param
 * new_lcp_size	The length of the longest common prefix shared by
 * 		all the suffixes in all the partitions represented by
 * 		this partitions stack record.
 * @param
 * new_tnode_offset	The offset of an entry in the table tnode,
 * 			which is reserved for the "pointer"
 * 			(an index to the table tnode) to the first child
 * 			of the yet unevaluated branching node,
 * 			which is described by this partitions stack entry.
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	If the pushing onto the stack is successful,
 * 		this function returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int pwotd_partitions_stack_push (size_t new_range_begin,
		size_t new_range_end,
		size_t new_lcp_size,
		size_t new_tnode_offset,
		size_t length,
		pwotd_construction_data *cdata) {
	cdata->partitions_stack[cdata->partitions_stack_top].range_begin =
		new_range_begin;
	cdata->partitions_stack[cdata->partitions_stack_top].range_end =
		new_range_end;
	cdata->partitions_stack[cdata->partitions_stack_top].lcp_size =
		new_lcp_size;
	cdata->partitions_stack[cdata->partitions_stack_top].tnode_offset =
		new_tnode_offset;
	++cdata->partitions_stack_top;
	/*
	 * we have to check whether the stack
	 * is still not full
	 */
	if (cdata->partitions_stack_top == cdata->partitions_stack_size) {
		if (pwotd_cdata_partitions_stack_reallocate(
		/* we would like at least to double its current size */
					cdata->partitions_stack_size * 2,
					length, cdata) > 0) {
			fprintf(stderr, "Error: Could not reallocate "
			"the memory for the stack\nof the unevaluated "
			"partition ranges. Exiting.\n");
			return (1);
		}
	}
	return (0);
}

/**
 * A function which sorts the specified part of the current partition
 * of the table of suffixes, using the characters
 * at the specified prefix_offset of each suffix.
 *
 * @param
 * prefix_offset	the offset of the prefix character,
 * 			which will be considered when sorting the suffixes
 * @param
 * pts_begin	the starting offset of the desired part
 * 		of the current partition of suffixes to be sorted
 * @param
 * pts_end	the offset just after the last valid offset
 * 		of the desired part of the current partition of suffixes
 * 		to be considered when sorting
 * @param
 * text		the actual underlying text of the suffix tree
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	This function always returns zero (0).
 */
int pwotd_sort_suffixes (size_t prefix_offset,
		size_t pts_begin,
		size_t pts_end,
		const character_type *text,
		pwotd_construction_data *cdata) {
	size_t i = 0;
	/* the currently processed byte of the currently processed character */
	unsigned char cbyte = 0;
	/*
	 * the index of the currently processed byte
	 * in the desired prefix character
	 */
	size_t bi = 0;
	/*
	 * the number of bits by which it is necessary to shift
	 * the currently processed character to obtain
	 * the currently processed byte
	 */
	size_t shift_size = 0;
	/*
	 * The size of the data type 'character_type' containing
	 * the individual letters of the text.
	 */
	size_t character_type_size = sizeof (character_type);
	/*
	 * the array used by the counting sort containing the number
	 * of occurrences of every byte
	 *
	 * the non-listed members of the array
	 * are initialized to zero automatically
	 *
	 * moreover, the static members are always initialized to zero
	 * when no initializer is present
	 */
	static size_t occurrences[256] = {0};
	/*
	 * At the beginning, we just check whether the size
	 * of the desired part of the current partition
	 * of suffixes to be sorted is greater than one.
	 * If it isn't, there is nothing to do and we can return immediately.
	 */
	if (pts_begin + 1 == pts_end) {
		return (0); /* nothing to do */
	}
	/*
	 * resetting the number of occurrences of each byte to zero
	 *
	 * it is necessary to perform it there,
	 * because the occurrences array is static
	 */
	for (i = 0; i < 256; ++i) {
		occurrences[i] = 0;
	}
	/*
	 * We start with the iteration for the least significant byte,
	 * which does not require the bitwise shift operation.
	 */
	for (i = pts_begin; i < pts_end; ++i) {
		/*
		 * according to the conversion conventions,
		 * this is equivalent to: cbyte =
		 * 	text[cdata->current_partition[i] +
		 * 	prefix_offset] % 256;
		 */
		cbyte = (unsigned char)(text[cdata->current_partition[i] +
				prefix_offset]);
		/*
		 * we do not have to use the parentheses like this:
		 * ++(occurrences[cbyte]), because the prefix
		 * increment operator has lower priority
		 * than the square brackets operator
		 */
		++occurrences[cbyte];
	}
	shift_size = 0;
	order_suffixes((size_t)(256), occurrences, shift_size,
			cdata->current_partition + pts_begin,
			pts_end - pts_begin,
			cdata->tsuffixes_tmp, prefix_offset, text);
	/*
	 * Here, it is important to start from 1,
	 * because the iteration for zero has already been done.
	 */
	for (bi = 1; bi < character_type_size; ++bi) {
		/* resetting the occurrences of each byte to zero */
		for (i = 0; i < 256; ++i) {
			occurrences[i] = 0;
		}
		/* adjusting the shift size */
		shift_size += 8;
		/*
		 * The text starts at the index of 1, so we need
		 * to remember it when using the prefix letter offset.
		 */
		for (i = pts_begin; i < pts_end; ++i) {
			/*
			 * according to the conversion conventions,
			 * this is equivalent to: cbyte =
			 * 	(text[cdata->current_partition[i] +
			 * 	prefix_offset] >> shift_size) % 256;
			 */
			cbyte = (unsigned char)
					(text[cdata->current_partition[i] +
					prefix_offset] >> shift_size);
			/*
			 * again, we do not have to use
			 * the parentheses, because the prefix
			 * increment operator has lower priority
			 * than the square brackets operator
			 */
			++occurrences[cbyte];
		}
		order_suffixes((size_t)(256), occurrences, shift_size,
			cdata->current_partition + pts_begin,
			pts_end - pts_begin,
			cdata->tsuffixes_tmp, prefix_offset, text);
	}
	return (0);
}

/**
 * A function which inserts the new partition into the table of partitions.
 *
 * We suppose that when this function is called,
 * all the suffixes sharing the same prefix, which will immediately
 * become the members of a new partition, are ordered by their length
 * from the longest to the shortest.
 *
 * @param
 * begin_offset	the position in the table of suffixes
 * 		where the new partition starts
 * @param
 * end_offset	the position in the table of suffixes
 * 		just after the position where the new partition ends
 * @param
 * lcp_size	The length of the longest common prefix
 * 		of all the suffixes in the provided range
 * 		of the table of suffixes.
 * @param
 * length	the final length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	If the insertion of the new partition is successful,
 * 		and does not require the reallocation of the table
 * 		of partitions, this function returns 0.
 * 		In case the reallocation of the table of partitions
 * 		is necessary and succeeds, this function returns (-1).
 * 		Otherwise, a positive error number is returned.
 */
int pwotd_insert_partition (size_t begin_offset,
		size_t end_offset,
		size_t lcp_size,
		size_t length,
		pwotd_construction_data *cdata) {
	int return_value = 0;
	/* if the table of partitions is full */
	if (cdata->partitions_number == cdata->partitions_size) {
		/* we want at least to double its current size */
		if (pwotd_cdata_partitions_reallocate(
			cdata->partitions_size * 2, length, cdata) > 0) {
			fprintf(stderr, "Error: pwotd_insert_partition:\n"
				"Could not reallocate the memory for "
				"the table of partitions!\n");
			return (1);
		}
		return_value = (-1);
	}
	cdata->partitions[cdata->partitions_number].begin_offset =
		begin_offset;
	cdata->partitions[cdata->partitions_number].end_offset = end_offset;
	cdata->partitions[cdata->partitions_number].lcp_size = lcp_size;
	/*
	 * the longest suffix contained in this partition is by our assumption
	 * present at the beginning of the supplied range of suffixes
	 */
	cdata->partitions[cdata->partitions_number].text_offset =
		(size_t)(cdata->tsuffixes[begin_offset]);
	/*
	 * we do not have to use the parentheses like this:
	 * ++(cdata->partitions_number), because the prefix
	 * increment operator has lower priority
	 * than the member selection via pointer
	 */
	++cdata->partitions_number;
	return (return_value);
}

/**
 * A function which selects the desired partition and makes it active.
 *
 * @param
 * partition_index	the index of the partition which we would like
 * 			to make active
 * @param
 * cdata	the actual data structures necessary
 * 		for the suffix tree construction
 *
 * @return	If the partition selection is successful,
 * 		this function returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int pwotd_select_partition (size_t partition_index,
		pwotd_construction_data *cdata) {
	/* if there is no partition with the desired index */
	if (partition_index >= cdata->partitions_number) {
		fprintf(stderr, "Error: The provided partition index (%zu) "
				"is invalid!\n", partition_index);
		return (1);
	}
	cdata->current_partition = cdata->tsuffixes +
		cdata->partitions[partition_index].begin_offset;
	/* we also have to remember which partition is currently active */
	cdata->cp_index = partition_index;
	return (0);
}
