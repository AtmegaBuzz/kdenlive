/*
    SPDX-FileCopyrightText: 2018 Jean-Baptiste Mardelle <jb@kdenlive.org>
    This file is part of Kdenlive. See www.kdenlive.org.

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "monitorproxy.h"
#include "core.h"
#include "doc/kthumb.h"
#include "glwidget.h"
#include "kdenlivesettings.h"
#include "monitormanager.h"
#include "profiles/profilemodel.hpp"

#include <QUuid>

#include <mlt++/MltConsumer.h>
#include <mlt++/MltFilter.h>
#include <mlt++/MltProducer.h>
#include <mlt++/MltProfile.h>

MonitorProxy::MonitorProxy(GLWidget *parent)
    : QObject(parent)
    , q(parent)
    , m_position(-1)
    , m_zoneIn(0)
    , m_zoneOut(-1)
    , m_hasAV(false)
    , m_speed(0)
    , m_clipType(0)
    , m_clipId(-1)
    , m_seekFinished(true)
    , m_td(nullptr)
    , m_trimmingFrames1(0)
    , m_trimmingFrames2(0)
    , m_boundsCount(0)
{
}

int MonitorProxy::getPosition() const
{
    return m_position;
}

void MonitorProxy::resetPosition()
{
    m_position = -1;
}

int MonitorProxy::rulerHeight() const
{
    return q->m_rulerHeight;
}

void MonitorProxy::setRulerHeight(int addedHeight)
{
    q->updateRulerHeight(addedHeight);
}

void MonitorProxy::seek(int delta, uint modifiers)
{
    emit q->mouseSeek(delta, modifiers);
}

int MonitorProxy::overlayType() const
{
    return (q->m_id == int(Kdenlive::ClipMonitor) ? KdenliveSettings::clipMonitorOverlayGuides() : KdenliveSettings::projectMonitorOverlayGuides());
}

void MonitorProxy::setOverlayType(int ix)
{
    if (q->m_id == int(Kdenlive::ClipMonitor)) {
        KdenliveSettings::setClipMonitorOverlayGuides(ix);
    } else {
        KdenliveSettings::setProjectMonitorOverlayGuides(ix);
    }
}

bool MonitorProxy::setPosition(int pos)
{
    return setPositionAdvanced(pos, false);
}

bool MonitorProxy::setPositionAdvanced(int pos, bool noAudioScrub)
{
    if (m_position == pos) {
        return true;
    }
    m_position = pos;
    emit requestSeek(pos, noAudioScrub);
    if (m_seekFinished) {
        m_seekFinished = false;
        emit seekFinishedChanged();
    }
    emit positionChanged(pos);
    return false;
}

void MonitorProxy::positionFromConsumer(int pos, bool playing)
{
    if (playing) {
        m_position = pos;
        emit positionChanged(pos);
        if (!m_seekFinished) {
            m_seekFinished = true;
            emit seekFinishedChanged();
        }
    } else {
        if (!m_seekFinished && m_position == pos) {
            m_seekFinished = true;
            emit seekFinishedChanged();
        }
    }
}

void MonitorProxy::setMarker(const QString &comment, const QColor &color)
{
    if (m_markerComment == comment) {
        return;
    }
    m_markerComment = comment;
    m_markerColor = color;
    emit markerChanged();
}

int MonitorProxy::zoneIn() const
{
    return m_zoneIn;
}

int MonitorProxy::zoneOut() const
{
    return m_zoneOut;
}

void MonitorProxy::setZoneIn(int pos)
{
    if (m_zoneIn > 0) {
        emit removeSnap(m_zoneIn);
    }
    m_zoneIn = pos;
    if (pos > 0) {
        emit addSnap(pos);
    }
    emit zoneChanged();
    emit saveZone(QPoint(m_zoneIn, m_zoneOut));
}

void MonitorProxy::setZoneOut(int pos)
{
    if (m_zoneOut > 0) {
        emit removeSnap(m_zoneOut - 1);
    }
    m_zoneOut = pos;
    if (pos > 0) {
        emit addSnap(m_zoneOut - 1);
    }
    emit zoneChanged();
    emit saveZone(QPoint(m_zoneIn, m_zoneOut));
}

void MonitorProxy::startZoneMove()
{
    m_undoZone = QPoint(m_zoneIn, m_zoneOut);
}

void MonitorProxy::endZoneMove()
{
    emit saveZoneWithUndo(m_undoZone, QPoint(m_zoneIn, m_zoneOut));
}

void MonitorProxy::setZone(int in, int out, bool sendUpdate)
{
    if (m_zoneIn > 0) {
        emit removeSnap(m_zoneIn);
    }
    if (m_zoneOut > 0) {
        emit removeSnap(m_zoneOut - 1);
    }
    m_zoneIn = in;
    m_zoneOut = out;
    if (m_zoneIn > 0) {
        emit addSnap(m_zoneIn);
    }
    if (m_zoneOut > 0) {
        emit addSnap(m_zoneOut - 1);
    }
    emit zoneChanged();
    if (sendUpdate) {
        emit saveZone(QPoint(m_zoneIn, m_zoneOut));
    }
}

void MonitorProxy::setZone(QPoint zone, bool sendUpdate)
{
    setZone(zone.x(), zone.y(), sendUpdate);
}

void MonitorProxy::resetZone()
{
    m_zoneIn = 0;
    m_zoneOut = -1;
    m_clipBounds = {};
    m_boundsCount = 0;
    emit clipBoundsChanged();
}

double MonitorProxy::fps() const
{
    return pCore->getCurrentFps();
}

QPoint MonitorProxy::zone() const
{
    return {m_zoneIn, m_zoneOut};
}

QImage MonitorProxy::extractFrame(int frame_position, const QString &path, int width, int height, bool useSourceProfile)
{
    if (width == -1) {
        width = pCore->getCurrentProfile()->width();
        height = pCore->getCurrentProfile()->height();
    } else if (width % 2 == 1) {
        width++;
    }
    if (!path.isEmpty()) {
        QScopedPointer<Mlt::Profile> tmpProfile(new Mlt::Profile());
        QScopedPointer<Mlt::Producer> producer(new Mlt::Producer(*tmpProfile, path.toUtf8().constData()));
        if (producer && producer->is_valid()) {
            tmpProfile->from_producer(*producer);
            width = tmpProfile->width();
            height = tmpProfile->height();
            double projectFps = pCore->getCurrentFps();
            double currentFps = tmpProfile->fps();
            if (!qFuzzyCompare(projectFps, currentFps)) {
                frame_position = int(frame_position * currentFps / projectFps);
            }
            QImage img = KThumb::getFrame(producer.data(), frame_position, width, height);
            return img;
        }
    }

    if ((q->m_producer == nullptr) || !path.isEmpty()) {
        QImage pix(width, height, QImage::Format_RGB32);
        pix.fill(Qt::black);
        return pix;
    }
    Mlt::Frame *frame = nullptr;
    QImage img;
    if (useSourceProfile) {
        // Our source clip's resolution is higher than current profile, export at full res
        QScopedPointer<Mlt::Profile> tmpProfile(new Mlt::Profile());
        QString service = q->m_producer->get("mlt_service");
        QScopedPointer<Mlt::Producer> tmpProd(new Mlt::Producer(*tmpProfile, service.toUtf8().constData(), q->m_producer->get("resource")));
        tmpProfile->from_producer(*tmpProd);
        width = tmpProfile->width();
        height = tmpProfile->height();
        if (tmpProd && tmpProd->is_valid()) {
            Mlt::Filter scaler(*tmpProfile, "swscale");
            Mlt::Filter converter(*tmpProfile, "avcolor_space");
            tmpProd->attach(scaler);
            tmpProd->attach(converter);
            // TODO: paste effects
            // Clip(*tmpProd).addEffects(*q->m_producer);
            double projectFps = pCore->getCurrentFps();
            double currentFps = tmpProfile->fps();
            if (qFuzzyCompare(projectFps, currentFps)) {
                tmpProd->seek(q->m_producer->position());
            } else {
                int maxLength = int(q->m_producer->get_length() * currentFps / projectFps);
                tmpProd->set("length", maxLength);
                tmpProd->set("out", maxLength - 1);
                tmpProd->seek(int(q->m_producer->position() * currentFps / projectFps));
            }
            frame = tmpProd->get_frame();
            img = KThumb::getFrame(frame, width, height);
            delete frame;
        }
    } else if (KdenliveSettings::gpu_accel()) {
        QString service = q->m_producer->get("mlt_service");
        QScopedPointer<Mlt::Producer> tmpProd(
            new Mlt::Producer(pCore->getCurrentProfile()->profile(), service.toUtf8().constData(), q->m_producer->get("resource")));
        Mlt::Filter scaler(pCore->getCurrentProfile()->profile(), "swscale");
        Mlt::Filter converter(pCore->getCurrentProfile()->profile(), "avcolor_space");
        tmpProd->attach(scaler);
        tmpProd->attach(converter);
        tmpProd->seek(q->m_producer->position());
        frame = tmpProd->get_frame();
        img = KThumb::getFrame(frame, width, height);
        delete frame;
    } else {
        frame = q->m_producer->get_frame();
        img = KThumb::getFrame(frame, width, height);
        delete frame;
    }
    return img;
}

void MonitorProxy::activateClipMonitor(bool isClipMonitor)
{
    pCore->monitorManager()->activateMonitor(isClipMonitor ? Kdenlive::ClipMonitor : Kdenlive::ProjectMonitor);
}

QString MonitorProxy::toTimecode(int frames) const
{
    return KdenliveSettings::frametimecode() ? QString::number(frames) : q->frameToTime(frames);
}

void MonitorProxy::setClipProperties(int clipId, ClipType::ProducerType type, bool hasAV, const QString &clipName)
{
    if (clipId != m_clipId) {
        m_clipId = clipId;
        emit clipIdChanged();
    }
    if (hasAV != m_hasAV) {
        m_hasAV = hasAV;
        emit clipHasAVChanged();
    }
    if (type != m_clipType) {
        m_clipType = type;
        emit clipTypeChanged();
    }
    if (clipName == m_clipName) {
        m_clipName.clear();
        emit clipNameChanged();
    }
    m_clipName = clipName;
    emit clipNameChanged();
}

void MonitorProxy::setAudioThumb(const QList <int> &streamIndexes, const QList <int> &channels)
{
    m_audioChannels = channels;
    m_audioStreams = streamIndexes;
    emit audioThumbChanged();
}

void MonitorProxy::setAudioStream(const QString &name)
{
    m_clipStream = name;
    emit clipStreamChanged();
}


QPoint MonitorProxy::profile()
{
    QSize s = pCore->getCurrentFrameSize();
    return QPoint(s.width(), s.height());
}

QColor MonitorProxy::thumbColor1() const
{
    return KdenliveSettings::thumbColor1();
}

QColor MonitorProxy::thumbColor2() const
{
    return KdenliveSettings::thumbColor2();
}

bool MonitorProxy::audioThumbFormat() const
{
    return KdenliveSettings::displayallchannels();
}

bool MonitorProxy::audioThumbNormalize() const
{
    return KdenliveSettings::normalizechannels();
}

void MonitorProxy::switchAutoKeyframe()
{
    KdenliveSettings::setAutoKeyframe(!KdenliveSettings::autoKeyframe());
    emit autoKeyframeChanged();
}

bool MonitorProxy::autoKeyframe() const
{
    return KdenliveSettings::autoKeyframe();
}

const QString MonitorProxy::timecode() const
{
    if (m_td) {
        return m_td->displayText();
    }
    return QString();
}

const QString MonitorProxy::trimmingTC1() const
{
    return toTimecode(m_trimmingFrames1);
}

const QString MonitorProxy::trimmingTC2() const
{
    return toTimecode(m_trimmingFrames2);
}

void MonitorProxy::setTimeCode(TimecodeDisplay *td)
{
    m_td = td;
    connect(m_td, &TimecodeDisplay::timeCodeUpdated, this, &MonitorProxy::timecodeChanged);
}

void MonitorProxy::setTrimmingTC1(int frames, bool isRelativ)
{
    if (isRelativ) {
        m_trimmingFrames1 -= frames;
    } else {
        m_trimmingFrames1 = frames;
    }
    emit trimmingTC1Changed();
}

void MonitorProxy::setTrimmingTC2(int frames, bool isRelativ)
{
    if (isRelativ) {
        m_trimmingFrames2 -= frames;
    } else {
        m_trimmingFrames2 = frames;
    }
    emit trimmingTC2Changed();
}

void MonitorProxy::setWidgetKeyBinding(const QString &text) const
{
    pCore->setWidgetKeyBinding(text);
}

void MonitorProxy::setSpeed(double speed)
{
    if (qAbs(m_speed) > 1. || qAbs(speed) > 1.) {
        // check if we have or had a speed > 1 or < -1
        m_speed = speed;
        emit speedChanged();
    }
}

QByteArray MonitorProxy::getUuid() const
{
    return QUuid::createUuid().toByteArray();
}

void MonitorProxy::updateClipBounds(const QVector<QPoint> &bounds)
{
    if (bounds.size() == m_boundsCount) {
        // Enforce refresh, in/out points may have changed
        m_boundsCount = 0;
        emit clipBoundsChanged();
    }
    m_clipBounds = bounds;
    m_boundsCount = bounds.size();
    emit clipBoundsChanged();
}

const QPoint MonitorProxy::clipBoundary(int ix)
{
    return m_clipBounds.at(ix);
}

bool MonitorProxy::seekOnDrop() const
{
    return KdenliveSettings::seekonaddeffect();
}

