// ************************************************************************** //
//
//  Steca: stress and texture calculator
//
//! @file      gui/panels/panel_diffractogram.cpp
//! @brief     Implements classes DiffractogramPlot and Diffractogram.
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#include "panel_diffractogram.h"
#include "core/session.h"

// ************************************************************************** //
//  class DiffractogramPlotOverlay
// ************************************************************************** //

DiffractogramPlotOverlay::DiffractogramPlotOverlay(DiffractogramPlot& plot_)
    : QWidget(&plot_)
    , plot_(plot_)
    , cursorPos_(0)
    , mouseDownPos_(0)
    , hasCursor_(false)
    , mouseDown_(false)
{
    setMouseTracking(true);
    setMargins(0, 0);
}

void DiffractogramPlotOverlay::setMargins(int left, int right) {
    marginLeft_ = left;
    marginRight_ = right;
}

void DiffractogramPlotOverlay::enterEvent(QEvent*) {
    hasCursor_ = true;
    updateCursorRegion();
}

void DiffractogramPlotOverlay::leaveEvent(QEvent*) {
    hasCursor_ = false;
    updateCursorRegion();
}

void DiffractogramPlotOverlay::mousePressEvent(QMouseEvent* e) {
    mouseDownPos_ = cursorPos_;
    mouseDown_ = true;
    addColor_ = (gHub->fittingTab() == eFittingTab::BACKGROUND) ? bgColor_ : reflColor_;
    color_ = e->button() == Qt::LeftButton ? addColor_ : removeColor_;
    update();
}

void DiffractogramPlotOverlay::mouseReleaseEvent(QMouseEvent* e) {
    mouseDown_ = false;
    update();
    Range range(plot_.fromPixels(mouseDownPos_, cursorPos_));
    switch (plot_.getTool()) {
    case DiffractogramPlot::eTool::BACKGROUND:
        if (Qt::LeftButton == e->button())
            gSession->baseline().addRange(range);
        else
            gSession->baseline().removeRange(range);
        break;
    case DiffractogramPlot::eTool::PEAK_REGION:
        plot_.setNewReflRange(range);
        break;
    case DiffractogramPlot::eTool::NONE:
        break;
    }
}

void DiffractogramPlotOverlay::mouseMoveEvent(QMouseEvent* e) {
    updateCursorRegion();
    cursorPos_ = qBound(marginLeft_, e->x(), width() - marginRight_);
    updateCursorRegion();
    if (mouseDown_)
        update();
}

void DiffractogramPlotOverlay::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QRect g = geometry();

    if (mouseDown_) {
        g.setLeft(qMin(mouseDownPos_, cursorPos_));
        g.setRight(qMax(mouseDownPos_, cursorPos_));
        painter.fillRect(g, color_);
    }

    if (hasCursor_) {
        QLineF cursor(cursorPos_, g.top(), cursorPos_, g.bottom());
        painter.setPen(Qt::red);
        painter.drawLine(cursor);
    }
}

void DiffractogramPlotOverlay::updateCursorRegion() {
    const QRect& g = geometry();
    // updating 2 pixels seems to work both on Linux & Mac
    update(cursorPos_ - 1, g.top(), 2, g.height());
}

// ************************************************************************** //
//  class DiffractogramPlot
// ************************************************************************** //

DiffractogramPlot::DiffractogramPlot(Diffractogram& diffractogram)
    : diffractogram_(diffractogram), showBgFit_(false) {

    overlay_ = new DiffractogramPlotOverlay(*this);

    QCPAxisRect* ar = axisRect();

    // fix margins
    QFontMetrics fontMetrics(font());
    int em = fontMetrics.width('M'), ascent = fontMetrics.ascent();

    QMargins margins(6 * em, ascent, em, 2 * ascent);
    ar->setAutoMargins(QCP::msNone);
    ar->setMargins(margins);
    overlay_->setMargins(margins.left(), margins.right());

    // colours
    setBackground(palette().background().color());
    ar->setBackground(Qt::white);

    // graphs in the "main" layer; in the display order
    bgGraph_ = addGraph();
    dgramGraph_ = addGraph();
    dgramBgFittedGraph2_ = addGraph();
    dgramBgFittedGraph2_->setVisible(false);
    dgramBgFittedGraph_ = addGraph();

    bgGraph_->setPen(QPen(QColor(0x21, 0xa1, 0x21, 0xff), 2));

    dgramBgFittedGraph_->setPen(QPen(Qt::black, 2));

    dgramBgFittedGraph2_->setLineStyle(QCPGraph::LineStyle::lsNone);
    dgramBgFittedGraph2_->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc, QColor(255, 0, 0), 4));

    dgramGraph_->setLineStyle(QCPGraph::LineStyle::lsNone);
    dgramGraph_->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc, Qt::gray, 2));

    // background regions
    addLayer("bg", layer("background"), QCustomPlot::limAbove);
    // peaks
    addLayer("refl", layer("main"), QCustomPlot::limAbove);
    // peaks
    addLayer("marks", layer("refl"), QCustomPlot::limAbove);

    setCurrentLayer("marks");

    guesses_ = addGraph();
    guesses_->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 8));
    guesses_->setLineStyle(QCPGraph::lsNone);
    guesses_->setPen(QPen(Qt::darkGray));

    fits_ = addGraph();
    fits_->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 8));
    fits_->setLineStyle(QCPGraph::lsNone);
    fits_->setPen(QPen(Qt::red));

    connect(gHub->toggle_showBackground, &QAction::toggled, [this](bool on) {
        showBgFit_ = on;
        renderAll();
    });

    connect(gSession, &Session::sigBaseline, [this]() { renderAll(); });
    connect(gSession, &Session::sigCorr, this, &DiffractogramPlot::renderAll);
    connect(gSession, &Session::sigActivated, this, &DiffractogramPlot::renderAll);
    connect(gSession, &Session::sigDetector, this, &DiffractogramPlot::renderAll);
    connect(gHub, &MainWin::sigDisplayChanged, this, &DiffractogramPlot::renderAll);
    connect(gSession, &Session::sigDiffractogram, this, &DiffractogramPlot::renderAll);
    connect(gSession, &Session::sigBaseline, this, &DiffractogramPlot::renderAll);

    tool_ = eTool::NONE;
}

void DiffractogramPlot::setTool(eTool tool) {
    tool_ = tool;
    renderAll();
}

void DiffractogramPlot::plot(
    Curve const& dgram, Curve const& dgramBgFitted, Curve const& bg, curve_vec const& refls,
    int currReflIndex) {
    if (dgram.isEmpty()) {
        plotEmpty();
        return;
    }
    const Range& tthRange = dgram.rgeX();

    Range intenRange;
    if (gHub->isFixedIntenDgramScale()) {
        intenRange = gSession->highlightsLens()->rgeInten();
    } else {
        intenRange = dgramBgFitted.rgeY();
        intenRange.extendBy(dgram.rgeY());
    }

    xAxis->setRange(tthRange.min, tthRange.max);
    yAxis->setRange(qMin(0., intenRange.min), intenRange.max);
    yAxis->setNumberFormat("g");
    xAxis->setVisible(true);
    yAxis->setVisible(true);

    if (showBgFit_) {
        bgGraph_->setData(bg.xs().sup(), bg.ys().sup());
    } else {
        bgGraph_->clearData();
    }

    dgramGraph_->setData(dgram.xs().sup(), dgram.ys().sup());
    dgramBgFittedGraph_->setData(dgramBgFitted.xs().sup(), dgramBgFitted.ys().sup());
    dgramBgFittedGraph2_->setData(dgramBgFitted.xs().sup(), dgramBgFitted.ys().sup());

    clearReflLayer();
    setCurrentLayer("refl");

    for_i (refls.count()) {
        const Curve& r = refls.at(i);
        QCPGraph* graph = addGraph();
        reflGraph_.append(graph);
        graph->setPen(QPen(Qt::green, i == currReflIndex ? 2 : 1));
        graph->setData(r.xs().sup(), r.ys().sup());
    }

    replot();
}

void DiffractogramPlot::plotEmpty() {
    xAxis->setVisible(false);
    yAxis->setVisible(false);

    bgGraph_->clearData();
    dgramGraph_->clearData();
    dgramBgFittedGraph_->clearData();
    dgramBgFittedGraph2_->clearData();

    clearReflLayer();
    replot();
}

Range DiffractogramPlot::fromPixels(int pix1, int pix2) {
    return Range::safeFrom(xAxis->pixelToCoord(pix1), xAxis->pixelToCoord(pix2));
}

void DiffractogramPlot::setNewReflRange(const Range& range) {
    Peak* peak = gSession->peaks().selectedPeak();
    if (!peak)
        return;
    peak->setRange(range);
    renderAll();
}

void DiffractogramPlot::clearReflLayer() {
    for (QCPGraph* g : reflGraph_)
        removeGraph(g);
    reflGraph_.clear();
}

void DiffractogramPlot::enterZoom(bool on) {
    overlay_->setHidden(on);
    dgramBgFittedGraph2_->setVisible(on);
}

//! Paints a colored rectangle in the background layer, to indicate area of baseline or peak fit
void DiffractogramPlot::addBgItem(const Range& range) {
    setCurrentLayer("bg");

    QColor color;
    switch (gHub->fittingTab()) {
    case eFittingTab::BACKGROUND:
        color = BgColor_;
        break;
    case eFittingTab::REFLECTIONS:
        color = reflRgeColor_;
        break;
    default:
        break;
    }

    QCPItemRect* ir = new QCPItemRect(this);
    ir->setPen(QPen(color));
    ir->setBrush(QBrush(color));
    QCPItemPosition* br = ir->bottomRight;
    br->setTypeY(QCPItemPosition::ptViewportRatio);
    br->setCoords(range.max, 1);
    QCPItemPosition* tl = ir->topLeft;
    tl->setTypeY(QCPItemPosition::ptViewportRatio);
    tl->setCoords(range.min, 0);
    addItem(ir);
}

void DiffractogramPlot::resizeEvent(QResizeEvent* e) {
    QCustomPlot::resizeEvent(e);
    const QSize size = e->size();
    overlay_->setGeometry(0, 0, size.width(), size.height());
}

void DiffractogramPlot::onPeakData() {
    Peak* peak = gSession->peaks().selectedPeak();
    guesses_->clearData();
    fits_->clearData();

    if (peak && gSession->dataset().highlight().cluster()) {
        const PeakFunction& fun = peak->peakFunction();

        const qpair gp = fun.guessedPeak();
        if (gp.isValid()) {
            guesses_->addData(gp.x, gp.y);
            qreal gw2 = fun.guessedFWHM() / 2;
            guesses_->addData(gp.x - gw2, gp.y / 2);
            guesses_->addData(gp.x + gw2, gp.y / 2);
        }

        const qpair fp = fun.fittedPeak();
        if (fp.isValid()) {
            fits_->addData(fp.x, fp.y);
            qreal fw2 = fun.fittedFWHM() / 2;
            fits_->addData(fp.x - fw2, fp.y / 2);
            fits_->addData(fp.x + fw2, fp.y / 2);
        }
    }

    renderAll();
}

//! Repaints everything, including the colored background areas.
void DiffractogramPlot::renderAll() {
    clearItems();

    switch (tool_) {
    case eTool::BACKGROUND: {
        const Ranges& rs = gSession->baseline().ranges();
        for_i (rs.count())
            addBgItem(rs.at(i));
        break;
    }
    case eTool::PEAK_REGION: {
        Peak* peak = gSession->peaks().selectedPeak();
        addBgItem(peak ? peak->range() : Range());
        break;
    }
    case eTool::NONE:
        break;
    }

    if (!gSession->dataset().highlight().cluster()) {
        plotEmpty();
        return;
    }
    calcDgram();
    calcBackground();
    calcPeaks();
    plot(dgram_, dgramBgFitted_, bg_, refls_, currReflIndex_);
}

void DiffractogramPlot::calcDgram() {
    dgram_.clear();
    if (!gSession->dataset().highlight().cluster())
        return;
    if (gHub->isCombinedDgram())
        dgram_ = gSession->experiment().avgCurve();
    else
        dgram_ = gSession->highlightsLens()->makeCurve(gSession->gammaRange());
}

void DiffractogramPlot::calcBackground() {
    bg_.clear();
    dgramBgFitted_.clear();

    const Polynom& bgPolynom = Polynom::fromFit(
        gSession->baseline().polynomDegree(), dgram_, gSession->baseline().ranges());
        // TODO bundle this code line which similarly appears in at least one other place

    for_i (dgram_.count()) {
        qreal x = dgram_.x(i), y = bgPolynom.y(x);
        bg_.append(x, y);
        dgramBgFitted_.append(x, dgram_.y(i) - y);
    }
}

void DiffractogramPlot::calcPeaks() {
    refls_.clear();
    currReflIndex_ = 0;

    for_i (gSession->peaks().count()) {
        Peak& r = gSession->peaks().at(i);
        if (&r == gSession->peaks().selectedPeak())
            currReflIndex_ = i;

        r.fit(dgramBgFitted_);

        const Range& rge = r.range();
        const PeakFunction& fun = r.peakFunction();

        Curve c;
        for_i (dgramBgFitted_.count()) {
            qreal x = dgramBgFitted_.x(i);
            if (rge.contains(x))
                c.append(x, fun.y(x));
        }
        refls_.append(c);
    }
}
