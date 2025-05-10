;; mule-tests.el --- Test of international support     -*- coding: utf-8 -*-
;; Copyright (C) 1999 Free Software Foundation, Inc.
;; Copyright (C) 2010 Ben Wing.

;; Author: Hrvoje Niksic <hniksic@xemacs.org>
;; Maintainers: Hrvoje Niksic <hniksic@xemacs.org>,
;;              Martin Buchholz <martin@xemacs.org>
;; Created: 1999
;; Keywords: tests

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;; XEmacs is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
;; FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;; for more details.

;; You should have received a copy of the GNU General Public License
;; along with XEmacs.  If not, see <http://www.gnu.org/licenses/>.

;;; Synched up with: Not in FSF.

;;; Commentary:

;; Test some Mule functionality (most of these remain to be written) .
;; See test-harness.el for instructions on how to run these tests.

;; This file will be (read)ed by a non-mule XEmacs, so don't use
;; literal non-Latin1 characters.  Use (make-char) instead.

(require 'bytecomp)

;; This would give ?\x00 for a while on 64-bit XEmacs.

(Assert (eq (int-char most-negative-fixnum) nil)
	"checking for a bug with valid_ichar_p() on 64-bit builds")

;;-----------------------------------------------------------------
;; Test whether all legal chars may be safely inserted to a buffer.
;;-----------------------------------------------------------------

(defun* test-chars (&optional for-test-harness (limit char-code-limit))
  "Insert all characters in a buffer, to see if XEmacs will crash.
This is done by creating a string with all the legal characters 
\(see the documentation for the variable `char-code-limit' and the function
`char-int'), inserting it into the buffer, and checking that the buffer's
contents are equivalent to the string.

If FOR-TEST-HARNESS is specified, a temporary buffer is used, and
the Assert macro checks for correctness."
  (let ((list nil)
	(i 0))
    (while (< i limit)
      (and (not for-test-harness)
	   (zerop (% i 1000))
	   (message "%d" i))
      (and (int-char i)
	   ;; Don't aset to a string directly because random string
	   ;; access is O(n) under Mule.
	   (setq list (cons (int-char i) list)))
      (setq i (1+ i)))
    (let ((string
	   ;; This tests that xemacs_c_alloca () determines the stack direction
	   ;; correctly in passing. See the implementations of #'apply and
	   ;; #'string.
	   (apply #'string (nreverse list))))
      (if for-test-harness
	  ;; For use with test-harness, use Assert and a temporary
	  ;; buffer.
	  (with-temp-buffer
	    (insert string)
	    (Assert (equal (buffer-string) string)
		    "check that a large number of existing characters can be \
inserted into a buffer"))
	;; For use without test harness: use a normal buffer, so that
	;; you can also test whether redisplay works.
	(switch-to-buffer (get-buffer-create "test"))
	(erase-buffer)
	(buffer-disable-undo)
	(insert string)
	(assert (equal (buffer-string) string))))))

(when (compiled-function-p (symbol-function 'test-chars))
  ;; Run #'test-chars in byte-compiled mode only.
  (test-chars t
	      ;; unicode-internal has a value of #x40000000, (expt 2 30), for
	      ;; char-code-limit and even re-writing the above to avoid
	      ;; allocating the list and the string means I run out of memory
	      ;; when I attempt to run this.
	      (min char-code-limit #x200000)))
 
(defun unicode-code-point-to-utf-8-string (code-point)
  "Convert a Unicode code point to the equivalent UTF-8 string. 
This is a naive implementation in Lisp.  "
  (check-argument-type 'natnump code-point)
  (check-argument-range code-point 0 #x1fffff)
  (if (< code-point #x80)
      (format "%c" code-point)
    (if (< code-point #x800)
	(format "%c%c" 
		;; ochars[0] = 0xC0 | (input & ~(0xFFFFF83F)) >> 6;
		(logior #xc0 (lsh (logand code-point #x7c0) -6))
		;; ochars[1] = 0x80 | input & ~(0xFFFFFFC0);
		(logior #x80 (logand code-point #x3f)))
      (if (< code-point #x00010000)
	  (format "%c%c%c" 
		  ;; ochars[0] = 0xE0 | (input >> 12) & ~(0xFFFFFFF0); 
		  (logior #xe0 (logand (lsh code-point -12) #x0f))
		  ;; ochars[1] = 0x80 | (input >> 6) & ~(0xFFFFFFC0); 
		  (logior #x80 (logand (lsh code-point -6) #x3f))
		  ;; ochars[2] = 0x80 | input & ~(0xFFFFFFC0); 
		  (logior #x80 (logand code-point #x3f)))
	(if (< code-point #x200000)
	    (format "%c%c%c%c" 
		    ;; ochars[0] = 0xF0 | (input >> 18) & ~(0xFFFFFFF8)
		    (logior #xF0 (logand (lsh code-point -18) #x7))
		    ;; ochars[1] = 0x80 | (input >> 12) & ~(0xFFFFFFC0);
		    (logior #x80 (logand (lsh code-point -12) #x3f))
		    ;; ochars[2] = 0x80 | (input >> 6) & ~(0xFFFFFFC0); 
		    (logior #x80 (logand (lsh code-point -6) #x3f))
		    ;; ochars[3] = 0x80 | input & ~(0xFFFFFFC0); 
		    (logior #x80 (logand code-point #x3f))))))))

;;----------------------------------------------------------------
;; Test that revert-buffer resets the modiff
;; Bug reported 2007-06-20 <200706201902.32191.scop@xemacs.org>.
;; Fixed 2007-06-22 <18043.2793.611745.734215@parhasard.net>.
;;----------------------------------------------------------------

(let ((test-file-name
       (make-temp-file (expand-file-name "tXfXsKc" (temp-directory))))
      revert-buffer-function
      kill-buffer-hook)		; paranoia
  (find-file test-file-name)
  (erase-buffer)
  (insert "a string\n")
  (Silence-Message (save-buffer 0))
  (insert "more text\n")
  (revert-buffer t t)
  ;; Just "find-file" with autodetect coding didn't fail for me, but it does
  ;; fail under test harness.  Still we'll redo the test with an explicit
  ;; coding system just in case.
  (Assert (not (buffer-modified-p)))
  (kill-buffer nil)
  (when (find-coding-system 'utf-8)
    (find-file test-file-name 'utf-8)
    (insert "more text\n")
    (revert-buffer t t)
    (Assert (not (buffer-modified-p)))
    (kill-buffer nil))
  (delete-file test-file-name))

(let ((existing-file-name 
       (make-temp-file (expand-file-name "k7lCS2Mg" (temp-directory))))
      (nonexistent-file-name
       (make-temp-name (temp-directory))))
  (find-file existing-file-name)
  (Assert (not (eq 'undecided
                   (coding-system-type buffer-file-coding-system))))
  (kill-buffer nil)
  (dolist (coding-system '(utf-8 windows-1251 macintosh big5))
    (when (find-coding-system coding-system)
      (find-file existing-file-name coding-system)
      (Assert (eq (find-coding-system coding-system)
                  buffer-file-coding-system))
      (kill-buffer nil)
      (find-file nonexistent-file-name coding-system)
      (Assert (eq (find-coding-system coding-system)
                  buffer-file-coding-system))
      (set-buffer-modified-p nil)
      (kill-buffer nil)))
  (delete-file existing-file-name))

;; Make sure #'read isn't confused by a marker STREAM in a variable-length
;; buffer.

(with-temp-buffer
  (insert "\xe4quivalent")
  (goto-char (point-min))
  (Assert (eq (read (copy-marker (point)))
              (intern (concat (list ?\xe4) "quivalent")))))

;;-----------------------------------------------------------------
;; Check the range of the OCTET1 argument to #'make-char when CHARSET is
;; control-1.
;;-----------------------------------------------------------------

;; The following always worked:
(Assert (eq (make-char 'control-1 #x80) ?\x80))
(Assert (eq (make-char 'control-1 #x9f) ?\x9f))

;; The following never worked:
(Check-Error args-out-of-range (make-char 'control-1 most-positive-fixnum))
(Check-Error args-out-of-range (make-char 'control-1 -1))

;; The following used to work in 21.4 because any bits above eight were just
;; stripped, no other checks made on the range:
(Check-Error args-out-of-range (make-char 'control-1 most-negative-fixnum))
(Check-Error args-out-of-range (make-char 'control-1 #xFF00))

;; The following used to work after Ben's unicode internal work, where he
;; shifted the range of values accepted incompatibly:
(Check-Error args-out-of-range (make-char 'control-1 #x20))
(Check-Error args-out-of-range (make-char 'control-1 #x3f))

;; Check a #'split-char incompatibility introduced at the same time
;; has been fixed:
(Assert (equal '(control-1 0) (split-char (make-char 'control-1 0))))
(Assert (equal '(control-1 0) (split-char (make-char 'control-1 #x80))))
(Assert (equal '(control-1 31) (split-char (make-char 'control-1 #x1f))))
(Assert (equal '(control-1 31) (split-char (make-char 'control-1 #x9f))))

((macro
  . (lambda ()
      (cons
       'progn
       (mapcar
	(function*
	 (lambda ((count . result))
	  "Check all the OCTET1 values below #x100 have sane mappings\
with control-1"
	  `(Assert (eql ,result (ignore-errors
				  (make-char 'control-1 ,count))))))
	'((0 . ?\x80) (1 . ?\x81) (2 . ?\x82) (3 . ?\x83) (4 . ?\x84)
	  (5 . ?\x85) (6 . ?\x86) (7 . ?\x87) (8 . ?\x88) (9 . ?\x89)
	  (10 . ?\x8A) (11 . ?\x8B) (12 . ?\x8C) (13 . ?\x8D) (14 . ?\x8E)
	  (15 . ?\x8F) (16 . ?\x90) (17 . ?\x91) (18 . ?\x92) (19 . ?\x93)
	  (20 . ?\x94) (21 . ?\x95) (22 . ?\x96) (23 . ?\x97) (24 . ?\x98)
	  (25 . ?\x99) (26 . ?\x9A) (27 . ?\x9B) (28 . ?\x9C) (29 . ?\x9D)
	  (30 . ?\x9E) (31 . ?\x9F)
	  (32 . nil) (33 . nil) (34 . nil) (35 . nil) (36 . nil) (37 . nil)
	  (38 . nil) (39 . nil) (40 . nil) (41 . nil) (42 . nil) (43 . nil)
	  (44 . nil) (45 . nil) (46 . nil) (47 . nil) (48 . nil) (49 . nil)
	  (50 . nil) (51 . nil) (52 . nil) (53 . nil) (54 . nil) (55 . nil)
	  (56 . nil) (57 . nil) (58 . nil) (59 . nil) (60 . nil) (61 . nil)
	  (62 . nil) (63 . nil) (64 . nil) (65 . nil) (66 . nil) (67 . nil)
	  (68 . nil) (69 . nil) (70 . nil) (71 . nil) (72 . nil) (73 . nil)
	  (74 . nil) (75 . nil) (76 . nil) (77 . nil) (78 . nil) (79 . nil)
	  (80 . nil) (81 . nil) (82 . nil) (83 . nil) (84 . nil) (85 . nil)
	  (86 . nil) (87 . nil) (88 . nil) (89 . nil) (90 . nil) (91 . nil)
	  (92 . nil) (93 . nil) (94 . nil) (95 . nil) (96 . nil) (97 . nil)
	  (98 . nil) (99 . nil) (100 . nil) (101 . nil) (102 . nil)
	  (103 . nil) (104 . nil) (105 . nil) (106 . nil) (107 . nil)
	  (108 . nil) (109 . nil) (110 . nil) (111 . nil) (112 . nil)
	  (113 . nil) (114 . nil) (115 . nil) (116 . nil) (117 . nil)
	  (118 . nil) (119 . nil) (120 . nil) (121 . nil) (122 . nil)
	  (123 . nil) (124 . nil) (125 . nil) (126 . nil) (127 . nil)
	  (128 . ?\x80) (129 . ?\x81) (130 . ?\x82) (131 . ?\x83)
	  (132 . ?\x84) (133 . ?\x85) (134 . ?\x86) (135 . ?\x87)
	  (136 . ?\x88) (137 . ?\x89) (138 . ?\x8A) (139 . ?\x8B)
	  (140 . ?\x8C) (141 . ?\x8D) (142 . ?\x8E) (143 . ?\x8F)
	  (144 . ?\x90) (145 . ?\x91) (146 . ?\x92) (147 . ?\x93)
	  (148 . ?\x94) (149 . ?\x95) (150 . ?\x96) (151 . ?\x97)
	  (152 . ?\x98) (153 . ?\x99) (154 . ?\x9A) (155 . ?\x9B)
	  (156 . ?\x9C) (157 . ?\x9D) (158 . ?\x9E) (159 . ?\x9F)
	  (160 . nil) (161 . nil) (162 . nil) (163 . nil) (164 . nil)
	  (165 . nil) (166 . nil) (167 . nil) (168 . nil) (169 . nil)
	  (170 . nil) (171 . nil) (172 . nil) (173 . nil) (174 . nil)
	  (175 . nil) (176 . nil) (177 . nil) (178 . nil) (179 . nil)
	  (180 . nil) (181 . nil) (182 . nil) (183 . nil) (184 . nil)
	  (185 . nil) (186 . nil) (187 . nil) (188 . nil) (189 . nil)
	  (190 . nil) (191 . nil) (192 . nil) (193 . nil) (194 . nil)
	  (195 . nil) (196 . nil) (197 . nil) (198 . nil) (199 . nil)
	  (200 . nil) (201 . nil) (202 . nil) (203 . nil) (204 . nil)
	  (205 . nil) (206 . nil) (207 . nil) (208 . nil) (209 . nil)
	  (210 . nil) (211 . nil) (212 . nil) (213 . nil) (214 . nil)
	  (215 . nil) (216 . nil) (217 . nil) (218 . nil) (219 . nil)
	  (220 . nil) (221 . nil) (222 . nil) (223 . nil) (224 . nil)
	  (225 . nil) (226 . nil) (227 . nil) (228 . nil) (229 . nil)
	  (230 . nil) (231 . nil) (232 . nil) (233 . nil) (234 . nil)
	  (235 . nil) (236 . nil) (237 . nil) (238 . nil) (239 . nil)
	  (240 . nil) (241 . nil) (242 . nil) (243 . nil) (244 . nil)
	  (245 . nil) (246 . nil) (247 . nil) (248 . nil) (249 . nil)
	  (250 . nil) (251 . nil) (252 . nil) (253 . nil) (254 . nil)
	  (255 . nil)))))))

;;-----------------------------------------------------------------
;; Test string modification functions that modify the length of a char.
;;-----------------------------------------------------------------

(when (featurep 'mule)

  ;;-----------------------------------------------------------------
  ;; Some tests of the multibyte coding system
  ;;-----------------------------------------------------------------

  (let ((cap-y-umlaut (make-char 'latin-iso8859-15 190))
	(cap-y-umlaut2 (make-char 'latin-iso8859-16 190)))
    ;; Test that the `multibyte' coding system unifies characters by
    ;; Unicode codepoint.
    (Assert (equal (encode-coding-string cap-y-umlaut 'iso-8859-15) "¾"))
    (Assert (equal (encode-coding-string cap-y-umlaut 'iso-8859-16) "¾"))
    (Assert (equal (encode-coding-string cap-y-umlaut 'iso-8859-1) "?"))
    (Assert (equal (encode-coding-string cap-y-umlaut2 'iso-8859-15) "¾"))
    (Assert (equal (encode-coding-string cap-y-umlaut2 'iso-8859-16) "¾"))
    (Assert (equal (encode-coding-string cap-y-umlaut2 'iso-8859-1) "?")))

  ;;---------------------------------------------------------------
  ;; Test fillarray
  ;;---------------------------------------------------------------
  (macrolet
      ((fillarray-test
	(charset1 charset2)
	(let ((char1 (make-char charset1 69))
	      (char2 (make-char charset2 69)))
	  `(let ((string (make-string 1000 ,char1)))
	     (fillarray string ,char2)
	     (Assert (eq (aref string 0) ,char2))
	     (Assert (eq (aref string (1- (length string))) ,char2))
	     (Assert (eq (length string) 1000))))))
    (fillarray-test ascii latin-iso8859-1)
    (fillarray-test ascii latin-iso8859-2)
    (fillarray-test latin-iso8859-1 ascii)
    (fillarray-test latin-iso8859-2 ascii))

  ;; Test aset
  (let ((string (string (make-char 'ascii 69) (make-char 'latin-iso8859-2 69))))
    (aset string 0 (make-char 'latin-iso8859-2 42))
    (Assert (eq (aref string 1) (make-char 'latin-iso8859-2 69)))

    (when (fboundp 'string-char-byte-conversion-info)
      (let ((latin1 "aufw\xe4ndiges Basteln gef\xe4llt mir wenig"))
        (Assert (eql (count ?\xe4 latin1) 2))
        (Assert (eql (position ?\xe4 latin1)
                     (getf (string-char-byte-conversion-info latin1)
                           'ascii-begin)))
        (Assert (eql (aset latin1 4 ?e) ?e))
        (Assert (eql (count ?\xe4 latin1) 1))
        (Assert (eql (position ?\xe4 latin1)
                     (getf (string-char-byte-conversion-info latin1)
                           'ascii-begin))))))

  (let ((big-string (make-string 8200 ?a))
        (medium-string
         (make-string 8192 (make-char 'japanese-jisx0213-2 126 126)))
        (small-string (make-string 1 ?a)))
    ;; Test resize_string ().
    ;; 1. Big string resized to big string, growing.
    (fill big-string (make-char 'japanese-jisx0213-2 126 126))
    (garbage-collect)
    (Assert (eq (aref big-string 0) (make-char 'japanese-jisx0213-2 126 126)))
    ;; 2. Big string resized to big string, shrinking.
    (fill big-string ?z)
    (garbage-collect)
    (Assert (eq (aref big-string 0) ?z))
    ;; 3. Big string resized to small string (has to be shrinking).
    (fill medium-string ?a)
    (garbage-collect)
    (Assert (eq (aref medium-string 0) ?a))
    ;; 4. Small string changes size but allocation size does not (given
    ;; alignment constraints).
    (aset small-string 0 (make-char 'latin-iso8859-1 127))
    (garbage-collect)
    (Assert (eq (aref small-string 0) (make-char 'latin-iso8859-1 127)))

    ;; 5. Small string resized to big string (medium string is now a small
    ;; string).
    (fill medium-string (make-char 'japanese-jisx0213-2 126 126))
    (garbage-collect)
    (Assert (eq (aref medium-string 0)
                (make-char 'japanese-jisx0213-2 126 126)))

    ;; 6. Small string remaining small.
    (fill small-string (make-char 'japanese-jisx0213-2 126 126))
    (garbage-collect)
    (Assert (eq (aref small-string 0)
                (make-char 'japanese-jisx0213-2 126 126))))


  ;;---------------------------------------------------------------
  ;; Test coding system functions
  ;;---------------------------------------------------------------

  ;; Create alias for coding system without subsidiaries
  (Assert (coding-system-p (find-coding-system 'binary)))
  (Assert (coding-system-canonical-name-p 'binary))
  (Assert (not (coding-system-alias-p 'binary)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias)))
  (Assert (not (coding-system-canonical-name-p 'mule-tests-alias)))
  (Check-Error-Message
   error "Symbol is the canonical name of a coding system and cannot be redefined"
   (define-coding-system-alias 'binary 'iso8859-2))
  (Check-Error-Message
   error "Symbol is not a coding system alias"
   (coding-system-aliasee 'binary))

  (define-coding-system-alias 'mule-tests-alias 'binary)
  (Assert (coding-system-alias-p 'mule-tests-alias))
  (Assert (not (coding-system-canonical-name-p 'mule-tests-alias)))
  (Assert (eq (get-coding-system 'binary) (get-coding-system 'mule-tests-alias)))
  (Assert (eq 'binary (coding-system-aliasee 'mule-tests-alias)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias-unix)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias-dos)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias-mac)))

  (define-coding-system-alias 'mule-tests-alias (get-coding-system 'binary))
  (Assert (coding-system-alias-p 'mule-tests-alias))
  (Assert (not (coding-system-canonical-name-p 'mule-tests-alias)))
  (Assert (eq (get-coding-system 'binary) (get-coding-system 'mule-tests-alias)))
  (Assert (eq 'binary (coding-system-aliasee 'mule-tests-alias)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias-unix)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias-dos)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias-mac)))

  (define-coding-system-alias 'nested-mule-tests-alias 'mule-tests-alias)
  (Assert (coding-system-alias-p 'nested-mule-tests-alias))
  (Assert (not (coding-system-canonical-name-p 'nested-mule-tests-alias)))
  (Assert (eq (get-coding-system 'binary) (get-coding-system 'nested-mule-tests-alias)))
  (Assert (eq (coding-system-aliasee 'nested-mule-tests-alias) 'mule-tests-alias))
  (Assert (eq 'mule-tests-alias (coding-system-aliasee 'nested-mule-tests-alias)))
  (Assert (not (coding-system-alias-p 'nested-mule-tests-alias-unix)))
  (Assert (not (coding-system-alias-p 'nested-mule-tests-alias-dos)))
  (Assert (not (coding-system-alias-p 'nested-mule-tests-alias-mac)))

  (Check-Error-Message
   error "Attempt to create a coding system alias loop"
   (define-coding-system-alias 'mule-tests-alias 'nested-mule-tests-alias))
  (Check-Error-Message
   error "No such coding system"
   (define-coding-system-alias 'no-such-coding-system 'no-such-coding-system))
  (Check-Error-Message
   error "Attempt to create a coding system alias loop"
   (define-coding-system-alias 'mule-tests-alias 'mule-tests-alias))

  (define-coding-system-alias 'nested-mule-tests-alias nil)
  (define-coding-system-alias 'mule-tests-alias nil)
  (Assert (coding-system-p (find-coding-system 'binary)))
  (Assert (coding-system-canonical-name-p 'binary))
  (Assert (not (coding-system-alias-p 'binary)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias)))
  (Assert (not (coding-system-canonical-name-p 'mule-tests-alias)))
  (Check-Error-Message
   error "Symbol is the canonical name of a coding system and cannot be redefined"
   (define-coding-system-alias 'binary 'iso8859-2))
  (Check-Error-Message
   error "Symbol is not a coding system alias"
   (coding-system-aliasee 'binary))

  (define-coding-system-alias 'nested-mule-tests-alias nil)
  (define-coding-system-alias 'mule-tests-alias nil)

  ;; Create alias for coding system with subsidiaries
  (define-coding-system-alias 'mule-tests-alias 'iso-8859-7)
  (Assert (coding-system-alias-p 'mule-tests-alias))
  (Assert (not (coding-system-canonical-name-p 'mule-tests-alias)))
  (Assert (eq (get-coding-system 'iso-8859-7) (get-coding-system 'mule-tests-alias)))
  (Assert (eq 'iso-8859-7 (coding-system-aliasee 'mule-tests-alias)))
  (Assert (coding-system-alias-p 'mule-tests-alias-unix))
  (Assert (coding-system-alias-p 'mule-tests-alias-dos))
  (Assert (coding-system-alias-p 'mule-tests-alias-mac))

  (define-coding-system-alias 'mule-tests-alias (get-coding-system 'iso-8859-7))
  (Assert (coding-system-alias-p 'mule-tests-alias))
  (Assert (not (coding-system-canonical-name-p 'mule-tests-alias)))
  (Assert (eq (get-coding-system 'iso-8859-7) (get-coding-system 'mule-tests-alias)))
  (Assert (eq 'iso-8859-7 (coding-system-aliasee 'mule-tests-alias)))
  (Assert (coding-system-alias-p 'mule-tests-alias-unix))
  (Assert (coding-system-alias-p 'mule-tests-alias-dos))
  (Assert (coding-system-alias-p 'mule-tests-alias-mac))
  (Assert (eq (find-coding-system 'mule-tests-alias-mac)
	      (find-coding-system 'iso-8859-7-mac)))

  (define-coding-system-alias 'nested-mule-tests-alias 'mule-tests-alias)
  (Assert (coding-system-alias-p 'nested-mule-tests-alias))
  (Assert (not (coding-system-canonical-name-p 'nested-mule-tests-alias)))
  (Assert (eq (get-coding-system 'iso-8859-7)
	      (get-coding-system 'nested-mule-tests-alias)))
  (Assert (eq (coding-system-aliasee 'nested-mule-tests-alias) 'mule-tests-alias))
  (Assert (eq 'mule-tests-alias (coding-system-aliasee 'nested-mule-tests-alias)))
  (Assert (coding-system-alias-p 'nested-mule-tests-alias-unix))
  (Assert (coding-system-alias-p 'nested-mule-tests-alias-dos))
  (Assert (coding-system-alias-p 'nested-mule-tests-alias-mac))
  (Assert (eq (find-coding-system 'nested-mule-tests-alias-unix)
	      (find-coding-system 'iso-8859-7-unix)))

  (Check-Error-Message
   error "Attempt to create a coding system alias loop"
   (define-coding-system-alias 'mule-tests-alias 'nested-mule-tests-alias))
  (Check-Error-Message
   error "No such coding system"
   (define-coding-system-alias 'no-such-coding-system 'no-such-coding-system))
  (Check-Error-Message
   error "Attempt to create a coding system alias loop"
   (define-coding-system-alias 'mule-tests-alias 'mule-tests-alias))

  ;; Test dangling alias deletion
  (define-coding-system-alias 'mule-tests-alias nil)
  (Assert (not (coding-system-alias-p 'mule-tests-alias)))
  (Assert (not (coding-system-alias-p 'mule-tests-alias-unix)))
  (Assert (not (coding-system-alias-p 'nested-mule-tests-alias)))
  (Assert (not (coding-system-alias-p 'nested-mule-tests-alias-dos)))

  ;;---------------------------------------------------------------
  ;; Test strings waxing and waning across the 8k BIG_STRING limit (see
  ;; alloc.c)
  ;;---------------------------------------------------------------
  (defun charset-char-string (charset)
    (let ((gc-cons-threshold most-positive-fixnum)
	  string n
	  (chars (charset-chars charset))
	  (offset (charset-offset charset)))
      (if (= (charset-dimension charset) 1)
	  (progn
	    (setq string (make-string (charset-chars charset) ??))
	    (setq n 0)
	    (loop for j from offset to (+ offset chars -1)
	      for ch = (make-char charset j) do
	      (progn
		(when ch (aset string n ch))
		(incf n)))
	    (garbage-collect)
	    string)
	(let ((ch1 (first chars)) (ch2 (second chars))
	      (off1 (first offset)) (off2 (second offset)))
	  (setq string (make-string (* ch1 ch2) ??))
	  (setq n 0)
	  (loop for j from off1 to (+ off1 ch1 -1) do
	    (loop for k from off2 to (+ off2 ch2 -1)
	      for ch = (make-char charset j k) do
	      (progn
		(when ch (aset string n ch))
		(incf n))))
	  (garbage-collect)
	  string))))

  ;; The following two used to crash xemacs!
  (Assert (charset-char-string 'japanese-jisx0208))
  (aset (make-string 9003 ??) 1 (make-char 'latin-iso8859-1 77))

  (loop for (charset ch) in `((greek-iso8859-7 ??)
			      (ascii ,(make-char 'greek-iso8859-7 57)))
    do

    (let* ((charset-string (charset-char-string charset))
	   (len (length charset-string))
	   (string (make-string (* len 60) ch)))
      (loop for j from 0 below (length string) do
	(aset string j (aref charset-string (mod j len))))
      (loop for k in '(0 1 58 59) do
	(Assert (equal (substring string (* len k) (* len (1+ k)))
		       charset-string))))

    (let* ((charset-string (charset-char-string charset))
	   (len (length charset-string))
	   (string (make-string (* len 60) ch)))
      (loop for j from (1- (length string)) downto 0 do
	(aset string j (aref charset-string (mod j len))))
      (loop for k in '(0 1 58 59) do
	(Assert (equal (substring string (* len k) (* len (1+ k)))
		       charset-string))))
    )

  ;;---------------------------------------------------------------
  ;; Test string character conversion
  ;;---------------------------------------------------------------

  ;; #### This should test all coding systems!

  (let ((all-octets (let ((s (make-string 256 ?\000)))
		      (loop for i from (1- (length s)) downto 0 do
			(aset s i (int-char i)))
		      s))
	(escape-quoted-result (let ((schar '(27 155 142 143 14 15))
				    (s (make-string 262 ?\000))
				    (pos 0))
				(loop for ord from 0 to 255 do
				  (when (member ord schar)
				    (aset s pos ?\033)
				    (incf pos))
				  (aset s pos (int-char ord))
				  (incf pos))
				s)))
    (Assert (string= (encode-coding-string all-octets 'escape-quoted)
		     escape-quoted-result)))

  ;;---------------------------------------------------------------
  ;; Test file-system character conversion (and, en passant, file ops)
  ;;---------------------------------------------------------------
  (let* ((dstroke (make-char 'latin-iso8859-2 80))
	 (latin2-string (make-string 4 dstroke))
	 (prefix (concat (file-name-as-directory
			  (file-truename (temp-directory)))
			 latin2-string))
	 (file-name-coding-system
	  ;; 'iso-8859-X doesn't work on darwin (as of "Panther" 10.3), it
	  ;; seems to know that file-name-coding-system is definitely utf-8
	  (if (or (string-match "darwin" system-configuration)
		  (featurep 'cygwin-use-utf-8))
	      'utf-8
	    'iso-8859-2))
         ;; make-temp-name does stat(), which on OS X requires that you
         ;; normalise, where open() will normalise for you. Previously we
         ;; used scaron as the Latin-2 character, and make-temp-name errored
         ;; on OS X. LATIN CAPITAL LETTER D WITH STROKE does not decompose.
         (name1 (make-temp-name prefix))
         (name2 (make-temp-name prefix))
         (name3 (make-temp-name prefix))
         working-symlinks)
    (Assert (not (equal name1 name2)))
    (Assert (not (file-exists-p name1)))
    ;; This is how you suppress output from `message', called by `write-region'
    (Silence-Message
     (write-region (point-min) (point-max) name1))
    (Assert (file-exists-p name1))
    (Silence-Message 
     (write-region (point-min) (point-max) name3))
    (Assert (file-exists-p name3))
    (condition-case nil
        (make-symbolic-link name1 name3)
      (file-already-exists
       ;; If we actually have functioning symlinks, we end up here, since
       ;; name3 already exists and OK-IF-ALREADY-EXISTS was not specified.
       (setq working-symlinks t)))
    (when working-symlinks
      (make-symbolic-link name1 name2)
      (Assert (file-exists-p name2))
      (Assert (equal (file-truename name2) name1))
      (Assert (equal (file-truename name1) name1)))
    (ignore-file-errors (delete-file name1))
    (ignore-file-errors (delete-file name2))
    (ignore-file-errors (delete-file name3)))

  ;; Add many more file operation tests here...

  ;;---------------------------------------------------------------
  ;; Test Unicode-related functions
  ;;---------------------------------------------------------------
  (let* ((scaron '(latin-iso8859-2 185)))
    ;; Used to try #x0000, but you can't change ASCII or Latin-1
    (loop
      ;; We have to be VERY careful here not to mess up the Unicode
      ;; conversion tables.  The problem is that the code internally has
      ;; tables to convert in both directions; however, the two directions
      ;; are not equivalent in function, since a charset codepoint maps to
      ;; only one Unicode code, but a single Unicode code can map to
      ;; multiple charset codepoints.  Because the tables go in both
      ;; directions, we have to put both directions back when undoing a
      ;; single change.  Currently there is no way to put a null conversion
      ;; back, but that isn't a problem because the C code is able to
      ;; handle this case by itself.
      with initial-unicode = (apply 'charset-codepoint-to-unicode scaron)
      for code in '(#x0100 #x2222 #x4444 #xffff)
      for initial-codepoint = (unicode-to-charset-codepoint code
							    '(latin-iso8859-2))
      do
      (progn
	(apply 'set-unicode-conversion code scaron)
	(Assert (eq code (apply 'charset-codepoint-to-unicode scaron)))
	(Assert (equal scaron (unicode-to-charset-codepoint
			       code '(latin-iso8859-2))))
	(when initial-codepoint
	  (apply 'set-unicode-conversion code initial-codepoint)))
      finally (apply 'set-unicode-conversion initial-unicode scaron))
    (Check-Error 'invalid-argument (apply 'set-unicode-conversion -10000
					  scaron)))

  (Assert (not (natnump (char-to-unicode (make-char 'japanese-jisx0208
                                                    34 49))))
          "checking character with no Unicode mapping treated as such")

  (Assert (equal (decode-coding-string
                  (encode-coding-string (make-char 'japanese-jisx0208 34 49)
                                        'utf-8) 'utf-8)
                 "\uFFFD")
          "checking REPLACEMENT CHARACTER used correctly")

  (dolist (utf-8-char 
	   '("\xc6\x92"		  ;; U+0192 LATIN SMALL LETTER F WITH HOOK
	     "\xe2\x81\x8a"	  ;; U+204A TIRONIAN SIGN ET
	     "\xe2\x82\xae"	  ;; U+20AE TUGRIK SIGN
	     "\xf0\x9d\x92\xbd"	  ;; U+1D4BD MATHEMATICAL SCRIPT SMALL H
	     "\xf0\x9d\x96\x93"   ;; U+1D593 MATHEMATICAL BOLD FRAKTUR SMALL N
	     "\xf0\xaf\xa8\x88"   ;; U+2FA08 CJK COMPATIBILITY FOR U+4BCE
	     "\xf4\x8f\xbf\xbd")) ;; U+10FFFD <Plane 16 Private Use, Last>
    (let* ((xemacs-character (car (append 
				  (decode-coding-string utf-8-char 'utf-8) 
				  nil)))
	   (xemacs-charset (car (split-char xemacs-character))))

      ;; Trivial test of the UTF-8 support of the escape-quoted character set. 
      (Assert (equal (decode-coding-string utf-8-char 'utf-8)
		     (decode-coding-string (concat "\033%G" utf-8-char)
					   'escape-quoted)))

      ;; Check that the reverse mapping holds. 
      (Assert (equal (unicode-code-point-to-utf-8-string 
		      (encode-char xemacs-character 'ucs))
		     utf-8-char))

      ;; Check that, if this character has no corresponding ISO-2022 charset
      ;; (under old-Mule, this means it's been JIT-allocated), it is encoded
      ;; in escape-quoted using the corresponding UTF-8 escape. 
      (when (and xemacs-charset (not (charset-iso-2022-p xemacs-charset)))
	(Assert (equal (concat "\033%G" utf-8-char)
		       (encode-coding-string xemacs-character 'escape-quoted)))
	(Assert (equal (concat "\033%G" utf-8-char)
		       (encode-coding-string xemacs-character 'ctext))))))

  (loop
    for (code-point utf-16-big-endian utf-16-little-endian) 
    in '((#x10000 "\xd8\x00\xdc\x00" "\x00\xd8\x00\xdc")
         (#x10FFFD "\xdb\xff\xdf\xfd" "\xff\xdb\xfd\xdf"))
    do
    (Assert (equal (encode-coding-string 
                    (decode-char 'ucs code-point) 'utf-16)
                   utf-16-big-endian))
    (Assert (equal (encode-coding-string 
                    (decode-char 'ucs code-point) 'utf-16-le)
                   utf-16-little-endian)))

  ;; Create a 256x256 charset and test that we can assign Unicode codepoints
  ;; to all charset codepoints, and the conversion works in both directions.
  ;; Then remove all conversions and test that conversion in both directions
  ;; for all codepoints returns nil.  Among other things, this tests the
  ;; C code that handles charset codepoints equal to BADVAL_FROM_TABLE, and
  ;; tests that removal of codepoints works correctly.
  (when (not (find-charset 'mule-test-unicode))
    ;; The second time around (testing byte-compiled code), the charset
    ;; will already exist, and we can't create it again.
    (make-charset 'mule-test-unicode "Test Unicode"
		  '(dimension 2 chars 256 offset 0)))
  (let ((charset 'mule-test-unicode))
    (flet ((codepoint-to-unicode (c1 c2)
	     (+ 65536 (+ (* c1 256) c2))))
      (loop for c1 from 0 to 255 do
	(loop for c2 from 0 to 255 do
	  (set-unicode-conversion (codepoint-to-unicode c1 c2) charset c1 c2)))
      ;; #### We used to loop over all 256 rows; this was reasonably fast
      ;; when called noninteractively, but horrendously show when called
      ;; interactively.  I don't know why.
      (loop for c1 in '(0 1 254 255) do
	(loop for c2 from 0 to 255 do
	  (Assert (eq (charset-codepoint-to-unicode charset c1 c2)
		     (codepoint-to-unicode c1 c2)))
	  (Assert (equal (unicode-to-charset-codepoint
			  (codepoint-to-unicode c1 c2) (list charset))
			 `(,charset ,c1 ,c2)))))
      (loop for c1 from 0 to 255 do
	(loop for c2 from 0 to 255 do
	  (set-unicode-conversion nil charset c1 c2)))
      (loop for c1 in '(0 1 254 255) do
	(loop for c2 from 0 to 255 do
	  (Assert (eq nil (charset-codepoint-to-unicode charset c1 c2)))
	  (Assert (equal (unicode-to-charset-codepoint
			  (codepoint-to-unicode c1 c2) (list charset))
			 nil))))))

  ;;---------------------------------------------------------------
  ;; Regression test for a couple of CCL-related bugs. 
  ;;---------------------------------------------------------------

  (let ((ccl-vector [0 0 0 0 0 0 0 0 0]))
    (define-ccl-program ccl-write-two-control-1-chars 
      `(1 
	((r0 = ,(charset-id 'control-1))
	 (r1 = 128) 
	 (write-multibyte-character r0 r1) 
	 (r1 = 159) 
	 (write-multibyte-character r0 r1))) 
      "CCL program that writes two control-1 multibyte characters.") 
 
    (Assert (equal 
	     (ccl-execute-on-string 'ccl-write-two-control-1-chars  
				    ccl-vector "") 
	     (format "%c%c" (make-char 'control-1 128) 
		     (make-char 'control-1 159))))

    (define-ccl-program ccl-unicode-two-control-1-chars 
      `(1 
	((r0 = ,(charset-id 'control-1))
	 (r1 = 159) 
	 (mule-to-unicode r0 r1) 
	 (r4 = r0) 
	 (r3 = ,(charset-id 'control-1))
	 (r2 = 128) 
	 (mule-to-unicode r3 r2))) 
      "CCL program that writes two control-1 UCS code points in r3 and r4")

    ;; Re-initialise the vector, mainly to clear the instruction counter,
    ;; which is its last element.
    (setq ccl-vector [0 0 0 0 0 0 0 0 0])
 
    (ccl-execute-on-string 'ccl-unicode-two-control-1-chars ccl-vector "") 
 
    (Assert (and (eq (aref ccl-vector 3)  
                   (encode-char (make-char 'control-1 128) 'ucs)) 
               (eq (aref ccl-vector 4)  
                   (encode-char (make-char 'control-1 159) 'ucs)))))


  ;; Test the 8 bit fixed-width coding systems for round-trip
  ;; compatibility with themselves.
  (loop
    for coding-system in (coding-system-list)
    with all-possible-octets = (apply #'string
				      (loop for i from ?\x00 to ?\xFF
					collect i))
    do
    (when (and (eq 'multibyte (coding-system-type coding-system))
	       ;; Should have only dimension-1 charsets.
	       (every #'(lambda (x)
			  (= 1 (charset-dimension x)))
		      (coding-system-property coding-system 'charsets))
	       ;; Don't check the coding systems with odd line endings
	       ;; (maybe we should):
	       (eq 'lf (coding-system-eol-type coding-system)))
      ;; These coding systems are round-trip compatible with themselves.
      (Assert (equal (encode-coding-string 
		      (decode-coding-string all-possible-octets
					    coding-system)
		      coding-system)
		     all-possible-octets)
              (format "checking %s is transparent" coding-system))))

  ;;---------------------------------------------------------------
  ;; Test charset-in-* functions
  ;;---------------------------------------------------------------
  (with-temp-buffer
    (let ((sorted-charsets-in-HELLO
           '(arabic-iso8859-6 ascii chinese-big5-1 chinese-gb2312
             cyrillic-iso8859-5 ethiopic greek-iso8859-7 hebrew-iso8859-8
             indian-is13194 japanese-jisx0208 japanese-jisx0212
             katakana-jisx0201 korean-ksc5601 lao latin-iso8859-1
             latin-iso8859-2 latin-iso8859-3 latin-iso8859-4 thai-tis620
             tibetan vietnamese-viscii-lower))
	  (coding-system-for-read 'iso-2022-7))
      (insert-file-contents (locate-data-file "HELLO"))
      (Assert (equal 
       ;; The sort is to make the algorithm of charsets-in-region
       ;; irrelevant.
       (sort (remove* "^jit-ucs-charset-" (charsets-in-region (point-min)
                                                              (point-max))
                      :test 'string-match :key 'symbol-name)
	     #'string<)
       sorted-charsets-in-HELLO))
      (Assert (equal 
       (sort (remove* "^jit-ucs-charset-" (charsets-in-string
                                           (buffer-substring (point-min)
                                                             (point-max)))
                      :test 'string-match :key 'symbol-name)
	     #'string<)
       sorted-charsets-in-HELLO))))

  ;;---------------------------------------------------------------
  ;; Language environments, and whether the specified values are sane.
  ;;---------------------------------------------------------------
  (loop
    for language in (mapcar #'car language-info-alist)
    with language-input-method = nil
    with native-coding-system = nil
    with original-language-environment = current-language-environment
    do
    ;; s-l-e can call #'require, which says "Loading ..."
    (Silence-Message (set-language-environment language))
    (Assert (equal language current-language-environment))

    (setq language-input-method
	  (get-language-info language 'input-method))
    (when (and language-input-method
               ;; #### Not robust, if more input methods besides canna are
               ;; in core.  The intention of this is that if *any* of the
               ;; packages' input methods are available, we check that *all*
               ;; of the language environments' input methods actually
               ;; exist, which goes against the spirit of non-monolithic
               ;; packages. But I don't have a better approach to this.
               (> (length input-method-alist) 1))
      (Assert (assoc language-input-method input-method-alist))
      (Skip-Test-Unless
       (assoc language-input-method input-method-alist)
       "input method unavailable"
       (format "check that IM %s can be activated" language-input-method)
       ;; s-i-m can load files.
       (Silence-Message
	(set-input-method language-input-method))
       (Assert (equal language-input-method current-input-method))))

    (dolist (charset (get-language-info language 'charset))
      (Assert (charset-or-charset-tag-p (find-charset charset))))
    (dolist (coding-system (get-language-info language 'coding-system))
      (Assert (coding-system-p (find-coding-system coding-system))))
    (dolist (coding-system
             (if (listp (setq native-coding-system
                              (get-language-info language
                                                 'native-coding-system)))
                 native-coding-system
               (list native-coding-system)))
      ;; We don't have the appropriate POSIX locales to test with a
      ;; native-coding-system that is a function.
      (unless (functionp coding-system)
	(Assert (coding-system-p (find-coding-system coding-system)))))
    finally (set-language-environment original-language-environment))

  (with-temp-buffer
    (labels
        ((Assert-elc-is-escape-quoted ()
           "Assert the current buffer has an escape-quoted cookie if compiled."
           (save-excursion
             (let* ((temporary-file-name (make-temp-name
					  (expand-file-name "zjPQ2Pk"
							    (temp-directory))))
		    (byte-compile-result
                     (Silence-Message (byte-compile-from-buffer
                                       (current-buffer) temporary-file-name
                                       nil))))
               (Assert (string-match
                        "^;;;###coding system: escape-quoted"
                        (buffer-substring nil nil byte-compile-result))))))
         (Assert-elc-is-raw-text-unix ()
           "Assert the current buffer has no coding cookie if compiled."
           (save-excursion
             (let* ((temporary-file-name (make-temp-name
					  (expand-file-name "zjPQ2Pk"
							    (temp-directory))))
                    (byte-compile-result
                     (Silence-Message
                      (byte-compile-from-buffer (current-buffer)
                                                temporary-file-name nil))))
               (Assert (string-match
                        "^;;;###coding system: raw-text-unix"
                        (buffer-substring nil nil byte-compile-result)))))))
      (insert 
       ;; Create a buffer with Unicode escapes. The #'read call is at
       ;; runtime, because this file may be compiled and read in a non-Mule
       ;; XEmacs. (But it won't be run.)
       (read 
        "#r\" (defvar testing-mule-compilation-handling 
            (string ?\\u371E   ;; kDefinition beautiful; pretty, used 
                              ;; in girl's name
                ?\\U0002A6A9   ;; kDefinition	(Cant.) sound of shouting
                ?\\U0002A65B   ;; kDefinition	(Cant.) decayed teeth; 
                              ;; tongue-tied
                ?\\U00010400   ;; DESERET CAPITAL LETTER LONG I
                    ?\\u3263)) ;; CIRCLED HANGUL RIEUL \""))

      (Assert-elc-is-escape-quoted)
      (delete-region (point-min) (point-max))

      (insert
       ;; This time, the buffer will contain the actual characters, because of
       ;; u flag to the #r. 
       (read 
        "#ru\" (defvar testing-mule-compilation-handling 
            (string ?\\u371E   ;; kDefinition beautiful; pretty, used 
                              ;; in girl's name
                ?\\U0002A6A9   ;; kDefinition	(Cant.) sound of shouting
                ?\\U0002A65B   ;; kDefinition	(Cant.) decayed teeth; 
                              ;; tongue-tied
                ?\\U00010400   ;; DESERET CAPITAL LETTER LONG I
                    ?\\u3263)) ;; CIRCLED HANGUL RIEUL \""))
    
      (Assert-elc-is-escape-quoted)
      (delete-region (point-min) (point-max))

      (insert
       ;; Just a single four character escape. 
       (read
        "#r\" (defvar testing-mule-compilation-handling 
            (string ?\\u371E))   ;; kDefinition beautiful; pretty, used\""))

      (Assert-elc-is-escape-quoted)
      (delete-region (point-min) (point-max))

      (insert
       ;; Just a single eight character escape. 
       (read 
        "#r\" (defvar testing-mule-compilation-handling 
            (string ?\\U0002A65B))   ;; kDefinition (Cant.) decayed teeth;\""))

      (Assert-elc-is-escape-quoted)
      (delete-region (point-min) (point-max))

      (insert
       ;; A single latin-1 hex digit escape No run-time #'read call,
       ;; non-Mule can handle this too.
       #r" (defvar testing-mule-compilation-handling 
         (string ?\xab))   ;; LEFT-POINTING DOUBLE ANGLE QUOTATION MARK")
      
      (Assert-elc-is-raw-text-unix)
      (delete-region (point-min) (point-max))

      (insert
       ;; A single latin-1 character. No run-time #'read call.
       #ru" (defvar testing-mule-compilation-handling 
        (string ?\u00AB))   ;; LEFT-POINTING DOUBLE ANGLE QUOTATION MARK\")")
      
      (Assert-elc-is-raw-text-unix)
      (delete-region (point-min) (point-max))

      (insert
       ;; Just ASCII. No run-time #'read call
       #r" (defvar testing-mule-compilation-handling 
            (string ?A))   ;; LATIN CAPITAL LETTER A")
      
      (Assert-elc-is-raw-text-unix)
      (delete-region (point-min) (point-max))

      ;; There used to be a bug here because the coding-cookie insertion code
      ;; looks at the input buffer, not the output buffer.
      ;;
      ;; It looks at the input buffer because byte-compile-dynamic and
      ;; byte-compile-dynamic-docstrings currently need to be
      ;; unconditionally turned off for Mule files, since dynamic
      ;; compilation of function bodies and docstrings fails if you can't
      ;; call (point) and trivially get the byte offset in the file.
      ;;
      ;; And to unconditionally turn those two features off, you need to know
      ;; before byte-compilation whether the byte-compilation output file
      ;; contains non-Latin-1 characters. Or to check after compilation and
      ;; redo; the latter is what we do right now. This will only be necessary
      ;; in a very small minority of cases, it's not a performance-critical
      ;; issue.
      ;; 
      ;; Martin Buchholz thinks, in bytecomp.el, that we should implement lazy
      ;; loading for Mule files; I (Aidan Kehoe) don't think that's worth the
      ;; effort today (February 2009).
      (insert
       "(defvar testing-mule-compilation-handling (eval-when-compile
	(decode-char 'ucs #x371e))) ;; kDefinition beautiful; pretty, used\"")
      (Assert-elc-is-escape-quoted)
      (delete-region (point-min) (point-max))))

  (labels
      ((read-all-unicode ()
         (with-temp-buffer
           (loop for i from #x0 to #x10FFFF 
                 with exceptions = #s(range-table type start-closed-end-closed
                                                  data ((#xFFFE #xFFFF) t
                                                        (#xFDD0 #xFDEF) t
                                                        (#xD800 #xDBFF) t
                                                        (#xDC00 #xDFFF) t))
                 with result = t
                 do (unless (get-range-table i exceptions)
                      (delete-region (point-min) (point-max))
                      (format-into (current-buffer)
                                   (if (> i #xFFFF) #r"?\U%08X" #r"?\u%04X")
                                   i)
                      (goto-char (point-min))
                      (setq result (and (characterp (read (current-buffer)))
                                        result)))
                 finally (Assert result)))))
    (when (compiled-function-p #'read-all-unicode)
      (if (featurep 'unicode-internal)
          (read-all-unicode)
        (Known-Bug-Expect-Error syntax-error (read-all-unicode)))))

  (loop
    for i from #x00 to #xff
    do (Assert
        (= 1 (length (decode-coding-string (format "%c" i) 'utf-8-unix)))
        (format 
         "checking Unicode coding systems behave well with short input, %02X"
         i)))

  ;;---------------------------------------------------------------
  ;; Process tests
  ;; #### Should do network too.
  ;;---------------------------------------------------------------
  (Skip-Test-Unless (and (file-exists-p "/dev/null")
			 (fboundp 'executable-find)
			 (executable-find "cat"))
      "cat(1) or /dev/null missing"
      "Test that default-process-coding-system can be nil."
    (with-temp-buffer
      (Assert (let (default-process-coding-system)
		(shell-command "cat </dev/null >/dev/null")
		t))))

  ;; Test the Lisp printer when printing a string that has its data relocated
  ;; in the course of printing, and, relatedly, when printing a symbol that
  ;; has its name relocated in the course of printing.
  (let
      ((string
        (decode-coding-string
         ;; "ƒƒƒƒƒƒƒƒ\\a\\ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒ")
         (concat "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6"
                 "\x92\x5c\x61\x5c\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6"
                 "\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92"
                 "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6"
                 "\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92"
                 "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6"
                 "\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92"
                 "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92")
         'utf-8))
       (scratch-output (get-buffer-create (generate-new-buffer-name
                                           " *temp*")))
       symbol)
    (labels
        ((stream (character)
           (when (eq character ?a)
             (fill string ?a)
             (aset string (- (length string) 3) ?\")
	     (aset string 8 ?\\)
	     (aset string 10 ?\\))
           (write-char character scratch-output))
         (stream1 (character)
           (when (eq character ?\\)
             (fill string ?b))
           (write-char character scratch-output)))
      (Assert (zerop (- (point-max scratch-output) (point-min scratch-output)))
              "checking for empty buffer")
      (print string #'stream)
      (Assert (eql (- (point-max scratch-output) (point-min scratch-output))
                   ;; #'print adds two newlines.
                   (+ (length (prin1-to-string string)) 2))
              "correct number of chars printed despite byte len changes")
      (Assert (eql (encode-char (char-after 3 scratch-output) 'ucs)
                   #x0192)
              "first character printed as expected before byte len change")
      (Assert (eql ?a (char-after (- (point-max scratch-output) 4)
                                  scratch-output))
              "character printed as expected after byte len change")
      (delete-region (point-min scratch-output) (point-max scratch-output)
                     scratch-output)
      (setq string
            ;; "ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒ\\a\\ƒƒƒƒƒƒƒƒƒƒƒƒƒƒ")
            (decode-coding-string
             (concat "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6"
                     "\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92"
                     "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6"
                     "\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92"
                     "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6"
                     "\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92"
                     "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\x5c\x61\x5c\xc6\x92"
                     "\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6"
                     "\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92\xc6\x92")
             'utf-8))
      (Check-Error invalid-state (print string #'stream))
      (delete-region (point-min scratch-output) (point-max scratch-output)
                     scratch-output)

      ;; "ƒƒƒ;aa"
      (setq string (decode-coding-string "\xc6\x92\xc6\x92\xc6\x92;aa" 'utf-8)
            symbol (make-symbol string))

      ;; This would crash previously, since the offset inside print_symbol
      ;; became invalid:
      (print symbol #'stream1)
      (Assert (equal (buffer-string scratch-output)
                     (decode-coding-string
                      "\n#:\xc6\x92\xc6\x92\xc6\x92\\bbb\n"
                      'utf-8))
              "checking printing a symbol with relocated name does something")
      (kill-buffer scratch-output)))

  ;;; Test suite for truncate-string-to-width, from Colin Walters' tests in
  ;;; mule-util.el in GNU.
  (macrolet
      ((test-truncate-string-to-width (&rest tests)
         (let ((decode-any-string
                ;; We can't store the East Asian characters directly in this
                ;; file, since it needs to be read (but not executed) by
                ;; non-Mule. Store them as UTF-8, decode them at
                ;; macro-expansion time.
                #'(lambda (object)
                    (if (stringp object)
                        (decode-coding-string object 'utf-8)
                      object))))
           (cons
            'progn
            (mapcar
             (function*
              (lambda ((arguments . result))
                `(Assert (equal (truncate-string-to-width
                               ,@(mapcar decode-any-string arguments))
                                ,(funcall decode-any-string result)))))
             tests)))))
    (test-truncate-string-to-width
      (("" 0) . "")
      (("x" 1) . "x")
      (("xy" 1) . "x")
      (("xy" 2 1) . "y")
      (("xy" 0) . "")
      (("xy" 3) . "xy")
      (("\344\270\255" 0) . "")
      (("\344\270\255" 1) . "")
      (("\344\270\255" 2) . "\344\270\255")
      (("\344\270\255" 1 nil ? ) . " ")
      (("\344\270\255\346\226\207" 3 1 ? ) . "  ")
      (("x\344\270\255x" 2) . "x")
      (("x\344\270\255x" 3) . "x\344\270\255")
      (("x\344\270\255x" 3) . "x\344\270\255")
      (("x\344\270\255x" 4 1) . "\344\270\255x")
      (("kor\355\225\234e\352\270\200an" 8 1 ? ) .
       "or\355\225\234e\352\270\200")
      (("kor\355\225\234e\352\270\200an" 7 2 ? ) . "r\355\225\234e ")
      (("" 0 nil nil "...") . "")
      (("x" 3 nil nil "...") . "x")
      (("\344\270\255" 3 nil nil "...") . "\344\270\255")
      (("foo" 3 nil nil "...") . "foo")
      (("foo" 2 nil nil "...") . "fo") ;; (old) XEmacs failure?
      (("foobar" 6 0 nil "...") . "foobar")
      (("foobarbaz" 6 nil nil "...") . "foo...")
      (("foobarbaz" 7 2 nil "...") . "ob...")
      (("foobarbaz" 9 3 nil "...") . "barbaz")
      (("\343\201\223h\343\202\223e\343\201\253l\343\201\241l\343\201\257o" 15
        1 ?  t) . " h\343\202\223e\343\201\253l\343\201\241l\343\201\257o")
      (("\343\201\223h\343\202\223e\343\201\253l\343\201\241l\343\201\257o" 14
        1 ?  t) . " h\343\202\223e\343\201\253l\343\201\241...")
      (("x" 3 nil nil "\347\262\265\350\252\236") . "x")
      (("\344\270\255" 2 nil nil "\347\262\265\350\252\236") . "\344\270\255")
      ;; XEmacs used to error
      (("\344\270\255" 1 nil ?x "\347\262\265\350\252\236") . "x") 
      (("\344\270\255\346\226\207" 3 nil ?  "\347\262\265\350\252\236") .
       ;; XEmacs used to error
       "\344\270\255 ") 
      (("foobarbaz" 4 nil nil  "\347\262\265\350\252\236") .
       "\347\262\265\350\252\236")
      (("foobarbaz" 5 nil nil  "\347\262\265\350\252\236") .
       "f\347\262\265\350\252\236")
      (("foobarbaz" 6 nil nil  "\347\262\265\350\252\236") .
       "fo\347\262\265\350\252\236")
      (("foobarbaz" 8 3 nil "\347\262\265\350\252\236") .
       "b\347\262\265\350\252\236")
      (("\343\201\223h\343\202\223e\343\201\253l\343\201\241l\343\201\257o" 14
        4 ?x "\346\227\245\346\234\254\350\252\236") .
        "xe\343\201\253\346\227\245\346\234\254\350\252\236")
      (("\343\201\223h\343\202\223e\343\201\253l\343\201\241l\343\201\257o" 13
        4 ?x "\346\227\245\346\234\254\350\252\236") .
        "xex\346\227\245\346\234\254\350\252\236")))
  ) ; end of tests that require MULE built in.

;; Test detection of coding cookies.
(save-excursion
  (let ((test-file-name
         (make-temp-file (expand-file-name "nn724rW" (temp-directory))))
        revert-buffer-function
        kill-buffer-hook		; paranoia
        (current-no-conversion-category
         (coding-category-system 'no-conversion)))
    (unwind-protect
         (progn
           (find-file test-file-name 'no-conversion-unix)
           (erase-buffer)
           (insert "-*- coding: koi8-r -*-\n")
           (Silence-Message (save-buffer 0))
           (kill-buffer (current-buffer))
           (Assert (equal (find-coding-system-magic-cookie-in-file
                           test-file-name)
                          "koi8-r"))
           (find-file test-file-name)
           (Assert (eq buffer-file-coding-system 'koi8-r-unix))
           (erase-buffer)
           (setq buffer-file-coding-system 'utf-8-unix)
           (insert "Der Vogelfänger bin ich ja Local variables: stets lustig
Der Vogelfänger bin ich ja coding: iso-8859-2 stets lustig
Der Vogelfänger bin ich ja end: stets lustig\n")
           (Silence-Message (save-buffer 0))
           (kill-buffer (current-buffer))
           (Assert (equal (find-coding-system-magic-cookie-in-file
                           test-file-name) "iso-8859-2"))
           (find-file test-file-name)
           (Assert (eq buffer-file-coding-system 'iso-8859-2-unix))
           (erase-buffer)
           (setq buffer-file-coding-system 'utf-8-unix)

           ;; Invalid syntax, coding detection should give nil.
           (insert "Der Vogelfänger bin ich ja Local variables: stets lustig
Der Vogelfänger bin ich ja coding: iso-8859-2
Der Vogelfänger bin ich ja end: stets lustig\n")
           (Silence-Message (save-buffer 0))
           (kill-buffer (current-buffer))
           (set-coding-category-system 'no-conversion 'iso-8859-7)
           (Assert (eq (find-coding-system-magic-cookie-in-file
                        test-file-name) nil))
           (flet ((hack-local-variables-last-page (&optional force)))
             (find-file test-file-name))
           (Assert (not (eq buffer-file-coding-system 'iso-8859-2-unix)))
           (erase-buffer)
           (setq buffer-file-coding-system 'utf-8-unix)

           ;; Last page after the local variables specificiation, coding
           ;; detection should give nil.
           (insert "Der Vogelfänger bin ich ja Local variables: stets lustig
Der Vogelfänger bin ich ja coding: iso-8859-2
Der Vogelfänger bin ich ja end: stets lustig
Ich Vogelfänger bin bekannt, bei alt und jung in ganzem Land\n")
           (Silence-Message (save-buffer 0))
           (kill-buffer (current-buffer))
           (Assert (eq (find-coding-system-magic-cookie-in-file
                        test-file-name) nil))
           (flet ((hack-local-variables-last-page (&optional force)))
             (find-file test-file-name))
           (Assert (not (eq buffer-file-coding-system 'iso-8859-2-unix)))
           (Silence-Message (save-buffer 0))
           (kill-buffer (current-buffer)))
      (progn
        (delete-file test-file-name)
        (set-coding-category-system 'no-conversion
                                    current-no-conversion-category)))))
;;; end of mule-tests.el
