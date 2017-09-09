/* pcl.h - Poke Command Language definitions.  */

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

#ifndef PCL_H
#define PCL_H

#include <config.h>

#include <config.h>
#include <stdlib.h>

struct pcl_type
{
  int sign;	/* 0 => unsigned, 1 => signed.  */
  int size;	/* Size in bits.  */
};

/*
 * The PCL AST.
 */

enum pcl_ast_node_type
{
  PCL_AST_TYPE,
  PCL_AST_TYPEDEF,
  PCL_AST_INT,
  PCL_AST_STR,
  PCL_AST_ID
};

struct pcl_ast_node
{
  enum pcl_ast_node_type type;
  union
  {
    int64_t integer;
    char *str;
    char *id;
  } val;

  struct pcl_ast_node **children;
  size_t nchildren;
};



/*
 * The PCL stack machine.
 */

enum pcl_sm_op
  {
    SM_OP_NIL = 0,	/* End of program.  */

    SM_OP_DST,		/*                s DST -> - */
    SM_OP_DFI,		/*   [d] [q...] t i DFI -> - */
    SM_OP_DFS,		/* [f ..] [d] [q] i DFS -> - */
    SM_OP_DTY,          /* s w DTY -> - => pcl_type */
    SM_OP_DTF,		/* i i DTF -> - => pcl_type */
                        /* t i DTF -> - => pcl_type */
    SM_OP_POK,		/*     a v POK -> - */
    SM_OP_PEE,		/*       a PEE -> v */
    
    SM_OP_ASG,		/*     s d ASG -> s */
    SM_OP_IFE,		/*   t e c IFE -> - */
    SM_OP_FOR,		/* b p c i FOR -> - */
    
    SM_OP_NEG,		/*   y NEG -> z */
    SM_OP_ADD,		/* x y ADD -> z */
    SM_OP_SUB,		/* x y SUB -> z */
    SM_OP_MUL,		/* x y MUL -> z */
    SM_OP_DIV,		/* x y DIV -> z */
    SM_OP_MOD,		/* x y MOD -> z */
    SM_OP_EQL,		/* x y EQL -> z */
    SM_OP_NEQ,		/* x y NEQ -> z */
    SM_OP_LT,		/* x y LT  -> z */
    SM_OP_GT,		/* x y GT  -> z */
    SM_OP_LTE,		/* x y LTE -> z */
    SM_OP_GTE,		/* x y GTE -> z */
    SM_OP_AND,		/* x y AND -> z */
    SM_OP_OR,		/* x y IOR -> z */
    SM_OP_NOT,		/* x y NOT -> z */

    SM_OP_SYM,
    SM_OP_INT,
    SM_OP_STR
  };



#if 0
/* The struct construction (scons) machine gets a sequence of
   bytecodes, an IO space and an address in the space as input
   arguments.  The result of executing the program is a `pcl_struct'
   mapped into the IO space at the given address, or an error.  */

struct pcl_struct *scons (char *ops, struct poke_io *io,
                          poke_io_addr_t addr);
#endif

#endif /* ! PCL_H */
