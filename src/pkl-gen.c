/* pkl-gen.c - Code generator for Poke.  */

/* Copyright (C) 2017 Jose E. Marchesi */

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

#include <config.h>
#include <stdio.h>

#include <pkl-gen.h>

/* Emit the PVM code for a given PKL AST.
  
   Returns 0 if an error occurs.
   Returns 1 otherwise.
*/

int
pkl_gen (pkl_ast_node ast)
{
  pkl_ast_node tmp;
  size_t i;
  
  if (ast == NULL)
    {
      /* Pushes NIL to the stack.  */
      fprintf (stdout, "NIL\n");
      goto success;
    }

  switch (PKL_AST_CODE (ast))
    {
    case PKL_AST_PROGRAM:

      for (tmp = PKL_AST_PROGRAM_ELEMS (ast); tmp; tmp = PKL_AST_CHAIN (tmp))
        {
          if (!pkl_gen (tmp))
            goto error;
        }

      break;
    case PKL_AST_INTEGER:
      fprintf (stdout, "PUSH %d\n", PKL_AST_INTEGER_VALUE (ast));
      break;
        
    case PKL_AST_STRING:
      /* XXX: add string to the string table and push the offset plus
         a relocation.  */
      fprintf (stdout, "PUSH '%s'\n", PKL_AST_STRING_POINTER (ast));
      /* fprintf (stdout, "PUSH %d\n", PKL_AST_STRING_LENGTH (ast)); */
      break;

    case PKL_AST_IDENTIFIER:
      fprintf (stdout, "PUSH '%s'\n", PKL_AST_IDENTIFIER_POINTER (ast));
      fprintf (stdout, "PUSH %d\n", PKL_AST_IDENTIFIER_LENGTH (ast));
      fprintf (stdout, "GETID\n");
      break;

    case PKL_AST_DOC_STRING:
      fprintf (stdout, "PUSH '%s'\n", PKL_AST_DOC_STRING_POINTER (ast));
      break;

    case PKL_AST_LOC:
      /* Pushes the current value of the location counter to the
         stack.  */
      fprintf (stdout, "LOC\n");
      break;

    case PKL_AST_ARRAY_REF:
      if (! pkl_gen (PKL_AST_ARRAY_REF_INDEX (ast)))
        goto error;
      if (! pkl_gen (PKL_AST_ARRAY_REF_BASE (ast)))
        goto error;
      fprintf (stdout, "AREF\n");
      break;

    case PKL_AST_STRUCT_REF:
      if (! pkl_gen (PKL_AST_STRUCT_REF_IDENTIFIER (ast)))
        goto error;
      if (! pkl_gen (PKL_AST_STRUCT_REF_BASE (ast)))
        goto error;
      fprintf (stdout, "SREF\n");
      break;

    case PKL_AST_TYPE:
      switch (PKL_AST_TYPE_CODE (ast))
        {
        case PKL_TYPE_CHAR:
        case PKL_TYPE_SHORT:
        case PKL_TYPE_INT:
        case PKL_TYPE_LONG:
          fprintf (stdout, "PUSH %d\n", PKL_AST_TYPE_CODE (ast));
          break;
        case PKL_TYPE_ENUM:
          fprintf (stdout, "PUSH '%s'\n", PKL_AST_ENUM_TAG (PKL_AST_TYPE_ENUMERATION (ast)));
          fprintf (stdout, "ETYPE\n");
          break;
        case PKL_TYPE_STRUCT:
          fprintf (stdout, "PUSH '%s'\n", PKL_AST_STRUCT_TAG (PKL_AST_TYPE_STRUCT (ast)));
          fprintf (stdout, "STYPE\n");
          break;
        default:
          fprintf (stderr, "unknown type code\n");
          goto error;
        }
      break;

    case PKL_AST_ASSERTION:
      if (! pkl_gen (PKL_AST_ASSERTION_EXP (ast)))
        goto error;
      fprintf (stdout, "ASSERT\n");
      break;

    case PKL_AST_LOOP:     
      if (PKL_AST_LOOP_PRE (ast))
        {
          if (!pkl_gen (PKL_AST_LOOP_PRE (ast)))
            goto error;
        }

      /* XXX: get and generate label Ln.  */
      fprintf (stdout, "Ln:\n");

      if (PKL_AST_LOOP_COND (ast))
        {
          if (!pkl_gen (PKL_AST_LOOP_COND (ast)))
            goto error;
        }

      /* XXX: get label for Le.  */
      fprintf (stdout, "BNZ Le\n");

      if (PKL_AST_LOOP_BODY (ast))
        {
          if (!pkl_gen (PKL_AST_LOOP_BODY (ast)))
            goto error;
        }

      if (PKL_AST_LOOP_POST (ast))
        {
          if (! pkl_gen (PKL_AST_LOOP_POST (ast)))
            goto error;
        }

      fprintf (stdout, "BA Ln\n");

      /* XXX: generate label for Le. */
      fprintf (stdout, "Le:\n");
      break;

    case PKL_AST_COND:

      if (!pkl_gen (PKL_AST_COND_EXP (ast)))
        goto error;

      /* Generate label Le.  */
      fprintf (stdout, "BZ Le\n");

      if (!pkl_gen (PKL_AST_COND_THENPART (ast)))
        goto error;

      fprintf (stdout, "Le:\n");

      if (PKL_AST_COND_ELSEPART (ast))
        {
          if (!pkl_gen (PKL_AST_COND_ELSEPART (ast)))
            goto error;
        }
      
      break;

    case PKL_AST_FIELD:
      {
        if (PKL_AST_TYPE_CODE (PKL_AST_FIELD_TYPE (ast)) == PKL_TYPE_STRUCT)
          {
            /* if the field type is a STYPE, do a CALL to the referred
               struct passing LOC in the stack, and getting the new
               LOC in the stack.  */
          }
        
        if (!pkl_gen (PKL_AST_FIELD_SIZE (ast)))
          goto error;
        if (!pkl_gen (PKL_AST_FIELD_NUM_ENTS (ast)))
          goto error;
        if (!pkl_gen (PKL_AST_FIELD_DOCSTR (ast)))
          goto error;
        if (!pkl_gen (PKL_AST_FIELD_TYPE (ast)))
          goto error;
        fprintf (stdout, "PUSH %d\n", PKL_AST_FIELD_ENDIAN (ast));
        if (!pkl_gen (PKL_AST_FIELD_NAME (ast)))
          goto error;
        fprintf (stdout, "DFIELD\n");

        /* Update LOC.  */
        fprintf (stdout, "LOC\n");
        fprintf (stdout, "ADD\n");
        fprintf (stdout, "SETLOC\n");
      }
      
      break;

    case PKL_AST_STRUCT:

      if (!pkl_gen (PKL_AST_STRUCT_MEM (ast)))
        goto error;
      if (!pkl_gen (PKL_AST_STRUCT_DOCSTR (ast)))
        goto error;
      if (!pkl_gen (PKL_AST_STRUCT_TAG (ast)))
        goto error;
      fprintf (stdout, "DSTRUCT\n");

      break;

    case PKL_AST_MEM:

      for (i = 0, tmp = PKL_AST_MEM_COMPONENTS (ast); tmp; tmp = PKL_AST_CHAIN (tmp), ++i)
        {
          if (!pkl_gen (tmp))
            goto error;
        }

      fprintf (stdout, "PUSH %d\n", PKL_AST_MEM_ENDIAN (ast));
      fprintf (stdout, "PUSH %d\n", i); /* Number of components.  */
      fprintf (stdout, "DMEM\n");
      
      break;

    case PKL_AST_ENUM:

      if (!pkl_gen (PKL_AST_ENUM_DOCSTR (ast)))
        goto error;

      for (i = 0, tmp = PKL_AST_ENUM_VALUES (ast); tmp; tmp = PKL_AST_CHAIN (tmp), ++i)
        {
          if (!pkl_gen (tmp))
            goto error;
        }

      fprintf (stdout, "PUSH %d\n", i); /* Number of enumerators.  */

      if (!pkl_gen (PKL_AST_ENUM_TAG (ast)))
        goto error;

      fprintf (stdout, "DENUM\n");
      
      break;

    case PKL_AST_ENUMERATOR:

      if (!pkl_gen (PKL_AST_ENUMERATOR_DOCSTR (ast)))
        goto error;
      if (!pkl_gen (PKL_AST_ENUMERATOR_VALUE (ast)))
        goto error;
      if (!pkl_gen (PKL_AST_ENUMERATOR_IDENTIFIER (ast)))
        goto error;

      /* No need for explicit command for ENUMERATOR.  */
      
      break;

    case PKL_AST_EXP:
      {
#define PKL_DEF_OP(SYM, OPCODE) OPCODE,
        static char *pkl_ast_op_opcode[] =
          {
#include "pkl-ops.def"
          };
#undef PKL_DEF_OP

        
      /* Generate operators.  */
      for (i = 0; i < PKL_AST_EXP_NUMOPS (ast); ++i)
        {
          if (!pkl_gen (PKL_AST_EXP_OPERAND (ast, i)))
            goto error;
        }

      fprintf (stdout, "%s\n", pkl_ast_op_opcode [PKL_AST_EXP_CODE (ast)]);
      break;
      }
      
    default:
      fprintf (stderr, "Unknown AST node.\n");
      goto error;
    }

 success:
  
  return 1;
  
 error:
    return 0;
}
