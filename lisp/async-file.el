;;; async-file.el --- async file IO and shell for XEmacs

(require 'async-core)
(require 'actor)

(defun async-read-file (path &rest keys)
  "Asynchronously read file PATH. Pass to `await' for string contents.
:coding CODING-SYSTEM"
  (let ((coding (or (plist-get keys :coding) 'utf-8)))
    (actor-spawn-internal nil
                          (lambda (args)
                            (let ((path   (car args))
                                  (coding (cadr args)))
                              (with-temp-buffer
                                (insert-file-contents-literally path)
                                (decode-coding-region (point-min) (point-max) coding)
                                (buffer-string))))
                          (list (list path coding)))))

(defun async-write-file (path content &rest keys)
  "Asynchronously write CONTENT to PATH. Pass to `await' for t.
:coding CODING-SYSTEM"
  (let ((coding (or (plist-get keys :coding) 'utf-8)))
    (actor-spawn-internal nil
                          (lambda (args)
                            (let ((path    (car args))
                                  (content (cadr args))
                                  (coding  (caddr args)))
                              (with-temp-buffer
                                (insert content)
                                (encode-coding-region (point-min) (point-max) coding)
                                (write-region (point-min) (point-max) path nil 'silent))
                              t))
                          (list (list path content coding)))))

(defun async-shell-command (cmd &rest keys)
  "Asynchronously run shell CMD. Pass to `await' for output string.
:timeout MS — not yet implemented; the argument is accepted and ignored.
Timeout support requires C-level WAIT_FD integration (future work)."
  (let ((_timeout (plist-get keys :timeout)))
    (actor-spawn-internal
     nil
     (lambda (args)
       (let* ((cmd (car args))
              (buf (generate-new-buffer " *async-shell*")))
         (unwind-protect
             (progn
               (call-process shell-file-name nil buf nil
                             shell-command-switch cmd)
               (with-current-buffer buf (buffer-string)))
           (when (buffer-live-p buf)
             (kill-buffer buf)))))
     (list (list cmd)))))

(provide 'async-file)
