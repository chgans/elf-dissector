/*
    Copyright (C) 2013-2014 Volker Krause <vkrause@kde.org>

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

#ifndef ELFFILE_H
#define ELFFILE_H

#include "elfsectionheader.h"
#include "elfsection.h"
#include "elfdynamicsection.h"

#include <QFile>
#include <QMetaType>
#include <QVector>

#include <memory>

class DwarfInfo;
class ElfHashSection;
class ElfHeader;
class ElfSymbolTableSection;

/** Represents a ELF file. */
class ElfFile
{
public:
    explicit ElfFile(const QString &fileName);
    ElfFile(const ElfFile &other) = delete;
    ~ElfFile();

    ElfFile& operator=(const ElfFile &other) = delete;

    /** Returns @c true if the file could be loaded and is parsed correctly. */
    bool isValid() const;

    /** Returns a user readable label for this file. */
    QString displayName() const;
    /** Returns the full path of this file. */
    QString fileName() const;
    /** Returns the size of the entire file. */
    uint64_t size() const;

    /** Returns a pointer to the raw ELF data. */
    const unsigned char* rawData() const;

    /** ELF class type (32/64 bit). */
    int type() const;
    /** Returns the numbers of bytes needed to store an address. */
    int addressSize() const;
    /** Endianess. */
    int byteOrder() const;
    /** OS ABI. */
    uint8_t osAbi() const;

    /** Returns the ELF header. */
    ElfHeader* header() const;
    /** Returns a list of all available section headers. */
    QVector<ElfSectionHeader*> sectionHeaders() const;
    /** Returns the section at index @p index. */
    template <typename T>
    inline T* section(int index) const
    {
        return dynamic_cast<T*>(m_sections.at(index));
    }
    /** Finds a section by type. */
    int indexOfSection(uint32_t type) const;
    /** Finds a section by name. */
    int indexOfSection(const char* name) const;

    /** Returns the dynamic section. */
    ElfDynamicSection* dynamicSection() const;
    /** Returns .symtab if present, .dynsym otherwise. */
    ElfSymbolTableSection* symbolTable() const;
    /** Returns a symbol table hash section for fast lookup. */
    ElfHashSection* hash() const;

    /** DWARF debug information, if present. */
    DwarfInfo* dwarfInfo() const;

private:
    void open(const QString& fileName);
    void close();
    void parse();
    void parseHeader();
    void parseSections();

private:
    QFile m_file;
    uchar *m_data;
    std::unique_ptr<ElfHeader> m_header;
    QVector<ElfSectionHeader*> m_sectionHeaders;
    QVector<ElfSection*> m_sections;
    ElfDynamicSection* m_dynamicSection = nullptr;
    DwarfInfo *m_dwarfInfo = nullptr;
};

Q_DECLARE_METATYPE(ElfFile*)

#endif // ELFFILE_H
