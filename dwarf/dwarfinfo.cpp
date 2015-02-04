/*
    Copyright (C) 2014-2015 Volker Krause <vkrause@kde.org>

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

#include "dwarfinfo.h"
#include "dwarfdie.h"

#include <QDebug>

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

class DwarfInfoPrivate {
public:
    DwarfInfoPrivate(DwarfInfo* qq);
    ~DwarfInfoPrivate();

    void scanCompilationUnits();
    QString sourceLocationForMangledSymbolRecursive(const QByteArray &symbol, DwarfDie *die) const;

    ElfFile *elfFile = nullptr;
    QVector<DwarfDie*> compilationUnits;
    Dwarf_Obj_Access_Interface objAccessIface;
    Dwarf_Obj_Access_Methods objAccessMethods;

    Dwarf_Debug dbg;

    DwarfInfo *q;
};


static Dwarf_Endianness callback_get_byte_order(void *obj)
{
    qDebug();
    return DW_OBJECT_LSB;
}

static Dwarf_Small callback_get_length_size(void *obj)
{
    qDebug();
    return 8;
}

static Dwarf_Small callback_get_pointer_size(void *obj)
{
    qDebug();
    return 8;
}

static Dwarf_Unsigned callback_get_section_count(void *obj)
{
    const DwarfInfoPrivate *d = reinterpret_cast<DwarfInfoPrivate*>(obj);
    return d->elfFile->sectionHeaders().size();
}

static int callback_get_section_info(void *obj, Dwarf_Half index, Dwarf_Obj_Access_Section *sectionInfo, int *error)
{
    const DwarfInfoPrivate *d = reinterpret_cast<DwarfInfoPrivate*>(obj);
    const ElfSectionHeader::Ptr sectionHeader = d->elfFile->sectionHeaders().at(index);
    sectionInfo->addr = (Dwarf_Addr)(d->elfFile->rawData() + sectionHeader->sectionOffset());
    sectionInfo->size = sectionHeader->size();
    sectionInfo->name = sectionHeader->name();
    qDebug() << index;
    *error = DW_DLV_OK;
    return DW_DLV_OK;
}

static int callback_load_section(void *obj, Dwarf_Half index, Dwarf_Small **returnData, int *error)
{
    qDebug() << index;
    const DwarfInfoPrivate *d = reinterpret_cast<DwarfInfoPrivate*>(obj);
    const ElfSectionHeader::Ptr sectionHeader = d->elfFile->sectionHeaders().at(index);
    *returnData = const_cast<Dwarf_Small*>(d->elfFile->rawData() + sectionHeader->sectionOffset());
    *error = DW_DLV_OK;
    return DW_DLV_OK;
}


DwarfInfoPrivate::DwarfInfoPrivate(DwarfInfo *qq) :
    q(qq)
{
    objAccessIface.object = this;
    objAccessIface.methods = &objAccessMethods;
    objAccessMethods.get_byte_order = callback_get_byte_order;
    objAccessMethods.get_length_size = callback_get_length_size;
    objAccessMethods.get_pointer_size = callback_get_pointer_size;
    objAccessMethods.get_section_count = callback_get_section_count;
    objAccessMethods.get_section_info = callback_get_section_info;
    objAccessMethods.load_section = callback_load_section;
    objAccessMethods.relocate_a_section = 0;
}

DwarfInfoPrivate::~DwarfInfoPrivate()
{
    qDeleteAll(compilationUnits);
}

void DwarfInfoPrivate::scanCompilationUnits()
{
    Dwarf_Unsigned nextHeader = 0;
    forever {
        auto res = dwarf_next_cu_header(dbg, 0, 0, 0, 0, &nextHeader, 0);
        if (res != DW_DLV_OK)
            return;

        Dwarf_Die cuDie = nullptr;
        res = dwarf_siblingof(dbg, 0, &cuDie, 0);
        if(res != DW_DLV_OK)
            return;

        compilationUnits.push_back(new DwarfDie(cuDie, q));
    }
}


QString DwarfInfoPrivate::sourceLocationForMangledSymbolRecursive(const QByteArray& symbol, DwarfDie *die) const
{
    if (die->attribute(DW_AT_linkage_name).toByteArray() == symbol)
        return die->attribute(DW_AT_decl_file).toString() + ":" + die->attribute(DW_AT_decl_line).toString();
    for (auto childDie : die->children()) {
        const auto loc = sourceLocationForMangledSymbolRecursive(symbol, childDie);
        if (!loc.isEmpty())
            return loc;
    }
    return {};
}

DwarfInfo::DwarfInfo(ElfFile* elfFile) :
    d(new DwarfInfoPrivate(this))
{
    d->elfFile = elfFile;

    if (dwarf_object_init(&d->objAccessIface, 0, 0, &d->dbg, 0) != DW_DLV_OK) {
        qDebug() << "error loading dwarf data";
    }
    qDebug() << "dwarf data loaded";
}

DwarfInfo::~DwarfInfo()
{
    dwarf_object_finish(d->dbg, 0);

    delete d;
}

ElfFile* DwarfInfo::elfFile() const
{
    return d->elfFile;
}

Dwarf_Debug DwarfInfo::dwarfHandle() const
{
    return d->dbg;
}

QVector< DwarfDie* > DwarfInfo::compilationUnits() const
{
    if (d->compilationUnits.isEmpty())
        d->scanCompilationUnits();
    return d->compilationUnits;
}

DwarfDie* DwarfInfo::dieAtOffset(Dwarf_Off offset) const
{
    const auto cus = compilationUnits();
    auto it = std::lower_bound(cus.begin(), cus.end(), offset, [](DwarfDie* lhs, Dwarf_Off rhs) { return lhs->offset() < rhs; });

    if (it != cus.end() && (*it)->offset() == offset)
        return *it;

    Q_ASSERT(it != cus.begin());
    --it;
    return (*it)->dieAtOffset(offset);
}

QString DwarfInfo::sourceLocationForMangledSymbol(const QByteArray& symbol) const
{
    for (auto die : compilationUnits()) {
        const auto loc = d->sourceLocationForMangledSymbolRecursive(symbol, die);
        if (!loc.isEmpty())
            return loc;
    }
    return {};
}