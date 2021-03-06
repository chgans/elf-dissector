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

#include "sectionproxymodel.h"
#include "elfmodel.h"

SectionProxyModel::SectionProxyModel(QObject* parent): KRecursiveFilterProxyModel(parent)
{
}

SectionProxyModel::~SectionProxyModel() = default;

bool SectionProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    const auto sourceIndex = sourceModel()->index(source_row, 0, source_parent);
    const auto fileData = sourceModel()->data(sourceIndex, ElfModel::FileRole);
    const auto secData = sourceModel()->data(sourceIndex, ElfModel::SectionRole);
    if (fileData.isNull() && secData.isNull())
        return false;
    return KRecursiveFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
