// ************************************************************************** //
//
//  Steca: stress and texture calculator
//
//! @file      gui/panels/mainframe.cpp
//! @brief     Implements class SubframeImage
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#include "mainframe.h"
#include "core/session.h"
#include "gui/panels/tab_image.h"
#include "gui/panels/tab_table.h"
#include "gui/panels/tab_diagram.h"
#include "gui/panels/tab_polefig.h"

// ************************************************************************** //
//  class Mainframe
// ************************************************************************** //

Mainframe::Mainframe()
    : CTabWidget {"images"}
{
    // local connection
    connect(this, &Mainframe::currentChanged, [this](int) { render(); });

    // inbound connections
    connect(gSession, &Session::sigImage, [this]() { render(); });
    connect(gSession, &Session::sigCorr, [this]() {
            setTabEnabled(1, gSession->corrset().hasFile()); });

    // layout
    setTabPosition(QTabWidget::North);
    addTab((dataImageTab_ = new DataImageTab), "Data image");
    addTab((corrImageTab_ = new CorrImageTab), "Corr image");
    addTab((tableWidget_ = new TableWidget), "Table");
    addTab((diagramWidget_ = new DiagramWidget), "Diagram");
    addTab((polefigWidget_ = new PolefigWidget), "Polefig");
    setTabEnabled(1, false);
}

void Mainframe::render()
{
    dynamic_cast<TabMainframe*>(currentWidget())->render();
}
