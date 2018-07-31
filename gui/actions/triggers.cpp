//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      gui/actions/triggers.cpp
//! @brief     Implements class Triggers
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#include "triggers.h"
#include "core/session.h"
#include "gui/dialogs/message_boxes.h"
#include "gui/dialogs/check_update.h"
#include "gui/dialogs/export_bigtable.h"
#include "gui/dialogs/export_dfgram.h"
#include "gui/dialogs/export_diagram.h"
#include "gui/dialogs/export_polefig.h"
#include "gui/dialogs/popup_bigtable.h"
#include "gui/dialogs/popup_diagram.h"
#include "gui/dialogs/popup_polefig.h"
#include "gui/mainwin.h"
#include "manifest.h"
#include <QDesktopServices>

Triggers::Triggers()
{
#define AT &QAction::triggered
    connect(&about, AT, [](){ AboutBox().exec(); });
    connect(&baserangeAdd, &QAction::triggered, [](){ AddRangeBox("baseline").exec(); });
    connect(&peakAdd, &QAction::triggered, [](){ AddRangeBox("peak").exec(); });
    connect(&addFiles, AT, []() { gGui->addFiles(); });
    connect(&checkUpdate, AT, []() { CheckUpdate _(gGui); });
    connect(&clearSession, AT, []() { gSession->clear(); });
    connect(&corrFile, AT, []() { gGui->loadCorrFile(); });
    connect(&exportDfgram, AT, [](){ ExportDfgram().exec(); });
    connect(&exportPolefig, AT, [](){ ExportPolefig().exec(); });
    connect(&exportBigtable, AT, [](){ ExportBigtable().exec(); });
    connect(&exportDiagram, AT, [](){ ExportDiagram().exec(); });
    connect(&loadSession, AT, []() { gGui->loadSession(); });
    connect(&online, AT, []() { QDesktopServices::openUrl(QUrl(STECA2_PAGES_URL)); });
    connect(&quit, AT, []() { gGui->deleteLater(); });
    connect(&removeFile, AT, []() { gSession->dataset.removeFile(); });
    connect(&saveSession, AT, []() { gGui->saveSession(); });
    connect(&spawnDiagram, AT, [](){ new PopupDiagram(); });
    connect(&spawnTable, AT, [](){ new PopupBigtable(); });
    connect(&spawnPolefig, AT, [](){ new PopupPolefig(); });
    connect(&viewReset, AT, []() { gGui->viewReset(); });
}
