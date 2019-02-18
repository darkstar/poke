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

;;; RAS_MACRO_OGETMN
;;; ( OFF -- OFF ULONG )
;;;
;;; Auxiliary macro to get the normalized magnitude (i.e. in bits) of a
;;; given offset<uint<64>,*>.

        .macro ogetmn
        ogetm                   ; OFF OGETM
        swap                    ; OGETM OFF
        ogetu                   ; OGETM OFF OGETU
        rot                     ; OFF OGETU OGETM
        mullu
        nip2                    ; OFF (OGETU*OGETM
        .end
        
;;; RAS_MACRO_REMAP
;;; ( VAL -- VAL )
;;;
;;; Given a map-able PVM value on the TOS, remap it.  This is the
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
;;; Given a map-able PVM value on the TOS, invoke its writer.  This is
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
;;; @from_base_type
;;;    pkl_ast_node reflecting the type of the source offset magnitude.
;;; @from_unit_type
;;;    pkl_ast_node reflecting the type of the source offset unit.
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

;;; OGCDUNIT unit_type base_type
;;; ( OFF1 OFF2 -- OFF1 OFF2 UNIT )
;;;
;;; Given two offsets in the stack, of base type BASE_TYPE, calculate
;;; the greatest common divisor of their units and push it in the
;;; stack.
;;;
;;; Macro arguments:
;;; @unit_type
;;;   a pkl_ast_node with a type uint<64>.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro ogcdunit @unit_type @base_type
        dup                     ; OFF1 OFF2 OFF2
        rot                     ; OFF2 OFF2 OFF1
        dup                     ; OFF2 OFF2 OFF1 OFF1
        rot                     ; OFF2 OFF1 OFF1 OFF2
        ogetu                   ;    ...    OFF1 OFF2 OFF2U
        swap                    ;    ...    OFF1 OFF2U OFF2
        ogetm                   ;    ...    OFF1 OFF2U OFF2 OFF2M
        nton @base_type, @unit_type
        nip                     ;    ...    OFF1 OFF2U OFF2 OFF2M
        nip                     ;    ...    OFF1 OFF2U OFF2M
        mul @unit_type
        nip2                    ;    ...    OFF1 (OFF2U*OFF2M)
        swap                    ;    ...    (OFF2U*OFF2M) OFF1
        ogetu                   ;    ...    (OFF2U*OFF2M) OFF1 OFF1U
        swap                    ;    ...    (OFF2U*OFF2M) OFF1U OFF1
        ogetm                   ;    ...    (OFF2U*OFF2M) OFF1U OFF1 OFF1M
        nton @base_type, @unit_type
        nip                     ;    ...    (OFF2U*OFF2M) OFF1U OFF1 OFF1M
        nip                     ;    ...    (OFF2U*OFF2M) OFF1U OFF1M
        mul @unit_type
        nip2                    ;    ...    (OFF2U*OFF2M) (OFF1U*OFF1M)
        gcd @unit_type          ;    ...    OFF2 OFF1 OFF2U OFF1U RESU
        nip2                    ; OFF2 OFF1 RESU
        swap                    ; OFF2 RESU OFF1
        nrot                    ; OFF1 OFF2 RESU
        ;; If the result unit is 0, then turn it into bits.
        bz @unit_type, .one
        ba .end
.one:
        drop
        push ulong<64>1         ; OFF1 OFF2 1UL
.end:
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
        .e ogcdunit @unit_type, @base_type ; OFF1 OFF2 RESU
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
        pushvar $off1           ; RESO OFF1
        pushvar $off2           ; RESO OFF1 OFF2
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
        .e ogcdunit @unit_type, @base_type ; OFF1 OFF2 RESU
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
        pushvar $off1           ; RESO OFF1
        pushvar $off2           ; RESO OFF1 OFF2
        rot                     ; OFF1 OFF2 RESO
        popf 1
        .end

;;; MULO base_type
;;; ( OFF VAL -- OFF VAL OFF )
;;;
;;; Multiply an offset with a magnitude.  The result of the operation
;;; is an offset with base type BASE_TYPE.
;;; 
;;; Macro arguments:
;;; @base_type
;;;   a pkl_ast_node with the base type of the offset.

        .macro mulo @base_type
        dup                     ; OFF VAL VAL
        rot                     ; VAL VAL OFF
        dup                     ; VAL VAL OFF OFF
        rot                     ; VAL OFF OFF VAL
        swap                    ;   ...   VAL OFF
        ogetm                   ;   ...   VAL OFF OFFM
        rot                     ;   ...   OFF OFFM VAL
        mul @base_type
        nip2                    ;   ...   OFF (VAL*OFFM)
        swap                    ;   ...   (VAL*OFFM) OFF
        ogetu
        nip                     ;   ...   (VAL*OFFM) OFFU
        mko                     ; VAL OFF OFFR
        nrot                    ; OFFR VAL OFF
        swap                    ; OFFR OFF VAL
        rot                     ; OFF VAL OFFR
        .end

;;; DIVO unit_type base_type
;;; ( OFF OFF -- OFF OFF VAL )
;;;
;;; Divide an offset by another offset.  The result of the operation is
;;; a magnitude.  The types of both the offsets type and the
;;; magnitude type is BASE_TYPE.
;;;
;;; Macro arguments:
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro divo @base_type
        ;; Normalize the magnitude of the first offset to bits.
        swap                    ; OFF2 OFF1
        push ulong<64>1         ; OFF2 OFF1 1UL
        ogetmc @base_type       ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ;; Normalize the magnitude of the second offset to bits.
        push ulong<64>1         ; OFF1 OFF1M OFF2 1UL
        ogetmc @base_type       ; OFF1 OFF1M OFF2 OFF2M
        rot                     ; OFF1 OFF2 OFF2M OFF1M
        swap                    ; OFF1 OFF2 OFF1M OFF2M

        div @base_type
        nip2                    ; OFF1 OFF2 (OFF1M/OFF2M)
        .end

;;; MODO unit_type base_type
;;; ( OFF OFF UNIT -- OFF OFF UNIT OFF )
;;;
;;; Calculate the modulus of two given offsets. The result of the
;;; operation is an offset having unit UNIT.  The types of both the
;;; offsets type and the magnitude type is BASE_TYPE.
;;;
;;; Macro arguments:
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro modo @base_type
        ;; Calculate the magnitude fo the new offset, which is
        ;; the modulus of both magnitudes, the second argument
        ;; converted to first's units.
        tor			; OFF1 OFF2
        atr                     ; OFF1 OFF2 UNIT
        ogetmc @base_type       ; OFF1 OFF2 OFF2M
        rot                     ; OFF2 OFF2U OFF1
        ogetm                   ; OFF2 OFF2U OFF1 OFF1M
        rot                     ; OFF2 OFF1 OFF1M OFF2M
        mod @base_type
        nip2                    ; OFF2 OFF1 (OFF1M%OFF2M)
        nrot                    ; (OFF1M%OFF2M) OFF2 OFF1
        swap                    ; (OFF1M%OFF2M) OFF1 OFF2
        rot                     ; OFF1 OFF2 (OFF1M%OFF2M)
        atr                     ; OFF1 OFF2 (OFF1M%OFF2M) UNIT
        mko                     ; OFF1 OFF2 OFFRES
        fromr                   ; OFF1 OFF2 OFFRES UNIT
        swap                    ; OFF1 OFF2 UNIT OFFRES
        .end

;;; ATRIM array_type
;;; ( ARR ULONG ULONG -- ARR )
;;;
;;; Push a new array resulting from the trimming of ARR to indexes
;;; [ULONG..ULONG].
;;;
;;; Macro arguments:
;;; @array_type
;;;    a pkl_ast_node with the type of ARR.

        .macro atrim @array_type
        pushf
        regvar $to
        regvar $from
        regvar $array
        ;; Check boundaries
        pushvar $array          ; ARR
        sel                     ; ARR NELEM
        pushvar $to             ; ARR NELEM TO
        lelu                    ; ARR NELEM TO (NELEM<=TO)
        bnzi .ebounds
        drop                    ; ARR NELEM TO
        drop                    ; ARR NELEM
        pushvar $from           ; ARR NELEM FROM
        lelu                    ; ARR NELEM FROM (NELEM<=FROM)
        bnzi .ebounds
        drop                    ; ARR NELEM FROM
        drop                    ; ARR NELEM
        drop                    ; ARR
        pushvar $from           ; ARR FROM
        pushvar $to             ; ARR TO
        gtlu
        nip2                    ; ARR (FROM>TO)
        bnzi .ebounds
        drop                    ; ARR
        ba .bounds_ok
.ebounds:
        push PVM_E_OUT_OF_BOUNDS
        raise
.bounds_ok:
        ;; Boundaries are ok.  Build the trimmed array with a
        ;; subset of the elements of the array.
        typof                   ; ARR ATYP
        tyagett                 ; ARR ATYP ETYP
        nip2                    ; ETYP
        pushvar $from
        regvar $idx
      .while
        pushvar $idx            ; ... IDX
        pushvar $to             ; ... IDX TO
        lelu                    ; ... IDX TO (IDX<=TO)
        nip2                    ; ... (IDX<=TO)
      .loop
        ;; Mount the IDX-FROMth element of the new array.
        pushvar $idx            ; ... IDX
        pushvar $array          ; ... IDX ARR
        swap                    ; ... ARR IDX
        aref                    ; ... ARR IDX EVAL
        rot                     ; ... IDX EVAL ARR
        drop                    ; ... IDX EVAL
        pushvar $from           ; ... IDX EVAL FROM
        rot                     ; ... EVAL FROM IDX
        swap                    ; ... EVAL IDX FROM
        sublu
        nip2                    ; ... EVAL (IDX-FROM)
        swap                    ; ... (IDX-FROM) EVAL
        ;; Increase index and loop.
        pushvar $idx            ; ... IDX
        push ulong<64>1         ; ... IDX 1UL
        addlu
        nip2                    ; (IDX+1UL)
        popvar $idx
      .endloop
        ;; Ok, the elements are in the stack.  Calculate the
        ;; number of initializers and elements and make the
        ;; new array.
        pushvar $to             ; ... TO
        pushvar $from           ; ... TO FROM
        sublu
        nip2                    ; ... (TO-FROM)
        push ulong<64>1         ; ... (TO-FROM) 1
        addlu
        nip2                    ; ... (TO-FROM+1)
        dup                     ; ETYP [IDX VAL...] NELEM NINIT
        mka
        ;; If the trimmed array is mapped then the resulting array
        ;; is mapped as well, with the following attributes:
        ;;
        ;;   OFFSET = original OFFSET + OFF(FROM)
        ;;   EBOUND = TO - FROM + 1
        ;;
        ;; The mapping of the resulting array is always
        ;; bounded by number of elements, regardless of the
        ;; characteristics of the mapping of the trimmed array.
        pushvar $array          ; TARR ARR
        mgeto                   ; TARR ARR OFFSET
        bn .notmapped
        ;; Calculate the new offset.
        swap                    ; TARR OFFSET ARR
        pushvar $from           ; TARR OFFSET ARR FROM
        arefo                   ; TARR OFFSET ARR FROM OFF(FROM)
        nip                     ; TARR OFFSET ARR OFF(FROM)
        rot                     ; TARR ARR OFF(FROM) OFFSET
        .e ogetmn               ; TARR ARR OFF(FROM) OFFSET OFFSETM
        nip
        swap                    ; TARR ARR OFFSETM OFF(FROM)
        .e ogetmn
        nip                     ; TARR ARR OFFSETM OFFM(FROM)
        addlu
        nip2                    ; TARR ARR NOFFSETM
        push ulong<64>1
        mko                     ; TARR ARR OFFSET
        rot                     ; ARR OFFSET TARR
        regvar $tarr
        ;; Calculate the new EBOUND.
        swap                    ; OFFSET ARR
        mgetm                   ; OFFSET ARR MAPPER
        swap                    ; OFFSET MAPPER ARR
        mgetw                   ; OFFSET MAPPER ARR WRITER
        nip                     ; OFFSET MAPPER WRITER
        pushvar $to
        pushvar $from           ; OFFSET MAPPER WRITER TO FROM
        sublu
        nip2                    ; OFFSET MAPPER WRITER (TO-FROM)
        push ulong<64>1
        addlu
        nip2                    ; OFFSET MAPPER WRITER (TO-FROM+1UL)
        ;; Install mapper, writer, offset and ebound.
        pushvar $tarr           ; OFFSET MAPPER WRITER (TO-FROM+!UL) TARR
        swap                    ; OFFSET MAPPER WRITER TARR (TO-FROM+!UL)
        msetsel                 ; OFFSET MAPPER WRITER TARR
        swap                    ; OFFSET MAPPER TARR WRITER
        msetw                   ; OFFSET MAPPER TARR
        swap                    ; OFFSET TARR MAPPER
        msetm                   ; OFFSET TARR
        swap                    ; TARR OFFSET
        mseto                   ; TARR
        ;; Remap!!
        remap
        push null
        push null
.notmapped:
        drop
        drop
        popf 1
        .end
