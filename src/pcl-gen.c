/* pcl-gen.c - Code generator for PCL.  */

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

#include <pcl-gen.h>

/* Emit the PVM code for a given PCL AST.
  
   Returns 0 if an error occurs.
   Returns 1 otherwise.
*/

int
pcl_gen (pcl_ast ast)
{
  pcl_ast tmp;
  size_t i;
  
  if (ast == NULL)
    {
      /* Pushes NIL to the stack.  */
      fprintf (stdout, "NIL\n");
      goto success;
    }

  switch (PCL_AST_CODE (ast))
    {
    case PCL_AST_PROGRAM:

      for (tmp = PCL_AST_PROGRAM_DECLARATIONS (ast); tmp; tmp = PCL_AST_CHAIN (tmp))
        {
          if (!pcl_gen (tmp))
            goto error;
        }

      break;
    case PCL_AST_INTEGER:
      fprintf (stdout, "PUSH %d\n", PCL_AST_INTEGER_VALUE (ast));
      break;
        
    case PCL_AST_STRING:
      /* XXX: add string to the string table and push the offset plus
         a relocation.  */
      fprintf (stdout, "PUSH '%s'\n", PCL_AST_STRING_POINTER (ast));
      /* fprintf (stdout, "PUSH %d\n", PCL_AST_STRING_LENGTH (ast)); */
      break;

    case PCL_AST_IDENTIFIER:
      fprintf (stdout, "PUSH '%s'\n", PCL_AST_IDENTIFIER_POINTER (ast));
      fprintf (stdout, "PUSH %d\n", PCL_AST_IDENTIFIER_LENGTH (ast));
      fprintf (stdout, "GETID\n");
      break;

    case PCL_AST_DOC_STRING:
      fprintf (stdout, "PUSH '%s'\n", PCL_AST_DOC_STRING_POINTER (ast));
      break;

    case PCL_AST_LOC:
      /* Pushes the current value of the location counter to the
         stack.  */
      fprintf (stdout, "LOC\n");
      break;

    case PCL_AST_ARRAY_REF:
      if (! pcl_gen (PCL_AST_ARRAY_REF_INDEX (ast)))
        goto error;
      if (! pcl_gen (PCL_AST_ARRAY_REF_BASE (ast)))
        goto error;
      fprintf (stdout, "AREF\n");
      break;

    case PCL_AST_STRUCT_REF:
      if (! pcl_gen (PCL_AST_STRUCT_REF_IDENTIFIER (ast)))
        goto error;
      if (! pcl_gen (PCL_AST_STRUCT_REF_BASE (ast)))
        goto error;
      fprintf (stdout, "SREF\n");
      break;

    case PCL_AST_TYPE:
      switch (PCL_AST_TYPE_CODE (ast))
        {
        case PCL_TYPE_CHAR:
        case PCL_TYPE_SHORT:
        case PCL_TYPE_INT:
        case PCL_TYPE_LONG:
          fprintf (stdout, "PUSH %d\n", PCL_AST_TYPE_CODE (ast));
          break;
        case PCL_TYPE_ENUM:
          fprintf (stdout, "PUSH '%s'\n", PCL_AST_ENUM_TAG (PCL_AST_TYPE_ENUMERATION (ast)));
          fprintf (stdout, "ETYPE\n");
          break;
        case PCL_TYPE_STRUCT:
          fprintf (stdout, "PUSH '%s'\n", PCL_AST_STRUCT_TAG (PCL_AST_TYPE_STRUCT (ast)));
          fprintf (stdout, "STYPE\n");
          break;
        default:
          fprintf (stderr, "unknown type code\n");
          goto error;
        }
      break;

    case PCL_AST_ASSERTION:
      if (! pcl_gen (PCL_AST_ASSERTION_EXP (ast)))
        goto error;
      fprintf (stdout, "ASSERT\n");
      break;

    case PCL_AST_LOOP:     
      if (PCL_AST_LOOP_PRE (ast))
        {
          if (!pcl_gen (PCL_AST_LOOP_PRE (ast)))
            goto error;
        }

      /* XXX: get and generate label Ln.  */
      fprintf (stdout, "Ln:\n");

      if (PCL_AST_LOOP_COND (ast))
        {
          if (!pcl_gen (PCL_AST_LOOP_COND (ast)))
            goto error;
        }

      /* XXX: get label for Le.  */
      fprintf (stdout, "BNZ Le\n");

      if (PCL_AST_LOOP_BODY (ast))
        {
          if (!pcl_gen (PCL_AST_LOOP_BODY (ast)))
            goto error;
        }

      if (PCL_AST_LOOP_POST (ast))
        {
          if (! pcl_gen (PCL_AST_LOOP_POST (ast)))
            goto error;
        }

      fprintf (stdout, "BA Ln\n");

      /* XXX: generate label for Le. */
      fprintf (stdout, "Le:\n");
      break;

    case PCL_AST_COND:

      if (!pcl_gen (PCL_AST_COND_EXP (ast)))
        goto error;

      /* Generate label Le.  */
      fprintf (stdout, "BZ Le\n");

      if (!pcl_gen (PCL_AST_COND_THENPART (ast)))
        goto error;

      fprintf (stdout, "Le:\n");

      if (PCL_AST_COND_ELSEPART (ast))
        {
          if (!pcl_gen (PCL_AST_COND_ELSEPART (ast)))
            goto error;
        }
      
      break;

    case PCL_AST_FIELD:
      {
        if (PCL_AST_FIELD_TYPE (ast) == PCL_TYPE_STRUCT)
          {
            /* if the field type is a STYPE, do a CALL to the referred
               struct passing LOC in the stack, and getting the new
               LOC in the stack.  */
          }
        
        if (!pcl_gen (PCL_AST_FIELD_SIZE (ast)))
          goto error;
        if (!pcl_gen (PCL_AST_FIELD_NUM_ENTS (ast)))
          goto error;
        if (!pcl_gen (PCL_AST_FIELD_DOCSTR (ast)))
          goto error;
        if (!pcl_gen (PCL_AST_FIELD_TYPE (ast)))
          goto error;
        fprintf (stdout, "PUSH %d\n", PCL_AST_FIELD_ENDIAN (ast));
        if (!pcl_gen (PCL_AST_FIELD_NAME (ast)))
          goto error;
        fprintf (stdout, "DFIELD\n");

        /* Update LOC.  */
        fprintf (stdout, "LOC\n");
        fprintf (stdout, "ADD\n");
        fprintf (stdout, "SETLOC\n");
      }
      
      break;

    case PCL_AST_STRUCT:

      if (!pcl_gen (PCL_AST_STRUCT_MEM (ast)))
        goto error;
      if (!pcl_gen (PCL_AST_STRUCT_DOCSTR (ast)))
        goto error;
      if (!pcl_gen (PCL_AST_STRUCT_TAG (ast)))
        goto error;
      fprintf (stdout, "DSTRUCT\n");

      break;

    case PCL_AST_MEM:

      for (i = 0, tmp = PCL_AST_MEM_COMPONENTS (ast); tmp; tmp = PCL_AST_CHAIN (tmp), ++i)
        {
          if (!pcl_gen (tmp))
            goto error;
        }

      fprintf (stdout, "PUSH %d\n", PCL_AST_MEM_ENDIAN (ast));
      fprintf (stdout, "PUSH %d\n", i); /* Number of components.  */
      fprintf (stdout, "DMEM\n");
      
      break;

    case PCL_AST_ENUM:

      if (!pcl_gen (PCL_AST_ENUM_DOCSTR (ast)))
        goto error;

      for (i = 0, tmp = PCL_AST_ENUM_VALUES (ast); tmp; tmp = PCL_AST_CHAIN (tmp), ++i)
        {
          if (!pcl_gen (tmp))
            goto error;
        }

      fprintf (stdout, "PUSH %d\n", i); /* Number of enumerators.  */

      if (!pcl_gen (PCL_AST_ENUM_TAG (ast)))
        goto error;

      fprintf (stdout, "DENUM\n");
      
      break;

    case PCL_AST_ENUMERATOR:

      if (!pcl_gen (PCL_AST_ENUMERATOR_DOCSTR (ast)))
        goto error;
      if (!pcl_gen (PCL_AST_ENUMERATOR_VALUE (ast)))
        goto error;
      if (!pcl_gen (PCL_AST_ENUMERATOR_IDENTIFIER (ast)))
        goto error;

      /* No need for explicit command for ENUMERATOR.  */
      
      break;

    case PCL_AST_EXP:
      {
#define PCL_DEF_OP(SYM, OPCODE) OPCODE,
        static char *pcl_ast_op_opcode[] =
          {
#include "pcl-ops.def"
          };
#undef PCL_DEF_OP

        
      /* Generate operators.  */
      for (i = 0; i < PCL_AST_EXP_NUMOPS (ast); ++i)
        {
          if (!pcl_gen (PCL_AST_EXP_OPERAND (ast, i)))
            goto error;
        }

      fprintf (stdout, "%s\n", pcl_ast_op_opcode [PCL_AST_EXP_CODE (ast)]);
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
