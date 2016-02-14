/* Disassemble memory
   Copyright (C) 2001, 2002, 2003, 2004, 2005
   Craig Franklin

This file is part of gputils.

gputils is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gputils is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gputils; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

//#include "stdhdr.h"
//#include "libgputils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gpdis.h"
#include "gpopcode.h"

#include "portlist.h"


typedef struct MemBlock {
  unsigned int    base;
  unsigned int    size;
  unsigned short *memory;
//  struct MemBlock *next;
} MemBlock;


#define DECODE_ARG0 snprintf(buffer, sizeof_buffer, "%s", instruction->name)

#define DECODE_ARG1(ARG1) snprintf(buffer, sizeof_buffer, "%s\t%#lx", \
                                  instruction->name,\
                                  ARG1)

#define DECODE_ARG1WF(ARG1, ARG2) snprintf(buffer, sizeof_buffer, "%s\t%#lx, %s", \
                                          instruction->name,\
                                          ARG1, \
                                          (ARG2 ? "f" : "w"))

#define DECODE_ARG2(ARG1, ARG2) snprintf(buffer, sizeof_buffer, "%s\t%#lx, %#lx", \
                                        instruction->name,\
                                        ARG1, \
                                        ARG2)

#define DECODE_ARG3(ARG1, ARG2, ARG3) snprintf(buffer, sizeof_buffer, "%s\t%#lx, %#lx, %#lx", \
                                             instruction->name,\
                                             ARG1, \
                                             ARG2, \
                                             ARG3)


gp_boolean gp_decode_mnemonics = false;
gp_boolean gp_decode_extended = false;


#define DECODE_arg2(ARG1, ARG2)   decode_arg2(buffer,sizeof_buffer,instruction,ARG1,ARG2)
#define DECODE_arg2ff(ARG1, ARG2) decode_arg2ff(buffer,sizeof_buffer,instruction,ARG1,ARG2)
#define DECODE_arg2fsr(ARG1, ARG2) decode_arg2fsr(buffer,sizeof_buffer,instruction,ARG1,ARG2)
#define DECODE_arg3(ARG1, ARG2, ARG3) decode_arg3(buffer, sizeof_buffer,instruction,ARG1,ARG2,ARG3)
#define DECODE_arg3bit(ARG1, ARG2, ARG3) decode_arg3bit(buffer, sizeof_buffer,instruction,ARG1,ARG2,ARG3)

//	movwf f,a
void decode_arg2(char *buffer, int sizeof_buffer,struct insn *inst,long arg1,long access)
{
	char *portname=NULL;
	if( (arg1>=0x60) && (access==0) ) {
		portname=GetPortName(0x0f00 | arg1);
	}
	
	if(	portname ) {
		snprintf(buffer, sizeof_buffer,
				"%s\t%s",
				inst->name,portname);
	}else{
		snprintf(buffer, sizeof_buffer,
				"%s\t%#lx, %#lx",
				inst->name,arg1,access);
	}
}

// lfsr FSR%d, int12
void decode_arg2fsr(char *buffer, int sizeof_buffer,struct insn *inst,long fsr,long arg12)
{
		snprintf(buffer, sizeof_buffer,
				"%s\tFSR%ld, 0x%03lx",
				inst->name,fsr,arg12);
}

// movf f,dst,a   dst=W(=0),f(=1)のどちらか.
void decode_arg3(char *buffer, int sizeof_buffer,struct insn *inst,long arg1,long arg2,long access)
{
	char *portname=NULL;
	char *dst;

	// arg1がポートアドレスなら、名称に変換.
	if( (arg1>=0x60) && (access==0) ) {
		portname=GetPortName(0x0f00 | arg1);
	}

	// デスティネーションが Wかfのどちらか.
	if(arg2==1) dst = "f";
	else		dst = "W";

	if(	portname ) {
		snprintf(buffer, sizeof_buffer,
				"%s\t%s, %s",
				inst->name,portname,dst);
	}else{
		snprintf(buffer, sizeof_buffer,
				"%s\t%#lx, %s, %#lx",
				inst->name,arg1,dst,access);
	}
}

// bsf  f,bit,a
void decode_arg3bit(char *buffer,int sizeof_buffer,struct insn *inst,long arg1,long bit,long access)
{
	char *portname=NULL;

	// arg1がポートアドレスなら、名称に変換.
	if( (arg1>=0x60) && (access==0) ) {
		portname=GetPortName(0x0f00 | arg1);
	}

	if(	portname ) {
		snprintf(buffer, sizeof_buffer,
				"%s\t%s, bit%ld",
				inst->name,portname	,bit);
	}else{
		snprintf(buffer, sizeof_buffer,
				"%s\t%#lx, bit%ld, %#lx",
				inst->name,arg1		,bit,access);
	}
}

void decode_port_ff(char *name1,long arg1)
{
	char *namep;

	sprintf(name1,"0x%03lx",arg1);

	if( arg1>=0xf50 ) {
		namep = GetPortName(arg1);
		if(namep) {
			strcpy(name1,namep);
		}
	}
}

void decode_arg2ff(char *buffer, int sizeof_buffer,struct insn *inst,long arg1,long arg2)
{
	char name1[128];
	char name2[128];

	decode_port_ff(name1,arg1);
	decode_port_ff(name2,arg2);
	snprintf(buffer, sizeof_buffer,
			"%s\t%s, %s",
			inst->name,name1,name2);
}


/************************************************************************
 * i_memory_get
 *
 * Fetch a word from the pic memory. This function will traverse through
 * the linked list of memory blocks searching for the address from the 
 * word will be fetched. If the address is not found, then `0' will be
 * returned.
 *
 * Inputs:
 *  address - 
 *  m - start of the instruction memory
 * Returns
 *  the word from that address
 *
 ************************************************************************/
int i_memory_get(MemBlock *m, unsigned int address)
{
	int offset = address  - m->base;
	if((offset>=0)&&(offset<m->size)) {
		return m->memory[offset];
	}
#if	0
//  do {
    assert(m->memory != NULL);

    if( ((address >> I_MEM_BITS) & 0xFFFF) == m->base )
      return m->memory[address & I_MEM_MASK];

//    m = m->next;
//  } while(m);
#endif

  return 0;  
}





int
gp_disassemble(MemBlock *m,
               int org,
               enum proc_class class,
               char *buffer,
               size_t sizeof_buffer)
{
  int i;
  int value;
  long int opcode;
  struct insn *instruction = NULL;
  int num_words = 1;

  opcode = i_memory_get(m, org) & 0xffff;

  switch (class) {
  case PROC_CLASS_EEPROM8:
  case PROC_CLASS_EEPROM16:
  case PROC_CLASS_GENERIC:
    snprintf(buffer, sizeof_buffer, "unsupported processor class");
    return 0;
  case PROC_CLASS_PIC12:
    for(i = 0; i < num_op_12c5xx; i++) {
      if((op_12c5xx[i].mask & opcode) == op_12c5xx[i].opcode) {
        instruction = &op_12c5xx[i];
        break;
      }
    }
    break;
  case PROC_CLASS_SX:
    for(i = 0; i < num_op_sx; i++) {
      if((op_sx[i].mask & opcode) == op_sx[i].opcode) {
        instruction = &op_sx[i];
        break;
      }
    }
    break;
  case PROC_CLASS_PIC14:
    for(i = 0; i < num_op_16cxx; i++) {
      if((op_16cxx[i].mask & opcode) == op_16cxx[i].opcode) {
        instruction = &op_16cxx[i];
        break;
      }
    }
    break;
  case PROC_CLASS_PIC16:
    for(i = 0; i < num_op_17cxx; i++) {
      if((op_17cxx[i].mask & opcode) == op_17cxx[i].opcode) {
        instruction = &op_17cxx[i];
        break;
      }
    }
    break;


//★とりあえず、classはここに決め撃ちされている.
  case PROC_CLASS_PIC16E:
    if (gp_decode_mnemonics) {
      for(i = 0; i < num_op_18cxx_sp; i++) {
        if((op_18cxx_sp[i].mask & opcode) == op_18cxx_sp[i].opcode) {
          instruction = &op_18cxx_sp[i];
          break;
        }
      }
    }
    if (instruction == NULL) {
      for(i = 0; i < num_op_18cxx; i++) {
        if((op_18cxx[i].mask & opcode) == op_18cxx[i].opcode) {
          instruction = &op_18cxx[i];
          break;
        }
      }
    }
    
    //拡張命令は通常は使用しない予定なのでfalse
    if ((instruction == NULL) && (gp_decode_extended)) {
      /* might be from the extended instruction set */
      for(i = 0; i < num_op_18cxx_ext; i++) {
        if((op_18cxx_ext[i].mask & opcode) == op_18cxx_ext[i].opcode) {
          instruction = &op_18cxx_ext[i];
          break;
        }
      }
    }
    break;
  default:
    assert(0);
  }

  if (instruction == NULL)  {
    snprintf(buffer, sizeof_buffer, "dw\t%#lx  ;unknown opcode", opcode);
    return num_words;
  }

  switch (instruction->class)
    {
    case INSN_CLASS_LIT3_BANK:
      DECODE_ARG1((opcode & 0x7) << 5); 
      break;
    case INSN_CLASS_LIT3_PAGE:
      DECODE_ARG1((opcode & 0x7) << 9); 
      break;
    case INSN_CLASS_LIT1:
      DECODE_ARG1(opcode & 1); 
      break;
    case INSN_CLASS_LIT4:
      DECODE_ARG1(opcode & 0xf); 
      break;
    case INSN_CLASS_LIT4S:
      DECODE_ARG1((opcode & 0xf0) >> 4); 
      break;
    case INSN_CLASS_LIT6:
      DECODE_ARG1(opcode & 0x3f); 
      break;
    case INSN_CLASS_LIT8:
    case INSN_CLASS_LIT8C12:
    case INSN_CLASS_LIT8C16:
      DECODE_ARG1(opcode & 0xff); 
      break;
    case INSN_CLASS_LIT9:
      DECODE_ARG1(opcode & 0x1ff); 
      break;
    case INSN_CLASS_LIT11:
      DECODE_ARG1(opcode & 0x7ff); 
      break;
    case INSN_CLASS_LIT13:
      DECODE_ARG1(opcode & 0x1fff); 
      break;
    case INSN_CLASS_LITFSR:
      DECODE_ARG2(((opcode >> 6) & 0x3), (opcode & 0x3f));
      break;
    case INSN_CLASS_RBRA8:
      value = opcode & 0xff;
      /* twos complement number */
      if (value & 0x80) {
        value = -((value ^ 0xff) + 1);
      }
      DECODE_ARG1((unsigned long)(org + value + 1) * 2); 
      break;
    case INSN_CLASS_RBRA11:
      value = opcode  & 0x7ff;
      /* twos complement number */
      if (value & 0x400) {
        value = -((value ^ 0x7ff) + 1);
      }      
      DECODE_ARG1((unsigned long)(org + value + 1) * 2); 
      break;
    case INSN_CLASS_LIT20:
      {
        long int dest;

        num_words = 2;
        dest = (i_memory_get(m, org + 1) & 0xfff) << 8;
        dest |= opcode & 0xff;      
        DECODE_ARG1(dest * 2); 
      }
      break;
    case INSN_CLASS_CALL20:
      {
        long int dest;

        num_words = 2;
        dest = (i_memory_get(m, org + 1) & 0xfff) << 8;
        dest |= opcode & 0xff;      
	snprintf(buffer, sizeof_buffer, "%s\t%#lx, %#lx",
                instruction->name,
                dest * 2,
		(opcode >> 8) & 1);
      }
      break;
    case INSN_CLASS_FLIT12:
      {
        long int k;
        long int file;

        num_words = 2;
        k = i_memory_get(m, org + 1) & 0xff;
        k |= ((opcode & 0xf) << 8);
		file = (opcode >> 4) & 0x3;
        DECODE_arg2fsr(file, k);
      }
      break;
    
    // movff
    case INSN_CLASS_FF:
      {
        long int file1;
        long int file2;

        num_words = 2;
        file1 = opcode & 0xfff;
        file2 = i_memory_get(m, org + 1) & 0xfff;
        DECODE_arg2ff(file1, file2);
      }
      break;
    case INSN_CLASS_FP:
      DECODE_ARG2((opcode & 0xff), ((opcode >> 8) & 0x1f));
      break;
    case INSN_CLASS_PF:
      DECODE_ARG2(((opcode >> 8) & 0x1f), (opcode & 0xff));
      break;
    case INSN_CLASS_SF:
      {
        long int offset;
        long int file;

        num_words = 2;
        offset = opcode & 0x7f;
        file = i_memory_get(m, org + 1) & 0xfff;
        DECODE_ARG2(offset, file);
      }
      break;
    case INSN_CLASS_SS:
      {
        long int offset1;
        long int offset2;

        num_words = 2;
        offset1 = opcode & 0x7f;
        offset2 = i_memory_get(m, org + 1) & 0x7f;
        DECODE_ARG2(offset1, offset2);
      }
      break;

    case INSN_CLASS_OPF5:
      DECODE_ARG1(opcode & 0x1f);
      break;
    case INSN_CLASS_OPWF5:
      DECODE_ARG1WF((opcode & 0x1f), ((opcode >> 5) & 1));
      break;
    case INSN_CLASS_B5:
      DECODE_ARG2((opcode & 0x1f), ((opcode >> 5) & 7));
      break;
    case INSN_CLASS_B8:
      DECODE_ARG2((opcode & 0xff), ((opcode >> 8) & 7));
      break;
    case INSN_CLASS_OPF7:
      DECODE_ARG1(opcode & 0x7f);
      break;
    case INSN_CLASS_OPF8:
      DECODE_ARG1(opcode & 0xff);
      break;
    case INSN_CLASS_OPWF7:
      DECODE_ARG1WF((opcode & 0x7f), ((opcode >> 7) & 1));
      break;
    case INSN_CLASS_OPWF8:
      DECODE_ARG1WF((opcode & 0xff), ((opcode >> 8) & 1));
      break;
    case INSN_CLASS_B7:
      DECODE_ARG2((opcode & 0x7f), ((opcode >> 7) & 7));
      break;


	// movwf LABEL,f
    case INSN_CLASS_OPFA8:
//	  DECODE_ARG2((opcode & 0xff), ((opcode >> 8) & 1));
      DECODE_arg2((opcode & 0xff), ((opcode >> 8) & 1));
      break;
	// bsf  f,bit,a
    case INSN_CLASS_BA8:
      DECODE_arg3bit((opcode & 0xff), ((opcode >> 9) & 7), ((opcode >> 8) & 1));
      break;

	// movf f,dst,a   dst=W(=0),f(=1)のどちらか.
    case INSN_CLASS_OPWFA8:
      DECODE_arg3((opcode & 0xff), ((opcode >> 9) & 1), ((opcode >> 8) & 1));
      break;
    case INSN_CLASS_IMPLICIT:
      DECODE_ARG0;
      break;
    case INSN_CLASS_TBL:
      {
        char operator[5]; 

	switch(opcode & 0x3)
	  {
	  case 0:
	    strncpy(operator, "*", sizeof(operator));
	    break;
	  case 1:
	    strncpy(operator, "*+", sizeof(operator));
	    break;
	  case 2:
	    strncpy(operator, "*-", sizeof(operator));
	    break;
	  case 3:
	    strncpy(operator, "+*", sizeof(operator));
	    break;
	  default:
	    assert(0);
	  }

        snprintf(buffer,
                 sizeof_buffer,
                 "%s\t%s",
                 instruction->name,
                 operator);
      }
      break;
    case INSN_CLASS_TBL2:
      DECODE_ARG2(((opcode >> 9) & 1), (opcode & 0xff));
      break;
    case INSN_CLASS_TBL3:
      DECODE_ARG3(((opcode >> 9) & 1), 
                  ((opcode >> 8) & 1), 
		   (opcode & 0xff));
      break;
    default:
      assert(0);
    }

  return num_words;
}

/*
typedef struct MemBlock {
  unsigned int base;
  unsigned int *memory;
  struct MemBlock *next;
} MemBlock;
*/

int disasm_print(unsigned char *buf,int size,int adr)
{
	unsigned short *inst = (unsigned short *)buf;
	char 			outbuf[256];

	char 			inst1[32];		//2word命令のときは2word目の16進表現を文字列にする.
	MemBlock 		mem;
	int 			num_word;

	mem.base = adr>>1;
	mem.size = size;
	mem.memory = (unsigned short *) buf;

	num_word = gp_disassemble(&mem,adr>>1,PROC_CLASS_PIC16E,outbuf,80);

	if(num_word==0) num_word = 2;
	strcpy(inst1,"    ");
	if(	num_word >= 2) {
		sprintf(inst1,"%04x",inst[1]);
	}

	printf("%04x %04x %s   %s\n",adr,inst[0],inst1,outbuf);
	return 2*num_word;
}
