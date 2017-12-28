// ************************************************************************** //
//
//  Steca2: stress and texture calculator
//
//! @file      gui/panels/panel_diffractogram.h
//! @brief     Defines class Diffractogram
//!
//! @homepage  https://github.com/scgmlz/Steca2
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2017
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#ifndef PANEL_DIFFRACTOGRAM_H
#define PANEL_DIFFRACTOGRAM_H

#include "actions.h"
#include "panel.h"
#include "signalling.h" // for eFittingTab

namespace gui {

class TheHub;

namespace panel {

class Diffractogram : public QWidget {
public:
    Diffractogram(TheHub&);

    void render();

    data::shp_Dataset dataset() const { return dataset_; }
    QBoxLayout* box() const { return box_; }

private:
    TheHub& hub_;
    QBoxLayout* box_;
    void onNormChanged();
    void onFittingTab(eFittingTab tab);

    void setDataset(data::shp_Dataset);

    data::shp_Dataset dataset_;

    class DiffractogramPlot* plot_;

    typ::Curve dgram_, dgramBgFitted_, bg_;
    typ::curve_vec refls_;

    uint currReflIndex_;
    calc::shp_Reflection currentReflection_;

    QComboBox* comboNormType_;
    QRadioButton *intenSum_, *intenAvg_;
    QDoubleSpinBox* intenScale_;
    QToolButton* enableZoom_;
    Action* actZoom_;

public:
    void calcDgram();
    void calcBackground();
    void calcReflections();

    void setCurrReflNewRange(typ::Range const&);
    typ::Range currReflRange() const;
};

} // namespace panel
} // namespace gui

#endif // PANEL_DIFFRACTOGRAM_H
