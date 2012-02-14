/* Linker script for Cortex-M3
 *
 * Version:CodeSourcery Sourcery G++ Lite 2007q3-53
 * BugURL:https://support.codesourcery.com/GNUToolchain/
 *
 *  Copyright 2007 CodeSourcery.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply. */

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
ENTRY(_start)
SEARCH_DIR(.)
GROUP(-lgcc -lc -lcs3 -lcs3unhosted -lcs3micro)

MEMORY
{
  rom (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  ram (rwx) : ORIGIN = 0x10000000, LENGTH =  32K
  
  /* ram1: Currently used for buffers for the Ethernet controller ram1(rwx) : ORIGIN = 0x2007C000, LENGTH = 16k	 */
  ram2(rwx) : ORIGIN = 0x20080000, LENGTH = 16k	/* appears to be free, could be used by USB if it is in that mode */
}

/* These force the linker to search for particular symbols from
 * the start of the link process and thus ensure the user's
 * overrides are picked up
 */
EXTERN(__cs3_reset_cortex_m)
EXTERN(__cs3_interrupt_vector_cortex_m)
EXTERN(__cs3_start_c main __cs3_stack __cs3_stack_size __cs3_heap_end)

/* MTJ: I have the stack at the end of the first RAM section and it takes up 0x100 bytes, which should be plenty */
/*      because it is just for main() and for saving the state on an interrupt */
PROVIDE(__cs3_stack = __cs3_region_start_ram + __cs3_region_size_ram);
/* MTJ changed PROVIDE(__cs3_stack_size = __cs3_region_start_ram + __cs3_region_size_ram - _end); */
PROVIDE(__cs3_stack_size = 0x100);
/* MTJ: I have the first heap section set to be from the end of data placed in the first RAM section up to the stack */
PROVIDE(__cs3_heap_start = _end); 
PROVIDE(__cs3_heap_end = __cs3_region_start_ram + __cs3_region_size_ram - __cs3_stack_size);
/* MTJ: I have the second heap section to be all of the second RAM section */
PROVIDE(__cs3_heap_start2 = __cs3_region_start_ram2); 
PROVIDE(__cs3_heap_end2 = __cs3_region_start_ram2 + 16K);

SECTIONS
{
  /* This is the section for code and is placed in the ROM block */
  /* It starts at address 0 *and* the interrupt vector block must go first */
  .text :
  {
    CREATE_OBJECT_SYMBOLS
    __cs3_region_start_rom = .;
    *(.cs3.region-head.rom)
    __cs3_interrupt_vector = __cs3_interrupt_vector_cortex_m;
    *(.cs3.interrupt_vector)
    /* Make sure we pulled in an interrupt vector.  */
    ASSERT (. != __cs3_interrupt_vector_cortex_m, "No interrupt vector");
    *(.rom)
    *(.rom.b)

    __cs3_reset = __cs3_reset_cortex_m;
    *(.cs3.reset)
    /* Make sure we pulled in some reset code.  */
    ASSERT (. != __cs3_reset, "No reset code");

    *(.text .text.* .gnu.linkonce.t.*)
    *(.plt)
    *(.gnu.warning)
    *(.glue_7t) *(.glue_7) *(.vfp11_veneer)

    *(.rodata .rodata.* .gnu.linkonce.r.*)

    *(.ARM.extab* .gnu.linkonce.armextab.*)
    *(.gcc_except_table)
    *(.eh_frame_hdr)
    *(.eh_frame)

    . = ALIGN(4);
    KEEP(*(.init))

    . = ALIGN(4);
    __preinit_array_start = .;
    KEEP (*(.preinit_array))
    __preinit_array_end = .;

    . = ALIGN(4);
    __init_array_start = .;
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    __init_array_end = .;

    . = ALIGN(0x4);
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*crtend.o(.ctors))

    . = ALIGN(4);
    KEEP(*(.fini))

    . = ALIGN(4);
    __fini_array_start = .;
    KEEP (*(.fini_array))
    KEEP (*(SORT(.fini_array.*)))
    __fini_array_end = .;

    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*crtend.o(.dtors))

    . = ALIGN(4);
    __cs3_regions = .;
    LONG (0)
    LONG (__cs3_region_init_ram)
    LONG (__cs3_region_start_ram)
    LONG (__cs3_region_init_size_ram)
    LONG (__cs3_region_zero_size_ram)
  }

  /* .ARM.exidx is sorted, so has to go in its own output section.  */
  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } >rom
  __exidx_end = .;
  .text.align :
  {
    . = ALIGN(8);
    _etext = .;
  } >rom
  __cs3_region_size_rom = LENGTH(rom);
  __cs3_region_num = 1;

  /* This is the block for data and is placed (mostly) in RAM */
  /* The initialization data is stored in ROM so it can be loaded at start time */
  .data :
  {
    __cs3_region_start_ram = .;
    *(.cs3.region-head.ram)
    KEEP(*(.jcr))
    *(.got.plt) *(.got)
    *(.shdata)
    *(.data .data.* .gnu.linkonce.d.*)
    *(.ram)
    . = ALIGN (8);
    _edata = .;
  } >ram AT>rom
  /* This is another block for data that (in theory) is for static variables that are */
  /*   not initialized by the programmer (but you can see that there is space allocated */
  /*   in ROM for that) */
  .bss :
  {
    *(.shbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    *(.ram.b)
    . = ALIGN (8);
    _end = .;
    __end = .;
  } >ram AT>rom
  /* This used for USB RAM section */
  /* NOTE: Actually, it is not used right now and I am using all of that RAM for heap */
	.usb_ram (NOLOAD):
	{
		*.o (USB_RAM)
	} > ram2
  /* Just keep track of where the first heap block lives */
  .heap (NOLOAD) :
  {
    *(.heap)
  } >ram
  /* Just keep track of where the second heap block lives */
  .heap2 (NOLOAD) :
  {
  	__cs3_region_start_ram2 = .;
  	*(.heap2)
  } > ram2
  /* Just keep track of where the stack block lives */
  .stack (__cs3_stack - __cs3_stack_size) (NOLOAD):
  {
    *(.stack)
    _estack = .;
    PROVIDE(estack = .);
  } >ram

  __cs3_region_init_ram = LOADADDR (.data);
  __cs3_region_init_size_ram = _edata - __cs3_region_start_ram;
  __cs3_region_zero_size_ram = _end - _edata;
  __cs3_region_size_ram = LENGTH(ram);
  __cs3_region_num = 1;

  .stab 0 (NOLOAD) : { *(.stab) }
  .stabstr 0 (NOLOAD) : { *(.stabstr) }
  /* DWARF debug sections.
   * Symbols in the DWARF debugging sections are relative to the beginning
   * of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }

  .note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) }
  .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) }
  /DISCARD/ : { *(.note.GNU-stack) }
}
