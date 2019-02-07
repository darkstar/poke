/* pkl-gen-maps.c - Macros implementing map-related programs.  */

/* Copyright (C) 2019 Jose E. Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file is intended to be #included from pkl-gen.c.  The macros
   defined here should be expanded from within handlers in the code
   generator.  */

/* XXX: generate this crap from nice .pks files.  */

#include <config.h>

/* PKL_ASM_ARRAY_MAPPER
  ( OFF EBOUND SBOUND -- ARR )

   Assemble a function that maps an array value at the given offset
   OFF, with mapping attributes EBOUND and SBOUND.

   If both EBOUND and SBOUND are null, then perform an unbounded map,
   i.e. read array elements from IO until EOF.  XXX: what about empty
   arrays?

   Otherwise, if EBOUND is not null, then perform a map bounded by the
   given number of elements.  If EOF is encountered before the given
   amount of elements are read, then raise PVM_E_MAP_BOUNDS.

   Otherwise, if SBOUND is not null, then perform a map bounded by the
   given size (an offset), i.e. read array elements from IO until the
   total size of the array is exactly SBOUND.  If SBOUND is exceeded,
   then raise PVM_E_MAP_BOUNDS.

   Only one of EBOUND or SBOUND simultanously are supported.
   Note that OFF should be of type offset<uint<64>,*>.

   PROLOG

   PUSHF
   REGVAR  ; Argument: SBOUND, 0,0
   REGVAR  ; Argument: EBOUND, 0,1
   REGVAR  ; Argument: OFF,    0,2

   ; Determine the offset of the array, in bits, and put it
   ; in a local.
   PUSHVAR 0,2              ; OFF
   OGETM		    ; OFF OMAG
   SWAP                     ; OMAG OFF
   OGETU                    ; OMAG OFF OUNIT
   ROT                      ; OFF OUNIT OMAG
   MULLU                    ; OFF OUNIT OMAG (OUNIT*OMAG)
   NIP2                     ; OFF (OUNIT*OMAG)
   REGVAR (0,3 EOMAG)       ; OFF

   ; Initialize the element index to 0UL, and put it
   ; in a local.
   PUSH 0UL                 ; OFF 0UL
   REGVAR (0,4 EIDX)        ; OFF

   ; Save the offset in bits of the beginning of the array in a local.
   PUSHVAR 0,3 (EOMAG)      ; OFF EOMAG
   REGVAR (0,5 AOMAG)       ; OFF

   ; If it is not null, transform the SBOUND from an offset to
   ; a magnitude in bits.
   PUSHVAR 0,0 (SBOUND)     ; OFF SBOUND
   BN after_sbound_conv
   OGETM                    ; OFF SBOUND SBOUNDM
   SWAP                     ; OFF SBOUNDM SBOUND
   OGETU                    ; OFF SBOUNDM SBOUND SBOUNDU
   SWAP                     ; OFF SBOUNDM SBOUNDU SBOUND
   DROP                     ; OFF SOBUNDM SBOUNDU
   MULLU                    ; OFF SBOUNDM SBOUNDU (SBOUNDM*SBOUNDU)
   NIP2                     ; OFF (SBOUNDM*SBOUNDU)
   REGVAR (0,6 SBOUNDM)     ; OFF
   PUSH null                ; OFF null
after_sbound_conv:
   DROP                     ; OFF

   SUBPASS array_type       ; OFF ATYPE

   .while
   ; If there is an EBOUND, check it.
   ; Else, if there is a SBOUND, check it.
   ; Else, iterate (unbounded).
   PUSHVAR 0,1 (EBOUND)     ; OFF ATYPE NELEM
   BN loop_on_sbound
   PUSHVAR 0,4 (EIDX)       ; OFF ATYPE NELEM I
   GTLU                     ; OFF ATYPE NELEM I (NELEM>I)
   NIP2                     ; OFF ATYPE (NELEM>I)
   BA end_loop_on
loop_on_sbound:
   DROP                     ; OFF ATYPE
   PUSHVAR 0,6 (SBOUNDM)    ; OFF ATYPE SBOUNDM
   BN loop_unbounded
   PUSHVAR 0,5 (AOMAG)      ; OFF ATYPE SBOUNDM AOMAG
   ADDLU                    ; OFF ATYPE SBOUNDM AOMAG (SBOUNDM+AOMAG)
   NIP2                     ; OFF ATYPE (SBOUNDM+AOMAG)
   PUSHVAR 0,3 (EOMAG)      ; OFF ATYPE (SBOUNDM+AOMAG) EOMAG
   GTLU                     ; OFF ATYPE (SBOUNDM+AOMAG) EOMAG ((SBOUNDM+AOMAG)>EOMAG)
   NIP2                     ; OFF ATYPE ((SBOUNDM+AOMAG)>EOMAG)  
   BA end_loop_on
loop_unbounded:
   DROP                     ; OFF ATYPE
   PUSH 1                   ; OFF ATYPE 1
end_loop_on:
   .loop
                            ; OFF ATYPE

   ; Mount the Ith element triplet: [EOFF EIDX EVAL]
   PUSHVAR 0,3 (EOMAG)      ; ... EOMAG
   PUSH 1UL                 ; ... EOMAG EOUNIT
   MKO                      ; ... EOFF
   DUP                      ; ... EOFF EOFF
   SUBPASS array_type       ; ... EOFF EVAL
   BN eof

   ; Update the current offset with the size of the value just
   ; peeked.
   SIZ                      ; ... EOFF EVAL ESIZ
   ROT                      ; ... EVAL ESIZ EOFF
   OGETM                    ; ... EVAL ESIZ EOFF EOMAG
   ROT                      ; ... EVAL EOFF EOMAG ESIZ
   OGETM                    ; ... EVAL EOFF EOMAG ESIZ ESIGMAG
   ROT                      ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
   ADDLU                    ; ... EVAL EOFF ESIZ ESIGMAG EOMAG (ESIGMAG+EOMAG)
   POPVAR 0,3 (EOMAG)       ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
   DROP                     ; ... EVAL EOFF ESIZ ESIGMAG
   DROP                     ; ... EVAL EOFF ESIZ
   DROP                     ; ... EVAL EOFF
   PUSHVAR 0,4 (EIDX)       ; ... EVAL EOFF EIDX
   ROT                      ; ... EOFF EIDX EVAL

   ; Increase the current index and process the next element.
   PUSHVAR 0,4 (EIDX)      ; ... EOFF EIDX EVAL EIDX
   PUSH 1UL                ; ... EOFF EIDX EVAL EIDX 1UL
   ADDLU                   ; ... EOFF EIDX EVAL EDIX 1UL (EIDX+1UL)
   NIP2                    ; ... EOFF EIDX EVAL (EIDX+1UL)
   POPVAR 0,4 (EIDX)       ; ... EOFF EIDX EVAL
   .endloop

   PUSH null
   BA mountarray
eof:
   ; Remove the partial EOFF null element from the stack.
                           ; ... EOFF null
   DROP                    ; ... EOFF
   DROP                    ; ...
   ; If the array is bounded, raise E_EOF
   PUSHVAR 0,1 (EBOUND)    ; ... EBOUND
   NN                      ; ... EBOUND (EBOUND!=NULL)
   NIP                     ; ... (EBOUND!=NULL)
   PUSHVAR 0,0 (SBOUND)    ; ... (EBOUND!=NULL) SBOUND
   NN                      ; ... (EBOUND!=NULL) SBOUND (SBOUND!=NULL)
   NIP                     ; ... (EBOUND!=NULL) (SBOUND!=NULL)
   OR                      ; ... (EBOUND!=NULL) (SBOUND!=NULL) ARRAYBOUNDED
   NIP2                    ; ... ARRAYBOUNDED
   BZI mountarray
   PUSH E_EOF
   RAISE
mountarray:
   DROP                    ; OFF ATYPE [EOFF EIDX EVAL]...
   PUSHVAR 0,4 (EIDX)      ; OFF ATYPE [EOFF EIDX EVAL]... NELEM
   DUP                     ; OFF ATYPE [EOFF EIDX EVAL]... NELEM NINITIALIZER
   MKMA                    ; ARRAY

   ; Check that the resulting array satisfies the mapping's
   ; bounds (number of elements and total size.)
   PUSHVAR 0,1 (EBOUND)    ; ARRAY EBOUND
   BNN check_ebound
   DROP                    ; ARRAY
   PUSHVAR 0,6 (SBOUNDM)   ; ARRAY SBOUNDM
   BNN check_sbound
   DROP
   BA bounds_ok

check_ebound:
   SWAP                    ; EBOUND ARRAY
   SEL                     ; EBOUND ARRAY NELEM
   ROT                     ; ARRAY NELEM EBOUND
   SUBLU                   ; ARRAY NELEM EBOUND (NELEM-EBOUND)
   BNZLU bounds_fail
   DROP                    ; ARRAY NELEM EBOUND
   DROP                    ; ARRAY NELEM
   DROP                    ; ARRAY
   BA bounds_ok

check_sbound:
   SWAP                    ; SBOUNDM ARRAY
   SIZ                     ; SBOUNDM ARRAY OFF
   OGETM                   ; SBOUNDM ARRAY OFF OFFM
   SWAP                    ; SBOUNDM ARRAY OFFM OFF
   OGETU                   ; SBOUNDM ARRAY OFFM OFF OFFU
   NIP                     ; SBOUNDM ARRAY OFFM OFFU
   MULLU                   ; SBOUNDM ARRAY OFFM OFFU (OFFM*OFFU)
   NIP2                    ; SBOUNDM ARRAY (OFFM*OFFU)
   ROT                     ; ARRAY (OFFM*OFFU) SBOUNDM
   SUBLU                   ; ARRAY (OFFM*OFFU) SBOUNDM ((OFFM*OFFU)-SBOUND)
   BNZLU bounds_fail
   DROP                    ; ARRAY (OFFU*OFFM) SBOUNDM
   DROP                    ; ARRAY (OFFU*OFFM)
   DROP                    ; ARRAY

bounds_ok:

   ; Set the map bound attributes in the new object.
   PUSHVAR 0,0 (SBOUND)    ; ARRAY SBOUND
   MSETSIZ                 ; ARRAY
   PUSHVAR 0,1 (EBOUND)    ; ARRAY EBOUND
   MSETSEL                 ; ARRAY

   POPF 1
   RETURN

bounds_fail:
   PUSH E_MAP_BOUNDS
   RAISE
*/

#define PKL_ASM_ARRAY_MAPPER(CLOSURE)                                   \
  do                                                                    \
    {                                                                   \
      pvm_program mapper_program;                                       \
                                                                        \
      jitter_label eof_label;                                           \
      jitter_label mountarray_label;                                    \
      jitter_label check_ebound_label;                                  \
      jitter_label check_sbound_label;                                  \
      jitter_label bounds_ok_label;                                     \
      jitter_label bounds_fail_label;                                   \
      jitter_label after_sbound_conv_label;                             \
                                                                        \
      PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,                      \
                                     PKL_GEN_PAYLOAD->compiler,         \
                                     0 /* prologue */));                \
                                                                        \
      eof_label = pkl_asm_fresh_label (PKL_GEN_ASM);                    \
      mountarray_label = pkl_asm_fresh_label (PKL_GEN_ASM);             \
      check_ebound_label = pkl_asm_fresh_label (PKL_GEN_ASM);           \
      check_sbound_label = pkl_asm_fresh_label (PKL_GEN_ASM);           \
      bounds_ok_label = pkl_asm_fresh_label (PKL_GEN_ASM);              \
      bounds_fail_label = pkl_asm_fresh_label (PKL_GEN_ASM);            \
      after_sbound_conv_label = pkl_asm_fresh_label (PKL_GEN_ASM);      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 2);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (0, 64)); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 3);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BN, after_sbound_conv_label); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);              \
      pkl_asm_label (PKL_GEN_ASM, after_sbound_conv_label);             \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
                                                                        \
      PKL_GEN_PAYLOAD->in_mapper = 0;                                   \
      PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));             \
      PKL_GEN_PAYLOAD->in_mapper = 1;                                   \
                                                                        \
      pkl_asm_while (PKL_GEN_ASM);                                      \
      {                                                                 \
      jitter_label loop_on_sbound_label = pkl_asm_fresh_label (PKL_GEN_ASM); \
      jitter_label end_loop_on_label = pkl_asm_fresh_label (PKL_GEN_ASM); \
      jitter_label loop_unbounded_label = pkl_asm_fresh_label (PKL_GEN_ASM); \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BN, loop_on_sbound_label);  \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 4);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_GTLU);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BA, end_loop_on_label);     \
                                                                        \
        pkl_asm_label (PKL_GEN_ASM, loop_on_sbound_label);              \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 6);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BN, loop_unbounded_label);  \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 5);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 3);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_GTLU);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BA, end_loop_on_label);     \
                                                                        \
        pkl_asm_label (PKL_GEN_ASM, loop_unbounded_label);              \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_int (1, 32)); \
                                                                        \
        pkl_asm_label (PKL_GEN_ASM, end_loop_on_label);                 \
      }                                                                 \
      pkl_asm_loop (PKL_GEN_ASM);                                       \
      {                                                                 \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 3);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKO);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                        \
       PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));            \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BN, eof_label);              \
                                                                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SIZ);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                      \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                      \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                      \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0, 3);               \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 4);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                        \
                                                                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 4);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                      \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0, 4);               \
      }                                                                 \
      pkl_asm_endloop (PKL_GEN_ASM);                                    \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);              \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BA, mountarray_label);        \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, eof_label);                           \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NN);                          \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NN);                          \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OR);                          \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BZI, mountarray_label);        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_int (PVM_E_EOF, 32));\
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RAISE);                       \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, mountarray_label);                    \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 4);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKMA);                        \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BNN, check_ebound_label);     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 6);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BNN, check_sbound_label);     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BA, bounds_ok_label);         \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, check_ebound_label);                  \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SEL);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SUBLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BNZLU, bounds_fail_label);    \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BA, bounds_ok_label);         \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, check_sbound_label);                  \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SIZ);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SUBLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BNZLU, bounds_fail_label);    \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, bounds_ok_label);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MSETSIZ);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MSETSEL);                     \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);                      \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, bounds_fail_label);                   \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_int (PVM_E_MAP_BOUNDS, 32)); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RAISE);                       \
                                                                        \
      mapper_program = pkl_asm_finish (PKL_GEN_ASM,                     \
                                       0 /* epilogue */);               \
                                                                        \
      PKL_GEN_POP_ASM;                                                  \
      pvm_specialize_program (mapper_program);                          \
      (CLOSURE) = pvm_make_cls (mapper_program);                        \
    } while (0)

/* PKL_ASM_ARRAY_VALMAPPER
   ( VAL NVAL OFF -- ARR )

   Assemble a function that "valmaps" a given NVAL at the given offset
   OFF, using the data of NVAL, and the mapping attributes of VAL.

   This function can raise PVM_E_MAP_BOUNDS if the characteristics of
   NVAL violate the bounds of the map.

   Only one of EBOUND or SBOUND simultanously are supported.
   Note that OFF should be of type offset<uint<64>,*>.

   PROLOG

   PUSHF
   REGVAR  ; Argument: OFF,    0,0
   REGVAR  ; Argument: NVAL,   0,1
   REGVAR  ; Argument: VAL,    0,2

   ; Determine VAL's bounds and set them in locals to be used later.
   PUSHVAR 0,2 (VAL)        ; VAL
   MGETSEL                  ; VAL EBOUND
   REGVAR (EBOUND 0,3)      ; VAL
   MGETSIZ                  ; VAL SBOUND
   REGVAR (SBOUND 0,4)      ; VAL
   DROP                     ; _

   ; Determine the offset of the array, in bits, and put it
   ; in a local.
   PUSHVAR 0,0 (OFF)        ; OFF
   OGETM		    ; OFF OMAG
   SWAP                     ; OMAG OFF
   OGETU                    ; OMAG OFF OUNIT
   ROT                      ; OFF OUNIT OMAG
   MULLU                    ; OFF OUNIT OMAG (OUNIT*OMAG)
   NIP2                     ; OFF (OUNIT*OMAG)
   REGVAR (0,5 EOMAG)       ; OFF

   ; Initialize the element index to 0UL, and put it
   ; in a local.
   PUSH 0UL                 ; OFF 0UL
   REGVAR (0,6 EIDX)        ; OFF

   ; Get the number of elements in NVAL, and put it in a local.
   PUSHVAR 0,1 (NVAL)       ; OFF NVAL
   SEL                      ; OFF NVAL NELEM
   NIP                      ; OFF NELEM
   REGVAR (0,7 NELEM)       ; OFF

   ; If it is not null, transform the SBOUND from an offset to
   ; a magnitude in bits.
   PUSHVAR 0,4 (SBOUND)     ; OFF SBOUND
   BN after_sbound_conv
   OGETM                    ; OFF SBOUND SBOUNDM
   SWAP                     ; OFF SBOUNDM SBOUND
   OGETU                    ; OFF SBOUNDM SBOUND SBOUNDU
   SWAP                     ; OFF SBOUNDM SBOUNDU SBOUND
   DROP                     ; OFF SOBUNDM SBOUNDU
   MULLU                    ; OFF SBOUNDM SBOUNDU (SBOUNDM*SBOUNDU)
   NIP2                     ; OFF (SBOUNDM*SBOUNDU)
   REGVAR (0,8 SBOUNDM)     ; OFF
   PUSH null                ; OFF null
after_sbound_conv:
   DROP                     ; OFF

   ; Check that NVAL satisfies EBOUND if this bound is specified
   ; i.e. the number of elements stored in the array matches the
   ; bound.
   PUSHVAR 0,3 (EBOUND)     ; OFF EBOUND
   BNN check_ebound
   DROP                     ; OFF
   BA ebound_ok
   
check_ebound:
   PUSHVAR 0,7 (NELEM)      ; OFF EBOUND NELEM
   SUBLU                    ; OFF EBOUND NELEM (EBOUND-NELEM)
   BNZLU bounds_fail
   DROP                     ; OFF EBOUND NELEM
   DROP                     ; OFF EBOUND
   DROP                     ; OFF

ebound_ok:
   SUBPASS array_type       ; OFF ATYPE

   .while
   PUSHVAR 0,6 (EIDX)       ; OFF ATYPE I
   PUSHVAR 0,7 (NELEM)      ; OFF ATYPE I NELEM
   LTLU                     ; OFF ATYPE I NELEM (NELEM<I)
   NIP2                     ; OFF ATYPE (NELEM<I)
   .loop
                            ; OFF ATYPE

   ; Mount the Ith element triplet: [EOFF EIDX EVAL]
   PUSHVAR 0,5 (EOMAG)      ; ... EOMAG
   PUSH 1UL                 ; ... EOMAG EOUNIT
   MKO                      ; ... EOFF
   DUP                      ; ... EOFF EOFF
   
   PUSHVAR 0,1 (NVAL)       ; ... EOFF EOFF NVAL
   PUSHVAR 0,6 (EIDX)       ; ... EOFF EOFF NVAL IDX
   AREF                     ; ... EOFF EOFF NVAL IDX ENVAL
   NIP2                     ; ... EOFF EOFF ENVAL
   SWAP                     ; ... EOFF ENVAL EOFF
   PUSHVAR 0,2 (VAL)        ; ... EOFF ENVAL EOFF VAL
   PUSHVAR 0,6 (EIDX)       ; ... EOFF ENVAL EOFF VAL EIDX
   AREF                     ; ... EOFF ENVAL EOFF VAL EIDX OVAL
   NIP2                     ; ... EOFF ENVAL EOFF OVAL
   NROT                     ; ... EOFF OVAL ENVAL EOFF
   SUBPASS array_type       ; ... EOFF EVAL

   ; Update the current offset with the size of the value just
   ; peeked.
   SIZ                      ; ... EOFF EVAL ESIZ
   ROT                      ; ... EVAL ESIZ EOFF
   OGETM                    ; ... EVAL ESIZ EOFF EOMAG
   ROT                      ; ... EVAL EOFF EOMAG ESIZ
   OGETM                    ; ... EVAL EOFF EOMAG ESIZ ESIGMAG
   ROT                      ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
   ADDLU                    ; ... EVAL EOFF ESIZ ESIGMAG EOMAG (ESIGMAG+EOMAG)
   POPVAR 0,5 (EOMAG)       ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
   DROP                     ; ... EVAL EOFF ESIZ ESIGMAG
   DROP                     ; ... EVAL EOFF ESIZ
   DROP                     ; ... EVAL EOFF
   PUSHVAR 0,6 (EIDX)       ; ... EVAL EOFF EIDX
   ROT                      ; ... EOFF EIDX EVAL

   ; Increase the current index and process the next element.
   PUSHVAR 0,6 (EIDX)      ; ... EOFF EIDX EVAL EIDX
   PUSH 1UL                ; ... EOFF EIDX EVAL EIDX 1UL
   ADDLU                   ; ... EOFF EIDX EVAL EDIX 1UL (EIDX+1UL)
   NIP2                    ; ... EOFF EIDX EVAL (EIDX+1UL)
   POPVAR 0,6 (EIDX)       ; ... EOFF EIDX EVAL
   .endloop

   PUSHVAR 0,6 (EIDX)      ; OFF ATYPE [EOFF EIDX EVAL]... NELEM
   DUP                     ; OFF ATYPE [EOFF EIDX EVAL]... NELEM NINITIALIZER
   MKMA                    ; ARRAY

   ; Check that the resulting array satisfies the mapping's
   ; total size bound.
   PUSHVAR 0,8 (SBOUNDM)   ; ARRAY SBOUNDM
   BNN check_sbound
   DROP
   BA sbound_ok

check_sbound:
   SWAP                    ; SBOUND ARRAY
   SIZ                     ; SBOUND ARRAY OFF
   OGETM                   ; SBOUND ARRAY OFF OFFM
   SWAP                    ; SBOUND ARRAY OFFM OFF
   OGETU                   ; SBOUND ARRAY OFFM OFF OFFU
   NIP                     ; SBOUND ARRAY OFFM OFFU
   MULLU                   ; SBOUND ARRAY OFFM OFFU (OFFM*OFFU)
   NIP2                    ; SBOUND ARRAY (OFFM*OFFU)
   ROT                     ; ARRAY (OFFM*OFFU) SBOUND
   SUBLU                   ; ARRAY (OFFM*OFFU) SBOUND ((OFFM*OFFU)-SBOUND)
   BNZLU bounds_fail
   DROP                    ; ARRAY (OFFU*OFFM) SBOUND
   DROP                    ; ARRAY (OFFU*OFFM)
   DROP                    ; ARRAY

sbound_ok:

   ; Set the map bound attributes in the new object.
   PUSHVAR 0,4 (SBOUND)    ; ARRAY SBOUND
   MSETSIZ                 ; ARRAY
   PUSHVAR 0,3 (EBOUND)    ; ARRAY EBOUND
   MSETSEL                 ; ARRAY

   POPF 1
   RETURN

bounds_fail:
   PUSH E_MAP_BOUNDS
   RAISE
*/

#define PKL_ASM_ARRAY_VALMAPPER(CLOSURE)                                \
  do                                                                    \
    {                                                                   \
      pvm_program mapper_program;                                       \
                                                                        \
      jitter_label check_ebound_label;                                  \
      jitter_label check_sbound_label;                                  \
      jitter_label ebound_ok_label;                                     \
      jitter_label sbound_ok_label;                                     \
      jitter_label bounds_fail_label;                                   \
      jitter_label after_sbound_conv_label;                             \
                                                                        \
      PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,                      \
                                     PKL_GEN_PAYLOAD->compiler,         \
                                     0 /* prologue */));                \
                                                                        \
      check_ebound_label = pkl_asm_fresh_label (PKL_GEN_ASM);           \
      check_sbound_label = pkl_asm_fresh_label (PKL_GEN_ASM);           \
      ebound_ok_label = pkl_asm_fresh_label (PKL_GEN_ASM);              \
      sbound_ok_label = pkl_asm_fresh_label (PKL_GEN_ASM);              \
      bounds_fail_label = pkl_asm_fresh_label (PKL_GEN_ASM);            \
      after_sbound_conv_label = pkl_asm_fresh_label (PKL_GEN_ASM);      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 2);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MGETSEL);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MGETSIZ);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (0, 64)); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SEL);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 4);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BN, after_sbound_conv_label); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);              \
      pkl_asm_label (PKL_GEN_ASM, after_sbound_conv_label);             \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 3);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BNN, check_ebound_label);     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BA, ebound_ok_label);         \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, check_ebound_label);                  \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 7);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SUBLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BNZLU, bounds_fail_label);    \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, ebound_ok_label);                     \
      PKL_GEN_PAYLOAD->in_valmapper = 0;                                \
      PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));             \
      PKL_GEN_PAYLOAD->in_valmapper = 1;                                \
                                                                        \
      pkl_asm_while (PKL_GEN_ASM);                                      \
      {                                                                 \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 6);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 7);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_LTLU);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                       \
      }                                                                 \
      pkl_asm_loop (PKL_GEN_ASM);                                       \
      {                                                                 \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 5);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKO);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                        \
                                                                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 6);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_AREF);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 2);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 6);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_AREF);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NROT);                       \
       PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));            \
                                                                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SIZ);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                      \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                      \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                      \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0, 5);               \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 6);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                        \
                                                                        \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 6);              \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                      \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                       \
       pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0, 6);               \
      }                                                                 \
      pkl_asm_endloop (PKL_GEN_ASM);                                    \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 6);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKMA);                        \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 8);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BNN, check_sbound_label);     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BA, sbound_ok_label);         \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, check_sbound_label);                  \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SIZ);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SUBLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_BNZLU, bounds_fail_label);    \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                        \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, sbound_ok_label);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 4);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MSETSIZ);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 3);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MSETSEL);                     \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);                      \
                                                                        \
      pkl_asm_label (PKL_GEN_ASM, bounds_fail_label);                   \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_int (PVM_E_MAP_BOUNDS, 32)); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RAISE);                       \
                                                                        \
      mapper_program = pkl_asm_finish (PKL_GEN_ASM,                     \
                                       0 /* epilogue */);               \
                                                                        \
      PKL_GEN_POP_ASM;                                                  \
      pvm_specialize_program (mapper_program);                          \
      (CLOSURE) = pvm_make_cls (mapper_program);                        \
    } while (0)

/* PKL_ASM_ARRAY_WRITER
   ( OFFSET VAL -- )

   Assemble a function that pokes a mapped array value to it's mapped
   offset in the current IOS.

   Note that it is important for the elements of the array to be poked
   in order.

   PROLOG
   
   ; Register arguments in a new environment frame:
   ;   Offset: 0,1
   ;   Value: 0,0
   PUSHF
   REGVAR (0,0 VALUE)
   REGVAR (0,1 OFFSET)
           
   PUSH 0UL                 ; 0UL
   REGVAR (0,2 IDX)         ; _

   .while
     PUSHVAR 0,2 (IDX)      ; I
     PUSHVAR 0,0 (VALUE)    ; I ARRAY
     SEL                    ; I ARRAY NELEM
     NIP                    ; I NELEM
     LTLU                   ; I NELEM (NELEM<I)
     NIP2                   ; (NELEM<I)
   .loop
                            ; _

     ; Poke this array element
     PUSHVAR 0,1 (OFFSET)   ; OFF
     PUSHVAR 0,0 (VALUE)    ; OFF ARRAY
     PUSHVAR 0,2 (IDX)      ; OFF ARRAY I
     AREF                   ; OFF ARRAY I VAL
     NROT                   ; OFF VAL ARRAY I
     AREFO                  ; OFF VAL ARRAY I EOFF
     NIP2                   ; OFF VAL EOFF
     SWAP                   ; OFF EOFF VAL
     SUBPASS array_type     ; OFF
     DROP                   ; _

     ; Increase the current index and process the next
     ; element.
     PUSHVAR 0,2 (IDX)      ; EIDX
     PUSH 1UL               ; EIDX 1UL
     ADDLU                  ; EDIX 1UL (EIDX+1UL)
     NIP2                   ; (EIDX+1UL)
     POPVAR 0,2 (IDX)       ; _
   .endloop 

   POPF
   PUSH null
   RETURN
*/

#define PKL_ASM_ARRAY_WRITER(CLOSURE)                                  \
  do                                                                   \
    {                                                                  \
      pvm_program writer_program;                                      \
                                                                       \
      PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,                      \
                                     PKL_GEN_PAYLOAD->compiler,         \
                                     0 /* prologue */));                \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (0, 64)); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_while (PKL_GEN_ASM);                                      \
      {                                                                 \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 2);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);             \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SEL);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_LTLU);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
      }                                                                 \
      pkl_asm_loop (PKL_GEN_ASM);                                       \
      {                                                                 \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 2);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_AREF);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NROT);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_AREFO);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                      \
                                                                        \
        PKL_GEN_PAYLOAD->in_writer = 1;                                 \
        PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));           \
        PKL_GEN_PAYLOAD->in_writer = 0;                                 \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 2);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0, 2);              \
      }                                                                 \
      pkl_asm_endloop (PKL_GEN_ASM);                                    \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);              \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);                      \
                                                                        \
      writer_program = pkl_asm_finish (PKL_GEN_ASM,                     \
                                       0 /* epilogue */);               \
                                                                        \
      PKL_GEN_POP_ASM;                                                  \
      pvm_specialize_program (writer_program);                          \
      (CLOSURE) = pvm_make_cls (writer_program);                        \
    } while (0)
