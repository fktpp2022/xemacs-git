;; Copyright (C) 2024 Free Software Foundation, Inc. -*- coding: iso-8859-1 -*-

;; Author: Aidan Kehoe <kehoea@parhasard.net>
;; Maintainer: Aidan Kehoe <kehoea@parhasard.net>
;; Created: 2024
;; Keywords: tests, call-interactively

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

;; Test processing of the (interactive ...) specifications.

(macrolet
    ((test-interactive-functions-with-code-letter (&rest args)
       (cons
	'progn
	(mapcar
	 (function*
	  (lambda ((code-letter passing-arg passing-interactive
				failing-interactive))
	   (let ((name (make-symbol
			(format "test-interactive-%c" code-letter)))
		 (block (make-symbol 
			 (format "block-fail-interactive-%c" code-letter))))
	     `(labels ((,name (arg)
			(interactive ,(string code-letter))
			(list arg pi e)))
	       (with-temp-buffer
		 (use-local-map (make-sparse-keymap))
		 (define-key (current-local-map) [space] #',name)
		 (Assert (equal (,name ,passing-arg)
				(list ,passing-arg ,pi ,e)) nil
				,(format "checking noninteractive call OK, \
#'test-interactive-%c" code-letter))
		 (Assert (equal (let ((unread-command-events
				       (nconc
					(mapcar #'character-to-event
						,passing-interactive)
					unread-command-events)))
				  (call-interactively #',name))
				(list ,passing-arg ,pi ,e)) nil
				,(format "checking interactive call OK, \
#'test-interactive-%c" code-letter))
 		 (Assert
 		  (member*
 		   (car-safe
 		    (block ,block
 		      (labels
 			  ((no-debug (symbol &optional cons)
 			     (if (member* 'invalid-operation
 					  (get (car cons)
 					       'error-conditions))
 				 (return-from ,block cons))
 			     (if (eq 'quit (car-safe symbol))
 				 (return-from ,block symbol))))
                        (let ((debugger #'no-debug)
                              (executing-kbd-macro t)
                               (unread-command-events
                                (nconc
                                 (mapcar #'character-to-event
                                         ,failing-interactive)
                                 unread-command-events)))
                           (call-with-condition-handler
                               #'no-debug
                               #'call-interactively #',name)
                           'not-reached))))
 		   '(invalid-operation quit))
 		  nil
 		  ,(format "checking interactive call fails with argument \
 `%s', interactive spec %S" failing-interactive code-letter))
		 )))))
	 args))))
  (test-interactive-functions-with-code-letter
   (?a 'car "car\n" "khar\n")
   (?b "*scratch*" "*scratch*\n" "flx4U3k3Jt/NEoCf\n")
   (?B "*scratch*" "*scratch*\n" "\C-g")
   (?c ?Z "Z" "\C-g")
   (?C 'find-file "find-file\n" "fajnd-fajl\n")
   (?n 123456 "123456\n" "\C-g")))

((macro
  .
  (lambda ()
    (let* ((name (make-symbol "test-interactive-%c-%s"))
	   (block (make-symbol 
		   (format "block-fail-%s" name))))
      `(labels ((,name (arg1 arg2)
		  (interactive "cChar: \nsString reflecting: %c")
		  (list arg1 arg2 pi e)))
	(with-temp-buffer
	  (use-local-map (make-sparse-keymap))
	  (define-key (current-local-map) [space] #',name)
	  (Assert (equal (,name ?z "hello")
		   (list ?z "hello" ,pi ,e)) nil
		   ,(format "checking noninteractive call OK, \
#'%s" name))
	  (Assert (equal (let ((unread-command-events
				(nconc
				 (mapcar #'character-to-event
					 "zhello\n")
				 unread-command-events)))
			   (call-interactively #',name))
		   (list ?z "hello" ,pi ,e)) nil
		   ,(format "checking interactive call OK, \
#'%s" name))))))))

;;; end of call-interactively.el
