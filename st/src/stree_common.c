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
 * Suffix tree common functions implementation.
 * This file contains the implementation of the common functions,
 * which are used for the construction
 * of the suffix tree in the memory.
 */
#include "suffix_tree_common.h"

#include <errno.h>
#include <fcntl.h>
#include <iconv.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* constants */

/**
 * the number of extra characters allocated for the text
 *
 * These extra characters are: the first character ((*text)[0]),
 * the terminating character ($) and the terminating null character
 * (or all the allocated characters except for the "real" characters).
 */
const size_t extra_allocated_characters = 3;

/* reading function */

/**
 * A function which reads the text from file and stores it in memory.
 *
 * @param
 * file_name	the name of the input file from which the text will be read
 * @param
 * input_file_encoding	the character encoding used in the input file
 * @param
 * internal_text_encoding	the encoding used in the internal
 * 				representation of the text in memory
 * @param
 * text		(*text) will be replaced with memory address where the
 * 		gathered text is stored
 * @param
 * length	(*length) will be replaced with the total number of "real"
 * 		characters that are present in the memory at the address
 * 		(*text). This number does not not include the first character
 * 		((*text)[0]), the terminating character ($) and the terminating
 * 		null character.
 *
 * @return	If the reading was successful, this function returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int text_read (const char *file_name,
		/*
		 * We expect the file_name to consist of standard (short)
		 * characters only.
		 */
		const char *input_file_encoding,
		char **internal_text_encoding,
		character_type **text,
		size_t *length) {
	/* the conversion descriptor used by the iconv */
	iconv_t cd = NULL; /* iconv_t is just a typedef for void* */
	/* the variables used by the iconv */
	char *inbuf = NULL;
	char *outbuf = NULL;
	size_t inbytesleft = 0;
	size_t outbytesleft = 0;
	/* the return value of the iconv */
	size_t retval = 0;
	/* By default, we suppose that the input file encoding is UTF-8. */
	const char *fromcode = "UTF-8";
	/*
	 * The encoding used in the memory representation. It will be
	 * determined later according to the size of the character_type.
	 */
	char *tocode = NULL;
	/* the already mentioned size of the character_type */
	size_t character_type_size = sizeof (character_type);
	/* the file descriptor which will be used to read the input file */
	int fd = 0;
	/* the size of the input file */
	size_t file_size = 0;
	/*
	 * The current estimation of the number of characters
	 * in the input file. In the beginning, we set it to the maximum
	 * possible value, which is equal to the file size. Later, we will
	 * adjust this estimation to make it precise.
	 */
	size_t current_length = 0;
	/*
	 * The buffer used when reading the input file.
	 * It will be dynamically allocated (and deallocated).
	 */
	char *buffer = NULL;
	void *tmp_pointer = NULL;
	/* the size of this buffer */
	size_t buffer_size = 8388608; /* 8 MiB (2^23 bytes) */
	/* the number of bytes read by one function call to read() */
	ssize_t bytes_read = 0;
	/* the number of bytes read during this entire function */
	size_t total_bytes_read = 0;
	/*
	 * number of unused bytes in the input buffer
	 * used in the last call to the iconv function
	 */
	size_t unused_input_bytes = 0;
	/*
	 * According to the C specification, the non-listed members
	 * of the struct are initialized to "zero-like" values
	 * automatically when only some of the first elements
	 * are present in the initialization.
	 *
	 * But still, at least gcc produces
	 * an unpleasant warning message here.
	 *
	 * That's why we use the designated initializers,
	 * an alternative which does not produce gcc warnings.
	 */
	struct stat stat_struct = {.st_dev = 0};
	if (input_file_encoding != NULL) {
		/*
		 * If the input file character encoding was supplied,
		 * we set it accordingly.
		 */
		fromcode = input_file_encoding;
	}
	/* we try to open the input file for reading */
	fd = open(file_name, O_RDONLY);
	if (fd == -1) {
		perror("<file_name>: open");
		/* resetting the errno */
		errno = 0;
		return (1);
	}
	if (fstat(fd, &stat_struct) == (-1)) {
		perror("<file_name>: fstat");
		/* resetting the errno */
		errno = 0;
		return (2);
	}
	/* we get the current size of the input file */
	file_size = (size_t)(stat_struct.st_size);
	/*
	 * it is always safe to delete the NULL pointer,
	 * so we need not to check for it
	 */
	free(*text);
	(*text) = NULL;
	/*
	 * Our best estimation on the number of characters in the text file
	 * is the size in bytes of this text file. The actual number
	 * of characters might, of course, be smaller, but can never be larger.
	 */
	current_length = file_size;
	/*
	 * we want to allocate all the necessary memory for the text,
	 * including the memory for the extra characters
	 */
	(*text) = calloc(current_length + extra_allocated_characters,
			character_type_size);
	if ((*text) == NULL) {
		perror("text_read: calloc(text)");
		/* resetting the errno */
		errno = 0;
		return (3);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	/*
	 * we need not to free the buffer,
	 * because it is a local variable, which has been initialized to NULL
	 *
	 * Allocation of the memory for the buffer. Note that it doesn't need
	 * to be freed in advance, because it has been initialized to NULL.
	 */
	buffer = calloc(buffer_size, (size_t)(1));
	if (buffer == NULL) {
		perror("text_read: calloc(buffer)");
		/* resetting the errno */
		errno = 0;
		return (4);
	} else {
		/* resetting the errno */
		errno = 0;
	}
	/*
	 * We check the current size of the character_type
	 * and decide which encoding to use.
	 */
	if (character_type_size == 1) {
		/*
		 * we can not use Unicode, so by default we stick
		 * to the basic ASCII encoding
		 */
		tocode = "ASCII";
	} else if ((character_type_size > 1) && (character_type_size < 4)) {
		/*
		 * We can use limited Unicode (Basic Multilingual Plane,
		 * or BMP only). We prefer UCS-2 to UTF-16, because we would
		 * not like to deal with the byte order marks (BOM).
		 */
		/* we suppose we are on the little endian architecture */
		tocode = "UCS-2LE";
	} else { /* character_type_size >= 4 */
		/*
		 * We can use full Unicode (all the code points). We prefer
		 * UCS-4 to UTF-32, because we would not like to deal
		 * with the byte order marks (BOM).
		 */
		/* again, we suppose the little endian architecture */
		tocode = "UCS-4LE";
	}
	if ((**internal_text_encoding) == '\0') {
		/*
		 * we can safely skip the length test here,
		 * because we know exactly for which strings
		 * it is possible to be pointed to by tocode
		 */
		strcpy((*internal_text_encoding), tocode);
	} else { /* the caller has specified the internal text encoding */
		fprintf(stderr,	"Warning:\n========\nWe can not check "
				"whether the provided internal text "
				"encoding ('%s')\nis a single-byte encoding, "
				"variable length encoding or a multi-byte "
				"encoding.\nThe fixed internal character "
				"size is %zu byte(s), so in either of these "
				"cases\nyou might experience wrong "
				"interpretation of characters!\n\n",
				(*internal_text_encoding),
				character_type_size);
		tocode = (*internal_text_encoding);
	}
	/* we create the desired conversion descriptor */
	if ((cd = iconv_open(tocode, fromcode)) == (iconv_t)(-1)) {
		perror("text_read: iconv_open");
		/* resetting the errno */
		errno = 0;
		return (5);
	}
	/*
	 * we start writing at the address (*text) + 1,
	 * leaving the first ((*text)[0]) character intact
	 *
	 * The typedef to char* is necessary,
	 * because the text might be of the type wchar_t*
	 */
	outbuf = (char *)((*text) + 1);
	/* the maximum number of bytes to write */
	outbytesleft = current_length * character_type_size;
	/*
	 * the current text length will be determined precisely
	 * after reading the text from the file
	 */
	printf("Will now try to read the text from the file '%s'\n",
			file_name);
	printf("Selected file encoding: '%s'\n", fromcode);
	printf("Selected internal text encoding: '%s'\n",
			(*internal_text_encoding));
	printf("File size: %zu bytes (", file_size);
	print_human_readable_size(stdout, file_size);
	printf(")\n\n");
	/* while there are unread bytes in the input file */
	while ((bytes_read = read(fd, buffer + unused_input_bytes,
				buffer_size - unused_input_bytes)) > 0) {
		/*
		 * we set the iconv input buffer to the beginning
		 * of the buffer which has recently been read
		 */
		inbuf = buffer;
		/* the maximum number of input bytes to process */
		inbytesleft = unused_input_bytes + (size_t)(bytes_read);
		/*
		 * We increment the total number of bytes read so far,
		 * just to be able to tell whether we have read
		 * all the characters in the input file or not.
		 */
		total_bytes_read += (size_t)(bytes_read);
		/*
		 * we try to use iconv to convert the characters
		 * in the input buffer to the characters in the output buffer
		 */
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* resetting the number of unused bytes */
		unused_input_bytes = 0;
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			if (errno == EINVAL) { /* not really an error */
				/*
				 * An incomplete multi-byte sequence
				 * has been encountered at the end
				 * of the input buffer. We move it
				 * to the beginning of the input buffer
				 * for later processing.
				 */
				memmove(buffer, inbuf, inbytesleft);
				/* correcting the number of unused bytes */
				unused_input_bytes = inbytesleft;
				/* resetting the errno */
				errno = 0;
			} else {
				perror("text_read: iconv");
				/* resetting the errno */
				errno = 0;
				return (6);
			}
		}
	}
	/* we check whether the read has encountered an error */
	if (bytes_read == (-1)) {
		perror("text_read: read");
		/* resetting the errno */
		errno = 0;
		return (7);
	}
	if (unused_input_bytes != (size_t)(0)) {
		fprintf(stderr,	"Error: The last call to the function\n"
			"iconv"
			" did not convert all the provided input bytes.\n");
		return (8);
	}
	/*
	 * Freeing the memory allocated for the buffer.
	 * We can be sure that the buffer is not NULL here.
	 */
	free(buffer);
	buffer = NULL;
	/*
	 * Finally, we are able to determine the exact number of characters
	 * which have been read from the input file.
	 */
	current_length = (current_length * character_type_size -
			outbytesleft) / character_type_size;
	/* we delete the conversion descriptor used by the iconv */
	if (iconv_close(cd) == (-1)) {
		perror("text_read: iconv_close");
		/* resetting the errno */
		errno = 0;
		return (9);
	}
	/* if we were able to read the entire input file without any errors */
	if (total_bytes_read == file_size) {
		printf("Successfully read %zu bytes (", total_bytes_read);
		print_human_readable_size(stdout, total_bytes_read);
		printf("),\nwhich amount to %zu characters!\n",
				current_length);
		printf("Average character size: %2.3f bytes\n\n",
				(double)(total_bytes_read) /
				(double)(current_length));
	} else {
		fprintf(stderr,	"Error: Could not read "
				"the entire input file!\n"
				"Only %zu bytes (",
				total_bytes_read);
		print_human_readable_size(stderr, total_bytes_read);
		fprintf(stderr,	") out of %zu bytes (",
				file_size);
		print_human_readable_size(stderr, file_size);
		fprintf(stderr,	") have been read!\n");
		return (10);
	}
	/* we close the file descriptor used for reading the input file */
	if (close(fd) == -1) {
		perror("<file_name>: close");
		/* resetting the errno */
		errno = 0;
		return (11);
	}
	/*
	 * We temporarily adjust the current length of the text
	 * so that it includes the extra allocated characters.
	 * Then we use this length for reallocation of the text memory.
	 */
	(*length) = (unsigned_integral_type)(current_length +
			extra_allocated_characters);
	printf("Will now try to reallocate the memory for the text:\n"
			"final size: "
			"%zu characters of %zu bytes (totalling %zu bytes, ",
			(*length), character_type_size,
			(*length) * character_type_size);
	print_human_readable_size(stdout, (*length) * character_type_size);
	printf(").\n");
	tmp_pointer = realloc((*text), (*length) * character_type_size);
	if (tmp_pointer == NULL) {
		perror("text_read: text: realloc");
		/* resetting the errno */
		errno = 0;
		return (12);
	} else {
		/*
		 * Despite that the call to the realloc seems
		 * to have been successful, we reset the errno,
		 * because at least on Mac OS X
		 * it might have changed.
		 */
		errno = 0;
		(*text) = tmp_pointer;
	}
	printf("Successfully reallocated!\n\n");
	printf("Text statistics:\n----------------\n");
	printf("Total disk space read for the text: %zu bytes (",
			total_bytes_read);
	print_human_readable_size(stdout, total_bytes_read);
	printf(")\nTotal amount of memory used by the text: %zu bytes (",
			(*length) * character_type_size);
	print_human_readable_size(stdout, (*length) * character_type_size);
	printf(")\nThe current text representation in the memory consumes\n"
			"%2.2f%% of the disk space read for the text.\n\n",
			100 * (double)((*length) * character_type_size) /
			(double)(total_bytes_read));
	/*
	 * We set the length back to be the number
	 * of the "real" characters in the text.
	 */
	(*length) = (unsigned_integral_type)(current_length);
	/*
	 * we do not intend to use (*text)[0], that's why we fill it
	 * with "blank" (space) character
	 */
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	(**text) = L' ';
#else
	(**text) = ' ';
#endif
	/*
	 * we replace the character just after the last "real" character
	 * of the text by the terminating character ($)
	 */
	(*text)[current_length + 1] = terminating_character;
	/*
	 * We want the string to be safely printable, so we change
	 * the last character of the memory allocated for the text
	 * to the standard terminating null character.
	 */
#ifdef	SUFFIX_TREE_TEXT_WIDE_CHAR
	(*text)[current_length + 2] = L'\0';
#else
	(*text)[current_length + 2] = '\0';
#endif
	return (0);
}

/* printing functions */

/**
 * A function which prints the edge from parent to the child
 * within a suffix tree in human readable format.
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
 * childs_offset	The position in the text of the first letter
 * 			of the leftmost "branching occurrence" of the string
 * 			composed of the letters on the path
 * 			from the root to the child.
 * @param
 * internal_character_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix tree.
 * @param
 * text		the actual underlying text of the suffix tree
 *
 * @return	If we could successfully print the edge, 0 is returned.
 * 		In case of an error, a positive error number is returned.
 */
int st_print_edge (FILE *stream,
		int print_terminating_character,
		signed_integral_type parent,
		signed_integral_type child,
		signed_integral_type childs_suffix_link,
		unsigned_integral_type parents_depth,
		unsigned_integral_type childs_depth,
		size_t log10bn,
		size_t log10l,
		size_t childs_offset,
		const char *internal_character_encoding,
		const character_type *text) {
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
	if ((cd = iconv_open(tocode, internal_character_encoding)) ==
			(iconv_t)(-1)) {
		perror("st_print_edge: iconv_open");
		/* resetting the errno */
		errno = 0;
		return (2);
	}
	/* we print the edge label */
	if (text_length < 33) {
		inbuf = (char *)(text + childs_offset + parents_depth);
		if (print_terminating_character == 0) {
			sprintf(fs_buffer, "--\"%%s\"(%zu)-->", text_length);
			inbytesleft = text_length * character_type_size;
		} else {
			/*
			 * we do not want the terminating character
			 * to be printed from the memory as is, so
			 * we print the dollar sign ($) instead
			 */
			sprintf(fs_buffer, "--\"%%s$\"(%zu)-->", text_length);
			inbytesleft = (text_length - 1) * character_type_size;
		}
		outbuf = text_buffer;
		outbytesleft = 511;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("st_print_edge: iconv 1");
			/* resetting the errno */
			errno = 0;
			return (3);
		}
		/* we make sure that the string is safely printable */
		(*outbuf) = 0;
		fprintf(stream, fs_buffer, text_buffer);
	} else { /* text_length >= 33 */
		inbuf = (char *)(text + childs_offset + parents_depth);
		inbytesleft = 15 * character_type_size;
		outbuf = text_buffer;
		outbytesleft = 255;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("st_print_edge: iconv 2");
			/* resetting the errno */
			errno = 0;
			return (4);
		}
		/* we make sure that the string is safely printable */
		(*outbuf) = 0;
		inbuf = (char *)(text + childs_offset + childs_depth - 15);
		if (print_terminating_character == 0) {
			sprintf(fs_buffer, "--\"%%s...%%s\"(%zu)-->",
					text_length);
			inbytesleft = 15 * character_type_size;
		} else {
			/*
			 * we do not want the terminating character
			 * to be printed from the memory as is, so
			 * we print the dollar sign ($) instead
			 */
			sprintf(fs_buffer, "--\"%%s...%%s$\"(%zu)-->",
					text_length);
			inbytesleft = 14 * character_type_size;
		}
		outbuf = text_buffer + 256;
		outbytesleft = 255;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("st_print_edge: iconv 3");
			/* resetting the errno */
			errno = 0;
			return (5);
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
		perror("st_print_edge: iconv_close");
		/* resetting the errno */
		errno = 0;
		return (6);
	}
	if (errno != 0) {
		perror("st_print_edge: printf");
		/* resetting the errno */
		errno = 0;
		return (7);
	}
	return (0);
}

/**
 * A function which prints the provided single suffix
 * into the provided FILE * type stream.
 *
 * @param
 * stream	the FILE * type stream to which the provided suffix
 * 		will be printed
 * @param
 * suffix_index	the index of the suffix to be printed
 * @param
 * internal_character_encoding	The character encoding used in the internal
 * 				representation of the text for the suffix.
 * @param
 * suffix	the actual suffix to be printed
 * @param
 * suffix_length	the length of the suffix to be printed
 *
 * @return	If this function call is successful, this function returns 0.
 * 		Otherwise, a positive error number is returned.
 */
int st_print_single_suffix (FILE *stream,
		size_t suffix_index,
		const char *internal_character_encoding,
		const character_type *suffix,
		size_t suffix_length) {
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
	if ((cd = iconv_open(tocode, internal_character_encoding)) ==
			(iconv_t)(-1)) {
		perror("st_print_single_suffix: iconv_open");
		return (1);
	}
	/* if the suffix is short enough to be printed entirely */
	if (suffix_length < 33) {
		inbuf = (char *)(suffix);
		/* we do not want to convert the terminating character ($) */
		inbytesleft = (suffix_length - 1) * character_type_size;
		outbuf = text_buffer;
		outbytesleft = 511;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("st_print_single_suffix: iconv (short suffix)");
			return (2);
		}
		/* we make sure that the string is safely printable */
		(*outbuf) = 0;
		fprintf(stream, "%zu: %s$ (%zu)\n", suffix_index,
				text_buffer, suffix_length);
	/*
	 * otherwise we have to print only the first 15 characters
	 * and the last 15 characters
	 */
	} else {
		inbuf = (char *)(suffix);
		/* we convert just the first 15 characters */
		inbytesleft = 15 * character_type_size;
		outbuf = text_buffer;
		outbytesleft = 255;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("st_print_single_suffix: "
					"iconv (long suffix, 1)");
			return (3);
		}
		/* we make sure that the string is safely printable */
		(*outbuf) = 0;
		inbuf = (char *)(suffix + suffix_length - 15);
		/* we do not want to convert the terminating character ($) */
		inbytesleft = 14 * character_type_size;
		outbuf = text_buffer + 256;
		outbytesleft = 255;
		retval = iconv(cd, &inbuf, &inbytesleft,
				&outbuf, &outbytesleft);
		/* if the iconv has encountered an error */
		if (inbytesleft > 0 || retval != 0) {
			perror("st_print_single_suffix: "
					"iconv (long suffix, 2)");
			return (4);
		}
		/* we make sure that the string is safely printable */
		(*outbuf) = 0;
		fprintf(stream, "%zu: %s...%s$ (%zu)\n", suffix_index,
				text_buffer, text_buffer + 256, suffix_length);
	}
	/* we delete the conversion descriptor used by the iconv */
	if (iconv_close(cd) == (-1)) {
		perror("st_print_single_suffix: iconv_close");
		return (5);
	}
	if (errno != 0) {
		perror("st_print_single_suffix: printf");
		/* resetting the errno */
		errno = 0;
		return (6);
	}
	return (0);
}

/**
 * A function which prints the memory usage statistics for the suffix tree.
 *
 * @param
 * length	the actual length of the underlying text in the suffix tree
 * 		(number of the "real" characters in the text)
 * @param
 * edges		the current number of edges in the suffix tree
 * @param
 * branching_nodes	the current number of branching nodes
 * 			in the suffix tree
 * @param
 * la_records		the current number of records
 * 			in the simple linear array of the suffix tree
 * @param
 * tedge_size		the current size of the table tedge
 * @param
 * tbranch_size		the current size of the table tbranch
 * @param
 * tnode_size		the current size of the table tnode
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
 * la_record_size	the size of the simple linear array record
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
int st_print_stats (size_t length,
		size_t edges,
		size_t branching_nodes,
		size_t la_records,
		size_t tedge_size,
		size_t tbranch_size,
		size_t tnode_size,
		size_t leaf_record_size,
		size_t edge_record_size,
		size_t branch_record_size,
		size_t la_record_size,
		size_t extra_allocated_memory_size,
		size_t extra_used_memory_size) {
	size_t allocated_size = extra_allocated_memory_size;
	size_t used_size = extra_used_memory_size;
	/*
	 * adjusting the number of memory cells allocated for the leaves
	 * by including the unused first cell (tleaf[0])
	 * and the terminating character ($)
	 */
	allocated_size += (length + 2) * leaf_record_size;
	/* adding the memory allocated for the table tedge */
	allocated_size += tedge_size * edge_record_size;
	/* We have to include the leading unused branching node tbranch[0] */
	allocated_size += (tbranch_size + 1) * branch_record_size;
	/* adding the memory allocated for the table tnode */
	allocated_size += tnode_size * la_record_size;
	/*
	 * adjusting the number of memory cells used by table tleaf
	 * by including the terminating character ($)
	 */
	used_size += (length + 1) * leaf_record_size;
	/* adding the memory used by the table tedge */
	used_size += edges * edge_record_size;
	/* adding the memory used by the table tbranch */
	used_size += branching_nodes * branch_record_size;
	/* adding the memory used by the table tnode */
	used_size += la_records * la_record_size;
	printf("\nSuffix tree statistics:\n-----------------------\n");
	if (leaf_record_size > 0) {
		printf("The leaf record size: %zu bytes\n", leaf_record_size);
	}
	if (edge_record_size > 0) {
		printf("The edge record size: %zu bytes\n", edge_record_size);
	}
	if (branch_record_size > 0) {
		printf("The branch record size: %zu bytes\n",
				branch_record_size);
	}
	if (la_record_size > 0) {
		printf("The simple linear array record size: %zu bytes\n",
				la_record_size);
	}
	printf("The text length (number of the \"real\" characters): %zu\n",
			length);
	if (leaf_record_size > 0) {
		printf("Size of the table tleaf:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
			(length + 2), leaf_record_size,
			(length + 2) * leaf_record_size);
		print_human_readable_size(stdout,
			(length + 2) * leaf_record_size);
		printf(").\n");
	}
	if (edges > 0) {
		printf("Number of edges: %zu\n", edges);
	}
	if (tedge_size > 0) {
		printf("Size of the table tedge:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
			tedge_size, edge_record_size,
			tedge_size * edge_record_size);
		print_human_readable_size(stdout,
			tedge_size * edge_record_size);
		printf(").\n");
		printf("Edges load factor: %2.2f%%\n", 100 *
			(double)(edges) / (double)(tedge_size));
	}
	if (branching_nodes > 0) {
		printf("Number of branching nodes: %zu\n", branching_nodes);
	}
	if (tbranch_size > 0) {
		printf("Size of the table tbranch:\n"
			"%zu cells of %zu bytes (totalling %zu bytes, ",
			tbranch_size, branch_record_size,
			tbranch_size * branch_record_size);
		print_human_readable_size(stdout,
			tbranch_size * branch_record_size);
		printf(").\n");
		printf("Branching nodes load factor: %2.2f%%\n", 100 *
			(double)(branching_nodes) / (double)(tbranch_size));
	}
	if (la_records > 0) {
		printf("Number of simple linear array records in use: %zu\n",
				la_records);
	}
	if (tnode_size > 0) {
		printf("Size of the table tnode: %zu\n", tnode_size);
		printf("Linear array records load factor: %2.2f%%\n", 100 *
			(double)(la_records) / (double)(tnode_size));
	}
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
	printf("Total amount of memory allocated: %zu bytes (",
			allocated_size);
	print_human_readable_size(stdout, allocated_size);
	printf(")\nwhich is %.3f bytes per input character.\n",
			(double)(allocated_size) / (double)(length));
	printf("Total amount of memory used: %zu bytes (", used_size);
	print_human_readable_size(stdout, used_size);
	printf(")\nwhich is %.3f bytes per input character.\n",
			(double)(used_size) / (double)(length));
	printf("Memory load factor: %2.2Lf%%\n\n", 100 *
			(long double)(used_size) /
			(long double)(allocated_size));
	return (0);
}
