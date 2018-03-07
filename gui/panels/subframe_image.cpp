// ************************************************************************** //
//
//  Steca: stress and texture calculator
//
//! @file      gui/panels/subframe_image.cpp
//! @brief     Implements class SubframeImage
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#include "subframe_image.h"
#include "core/session.h"
#include "gui/cfg/colors.h"
#include "gui/mainwin.h"
#include "gui/actions/toggles.h"
#include "gui/actions/triggers.h"
#include <qmath.h>
#include <QPainter>

// ************************************************************************** //
//  local class ImageWidget
// ************************************************************************** //

//! Displays a 2d detector image, and possibly some overlay. Used in ImageTab.

class ImageWidget final : public QWidget {
public:
    ImageWidget();

    void setPixmap(const QPixmap&);
    void setScale();

private:
    void resizeEvent(QResizeEvent*);
    void paintEvent(QPaintEvent*);

    qreal scale_;
    QPixmap original_, scaled_;
};


ImageWidget::ImageWidget() : scale_(0) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(&gGui->toggles->showOverlay, &QAction::toggled, [this](bool /*unused*/) { update(); });
    connect(&gGui->toggles->stepScale, &QAction::toggled, [this](bool /*unused*/) { setScale(); });
}

void ImageWidget::setPixmap(const QPixmap& pixmap) {
    original_ = pixmap;
    setScale();
}

void ImageWidget::setScale() {
    if (original_.isNull()) {
        scale_ = 0;
    } else {
        const QSize& sz = size();
        const QSize& os = original_.size();
        scale_ = qMin(qreal(sz.width() - 2) / os.width(), qreal(sz.height() - 2) / os.height());
    }

    if (gGui->toggles->stepScale.isChecked() && scale_ > 0)
        scale_ = (scale_ >= 1) ? qFloor(scale_) : 1.0 / qCeil(1.0 / scale_);

    if (original_.isNull() || !(scale_ > 0))
        scaled_ = QPixmap();
    else
        scaled_ = original_.scaled(original_.size() * scale_);

    update();
}

void ImageWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    setScale();
}

void ImageWidget::paintEvent(QPaintEvent*) {
    // paint centered
    const QSize margin = (size() - scaled_.size()) / 2;
    const QRect rect(QPoint(margin.width(), margin.height()), scaled_.size());

    QPainter p(this);

    // image
    p.drawPixmap(rect.left(), rect.top(), scaled_);

    // overlay
    if (gGui->toggles->showOverlay.isChecked()) {
        p.setPen(Qt::lightGray);

        // cut
        const ImageCut& cut = gSession->imageCut();
        const QRect r = rect.adjusted(-1, -1, 0, 0)
                      .adjusted(
                          qRound(scale_ * cut.left()), qRound(scale_ * cut.top()),
                          -qRound(scale_ * cut.right()), -qRound(scale_ * cut.bottom()));
        p.drawRect(r);

        const QPoint rc = r.center();
        const int rcx = rc.x(), rcy = rc.y();

        int rl, rt, rr, rb;
        r.getCoords(&rl, &rt, &rr, &rb);
        const int rw = rr - rl;

        // cross
        const IJ& off = gSession->geometry().midPixOffset();
        const int x = qRound(rcx + scale_ * off.i);
        const int y = qRound(rcy + scale_ * off.j);
        p.drawLine(x, rt, x, rb);
        p.drawLine(rl, y, rr, y);

        // text
        QPoint pos(rr - rw / 5, rcy);
        p.setPen(Qt::cyan);
        p.drawText(pos, "γ=0");
    }

    // frame
    p.setPen(Qt::black);
    p.drawRect(rect.adjusted(-1, -1, 0, 0));
}


// ************************************************************************** //
//  local base class ImageTab
// ************************************************************************** //

//! Pure virtual base class for DataImageTab and CorrImageTab, shows a detector image and controls.

class ImageTab : public QWidget {
public:
    ImageTab();
    void render();
protected:
    virtual QPixmap pixmap() = 0;
    QHBoxLayout controls_;
    QPixmap makePixmap(shp_Image);
    QPixmap makePixmap(const class Measurement&, const Range&, const Range&);
    QPixmap makeBlankPixmap();
    QImage makeImage(shp_Image, bool curvedScale);
    ImageWidget imageView_;
private:
    QVBoxLayout box_;
    QHBoxLayout box1_;
};

ImageTab::ImageTab() {
    // inbound connections
    connect(gSession, &Session::sigDetector, this, &ImageTab::render);
    connect(gSession, &Session::sigNorm, this, &ImageTab::render);
    connect(gSession, &Session::sigImage, this, &ImageTab::render);

    // outbound connections
    connect(&gGui->toggles->enableCorr, &QAction::toggled, [this](bool /*unused*/) { render(); });
    connect(&gGui->toggles->showBins, &QAction::toggled, [this](bool /*unused*/) { render(); });

    // layout
    box1_.addWidget(new XIconButton(&gGui->toggles->fixedIntenImage));
    box1_.addWidget(new XIconButton(&gGui->toggles->stepScale));
    box1_.addWidget(new XIconButton(&gGui->toggles->showOverlay));
    box1_.addStretch(1);
    controls_.addLayout(&box1_);

    box_.addLayout(&controls_);
    box_.addWidget(&imageView_);
    setLayout(&box_);
}

void ImageTab::render() {
    gSession->corrset().clearIntens(); // TODO move this to more appriate place
    imageView_.setPixmap(pixmap());
}

QPixmap ImageTab::makePixmap(shp_Image image) {
    return QPixmap::fromImage(makeImage(image, !gGui->isFixedIntenImageScale()));
}

QPixmap ImageTab::makePixmap(
    const Measurement& cluster, const Range& rgeGma, const Range& rgeTth) {
    QImage im = makeImage(cluster.image(), !gGui->isFixedIntenImageScale());
    shp_AngleMap angleMap = gSession->angleMap(cluster);

    const QSize& size = im.size();
    for_ij (size.width(), size.height()) {
        const ScatterDirection& a = angleMap->at(i, j);
        QColor color = im.pixel(i, j);
        if (rgeGma.contains(a.gma)) {
            if (rgeTth.contains(a.tth)) {
                color = Qt::yellow;
            } else {
                color.setGreen(qFloor(color.green() * .3 + 255 * .7));
            }
        } else if (rgeTth.contains(a.tth)) {
            color.setGreen(qFloor(color.green() * .3 + 255 * .7));
        }
        im.setPixel(i, j, color.rgb());
    }

    return QPixmap::fromImage(im);
}

QPixmap ImageTab::makeBlankPixmap() {
    const size2d size = gSession->imageSize();
    QPixmap pixmap(size.w, size.h);
    pixmap.fill(QColor(0, 0, 0, 0));
    return pixmap;
}

QImage ImageTab::makeImage(shp_Image image, bool curvedScale) {
    if (!image)
        return {};

    ImageLens imageLens(*image, true, false);
    const size2d size = imageLens.imgSize();
    if (size.isEmpty())
        return {};

    QImage ret(QSize(size.w, size.h), QImage::Format_RGB32);

    const Range rgeInten = imageLens.rgeInten(gGui->isFixedIntenImageScale());
    inten_t maxInten = inten_t(rgeInten.max);

    for_ij (size.w, size.h)
        ret.setPixel(i, j,
                     colormap::intenImage(imageLens.imageInten(i, j), maxInten, curvedScale));
    return ret;
}

// ************************************************************************** //
//  local class IdxMeas
// ************************************************************************** //

//! A spin box that governs which measurement out of the highlighted group shall be shown.

class IdxMeas : public CSpinBox {
public:
    IdxMeas();
private:
    void fromCore();
};

IdxMeas::IdxMeas()
    : CSpinBox {"idxMeas", 2, false, 1, INT_MAX,
        "Number of measurement within the current group of measurements"}
{
    connect(gSession, &Session::sigDataHighlight, this, &IdxMeas::fromCore);
    connect(this, _SLOT_(QSpinBox, valueChanged, int), [this](int val) {
            gSession->dataset().highlight().setMeasurement(val-1); });
    fromCore();
}

void IdxMeas::fromCore()
{
    auto& hl = gSession->dataset().highlight();
    if (!hl.cluster()) {
        setEnabled(false);
        setValue(1);
        return;
    }
    setEnabled( gSession->dataset().binning() > 1);
    int max = hl.cluster()->count();
    setMaximum(max);
    if ( hl.measurementIndex()+1>max )
        hl.setMeasurement(max-1);
    setValue(hl.measurementIndex()+1);
}

// ************************************************************************** //
//  local class DataImageTab
// ************************************************************************** //

//! A tab for a data image and associated controls.

//! To display a 2d detector image of diffraction data,
//! as opposed to the correction data in CorrImageTab.

class DataImageTab : public ImageTab {
public:
    DataImageTab();
private:
    QHBoxLayout boxImg_, boxGreen_, boxGamma_;
    QPixmap pixmap() final;
    IdxMeas idxMeas_;
    CSpinBox numSlices_{"numSlices", 2, false, 0, INT_MAX,
            "Number of γ slices (0: no slicing, take entire image)" };
    CSpinBox idxSlice_{"numSlice", 2, false, 1, INT_MAX, "Number of γ slice to be shown" };
    CSpinBox idxTheta_ {"idxTheta", 4, false, 1, INT_MAX, "Number of 2θ bin to be shown" };
    CDoubleSpinBox minGamma_{"minGamma", 6};
    CDoubleSpinBox maxGamma_{"maxGamma", 6};
};

DataImageTab::DataImageTab() {
    // inbound connection
    connect(gSession, &Session::sigDataHighlight, this, &ImageTab::render);
    connect(gSession, &Session::sigGamma, [this]() {
            numSlices_.setValue(gSession->gammaSelection().numSlices());
            idxSlice_.setValue(gSession->gammaSelection().idxSlice());
            minGamma_.setValue(gSession->gammaSelection().range().min);
            maxGamma_.setValue(gSession->gammaSelection().range().max);
            render(); });
    connect(gSession, &Session::sigTheta, [this]() {
            idxTheta_.setValue(gSession->thetaSelection().iSlice()+1);
            render(); });

    // outbound connections and control widget setup
    connect(&idxTheta_, _SLOT_(QSpinBox, valueChanged, int), [this](int val) {
            gSession->thetaSelection().selectSlice(val-1); });
    connect(&numSlices_, _SLOT_(QSpinBox, valueChanged, int), [this](int val) {
            gSession->gammaSelection().setNumSlices(val); });
    connect(&idxSlice_, _SLOT_(QSpinBox, valueChanged, int), [this](int val) {
            gSession->gammaSelection().selectSlice(val); });
    minGamma_.setReadOnly(true);
    maxGamma_.setReadOnly(true);

    // layout
    boxImg_.addWidget(new QLabel("m#"));
    boxImg_.addWidget(&idxMeas_);
    boxImg_.addStretch(1);
    controls_.addLayout(&boxImg_);

    boxGreen_.addWidget(new XIconButton(&gGui->toggles->showBins));
    boxGreen_.addWidget(new QLabel("ϑ#"));
    boxGreen_.addWidget(&idxTheta_);
    boxGreen_.addStretch(1);
    controls_.addLayout(&boxGreen_);

    boxGamma_.addWidget(new QLabel("γ count"));
    boxGamma_.addWidget(&numSlices_);
    boxGamma_.addWidget(new QLabel("γ#"));
    boxGamma_.addWidget(&idxSlice_);
    boxGamma_.addWidget(new QLabel("min"));
    boxGamma_.addWidget(&minGamma_);
    boxGamma_.addWidget(new QLabel("max"));
    boxGamma_.addWidget(&maxGamma_);
    controls_.addLayout(&boxGamma_);
}

QPixmap DataImageTab::pixmap() {
    const Measurement* measurement = gSession->dataset().highlight().measurement();
    if (!measurement) {
        return makeBlankPixmap();
    } else if (gGui->toggles->showBins.isChecked()) {
        return makePixmap(*measurement,
                          gSession->gammaSelection().range(),
                          gSession->thetaSelection().range());
    }
    return makePixmap(measurement->image());
}

// ************************************************************************** //
//  local class CorrImageTab
// ************************************************************************** //

//! A tab for the correction image and associated controls.

//! To display a 2d detector image of correction data,
//! as opposed to the diffraction data in DataImageTab.

class CorrImageTab : public ImageTab {
public:
    CorrImageTab();
private:
    QPixmap pixmap() final;
};

CorrImageTab::CorrImageTab() {
    controls_.addStretch(1);
}

QPixmap CorrImageTab::pixmap() {
    return makePixmap(gSession->corrset().image());
}

// ************************************************************************** //
//  class SubframeImage
// ************************************************************************** //

SubframeImage::SubframeImage() {
    setTabPosition(QTabWidget::North);
    addTab(new DataImageTab, "Data image");
    addTab(new CorrImageTab, "Corr image");
    connect(gSession, &Session::sigCorr, [this]() {
            setTabEnabled(1, gSession->corrset().hasFile()); });
    setTabEnabled(1, false);
}
