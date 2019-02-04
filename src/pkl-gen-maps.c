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

/* ARRAY_ELEM_BOUND_MAPPER
   ( OFF NELEM -- ARR )

   Array mapper for maps bounded by number of elements.

   ; One scratch register is used in this code:
   ;
   ; %idx   is %r2 and contains the index of the
   ;        array element being processed.

   PROLOG

   PUSHF
   REGVAR  ; Argument: Offset, 0,0
   PUSH null
   REGVAR  ; Local: Array Offset, 0,1
   PUSH null
   REGVAR  ; Local: Current Offset, 0,2
   PUSH null
   REGVAR  ; Local: Number of Elements, 0,3

   PUSHVAR 0,0              ; OFF (must be offset<uint<64>,*>)
   ; Initialize locals.
   OGETM		    ; OFF OMAG
   SWAP                     ; OMAG OFF
   OGETU                    ; OMAG OFF OUNIT
   ROT                      ; OFF OUNIT OMAG
   MULLU                    ; OFF OUNIT OMAG (OUNIT*OMAG)
   NIP2                     ; OFF (OUNIT*OMAG)
   POPVAR 0,1               ; OFF
   PUSH 0UL                 ; OFF 0UL
   POPVAR 0,2               ; OFF
   PUSH 0UL                 ; OFF 0UL
   POPR %idx                ; OFF
   SUBPASS array_type_nelem ; OFF NELEM

   POPVAR 0,3               ; OFF
   SUBPASS array_type       ; OFF ATYPE

   .while
   PUSHR %idx               ; OFF ATYPE I
   PUSHVAR 0,3              ; OFF ATYPE I NELEM
   LTLU                     ; OFF ATYPE I NELEM (NELEM<I)
   NIP2                     ; OFF ATYPE (NELEM<I)
   .loop
   ; OFF ATYPE

   ; Mount the Ith element triplet: [EOFF EIDX EVAL]
   PUSHVAR 0,1              ; ... AOFFMAG
   PUSHVAR 0,2              ; ... AOFFMAG EOMAG
   ADDLU                    ; ... AOFFMAG EOMAG (AOFFMAG+EOMAG)
   NIP2                     ; ... (AOFFMAG+EOMAG)
   PUSH 1UL                 ; ... (AOFFMAG+EOMAG) EOUNIT
   MKO                      ; ... EOFF
   DUP                      ; ... EOFF EOFF
   SUBPASS array_type       ; ... EOFF EVAL

   ; XXX EOFF = EOFF - %aoff

   ; Update the current offset with the size of the value just
   ; peeked.
   SIZ                      ; ... EOFF EVAL ESIZ
   ROT                      ; ... EVAL ESIZ EOFF
   OGETM                    ; ... EVAL ESIZ EOFF EOMAG
   ROT                      ; ... EVAL EOFF EOMAG ESIZ
   OGETM                    ; ... EVAL EOFF EOMAG ESIZ ESIGMAG
   ROT                      ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
   ADDLU                    ; ... EVAL EOFF ESIZ ESIGMAG EOMAG (ESIGMAG+EOMAG)
   POPVAR 0,2               ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
   DROP                     ; ... EVAL EOFF ESIZ ESIGMAG
   DROP                     ; ... EVAL EOFF ESIZ
   DROP                     ; ... EVAL EOFF
   PUSHR %idx               ; ... EVAL EOFF EIDX
   ROT                      ; ... EOFF EIDX EVAL

   ; Increase the current index and process the next
   ; element.
   PUSHR %idx              ; ... EOFF EIDX EVAL EIDX
   PUSH 1UL                ; ... EOFF EIDX EVAL EIDX 1UL
   ADDLU                   ; ... EOFF EIDX EVAL EDIX 1UL (EIDX+1UL)
   NIP2                    ; ... EOFF EIDX EVAL (EIDX+1UL)
   POPR %idx               ; ... EOFF EIDX EVAL
   .endloop

   PUSHVAR 0,3             ; OFF ATYPE [EOFF EIDX EVAL]... NELEM
   DUP                     ; OFF ATYPE [EOFF EIDX EVAL]... NELEM NINITIALIZER
   MKMA                    ; ARRAY

   POPF 1
   RETURN

*/

#define COMPILE_ARRAY_ELEM_BOUND_MAPPER(CLOSURE)                        \
  do                                                                    \
    {                                                                   \
      pvm_program mapper_program;                                       \
                                                                        \
      /* Compile a mapper, or a valmapper, function to a closure.  */   \
      PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,                      \
                                     PKL_GEN_PAYLOAD->compiler,         \
                                     0 /* prologue */));                \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);              \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);              \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);              \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR,                      \
                    0, 0);                                              \
                                                                        \
      int idxreg = 2;                                                   \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0, 1);                \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (0, 64)); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0, 2);                \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH,                         \
                    pvm_make_ulong (0, 64));                            \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, idxreg);                \
                                                                        \
      if (use_valmapper_nelem)                                          \
        {                                                               \
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);           \
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SEL);                     \
          pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                     \
        }                                                               \
      else                                                              \
        {                                                               \
          PKL_GEN_PAYLOAD->in_mapper = 0;                               \
          PKL_PASS_SUBPASS (array_type_nelem);                          \
          PKL_GEN_PAYLOAD->in_mapper = 1;                               \
        }                                                               \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0,3);                 \
      PKL_GEN_PAYLOAD->in_mapper = 0;                                   \
      PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));             \
      PKL_GEN_PAYLOAD->in_mapper = 1;                                   \
                                                                        \
      pkl_asm_while (PKL_GEN_ASM);                                      \
      {                                                                 \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0,3);              \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_LTLU);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
      }                                                                 \
      pkl_asm_loop (PKL_GEN_ASM);                                       \
      {                                                                 \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 2);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKO);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                       \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 2);                  \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 3);                  \
        PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));           \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 3);               \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 2);               \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SIZ);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPVAR, 0, 2);              \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                       \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, idxreg);              \
      }                                                                 \
      pkl_asm_endloop (PKL_GEN_ASM);                                    \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 3);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKMA);                        \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);                      \
                                                                        \
      mapper_program = pkl_asm_finish (PKL_GEN_ASM,                     \
                                       0 /* epilogue */);               \
                                                                        \
      PKL_GEN_POP_ASM;                                                  \
      pvm_specialize_program (mapper_program);                          \
      (CLOSURE) = pvm_make_cls (mapper_program);                        \
    } while (0)

/* Array valmapper for maps bounded by number of elements.

   ; Four scratch registers are used in this code:
   ;
   ; %aoff  is %r0 and contains the offset of the
   ;        array.
   ;
   ; %off   is %r1 and contains the offset of the
   ;        array element being processed, relative to
   ;        the start of the array, in bits.
   ;
   ; %idx   is %r2 and contains the index of the
   ;        array element being processed.
   ;
   ; %nelem is %r3 and holds the total number of
   ;        elements contained in the array.

   ; Register arguments in a new environment frame:
   ;   Offset: 0,0 New value: 0,1
   PUSHF
   REGVAR
   REGVAR

   PUSHVAR 0,0              ; OFF (must be offset<uint<64>,*>)
   ; Initialize registers.
   OGETM		    ; OFF OMAG
   SWAP                     ; OMAG OFF
   OGETU                    ; OMAG OFF OUNIT
   ROT                      ; OFF OUNIT OMAG
   MULLU                    ; OFF OUNIT OMAG (OUNIT*OMAG)
   NIP2                     ; OFF (OUNIT*OMAG)
   POPR %aoff               ; OFF
   PUSH 0UL                 ; OFF 0UL
   POPR %off                ; OFF
   PUSH 0UL                 ; OFF 0UL
   POPR %idx                ; OFF
   PUSHVAR 0,1            ; OFF NVAL
   SEL                    ; OFF NVAL NELEM
   NIP                    ; OFF NELEM
   POPR %nelem              ; OFF ATYPE
   SUBPASS array_type       ; OFF ATYPE

   .while
   PUSHR %idx               ; OFF ATYPE I
   PUSHR %nelem             ; OFF ATYPE I NELEM
   LTLU                     ; OFF ATYPE I NELEM (NELEM<I)
   NIP2                     ; OFF ATYPE (NELEM<I)
   .loop
   ; OFF ATYPE

   ; Mount the Ith element triplet: [EOFF EIDX EVAL]
   PUSHR %aoff              ; ... AOFFMAG
   PUSHR %off               ; ... AOFFMAG EOMAG
   ADDLU                    ; ... AOFFMAG EOMAG (AOFFMAG+EOMAG)
   NIP2                     ; ... (AOFFMAG+EOMAG)
   PUSH 1UL                 ; ... (AOFFMAG+EOMAG) EOUNIT
   MKO                      ; ... EOFF
   DUP                      ; ... EOFF EOFF
   PUSHVAR 0,1              ; ... EOFF EOFF NVAL
   PUSHR %idx               ; ... EOFF EOFF NVAL IDX
   AREF                     ; ... EOFF EOFF NVAL IDX NELEM
   NIP2                     ; ... EOFF EOFF NELEM
   SWAP                     ; ... EOFF NELEM EOFF
   SUBPASS array_type       ; ... EOFF EVAL

   ; XXX EOFF = EOFF - %aoff

   ; Update the current offset with the size of the value just
   ; peeked.
   SIZ                      ; ... EOFF EVAL ESIZ
   ROT                      ; ... EVAL ESIZ EOFF
   OGETM                    ; ... EVAL ESIZ EOFF EOMAG
   ROT                      ; ... EVAL EOFF EOMAG ESIZ
   OGETM                    ; ... EVAL EOFF EOMAG ESIZ ESIGMAG
   ROT                      ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
   ADDLU                    ; ... EVAL EOFF ESIZ ESIGMAG EOMAG (ESIGMAG+EOMAG)
   POPR %off                ; ... EVAL EOFF ESIZ ESIGMAG EOMAG
   DROP                     ; ... EVAL EOFF ESIZ ESIGMAG
   DROP                     ; ... EVAL EOFF ESIZ
   DROP                     ; ... EVAL EOFF
   PUSHR %idx               ; ... EVAL EOFF EIDX
   ROT                      ; ... EOFF EIDX EVAL

   ; Increase the current index and process the next
   ; element.
   PUSHR %idx              ; ... EOFF EIDX EVAL EIDX
   PUSH 1UL                ; ... EOFF EIDX EVAL EIDX 1UL
   ADDLU                   ; ... EOFF EIDX EVAL EDIX 1UL (EIDX+1UL)
   NIP2                    ; ... EOFF EIDX EVAL (EIDX+1UL)
   POPR %idx               ; ... EOFF EIDX EVAL
   .endloop

   PUSHR %nelem            ; OFF ATYPE [EOFF EIDX EVAL]... NELEM
   DUP                     ; OFF ATYPE [EOFF EIDX EVAL]... NELEM NINITIALIZER
   MKMA                    ; ARRAY

   POPF 1
   RETURN
*/

#define COMPILE_ARRAY_ELEM_BOUND_VALMAPPER(CLOSURE)                     \
  do                                                                    \
    {                                                                   \
      pvm_program mapper_program;                                       \
                                                                        \
      PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,                      \
                                     PKL_GEN_PAYLOAD->compiler,         \
                                     0 /* prologue */));                \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR,                      \
                    0, 0);                                              \
                                                                        \
      int aoffreg = 0;                                                  \
      int offreg = 1;                                                   \
      int idxreg = 2;                                                   \
      int nelemreg = 3;                                                 \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MULLU);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, aoffreg);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (0, 64)); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, offreg);                \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH,                         \
                    pvm_make_ulong (0, 64));                            \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, idxreg);                \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);               \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SEL);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                         \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, nelemreg);              \
                                                                        \
      PKL_GEN_PAYLOAD->in_valmapper = 0;                                \
      PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));             \
      PKL_GEN_PAYLOAD->in_valmapper = 1;                                \
                                                                        \
      pkl_asm_while (PKL_GEN_ASM);                                      \
      {                                                                 \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, nelemreg);           \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_LTLU);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
      }                                                                 \
      pkl_asm_loop (PKL_GEN_ASM);                                       \
      {                                                                 \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, aoffreg);            \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, offreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKO);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                       \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 0);                  \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 1);                  \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 2);                  \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SAVER, 3);                  \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_AREF);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SWAP);                      \
                                                                        \
        PKL_PASS_SUBPASS (PKL_AST_TYPE_A_ETYPE (array_type));           \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 3);               \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 2);               \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 1);               \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RESTORER, 0);               \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SIZ);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_OGETM);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, offreg);              \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DROP);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ROT);                       \
                                                                        \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, idxreg);              \
      }                                                                 \
      pkl_asm_endloop (PKL_GEN_ASM);                                    \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, nelemreg);             \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_DUP);                         \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_MKMA);                        \
                                                                        \
      /* Finish the mapper function.  */                                \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);                      \
                                                                        \
      /* All right, the mapper function is compiled.  */                \
      mapper_program = pkl_asm_finish (PKL_GEN_ASM,                     \
                                       0 /* epilogue */);               \
                                                                        \
      PKL_GEN_POP_ASM;                                                  \
      pvm_specialize_program (mapper_program);                          \
      (CLOSURE) = pvm_make_cls (mapper_program);                        \
    } while (0)

/* The following macro compiles a "writer" function for array types.
   It pokes the elements of the array.

   This macro should only be expanded in the pkl_gen_pr_type_array
   handler in pkl-gen.c.

   Note that it is important for the elements of the array to be poked
   in order.

   ; The scratch registers used in this code are:
   ;
   ; %idx   is %r0 and contains the index of the array element
   ;        being processed.

   ; Register arguments in a new environment frame:
   ;   Offset: 0,0
   ;   Value: 0,1
   PUSHF
   REGVAR
   REGVAR
           
   PUSH 0UL                 ; 0UL
   POPR %idx                ; _

   .while
     PUSHR %idx             ; I
     PUSHVAR 0,1            ; I ARRAY
     SEL                    ; I ARRAY NELEM
     NIP                    ; I NELEM
     LTLU                   ; I NELEM (NELEM<I)
     NIP2                   ; (NELEM<I)
   .loop
                            ; _

     ; Poke this array element
     PUSHVAR 0,0            ; OFF
     PUSHVAR 0,1            ; OFF ARRAY
     PUSHR %idx             ; OFF ARRAY I
     AREF                   ; OFF ARRAY I VAL
     NROT                   ; OFF VAL ARRAY I
     AREFO                  ; OFF VAL ARRAY I EOFF
     NIP2                   ; OFF VAL EOFF
     SWAP                   ; OFF EOFF VAL
     SUBPASS array_type     ; OFF
     DROP                   ; _

     ; Increase the current index and process the next
     ; element.
     PUSHR %idx             ; EIDX
     PUSH 1UL               ; EIDX 1UL
     ADDLU                  ; EDIX 1UL (EIDX+1UL)
     NIP2                   ; (EIDX+1UL)
     POPR %idx              ; _
   .endloop 

   POPF
   PUSH null
   RETURN
*/

#define COMPILE_ARRAY_WRITER(CLOSURE)                                  \
  do                                                                   \
    {                                                                  \
      pvm_program writer_program;                                      \
      int idxreg = 0;                                                  \
                                                                       \
      PKL_GEN_PUSH_ASM (pkl_asm_new (PKL_PASS_AST,                     \
                                     PKL_GEN_PAYLOAD->compiler,         \
                                     0 /* prologue */));                \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PROLOG);                      \
                                                                        \
      /* Push a new frame and register the local variables passed to  */ \
      /* the function.  From this point on, the passed variables will */ \
      /* be referred using their lexical address.                     */ \
      /*                                                              */ \
      /* The offset is back=0, over=0.                                */ \
      /* The array to write is back=0, over=1.                        */ \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHF);                       \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_REGVAR);                      \
                                                                        \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (0, 64)); \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, idxreg);                \
                                                                        \
      pkl_asm_while (PKL_GEN_ASM);                                      \
      {                                                                 \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_SEL);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP);                       \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_LTLU);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
      }                                                                 \
      pkl_asm_loop (PKL_GEN_ASM);                                       \
      {                                                                 \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 0);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHVAR, 0, 1);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
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
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSHR, idxreg);             \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, pvm_make_ulong (1, 64)); \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_ADDLU);                     \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_NIP2);                      \
        pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPR, idxreg);              \
      }                                                                 \
      pkl_asm_endloop (PKL_GEN_ASM);                                    \
                                                                        \
      /* Finish the writer function.  */                                \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_POPF, 1);                     \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_PUSH, PVM_NULL);              \
      pkl_asm_insn (PKL_GEN_ASM, PKL_INSN_RETURN);                      \
                                                                        \
      /* All right, the writer function is compiled.  */                \
      writer_program = pkl_asm_finish (PKL_GEN_ASM,                     \
                                       0 /* epilogue */);               \
                                                                        \
      PKL_GEN_POP_ASM;                                                  \
      pvm_specialize_program (writer_program);                          \
      (CLOSURE) = pvm_make_cls (writer_program);                        \
    } while (0)
