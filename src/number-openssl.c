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

#ifdef __ROUND2__ /* This file includes itself, search for __ROUND2__. */
#  define C_(a,b) a ## b
#  define C(a,b) C_ (a, b)
#  define C3_(a,b,c) a ## b ## c
#  define C3(a,b,c) C3_ (a, b, c)
#else

#include <config.h>
#include "lisp.h"
#include <openssl/bio.h>
#include <openssl/err.h>

/* Uncomment this and run make to run the unit tests. They will be ran during
   the temacs phase and terminate the compilation with an error exit. */
/* #include "../tests/number-openssl-tests.c" */

static bignum bn_min_int, bn_min_long, bn_min_llong;
/* Since XEmacs assumes everywhere to be single-threaded, so here as well. */
static bignum scratch_static, scratch_static2;
static BN_CTX *ctx_static;
static BIO *bio_writer_static;

#define HANDLE_OP_ERROR(expr)                                                 \
  do                                                                          \
    {                                                                         \
      if (expr)                                                               \
        {                                                                     \
          ERR_clear_error ();                                                 \
          memory_full ();                                                     \
        }                                                                     \
    }                                                                         \
  while (0)

#ifdef WORDS_BIGENDIAN
#  define BN2BIN(b,to,len)   BN_bn2binpad (b, (unsigned char *) (to), len)
#  define BIN2BN(b,from,len) BN_bin2bn ((unsigned char *) (from), len, b)
#else
#  define BN2BIN(b,to,len)   BN_bn2lebinpad (b, (unsigned char *) (to), len)
#  define BIN2BN(b,from,len) BN_lebin2bn ((unsigned char *) (from), len, b)
#endif

int
bignum_sign(bignum b)
{
  return BN_is_negative (b) ? -1 : BN_is_zero (b) ? 0 : 1;
}

#endif /* !__ROUND2__ */
#ifdef __ROUND2_BIGNUM_FITS__
/*
  Here and further below in this file we generate repetitive functions by
  keeping the normal flow of the file content inside blocks where a __ROUND2__
  preprocessor define isn't defined.

  By defining __ROUND2__ and an additional per-block define, it's possible to
  have a block of function definitions that are parametrized by additional
  #defines that work like function-like macro parameters. Thus, instead of,
  simplifiedly, saying

  #define FOO(a) void a(void) { ... }

  we say

  #define A [value]
  #include "number-openssl.c"

  Why? So that the compiler gets the line numbers right. As you, my dear
  reader, probably know, the compiler marks a macro expanded multi line
  function to be defined in the line of the macro invocation. As a result, the
  compiler error messages are an unholy mess along the lines of "in the
  expansion of macro FOO, (something inexplicable)". As well, #if macros can be
  used inside the bodies of the included parts, unlike in macro expansions.

  Further, _debugging works_. The developer - in this case, me - can step into
  the functions and see and control what line the control flow is in.
*/
int
C3 (bignum_fits_, TYPE_ALIAS, _p)(bignum b)
{
  return BN_num_bits (b) <= (int) sizeof (TYPE) * 8 - 1 ||
         (BN_is_negative (b) && bignum_ge (b, C (bn_min_, TYPE_ALIAS)));
}

int
C3 (bignum_fits_u, TYPE_ALIAS, _p)(bignum b)
{
  return !BN_is_negative (b) && BN_num_bytes (b) <= (int) sizeof (TYPE);
}

#endif
#ifndef __ROUND2__
#define __ROUND2__
#define __ROUND2_BIGNUM_FITS__

#define TYPE       int
#define TYPE_ALIAS int
#include "number-openssl.c"
#undef TYPE_ALIAS
#undef TYPE

#define TYPE       long
#define TYPE_ALIAS long
#include "number-openssl.c"
#undef TYPE_ALIAS
#undef TYPE

#define TYPE       long long
#define TYPE_ALIAS llong
#include "number-openssl.c"
#undef TYPE_ALIAS
#undef TYPE

#undef __ROUND2_BIGNUM_FITS__
#undef __ROUND2__

/*
  If the bignums overflow the returned types, the options would be either to
  copy the whole thing into an allocated buffer and return the last bytes, to
  calculate a per integer size modulo that fits the return type, or to write
  the bignum (as a hexadecimal) to a BIO stream and then return the last bits.

  We take the third option, because that doesn't have either extra memory
  allocations or crunching a division through the whole bignum just to get the
  modulo. The bignum lib doesn't have any optimizations for power of two
  divisors (I checked.)
*/
static unsigned long long bignum_bio_number;

static int
bignum_bio_write(BIO *unused, const char *str, int size)
{
  (void) unused;
  
  if (*str == '-')
    {
      str++; size--;
    }

  /* BN_print() prints these one character at a time, but should the
     implementation change, let's loop just in case. */
  for (; size; size--, str++)
      bignum_bio_number = bignum_bio_number << 4 |
                          (*str <= '9' ?
                             *str - '0' :
                             tolower (*str) - 'a' + 10);

  return 1;
}

static unsigned long long
bignum_get_lowest_bits(bignum b)
{
  bignum_bio_number = 0;
  BN_print (bio_writer_static, b);
  return bignum_bio_number;
}

static unsigned long long
bignum_to_ullong_raw(bignum b)
{
  unsigned long long ret = 0;

  if (sizeof (BN_ULONG) >= sizeof (long long)) return BN_get_word (b);

  BN2BIN (b, &ret, sizeof (long long));
  return ret;
}

#endif /* !__ROUND2__ */
#ifdef __ROUND2_BIGNUM_TO_INT__

TYPE
C(bignum_to_, TYPE_ALIAS)(bignum b)
{
  unsigned TYPE ret = C(bignum_to_u, TYPE_ALIAS) (b);

  if (!ret) return 0;

  if (BN_is_negative (b))
    return ret == (unsigned TYPE) C(TYPE_ALIAS_UCASE, _MAX) + 1 ?
             C(TYPE_ALIAS_UCASE, _MIN) :
             -(TYPE) (ret & C(TYPE_ALIAS_UCASE, _MAX));

  return ret & C(TYPE_ALIAS_UCASE, _MAX);
}

unsigned TYPE
C(bignum_to_u, TYPE_ALIAS)(bignum b)
{
  int bits = BN_num_bits (b);

  if (!bits) return 0;

  if (bits <= (int) sizeof (TYPE) * 8)
    return sizeof (TYPE) <= sizeof (BN_ULONG) ?
             (unsigned TYPE) BN_get_word (b) :
             bignum_to_ullong_raw (b);

  return bignum_get_lowest_bits (b);
}

#endif
#ifndef __ROUND2__
#define __ROUND2__
#define __ROUND2_BIGNUM_TO_INT__

#define TYPE             int
#define TYPE_ALIAS       int
#define TYPE_ALIAS_UCASE INT
#include "number-openssl.c"
#undef TYPE_ALIAS_UCASE
#undef TYPE_ALIAS
#undef TYPE

#define TYPE             long
#define TYPE_ALIAS       long
#define TYPE_ALIAS_UCASE LONG
#include "number-openssl.c"
#undef TYPE_ALIAS_UCASE
#undef TYPE_ALIAS
#undef TYPE

#define TYPE             long long
#define TYPE_ALIAS       llong
#define TYPE_ALIAS_UCASE LLONG
#include "number-openssl.c"
#undef TYPE_ALIAS_UCASE
#undef TYPE_ALIAS
#undef TYPE

#undef __ROUND2_BIGNUM_TO_INT__
#undef __ROUND2__

double
bignum_to_double(bignum b)
{
  double ret;
  char *decstr;

  HANDLE_OP_ERROR (!(decstr = BN_bn2dec (b)));
  ret = strtod (decstr, NULL);
  OPENSSL_free (decstr);

  return ret;
}

static Lisp_Object
free_openssl_string(Lisp_Object str)
{
  OPENSSL_free (GET_VOID_FROM_LISP (str));
  return Qnil;
}

/* See doprnt.c:bignum_to_string(). */
Bytecount
bignum_to_string_openssl(Ibyte **buffer_inout, Bytecount size, bignum b,
                         UINT_16_BIT radix)
{
  size_t len;
  int speccount = -1, neg = 0;
  Ibyte *outbuf;
  char *str, *s;

  HANDLE_OP_ERROR (!(s = str = radix == 10 ? BN_bn2dec (b) : BN_bn2hex (b)));

  if (radix == 16 && !BN_is_zero (b))
    {
      if (*s == '-')
        {
          neg = 1;
          s++;
        }

      /* BN_bn2hex() zero pads the beginning of hexes with an odd number of
         decimals. */
      for (; *s == '0'; s++);
    }

  len = strlen (s) + neg;

  if (!*buffer_inout)
    {
      speccount = record_unwind_protect (free_openssl_string,
                                         STORE_VOID_IN_LISP (str));
      outbuf = *buffer_inout = (Ibyte *) xmalloc (len + 1);
    }
  else
    {
      text_checking_assert (size > (Bytecount) len);
      outbuf = *buffer_inout;
    }

  if (neg) *outbuf++ = '-';
  memcpy (outbuf, s, len + 1);

  if (speccount != -1)
    unbind_to (speccount);
  else
    OPENSSL_free(str);

  return len;
}

#define MULTIPLIER_ACCUM scratch_static
#define PIECE_VAL_BN     scratch_static2

/* Note that since this implementation parses the string from right to left,
   the result isn't stable for invalid strings. */
int
bignum_set_string (bignum b, const char *s, int base)
{
  double max_piece_len_d;
  BN_ULONG multiplier;
  size_t len, max_piece_len;
  int neg = 0, first = 1;

  if (base < 0 || base == 1 || base > 36) return -1;

  if (!base)
    {
      if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
        {
          base = 16;
          s += 2;
        }
      else if (!strncmp (s, "-0x", 3) || !strncmp (s, "-0X", 3) ||
               !strncmp (s, "+0x", 3) || !strncmp (s, "+0X", 3))
        { /* Insane values like +0x-ff get through. Don't use insane values. */
          neg = *s == '-';
          s += 3;
          base = 16;
        }
      else if (*s == '0' || ((*s == '-' || *s == '+') && s[1] == '0'))
        {
          neg = *s == '-';
          s += *s == '-' || *s == '+' ? 2 : 1;
          base = 8;

          for (; *s == '0'; s++);

          if (!*s)
            {
              BN_zero (b);
              return 0;
            }
        }
      else
        base = 10;
    }
  else if (base == 16)
    {
      if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
      else if (!strncmp (s, "-0x", 3) || !strncmp (s, "-0X", 3) ||
               !strncmp (s, "+0x", 3) || !strncmp (s, "+0X", 3))
        {
          neg = *s == '-';
          s += 3;
        }
    }
  else if (base != 10)
    {
      if (*s == '-' || *s == '+')
        {
          neg = *s == '-';
          s++;
        }

      for (; *s == '0'; s++);
    }

  len = strlen(s);

  if (!len)
    { /* A stupid answer to a stupid question. */
      BN_zero (b);
      return -1;
    }

  if (base == 10 || base == 16)
    {
      int ret;

      ERR_clear_error ();
      ret = base == 10 ? BN_dec2bn (&b, s) : BN_hex2bn (&b, s);

      /* ERR_get_error() == 0 means an invalid string, otherwise there has
         been a memory allocation error. */
      if (!ret && ERR_get_error ()) memory_full ();

      if (neg) BN_set_negative (b, 1);

      return (size_t) ret == len ? 0 : -1;
    }

  BN_zero (b);
  bignum_set_long (MULTIPLIER_ACCUM, neg ? -1 : 1);
  /* Calculate how many decimals would fit in BN_ULONG. */
  max_piece_len_d = sizeof (BN_ULONG) * 8 / log2 (base);
  max_piece_len = (double) (int) max_piece_len_d == max_piece_len_d ?
                    max_piece_len_d - 1 : max_piece_len_d;
  multiplier = pow (base, max_piece_len);
  s += len;

  do
    {
      BN_ULONG piece_val;
      size_t piece_len = min (len, max_piece_len);
      char piece[sizeof (BN_ULONG) * 8 + 1], *endptr;

      s -= piece_len;
      len -= piece_len;

      if (*s == '-' || *s == '+') return -1;

      /* We can't rely on the input string to be writable, so copy the current
         piece to the buffer for strtoul()'ing. */
      memcpy (piece, s, piece_len);
      piece[piece_len] = '\0';
      piece_val = strtoul (piece, &endptr, base);

      if (*endptr) return -1; /* strtoul() terminated before the string end. */

      if (first)
        {
          first = 0;
          HANDLE_OP_ERROR (!BN_set_word (b, piece_val));
        }
      else /* MULTIPLIER_ACCUM *= multiplier;    */
        {  /* b += piece_val * MULTIPLIER_ACCUM; */
          HANDLE_OP_ERROR (!BN_mul_word (MULTIPLIER_ACCUM, multiplier));

          if (piece_val)
            HANDLE_OP_ERROR
              (!BN_set_word (PIECE_VAL_BN, piece_val) ||
               !BN_mul (PIECE_VAL_BN, PIECE_VAL_BN, MULTIPLIER_ACCUM,
                        ctx_static) ||
               !BN_add (b, b, PIECE_VAL_BN));
        }
    }
  while (len);

  if (neg) BN_set_negative (b, 1);

  return 0;
}

#undef PIECE_VAL_BN
#undef MULTIPLIER_ACCUM

#endif /* !__ROUND2__ */
#ifdef __ROUND2_BIGNUM_SET__

void
C (bignum_set_, TYPE_ALIAS)(bignum b, TYPE val)
{
  if (!val)
    BN_zero (b);
  else if (val > 0)
    if (sizeof (TYPE) <= sizeof (BN_ULONG))
      HANDLE_OP_ERROR (!BN_set_word (b, val));
    else
      HANDLE_OP_ERROR (!BIN2BN (b, &val, sizeof (TYPE)));
  else if (val == C (TYPE_ALIAS_UCASE, _MIN))
    HANDLE_OP_ERROR (!BN_copy (b, C (bn_min_, TYPE_ALIAS)));
  else
    {
      C (bignum_set_u, TYPE_ALIAS) (b, -val);
      BN_set_negative(b, 1);
    }
}

void
C (bignum_set_u, TYPE_ALIAS)(bignum b, unsigned TYPE val)
{
  if (!val)
    BN_zero (b);
  else if (sizeof (TYPE) <= sizeof (BN_ULONG))
    HANDLE_OP_ERROR (!BN_set_word (b, val));
  else
    HANDLE_OP_ERROR (!BIN2BN (b, &val, sizeof (TYPE)));
}

#endif
#ifndef __ROUND2__
#define __ROUND2__
#define __ROUND2_BIGNUM_SET__

#define TYPE             long
#define TYPE_ALIAS       long
#define TYPE_ALIAS_UCASE LONG
#include "number-openssl.c"
#undef TYPE_ALIAS_UCASE
#undef TYPE_ALIAS
#undef TYPE

#define TYPE             long long
#define TYPE_ALIAS       llong
#define TYPE_ALIAS_UCASE LLONG
#include "number-openssl.c"
#undef TYPE_ALIAS_UCASE
#undef TYPE_ALIAS
#undef TYPE

#undef __ROUND2_BIGNUM_SET__
#undef __ROUND2__

/* This is an el cheapo implementation. The maximum digits of a double before
   it overflows to inf is a little bit over 300, so the buffer is big
   enough. Of course we check for overflows regardless. */
void bignum_set_double(bignum b, double d) {
#ifdef HAVE_SNPRINTF
  size_t required_len;
  char buf[512];
#endif
  char *allocated_buf;

  if (isinf (d) || isnan (d))
    {
      BN_zero (b);
      return;
    }

  d = d < 0 ? ceil (d) : floor (d);

#ifdef HAVE_SNPRINTF
  required_len = snprintf (buf, sizeof (buf), "%.0f", d);

  if (required_len < sizeof (buf))
    {
      HANDLE_OP_ERROR (!BN_dec2bn (&b, buf));
      return;
    }

  allocated_buf = (char *) ALLOCA (required_len + 1);
  sprintf (allocated_buf, "%.0f", d);
#else
  allocated_buf = (char *) XSTRING_DATA (emacs_sprintf_string ("%.0f", d));
#endif

  HANDLE_OP_ERROR (!BN_dec2bn (&b, allocated_buf));
}

void
bignum_abs(bignum to, bignum from)
{
  HANDLE_OP_ERROR (!BN_copy (to, from));
  BN_set_negative (to, 0);
}

void
bignum_neg(bignum to, bignum from)
{
  int sign = bignum_sign (from);

  if (!sign)
    {
      BN_zero(to);
      return;
    }

  if (to != from)
    HANDLE_OP_ERROR (!BN_copy (to, from));

  BN_set_negative (to, sign == 1 ? 1 : 0);
}

void
bignum_mul(bignum to, bignum a, bignum b)
{
  HANDLE_OP_ERROR (!BN_mul (to, a, b, ctx_static));
}

void
bignum_div(bignum to, bignum a, bignum b)
{
  if (BN_is_zero (b))
    { /* Don't want to differentiate between divide-by-zero and OOM errors. */
      BN_zero (to);
      return;
    }

  HANDLE_OP_ERROR (!BN_div (to, NULL, a, b, ctx_static));
}

void
bignum_mod(bignum to, bignum a, bignum b)
{
  if (BN_is_zero (b))
    {
      BN_zero (to);
      return;
    }

  HANDLE_OP_ERROR (!BN_mod (to, a, b, ctx_static));
}

int
bignum_divisible_p(bignum a, bignum b)
{
  if (BN_is_zero (b)) return 0;

  HANDLE_OP_ERROR (!BN_mod (scratch_static, a, b, ctx_static));
  return BN_is_zero (scratch_static);
}

void
bignum_ceil(bignum to, bignum a, bignum b)
{
  int aneg, bneg, neg;

  if (BN_is_zero (b))
    {
      BN_zero (to);
      return;
    }

  aneg = BN_is_negative (a); bneg = BN_is_negative (b);
  neg = (aneg && !bneg) || (bneg && !aneg);

  HANDLE_OP_ERROR (!BN_div (to, scratch_static, a, b, ctx_static));

  /* If the result was expected to be positive and the division wasn't even,
     eg. there's a modulus, increment by one to emulate ceil()ing. */
  if (!neg && !BN_is_zero (scratch_static))
    HANDLE_OP_ERROR (!BN_add_word (to, 1));
}

void
bignum_floor(bignum to, bignum a, bignum b)
{
  int aneg, bneg, neg;

  if (BN_is_zero (b))
    {
      BN_zero (to);
      return;
    }

  aneg = BN_is_negative (a); bneg = BN_is_negative (b);
  neg = (aneg && !bneg) || (bneg && !aneg);

  HANDLE_OP_ERROR (!BN_div (to, scratch_static, a, b, ctx_static));

  /* If the result was to be negative and a % b != 0, decrement to floor(). */
  if (neg && !BN_is_zero (scratch_static))
    HANDLE_OP_ERROR (!BN_sub_word (to, 1));
}

void
bignum_pow(bignum to, bignum a, unsigned long b)
{
  bignum_set_ulong (scratch_static, b);
  HANDLE_OP_ERROR (!BN_exp (to, a, scratch_static, ctx_static));
}

void
bignum_gcd(bignum to, bignum a, bignum b)
{
  if (BN_is_zero (a) || BN_is_zero (b))
    {
      BN_zero (to);
      return;
    }

  HANDLE_OP_ERROR (!BN_gcd (to, a, b, ctx_static));
}

void /* lcm(a, b) = abs(a * b) / gcd(a, b) */
bignum_lcm(bignum to, bignum a, bignum b)
{
  if (BN_is_zero (a) || BN_is_zero (b))
    {
      BN_zero (to);
      return;
    }

  HANDLE_OP_ERROR (!BN_mul (scratch_static, a, b, ctx_static));
  BN_set_negative (scratch_static, 0);
  HANDLE_OP_ERROR (!BN_gcd (scratch_static2, a, b, ctx_static) ||
                   !BN_div (to, NULL, scratch_static, scratch_static2,
                            ctx_static));
}

UINT_16_BIT
bignum_div_rem_uint_16_bit(bignum res, bignum numerator, UINT_16_BIT denom)
{
  if (!denom)
    {
      BN_zero (res);
      return 0;
    }

  HANDLE_OP_ERROR (!BN_set_word (scratch_static, denom) ||
                   !BN_div (res, scratch_static, numerator, scratch_static,
                            ctx_static));
  return BN_get_word (scratch_static);
}

#ifdef WORDS_BIGENDIAN
#  define INC_OR_DEC(a, amount) a += (amount)
#else
#  define INC_OR_DEC(a, amount) a -= (amount)
#endif
#define BIGGER(comp1,comp2,op1,op2) ((comp1) > (comp2) ? (op1) : (op2))
#define SMALLER(comp1,comp2,op1,op2) ((comp1) > (comp2) ? (op2) : (op1))

/*
  The bit operations are incredibly clumsy, because there's zero API support
  for them in the OpenSSL bignum suite. We have to do what their responses to
  requests for bitop support say - extract out the bignums as blobs of bits and
  iterate through them word by word.

  The tendency to use ints for sizes comes from BN_num_(bits|bytes)()
  returning them.
*/
static void
bignum_binary_bitop(bignum to, bignum a, bignum b, int abits, int bbits,
                    int (*process)(int, int, EMACS_UINT *, EMACS_UINT *))
{
  bignum bigger  = BIGGER  (abits, bbits, a, b),
         smaller = SMALLER (abits, bbits, a, b);
  EMACS_UINT *biggerp, *smallerp;
  int bigger_bits  = BIGGER  (abits, bbits, abits, bbits),
      smaller_bits = SMALLER (abits, bbits, abits, bbits),
      bigger_bytes  = ALIGN_FOR_TYPE ((bigger_bits  + 7) / 8, EMACS_UINT),
      smaller_bytes = ALIGN_FOR_TYPE ((smaller_bits + 7) / 8, EMACS_UINT),
      result_size;
  char *buf = (char *) ALLOCA (bigger_bytes + smaller_bytes +
                               sizeof (EMACS_UINT));

  /* Internet says IIUC that alloca() doesn't always give correct alignment. */
  buf = (char *) ALIGN_FOR_TYPE ((EMACS_UINT) buf, EMACS_UINT);

  BN2BIN (bigger, buf, bigger_bytes);
  BN2BIN (smaller, buf + bigger_bytes, smaller_bytes);

#ifdef WORDS_BIGENDIAN
  biggerp  = (EMACS_UINT *) buf;
  smallerp = (EMACS_UINT *) (buf + bigger_bytes);
#else
  biggerp  = (EMACS_UINT *) (buf + bigger_bytes) - 1;
  smallerp = (EMACS_UINT *) (buf + bigger_bytes + smaller_bytes) - 1;
#endif

  result_size = process (bigger_bytes / sizeof (EMACS_UINT),
                         smaller_bytes / sizeof (EMACS_UINT),
                         biggerp, smallerp);

#ifdef WORDS_BIGENDIAN
  HANDLE_OP_ERROR (!BIN2BN (to,
                            buf + (bigger_bytes -
                                   result_size * sizeof (EMACS_UINT)),
                            result_size * sizeof (EMACS_UINT)));
#else
  HANDLE_OP_ERROR (!BIN2BN (to, buf, result_size * sizeof (EMACS_UINT)));
#endif
}

static int
bignum_and_process(int bigger_words, int smaller_words, EMACS_UINT *biggerp,
                   EMACS_UINT *smallerp)
{
  int count = smaller_words;

  for (INC_OR_DEC (biggerp, bigger_words - smaller_words);
       count;
       count--, INC_OR_DEC (biggerp, 1), INC_OR_DEC (smallerp, 1))
    *biggerp &= *smallerp;

  return smaller_words;
}

void
bignum_and(bignum to, bignum a, bignum b)
{
  int abits = BN_num_bits (a), bbits = BN_num_bits(b);

  if (!abits || !bbits) /* a & 0 == 0 */
    {
      BN_zero (to);
      return;
    }

  if (a == b) /* a & a == a */
    {
      if (to != a) bignum_set (to, a);

      return;
    }

  if (min (abits, bbits) <= (int) sizeof (long long) * 8)
    {
      bignum_set_ullong (to, bignum_to_ullong (a) & bignum_to_ullong (b));
      return;
    }

  bignum_binary_bitop (to, a, b, abits, bbits, bignum_and_process);
}

#endif /* !__ROUND2__ */
#ifdef __ROUND2_BINARY_BITOPS__

static int
C3 (bignum_, OP_NAME, _process)(int bigger_words, int smaller_words,
                                EMACS_UINT *biggerp, EMACS_UINT *smallerp)
{
  for (INC_OR_DEC (biggerp, bigger_words - smaller_words);
       smaller_words;
       smaller_words--, INC_OR_DEC (biggerp, 1), INC_OR_DEC (smallerp, 1))
    *biggerp OP_ASSIGN *smallerp;

  return bigger_words;
}

void
C (bignum_, OP_NAME)(bignum to, bignum a, bignum b)
{
  int abits = BN_num_bits (a), bbits = BN_num_bits(b);

  if (!abits && !bbits)
    {
      BN_zero (to);
      return;
    }

#ifdef IS_IOR
  if (a == b) /* a | a == a */
    {
      if (to != a) bignum_set (to, a);

      return;
    }
#else
  if (a == b) /* a ^ a == 0 */
   {
      BN_zero (to);
      return;
    }
#endif

  if (!abits) /* b | 0 == b ^ 0 == b */
    {
      bignum_set (to, b);
      return;
    }

  if (!bbits)
    {
      bignum_set (to, a);
      return;
    }

  if (abits <= (int) sizeof (long long) * 8 &&
      bbits <= (int) sizeof (long long) * 8)
    {
      bignum_set_ullong (to, bignum_to_ullong (a) OP bignum_to_ullong (b));
      return;
    }

  bignum_binary_bitop (to, a, b, abits, bbits,
                       C3 (bignum_, OP_NAME, _process));
}

#endif
#ifndef __ROUND2__
#define __ROUND2__
#define __ROUND2_BINARY_BITOPS__

#define OP_NAME   ior
#define OP        |
#define OP_ASSIGN |=
#define IS_IOR
#include "number-openssl.c"
#undef IS_IOR
#undef OP_ASSIGN
#undef OP
#undef OP_NAME

#define OP_NAME   xor
#define OP        ^
#define OP_ASSIGN ^=
#include "number-openssl.c"
#undef OP_ASSIGN
#undef OP
#undef OP_NAME

#undef __ROUND2__
#undef __ROUND2_BINARY_BITOPS__
#undef BINARY_PROCESS_FN
#undef SMALLER
#undef BIGGER
#undef INC_OR_DEC

/*
  What a binary not of a bignum is isn't very well defined. This implementation
  emulates GMP and does what a two's complement not would look like with a
  finite signed two's complement integer.
*/
void
bignum_not(bignum to, bignum from)
{
  int neg = BN_is_negative (from);

  if (to != from) bignum_set(to, from);

  HANDLE_OP_ERROR (!BN_add_word (to, 1));
  BN_set_negative (to, !neg);
}

#define RANDOM_BN_ULONG_BITS min (FIXNUM_VALBITS, sizeof (BN_ULONG) * 8)

void
bignum_random(bignum to, bignum limit)
{
  int needed_bits;

  if (bignum_lt (limit, BN_value_one ()))
    {
      BN_zero (to);
      return;
    }

  ERR_clear_error ();

  if (BN_rand_range (to, limit)) return;

  /*
    The manual page is vague about what error conditions there are, other than
    a memory allocation error. man 3 BN_rand says:

NOTES
       Always check the error return value of these functions and do not take
       randomness for granted: an error occurs if the CSPRNG has not been
       seeded with enough randomness to ensure an unpredictable byte sequence.

    By reading the source, the other error conditions happen if the RAND(7)
    suite fails for reasons related to there not being enough seed entropy, or
    some kind of operational failure within a loop that generates the random.
    numbers. We separate those cases from a malloc failure and fall back to a
    home baked algorithm that uses the XEmacs configured PRNG.
  */

  if (ERR_GET_REASON (ERR_get_error ()) == ERR_R_MALLOC_FAILURE)
    memory_full ();

  HANDLE_OP_ERROR (!BN_set_word (to, get_random ()));

  for (needed_bits = BN_num_bits (limit) - RANDOM_BN_ULONG_BITS;
       needed_bits > 0;
       needed_bits -= RANDOM_BN_ULONG_BITS)
    HANDLE_OP_ERROR (!BN_lshift (to, to, RANDOM_BN_ULONG_BITS) ||
                     !BN_add_word (to, get_random ()));

  HANDLE_OP_ERROR (!BN_copy (scratch_static, limit) ||
                   !BN_add_word (scratch_static, 1) ||
                   !BN_mod (to, to, scratch_static, ctx_static));
}

#undef RANDOM_BN_ULONG_BITS

/* Can't use bignum_set_*() in the minimum initializations, since they use
   the bn_min_* values. */
#define BIGNUM_INIT_MINIMUM(type,type_alias,type_alias_ucase)                 \
  do {                                                                        \
    unsigned long long tmp =                                                  \
      (unsigned long long) type_alias_ucase ## _MAX + 1;                      \
                                                                              \
    HANDLE_OP_ERROR (!(bn_min_ ## type_alias = BN_new()) ||                   \
                     !BIN2BN (bn_min_ ## type_alias, &tmp,                    \
                              sizeof (long long)));                           \
    BN_set_negative (bn_min_ ## type_alias, 1);                               \
  } while (0)

void
init_number_openssl(void)
{
  BIO_METHOD *bio_method;
  int bio_idx;

  HANDLE_OP_ERROR
    (!(ctx_static = BN_CTX_new ()) ||
     (bio_idx = BIO_get_new_index ()) == -1 ||
     !(bio_method = BIO_meth_new (bio_idx, "XEmacs bignum writer BIO")) ||
     !BIO_meth_set_write (bio_method, bignum_bio_write) ||
     !(bio_writer_static = BIO_new (bio_method)));

  bignum_init (scratch_bignum);
  bignum_init (scratch_bignum2);
  bignum_init (scratch_static);
  bignum_init (scratch_static2);

  BIGNUM_INIT_MINIMUM (int, int, INT);
  BIGNUM_INIT_MINIMUM (long, long, LONG);
  BIGNUM_INIT_MINIMUM (long long, llong, LLONG);
}

void
bignum_memory_full(void)
{
  ERR_clear_error ();
  memory_full ();
}

#undef BIGNUM_INIT_MINIMUM
#undef BIN2BN
#undef BN2BIN
#undef HANDLE_OP_ERROR
#endif /* !__ROUND2__ */

#ifdef __ROUND2__
#  undef C3
#  undef C3_
#  undef C
#  undef C_
#endif
