ENTRY(_start)

SECTIONS
{
    . = 0x84000000;
    .text : {
        *(*.text)
    }

    .rodata : {
        *(*.rodata)
    }

    .data : {
        *(*.data)
    }

    .bss : {
        PROVIDE(BSS_START = .);
        *(*.bss)
        PROVIDE(BSS_END = .);
    }
}
