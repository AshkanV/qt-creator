/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/fancylineedit.h>

#include <QStyledItemDelegate>

namespace Squish {
namespace Internal {

class PropertyItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    PropertyItemDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

class ValidatingPropertyNameLineEdit : public Utils::FancyLineEdit
{
    Q_OBJECT
public:
    ValidatingPropertyNameLineEdit(const QStringList &forbidden, QWidget *parent = nullptr);

private:
    QStringList m_forbidden;
};

class ValidatingPropertyContainerLineEdit : public Utils::FancyLineEdit
{
    Q_OBJECT
public:
    ValidatingPropertyContainerLineEdit(const QStringList &allowed, QWidget *parent = nullptr);

private:
    QStringList m_allowed;
};

} // namespace Internal
} // namespace Squish