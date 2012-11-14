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
 * The very common hash table-related functions implementation.
 * This file contains the implementation of the very common functions,
 * which are used by the functions which use the hash table
 * in both the in-memory suffix tree construction
 * as well as in the suffix tree over a sliding window
 * construction and maintenance.
 */

/* feature test macros */

/** A macro necessary for the function random. */
#define	_BSD_SOURCE

#include "suffix_tree_hash_table_common.h"
#include "primality_test.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* hashing-related functions */

/**
 * A function which updates the hash settings
 * according to the current hash table size change.
 *
 * @param
 * verbosity_level	the desired level of messaging verbosity
 * @param
 * new_size	The desired new size of the hash table.
 * 		When this function successfully returns,
 * 		it will be set to the currently valid hash table size,
 * 		which might be different from the desired size,
 * 		but it will always be at least as large as the desired size.
 * @param
 * hs		The hash settings to be updated.
 *
 * @return	If this function call was successful, 0 is returned.
 * 		Otherwise, a positive error number is returned.
 */
int hs_update (const int verbosity_level,
		size_t *new_size,
		hash_settings *hs) {
	/* the default number of the Cuckoo hash functions */
	static const size_t chf_count_default = 8;
	/* the current index of the Cuckoo hash function */
	unsigned_integral_type i = 0;
	if (hs == NULL) {
		fprintf(stderr, "Error: The provided hash settings "
				"pointer is NULL!\n");
		return (1);
	}
	if (hs->crt_type == 0) { /* the hashing type has not been set yet */
		/* the default hashing type is the Cuckoo hashing */
		hs->crt_type = 1;
	}
	if (hs->crt_type == 1) { /* the Cuckoo hashing */
		if (verbosity_level > 1) {
			printf("The selected collision resolution technique: "
					"Cuckoo hashing\n");
		}
		if (hs->chf_number == 0) {
			/*
			 * The number of Cuckoo hash functions
			 * has not yet been explicitly set, so we set it
			 * to the default value.
			 */
			hs->chf_number = chf_count_default;
		}
		if (hs->chf_number < 2) {
			fprintf(stderr, "Warning: The number of the "
					"Cuckoo hash functions\n"
					"needs to be at least 2. "
					"The provided value (%zu)\n"
					"will be adjusted to the "
					"default value of %zu.\n",
					hs->chf_number, chf_count_default);
			hs->chf_number = chf_count_default;
		}
		/*
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(hs->chf_as);
		hs->chf_as = calloc(hs->chf_number,
				sizeof (unsigned_integral_type));
		if (hs->chf_as == NULL) {
			perror("calloc(hs->chf_as)");
			/* resetting the errno */
			errno = 0;
			return (2);
		} else {
			/* resetting the errno */
			errno = 0;
		}
		/*
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(hs->chf_bs);
		hs->chf_bs = calloc(hs->chf_number,
				sizeof (unsigned_integral_type));
		if (hs->chf_bs == NULL) {
			perror("calloc(hs->chf_bs)");
			/* resetting the errno */
			errno = 0;
			return (3);
		} else {
			/* resetting the errno */
			errno = 0;
		}
		/*
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(hs->cp_offsets);
		hs->cp_offsets = calloc(hs->chf_number,
				sizeof (size_t));
		if (hs->cp_offsets == NULL) {
			perror("calloc(hs->cp_offsets)");
			/* resetting the errno */
			errno = 0;
			return (4);
		} else {
			/* resetting the errno */
			errno = 0;
		}
		/*
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(hs->cp_sizes);
		hs->cp_sizes = calloc(hs->chf_number,
				sizeof (size_t));
		if (hs->cp_sizes == NULL) {
			perror("calloc(hs->cp_sizes)");
			/* resetting the errno */
			errno = 0;
			return (5);
		} else {
			/* resetting the errno */
			errno = 0;
		}
		/*
		 * We want the next prime following the size of the universum
		 * to be equal to the largest prime number that can fit
		 * into the 32 bit unsigned integer
		 */
		hs->npu_size = 4294967291; /* a prime */
		hs->cp_offsets[0] = 0;
		hs->cp_sizes[0] = (*new_size) / hs->chf_number;
		if (hs->cp_sizes[0] == 0) {
			fprintf(stderr, "\nWarning: The requested size of "
					"the hash table (%zu) is too small."
					"\nIt will now be adjusted.\n\n",
					(*new_size));
			hs->cp_sizes[0] = 1;
		}
		hs->cp_sizes[0] = (size_t)
			(next_prime((ull)(hs->cp_sizes[0])));
		(*new_size) = hs->cp_sizes[0];
		hs->chf_as[0] = (unsigned_integral_type)(random()) %
			(hs->npu_size - 1) + 1;
		hs->chf_bs[0] = (unsigned_integral_type)(random()) %
			hs->npu_size;
		if (verbosity_level > 1) {
			printf("The Cuckoo hash function parameters:\n");
			printf("0: {a = %u, b = %u, offset = %zu, "
					"size = %zu}\n", hs->chf_as[0],
					hs->chf_bs[0], hs->cp_offsets[0],
					hs->cp_sizes[0]);
		}
		for (i = 1; i < hs->chf_number; ++i) {
			hs->cp_offsets[i] = hs->cp_offsets[i - 1]
				+ hs->cp_sizes[i - 1];
			hs->cp_sizes[i] = (size_t)(
					next_prime((ull)(hs->
							cp_sizes[i - 1])));
			(*new_size) += hs->cp_sizes[i];
			hs->chf_as[i] = (unsigned_integral_type)(random()) %
				(hs->npu_size - 1) + 1;
			hs->chf_bs[i] = (unsigned_integral_type)(random()) %
				hs->npu_size;
			if (verbosity_level > 1) {
				printf("%u: {a = %u, b = %u, offset = %zu, "
						"size = %zu}\n", i,
						hs->chf_as[i], hs->chf_bs[i],
						hs->cp_offsets[i],
						hs->cp_sizes[i]);
			}
		}
		/*
		 * adjusting the total size of the memory
		 * allocated for the hash settings
		 */
		hs->allocated_size = sizeof (hash_settings) +
			(sizeof (unsigned_integral_type) * 2 +
			sizeof (size_t) * 2 - 1) * hs->chf_number;
		if (verbosity_level > 1) {
			printf("The new hash table size: %zu\n", (*new_size));
		}
		return (0);
	} else { /* the double hashing */
		if (verbosity_level > 1) {
			printf("The selected collision resolution technique: "
					"double hashing\n");
		}
		if ((*new_size) == 0) {
			fprintf(stderr, "\nWarning: The requested size of "
					"the hash table (%zu) is too small."
					"\nIt will now be adjusted.\n\n",
					(*new_size));
			(*new_size) = 1;
		}
		(*new_size) = (unsigned_integral_type)(next_prime((ull)
					(*new_size)));
		if (verbosity_level > 1) {
			printf("The new hash table size: %zu\n", (*new_size));
		}
		hs->phf_max = (unsigned_integral_type)(*new_size);
		/*
		 * A wise choice according to Donald Knuth.
		 * If the hash table size is a prime, it is possible that
		 * the size - 2 will also be a prime, but it is impossible
		 * for size - 1 to be a prime, because it is surely even.
		 */
		hs->shf_max = (unsigned_integral_type)((*new_size) - 2);
		return (0);
	}
}

/**
 * A function which deallocates the memory used by the hash settings.
 *
 * @param
 * hs		The hash settings to be deallocated.
 *
 * @return	If this function call was successful, 0 is returned.
 * 		If the hash setting has not been allocated yet,
 * 		this function returns (-1);
 */
int hs_deallocate (hash_settings *hs) {
	/* If there is nothing to be done... */
	if (hs == NULL) {
		return (-1);
	} else {
		/*
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(hs->chf_as);
		hs->chf_as = NULL;
		/*
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(hs->chf_bs);
		hs->chf_bs = NULL;
		/*
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(hs->cp_offsets);
		hs->cp_offsets = NULL;
		/*
		 * it is always safe to delete the NULL pointer,
		 * so we need not to check for it
		 */
		free(hs->cp_sizes);
		hs->cp_sizes = NULL;
		free(hs);
		hs = NULL;
		return (0);
	}
}

/**
 * A function which checks whether the provided edge record is empty.
 *
 * @param
 * er		the edge record to be checked
 *
 * @return	If the edge record is empty, 1 is returned.
 * 		Otherwise, 0 is returned.
 */
int er_empty (const edge_record er) {
	if ((er.source_node == 0) && (er.target_node == 0)) {
		return (1);
	} else {
		return (0);
	}
}

/**
 * A function which checks whether the provided edge record is vacant.
 * Note that the empty edge record and vacant edge record
 * is not exactly the same thing!
 * Empty edge record has both the source and the target nodes set to zero.
 * Vacant edge record only needs to have the source node set to zero,
 * while the target node can have any value.
 * This means that all the empty edge records are also vacant.
 * But not every vacant edge record is empty.
 * Vacant edge records are used by the double hashing
 * collision resolution technique to indicate an edge record,
 * which has been deleted and is not occupied right now.
 * When using the Cuckoo hashing collision resolution technique,
 * the utilization of the vacant edge records is not necessary,
 * because the number of the Cuckoo hash functions is constant
 * and when we manipulate the hash table we always examine all of them.
 *
 * @param
 * er		the edge record to be checked
 *
 * @return	If the edge record is vacant, 1 is returned.
 * 		Otherwise, 0 is returned.
 */
int er_vacant (const edge_record er) {
	if (er.source_node == 0) {
		return (1);
	} else {
		return (0);
	}
}

/**
 * A function which dumps the contents of the hash table
 * to a FILE * type of stream. It will print only
 * the occupied records of the hash table.
 *
 * @param
 * stream	A stream to which the contents of the hash table
 * 		will be dumped.
 * @param
 * tedge_size	The current size of the hash table.
 * @param
 * tedge	The hash table which contents will be dumped.
 *
 * @return	If the function successfully finishes, it returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int ht_dump (FILE *stream,
		unsigned_integral_type tedge_size,
		const edge_record *tedge) {
	unsigned_integral_type i = 0; /* iteration variable */
	/* the current number of dumped records */
	unsigned_integral_type count = 0;
	fprintf(stream, "Dumping the hash table contents.\n");
	for (; i < tedge_size; ++i) {
		/* if the current edge record is not empty */
		if (er_empty(tedge[i]) == 0) {
			++count;
			/* we print it */
			fprintf(stream, "{%u}[%d, %d], ", i,
					tedge[i].source_node,
					tedge[i].target_node);
			/* printing 10 records per line */
			if (count % 10 == 9) {
				fprintf(stream, "\n");
			}
		}
	}
	fprintf(stream, "\nDump complete.\n");
	return (0);
}

/**
 * Primary hash function.
 * This function is used to determine the initial lookup place
 * in the hash table.
 *
 * @param
 * source_node	the first part of the hash key
 * @param
 * letter	the second part of the hash key
 * @param
 * hs		the hash settings to use
 *
 * @return	This is a hash function, so it always returns the hash value.
 */
size_t primary_hf (signed_integral_type source_node,
		character_type letter,
		const hash_settings *hs) {
	unsigned long long key =
		(unsigned long long)(source_node) ^
		((unsigned long long)(letter) << 32);
	/* this hash function is based on the division method */
	return ((size_t)(key % hs->phf_max));
}

/**
 * Secondary hash function.
 * This function is used to determine the size of the shift interval
 * for repeated lookups in case of previous failure.
 *
 * @param
 * source_node	the first part of the hash key
 * @param
 * letter	the second part of the hash key
 * @param
 * hs		the hash settings to use
 *
 * @return	This is a hash function, so it always returns the hash value.
 */
size_t secondary_hf (signed_integral_type source_node,
		character_type letter,
		const hash_settings *hs) {
	unsigned long long key =
		(unsigned long long)(source_node) ^
		((unsigned long long)(letter) << 32);
	/*
	 * This hash function is also based on the division method.
	 * The + 1 at the end is essential, because it ensures that
	 * the value of this hash function will always be positive,
	 * which is a key property for the shift interval.
	 */
	return ((size_t)(key % hs->shf_max + 1));
}

/**
 * The Cuckoo hash function.
 *
 * This function is used to determine the hash table entry
 * using "index".th Cuckoo hash function.
 *
 * @param
 * index	the index of the Cuckoo hash function to use
 * @param
 * source_node	the first part of the hash key
 * @param
 * letter	the second part of the hash key
 * @param
 * hs		the hash settings to use
 *
 * @return	This is a hash function, so it always returns the hash value.
 */
size_t cuckoo_hf (size_t index,
		signed_integral_type source_node,
		character_type letter,
		const hash_settings *hs) {
	/*
	 * we combine the two parts of the hash key
	 * using the eXclusive OR
	 */
	unsigned long long key =
		(unsigned long long)(source_node) ^
		((unsigned long long)(letter) << 32);
	/* this hash function is also based on the division method */
	return (size_t)(((
				(unsigned long long)(hs->chf_as[index]) *
				key +
				(unsigned long long)(hs->chf_bs[index])) %
				(unsigned long long)(hs->npu_size)) %
			(unsigned long long)(hs->cp_sizes[index]) +
			(unsigned long long)(hs->cp_offsets[index]));
}
