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
 * The primality test-related declarations.
 * This file contains the declarations of the functions,
 * which are used for the primality testing.
 * The primality testing is used by the hash table-related functions.
 */
#ifndef	PRIMALITY_TEST_HEADER
#define	PRIMALITY_TEST_HEADER

/** the typedef for shorter code */
typedef unsigned long long ull;

ull ullrandom (ull upper_bound);

ull mul_modular (ull a, ull b, ull c);
ull pow_modular (ull a, ull b, ull c);

ull fermat_test (ull number, ull repeats);
ull mr_test (ull number, ull repeats);

ull next_prime (ull number);

#endif /* PRIMALITY_TEST_HEADER */
