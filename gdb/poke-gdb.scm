;; poke-gdb.scm -- GDB extensions for debugging GNU poke

;; Copyright (C) 2018 Jose E. Marchesi

;; This program is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

(use-modules (gdb))

;; Pretty-printer for pvm_val objects.

(define (make-poke-pvm-val-printer value)
  "Print a pvm_val object"
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (let ((pvm-tag (value-logand value #b111)))
       (case (value->integer pvm-tag)
         ((#x0) ;; PVM_VAL_TAG_INT
          (let* ((int-size (value-add (value-cast (value-logand (value-rsh value 3) #x1f)
                                                  (lookup-type "int32_t"))
                                      1))
                 (int-val (value-rsh (value-lsh (value-cast
                                                 (value-rsh value 32)
                                                 (lookup-type "int32_t"))
                                                (value-sub 32 int-size))
                                     (value-sub 32 int-size))))
            (format #f "(pvm:int<~a>) ~a" int-size int-val)))
         ((#x1) ;; PVM_VAL_TAG_UINT
          (let* ((uint-size (value-add (value-cast (value-logand (value-rsh value 3) #x1f)
                                                   (lookup-type "int32_t"))
                                       1))
                 (uint-val (value-logand (value-cast
                                          (value-rsh value 32)
                                          (lookup-type "uint32_t"))
                                         (value-cast
                                          (value-lognot
                                           (value-lsh
                                            (value-lsh
                                             (value-lognot
                                              (value-cast (make-value 0)
                                                          (lookup-type "uint64_t")))
                                             (value-sub uint-size 1))  
                                            (make-value 1)))
                                          (lookup-type "uint32_t")))))
            (format #f "(pvm:uint<~a>) ~a" uint-size uint-val)))
         ((#x2) ;; PVM_VAL_TAG_LONG
          (let ((long-size #t)
                (long-val #t))
            (format #f "(pvm:int<~a>) ~a" long-size long-val)))
         ((#x3) ;; PVM_VAL_TAG_ULONG
          (let* ((long-ulong-val (value-subscript
                                  (value-cast
                                   (value-logand (value-cast value (lookup-type "uintptr_t"))
                                                 (value-lognot #x7))
                                   (type-pointer (lookup-type "int64_t")))
                                  0))
                 (ulong-size (make-value 64))
                 (ulong-val (logand
                             long-ulong-val
                             (value-cast
                                          (value-lognot
                                           (value-lsh
                                            (value-lsh
                                             (value-lognot
                                              (value-cast (make-value 0)
                                                          (lookup-type "unsigned long long")))
                                             (value-sub ulong-size 1))  
                                            (make-value 1)))
                                          (lookup-type "uint64_t")))))
            (format #f "(pvm:ulong<~a>) ~a" ulong-size ulong-val)))
         ((#x6) ;; PVM_VAL_TAG_BOX
          (let ((pvm_ptr (value-cast (value-rsh value 3)
                                     (lookup-type "pvm_val_box"))))
            "A BOX"))
         ((#x7) ;; PVM_NULL
          "PVM_NULL")
         (else
          "Unknown PVM_VAL tag"))))
   #f))

(define (poke-lookup-function pretty-printer value)
  (let ((tname (type-name (value-type value))))
    (and tname
         (equal? tname "pvm_val")
         (make-poke-pvm-val-printer value))))

(set-pretty-printers!
 (cons
  (make-pretty-printer "pvm_val" poke-lookup-function)
  (pretty-printers)))
