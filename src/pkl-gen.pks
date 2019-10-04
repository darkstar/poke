;;; -*- mode: asm -*-
;;; pkl-gen.pks - Assembly routines for the codegen
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

;;; RAS_MACRO_OP_UNMAP
;;; ( VAL -- VAL )
;;;
;;; Turn the value on the stack into a non-mapped value, if the value
;;; is mapped.  If the value is not mapped, this is a NOP.

        .macro op_unmap
        push null
        mseto
        push null
        msetm
        push null
        msetw
        .end

;;; RAS_FUNCTION_ARRAY_MAPPER
;;; ( OFF EBOUND SBOUND -- ARR )
;;;
;;; Assemble a function that maps an array value at the given offset
;;; OFF, with mapping attributes EBOUND and SBOUND.
;;;
;;; If both EBOUND and SBOUND are null, then perform an unbounded map,
;;; i.e. read array elements from IO until EOF.  XXX: what about empty
;;; arrays?
;;;
;;; Otherwise, if EBOUND is not null, then perform a map bounded by the
;;; given number of elements.  If EOF is encountered before the given
;;; amount of elements are read, then raise PVM_E_MAP_BOUNDS.
;;;
;;; Otherwise, if SBOUND is not null, then perform a map bounded by the
;;; given size (an offset), i.e. read array elements from IO until the
;;; total size of the array is exactly SBOUND.  If SBOUND is exceeded,
;;; then raise PVM_E_MAP_BOUNDS.
;;;
;;; Only one of EBOUND or SBOUND simultanously are supported.
;;; Note that OFF should be of type offset<uint<64>,*>.
;;;
;;; The C environment required is:
;;;
;;; `array_type' is a pkl_ast_node with the array type being mapped.

        .function array_mapper
        prolog
        pushf
        regvar $sbound           ; Argument
        regvar $ebound           ; Argument
        regvar $off              ; Argument
        ;; Determine the offset of the array, in bits, and put it in a
        ;; local.
        pushvar $off            ; OFF
        ogetm		        ; OFF OMAG
        swap                    ; OMAG OFF
        ogetu                   ; OMAG OFF OUNIT
        rot                     ; OFF OUNIT OMAG
        mullu                   ; OFF OUNIT OMAG (OUNIT*OMAG)
        nip2                    ; OFF (OUNIT*OMAG)
        regvar $eomag           ; OFF
        ;; Initialize the element index to 0UL, and put it
        ;; in a local.
        push ulong<64>0         ; OFF 0UL
        regvar $eidx            ; OFF
        ;; Save the offset in bits of the beginning of the array in a
        ;; local.
        pushvar $eomag          ; OFF EOMAG
        regvar $aomag           ; OFF
        ;; If it is not null, transform the SBOUND from an offset to a
        ;; magnitude in bits.
        pushvar $sbound         ; OFF SBOUND
        bn .after_sbound_conv
        ogetm                   ; OFF SBOUND SBOUNDM
        swap                    ; OFF SBOUNDM SBOUND
        ogetu                   ; OFF SBOUNDM SBOUND SBOUNDU
        swap                    ; OFF SBOUNDM SBOUNDU SBOUND
        drop                    ; OFF SOBUNDM SBOUNDU
        mullu                   ; OFF SBOUNDM SBOUNDU (SBOUNDM*SBOUNDU)
        nip2                    ; OFF (SBOUNDM*SBOUNDU)
        regvar $sboundm         ; OFF
        push null               ; OFF null
.after_sbound_conv:
        drop                    ; OFF
        ;; Build the type of the new mapped array.  Note that we use
        ;; the bounds passed to the mapper instead of just subpassing
        ;; in array_type.  This is because this mapper should work for
        ;; both bounded and unbounded array types.  Also, this avoids
        ;; evaluating the boundary expression in the array type
        ;; twice.
        .c PKL_GEN_PAYLOAD->in_mapper = 0;
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
        .c PKL_GEN_PAYLOAD->in_mapper = 1;
                                ; OFF ETYPE
        pushvar $ebound         ; OFF ETYPE EBOUND
        bnn .atype_bound_done
        drop                    ; OFF ETYPE
        pushvar $sbound         ; OFF ETYPE (SBOUND|NULL)
.atype_bound_done:
        mktya                   ; OFF ATYPE
        .while
        ;; If there is an EBOUND, check it.
        ;; Else, if there is a SBOUND, check it.
        ;; Else, iterate (unbounded).
        pushvar $ebound     	; OFF ATYPE NELEM
        bn .loop_on_sbound
        pushvar $eidx		; OFF ATYPE NELEM I
        gtlu                    ; OFF ATYPE NELEM I (NELEM>I)
        nip2                    ; OFF ATYPE (NELEM>I)
        ba .end_loop_on
.loop_on_sbound:
        drop                    ; OFF ATYPE
        pushvar $sboundm        ; OFF ATYPE SBOUNDM
        bn .loop_unbounded
        pushvar $aomag          ; OFF ATYPE SBOUNDM AOMAG
        addlu                   ; OFF ATYPE SBOUNDM AOMAG (SBOUNDM+AOMAG)
        nip2                    ; OFF ATYPE (SBOUNDM+AOMAG)
        pushvar $eomag          ; OFF ATYPE (SBOUNDM+AOMAG) EOMAG
        gtlu                    ; OFF ATYPE (SBOUNDM+AOMAG) EOMAG ((SBOUNDM+AOMAG)>EOMAG)
        nip2                    ; OFF ATYPE ((SBOUNDM+AOMAG)>EOMAG)
        ba .end_loop_on
.loop_unbounded:
        drop                    ; OFF ATYPE
        push int<32>1           ; OFF ATYPE 1
.end_loop_on:
        .loop
                                ; OFF ATYPE
        ;; Mount the Ith element triplet: [EOFF EIDX EVAL]
        pushvar $eomag          ; ... EOMAG
        push ulong<64>1         ; ... EOMAG EOUNIT
        mko                     ; ... EOFF
        dup                     ; ... EOFF EOFF
        push PVM_E_CONSTRAINT
        pushe .constraint_error
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
        pope
        bn .eof
        ;; Update the current offset with the size of the value just
        ;; peeked.
        siz                     ; ... EOFF EVAL ESIZ
        rot                     ; ... EVAL ESIZ EOFF
        ogetm                   ; ... EVAL ESIZ EOFF EOMAG
        rot                     ; ... EVAL EOFF EOMAG ESIZ
        ogetm                   ; ... EVAL EOFF EOMAG ESIZ ESIGMAG
        rot                     ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
        addlu                   ; ... EVAL EOFF ESIZ ESIGMAG EOMAG (ESIGMAG+EOMAG)
        popvar $eomag           ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
        drop                    ; ... EVAL EOFF ESIZ ESIGMAG
        drop                    ; ... EVAL EOFF ESIZ
        drop                    ; ... EVAL EOFF
        pushvar $eidx           ; ... EVAL EOFF EIDX
        rot                     ; ... EOFF EIDX EVAL
        ;; Increase the current index and process the next element.
        pushvar $eidx           ; ... EOFF EIDX EVAL EIDX
        push ulong<64>1         ; ... EOFF EIDX EVAL EIDX 1UL
        addlu                   ; ... EOFF EIDX EVAL EDIX 1UL (EIDX+1UL)
        nip2                    ; ... EOFF EIDX EVAL (EIDX+1UL)
        popvar $eidx            ; ... EOFF EIDX EVAL
        .endloop
        push null
        ba .mountarray
.constraint_error:
        ;; Remove the partial element from the stack.
                                ; ... EOFF EOFF EXCEPTION
        drop
        drop
        drop
        ;; If the array is bounded, raise E_CONSTRAINT
        pushvar $ebound         ; ... EBOUND
        nn                      ; ... EBOUND (EBOUND!=NULL)
        nip                     ; ... (EBOUND!=NULL)
        pushvar $sbound         ; ... (EBOUND!=NULL) SBOUND
        nn                      ; ... (EBOUND!=NULL) SBOUND (SBOUND!=NULL)
        nip                     ; ... (EBOUND!=NULL) (SBOUND!=NULL)
        or                      ; ... (EBOUND!=NULL) (SBOUND!=NULL) ARRAYBOUNDED
        nip2                    ; ... ARRAYBOUNDED
        bzi .mountarray
        push PVM_E_CONSTRAINT
        raise
.eof:
        ;; Remove the partial EOFF null element from the stack.
                                ; ... EOFF null
        drop                    ; ... EOFF
        drop                    ; ...
        ;; If the array is bounded, raise E_EOF
        pushvar $ebound         ; ... EBOUND
        nn                      ; ... EBOUND (EBOUND!=NULL)
        nip                     ; ... (EBOUND!=NULL)
        pushvar $sbound         ; ... (EBOUND!=NULL) SBOUND
        nn                      ; ... (EBOUND!=NULL) SBOUND (SBOUND!=NULL)
        nip                     ; ... (EBOUND!=NULL) (SBOUND!=NULL)
        or                      ; ... (EBOUND!=NULL) (SBOUND!=NULL) ARRAYBOUNDED
        nip2                    ; ... ARRAYBOUNDED
        bzi .mountarray
        push PVM_E_EOF
        raise
.mountarray:
        drop                   ; OFF ATYPE [EOFF EIDX EVAL]...
        pushvar $eidx          ; OFF ATYPE [EOFF EIDX EVAL]... NELEM
        dup                    ; OFF ATYPE [EOFF EIDX EVAL]... NELEM NINITIALIZER
        mka                    ; ARRAY
        ;; Check that the resulting array satisfies the mapping's
        ;; bounds (number of elements and total size.)
        pushvar $ebound        ; ARRAY EBOUND
        bnn .check_ebound
        drop                   ; ARRAY
        pushvar $sboundm       ; ARRAY SBOUNDM
        bnn .check_sbound
        drop
        ba .bounds_ok
.check_ebound:
        swap                   ; EBOUND ARRAY
        sel                    ; EBOUND ARRAY NELEM
        rot                    ; ARRAY NELEM EBOUND
        sublu                  ; ARRAY NELEM EBOUND (NELEM-EBOUND)
        bnzlu .bounds_fail
        drop                   ; ARRAY NELEM EBOUND
        drop                   ; ARRAY NELEM
        drop                   ; ARRAY
        ba .bounds_ok
.check_sbound:
        swap                   ; SBOUNDM ARRAY
        siz                    ; SBOUNDM ARRAY OFF
        ogetm                  ; SBOUNDM ARRAY OFF OFFM
        swap                   ; SBOUNDM ARRAY OFFM OFF
        ogetu                  ; SBOUNDM ARRAY OFFM OFF OFFU
        nip                    ; SBOUNDM ARRAY OFFM OFFU
        mullu                  ; SBOUNDM ARRAY OFFM OFFU (OFFM*OFFU)
        nip2                   ; SBOUNDM ARRAY (OFFM*OFFU)
        rot                    ; ARRAY (OFFM*OFFU) SBOUNDM
        sublu                  ; ARRAY (OFFM*OFFU) SBOUNDM ((OFFM*OFFU)-SBOUND)
        bnzlu .bounds_fail
        drop                   ; ARRAY (OFFU*OFFM) SBOUNDM
        drop                   ; ARRAY (OFFU*OFFM)
        drop                   ; ARRAY
.bounds_ok:
        ;; Set the map bound attributes in the new object.
        pushvar $sbound       ; ARRAY SBOUND
        msetsiz               ; ARRAY
        pushvar $ebound       ; ARRAY EBOUND
        msetsel               ; ARRAY
        popf 1
        return
.bounds_fail:
        push PVM_E_MAP_BOUNDS
        raise
        .end

;;; RAS_FUNCTION_ARRAY_VALMAPPER
;;; ( VAL NVAL OFF -- ARR )
;;;
;;; Assemble a function that "valmaps" a given NVAL at the given offset
;;; OFF, using the data of NVAL, and the mapping attributes of VAL.
;;;
;;; This function can raise PVM_E_MAP_BOUNDS if the characteristics of
;;; NVAL violate the bounds of the map.
;;;
;;; Note that OFF should be of type offset<uint<64>,*>.

        .function array_valmapper
        prolog
        pushf
        regvar $off             ; Argument
        regvar $nval            ; Argument
        regvar $val             ; Argument

        ;; Determine VAL's bounds and set them in locals to be used
        ;; later.
        pushvar $val            ; VAL
        mgetsel                 ; VAL EBOUND
        regvar $ebound          ; VAL
        mgetsiz                 ; VAL SBOUND
        regvar $sbound          ; VAL
        drop                    ; _

        ;; Determine the offset of the array, in bits, and put it in a
        ;; local.
        pushvar $off            ; OFF
        ogetm                   ; OFF OMAG
        swap                    ; OMAG OFF
        ogetu                   ; OMAG OFF OUNIT
        rot                     ; OFF OUNIT OMAG
        mullu                   ; OFF OUNIT OMAG (OUNIT*OMAG)
        nip2                    ; OFF (OUNIT*OMAG)
        regvar $eomag           ; OFF

        ;; Initialize the element index to 0UL, and put it
        ;; in a local.
        push ulong<64>0         ; OFF 0UL
        regvar $eidx	        ; OFF

        ;; Get the number of elements in NVAL, and put it in a local.
        pushvar $nval           ; OFF NVAL
        sel                     ; OFF NVAL NELEM
        nip                     ; OFF NELEM
        regvar $nelem           ; OFF

        ;; If it is not null, transform the SBOUND from an offset to
        ;; a magnitude in bits.
        pushvar $sbound         ; OFF SBOUND
        bn .after_sbound_conv
        ogetm                   ; OFF SBOUND SBOUNDM
        swap                    ; OFF SBOUNDM SBOUND
        ogetu                   ; OFF SBOUNDM SBOUND SBOUNDU
        swap                    ; OFF SBOUNDM SBOUNDU SBOUND
        drop                    ; OFF SOBUNDM SBOUNDU
        mullu                   ; OFF SBOUNDM SBOUNDU (SBOUNDM*SBOUNDU)
        nip2                    ; OFF (SBOUNDM*SBOUNDU)
        regvar $sboundm         ; OFF
        push null               ; OFF null
.after_sbound_conv:
        drop                    ; OFF

        ;; Check that NVAL satisfies EBOUND if this bound is specified
        ;; i.e. the number of elements stored in the array matches the
        ;; bound.
        pushvar $ebound         ; OFF EBOUND
        bnn .check_ebound
        drop                    ; OFF
        ba .ebound_ok

.check_ebound:
        pushvar $nelem          ; OFF EBOUND NELEM
        sublu                   ; OFF EBOUND NELEM (EBOUND-NELEM)
        bnzlu .bounds_fail
        drop                    ; OFF EBOUND NELEM
        drop                    ; OFF EBOUND
        drop                    ; OFF

.ebound_ok:
        ;; Build the type of the new mapped array.  Note that we use
        ;; the bounds extracted above instead of just subpassing in
        ;; array_type.  This is because this function should work for
        ;; both bounded and unbounded array types.  Also, this avoids
        ;; evaluating the boundary expression in the array type
        ;; twice.
        .c PKL_GEN_PAYLOAD->in_valmapper = 0;
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
        .c PKL_GEN_PAYLOAD->in_valmapper = 1;
                                ; OFF ETYPE
        pushvar $ebound         ; OFF ETYPE EBOUND
        bnn .atype_bound_done
        drop                    ; OFF ETYPE
        pushvar $sbound         ; OFF ETYPE (SBOUND|NULL)
.atype_bound_done:
        mktya                   ; OFF ATYPE
        .while
        pushvar $eidx           ; OFF ATYPE I
        pushvar $nelem          ; OFF ATYPE I NELEM
        ltlu                    ; OFF ATYPE I NELEM (NELEM<I)
        nip2                    ; OFF ATYPE (NELEM<I)
        .loop
                                ; OFF ATYPE

        ;; Mount the Ith element triplet: [EOFF EIDX EVAL]
        pushvar $eomag          ; ... EOMAG
        push ulong<64>1         ; ... EOMAG EOUNIT
        mko                     ; ... EOFF
        dup                     ; ... EOFF EOFF

        pushvar $nval           ; ... EOFF EOFF NVAL
        pushvar $eidx           ; ... EOFF EOFF NVAL IDX
        aref                    ; ... EOFF EOFF NVAL IDX ENVAL
        nip2                    ; ... EOFF EOFF ENVAL
        swap                    ; ... EOFF ENVAL EOFF
        pushvar $val            ; ... EOFF ENVAL EOFF VAL
        pushvar $eidx           ; ... EOFF ENVAL EOFF VAL EIDX
        aref                    ; ... EOFF ENVAL EOFF VAL EIDX OVAL
        nip2                    ; ... EOFF ENVAL EOFF OVAL
        nrot                    ; ... EOFF OVAL ENVAL EOFF
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
                                ; ... EOFF EVAL
        ;; Update the current offset with the size of the value just
        ;; peeked.
        siz                     ; ... EOFF EVAL ESIZ
        rot                     ; ... EVAL ESIZ EOFF
        ogetm                   ; ... EVAL ESIZ EOFF EOMAG
        rot                     ; ... EVAL EOFF EOMAG ESIZ
        ogetm                   ; ... EVAL EOFF EOMAG ESIZ ESIGMAG
        rot                     ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
        addlu                   ; ... EVAL EOFF ESIZ ESIGMAG EOMAG (ESIGMAG+EOMAG)
        popvar $eomag           ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
        drop                    ; ... EVAL EOFF ESIZ ESIGMAG
        drop                    ; ... EVAL EOFF ESIZ
        drop                    ; ... EVAL EOFF
        pushvar $eidx           ; ... EVAL EOFF EIDX
        rot                     ; ... EOFF EIDX EVAL

        ;; Increase the current index and process the next element.
        pushvar $eidx           ; ... EOFF EIDX EVAL EIDX
        push ulong<64>1         ; ... EOFF EIDX EVAL EIDX 1UL
        addlu                   ; ... EOFF EIDX EVAL EDIX 1UL (EIDX+1UL)
        nip2                    ; ... EOFF EIDX EVAL (EIDX+1UL)
        popvar $eidx            ; ... EOFF EIDX EVAL
        .endloop

        pushvar $eidx           ; OFF ATYPE [EOFF EIDX EVAL]... NELEM
        dup                     ; OFF ATYPE [EOFF EIDX EVAL]... NELEM NINITIALIZER
        mka                     ; ARRAY

        ;; Check that the resulting array satisfies the mapping's
        ;; total size bound.
        pushvar $sboundm        ; ARRAY SBOUNDM
        bnn .check_sbound
        drop
        ba .sbound_ok

.check_sbound:
        swap                    ; SBOUNDM ARRAY
        siz                     ; SBOUNDM ARRAY OFF
        ogetm                   ; SBOUNDM ARRAY OFF OFFM
        swap                    ; SBOUNDM ARRAY OFFM OFF
        ogetu                   ; SBOUNDM ARRAY OFFM OFF OFFU
        nip                     ; SBOUNDM ARRAY OFFM OFFU
        mullu                   ; SBOUNDM ARRAY OFFM OFFU (OFFM*OFFU)
        nip2                    ; SBOUNDM ARRAY (OFFM*OFFU)
        rot                     ; ARRAY (OFFM*OFFU) SBOUNDM
        sublu                   ; ARRAY (OFFM*OFFU) SBOUNDM ((OFFM*OFFU)-SBOUND)
        bnzlu .bounds_fail
        drop                    ; ARRAY (OFFU*OFFM) SBOUNDM
        drop                    ; ARRAY (OFFU*OFFM)
        drop                    ; ARRAY

.sbound_ok:
        ;; Set the map bound attributes in the new object.
        pushvar $sbound         ; ARRAY SBOUND
        msetsiz                 ; ARRAY
        pushvar $ebound         ; ARRAY EBOUND
        msetsel                 ; ARRAY

        popf 1
        return

.bounds_fail:
        push PVM_E_MAP_BOUNDS
        raise
        .end

;;; RAS_FUNCTION_ARRAY_WRITER
;;; ( OFF VAL -- )
;;;
;;; Assemble a function that pokes a mapped array value to it's mapped
;;; offset in the current IOS.
;;;
;;; Note that it is important for the elements of the array to be poked
;;; in order.

        .function array_writer
        prolog
        pushf
        regvar $value           ; Argument
        drop                    ; The offset is not used.
        push ulong<64>0         ; 0UL
        regvar $idx             ; _
     .while
        pushvar $idx            ; I
        pushvar $value          ; I ARRAY
        sel                     ; I ARRAY NELEM
        nip                     ; I NELEM
        ltlu                    ; I NELEM (NELEM<I)
        nip2                    ; (NELEM<I)
     .loop
                                ; _
        ;; Poke this array element
        pushvar $value          ; ARRAY
        pushvar $idx            ; ARRAY I
        aref                    ; ARRAY I VAL
        nrot                    ; VAL ARRAY I
        arefo                   ; VAL ARRAY I EOFF
        nip2                    ; VAL EOFF
        swap                    ; EOFF VAL
        .c PKL_GEN_PAYLOAD->in_writer = 1;
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
        .c PKL_GEN_PAYLOAD->in_writer = 0;
                                ; _
        ;; Increase the current index and process the next
        ;; element.
        pushvar $idx            ; EIDX
        push ulong<64>1         ; EIDX 1UL
        addlu                   ; EDIX 1UL (EIDX+1UL)
        nip2                    ; (EIDX+1UL)
        popvar $idx             ; _
     .endloop
        popf 1
        push null
        return
        .end                    ; array_writer

;;; RAS_FUNCTION_ARRAY_BOUNDER
;;; ( ARR -- BOUND )
;;;
;;; Assemble a function that returns the boundary of a given array.
;;; If the array is not bounded by either number of elements nor size
;;; then PVM_NULL is returned.
;;;
;;; Note how this function doesn't introduce any lexical level.  This
;;; is important, so keep it this way!
;;;
;;; The C environment required is:
;;;
;;; `array_type' is a pkl_ast_node with the type of ARR.

        .function array_bounder
        prolog
        .c if (PKL_AST_TYPE_A_BOUND (array_type))
        .c {
        .c   PKL_GEN_PAYLOAD->in_array_bounder = 0;
        .c   PKL_PASS_SUBPASS (PKL_AST_TYPE_A_BOUND (array_type)) ;
        .c   PKL_GEN_PAYLOAD->in_array_bounder = 1;
        .c }
        .c else
             push null
        return
        .end

;;; RAS_MACRO_OFF_PLUS_SIZEOF
;;; ( VAL OFF -- VAL OFF NOFF )
;;;
;;; Given a value and an offset in the stack, provide an offset whose
;;; value is OFF + sizeof(VAL).
;;;
;;; XXX this can be greatly simplified if it can be assumed OFF
;;; is of type offset<uint<64>,b>.

        .macro off_plus_sizeof
        swap                   ; OFF VAL
        siz                    ; OFF VAL ESIZ
        rot                    ; VAL ESIZ OFF
        swap                   ; VAL OFF ESIZ
        ogetm                  ; VAL OFF ESIZ ESIZM
        nip                    ; VAL OFF ESIZM
        swap                   ; VAL ESIZM OFF
        ogetm                  ; VAL ESIZM OFF OFFM
        swap                   ; VAL ESIZM OFFM OFF
        ogetu                  ; VAL ESIZM OFFM OFF OFFU
        rot                    ; VAL ESIZM OFF OFFU OFFM
        mullu
        nip2                   ; VAL ESIZM OFF (OFFU*OFFM)
        rot                    ; VAL OFF (OFFU*OFFM) ESIZM
        addlu
        nip2                   ; VAL OFF (OFFU*OFFM+ESIZM)
        push ulong<64>1        ; VAL OFF (OFFU*OFFM+ESIZM) 1UL
        mko                    ; VAL OFF NOFF
        .end

;;; RAS_MACRO_HANDLE_STRUCT_FIELD_LABEL
;;; ( OFF SOFF - OFF )
;;;
;;; Given a struct type element, it's offset and the offset of the struct
;;; on the stack, increase the offset by the element's label, in case
;;; it exists.
;;;
;;; The C environment required is:
;;;
;;; `field' is a pkl_ast_node with the struct field being
;;; mapped.

        .macro handle_struct_field_label
   .c if (PKL_AST_STRUCT_FIELD_TYPE_LABEL (field) == NULL)
        drop                    ; OFF
   .c else
   .c {
        nip                     ; SOFF
        .c PKL_GEN_PAYLOAD->in_mapper = 0;
        .c PKL_PASS_SUBPASS (PKL_AST_STRUCT_FIELD_TYPE_LABEL (field));
        .c PKL_GEN_PAYLOAD->in_mapper = 1;
                                ; SOFF LOFF
        ogetm                   ; SOFF LOFF LOFFM
        swap                    ; SOFF LOFFM LOFF
        ogetu                   ; SOFF LOFFM LOFF LOFFU
        nip                     ; SOFF LOFFM LOFFU
        mullu
        nip2                    ; SOFF (LOFFM*LOFFU)
        swap                    ; (LOFFM*LOFFU) SOFF
        ogetm                   ; (LOFFM*LOFFU) SOFF SOFFM
        swap                    ; (LOFFM*LOFFU) SOFFM SOFF
        ogetu                   ; (LOFFM*LOFFU) SOFFM SOFF SOFFU
        nip                     ; (LOFFM*LOFFU) SOFFM SOFFU
        mullu
        nip2                    ; (LOFFM*LOFFU) (SOFFM*SOFFU)
        addlu
        nip2                    ; (LOFFM*LOFFU+SOFFM*SOFFU)
        push ulong<64>1         ; (LOFFM*LOFFU+SOFFM*SOFFU) 1UL
        mko                     ; OFF
   .c }
        .end

;;; RAS_MACRO_CHECK_STRUCT_FIELD_CONSTRAINT
;;; ( -- )
;;;
;;; Evaluate the given struct field's constraint, raising an
;;; exception if not satisfied.
;;;
;;; The C environment required is:
;;;
;;; `field' is a pkl_ast_node with the struct field being
;;; mapped.

        .macro check_struct_field_constraint
   .c if (PKL_AST_STRUCT_FIELD_TYPE_CONSTRAINT (field) != NULL)
   .c {
        .c PKL_GEN_PAYLOAD->in_mapper = 0;
        .c PKL_PASS_SUBPASS (PKL_AST_STRUCT_FIELD_TYPE_CONSTRAINT (field));
        .c PKL_GEN_PAYLOAD->in_mapper = 1;
        bnzi .constraint_ok
        drop
        push PVM_E_CONSTRAINT
        raise
.constraint_ok:
        drop
   .c }
        .end

;;; RAS_MACRO_STRUCT_FIELD_MAPPER
;;; ( OFF SOFF -- OFF STR VAL NOFF )
;;;
;;; Map a struct field from the current IOS.
;;; SOFF is the offset of the beginning of the struct.
;;; NOFF is the offset marking the end of this field.
;;;
;;; The C environment required is:
;;;
;;; `field' is a pkl_ast_node with the struct field being
;;; mapped.

        .macro struct_field_mapper
        ;; Increase OFF by the label, if the field has one.
        .e handle_struct_field_label     ; OFF
        dup                             ; OFF OFF
        .c PKL_PASS_SUBPASS (PKL_AST_STRUCT_FIELD_TYPE_TYPE (field));
                                	; OFF VAL
        dup                             ; OFF VAL VAL
        regvar $val                     ; OFF VAL
   .c if (PKL_AST_STRUCT_FIELD_TYPE_NAME (field) == NULL)
        push null
   .c else
        .c PKL_PASS_SUBPASS (PKL_AST_STRUCT_FIELD_TYPE_NAME (field));
                                	; OFF VAL STR
        swap                            ; OFF STR VAL
        ;; Evaluate the field's constraint and raise
        ;; an exception if not satisfied.
        .e check_struct_field_constraint
        ;; Calculate the offset marking the end of the field, which is
        ;; the field's offset plus it's size.
        rot                    ; STR VAL OFF
        .e off_plus_sizeof     ; STR VAL OFF NOFF
        tor                    ; STR VAL OFF
        nrot                   ; OFF STR VAL
        fromr                  ; OFF STR VAL NOFF
        .end

;;; RAS_FUNCTION_STRUCT_MAPPER
;;; ( OFF EBOUND SBOUND -- SCT )
;;;
;;; Assemble a function that maps a struct value at the given offset
;;; OFF.
;;;
;;; Both EBOUND and SBOUND are always null, and not used, i.e. struct maps
;;; are not bounded by either number of fields or size.
;;;
;;; OFF should be of type offset<uint<64>,*>.
;;;
;;; The C environment required is:
;;;
;;; `type_struct' is a pkl_ast_node with the struct type being
;;;  processed.
;;;
;;; `type_struct_fields' is a pkl_ast_node with the chained list fields
;;; of the struct type being processed.
;;;
;;; `field' is a scratch pkl_ast_node.

        ;; NOTE: please be careful when altering the lexical structure of
        ;; this code (and of the code in expanded macros). Every local
        ;; added should be also reflected in the compile-time environment
        ;; in pkl-tab.y, or horrible things _will_ happen.  So if you
        ;; add/remove locals here, adjust accordingly in
        ;; pkl-tab.y:struct_type_specifier.  Thank you very mucho!

        .function struct_mapper
        prolog
        pushf
        drop                    ; sbound
        drop                    ; ebound
        regvar $off
        push ulong<64>0
        regvar $nfield
        pushvar $off            ; OFF
        dup                     ; OFF OFF
        ;; Iterate over the fields of the struct type.
 .c for (field = type_struct_fields; field; field = PKL_AST_CHAIN (field))
 .c {
        pushvar $off             ; ...[EOFF ENAME EVAL] NEOFF OFF
        .e struct_field_mapper   ; ...[EOFF ENAME EVAL] NEOFF
        ;; If the struct is pinned, replace NEOFF with OFF
   .c if (PKL_AST_TYPE_S_PINNED (type_struct))
   .c {
        drop
        pushvar $off            ; ...[EOFF ENAME EVAL] OFF
   .c }
        ;; Increase the number of fields.
        pushvar $nfield         ; ...[EOFF ENAME EVAL] NEOFF NFIELD
        push ulong<64>1         ; ...[EOFF ENAME EVAL] NEOFF NFIELD 1UL
        addl
        nip2                    ; ...[EOFF ENAME EVAL] NEOFF (NFIELD+1UL)
        popvar $nfield          ; ...[EOFF ENAME EVAL] NEOFF
 .c }
        drop  			; ...[EOFF ENAME EVAL]
        ;; Ok, at this point all the struct field triplets are
        ;; in the stack.  Push the number of fields, create
        ;; the mapped struct and return it.
        pushvar $nfield         ; OFF [OFF STR VAL]... NFIELD
        .c PKL_GEN_PAYLOAD->in_mapper = 0;
        .c PKL_PASS_SUBPASS (type_struct);
        .c PKL_GEN_PAYLOAD->in_mapper = 1;
                                ; OFF [OFF STR VAL]... NFIELD TYP
        mksct                   ; SCT
        popf 1
        return
        .end

;;; RAS_FUNCTION_STRUCT_CONSTRUCTOR
;;; ( SCT -- SCT SCT )
;;;
;;; Assemble a function that constructs a struct value of a given type
;;; from another struct value.
;;;
;;; The C environment required is:
;;;
;;; `type_struct' is a pkl_ast_node with the struct type being
;;;  processed.
;;;
;;; `type_struct_fields' is a pkl_ast_node with the chained list fields
;;; of the struct type being processed.
;;;
;;; `field' is a scratch pkl_ast_node.

        .function struct_constructor
        prolog
        pushf
        push null               ; SCT OFF(NULL)
        ;; Initialize $nfield to 0UL
        push ulong<64>0
        regvar $nfield
        ;; Initialize $off to 0UL#b.
        push ulong<64>0
        push ulong<64>1
        mko
        dup                     ; OFF OFF
        regvar $off             ; OFF
        ;; Iterate over the fields of the struct type.
 .c for (field = type_struct_fields; field; field = PKL_AST_CHAIN (field))
 .c {
        pushvar $off               ; ...[EOFF ENAME EVAL] NEOFF OFF
;        .e struct_field_mapper      ; ...[EOFF ENAME EVAL] NEOFF
        ;; If the struct is pinned, replace NEOFF with OFF
   .c if (PKL_AST_TYPE_S_PINNED (type_struct))
   .c {
        drop
        pushvar $off            ; ...[EOFF ENAME EVAL] OFF
   .c }
        ;; Increase the number of fields.
        pushvar $nfield         ; ...[EOFF ENAME EVAL] NEOFF NFIELD
        push ulong<64>1         ; ...[EOFF ENAME EVAL] NEOFF NFIELD 1UL
        addl
        nip2                    ; ...[EOFF ENAME EVAL] NEOFF (NFIELD+1UL)
        popvar $nfield          ; ...[EOFF ENAME EVAL] NEOFF
 .c }
        drop  			; ...[EOFF ENAME EVAL]
        ;; Ok, at this point all the struct field triplets are
        ;; in the stack.  Push the number of fields, create
        ;; the struct and return it.
        pushvar $nfield        ; OFF [OFF STR VAL]... NFIELD
        .c PKL_GEN_PAYLOAD->in_constructor = 0;
        .c PKL_PASS_SUBPASS (type_struct);
        .c PKL_GEN_PAYLOAD->in_constructor = 1;
                                ; OFF [OFF STR VAL]... NFIELD TYP
        mksct                   ; SCT
        popf 1
        return
        .end

;;; RAS_MACRO_STRUCT_FIELD_WRITER
;;; ( SCT I -- )
;;;
;;; Macro that writes the Ith field of struct SCT to IO space.
;;;
;;; C environment required:
;;; `field' is a pkl_ast_node with the type of the field to write.

        .macro struct_field_writer
        ;; The field is written out only if it hasn't
        ;; been modified since the last mapping.
        smodi                   ; SCT I MODIFIED
        bzi .unmodified
        drop                    ; SCT I
        srefi                   ; SCT I EVAL
        nrot                    ; EVAL SCT I
        srefio                  ; EVAL SCT I EOFF
        nip2                    ; EVAL EOFF
        swap                    ; EOFF EVAL
        .c PKL_GEN_PAYLOAD->in_writer = 1;
        .c PKL_PASS_SUBPASS (PKL_AST_STRUCT_FIELD_TYPE_TYPE (field));
        .c PKL_GEN_PAYLOAD->in_writer = 0;
        ba .next
.unmodified:
        drop                    ; SCT I
        drop                    ; SCT
        drop                    ; _
.next:
        .end

;;; RAS_FUNCTION_STRUCT_WRITER
;;; ( OFF VAL -- )
;;;
;;; Assemble a function that pokes a mapped struct value to it's mapped
;;; offset in the current IOS.
;;; The C environment required is:
;;;
;;; `type_struct' is a pkl_ast_node with the struct type being
;;;  processed.
;;;
;;; `type_struct_fields' is a pkl_ast_node with the chained list fields
;;; of the struct type being processed.
;;;
;;; `field' is a scratch pkl_ast_node.

        .function struct_writer
        prolog
        pushf
        regvar $sct
        drop                    ; OFF is not used.
.c { uint64_t i;
 .c for (i = 0, field = type_struct_fields; field; field = PKL_AST_CHAIN (field), ++i)
 .c {
        ;; Poke this struct field, but only if it has been modified
        ;; since the last mapping.
        pushvar $sct            ; SCT
        .c pkl_asm_insn (RAS_ASM, PKL_INSN_PUSH, pvm_make_ulong (i, 64));
                                ; SCT I
        .e struct_field_writer
 .c }
.c }
        popf 1
        push null
        return
        .end
