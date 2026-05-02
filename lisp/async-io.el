
;;; async-io.el --- As
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

;; This file is
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option)
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;; XEmacs is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;; XEmacs is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
;; FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending,
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUT
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))


;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (func
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (prom
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (prom
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
  "Return a promise that resolves when any of PROMISES resolves."

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
  "Return a promise that resolves when any of PROMISES resolves."
  (promise-new
   (lambda (resolve reject)
     (let ((done nil
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
  "Return a promise that resolves when any of PROMISES resolves."
  (promise-new
   (lambda (resolve reject)
     (let ((done nil))
       (dotimes (i (length promises))
         (promise-then (
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
  "Return a promise that resolves when any of PROMISES resolves."
  (promise-new
   (lambda (resolve reject)
     (let ((done nil))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (unless done
                          (
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
  "Return a promise that resolves when any of PROMISES resolves."
  (promise-new
   (lambda (resolve reject)
     (let ((done nil))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (unless done
                          (setq done t)
                          (funcall resolve value)))
                      (lambda (error)
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
  "Return a promise that resolves when any of PROMISES resolves."
  (promise-new
   (lambda (resolve reject)
     (let ((done nil))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (unless done
                          (setq done t)
                          (funcall resolve value)))
                      (lambda (error)
                        ;; Ignore errors until all fail

;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
  "Return a promise that resolves when any of PROMISES resolves."
  (promise-new
   (lambda (resolve reject)
     (let ((done nil))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (unless done
                          (setq done t)
                          (funcall resolve value)))
                      (lambda (error)
                        ;; Ignore errors until all fail
                        )))))))

;; Async I/O functions using promises
(defun async-read-file-promise (
;;; async-io.el --- Asynchronous I/O library for XEmacs

;; Copyright (C) 2025 Free Software Foundation, Inc.

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
;; along with XEmacs.  If not, see &lt;http://www.gnu.org/licenses/&gt;.

;;; Commentary:

;; This library provides modern asynchronous I/O capabilities for XEmacs,
;; inspired by Go's goroutines and JavaScript's promises.

;;; Code:

(provide 'async-io)

;; Promise structure
(defstruct (promise (:constructor promise-create)
                    (:copier nil))
  state       ; :pending, :resolved, :rejected
  value       ; Result or error
  callbacks   ; List of (callback . error-callback)
  error-callbacks)

(defun promise-new (executor)
  "Create a new promise with EXECUTOR function.
EXECUTOR is called with two arguments: RESOLVE and REJECT functions."
  (let ((promise (promise-create
                  :state :pending
                  :value nil
                  :callbacks nil
                  :error-callbacks nil)))
    (condition-case err
        (funcall executor
                 (lambda (value)
                   (promise-resolve promise value))
                 (lambda (error)
                   (promise-reject promise error)))
      (error
       (promise-reject promise err)))
    promise))

(defun promise-resolve (promise value)
  "Resolve PROMISE with VALUE."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :resolved
          (promise-value promise) value)
    (dolist (callback (promise-callbacks promise))
      (condition-case err
          (funcall callback value)
        (error (message "Promise callback error: %s" err))))))

(defun promise-reject (promise error)
  "Reject PROMISE with ERROR."
  (when (eq (promise-state promise) :pending)
    (setf (promise-state promise) :rejected
          (promise-value promise) error)
    (dolist (callback (promise-error-callbacks promise))
      (condition-case err
          (funcall callback error)
        (error (message "Promise error callback error: %s" err))))))

(defun promise-then (promise on-success &amp;optional on-error)
  "Chain PROMISE with ON-SUCCESS and optional ON-ERROR callbacks."
  (promise-new
   (lambda (resolve reject)
     (let ((callback (lambda (value)
                       (if on-success
                           (condition-case err
                               (funcall resolve (funcall on-success value))
                             (error (funcall reject err)))
                         (funcall resolve value))))
           (err-callback (lambda (error)
                          (if on-error
                              (condition-case err
                                  (funcall resolve (funcall on-error error))
                                (error (funcall reject err)))
                            (funcall reject error)))))
       (if (eq (promise-state promise) :pending)
           (progn
             (push callback (promise-callbacks promise))
             (push err-callback (promise-error-callbacks promise)))
         (if (eq (promise-state promise) :resolved)
             (funcall callback (promise-value promise))
           (funcall err-callback (promise-value promise))))))))

(defmacro promise-chain (first &amp;rest rest)
  "Chain promises: FIRST then REST forms."
  (if rest
      (let ((result (make-symbol "result")))
        `(promise-then ,first
                      (lambda (,result)
                        (promise-chain ,@rest))))
    first))

(defun promise-all (promises)
  "Return a promise that resolves when all PROMISES resolve."
  (promise-new
   (lambda (resolve reject)
     (let ((results (make-vector (length promises) nil))
           (count 0))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (setf (aref results i) value
                              count (1+ count))
                        (when (= count (length promises))
                          (funcall resolve (append results nil))))
                      (lambda (error)
                        (funcall reject error))))))))

(defun promise-any (promises)
  "Return a promise that resolves when any of PROMISES resolves."
  (promise-new
   (lambda (resolve reject)
     (let ((done nil))
       (dotimes (i (length promises))
         (promise-then (aref promises i)
                      (lambda (value)
                        (unless done
                          (setq done t)
                          (funcall resolve value)))
                      (lambda (error)
                        ;; Ignore errors until all fail
                        )))))))

;; Async I/O functions using promises
(defun async-read-file-promise (filename)
  "Read FILENAME asynchron