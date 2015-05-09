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

#include "typemodel.h"

#include <navigator/codenavigatorprinter.h>
#include <elf/elffileset.h>
#include <dwarf/dwarfinfo.h>
#include <dwarf/dwarfdie.h>
#include <printers/dwarfprinter.h>
#include <checks/structurepackingcheck.h>

#include <libdwarf/dwarf.h>

#include <QIcon>
#include <QTime>

TypeModel::TypeModel(QObject* parent): QAbstractItemModel(parent)
{
    setFileSet(nullptr);
}

TypeModel::~TypeModel() = default;

void TypeModel::setFileSet(ElfFileSet* fileSet)
{
    QTime t;
    t.start();
    beginResetModel();
    const auto l = [](TypeModel* m) { m->endResetModel(); };
    const auto endReset = std::unique_ptr<TypeModel, decltype(l)>(this, l);

    m_fileSet = fileSet;
    m_childMap.clear();
    m_parentMap.clear();
    m_nodes.clear();
    m_nodes.resize(1);
    m_childMap.resize(1);

    if (!fileSet)
        return;

    for (int i = 0; i < fileSet->size(); ++i)
        addFile(fileSet->file(i));

    qDebug() << "Found" << m_nodes.size() << "types, took" << t.elapsed() << "ms.";
}

void TypeModel::addFile(ElfFile* file)
{
    const auto dwarf = file->dwarfInfo();
    if (!dwarf)
        return;

    for (const auto cu : dwarf->compilationUnits()) {
        for (const auto die : cu->children())
            addDwarfDieRecursive(die, 0);
    }
}

bool dieInherits(DwarfDie *parentDie, DwarfDie *childDie)
{
    if (parentDie == childDie)
        return true;
    DwarfDie *baseDie = childDie->inheritedFrom();
    if (!baseDie)
        return false;
    return dieInherits(parentDie, baseDie);
}

bool isBetterDie(DwarfDie *prevDie, DwarfDie *newDie)
{
    // we don't care about increasing the level of detail for structure nodes
    if (prevDie->tag() != DW_TAG_class_type && prevDie->tag() != DW_TAG_structure_type)
        return false;

    // declarations are always worse then the real one
    if (prevDie->attribute(DW_AT_declaration).toBool())
        return true;

    // size is also a good indicator for this belonging to a complete DIE
    if (prevDie->typeSize() == 0)
        return true;

    // walk down the inheritance tree
    if (dieInherits(prevDie, newDie))
        return true;

    // if we have member children, that's better
    // TODO

    // TODO what else?

    return false;
}

void TypeModel::addDwarfDieRecursive(DwarfDie* die, uint32_t parentId)
{
    switch (die->tag()) {
        case DW_TAG_class_type:
        case DW_TAG_namespace:
        case DW_TAG_structure_type: // TODO we can also have nested types in DW_TAG_subprograms!
            break;
        default:
            return;
    }

    auto& children = m_childMap[parentId];
    const auto dieName = die->name();
    const auto it = std::lower_bound(children.begin(), children.end(), die, [this, &dieName](uint32_t nodeId, DwarfDie *die) {
        const auto lhs = m_nodes.at(nodeId);
        if (lhs.die->tag() == die->tag())
            return m_nodes.at(nodeId).die->name() < dieName;
        return lhs.die->tag() < die->tag();
    });

    uint32_t nodeId;
    // TODO what about anon structs? name() is empty there
    // TODO what about local symbols, compare CUs?
    if (it != children.constEnd() && m_nodes.at(*it).die->tag() == die->tag() && m_nodes.at(*it).die->name() == dieName) {
        nodeId = *it;
        if (isBetterDie(m_nodes.at(nodeId).die, die))
            m_nodes[nodeId].die = die;
    } else {
        nodeId = m_nodes.size();
        m_nodes.resize(nodeId + 1);
        m_nodes[nodeId].die = die;
        children.insert(it, nodeId);
        m_parentMap.resize(nodeId + 1);
        m_parentMap[nodeId] = parentId;
        m_childMap.resize(nodeId + 1);
    }

    // TODO suppress empty structure nodes
    for (auto child : die->children())
        addDwarfDieRecursive(child, nodeId);
}

int TypeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 2;
}

int TypeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;
    const uint32_t parentId = parent.internalId();
    return m_childMap.at(parentId).size();
}

QVariant TypeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    const auto node = m_nodes.at(index.internalId());
    switch (role) {
        case Qt::DisplayRole:
            if (index.column() == 0)
                return node.die->name();
            else if (index.column() == 1 && (node.die->tag() == DW_TAG_class_type || node.die->tag() == DW_TAG_structure_type))
                return node.die->typeSize();
            return {};
        case TypeModel::DetailRole:
        {
            QString s = DwarfPrinter::dieRichText(node.die);
            s += CodeNavigatorPrinter::sourceLocationRichText(node.die);

            if ((node.die->tag() == DW_TAG_structure_type || node.die->tag() == DW_TAG_class_type) && node.die->typeSize() > 0) {
                s += "<tt><pre>";
                StructurePackingCheck check;
                check.setElfFileSet(m_fileSet);
                s += check.checkOneStructure(node.die).toHtmlEscaped();
                s += "</pre></tt><br/>";
            }

            return s;
        }
        case Qt::DecorationRole:
            if (index.column() != 0)
                return {};
            switch (node.die->tag()) {
                case DW_TAG_namespace:
                    return QIcon::fromTheme("code-context");
                case DW_TAG_class_type:
                case DW_TAG_structure_type:
                    return QIcon::fromTheme("code-class");
            }
            return {};
    };

    return {};
}

QModelIndex TypeModel::index(int row, int column, const QModelIndex& parent) const
{
    const int32_t parentId = parent.internalId();
    if (!hasIndex(row, column, parent))
        return {};

    return createIndex(row, column, m_childMap.at(parentId).at(row));
}

QModelIndex TypeModel::parent(const QModelIndex& child) const
{
    const int32_t childId = child.internalId();
    if (childId == 0)
        return QModelIndex();
    const int32_t parentId = m_parentMap.at(childId);
    if (parentId == 0)
        return QModelIndex();
    const int32_t grandParentId = m_parentMap.at(parentId);
    const int row = m_childMap.at(grandParentId).indexOf(parentId);

    return createIndex(row, 0, parentId);
}

QVariant TypeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Data Type");
            case 1: return tr("Size");
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}