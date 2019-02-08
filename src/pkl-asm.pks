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

;;; RAS_MACRO_REMAP
;;; ( VAL -- VAL )
;;;
;;; Given a mapeable PVM value on the TOS, remap it.  This is the
;;; implementation of the PKL_INSN_REMAP macro-instruction.

        .macro remap
        ;; The re-map should be done only if the value has a mapper.
        mgetm                   ; VAL MCLS
        bn .label               ; VAL MCLS
        drop                    ; VAL

        ;; XXX do not re-map if the object is up to date (cached
        ;; value.)

        ;; XXX rewrite using variables to avoid extra instructions.

        mgetw                   ; VAL WCLS
        swap                    ; WCLS VAL
        mgetm                   ; WCLS VAL MCLS
        swap                    ; WCLS MCLS VAL

        mgeto                   ; WCLS MCLS VAL OFF
        swap                    ; WCLS MCLS OFF VAL
        mgetsel                 ; WCLS MCLS OFF VAL EBOUND
        swap                    ; WCLS MCLS OFF EBOUND VAL
        mgetsiz                 ; WCLS MCLS OFF EBOUND VAL SBOUND
        swap                    ; WCLS MCLS OFF EBOUND SBOUND VAL
        mgetm                   ; WCLS MCLS OFF EBOUND SBOUND VAL MCLS
        swap                    ; WCLS MCLS OFF EBOUND SBOUND MCLS VAL
        drop                    ; WCLS MCLS OFF EBOUND SBOUND MCLS
        call                    ; WCLS MCLS NVAL
        swap                    ; WCLS NVAL MCLS
        msetm                   ; WCLS NVAL
        swap                    ; NVAL WCLS
        msetw                   ; NVAL

        ;; If the mapped value is null then raise an EOF
        ;; exception.
        dup                     ; NVAL NVAL
        bnn .label
        push PVM_E_EOF
        raise

        push null               ; NVAL null
.label:
        drop                    ; NVAL
        .end

;;; RAS_MACRO_WRITE
;;; ( VAL -- VAL )
;;;
;;; Given a mapeable PVM value on the TOS, invoke its writer.  This is
;;; the implementation of the PKL_INSN_WRITE macro-instruction.

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

;;; RAS_MACRO_OGETMC
;;; ( OFFSET UNIT -- OFFSET CONVERTED_MAGNITUDE )
;;;
;;; Given an offset and an unit in the stack, generate code to push
;;; its magnitude converted to the given unit.  This is the implementation
;;; of the PKL_INSN_OGETMC macro-instruction.
;;;
;;; Macro arguments:
;;;
;;; @unit_type
;;;    a pkl_ast_node with the type of an offset unit, i.e.
;;;    uint<64>.
;;; @base_type
;;;    a pkl_ast_node with the base type of the offset.

        .macro ogetmc @unit_type @base_type
        swap                    ; TOUNIT OFF
        ogetm                   ; TOUNIT OFF MAGNITUDE
        swap                    ; TOUNIT MAGNITUDE OFF
        ogetu                   ; TOUNIT MAGNITUDE OFF UNIT
        nton @unit_type, @base_type ; TOUNIT MAGNITUDE OFF UNIT
        nip                     ; TOUNIT MAGNITUDE OFF UNIT
        rot                     ; TOUNIT OFF UNIT MAGNITUDE
        mul @base_type          ; TOUNIT OFF UNIT MAGNITUDE (UNIT*MAGNITUDE)
        nip2                    
        rot                     ; OFF (UNIT*MAGNITUIDE) TOUNIT
        nton @unit_type, @base_type ; OFF (MAGNITUDE*UNIT) TOUNIT
        nip                     ; OFF (MAGNITUDE*UNIT) TOUNIT
        div @base_type
        nip2                    ; OFF (MAGNITUDE*UNIT/TOUNIT)
        .end

;;; RAS_MACRO_OFFSET_CAST
;;; ( OFF FROMUNIT TOUNIT -- OFF )
;;;
;;; This macro generates code that converts an offset of a given type
;;; into an offset of another given type.  This is the implementation of
;;; the PKL_INSN_OTO macro-instruction.
;;;
;;; Macro arguments:
;;;
;;; @from_base_type
;;;    pkl_ast_node reflecting the type of the source offset magnitude.
;;; @from_unit_type
;;;    pkl_ast_node reflecting the type of the source offset unit.
;;; 
;;; @to_base_type
;;;    pkl_ast_node reflecting the type of the result offset magnitude.
;;; @to_unit_type
;;;    pkl_ast_node reflecting the type of the result offset unit.

        .macro offset_cast @from_base_type @from_unit_type @to_base_type @to_unit_type
        ;; XXX use OGETMC here.
        ;; XXX can't FROMUNIT be derived from OFF? (ogetu)
        ;; XXX we have to do the arithmetic in unit_types, then
        ;; convert to to_base_type, to assure that to_base_type can hold
        ;; the to_base_unit.  Otherwise weird division by zero occurs.
        pushf
        regvar $tounit                          ; OFF FROMUNIT
        regvar $fromunit                        ; OFF
        ;; Get the magnitude of the offset and convert it to the new
        ;; magnitude type.
        ogetm                                   ; OFF OFFM
        nton @from_base_type, @to_base_type     ; OFF OFFM OFFMC
        nip                                     ; OFF OFFMC
        ;; Now do the same for the unit.
        pushvar $fromunit                       ; OFF OFFMC OFFU
        nton @from_unit_type, @to_base_type     ; OFF OFFMC OFFU OFFUC
        nip                                     ; OFF OFFMC OFFUC
        mul @to_base_type                       ; OFF OFFMC OFFUC (OFFMC*OFFUC)
        nip2                                    ; OFF (OFFMC*OFFUC)
        ;; Convert the new unit.
        pushvar $tounit                         ; OFF (OFFMC*OFFUC) TOUNIT
        nton @to_unit_type, @to_base_type       ; OFF (OFFMC*OFFUNC) TOUNIT TOUNITC
        nip                                     ; OFF (OFFMC*OFFUNC) TOUNITC
        div @to_base_type
        nip2                                    ; OFF (OFFMC*OFFUNC/TOUNITC)
        swap                                    ; (OFFMC*OFFUNC/TOUNITC) OFF
        pushvar $tounit                         ; (OFFMC*OFFUNC/TOUNITC) OFF TOUNIT
        nip                                     ; (OFFMC*OFFUNC/TOUNITC) TOUNIT
        mko                                     ; OFFC
        popf 1
        .end

;;; GCD type
;;; ( VAL VAL -- VAL VAL VAL )
;;;
;;; Calculate the greatest common divisor of the integral value
;;; at the TOS, which should be of type TYPE.
;;;
;;; Macro arguments:
;;; 
;;; @type
;;;   type of the value at the TOS.  It should be an integral type.

        .macro gcd @type
        ;; Iterative Euclid's Algorithm.
        over                     ; A B A
        over                     ; A B A B
.loop:
        bz @type, .endloop      ; ... A B
        mod @type               ; ... A B A%B
        rot                     ; ... B A%B A
        drop                    ; ... B A%B
        ba .loop
.endloop:
        drop                    ; A B GCD
        .end
        
;;; ADDO unit_type base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; Add the two given offsets in the stack, which must be of the
;;; given base type.  The unit of the result is the greatest common
;;; divisor of the operands units.  The base type  of the result is the base
;;; type of the operands.
;;;
;;; Macro arguments:
;;;
;;; @unit_type
;;;   a pkl_ast_node with a type uint<64>.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro addo @unit_type @base_type
        pushf
        regvar $off2
        regvar $off1
        ;; Calculate the unit of the result.
        pushvar $off1           ; OFF1
        pushvar $off2           ; OFF2
        ogetu                   ; OFF1 OFF2 OFF2U
        swap                    ; OFF1 OFF2U OFF2
        ogetm                   ; OFF1 OFF2U OFF2 OFF2M
        nton @base_type, @unit_type
        nip                     ; OFF1 OFF2U OFF2 OFF2M
        nip                     ; OFF1 OFF2U OFF2M
        mul @unit_type
        nip2                    ; OFF1 (OFF2U*OFF2M)
        swap                    ; (OFF2U*OFF2M) OFF1
        ogetu                   ; (OFF2U*OFF2M) OFF1 OFF1U
        swap                    ; (OFF2U*OFF2M) OFF1U OFF1
        ogetm                   ; (OFF2U*OFF2M) OFF1U OFF1 OFF1M
        nton @base_type, @unit_type
        nip                     ; (OFF2U*OFF2M) OFF1U OFF1 OFF1M
        nip                     ; (OFF2U*OFF2M) OFF1U OFF1M
        mul @unit_type
        nip2                    ; (OFF2U*OFF2M) (OFF1U*OFF1M)
        gcd @unit_type          ; OFF2 OFF1 OFF2U OFF1U RESU
        nip2                    ; RESU
        ;; Get the magnitude of the first array, in result's units.
        pushvar $off1           ; RESU OFF1
        swap                    ; OFF1 RESU
        dup                     ; OFF1 RESU RESU
        nrot                    ; RESU OFF1 RESU
        ogetmc @base_type       ; RESU OFF1 OFF1M
        nip                     ; RESU OFF1M
        ;; Get the magnitude of the second array, in result's units.
        pushvar $off2           ; RESU OFF1M OFF2
        rot                     ; OFF1M OFF2 RESU
        dup                     ; OFF1M OFF2 RESU RESU
        nrot                    ; OFF1M RESU OFF2 RESU
        ogetmc @base_type       ; OFF1M RESU OFF2 OFF2M
        nip                     ; OFF1M RESU OFF2M
        ;; Add the two magnitudes.
        rot                     ; RESU OFF2M OFF1M
        add @base_type          ; RESU OFF2M OFF1M RESM
        nip2                    ; RESU RESM
        ;; Build the result offset.
        swap                    ; RESM RESU
        mko                     ; RESO
        push $off1              ; RESO OFF1
        push $off2              ; RESO OFF1 OFF2
        rot                     ; OFF1 OFF2 RESO
        popf 1
        .end


;;; SUBO unit_type base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; Add the two given offsets in the stack, which must be of the
;;; given base type.  The unit of the result is the greatest common
;;; divisor of the operands units.  The base type  of the result is the base
;;; type of the operands.
;;;
;;; Macro arguments:
;;;
;;; @unit_type
;;;   a pkl_ast_node with a type uint<64>.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro subo @unit_type @base_type
        pushf
        regvar $off2
        regvar $off1
        ;; Calculate the unit of the result.
        pushvar $off1           ; OFF1
        pushvar $off2           ; OFF2
        ogetu                   ; OFF1 OFF2 OFF2U
        swap                    ; OFF1 OFF2U OFF2
        ogetm                   ; OFF1 OFF2U OFF2 OFF2M
        nton @base_type, @unit_type
        nip                     ; OFF1 OFF2U OFF2 OFF2M
        nip                     ; OFF1 OFF2U OFF2M
        mul @unit_type
        nip2                    ; OFF1 (OFF2U*OFF2M)
        swap                    ; (OFF2U*OFF2M) OFF1
        ogetu                   ; (OFF2U*OFF2M) OFF1 OFF1U
        swap                    ; (OFF2U*OFF2M) OFF1U OFF1
        ogetm                   ; (OFF2U*OFF2M) OFF1U OFF1 OFF1M
        nton @base_type, @unit_type
        nip                     ; (OFF2U*OFF2M) OFF1U OFF1 OFF1M
        nip                     ; (OFF2U*OFF2M) OFF1U OFF1M
        mul @unit_type
        nip2                    ; (OFF2U*OFF2M) (OFF1U*OFF1M)
        gcd @unit_type          ; OFF2 OFF1 OFF2U OFF1U RESU
        nip2                    ; RESU
        ;; Get the magnitude of the first array, in result's units.
        pushvar $off1           ; RESU OFF1
        swap                    ; OFF1 RESU
        dup                     ; OFF1 RESU RESU
        nrot                    ; RESU OFF1 RESU
        ogetmc @base_type       ; RESU OFF1 OFF1M
        nip                     ; RESU OFF1M
        ;; Get the magnitude of the second array, in result's units.
        pushvar $off2           ; RESU OFF1M OFF2
        rot                     ; OFF1M OFF2 RESU
        dup                     ; OFF1M OFF2 RESU RESU
        nrot                    ; OFF1M RESU OFF2 RESU
        ogetmc @base_type       ; OFF1M RESU OFF2 OFF2M
        nip                     ; OFF1M RESU OFF2M
        ;; Subtrace the two magnitudes.
        rot                     ; RESU OFF2M OFF1M
        swap                    ; RESU OFF1M OFF2M
        sub @base_type
        nip2                    ; RESU RESM
        ;; Build the result offset.
        swap                    ; RESM RESU
        mko                     ; RESO
        push $off1              ; RESO OFF1
        push $off2              ; RESO OFF1 OFF2
        rot                     ; OFF1 OFF2 RESO
        popf 1
        .end

;;; MULO base_type
;;; ( OFF VAL -- OFF VAL OFF )
;;;
;;; Multiply an offset with a magnitude.  The types of both the offset
;;; base type and the magnitude type is BASE_TYPE.
;;;
;;; Macro arguments:
;;;
;;; @base_type
;;;   a pkl_ast_node with the base type of the offset.

        .macro mulo @base_type
        dup                     ; OFF VAL VAL
        rot                     ; VAL VAL OFF
        dup                     ; VAL VAL OFF OFF
        rot                     ; VAL OFF OFF VAL
        swap                    ; VAL OFF VAL OFF
        ogetm                   ; VAL OFF VAL OFF OFFM
        rot                     ; VAL OFF OFF OFFM VAL
        mul @base_type
        nip2                    ; VAL OFF OFF (VAL*OFFM)
        swap                    ; VAL OFF (VAL*OFFM) OFF
        ogetu
        nip                     ; VAL OFF (VAL*OFFM) OFFU
        mko                     ; VAL OFF OFFR
        nrot                    ; OFFR VAL OFF
        swap                    ; OFFR OFF VAL
        rot                     ; OFF VAL OFFR
        .end
