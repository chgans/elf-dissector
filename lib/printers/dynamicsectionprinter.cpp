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

#include "dynamicsectionprinter.h"

#include <QByteArray>
#include <QList>

#include <elf.h>

struct FlagInfo
{
    uint64_t flag;
    const char *desc;
};

static const FlagInfo dt_flag_infos[] {
    { DF_ORIGIN, "object may use DF_ORIGIN" },
    { DF_SYMBOLIC, "symbol resolutions starts here" },
    { DF_TEXTREL, "object contains text relocations" },
    { DF_BIND_NOW, "no lazy binding for this object" },
    { DF_STATIC_TLS, "module uses the static TLS model" }
};

static const int dt_flag_infos_size = sizeof(dt_flag_infos) / sizeof(FlagInfo);

static const FlagInfo dt_flag_1_infos[] {
    { DF_1_NOW, "set RTLD_NOW for this object" },
    { DF_1_GLOBAL, "set RTLD_GLOBAL for this object" },
    { DF_1_GROUP, "set RTLD_GROUP for this object" },
    { DF_1_NODELETE, "set RTLD_NODELETE for this object" },
    { DF_1_LOADFLTR, "trigger filtee loading at runtime" },
    { DF_1_INITFIRST, "set RTLD_INITFIRST for this object" },
    { DF_1_NOOPEN, "set RTLD_NOOPEN for this object" },
    { DF_1_ORIGIN, "$ORIGIN must be handled" },
    { DF_1_DIRECT, "direct binding enabled" },
    { DF_1_TRANS, "DF_1_TRANS" },
    { DF_1_INTERPOSE, "object is used to interpose" },
    { DF_1_NODEFLIB, "ignore default lib search path" },
    { DF_1_NODUMP, "object can't be dldump'ed" },
    { DF_1_CONFALT, "configuration alternative created" },
    { DF_1_ENDFILTEE, "filtee terminates filters search" },
    { DF_1_DISPRELDNE, "disp reloc applied at build time" },
    { DF_1_DISPRELPND, "disp reloc applied at run-time" },
    { DF_1_NODIRECT, "object has no-direct binding" },
    { DF_1_IGNMULDEF, "DF_1_IGNMULDEF" },
    { DF_1_NOKSYMS, "DF_1_NOKSYMS" },
    { DF_1_NOHDR, "DF_!_NOHDR" },
    { DF_1_EDITED, "Object is modified after built" },
    { DF_1_NORELOC, "DF_1_NORELOC" },
    { DF_1_SYMINTPOSE, "Object has individual interposers" },
    { DF_1_GLOBAUDIT, "Global auditing required" },
    { DF_1_SINGLETON, "Singleton symbols are used" }
};

static const int dt_flag_1_infos_size = sizeof(dt_flag_1_infos) / sizeof(FlagInfo);

static QByteArray dtFlagsToString(uint64_t flags, const FlagInfo* infos, int size)
{
    QList<QByteArray> l;
    uint64_t handledFlags = 0;

    for (int i = 0; i < size; ++i) {
        if (flags & infos[i].flag)
            l.push_back(QByteArray::fromRawData(infos[i].desc, strlen(infos[i].desc)));
        handledFlags |= infos[i].flag;
    }

    if (flags & ~handledFlags)
        l.push_back(QByteArray("unhandled flags 0x") + QByteArray::number(qulonglong(flags & ~handledFlags), 16));

    if (l.isEmpty())
        return "<none>";
    return l.join(", ");
}


namespace DynamicSectionPrinter {

QByteArray flagsToDescriptions(uint64_t flags)
{
    return dtFlagsToString(flags, dt_flag_infos, dt_flag_infos_size);
}

QByteArray flags1ToDescriptions(uint64_t flags)
{
    return dtFlagsToString(flags, dt_flag_1_infos, dt_flag_1_infos_size);
}

}