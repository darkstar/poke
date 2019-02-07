;;; -*- mode: asm -*-
;;; pkl-gen-maps.pks - Map-related functions and macros
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

        .function array_mapper
        
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

        .c PKL_GEN_PAYLOAD->in_mapper = 0;
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
        .c PKL_GEN_PAYLOAD->in_mapper = 1;

                                ; OFF ATYPE
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
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
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
        mkma                   ; ARRAY

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
        .end                    ; array_mapper


        .function array_valmapper

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
        .c PKL_GEN_PAYLOAD->in_valmapper = 0;
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
        .c PKL_GEN_PAYLOAD->in_valmapper = 1;
                                ; OFF ATYPE

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
        mkma                    ; ARRAY

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
        .end                    ; array_valmapper


        .function array_writer

;;; RAS_FUNCTION_ARRAY_WRITER
;;; ( OFFSET VAL -- )
;;;
;;; Assemble a function that pokes a mapped array value to it's mapped
;;; offset in the current IOS.
;;;
;;; Note that it is important for the elements of the array to be poked
;;; in order.

        prolog
        pushf
        regvar $value           ; Argument
        regvar $offset          ; Argument
           
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
        pushvar $offset         ; OFF
        pushvar $value          ; OFF ARRAY
        pushvar $idx            ; OFF ARRAY I
        aref                    ; OFF ARRAY I VAL
        nrot                    ; OFF VAL ARRAY I
        arefo                   ; OFF VAL ARRAY I EOFF
        nip2                    ; OFF VAL EOFF
        swap                    ; OFF EOFF VAL

        .c PKL_GEN_PAYLOAD->in_writer = 1;
        .c PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));
        .c PKL_GEN_PAYLOAD->in_writer = 0;
                                ; OFF
        drop                    ; _

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

