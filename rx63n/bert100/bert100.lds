OUTPUT_ARCH(rx)
SECTIONS
{
	.fvectors 0xFFFFFF80 : AT (0xFFFFFF80)
	{
		KEEP(*(.fvectors))
	}
	.jmptable 0xFFFFeF80 : AT (0xFFFFeF80)
        {
                KEEP(*(.jmptable))
        }
	.bootloader 0x16000: AT (0xFFFFF004) /* For bytes are for CRC32 */
        {
                _bootloader_start = .;
                *(.bootloader)
                *(.bootloader.*)
                _bootloader_end = .;
        }
        .bootloader_flash LOADADDR(.bootloader) + SIZEOF (.bootloader): AT(LOADADDR(.bootloader) + SIZEOF (.bootloader))
        {
                *(.bootloader_flash)
                *(.bootloader_flash.*)
        }
	.text 0xFFF80000 : AT (0xFFF80000)
	{
		*(.text)
		*(.text.*)
		*(P)
		etext = .;
	}
	.rvectors :
	{
		_rvectors_start = .;
		KEEP(*(.rvectors))
		_rvectors_end = .;
	}
	.init :
	{
		*(.init)
		__preinit_array_start = .;
		*(.preinit_array)
		__preinit_array_end = .;
		__init_array_start = (. + 3) & ~ 3;
		*(.init_array)
		*(SORT(.init_array.*))
		__init_array_end = .;
		__fini_array_start = .;
		*(.fini_array)
		*(SORT(.fini_array.*))
		__fini_array_end = .;
	}
	.fini :
	{
		*(.fini)
	}
	.got :
	{
		*(.got)
		*(.got.plt)
	}
	.rodata :
	{
		*(.rodata)
		*(.rodata.*)
		*(C_1)
		*(C_2)
		*(C)
		_erodata = .;
	}
	.eh_frame_hdr :
	{
		*(.eh_frame_hdr)
	}
	.eh_frame :
	{
		*(.eh_frame)
	}
	.jcr :
	{
		*(.jcr)
	}
	.tors :
	{
		__CTOR_LIST__ = .;
		___ctors = .;
		*(.ctors)
		___ctors_end = .;
		__CTOR_END__ = .;
		__DTOR_LIST__ = .;
		___dtors = .;
		*(.dtors)
		___dtors_end = .;
		__DTOR_END__ = .;
		_mdata = .;
	}
	.data 0x00001001 : AT (_mdata)
	{
		_data = .;
		*(.data)
		*(.data.*)
		*(D)
		*(D_1)
		*(D_2)
		_edata = .;
	}
	.gcc_exc :
	{
		*(.gcc_exc)
	}
	.bss :
	{
		_bss = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		*(B)
		*(B_1)
		*(B_2)
		_ebss = .;
		_end = .;
	}
}
