/* Convert one word from memory into an equivalent assembly instruction
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

#ifndef __GPDIS_H__
#define __GPDIS_H__

#define	false 0
#define	true  1
#define	gp_boolean	int

enum proc_class {
  PROC_CLASS_UNKNOWN,   /* Unknown device */
  PROC_CLASS_EEPROM8,   /* 8 bit EEPROM */
  PROC_CLASS_EEPROM16,  /* 16 bit EEPROM */
  PROC_CLASS_GENERIC,   /* 12 bit device */
  PROC_CLASS_PIC12,     /* 12 bit devices */
  PROC_CLASS_SX,        /* 12 bit devices */
  PROC_CLASS_PIC14,     /* 14 bit devices */
  PROC_CLASS_PIC16,     /* 16 bit devices */
  PROC_CLASS_PIC16E     /* enhanced 16 bit devices */
};


extern gp_boolean gp_decode_mnemonics;
extern gp_boolean gp_decode_extended;

/*	internal function.
int gp_disassemble(MemBlock *m,
                   int org,
                   enum proc_class class,
                   char *buffer,
                   size_t sizeof_buffer);
 */
#endif
