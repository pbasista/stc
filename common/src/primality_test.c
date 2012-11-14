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
 * The primality test-related functions implementation.
 * This file contains the implementation of the functions,
 * which are used for the primality testing.
 * The primality testing is used by the hash table-related functions.
 */

/* feature test macros */

/** A macro necessary for the function random. */
#define	_BSD_SOURCE

#include "primality_test.h"

#include <stdlib.h>

/**
 * A function which tries to generate a 64 bit random number,
 * provided that the standard random number generator generates
 * 32 bit random numbers.
 *
 * @param
 * upper_bound	The upper bound for the generated random numbers,
 * 		which will always stay within the interval
 * 		[0, upper_bound - 1].
 *
 * @return	The generated random number.
 */
ull ullrandom (ull upper_bound) {
	ull result = 0;
	/* the lower order 32 bits */
	long int low_bits = random();
	/* the higher order 32 bits */
	long int high_bits = random();
	/*
	 * we combine the two 32 bit random numbers
	 * into one 64 bit random number
	 */
	result = (((ull)(high_bits) << 32) | (ull)(low_bits)) % upper_bound;
	return (result);
}

/**
 * A function which calculates the expression: (a*b) modulo c
 * while taking into account that a*b might overflow
 * the type unsigned long long.
 *
 * @param
 * a		The a operand in the expression (a*b) modulo c.
 * @param
 * b		The b operand in the expression (a*b) modulo c.
 * @param
 * c		The c operand in the expression (a*b) modulo c.
 *
 * @return	The result of operation (a*b) modulo c.
 */
ull mul_modular (ull a, ull b, ull c) {
	ull result = 0; /* this value will become the final result */
	ull add = a % c; /* the current addend */
	/* while the multiplier is positive */
	while (b > 0) {
		/*
		 * We bitwisely examine the multiplier
		 * and for every bit set to 1 we add the corresponding multiple
		 * of the addend to the result.
		 */
		if (b % 2 == 1) { /* if the current multiplier is odd */
			/*
			 * we adjust the result
			 * by adding the current addend to it
			 */
			result = (result + add) % c;
		}
		/*
		 * note that we need not to use parentheses,
		 * because the associative rule for the multiplication,
		 * division and modulo ensures that the expression
		 * is evaluated from the left to the right,
		 * e.g. (add * 2) % c
		 */
		add = add * 2 % c; /* doubling the addend */
		b /= 2; /* dividing the multiplier by 2 */
	}
	return (result);
}

/**
 * A function which calculates the expression: (a^b) modulo c.
 *
 * @param
 * a		The a operand in the expression (a^b) modulo c.
 * @param
 * b		The b operand in the expression (a^b) modulo c.
 * @param
 * c		The c operand in the expression (a^b) modulo c.
 *
 * @return	The result of operation (a^b) modulo c.
 */
ull pow_modular (ull a, ull b, ull c) {
	ull result = 1; /* this value will become the final result */
	ull mul = a; /* the current multiplicand */
	/* while the exponent is positive */
	while (b > 0) {
		/*
		 * We bitwisely examine the exponent
		 * and for every bit set to 1 we multiply the result
		 * by the corresponding power of the base.
		 */
		if (b % 2 == 1) { /* if current the exponent is odd */
			/*
			 * we adjust the result for the odd exponent
			 * by multiplying it by the current multiplicand
			 */
			result = mul_modular(result, mul, c);
		}
		mul = mul_modular(mul, mul, c); /* squaring the multiplicand */
		b /= 2; /* dividing the exponent by 2 */
	}
	return (result);
}

/**
 * A function which tries to test whether the provided number is a prime.
 * It attempts to find the Fermat's witness, which will testify that
 * the provided number is a composite number.
 * This function tries to perform the test based on Fermat's little theorem
 * at most "repeats" times.
 *
 * @param
 * number	The number which is about to be tested for primality.
 * @param
 * repeats	The number of consecutive tests to perform.
 *
 * @return	If a Fermat's witness is found, it is returned.
 * 		Otherwise, zero is returned.
 */
ull fermat_test (ull number, ull repeats) {
	ull i = 0;
	ull nd1 = number - 1; /* the number descreased by one */
	ull witness = 0;
	for (i = 0; i < repeats; ++i) {
		/* we want the witness to be greater than 1 */
		witness = ullrandom(nd1) + 1;
		if (pow_modular(witness, nd1, number) != 1) {
			return (witness); /* surely a composite number */
		}
	}
	return (0); /* probably a prime */
}

/**
 * A function which tries to test whether the provided number is a prime.
 * It tries to perform the Miller-Rabin primality test and it is repeated
 * at most "repeats" times. The idea is to find either the Fermat's
 * witness or the Riemann's witness, any of which will testify that
 * the provided number is a composite number.
 *
 * The probability of incorrect presentation of a number
 * as a composite number (false negative) is zero.
 * The probability of incorrect presentation of a number
 * as a prime number (false positive) is (1/4) ^ "repeats".
 *
 * @param
 * number	The number which is about to be tested for primality.
 * @param
 * repeats	The number of consecutive tests to perform.
 *
 * @return	If any kind of witness is found, it is returned.
 * 		Otherwise, zero is returned.
 */
ull mr_test (ull number, ull repeats) {
	ull i = 0;
	ull witness = 0;
	ull nd1 = number - 1; /* the number descreased by one */
	ull rex = 0; /* the exponent used when finding the Riemann's witness */
	/*
	 * the result from the pow_modular function
	 * used when finding the Riemann's witness
	 */
	ull mod = 0;
	for (i = 0; i < repeats; ++i) {
		/* we want the witness to be greater than 1 */
		witness = ullrandom(nd1) + 1;
		if (pow_modular(witness, nd1, number) == 1) {
			/* the Fermat's test was successful */
			rex = nd1;
			/* while the exponent is divisible by 2 */
			while (rex % 2 == 0) {
				rex /= 2; /* rex = rex / 2 */
				mod = pow_modular(witness, rex, number);
				if (mod == nd1) {
					/*
					 * the current witness is not
					 * the Riemann's witness
					 */
					break;
				} else if (mod != 1) {
					/*
					 * we have found
					 * the Riemann's witness
					 */
					return (witness);
				}
				/*
				 * if we got here, mod == 1
				 * and we can continue
				 */
			}
			/*
			 * If we got here, it means that we have tried
			 * all the possible exponents, but for none of them
			 * it could be proved that the current witness
			 * is a Riemann's witness. So, we try another one.
			 */
		} else {
			/* we have found the Fermat's witness */
			return (witness);
		}
	}
	/*
	 * If we got here, it means that we have tried "repeats" numbers,
	 * but none of them was found to be a Fermat's or Riemann's witness
	 * for the provided number. So, it means that this number
	 * is *probably* a prime.
	 */
	return (0); /* probably a prime */
}

/**
 * A function which tries to find the next *probable* prime greater than
 * the provided number. The number found is a prime with very high
 * probability of 1 - (1/4)^100.
 *
 * @param
 * number	The primality testing will begin at the (number + 1).
 *
 * @return	The next *probable* prime greater than the provided number.
 */
ull next_prime (ull number) {
	while (mr_test(++number, (ull)(100)) != 0) {
	}
	return (number);
}
