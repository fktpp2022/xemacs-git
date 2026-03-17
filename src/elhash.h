/* Lisp interface to hash tables -- include file.
   Copyright (C) 1995, 1996 Ben Wing.
   Copyright (C) 2025 Free Software Foundation, Inc.

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

#ifndef INCLUDED_elhash_h_
#define INCLUDED_elhash_h_

typedef struct Lisp_Hash_Table Lisp_Hash_Table;

DECLARE_LISP_OBJECT (hash_table, Lisp_Hash_Table);

#define XHASH_TABLE(x) XRECORD (x, hash_table, Lisp_Hash_Table)
#define wrap_hash_table(p) wrap_record (p, hash_table)
#define HASH_TABLEP(x) RECORDP (x, hash_table)
#define CHECK_HASH_TABLE(x) CHECK_RECORD (x, hash_table)
#define CONCHECK_HASH_TABLE(x) CONCHECK_RECORD (x, hash_table)

typedef struct htentry
{
  Lisp_Object key;
  Lisp_Object value;
} htentry;

#define HTENTRY_CLEAR_P(htentry) ((*(EMACS_UINT*)(&((htentry)->key))) == 0)


enum hash_table_weakness
{
  HASH_TABLE_NON_WEAK,
  HASH_TABLE_KEY_WEAK,
  HASH_TABLE_VALUE_WEAK,
  HASH_TABLE_KEY_VALUE_WEAK,
  HASH_TABLE_KEY_CAR_WEAK,
  HASH_TABLE_VALUE_CAR_WEAK,
  HASH_TABLE_KEY_CAR_VALUE_WEAK,
  HASH_TABLE_WEAK
};

extern const struct memory_description hash_table_description[];

EXFUN (Fcopy_hash_table, 1);
EXFUN (Fhash_table_count, 1);
EXFUN (Fgethash, 3);
EXFUN (Fputhash, 3);
EXFUN (Fremhash, 2);
EXFUN (Fclrhash, 1);

typedef struct Hash_Table_Test Hash_Table_Test;

DECLARE_LISP_OBJECT (hash_table_test, struct Hash_Table_Test);
#define XHASH_TABLE_TEST(x) XRECORD (x, hash_table_test, struct Hash_Table_Test)
#define wrap_hash_table_test(p) wrap_record (p, hash_table_test)
#define HASH_TABLE_TESTP(x) RECORDP (x, hash_table_test)
#define CHECK_HASH_TABLE_TEST(x) CHECK_RECORD (x, hash_table_test)
#define CONCHECK_HASH_TABLE_TEST(x) CONCHECK_RECORD (x, hash_table_test)

typedef Boolint (*hash_table_equal_function_t) (const Hash_Table_Test *http,
						Lisp_Object obj1, Lisp_Object obj2);
typedef Hashcode (*hash_table_hash_function_t) (const Hash_Table_Test *http,
                                                Lisp_Object obj);
typedef Boolint (*hash_table_reorganize_needed_t) (Lisp_Object func,
                                                   Lisp_Object hash_table);
typedef int (*maphash_function_t) (Lisp_Object key, Lisp_Object value,
				   void* extra_arg);

/* TEST here is a Lisp_Object of type hash-table-test. You probably don't
   want to call this, unless you have registered your own test. */
Lisp_Object make_general_lisp_hash_table (Lisp_Object test,
					  Elemcount size,
					  double rehash_size,
					  double rehash_threshold,
					  enum hash_table_weakness weakness);

/* TEST here is a symbol, e.g. Qeq, Qequal. */
Lisp_Object make_lisp_hash_table (Elemcount size,
				  enum hash_table_weakness weakness,
                                  Lisp_Object test);

void elisp_maphash (maphash_function_t function,
		    Lisp_Object hash_table, void *extra_arg);

void elisp_maphash_unsafe (maphash_function_t function,
			   Lisp_Object hash_table, void *extra_arg);

void elisp_map_remhash (maphash_function_t predicate,
			Lisp_Object hash_table, void *extra_arg);

int finish_marking_weak_hash_tables (void);
void prune_weak_hash_tables (void);

/* Used at dump time; Qnull_pointer in normal use. */
extern Lisp_Object Vpdump_hash_table_reorganize_keys;

/* Used at runtime. */
extern Elemcount pdump_hash_table_reorganize_count;
extern Lisp_Object *pdump_hash_tables_for_reorganize;

void pdump_reorganize_hash_tables (void);

extern htentry *inchash (Lisp_Object key, Lisp_Object table, EMACS_INT offset);

extern htentry *find_htentry (Lisp_Object key, const Lisp_Hash_Table *ht);

extern Boolint general_hash_table_reorganize_needed_p (Lisp_Object func,
						       Lisp_Object hash_table);

extern Lisp_Object
define_hash_table_test (Lisp_Object name,
                        hash_table_equal_function_t equal_function,
                        Lisp_Object lisp_equal_function,
                        hash_table_hash_function_t hash_function,
                        Lisp_Object lisp_hash_function,
                        hash_table_reorganize_needed_t reorganize_needed_p,
                        Lisp_Object lisp_reorganize_needed_p);

void mark_hash_table_tests (void);

#endif /* INCLUDED_elhash_h_ */
