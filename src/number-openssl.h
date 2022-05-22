/* Definitions of numeric types for XEmacs using the OpenSSL bignum library.
   Copyright (C) 2022 Jaakko Salomaa.
   Licensed reluctantly under GPLv3+ hoping this would be included in XEmacs.

This file is part of XEmacs.

XEmacs is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs.  If not, see <http://www.gnu.org/licenses/>. */

/* Synched up with: Not in FSF. */

/* This library defines the following types:
   bignum       = BIGNUM *

   OpenSSL does not include support for ratios or bigfloats.

   Even though number-mp.h and number-gmp.h have macros that evaluate their
   parameters multiple times, this implementation has none. All non-trivial
   pieces, as in those that wouldn't fit in a few lines, of the XEmacs bignum
   API are implemented as functions.
*/

#ifndef INCLUDED_number_openssl_h_
#define INCLUDED_number_openssl_h_

BEGIN_C_DECLS
#include <openssl/bn.h>
END_C_DECLS

typedef BIGNUM *bignum;

extern void init_number_openssl(void);


/********************************* Bignums **********************************/

#define HAVE_BIGNUM 1

/***** Bignum: basic functions *****/
#define bignum_init(b)                                                        \
  do {                                                                        \
    bignum *bn_init_val__ = &(b);                                             \
    if (!(*bn_init_val__ = BN_new ())) bignum_memory_full ();                 \
    BN_zero (*bn_init_val__);                                                 \
  } while (0)

#define bignum_fini(b)              BN_free (b)
#define bignum_hashcode(b)          bignum_to_ulong (b)
#define bignum_evenp(b)             !BN_is_odd (b)
#define bignum_oddp(b)              BN_is_odd (b)
extern int bignum_sign(bignum);

/***** Bignum: size *****/
extern int bignum_fits_int_p(bignum);
extern int bignum_fits_uint_p(bignum);
extern int bignum_fits_long_p(bignum);
extern int bignum_fits_ulong_p(bignum);
extern int bignum_fits_llong_p(bignum);
extern int bignum_fits_ullong_p(bignum);

/***** Bignum: conversions *****/
#define bignum_size_decimal(b) /* From lisp.h:DECIMAL_PRINT_SIZE() */         \
  ((((2410824 * BN_num_bytes (b)) / 1000000) + 3) * MAX_ICHAR_LEN)
#define bignum_size_octal(b)   ((BN_num_bytes (b) + 2) * MAX_ICHAR_LEN * 3)
#define bignum_size_hex(b)     ((BN_num_bytes (b) + 2) * MAX_ICHAR_LEN * 2)
#define bignum_size_binary(b)  ((BN_num_bytes (b) + 2) * MAX_ICHAR_LEN * 8)

extern int bignum_to_int(bignum);
extern unsigned int bignum_to_uint(bignum);
extern long bignum_to_long(bignum);
extern unsigned long bignum_to_ulong(bignum);
extern long long bignum_to_llong(bignum);
extern unsigned long long bignum_to_ullong(bignum);
extern double bignum_to_double(bignum);
extern Bytecount bignum_to_string_openssl(Ibyte **, Bytecount, bignum,
                                          UINT_16_BIT);

/***** Bignum: converting assignments *****/
#define bignum_set(b1,b2)                                                     \
  do { if (!BN_copy (b1, b2)) bignum_memory_full (); } while (0)
extern int bignum_set_string(bignum, const char *, int);
extern void bignum_set_long(bignum, long);
extern void bignum_set_ulong(bignum, unsigned long);
extern void bignum_set_llong(bignum, long long);
extern void bignum_set_ullong(bignum, unsigned long long);
extern void bignum_set_double(bignum, double);

/***** Bignum: comparisons *****/
#define bignum_cmp(b1,b2)           BN_cmp (b1, b2)
#define bignum_lt(b1,b2)            (BN_cmp (b1, b2) == -1)
#define bignum_le(b1,b2)            (BN_cmp (b1, b2) <=  0)
#define bignum_eql(b1,b2)           (BN_cmp (b1, b2) ==  0)
#define bignum_ge(b1,b2)            (BN_cmp (b1, b2) >=  0)
#define bignum_gt(b1,b2)            (BN_cmp (b1, b2) ==  1)

/***** Bignum: arithmetic *****/
#define bignum_add(b,b1,b2)                                                   \
  do { if (!BN_add (b, b1, b2)) bignum_memory_full (); } while (0)
#define bignum_sub(b,b1,b2)                                                   \
  do { if (!BN_sub (b, b1, b2)) bignum_memory_full (); } while (0)
extern void bignum_abs(bignum, bignum);
extern void bignum_neg(bignum, bignum);
extern void bignum_mul(bignum, bignum, bignum);
extern void bignum_div(bignum, bignum, bignum);
extern void bignum_mod(bignum, bignum, bignum);

extern int bignum_divisible_p(bignum, bignum);
extern void bignum_ceil(bignum, bignum, bignum);
extern void bignum_floor(bignum, bignum, bignum);
extern void bignum_pow(bignum, bignum, unsigned long);
extern void bignum_gcd(bignum, bignum, bignum);
extern void bignum_lcm(bignum, bignum, bignum);

UINT_16_BIT bignum_div_rem_uint_16_bit(bignum, bignum, UINT_16_BIT);

/***** Bignum: bit manipulations *****/
/* This will error if BN_get_bits(b) < b, but that doesn't really matter. */
#define bignum_clrbit(b,bit)        BN_clear_bit (b, bit)
#define bignum_testbit(b,bit)       BN_is_bit_set (b, bit)
#define bignum_setbit(b,bit)                                                  \
  do { if (!BN_set_bit (b, bit)) bignum_memory_full (); } while (0)
#define bignum_lshift(to,from,bits)                                           \
  do { if (!BN_lshift (to, from, bits)) bignum_memory_full (); } while (0)
#define bignum_rshift(to,from,bits)                                           \
  do { if (!BN_rshift (to, from, bits)) bignum_memory_full (); } while (0)

extern void bignum_and(bignum, bignum, bignum);
extern void bignum_ior(bignum, bignum, bignum);
extern void bignum_xor(bignum, bignum, bignum);
extern void bignum_not(bignum, bignum);

/***** Bignum: random numbers *****/
#define bignum_random_seed(s) do {} while (0) /* OpenSSL seeds itself. */
extern void bignum_random(bignum, bignum);

/***** Bignum: utility *****/
extern void bignum_memory_full(void);

#endif /* INCLUDED_number_openssl_h_ */
