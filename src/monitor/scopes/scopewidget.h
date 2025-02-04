/*
    SPDX-FileCopyrightText: 2015 Meltytech LLC
    SPDX-FileCopyrightText: 2015 Brian Matherly <code@brianmatherly.com>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef SCOPEWIDGET_H
#define SCOPEWIDGET_H

#include "dataqueue.h"
#include "sharedframe.h"
#include <QFuture>
#include <QMutex>
#include <QString>
#include <QThread>
#include <QWidget>

/*!
  \class ScopeWidget
  \brief The ScopeWidget provides a common interface for all scopes in Shotcut.

  ScopeWidget is a QWidget that provides some additional functionality that is
  common to all scopes. One common function is a queue that can receive and
  store frames until they can be processed by the scope. Another common function
  is the ability to trigger the "heavy lifting" to be done in a worker thread.

  Frames are received by the onNewFrame() slot. The ScopeWidget automatically
  places new frames in the DataQueue (m_queue). Subclasses shall implement the
  refreshScope() function and can check for new frames in m_queue.

  refreshScope() is run from a separate thread. Therefore, any members that are
  accessed by both the worker thread (refreshScope) and the GUI thread
  (paintEvent(), resizeEvent(), etc) must be protected by a mutex. After the
  refreshScope() function returns, the ScopeWidget will automatically request
  the GUI thread to update(). A well implemented ScopeWidget will be designed
  such that most of the CPU intensive work will be done in refreshScope() and
  the paintEvent() implementation will complete quickly to avoid hanging up the
  GUI thread.

  Subclasses shall also implement getTitle() so that the application can display
  an appropriate title for the scope.
*/

class ScopeWidget : public QWidget
{
    Q_OBJECT

public:
    /*!
      Constructs an ScopeWidget.

      The \a name will be set as the objectName and should be initialized by
      subclasses.
    */
    explicit ScopeWidget(QWidget *parent = nullptr);

    //! Destructs a ScopeWidget.
    ~ScopeWidget() override;

    /*!
      Returns the title of the scope to be displayed by the application.
      This virtual function must be implemented by subclasses.
    */
    // virtual QString getTitle() = 0;

    /*!
      Sets the preferred orientation on the scope.
      This virtual function may be reimplemented by subclasses.
    */
    // virtual void setOrientation(Qt::Orientation) {};

public slots:
    //! Provides a new frame to the scope. Should be called by the application.
    void onNewFrame(const SharedFrame &frame);

protected:
    /*!
      Triggers refreshScope() to be called in a new thread context.
      Typically requestRefresh would be called from the GUI thread
      (e.g. in resizeEvent()). onNewFrame() also calls requestRefresh().
    */
    void requestRefresh();

    /*!
      Performs the main, CPU intensive, scope drawing in a new thread.

      refreshScope() Shall be implemented by subclasses. Care must be taken to
      protect any members that may be accessed concurrently by the refresh
      thread and the GUI thread.
    */
    virtual void refreshScope(const QSize &size, bool full) = 0;

    /*!
      Stores frames received by onNewFrame().

      Subclasses should check this queue for new frames in the refreshScope()
      implementation.
    */
    DataQueue<SharedFrame> m_queue;

    void resizeEvent(QResizeEvent *) override;
    void changeEvent(QEvent *) override;

private:
    Q_INVOKABLE void onRefreshThreadComplete();
    void refreshInThread();
    QFuture<void> m_future;
    bool m_refreshPending{false};

    // Members accessed in multiple threads (mutex protected).
    QMutex m_mutex;
    bool m_forceRefresh{false};
    QSize m_size;
};

#endif // SCOPEWIDGET_H
