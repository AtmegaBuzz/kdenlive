/*
    SPDX-FileCopyrightText: 2005 Casper Boemann <cbr@boemann.dk>
    SPDX-FileCopyrightText: 2009 Dmitry Kazakov <dimula73@gmail.com>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
#ifndef KIS_CURVE_WIDGET_H
#define KIS_CURVE_WIDGET_H

// Qt includes.

#include <QWidget>

#include "../abstractcurvewidget.h"
#include "kis_cubic_curve.h"

class QMouseEvent;
class QPaintEvent;


/**
 * KisCurveWidget is a widget that shows a single curve that can be edited
 * by the user. The user can grab the curve and move it; this creates
 * a new control point. Control points can be deleted by selecting a point
 * and pressing the delete key.
 *
 * (From: http://techbase.kde.org/Projects/Widgets_and_Classes#KisCurveWidget)
 * KisCurveWidget allows editing of spline based y=f(x) curves. Handy for cases
 * where you want the user to control such things as tablet pressure
 * response, color transformations, acceleration by time, aeroplane lift
 *by angle of attack.
 */
class KisCurveWidget : public AbstractCurveWidget<KisCubicCurve>
{
    Q_OBJECT

public:
    using Point_t = QPointF;

    /**
     * Create a new curve widget with a default curve, that is a straight
     * line from bottom-left to top-right.
     */
    explicit KisCurveWidget(QWidget *parent = nullptr);

    ~KisCurveWidget() override;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

public:
    /**
     * Handy function that creates new point in the middle
     * of the curve and sets focus on the m_intIn field,
     * so the user can move this point anywhere in a moment
     */
    void addPointInTheMiddle();

    void setCurve(KisCubicCurve &&curve);

    QList<QPointF> getPoints() const override;

private:
    double io2sp(int x) const;
    int sp2io(double x) const;
    bool jumpOverExistingPoints(QPointF &pt, int skipIndex);
    int nearestPointInRange(QPointF pt, int wWidth, int wHeight) const;

    /* Dragging variables */
    double m_grabOffsetX;
    double m_grabOffsetY;
    double m_grabOriginalX;
    double m_grabOriginalY;
    QPointF m_draggedAwayPoint;
    int m_draggedAwayPointIndex;

    bool m_guideVisible;
    QColor m_colorGuide;
};

#endif /* KIS_CURVE_WIDGET_H */
