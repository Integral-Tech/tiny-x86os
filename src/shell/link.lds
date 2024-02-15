/*
 * SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

ENTRY(_start)

SECTIONS
{
    . = 0x81000000;
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
