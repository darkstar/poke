;;; -*- mode: asm -*-
;;; pkl-asm.pks - Assembly routines for the Pkl macro-assembler
;;;

;;; Copyright (C) 2019 Jose E. Marchesi

;;; This program is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY ; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program.  If not, see <http: //www.gnu.org/licenses/>.


;;; RAS_MACRO_WRITE
;;; ( VAL -- VAL )
;;;
;;; Given a mapeable PVM value on the TOS, invoke its writer.  This is
;;; used to implement the WRITE macro-instruction.

        .macro write

        dup                     ; VAL VAL

        ;; The write should be done only if the value has a writer.
        mgetw                   ; VAL VAL WCLS
        bn .label

        swap                    ; VAL WCLS VAL
        mgeto                   ; VAL WCLS VAL OFF
        swap                    ; VAL WCLS OFF VAL
        rot                     ; VAL OFF VAL WCLS
        ;; XXX exceptions etc.
        call                    ; VAL null

        push null               ; VAL null null
.label:
        drop                    ; VAL (VAL|null)
        drop                    ; VAL

        .end
