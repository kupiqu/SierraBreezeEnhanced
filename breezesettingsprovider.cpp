/*
 * Copyright 2014  Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "breezesettingsprovider.h"

#include "breezeexceptionlist.h"

#include <KWindowInfo>

#include <QRegularExpression>
#include <QTextStream>

namespace Breeze
{

    SettingsProvider *SettingsProvider::s_self = nullptr;

    //__________________________________________________________________
    SettingsProvider::SettingsProvider():
        m_config( KSharedConfig::openConfig( QStringLiteral("sierrabreezeenhancedrc") ) )
    { reconfigure(); }

    //__________________________________________________________________
    SettingsProvider::~SettingsProvider()
    { s_self = nullptr; }

    //__________________________________________________________________
    SettingsProvider *SettingsProvider::self()
    {
        // TODO: this is not thread safe!
        if (!s_self)
        { s_self = new SettingsProvider(); }

        return s_self;
    }

    //__________________________________________________________________
    void SettingsProvider::reconfigure()
    {
        if( !m_defaultSettings )
        {
            m_defaultSettings = InternalSettingsPtr(new InternalSettings());
            m_defaultSettings->setCurrentGroup( QStringLiteral("Windeco") );
        }

        m_defaultSettings->load();

        ExceptionList exceptions;
        exceptions.readConfig( m_config );
        m_exceptions = exceptions.get();

    }

    //__________________________________________________________________
    InternalSettingsPtr SettingsProvider::internalSettings( Decoration *decoration ) const
    {

        QString windowTitle;
        QString className;

        // get the client
        const auto client = decoration->client().toStrongRef();

        foreach( auto internalSettings, m_exceptions )
        {

            // discard disabled exceptions
            if( !internalSettings->enabled() ) continue;

            // discard exceptions with empty exception pattern
            if( internalSettings->exceptionPattern().isEmpty() ) continue;

            if (internalSettings->isDialog())
            {
              KWindowInfo info(client->windowId(), NET::WMWindowType);
              if (info.valid() && info.windowType(NET::NormalMask | NET::DialogMask) != NET::Dialog) {
                continue;
              }
            }

            /*
            decide which value is to be compared
            to the regular expression, based on exception type
            */
            QString value;
            switch( internalSettings->exceptionType() )
            {
                case InternalSettings::ExceptionWindowTitle:
                {
                    value = windowTitle.isEmpty() ? (windowTitle = client->caption()):windowTitle;
                    break;
                }

                default:
                case InternalSettings::ExceptionWindowClassName:
                {
                    if( className.isEmpty() )
                    {
                        // retrieve class name
                        KWindowInfo info( client->windowId(), {}, NET::WM2WindowClass );
                        QString window_className( QString::fromUtf8(info.windowClassName()) );
                        QString window_class( QString::fromUtf8(info.windowClassClass()) );
                        className = window_className + QStringLiteral(" ") + window_class;
                    }

                    value = className;
                    break;
                }

            }

            // check matching
            QRegularExpression rx( internalSettings->exceptionPattern() );
            if( rx.match( value ).hasMatch() )
            { return internalSettings; }

        }

        return m_defaultSettings;

    }

}
