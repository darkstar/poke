;;; -*- mode: poke-ras -*-
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

;;; RAS_MACRO_OFFSET_CAST
;;; ( OFF TOUNIT -- OFF )
;;;
;;; This macro generates code that converts an offset of a given type
;;; into an offset of another given type.  This is the implementation of
;;; the PKL_INSN_OTO macro-instruction.
;;;
;;; Macro arguments:
;;; @from_base_type
;;;    pkl_ast_node reflecting the type of the source offset magnitude.
;;; @to_base_type
;;;    pkl_ast_node reflecting the type of the result offset magnitude.
;;; @unit_type
;;;    pkl_ast_node reflecting the type of an unit, i.e. uint<64>.

        .macro offset_cast @from_base_type @to_base_type @unit_type
        ;; Note that we have to do the arithmetic in unit_types, then
        ;; convert to to_base_type, to assure that to_base_type can hold
        ;; the to_base_unit.  Otherwise weird division by zero occurs.
        pushf
        regvar $tounit                          ; OFF
        ogetu                                   ; OFF FROMUNIT
        regvar $fromunit                        ; OFF
        ;; Get the magnitude of the offset and convert it to the new
        ;; magnitude type.
        ogetm                                   ; OFF OFFM
        nton @from_base_type, @to_base_type     ; OFF OFFM OFFMC
        nip                                     ; OFF OFFMC
        ;; Now do the same for the unit.
        pushvar $fromunit                       ; OFF OFFMC OFFU
        nton @unit_type, @to_base_type          ; OFF OFFMC OFFU OFFUC
        nip                                     ; OFF OFFMC OFFUC
        mul @to_base_type                       ; OFF OFFMC OFFUC (OFFMC*OFFUC)
        nip2                                    ; OFF (OFFMC*OFFUC)
        ;; Convert the new unit.
        pushvar $tounit                         ; OFF (OFFMC*OFFUC) TOUNIT
        nton @unit_type, @to_base_type          ; OFF (OFFMC*OFFUNC) TOUNIT TOUNITC
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

;;; ADDO unit_type base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; Add the two given offsets in the stack, which must be of the
;;; same base type, and same units.
;;;
;;; Macro arguments:
;;;
;;; #unit
;;;   an ulong<64> with the unit of the result.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro addo @base_type #unit
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        rot                     ; OFF1 OFF2 OFF2M OFF1M
        swap                    ; OFF1 OFF2 OFF1M OFF2M
        add @base_type
        nip2                    ; OFF1 OFF2 (OFF1M+OFF2M)
        push #unit              ; OFF1 OFF2 (OFF1M+OFF2M) UNIT
        mko                     ; OFF1 OFF2 OFFR
        .end

;;; SUBO unit_type base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; Subtract the two given offsets in the stack, which must be of the
;;; same base type and same units.
;;;
;;; Macro arguments:
;;;
;;; #unit
;;;   an ulong<64> with the unit of the result.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro subo @base_type #unit
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        rot                     ; OFF1 OFF2 OFF2M OFF1M
        swap                    ; OFF1 OFF2 OFF1M OFF2M
        sub @base_type
        nip2                    ; OFF1 OFF2 (OFF1M+OFF2M)
        push #unit              ; OFF1 OFF2 (OFF1M+OFF2M) UNIT
        mko                     ; OFF1 OFF2 OFFR
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
        dup                     ; VAL VAL
        tor                     ; VAL
        swap                    ; VAL OFF
        ogetm                   ; VAL OFF OFFM
        rot                     ; OFF OFFM VAL
        mul @base_type
        nip2                    ; OFF (OFFM*VAL)
        swap                    ; (OFFM*VAL) OFF
        ogetu                   ; (OFFM*VAL) OFF UNIT
        rot                     ; OFF UNIT (OFFM*VAL)
        swap                    ; OFF (OFFM*VAL) UNIT
        mko                     ; OFF OFFR
        fromr                   ; OFF OFFR VAL
        swap                    ; OFF VAL OFFR
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
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        rot                     ; OFF1 OFF2 OFF2M OFF1M
        swap                    ; OFF1 OFF2 OFF1M OFF2M
        div @base_type
        nip2                    ; OFF1 OFF2 (OFF1M/OFF2M)
        .end

;;; MODO unit_type base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; Calculate the modulus of two given offsets. The result of the
;;; operation is an offset having unit UNIT.  The types of both the
;;; offsets type and the magnitude type is BASE_TYPE.
;;;
;;; Macro arguments:
;;;
;;; #unit
;;;   an ulong<64> with the unit of the result.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro modo @base_type #unit
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        rot                     ; OFF1 OFF2 OFF2M OFF1M
        swap                    ; OFF1 OFF2 OFF1M OFF2M
        mod @base_type
        nip2                    ; OFF1 OFF2 (OFF1M%OFF2M)
        push #unit              ; OFF1 OFF2 (OFF1M%OFF2M) UNIT
        mko                     ; OFF1 OFF2 OFFR
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
        push null               ; NULL ETYP
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
        push null               ; ... NULL IDX
        pushvar $idx            ; ... NULL IDX
        pushvar $array          ; ... NULL IDX ARR
        swap                    ; ... NULL ARR IDX
        aref                    ; ... NULL ARR IDX EVAL
        rot                     ; ... NULL IDX EVAL ARR
        drop                    ; ... NULL IDX EVAL
        pushvar $from           ; ... NULL IDX EVAL FROM
        rot                     ; ... NULL EVAL FROM IDX
        swap                    ; ... NULL EVAL IDX FROM
        sublu
        nip2                    ; ... NULL EVAL (IDX-FROM)
        swap                    ; ... NULL (IDX-FROM) EVAL
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
        dup                     ; NULL ETYP [NULL IDX VAL...] NELEM NINIT
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

;;; RAS_MACRO_ARRAY_CONV_SEL
;;; ( ARR -- ARR )
;;;
;;; This macro generates code that checks that ARR has the right number
;;; of elements as specified by an array type bounder.  If the check fails
;;; then PVM_E_CONV is raised.  If the check is ok, then it updates ARR's
;;; type boundary.
;;;
;;; Macro arguments:
;;; @bounder
;;;    a bounder closure.

        .macro array_conv_sel #bounder
        sel                     ; ARR SEL
        push #bounder           ; ARR SEL CLS
        call                    ; ARR SEL BOUND
        eqlu                    ; ARR SEL BOUND (SEL==BOUND)
        bnzi .bound_ok
        push PVM_E_CONV
        raise
.bound_ok:
        drop                    ; ARR SEL BOUND
        nip                     ; ARR BOUND
        asettb                  ; ARR
        .end

;;; RAS_MACRO_ARRAY_CONV_SIZ
;;; ( ARR -- ARR )
;;;
;;; This macro generates code that checks that ARR has the right size
;;; as specified by an array type bounder.  If the check fails then
;;; PVM_E_CONV is raised.  If the check is ok, then it updates ARR's
;;; type boundary.
;;;
;;; Macro arguments:
;;; @bounder
;;;    a bounder closure.

        .macro array_conv_siz #bounder
        siz                     ; ARR SIZ
        ogetm                   ; ARR SIZ SIZM
        nip                     ; ARR SIZM
        push #bounder           ; ARR SIZM CLS
        call                    ; ARR SIZM BOUND
        .e ogetmn               ; ARR SIZM BOUND BOUNDM
        rot                     ; ARR BOUND BOUNDM SIZM
        eqlu                    ; ARR BOUND BOUNDM SIZM (BOUNDM==SIZM)
        nip2                    ; ARR BOUND (BOUNDM==SIZM)
        bnzi .bound_ok
        push PVM_E_CONV
        raise
.bound_ok:
        drop                    ; ARR BOUND
        asettb                  ; ARR
        .end

;;; RAS_MACRO_CDIV
;;; ( VAL VAL -- VAL VAL VAL )
;;;
;;; This macro generates code that performs ceil-division on integral
;;; values.
;;;
;;; Macro arguments:
;;; #one
;;; @type
;;;    pkl_ast_node reflecting the type of the operands.

        .macro cdiv #one @type
        dup
        nrot
        push #one
        sub @type
        nip2
        add @type
        nip2
        swap
        div @type
        .end

;;; RAS_MACRO_CDIVO one base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; This macro generates code that performs ceil-division on integral
;;; values.
;;;
;;; Macro arguments:
;;; #one
;;; @type
;;;    pkl_ast_node reflecting the type of the operands.

        .macro cdivo @type
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        rot                     ; OFF1 OFF2 OFF2M OFF1M
        swap                    ; OFF1 OFF2 OFF1M OFF2M
        cdiv @type
        nip2                    ; OFF1 OFF2 (OFF1M/^OFF2M)
        .end

;;; RAS_MACRO_AIS etype
;;; ( VAL ARR -- VAL ARR BOOL )
;;;
;;; This macro generates code that, given an array ARR and a value VAL,
;;; determines whether VAL exists in ARR.  If it does, it pushes int<32>1
;;; to the stack.  Otherwise it pushes int<32>0.

        .macro ais @etype
        sel                     ; VAL ARR SEL
        swap                    ; VAL SEL ARR
        tor                     ; VAL SEL [ARR]
        push ulong<64>0         ; VAL SEL IDX [ARR]
        push int<32>0           ; VAL SEL IDX RES [ARR]
        tor                     ; VAL SEL IDX [ARR RES]
.loop:
        gtlu                    ; VAL SEL IDX (SEL>IDX) [ARR RES]
        bzi .endloop
        drop                    ; VAL SEL IDX [ARR RES]
        fromr                   ; VAL SEL IDX RES [ARR]
        fromr                   ; VAL SEL IDX RES ARR
        rot                     ; VAL SEL RES ARR IDX
        aref                    ; VAL SEL RES ARR IDX ELEM
        rot                     ; VAL SEL RES IDX ELEM ARR
        tor                     ; VAL SEL RES IDX ELEM [ARR]
        rot                     ; VAL SEL IDX ELEM RES [ARR]
        tor                     ; VAL SEL IDX ELEM [ARR RES]
        swap                    ; VAL SEL ELEM IDX [ARR RES]
        tor                     ; VAL SEL ELEM [ARR RES IDX]
        rot                     ; SEL ELEM VAL [ARR RES IDX]
        eq @etype               ; SEL ELEM VAL (ELEM==VAL) [ARR RES IDX]
        fromr                   ; SEL ELEM VAL (ELEM==VAL) IDX [ARR RES]
        fromr                   ; SEL ELEM VAL (ELEM==VAL) IDX RES [ARR]
        rot                     ; SEL ELEM VAL IDX RES (ELEM==VAL) [ARR]
        or                      ; SEL ELEM VAL IDX RES (ELEM==VAL) NRES [ARR]
        nip2                    ; SEL ELEM VAL IDX NRES [ARR]
        bnzi .foundit           ; SEL ELEM VAL IDX NRES [ARR]
        tor                     ; SEL ELEM VAL IDX [ARR NRES]
        push ulong<64>1
        addlu                   ; SEL ELEM VAL IDX 1UL (IDX+1UL) [ARR NRES]
        nip2                    ; SEL ELEM VAL NIDX [ARR NRES]
        rot                     ; SEL VAL NIDX ELEM [ARR NRES]
        drop                    ; SEL VAL NIDX [ARR NREGS]
        nrot                    ; NIDX SEL VAL [ARR NREGS]
        swap                    ; NIDX VAL SEL [ARR NRES]
        rot                     ; VAL SEL NIDX [ARR NRES]
        ba .loop
.foundit:
        tor                     ; SEL ELEM VAL IDX [ARR NRES]
        rot                     ; SEL VAL IDX ELEM [ARR NRES]
        drop                    ; SEL VAL IDX [ARR NRES]
        tor                     ; SEL VAL [ARR NRES IDX]
        swap                    ; VAL SEL [ARR NRES IDX]
        fromr                   ; VAL SEL IDX [ARR NRES]
        dup                     ; VAL SEL IDX IDX [ARR NRES]
.endloop:
        drop                    ; VAL SEL IDX [ARR RES]
        drop                    ; VAL SEL [ARR RES]
        drop                    ; VAL [ARR RES]
        fromr                   ; VAL RES [ARR]
        fromr                   ; VAL RES ARR
        swap                    ; VAL ARR RES
        .end
