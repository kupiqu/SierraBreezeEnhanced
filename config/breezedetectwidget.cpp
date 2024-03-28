
//////////////////////////////////////////////////////////////////////////////
// breezedetectwidget.cpp
// Note: this class is a stripped down version of
// /kdebase/workspace/kwin/kcmkwin/kwinrules/detectwidget.cpp
// Copyright (c) 2004 Lubos Lunak <l.lunak@kde.org>
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "breezedetectwidget.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

namespace Breeze
{

    //_________________________________________________________
    DetectDialog::DetectDialog( QObject* parent ):
        QObject( parent )
    {
    }

    //_________________________________________________________
    void DetectDialog::detect()
    {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                            QStringLiteral("/KWin"),
                                                            QStringLiteral("org.kde.KWin"),
                                                            QStringLiteral("queryWindowInfo"));

        QDBusPendingReply<QVariantMap> asyncReply = QDBusConnection::sessionBus().asyncCall(message);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncReply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QVariantMap> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
                Q_EMIT detectionDone(false);
                return;
            }
            m_properties = reply.value();
            Q_EMIT detectionDone(true);
        });
    }

    //_________________________________________________________
    QVariantMap DetectDialog::properties() const
    {
        return m_properties;
    }
}
