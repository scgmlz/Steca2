// ************************************************************************** //
//
//  Steca2: stress and texture calculator
//
//! @file      gui/widgets/tree_views.h
//! @brief     Defines classes ListView, MultiListView
//!
//! @homepage  https://github.com/scgmlz/Steca2
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#ifndef TREE_VIEWS_H
#define TREE_VIEWS_H

#include "typ/vec.h"
#include <QTreeView>

class TableModel;


//! abstract tree widget

class TreeView : public QTreeView {
public:
    TreeView();

    int sizeHintForColumn(int) const; // make narrow columns
};

//! A (tree-)list view with a reference to the hub. Single selection.

class ListView : public TreeView {
public:
    ListView() {}

    void setModel(TableModel* model);

protected:
    TableModel* model() const;

    void updateSingleSelection();
    void selectRow(int);
};


//! A (tree-)list view with a reference to the hub. Multiple selection.

class MultiListView : public ListView {
public:
    MultiListView();

protected:
    void selectRows(uint_vec);
};

#endif // TREE_VIEWS_H
