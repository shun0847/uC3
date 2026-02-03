_MAP_CODE_START = (0x40800000);
_MAP_CODE_SIZE = ((0x40900000) - (0x40800000));
_MAP_RAM_DATA_START = (0x40900000);
_MAP_RAM_DATA_SIZE = ((0x41000000) - (0x40900000));
_MAP_RAM_DATA_NC_START = (0x41000000);
_MAP_RAM_DATA_NC_SIZE = ((0x41100000) - (0x41000000));
_MAP_SYS_DATA_START = (0x41100000);
_MAP_SYS_DATA_SIZE = ((0x41500000) - (0x41100000));
_MAP_MMU_TBL_START = (0x41500000);
_MAP_MMU_TBL_SIZE = ((0x41600000) - (0x41500000));
_MAP_STACK_START = (0x41600000);
_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x100000;
_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0x0;
_IRQ_STACK_SIZE = DEFINED(_IRQ_STACK_SIZE) ? _IRQ_STACK_SIZE : 0x100000;
_EL0_STACK_SIZE = _STACK_SIZE;
_EL1_STACK_SIZE = _IRQ_STACK_SIZE;
MEMORY
{
    MAP_CODE : ORIGIN = _MAP_CODE_START, LENGTH = _MAP_CODE_SIZE
    MAP_RAM_DATA : ORIGIN = _MAP_RAM_DATA_START, LENGTH = _MAP_RAM_DATA_SIZE
    MAP_RAM_NC : ORIGIN = _MAP_RAM_DATA_NC_START, LENGTH = _MAP_RAM_DATA_NC_SIZE
    MAP_SYS_DATA : ORIGIN = _MAP_SYS_DATA_START, LENGTH = _MAP_SYS_DATA_SIZE
    MAP_MMU_TBL : ORIGIN = _MAP_MMU_TBL_START, LENGTH = _MAP_MMU_TBL_SIZE
    MAP_STACK : ORIGIN = _MAP_STACK_START, LENGTH = (_STACK_SIZE + _IRQ_STACK_SIZE)
}
ENTRY(_RPST)
SECTIONS
{
.vectors : {
    *(.prst)
    . = ALIGN(2048);
    KEEP (*(.vectors))
    . = ALIGN(2048);
    KEEP (*(.vectors0))
} > MAP_CODE
.text : {
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
    *(.plt)
    *(.gnu_warning)
    *(.gcc_execpt_table)
    *(.glue_7)
    *(.glue_7t)
    *(.vfp11_veneer)
    *(.ARM.extab)
    *(.gnu.linkonce.armextab.*)
} > MAP_CODE
.init (ALIGN(64)) : {
    KEEP (*(.init))
} > MAP_CODE
.fini (ALIGN(64)) : {
    KEEP (*(.fini))
} > MAP_CODE
.interp : {
    KEEP (*(.interp))
} > MAP_CODE
.note-ABI-tag : {
    KEEP (*(.note-ABI-tag))
} > MAP_CODE
.rodata : {
    . = ALIGN(64);
    __rodata_start = .;
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    __rodata_end = .;
} > MAP_CODE
.data : {
    . = ALIGN(64);
    __data_start = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    *(.jcr)
    *(.got)
    *(.got.plt)
    __data_end = .;
} > MAP_CODE
.got : {
    *(.got)
} > MAP_CODE
.ctors : {
    . = ALIGN(64);
    __CTOR_LIST__ = .;
    ___CTORS_LIST___ = .;
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE(*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    __CTOR_END__ = .;
    ___CTORS_END___ = .;
} > MAP_CODE
.dtors : {
    . = ALIGN(64);
    __DTOR_LIST__ = .;
    ___DTORS_LIST___ = .;
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE(*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    __DTOR_END__ = .;
    ___DTORS_END___ = .;
} > MAP_CODE
.fixup : {
    __fixup_start = .;
    *(.fixup)
    __fixup_end = .;
} > MAP_CODE
.eh_frame : {
    *(.eh_frame)
} > MAP_CODE
.eh_framehdr : {
    __eh_framehdr_start = .;
    *(.eh_framehdr)
    __eh_framehdr_end = .;
} > MAP_CODE
.gcc_except_table : {
    *(.gcc_except_table)
} > MAP_CODE
.ARM.exidx : {
    __exidx_start = .;
    *(.ARM.exidx*)
    *(.gnu.linkonce.armexidix.*.*)
    __exidx_end = .;
} > MAP_CODE
.preinit_array : {
    . = ALIGN(64);
    __preinit_array_start = .;
    KEEP (*(SORT(.preinit_array.*)))
    KEEP (*(.preinit_array))
    __preinit_array_end = .;
} > MAP_CODE
.init_array : {
    . = ALIGN(64);
    __init_array_start = .;
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    __init_array_end = .;
} > MAP_CODE
.fini_array : {
    . = ALIGN(64);
    __fini_array_start = .;
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array))
    __fini_array_end = .;
} > MAP_CODE
.ARM.attributes : {
    __ARM.attributes_start = .;
    *(.ARM.attributes)
    __ARM.attributes_end = .;
} > MAP_CODE
.uncache (NOLOAD) : {
    . = ALIGN(0x20);
    *(.uncache)
} > MAP_RAM_NC
.sdata : {
    . = ALIGN(64);
    __sdata_start = .;
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    __sdata_end = .;
} > MAP_RAM_DATA
.sbss (NOLOAD) : {
    . = ALIGN(64);
    __sbss_start = .;
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    . = ALIGN(64);
    __sbss_end = .;
} > MAP_RAM_DATA
.tdata : {
    . = ALIGN(64);
    __tdata_start = .;
    *(.tdata)
    *(.tdata.*)
    *(.gnu.linkonce.td.*)
    __tdata_end = .;
} > MAP_RAM_DATA
.tbss : {
    . = ALIGN(64);
    __tbss_start = .;
    *(.tbss)
    *(.tbss.*)
    *(.gnu.linkonce.tb.*)
    __tbss_end = .;
} > MAP_RAM_DATA
.bss (NOLOAD) : {
    . = ALIGN(64);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(64);
    __bss_end__ = .;
} > MAP_RAM_DATA
.heap (NOLOAD) : {
   . = ALIGN(64);
   HeapBase = .;
   __heap_start__ = .;
   end = __heap_start__;
   _end = end;
   __end = end;
   . += _HEAP_SIZE;
   _heap_end = .;
   HeapLimit = .;
} > MAP_RAM_DATA
.noinit (NOLOAD) : {
    . = ALIGN(64);
    __noinit_start__ = .;
    *(.noinit)
    __noinit_end__ = .;
} > MAP_RAM_DATA
.stack (NOLOAD) : {
    . = ALIGN(64);
    _el1_stack_end = .;
    . += _EL1_STACK_SIZE;
    . = ALIGN(64);
    __el1_stack = .;
    _el0_stack_end = .;
    . += _EL0_STACK_SIZE;
    . = ALIGN(64);
    __el0_stack = .;
    _stack = .;
    . = ALIGN(16);
} > MAP_STACK
.system (NOLOAD) :
{
    __sys_start = .;
    __vinftbl_begin = .;
    *(.vinftbl)
    __vinftbl_end = .;
    . = ALIGN(16);
    *(.sys)
    . = ALIGN(16);
    *(.sysmem)
    . = ALIGN(16);
    *(.stkmem)
    . = ALIGN(16);
    *(.mplmem)
    . = ALIGN(4);
    __sys_end = .;
} > MAP_SYS_DATA
.mmu_tbl :
{
    . = ALIGN(0x4000);
    __mmu_tbl_start = .;
    *(.mmu_tbl)
    __mmu_tbl_end = .;
} > MAP_MMU_TBL
_end = .;
}
