// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOADER_H
#define LOADER_H

#include "comm/boot_info.h"
#include "comm/cpu_instr.h"

void protected_mode_entry();

typedef struct SMAP_entry
{
    uint32_t BaseL; // base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL; // length uint64_t
    uint32_t LengthH;
    uint32_t Type; // entry type, when Type=1 -> it's the available RAM
    uint32_t ACPI; // extended, when bit0=0 -> this item should be ignored
} __attribute__((packed)) SMAP_entry_t;

extern boot_info_t boot_info;

#endif
