
//
//  libqcr: capture and replay Qt widget actions
//
//! @file      qcr/widgets/views.h
//! @brief     Defines enhanced view widgets QcrMainWindow, QcrWidget, QcrLineDisplay, and others
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2018-
//! @author    Joachim Wuttke
//
//  ***********************************************************************************************

#ifndef VIEWS_H
#define VIEWS_H

#include "qcr/engine/mixin.h"
#include <QDockWidget>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QStackedWidget>

class QcrMainWindow : public QMainWindow, public QcrRoot {
public:
    QcrMainWindow() : QcrRoot{*this, "mainwindow"} {}
};

class QcrWidget : public QWidget, public QcrMixin {
public:
QcrWidget() : QcrMixin(this) {}
};

class QcrFrame : public QFrame, public QcrMixin {
public:
    QcrFrame(const QString& name) : QcrMixin{*this, name} {}
};

class QcrStackedWidget : public QStackedWidget, public QcrMixin {
public:
    QcrStackedWidget(const QString& name) : QcrMixin{*this, name} {}
};

class QcrDockWidget : public QDockWidget, public QcrMixin {
public:
    QcrDockWidget(const QString& name) : QcrMixin{*this, name} {}
};

//! QLabel displaying an icon, with no associated action.
class QcrIcon : public QLabel, public QcrMixin {
public:
    QcrIcon(const QString& fileName);
};

//! Read-only QLineEdit for number display.
class QcrLineDisplay : public QLineEdit, public QcrMixin {
public:
    QcrLineDisplay() = delete;
    QcrLineDisplay(const QString& name, std::function<QString()> freshText);
    QcrLineDisplay(const QString& name, int ndigits, bool withDot);
};

#endif // VIEWS_H
