/* Unit tests for number-openssl.c.
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

#include <float.h>

/* Uncomment these to test the branches that rely on these defines. */
/* #undef HAVE_SNPRINTF */
/* #undef __STDC_VERSION__ */

static int fail_amount;

static void print_error(const char *msg, ...);
static int compare_to_string(bignum b, int hex, int type, ...);

static void test_init(void);
static void test_conversions_to_bignum(void);
static void test_conversions_from_bignum(void);
static void test_arithmetics(void);
static void test_bitops(void);

#define TEST(test_name,init_test,test_value)                                  \
  do                                                                          \
    {                                                                         \
      init_test;                                                              \
                                                                              \
      if (test_value)                                                         \
        printf ("Passed: %s\n", test_name);                                   \
      else                                                                    \
        {                                                                     \
          print_error ("Failed: %s\n(%s; %s)\n",                              \
                       test_name, #init_test, #test_value);                   \
          fail_amount++;                                                      \
        }                                                                     \
    }                                                                         \
  while (0)

#define IS_DEC(b,comparee) compare_to_string ((b), 0, 0, (comparee))
#define IS_HEX(b,comparee) compare_to_string ((b), 1, 0, (comparee))
#define IS_INT(b,comparee)                                                    \
  compare_to_string ((b), 0, 1, (long long) (comparee))
#define IS_UINT(b,comparee)                                                   \
  compare_to_string ((b), 0, 2, (unsigned long long) (comparee))

#define ENSURE(val)                                                           \
  do                                                                          \
    if (!(val))                                                               \
      {                                                                       \
        print_error ("Fatal error at line %d while attempting %s\n",          \
                     __LINE__, #val);                                         \
        exit (1);                                                             \
      }                                                                       \
  while (0)

#define ENASPRINTF(params) ENSURE (asprintf params != -1)
#define EN_ADD_WORD(a,b) ENSURE (BN_add_word (a, b))
#define EN_SUB_WORD(a,b) ENSURE (BN_sub_word (a, b))
#define EN_MUL_WORD(a,b) ENSURE (BN_mul_word (a, b))

static void
tests(void)
{
  fail_amount = 0;

  puts ("\n\nnumber-openssl-tests.c running tests");

  test_init ();
  test_conversions_to_bignum ();
  test_conversions_from_bignum ();
  test_arithmetics ();
  test_bitops ();

  if (fail_amount)
    if (fail_amount == 1)
      print_error ("1 test failed\n");
    else
      print_error ("%d tests failed\n", fail_amount);
  else
    puts ("All tests passed");

  /* This is ran under make, have a few extra newlines around the output. */
  puts("\n");
}

static void
test_init(void)
{
  bignum test = NULL;

  TEST
    ("Allocation",
     bignum_init (test),
     test != NULL);

  bignum_fini (test);
}

/* The tests for setting long longs do actually test setting integers bigger
   than BN_ULONG in 32 bit systems and that the endianness dependent
   conversions work. */
static void
test_conversions_to_bignum(void)
{
  bignum test, test2;
  int i;
  char *tmp = NULL;

  bignum_init (test);
  bignum_init (test2);

  TEST
    ("Set to long 0",
     bignum_set_long (test, 0),
     IS_INT (test, 0));
  TEST
    ("Set to long -1",
     bignum_set_long (test, -1),
     IS_INT (test, -1));
  TEST
    ("Set to long 1",
     bignum_set_long (test, 1),
     IS_INT (test, 1));

  TEST
    ("Set to long INT_MAX",
     bignum_set_long (test, INT_MAX),
     IS_INT (test, INT_MAX));
  TEST
    ("Set to long INT_MIN",
     bignum_set_long (test, INT_MIN),
     IS_INT (test, INT_MIN));
  TEST
    ("Set to ulong UINT_MAX",
     bignum_set_ulong (test, UINT_MAX),
     IS_UINT (test, UINT_MAX));

  TEST
    ("Set to long long LLONG_MAX",
     bignum_set_llong (test, LLONG_MAX),
     IS_INT (test, LLONG_MAX));
  TEST
    ("Set to long long LLONG_MIN",
     bignum_set_llong (test, LLONG_MIN),
     IS_INT (test, LLONG_MIN));
  TEST
    ("Set to unsigned long long ULLONG_MAX",
     bignum_set_ullong (test, ULLONG_MAX),
     IS_UINT (test, ULLONG_MAX));

  TEST
    ("Copy ULLONG_MAX from a bignum to another",
     bignum_set_long (test, 0);
     bignum_set_ullong (test2, ULLONG_MAX);
     bignum_set (test, test2),
     IS_UINT (test, ULLONG_MAX));

  TEST
    ("Set to double 1.0",
     bignum_set_double (test, 1.0),
     IS_INT (test, 1));
  TEST
    ("Set to double 2.5 (assume truncation)",
     bignum_set_double (test, 2.5),
     IS_INT (test, 2));
  TEST
    ("Set to double -3.9 (assume truncation)",
     bignum_set_double (test, -3.9),
     IS_INT (test, -3));
  TEST
    ("Set to negative double with 300 zeroes",
     bignum_set_double (test, -1.0E+300);
     ENSURE (tmp = BN_bn2dec (test)),
     strlen (tmp) == 302);
  OPENSSL_free (tmp);

#ifdef DBL_MAX
  TEST
    ("Set to double DBL_MAX",
     ENASPRINTF ((&tmp, "%.0f", DBL_MAX));
     bignum_set_double (test, DBL_MAX),
     IS_DEC (test, tmp));
  free (tmp);
#endif

#ifdef INFINITY
  TEST
    ("Set to double inf (-> 0)",
     bignum_set_long (test, 1); /* Ensure a non-zero initial value. */
     bignum_set_double (test, INFINITY),
     IS_INT (test, 0));
#endif

#ifdef NAN
  TEST
    ("Set to double NaN (-> 0)",
     bignum_set_long (test, 1); /* Here a non-zero as well. */
     bignum_set_double (test, NAN),
     IS_INT (test, 0));
#endif

  TEST
    ("Invalid string returns an error value",,
     bignum_set_string (test, "!", 2) == -1);
  TEST
    ("Invalid string starting with numbers returns an error value",,
     bignum_set_string (test, "35!42", 7) == -1);
  TEST
    ("Invalid base parameter to bignum_set_string() returns an error value",,
     bignum_set_string (test, "1", -1) == -1 &&
     bignum_set_string (test, "1", 37) == -1);

  for (i = 2; i <= 36; i++)
    {
      ENASPRINTF ((&tmp, "Set to string \"10\" in base %d", i));
      TEST
        (tmp,
         bignum_set_string (test, "10", i),
         IS_INT (test, i));
      free(tmp);
    }

  TEST
    ("Default string parsing a positive value in base 10",
     bignum_set_string (test, "12765", 0),
     IS_INT (test, 12765));
  TEST
    ("Default string parsing a negative value in base 10",
     bignum_set_string (test, "-12765", 0),
     IS_INT (test, -12765));
  TEST
    ("Base 8 detection for setting string \"0\" without returning an error "
     "value",,
     !bignum_set_string (test, "0", 0) && BN_is_zero (test));
  TEST
    ("Base 8 detection for a string with a leading 0 and a positive value",
     bignum_set_string (test, "070707", 0),
     IS_INT (test, 070707));
  TEST
    ("Base 8 detection for a string with a leading 0 and a negative value",
     bignum_set_string (test, "-070707", 0),
     IS_INT (test, -070707));
  TEST
    ("Base 16 detection for a a string with leading 0x",
     bignum_set_string (test, "0xB00B135", 0),
     IS_INT (test, 0xB00B135));
  TEST
    ("Base 16 detection for a a string with leading 0X",
     bignum_set_string (test, "0X31337", 0),
     IS_INT (test, 0x31337));
  TEST
    ("Base 16 detection for a string with a leading -0x",
     bignum_set_string (test, "-0x666", 0),
     IS_INT (test, -0x666));
  TEST
    ("Base 16 detection for a string with a leading -0X",
     bignum_set_string (test, "-0X888", 0),
     IS_INT (test, -0x888));
  TEST
    ("Base 16 detection for a string with a leading 0x-",
     bignum_set_string (test, "0x-12345", 0),
     IS_INT (test, -0x12345));
  TEST
    ("Base 16 detection for a string with a leading 0X-",
     bignum_set_string (test, "0X-54321", 0),
     IS_INT (test, -0x54321));

  /* This is redefined so that we'd get bignum_size_*() values that have one
     byte per character. */
#undef MAX_ICHAR_LEN
#define MAX_ICHAR_LEN 1

  TEST
    ("bignum_size_decimal() gives a correct estimate for 0",
     BN_zero (test),
     bignum_size_decimal (test) > 1);

  {
    /* If you modify these, make sure the values don't overflow buf. */
    char buf[4096], *tmp2;
    struct test_entry { const char *val, *expected; int base; } *p, values[] =
      {
        {
          "+110010100101010010101010110101011011010101011101010110010110011010"
          "010101010101010100110110100101011010101010101010110100101010101010",
          "CA54AAD5B55D596695554DA56AAAB4AAA",
          2
        },
        {
#define HEXTEST "FACEFEED" "FEEDB4B3" "DECAFBAAAAAAAD" "DEADD00D"
          HEXTEST, HEXTEST, 16
        },
        {
          HEXTEST, "E9B280F0DD6BBAD7393E7B237955DF4E04198BB4D", 20
#undef HEXTEST
        },
        {
          "Linus0is0not0wrong0about0the0GNU0coding0style0myself0id0prefer"
          "four0space0indents0but0almost0anything0would0be0better0than0the"
          "insane0two0spaces0for0curly0braces0then0two0more0for0their0content"
          "but0what0can0you0do0thems0the0house0rules",
          "668CA2DEAEAFCDBFE1698990A2C9AD0C543F20BDA50D4FC668B17BFE709195BEDE3"
          "8CADBF4E7AA792DF22C17509091F0392E5CFBF3E5FE43A0295A424E515384C401A8"
          "B29296AB8A818D1DAE440D01D1C12BF5779262DB60071C0BDACAAB95B38AF12D375"
          "4B3356C72507BF018321BCFE6D72572119289383FF6F4A3433ADA008311D9FCB07D"
          "B2ACD45DF16C92538CA22F25FC99A2E4",
          36
        },
        { NULL }
      };

    for (p = values; p->val; p++)
      {
        int nth = (p - values) + 1;

        ENASPRINTF ((&tmp, "Set big-ish string #%d in base %d", nth, p->base));
        TEST
          (tmp,
           bignum_set_string (test, p->val, p->base),
           IS_HEX (test, p->expected));
        free (tmp);

        BN_set_negative (test, 1);

        ENASPRINTF ((&tmp, "bignum_size_decimal() gives a correct estimate "
                     "for big-ish number #%d", nth));
        TEST
          (tmp,
           ENSURE (tmp2 = BN_bn2dec (test)),
           (size_t) bignum_size_decimal (test) >= strlen (tmp2));
        OPENSSL_free (tmp2); free (tmp);

        ENASPRINTF ((&tmp, "bignum_size_octal() gives a correct estimate "
                     "for big-ish number #%d", nth));
        TEST
          (tmp, tmp2 = buf,
           bignum_size_octal (test) >=
           (int) bignum_to_string ((Ibyte **) &tmp2, sizeof (buf), test, 8,
                                   Qnil));
        free (tmp);

        /* This uses doprnt.c:bignum_to_string() just so we don't need to
           account for the leading 0 BN_bn2hex() gives. */
        ENASPRINTF ((&tmp, "bignum_size_hex() gives a correct estimate "
                     "for big-ish number #%d", nth));
        TEST
          (tmp, tmp2 = buf,
           bignum_size_hex (test) >=
           (int) bignum_to_string ((Ibyte **) &tmp2, sizeof (buf), test, 16,
                                   Qnil));
        free (tmp);

        ENASPRINTF ((&tmp, "bignum_size_binary() gives a correct estimate "
                     "for big-ish number #%d", nth));
        TEST
          (tmp, tmp2 = buf,
           bignum_size_binary (test) >=
           (int) bignum_to_string ((Ibyte **) &tmp2, sizeof (buf), test, 2,
                                   Qnil));
        free (tmp);
      }
  }

  bignum_fini (test);
  bignum_fini (test2);
}

/* This tests the endianness parts, different sizes of BN_ULONG and
   bignum_get_lowest_bits(). */
static void
test_conversions_from_bignum(void)
{
  bignum test;
  bignum test2;
  int i, success;

  bignum_init (test);
  bignum_init (test2);

  TEST
    ("These tests don't give reliable results if "
     "sizeof(long long) != 8 or sizeof(int) != 4",,
     sizeof (long long) == 8 && sizeof (int) == 4);

#define FIT_TESTS(type,type_ucase)                                            \
  TEST                                                                        \
    ("bignum_fits_" #type "_p(0)",                                            \
     BN_zero (test),                                                          \
     bignum_fits_ ## type ## _p (test));                                      \
  TEST                                                                        \
    ("bignum_fits_" #type "_p(1)",                                            \
     bignum_set_llong (test, 1),                                              \
     bignum_fits_ ## type ## _p (test));                                      \
  TEST                                                                        \
    ("bignum_fits_" #type "_p(-1)",                                           \
     bignum_set_llong (test, -1),                                             \
     bignum_fits_ ## type ## _p (test));                                      \
  TEST                                                                        \
    ("bignum_fits_" #type "_p(" #type_ucase "_MAX)",                          \
     bignum_set_ullong (test, type_ucase ## _MAX),                            \
     bignum_fits_ ## type ## _p (test));                                      \
  TEST                                                                        \
    ("bignum_fits_" #type "_p(" #type_ucase "_MAX + 1)",                      \
     bignum_set_ullong (test, type_ucase ## _MAX); EN_ADD_WORD (test, 1),     \
     !bignum_fits_ ## type ## _p (test));                                     \
  TEST                                                                        \
    ("bignum_fits_" #type "_p(" #type_ucase "_MIN)",                          \
     bignum_set_llong (test, type_ucase ## _MIN),                             \
     bignum_fits_ ## type ## _p (test));                                      \
  TEST                                                                        \
    ("bignum_fits_" #type "_p(" #type_ucase "_MIN - 1)",                      \
     bignum_set_llong (test, type_ucase ## _MIN); EN_SUB_WORD (test, 1),      \
     !bignum_fits_ ## type ## _p (test));                                     \
  TEST                                                                        \
    ("bignum_fits_u" #type "_p(0)",                                           \
     BN_zero (test),                                                          \
     bignum_fits_u ## type ## _p (test));                                     \
  TEST                                                                        \
    ("bignum_fits_u" #type "_p(1)",                                           \
     bignum_set_llong (test, 1),                                              \
     bignum_fits_u ## type ## _p (test));                                     \
  TEST                                                                        \
    ("bignum_fits_u" #type "_p(-1)",                                          \
     bignum_set_llong (test, -1),                                             \
     !bignum_fits_u ## type ## _p (test));                                    \
  TEST                                                                        \
    ("bignum_fits_u" #type "_p(U" #type_ucase "_MAX)",                        \
     bignum_set_ullong (test, U ## type_ucase ## _MAX),                       \
     bignum_fits_u ## type ## _p (test));                                     \
  TEST                                                                        \
    ("bignum_fits_u" #type "_p(U" #type_ucase "_MAX + 1)",                    \
     bignum_set_ullong (test, U ## type_ucase ## _MAX); EN_ADD_WORD (test, 1),\
     !bignum_fits_u ## type ## _p (test))

  FIT_TESTS (int,   INT);
  FIT_TESTS (long,  LONG);
  FIT_TESTS (llong, LLONG);

#undef FIT_TESTS

  TEST
    ("UINT_MAX << 3 + 234324 as unsigned long long -> bignum -> uint",
     bignum_set_ullong (test, ((unsigned long long) UINT_MAX << 3) + 234324),
     bignum_to_uint(test) ==
     (unsigned int) ((unsigned long long) UINT_MAX << 3) + 234324);
  TEST
    ("LLONG_MAX to long long",
     bignum_set_llong (test, LLONG_MAX),
     bignum_to_llong (test) == LLONG_MAX);
  TEST
    ("LLONG_MIN to long long",
     bignum_set_llong (test, LLONG_MIN),
     bignum_to_llong (test) == LLONG_MIN);
  TEST
    ("ULLONG_MAX to unsigned long long",
     bignum_set_ullong (test, ULLONG_MAX),
     bignum_to_ullong (test) == ULLONG_MAX);
  /* This ensures the value will not be negative, aka -1 for all bits on. */
  TEST
    ("ULLONG_MAX to long long",
     bignum_set_ullong (test, ULLONG_MAX),
     bignum_to_llong (test) == LLONG_MAX);
  TEST
    ("ULLONG_MAX * 2",
     bignum_set_ullong (test, ULLONG_MAX); EN_MUL_WORD (test, 2),
     bignum_to_llong (test) == 0x7FFFFFFFFFFFFFFELL &&
     bignum_to_ullong (test) == 0xFFFFFFFFFFFFFFFEULL);

#define NICE_TEST_NUMBER "0x1234567890ABCDEFEDCBA0987654321"

  TEST
    ("Lowest bits of a big-ish number as an unsigned long long",
     bignum_set_string (test, NICE_TEST_NUMBER, 0),
     bignum_to_ullong (test) == 0xFEDCBA0987654321ULL);
  TEST
    ("Lowest bits of a negative big-ish number as a long long",
     bignum_set_string (test, "-" NICE_TEST_NUMBER, 0),
     bignum_to_llong (test) == -0x7EDCBA0987654321LL);

#undef NICE_TEST_NUMBER

  /* The only difference between signed and unsigned is the case in which the
     lowest bits are -[INT_TYPE]_MIN, so don't test for signed types here. */
  TEST
    ("100 rounds of converting a large random number to a uint and a ullong",
     bignum_setbit (test2, 1024);
     for (success = 1, i = 0; success && i < 100; i++)
       {
         size_t len;
         char *hex;

         bignum_random (test, test2);
         ENSURE (hex = BN_bn2hex (test));
         len = strlen (hex);

         if (len <= 16) /* Too small of a number to be useful. */
           {
             i--;
             OPENSSL_free (hex);
             continue;
           }

         success =
           bignum_to_uint (test) == strtoul (hex + len - 8, NULL, 16) &&
           bignum_to_ullong (test) == strtoull (hex + len - 16, NULL, 16);

         OPENSSL_free (hex);
       },
     success);

  bignum_fini (test);
  bignum_fini (test2);
}

#define CREATE_COMPARISON_CHECK(op_name)                                      \
  static int                                                                  \
  test_op_ ## op_name(int val1, int val2)                                     \
  {                                                                           \
    bignum test, test2;                                                       \
    int ret;                                                                  \
    bignum_init (test);  bignum_set_long (test,  val1);                       \
    bignum_init (test2); bignum_set_long (test2, val2);                       \
    ret = op_name (test, test2);                                              \
    bignum_fini (test); bignum_fini (test2);                                  \
    return ret;                                                               \
  }

CREATE_COMPARISON_CHECK (bignum_cmp)
CREATE_COMPARISON_CHECK (bignum_lt)
CREATE_COMPARISON_CHECK (bignum_le)
CREATE_COMPARISON_CHECK (bignum_eql)
CREATE_COMPARISON_CHECK (bignum_ge)
CREATE_COMPARISON_CHECK (bignum_gt)

#undef CREATE_COMPARISON_CHECK

/* These are supposedly supefluous and just checking for brain farts in the
   arithmetic macros and functions, except for that I did find two bugs while
   writing these. I guess that proves that even testing for the sake of testing
   does have some value. */
static void
test_arithmetics(void)
{
  bignum test, test2, test3;
  int i, success;

  bignum_init (test);
  bignum_init (test2);
  bignum_init (test3);

  TEST
    ("bignum_sign() returns 0 for value 0",
     bignum_set_long (test, 0),
     bignum_sign (test) == 0);
  TEST
    ("bignum_sign() return -1 for a negative value",
     bignum_set_long (test, -465464),
     bignum_sign (test) == -1);
  TEST
    ("bignum_sign() return 1 for a positive value",
     bignum_set_long (test, 68835676),
     bignum_sign (test) == 1);

  TEST
    ("bignum_evenp() returns true for 0",
     BN_zero (test),
     bignum_evenp (test));
  TEST
    ("bignum_evenp() return true for a positive even number",
     bignum_set_long (test, 22),
     bignum_evenp (test));
  TEST
    ("bignum_evenp() return true for a negative even number",
     bignum_set_long (test, -100),
     bignum_evenp (test));
  TEST
    ("bignum_oddp() return true for a positive odd number",
     bignum_set_long (test, 25),
     bignum_oddp (test));
  TEST
    ("bignum_oddp() return true for a negative odd number",
     bignum_set_long (test, -101),
     bignum_oddp (test));

  TEST
    ("bignum_cmp()",
     for (success = 1, i = -1;
          success && i <= 1;
          i++,
          success = test_op_bignum_cmp (i, i) == 0 &&
            test_op_bignum_cmp (i - 1, i) == -1 &&
            test_op_bignum_cmp (i, i - 1) == 1),
     success);
  TEST
    ("bignum_lt()",,
     test_op_bignum_lt (3, 5) && !test_op_bignum_lt (5, 3) &&
     !test_op_bignum_lt (3, 3));
  TEST
    ("bignum_le()",,
     test_op_bignum_le (3, 5) && test_op_bignum_le (3, 3) &&
     !test_op_bignum_le (5, 3));
  TEST
    ("bignum_eql()",,
     test_op_bignum_eql (3, 3) && !test_op_bignum_eql (3, 5));
  TEST
    ("bignum_ge()",,
     test_op_bignum_ge (5, 3) && test_op_bignum_ge (3, 3) &&
     !test_op_bignum_ge (3, 5));
  TEST
    ("bignum_gt()",,
     test_op_bignum_gt (5, 3) && !test_op_bignum_gt (3, 5) &&
     !test_op_bignum_gt (3, 3));

  TEST
    ("bignum_abs()",
     bignum_set_long (test, -5); bignum_set_long (test2, 10);
     bignum_abs (test, test); bignum_abs (test2, test2),
     IS_INT (test, 5) && IS_INT (test2, 10));
  TEST
    ("bignum_neg()",
     bignum_set_long (test, -5); bignum_set_long (test2, 10);
     bignum_neg (test, test); bignum_neg (test2, test2),
     IS_INT (test, 5) && IS_INT (test2, -10));

#define NICE_TEST_NUMBER "who0knows0this0might0reveal0a0hidden0bug"
#define TRIVIAL_TEST(description,op,comparer,result)    \
  TEST                                                  \
    (description,                                       \
     bignum_set_string (test, NICE_TEST_NUMBER, 36);    \
     op (test, test, test),                             \
     comparer (test, result));

  TRIVIAL_TEST
    ("bignum_add() works (when adding a number to itself and having the same "
     "bignum as the result operand)",
     bignum_add, IS_HEX,
     "C8B806DA28C40071ECC50A4800FEAF7B771BD4A07E2F126B75F0");
  TRIVIAL_TEST
    ("bignum_sub() works the same way as above", bignum_sub, IS_INT, 0);
  TRIVIAL_TEST
    ("bignum_mul() -,,-", bignum_mul, IS_HEX,
     "275803BFAE5EF39152830AA9036425A6FB44DBC6955AD9E724092384143B5821B9FDF2F8"
     "C01B4F5ACB9ED8449E3835F4633D5040");
  TRIVIAL_TEST
    ("bignum_div() -,,-", bignum_div, IS_INT, 1);

#undef TRIVIAL_TEST

  TEST
    ("bignum_mod()",
     bignum_set_string (test, NICE_TEST_NUMBER, 36);
     bignum_set_long (test2, 12765);
     bignum_mod (test, test, test2),
     IS_INT (test, 9721));

  TEST
    ("bignum_divisible_p()",
     bignum_set_string (test, NICE_TEST_NUMBER, 36);
     bignum_set_long (test2, 109);
     bignum_set_long (test3, 110),
     bignum_divisible_p (test, test2) && !bignum_divisible_p (test, test3));
  TEST
    ("bignum_ceil() with a positive division result rounds towards +inf",
     bignum_set_string (test, NICE_TEST_NUMBER, 36);
     bignum_set_string (test2,
                        "1D769C03D702EF9FFD2D5D1B027EA1FAA319390BD0C2CCAF7A",
                        16);
     bignum_ceil (test, test, test2),
     IS_INT (test, 873));
  TEST
    ("bignum_ceil() with a negative division result rounds towards +inf",
     bignum_set_string (test, NICE_TEST_NUMBER, 36);
     bignum_set_string (test2,
                        "-1D769C03D702EF9FFD2D5D1B027EA1FAA319390BD0C2CCAF7A",
                        16);
     bignum_ceil (test, test, test2),
     IS_INT (test, -872));
  TEST
    ("bignum_ceil(9, 10) == 1",
     bignum_set_long (test, 9); bignum_set_long (test2, 10);
     bignum_ceil (test, test, test2),
     IS_INT (test, 1));
  TEST
    ("bignum_ceil(9, 8) == 2",
     bignum_set_long (test, 9); bignum_set_long (test2, 8);
     bignum_ceil (test, test, test2),
     IS_INT (test, 2));
  TEST
    ("bignum_ceil(9, -10) == 0",
     bignum_set_long (test, 9); bignum_set_long (test2, -10);
     bignum_ceil (test, test, test2),
     IS_INT (test, 0));
  TEST
    ("bignum_ceil(9, -8) == -1",
     bignum_set_long (test, 9); bignum_set_long (test2, -8);
     bignum_ceil (test, test, test2),
     IS_INT (test, -1));
  TEST
    ("bignum_floor() with a positive division result rounds towards -inf",
     bignum_set_string (test, NICE_TEST_NUMBER, 36);
     bignum_set_string (test2,
                        "1D769C03D702EF9FFD2D5D1B027EA1FAA319390BD0C2CCAF7A",
                        16);
     bignum_floor (test, test, test2),
     IS_INT (test, 872));
  TEST
    ("bignum_floor() with a negative division result rounds towards -inf",
     bignum_set_string (test, NICE_TEST_NUMBER, 36);
     bignum_set_string (test2,
                        "-1D769C03D702EF9FFD2D5D1B027EA1FAA319390BD0C2CCAF7A",
                        16);
     bignum_floor (test, test, test2),
     IS_INT (test, -873));
  TEST
    ("bignum_floor(9, 10) == 0",
     bignum_set_long (test, 9); bignum_set_long (test2, 10);
     bignum_floor (test, test, test2),
     IS_INT (test, 0));
  TEST
    ("bignum_floor(9, 8) == 1",
     bignum_set_long (test, 9); bignum_set_long (test2, 8);
     bignum_floor (test, test, test2),
     IS_INT (test, 1));
  TEST
    ("bignum_floor(9, -8) == -2",
     bignum_set_long (test, 9); bignum_set_long (test2, -8);
     bignum_floor (test, test, test2),
     IS_INT (test, -2));
  TEST
    ("bignum_floor(9, -10) == -1",
     bignum_set_long (test, 9); bignum_set_long (test2, -10);
     bignum_floor (test, test, test2),
     IS_INT (test, -1));

  TEST
    ("bignum_pow() with a big number",
     bignum_set_string (test, NICE_TEST_NUMBER, 36);
     bignum_pow (test, test, 3),
     IS_HEX (test, "F6C859F04A688F1D1DBF52A252BA39560DB6AB46C87E8EF711551AE97D"
             "5A8C13D2A17957E2AA0F13E9D14EF6354B75C60DF2DC2D883E1B0576D8F9FBF8"
             "583614BE4E6F13315A79E263E8CF43E00"));
  TEST
    ("bignum_pow() with a small number",
     bignum_set_long (test, -3);
     bignum_pow (test, test, 3),
     IS_INT (test, -27));

  TEST /* 17 * 7 == 119, 17 * 29 == 493. */
    ("bignum_gcd()",
     bignum_set_long (test, 119); bignum_set_long (test2, 493);
     bignum_gcd (test, test, test2),
     IS_INT (test, 17));

  /* I copied the definition over lcm(), aka least common multiple, over from
     number-mp.c and just tested it does the same as in GMP. */
  TEST
    ("bignum_lcm()",
     bignum_set_ulong (test, 6); bignum_set_long (test2, 10);
     bignum_lcm (test, test, test2),
     IS_INT (test, 30));

  TEST
    ("bignum_div_rem_uint_16_bit() with big numbers",
     bignum_set_string (test, NICE_TEST_NUMBER, 36);
     i = bignum_div_rem_uint_16_bit (test, test, 1000),
     IS_HEX (test, "19B127CA011F3B72EF33D85B22F17EE8E43CE80428BA3FCC38") &&
     i == 56);
  TEST
    ("bignum_div_rem_uint_16_bit() with small numbers",
     bignum_set_long (test, 31337);
     i = bignum_div_rem_uint_16_bit (test, test, 1000),
     IS_INT (test, 31) && i == 337);

#undef NICE_TEST_NUMBER

  bignum_fini (test);
  bignum_fini (test2);
}

static const char *hexen = "0123456789ABCDEF";

static int
hex2num(char c)
{
  return c >= '0' && c <= '9' ? c - '0' : tolower (c) - 'a' + 10;
}

static const char *
stringly_and (char *hex, char *hex2)
{
  char *p1 = hex + strlen (hex) - 1, *p2 = hex2 + strlen (hex2) - 1;

  for (; p1 >= hex && p2 >= hex2; p1--, p2--)
    *p1 = hexen[hex2num (*p1) & hex2num (*p2)];

  while (*++p1 == '0');

  return *p1 ? p1 : "0";
}

#define STRINGLY_OP_FN(op_name,op)                                            \
  static char *                                                               \
  stringly_ ## op_name (char *hex, char *hex2)                                \
  {                                                                           \
    size_t len1 = strlen (hex), len2 = strlen (hex2), tmp;                    \
    char *p1, *p2;                                                            \
                                                                              \
    if (len2 > len1)                                                          \
      {                                                                       \
        p1 = hex; hex = hex2; hex2 = p1;                                      \
        tmp = len1; len1 = len2; len2 = tmp;                                  \
      }                                                                       \
                                                                              \
    for (p1 = hex + len1 - 1, p2 = hex2 + len2 - 1; len2; p1--, p2--, len2--) \
      *p1 = hexen[hex2num (*p1) op hex2num (*p2)];                            \
                                                                              \
    while (*hex == '0') hex++;                                                \
                                                                              \
    return hex;                                                               \
  }

STRINGLY_OP_FN (ior, |)
STRINGLY_OP_FN (xor, ^)

#undef STRINGLY_OP_FN

static void
test_bitops(void)
{
  bignum test, test2, test3;
  int i, success;

  bignum_init (test);
  bignum_init (test2);
  bignum_init (test3);

  TEST
    ("bignum_lshift()",
     bignum_set_string (test, "313370000", 16);
     bignum_lshift (test, test, 128),
     IS_HEX (test, "31337000000000000000000000000000000000000"));

  TEST
    ("bignum_rshift()",
     bignum_set_string (test, "31337000000000000000000000000000000000000", 16);
     bignum_rshift (test, test, 128),
     IS_HEX (test, "313370000"));

  TEST
    ("bignum_and(0, 0)",
     bignum_set_long (test, 1); BN_zero (test2); BN_zero (test3);
     bignum_and (test, test2, test3),
     IS_INT (test, 0));
  TEST
    ("bignum_and(number, 0)",
     bignum_set_long (test, 42342342); BN_zero (test2);
     bignum_and (test, test, test2),
     IS_INT (test, 0));
  TEST
    ("bignum_and(0, number)",
     bignum_set_string (test, "423423424345345534545353453345", 0);
     BN_zero (test2);
     bignum_and (test, test2, test),
     IS_INT (test, 0));
  TEST
    ("bignum_and() with numbers that fit ullongs",
     bignum_set_ullong (test,  0xF1F1F1F1F1F1F1F1ULL);
     bignum_set_ullong (test2, 0x1F1F1F1F1F1F1F1FULL);
     bignum_and (test, test, test2),
     IS_HEX (test, "1111111111111111"));
  TEST
    ("bignum_and() with a big-ish number and a number that fits an ullong",
     bignum_set_string (test, "frgtbgnfsp9fdrhtgpiunvbgsiufbnfdgs", 36);
     bignum_set_ullong (test2, 0x715517FACEULL);
     bignum_and (test, test, test2),
     IS_HEX (test, "700514E0CC"));
  TEST
    ("bignum_and() with a big-ish number and a number that fits an ullong "
     "with the parameters the other way around",
     bignum_set_string (test, "frgtbgnfsp9fdrhtgpiunvbgsiufbnfdgs", 36);
     bignum_set_ullong (test2, 0x715517FACEULL);
     bignum_and (test, test2, test),
     IS_HEX (test, "700514E0CC"));

#define A_100_ROUND_BITOP_TEST(op)                                            \
  TEST                                                                        \
    ("100 rounds of bignum_" #op "() with random big numbers",                \
     bignum_setbit (test3, 1024);                                             \
     for (i = 0, success = 1; success && i < 100; i++)                        \
       {                                                                      \
         char *hex; char *hex2; /* A comma here confuses the macro parser. */ \
                                                                              \
         bignum_random (test,  test3);                                        \
         bignum_random (test2, test3);                                        \
         bignum_rshift (i & 1 ? test : test2, i & 1 ? test : test2,           \
                        get_random() % 128);                                  \
                                                                              \
         ENSURE (hex  = BN_bn2hex (test));                                    \
         ENSURE (hex2 = BN_bn2hex (test2));                                   \
                                                                              \
         bignum_ ## op (test, test, test2);                                   \
         success = IS_HEX (test, stringly_ ## op(hex, hex2));                 \
                                                                              \
         OPENSSL_free (hex);                                                  \
         OPENSSL_free (hex2);                                                 \
       },                                                                     \
     success)

  A_100_ROUND_BITOP_TEST (and);

/* This is a random number. */
#define NICE_TEST_NUMBER "9999999999999999999999999999999999999999"

  TEST
    ("bignum_ior(0, 0)",
     bignum_set_long (test, 1); BN_zero (test2); BN_zero (test3);
     bignum_ior (test, test2, test3),
     IS_INT (test, 0));
  TEST
    ("bignum_ior() with numbers that fit ullongs",
     bignum_set_ullong (test,  0x1FFF00123000000ULL);
     bignum_set_ullong (test2, 0x40000032100FFFFULL);
     bignum_ior (test, test, test2),
     IS_INT (test, 0x1FFF00123000000ULL | 0x40000032100FFFFULL));
  TEST
    ("bignum_ior(0, number)",
     bignum_set_string (test, NICE_TEST_NUMBER, 16);
     BN_zero (test2);
     BN_zero (test3);
     bignum_ior (test2, test, test3),
     IS_HEX (test2, NICE_TEST_NUMBER));
  TEST
    ("bignum_ior(number, 0)",
     bignum_set_string (test, NICE_TEST_NUMBER, 16);
     BN_zero (test2);
     BN_zero (test3);
     bignum_ior (test2, test3, test),
     IS_HEX (test2, NICE_TEST_NUMBER));

  A_100_ROUND_BITOP_TEST (ior);

  TEST
    ("bignum_xor(0, 0)",
     bignum_set_long (test, 1); BN_zero (test2); BN_zero (test3);
     bignum_xor (test, test2, test3),
     IS_INT (test, 0));
  TEST
    ("bignum_xor() with numbers that fit ullongs",
     bignum_set_ullong (test,  0x1FFF00123000000ULL);
     bignum_set_ullong (test2, 0x40000032100FFFFULL);
     bignum_xor (test, test, test2),
     IS_INT (test, 0x1FFF00123000000ULL ^ 0x40000032100FFFFULL));
  TEST
    ("bignum_xor(0, number)",
     bignum_set_string (test, NICE_TEST_NUMBER, 16);
     BN_zero (test2);
     BN_zero (test3);
     bignum_xor (test2, test, test3),
     IS_HEX (test2, NICE_TEST_NUMBER));
  TEST
    ("bignum_xor(number, 0)",
     bignum_set_string (test, NICE_TEST_NUMBER, 16);
     BN_zero (test2);
     BN_zero (test3);
     bignum_xor (test2, test3, test),
     IS_HEX (test2, NICE_TEST_NUMBER));

  A_100_ROUND_BITOP_TEST (xor);

#undef A_100_ROUND_BITOP_TEST

  TEST
    ("bignum_not()",
     for (i = -15, success = 1; success && i <= 15; i++)
       {
         bignum_set_long (test, i);
         bignum_not (test, test);
         success = IS_INT (test, ~i);
       },
     success);

  TEST
    ("bignum_testbit()",
     bignum_set_ullong (test, 1 << 10),
     bignum_testbit (test, 10) && !bignum_testbit (test, 9));

  TEST
    ("bignum_setbit() set a bit to a number that fits a ullong",
     BN_zero (test);
     bignum_setbit (test, 34),
     IS_INT (test, 1ULL << 34));
  TEST
    ("bignum_setbit() sets a bit to become a big-ish number",
     BN_zero (test);
     bignum_setbit (test, 128),
     IS_HEX (test, "100000000000000000000000000000000"));
  TEST
    ("100 rounds of copying a number with bignum_testbit() and "
     "bignum_setbit()",
     for (success = 1, i = 0; success && i < 100; i++)
       {
         unsigned long long num = get_random ();
         int j;

         bignum_set_ulong (test, num);
         BN_zero (test2);

         for (j = 0; j < 64; j++)
           if (bignum_testbit (test, j))
             bignum_setbit (test2, j);

         success = bignum_eql (test, test2);
       },
     success);

  TEST
    ("bignum_clrbit() with a number that fits an ullong for a bit that is on",
     bignum_set_ullong (test, 0x40000321000FFFFULL);
     bignum_clrbit (test, 28),
     IS_INT (test, 0x40000320000FFFFULL));
  TEST
    ("bignum_clrbit() with a number that fits an ullong for a bit that is off",
     bignum_set_ullong (test, 0x40000321000FFFFULL);
     bignum_clrbit (test, 27),
     IS_INT (test, 0x40000321000FFFFULL));
  TEST
    ("bignum_clrbit() with a big-ish number for a bit that is on",
     bignum_set_string (test, NICE_TEST_NUMBER, 16);
     bignum_clrbit (test, 132),
     IS_HEX (test, "9999998999999999999999999999999999999999"));
  TEST
    ("bignum_clrbit() with a big-ish number for a bit that is off",
     bignum_set_string (test, NICE_TEST_NUMBER, 16);
     bignum_clrbit (test, 512),
     IS_HEX (test, NICE_TEST_NUMBER));
  TEST
    ("clearing all bits of a big number with bignum_clrbit()",
     bignum_setbit (test, 2048); bignum_sub (test, test, BN_value_one());
     for (i = 0; i <= 2048; i++) bignum_clrbit (test, i),
     IS_INT (test, 0));

#undef NICE_TEST_NUMBER

  bignum_fini (test);
  bignum_fini (test2);
  bignum_fini (test3);
}

static int
compare_to_string(bignum b, int hex, int type, ...)
{
  va_list ap;
  int ret, comparee_malloced = 0;
  char *val, *s, *comparee = NULL;

  ENSURE (s = val = hex ? BN_bn2hex (b) : BN_bn2dec (b));

  if (hex)
    {
      if (*s == '-' && s[1] == '0')
        *++s = '-';
      else if (*s == '0')
        s++;
    }

  va_start (ap, type);

  switch (type)
    {
    case 0:
      comparee = va_arg (ap, char *);
      break;

    case 1:
      {
        long long tmp = va_arg (ap, long long);
        comparee_malloced = 1;

        if (hex)
          ENASPRINTF ((&comparee, (tmp < 0 ? "-%llX" : "%llX"),
                       (unsigned long long) llabs (tmp)));
        else
          ENASPRINTF ((&comparee, "%lld", tmp));

        break;
      }

    case 2:
      {
        unsigned long long tmp = va_arg (ap, unsigned long long);
        comparee_malloced = 1;
        ENASPRINTF ((&comparee, hex ? "%llX" : "%llu", tmp));
        break;
      }
    }

  va_end (ap);

  if (!comparee)
    {
      print_error ("Invalid call to compare_to_string()\n");
      exit (1);
    }

  ret = !strcmp (s, comparee);

  if (!ret) print_error ("Value: %s\nExpected: %s\n", s, comparee);

  OPENSSL_free (val);
  if (comparee_malloced) free (comparee);

  return ret;
}

static void
print_error(const char *msg, ...)
{
  va_list ap;
  char *tmp;

  va_start (ap, msg);
  ENSURE (vasprintf (&tmp, msg, ap) != -1);
  va_end (ap);

  /* This should work in most terminals. Worst case scenario shows extra binary
     junk. */
  printf("\x1B[31m%s\x1B[37m", tmp);
  free(tmp);
}

void init_number_openssl_real(void);

void
init_number_openssl(void)
{
    init_number_openssl_real ();
    seed_random (time (NULL)); /* This is needed for the random fn tests. */
    tests ();
    exit (1);
}

#undef EN_MUL_WORD
#undef EN_SUB_WORD
#undef EN_ADD_WORD
#undef ENASPRINTF
#undef ENSURE
#undef IS_INT
#undef IS_UINT
#undef IS_HEX
#undef IS_DEC
#undef TEST

#define init_number_openssl init_number_openssl_real
