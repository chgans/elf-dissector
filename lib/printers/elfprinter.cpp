/*
    Copyright (C) 2015 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "elfprinter.h"
#include "printerutils_p.h"

#include <elf.h>

#include <QByteArray>
#include <QList>

static const LookupTableEntry<uint16_t> file_type_table[] {
    { ET_NONE, "none" },
    { ET_REL, "relocatable file" },
    { ET_EXEC, "executable file" },
    { ET_DYN, "shared object file" },
    { ET_CORE, "core file" }
};

QByteArray ElfPrinter::fileType(uint16_t fileType)
{
    return lookupLabel(fileType, file_type_table);
}

QByteArray ElfPrinter::machine(uint16_t machineType)
{
    #define M(x) case EM_ ## x: return QByteArray::fromRawData(#x, strlen(#x));
    switch (machineType) {
        M(NONE)
        M(386)
        M(ARM)
        M(X86_64)
        M(AVR)
        M(AARCH64)
    }
    return QByteArray("Unknown machine type (" ) + QByteArray::number(machineType) + ')';
}

static const LookupTableEntry<uint32_t> section_type_table[] {
    { SHT_NULL, "null" },
    { SHT_PROGBITS, "program data" },
    { SHT_SYMTAB, "symbol table" },
    { SHT_STRTAB, "string table" },
    { SHT_RELA, "relocation entries with addends" },
    { SHT_HASH, "symbol hash table" },
    { SHT_DYNAMIC, "dynamic linking information" },
    { SHT_NOTE, "notes" },
    { SHT_NOBITS, "bss" },
    { SHT_REL, "relocation entries, no addends" },
    { SHT_SHLIB, "reserved" },
    { SHT_DYNSYM, "dynamic linker symbol table" },
    { SHT_INIT_ARRAY, "array of constructors" },
    { SHT_FINI_ARRAY, "array of destructors" },
    { SHT_PREINIT_ARRAY, "array of preconstructors" },
    { SHT_GROUP, "section group" },
    { SHT_SYMTAB_SHNDX, "extended section indices" },

    { SHT_GNU_ATTRIBUTES, "GNU object attributes" },
    { SHT_GNU_HASH, "GNU-style hash table" },
    { SHT_GNU_LIBLIST, "GNU prelink library list" },
    { SHT_CHECKSUM, "checksum for DSO conent" },
    { SHT_GNU_verdef, "GNU version definition" },
    { SHT_GNU_verneed, "GNU version needs" },
    { SHT_GNU_versym, "GNU version symbol table" }
};

QByteArray ElfPrinter::sectionType(uint32_t sectionType)
{
    return lookupLabel(sectionType, section_type_table);
}

QByteArray ElfPrinter::sectionFlags(uint64_t flags)
{
    QByteArrayList s;
    if (flags & SHF_WRITE) s.push_back("writable");
    if (flags & SHF_ALLOC) s.push_back("occupies memory during execution");
    if (flags & SHF_EXECINSTR) s.push_back("executable");
    if (flags & SHF_MERGE) s.push_back("might be merged");
    if (flags & SHF_STRINGS) s.push_back("contains nul-terminated strings");
    if (flags & SHF_INFO_LINK) s.push_back("sh_info contains SHT index");
    if (flags & SHF_LINK_ORDER) s.push_back("preserve order after combining");
    if (flags & SHF_OS_NONCONFORMING) s.push_back("non-standard OS specific handling required");
    if (flags & SHF_GROUP) s.push_back("group member");
    if (flags & SHF_TLS) s.push_back("holds thread-local data");
    if (s.isEmpty())
        return "none";
    return s.join(", ");
}
