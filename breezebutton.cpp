/*
 * Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
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
#include "breezebutton.h"

#include <KDecoration2/DecoratedClient>
#include <KColorUtils>

#include <QPainter>

namespace Breeze
{

    using KDecoration2::ColorRole;
    using KDecoration2::ColorGroup;
    using KDecoration2::DecorationButtonType;


    //__________________________________________________________________
    Button::Button(DecorationButtonType type, Decoration* decoration, QObject* parent)
        : DecorationButton(type, decoration, parent)
        , m_animation( new QPropertyAnimation( this ) )
    {

        // setup animation
        m_animation->setStartValue( 0 );
        m_animation->setEndValue( 1.0 );
        m_animation->setTargetObject( this );
        m_animation->setPropertyName( "opacity" );
        m_animation->setEasingCurve( QEasingCurve::InOutQuad );

        // setup default geometry
        const int height = decoration->buttonHeight();
        setGeometry(QRect(0, 0, height, height));
        setIconSize(QSize( height, height ));

        // connections
        connect(decoration->client().data(), SIGNAL(iconChanged(QIcon)), this, SLOT(update()));
        connect(decoration->settings().data(), &KDecoration2::DecorationSettings::reconfigured, this, &Button::reconfigure);
        connect( this, &KDecoration2::DecorationButton::hoveredChanged, this, &Button::updateAnimationState );

        reconfigure();

    }

    //__________________________________________________________________
    Button::Button(QObject *parent, const QVariantList &args)
        : Button(args.at(0).value<DecorationButtonType>(), args.at(1).value<Decoration*>(), parent)
    {
        m_flag = FlagStandalone;
        //! icon size must return to !valid because it was altered from the default constructor,
        //! in Standalone mode the button is not using the decoration metrics but its geometry
        m_iconSize = QSize(-1, -1);
    }

    //__________________________________________________________________
    Button *Button::create(DecorationButtonType type, KDecoration2::Decoration *decoration, QObject *parent)
    {
        if (auto d = qobject_cast<Decoration*>(decoration))
        {
            Button *b = new Button(type, d, parent);
            switch( type )
            {

                case DecorationButtonType::Close:
                b->setVisible( d->client().data()->isCloseable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::closeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Maximize:
                b->setVisible( d->client().data()->isMaximizeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::maximizeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Minimize:
                b->setVisible( d->client().data()->isMinimizeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::minimizeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::ContextHelp:
                b->setVisible( d->client().data()->providesContextHelp() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::providesContextHelpChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Shade:
                b->setVisible( d->client().data()->isShadeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::shadeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Menu:
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::iconChanged, b, [b]() { b->update(); });
                break;

                default: break;

            }

            return b;
        }

        return nullptr;

    }

    //__________________________________________________________________
    void Button::paint(QPainter *painter, const QRect &repaintRegion)
    {
        Q_UNUSED(repaintRegion)

        if (!decoration()) return;

        painter->save();

        // translate from offset
        if( m_flag == FlagFirstInList ) painter->translate( m_offset );
        else painter->translate( 0, m_offset.y() );

        if( !m_iconSize.isValid() ) m_iconSize = geometry().size().toSize();

        // menu button
        if (type() == DecorationButtonType::Menu)
        {

            const QRectF iconRect( geometry().topLeft(), 0.8*m_iconSize );
            const qreal width( m_iconSize.width() );
            painter->translate( 0.1*width, 0.1*width );
            decoration()->client().data()->icon().paint(painter, iconRect.toRect());

        } else {

            auto d = qobject_cast<Decoration*>( decoration() );

            if ( d && d->internalSettings()->buttonStyle() == 3 )
                drawIconBreezeStyle( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 4 )
                drawIconAdwaitaStyle( painter );
            else if ( d && ( d->internalSettings()->buttonStyle() == 5 || d->internalSettings()->buttonStyle() == 6 || d->internalSettings()->buttonStyle() == 7 ) )
                drawIconDarkAuroraeStyle( painter );
            else
                drawIcon( painter );

        }

        painter->restore();

    }

    //__________________________________________________________________
    void Button::drawIcon( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        painter->scale( width/20, width/20 );
        painter->translate( 1, 1 );

        auto d = qobject_cast<Decoration*>( decoration() );
        bool inactiveWindow( d && !d->client().data()->isActive() );
        bool useActiveButtonStyle( d && d->internalSettings()->buttonStyle() == 1 );
        bool useInactiveButtonStyle( d && d->internalSettings()->buttonStyle() == 2 );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );

        const QColor darkSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(81, 102, 107) : QColor(34, 45, 50) );
        const QColor lightSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(192, 193, 194) : QColor(250, 251, 252) );

        // symbols color

        QColor symbolColor( this->autoColor( inactiveWindow, useActiveButtonStyle, useInactiveButtonStyle, isMatchTitleBarColor, darkSymbolColor, lightSymbolColor ) );

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        symbol_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                const QColor button_color = QColor(242, 80, 86);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && isHovered() )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  // it's a cross
                  painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
                  painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 6 ) );
                }
                break;
            }

            case DecorationButtonType::Maximize:
            {
                const QColor button_color = QColor(19, 209, 61);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && isHovered() )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( Qt::NoPen );

                  // two triangles
                  QPainterPath path1, path2;
                  if( isChecked() )
                  {
                      path1.moveTo(8.5, 9.5);
                      path1.lineTo(2.5, 9.5);
                      path1.lineTo(8.5, 15.5);

                      path2.moveTo(9.5, 8.5);
                      path2.lineTo(15.5, 8.5);
                      path2.lineTo(9.5, 2.5);
                  }
                  else
                  {
                      path1.moveTo(5, 13);
                      path1.lineTo(11, 13);
                      path1.lineTo(5, 7);

                      path2.moveTo(13, 5);
                      path2.lineTo(7, 5);
                      path2.lineTo(13, 11);
                  }

                  painter->fillPath(path1, QBrush(symbolColor));
                  painter->fillPath(path2, QBrush(symbolColor));
                }
                break;
            }

            case DecorationButtonType::Minimize:
            {
                const QColor button_color = QColor(252, 190, 7);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && isHovered() )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                }
                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                const QColor button_color = QColor(125, 209, 200);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( Qt::NoPen );
                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 6, 6, 6, 6 ) );
                }
                break;
            }

            case DecorationButtonType::Shade:
            {
                const QColor button_color = QColor(204, 176, 213);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if (isChecked())
                {
                    painter->setPen( symbol_pen );
                    painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 12 ) );
                    painter->setPen( Qt::NoPen );
                    QPainterPath path;
                    path.moveTo(9, 11);
                    path.lineTo(5, 6);
                    path.lineTo(13, 6);
                    painter->fillPath(path, QBrush(symbolColor));

                }
                else if ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) {
                    painter->setPen( symbol_pen );
                    painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 6 ) );
                    painter->setPen( Qt::NoPen );
                    QPainterPath path;
                    path.moveTo(9, 7);
                    path.lineTo(5, 12);
                    path.lineTo(13, 12);
                    painter->fillPath(path, QBrush(symbolColor));
                }
                break;

            }

            case DecorationButtonType::KeepBelow:
            {
                const QColor button_color = QColor(255, 137, 241);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( Qt::NoPen );

                  QPainterPath path;
                  path.moveTo(9, 12);
                  path.lineTo(5, 6);
                  path.lineTo(13, 6);
                  painter->fillPath(path, QBrush(symbolColor));
                }
                break;

            }

            case DecorationButtonType::KeepAbove:
            {
                const QColor button_color = QColor(135, 206, 249);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( Qt::NoPen );

                  QPainterPath path;
                  path.moveTo(9, 6);
                  path.lineTo(5, 12);
                  path.lineTo(13, 12);
                  painter->fillPath(path, QBrush(symbolColor));
                }
                break;
            }

            case DecorationButtonType::ApplicationMenu:
            {
                auto d = qobject_cast<Decoration*>( decoration() );

                const QColor matchedTitleBarColor(d->client().data()->palette().color(QPalette::Window));

                const QColor titleBarColor ( isMatchTitleBarColor ? matchedTitleBarColor : d->titleBarColor() );

                QColor menuSymbolColor;
                if ( qGray(titleBarColor.rgb()) > 128 )
                    menuSymbolColor = darkSymbolColor;
                else
                    menuSymbolColor = lightSymbolColor;

                QPen menuSymbol_pen( menuSymbolColor );
                menuSymbol_pen.setJoinStyle( Qt::MiterJoin );
                menuSymbol_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                painter->setPen( menuSymbol_pen );

                painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );

                break;
            }

            case DecorationButtonType::ContextHelp:
            {
                const QColor button_color = QColor(102, 156, 246);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || isChecked() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  QPainterPath path;
                  path.moveTo( 6, 6 );
                  path.arcTo( QRectF( 5.5, 4, 7.5, 4.5 ), 180, -180 );
                  path.cubicTo( QPointF(11, 9), QPointF( 9, 6 ), QPointF( 9, 10 ) );
                  painter->drawPath( path );
                  painter->drawPoint( 9, 13 );
                }
                break;
            }

            default: break;

        }
    }

    //__________________________________________________________________
    void Button::drawIconBreezeStyle( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        painter->scale( width/20, width/20 );
        painter->translate( 1, 1 );

        // render background
        const QColor backgroundColor( this->backgroundColor() );
        if( backgroundColor.isValid() )
        {
            painter->setPen( Qt::NoPen );
            painter->setBrush( backgroundColor );
            painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
        }

        // render mark
        const QColor foregroundColor( this->foregroundColor() );
        if( foregroundColor.isValid() )
        {

            // setup painter
            QPen pen( foregroundColor );
            pen.setCapStyle( Qt::RoundCap );
            pen.setJoinStyle( Qt::MiterJoin );
            pen.setWidthF( 1.1*qMax((qreal)1.0, 20/width ) );

            painter->setPen( pen );
            painter->setBrush( Qt::NoBrush );

            switch( type() )
            {

                case DecorationButtonType::Close:
                {
                    painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 13 ) );
                    painter->drawLine( 13, 5, 5, 13 );
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                    if( isChecked() )
                    {
                        pen.setJoinStyle( Qt::RoundJoin );
                        painter->setPen( pen );

                        painter->drawPolygon( QVector<QPointF>{
                            QPointF( 4, 9 ),
                            QPointF( 9, 4 ),
                            QPointF( 14, 9 ),
                            QPointF( 9, 14 )} );

                    } else {
                        painter->drawPolyline( QVector<QPointF>{
                            QPointF( 4, 11 ),
                            QPointF( 9, 6 ),
                            QPointF( 14, 11 )});
                    }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                    painter->drawPolyline( QVector<QPointF>{
                        QPointF( 4, 7 ),
                        QPointF( 9, 12 ),
                        QPointF( 14, 7 ) });
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    painter->setPen( Qt::NoPen );
                    painter->setBrush( foregroundColor );

                    if( isChecked())
                    {

                        // outer ring
                        painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );

                        // center dot
                        QColor backgroundColor( this->backgroundColor() );
                        auto d = qobject_cast<Decoration*>( decoration() );
                        if( !backgroundColor.isValid() && d ) backgroundColor = d->titleBarColor();

                        if( backgroundColor.isValid() )
                        {
                            painter->setBrush( backgroundColor );
                            painter->drawEllipse( QRectF( 8, 8, 2, 2 ) );
                        }

                    } else {

                        painter->drawPolygon( QVector<QPointF> {
                            QPointF( 6.5, 8.5 ),
                            QPointF( 12, 3 ),
                            QPointF( 15, 6 ),
                            QPointF( 9.5, 11.5 )} );

                        painter->setPen( pen );
                        painter->drawLine( QPointF( 5.5, 7.5 ), QPointF( 10.5, 12.5 ) );
                        painter->drawLine( QPointF( 12, 6 ), QPointF( 4.5, 13.5 ) );
                    }
                    break;
                }

                case DecorationButtonType::Shade:
                {

                    if (isChecked())
                    {

                        painter->drawLine( 4, 5, 14, 5 );
                        painter->drawPolyline( QVector<QPointF> {
                            QPointF( 4, 8 ),
                            QPointF( 9, 13 ),
                            QPointF( 14, 8 )} );

                    } else {

                        painter->drawLine( 4, 5, 14, 5 );
                        painter->drawPolyline(  QVector<QPointF> {
                            QPointF( 4, 13 ),
                            QPointF( 9, 8 ),
                            QPointF( 14, 13 ) });
                    }

                    break;

                }

                case DecorationButtonType::KeepBelow:
                {

                    painter->drawPolyline(  QVector<QPointF> {
                        QPointF( 4, 5 ),
                        QPointF( 9, 10 ),
                        QPointF( 14, 5 ) });

                    painter->drawPolyline(  QVector<QPointF> {
                        QPointF( 4, 9 ),
                        QPointF( 9, 14 ),
                        QPointF( 14, 9 ) });
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    painter->drawPolyline(  QVector<QPointF> {
                        QPointF( 4, 9 ),
                        QPointF( 9, 4 ),
                        QPointF( 14, 9 ) });

                    painter->drawPolyline(  QVector<QPointF> {
                        QPointF( 4, 13 ),
                        QPointF( 9, 8 ),
                        QPointF( 14, 13 ) });
                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                    painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                    painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QPainterPath path;
                    path.moveTo( 5, 6 );
                    path.arcTo( QRectF( 5, 3.5, 8, 5 ), 180, -180 );
                    path.cubicTo( QPointF(12.5, 9.5), QPointF( 9, 7.5 ), QPointF( 9, 11.5 ) );
                    painter->drawPath( path );

                    painter->drawPoint( 9, 15 );

                    break;
                }

                default: break;

            }

        }

    }

    //__________________________________________________________________
    void Button::drawIconAdwaitaStyle( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        painter->scale( width/20, width/20 );
        painter->translate( 1, 1 );

        // render background
        const QColor backgroundColor( this->backgroundColor() );
        if( backgroundColor.isValid() )
        {
            painter->setPen( Qt::NoPen );
            painter->setBrush( backgroundColor );
            painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
        }

        // render mark
        const QColor foregroundColor( this->foregroundColor() );
        if( foregroundColor.isValid() )
        {

            // setup painter
            QPen pen( foregroundColor );
            pen.setJoinStyle( Qt::MiterJoin );
            pen.setWidthF( 2.0*qMax((qreal)1.0, 20/width ) );

            painter->setPen( pen );
            painter->setBrush( Qt::NoBrush );

            switch( type() )
            {

                case DecorationButtonType::Close:
                {
                    painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 13 ) );
                    painter->drawLine( 13, 5, 5, 13 );
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                    if( isChecked() )
                        painter->drawRect( 7, 7, 4, 4 );
                    else
                        painter->drawRect( 5, 5, 8, 8 );
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                    painter->drawLine( QPointF( 5, 12 ), QPointF( 12, 12 ) );
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    painter->setPen( Qt::NoPen );
                    painter->setBrush( foregroundColor );

                    QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                    if( isChecked()) {
                        painter->drawEllipse( c, 9.0, 9.0 );
                        painter->setBrush( backgroundColor );
                        painter->drawEllipse( c, 2.0, 2.0 );
                    }
                    else
                        painter->drawEllipse( c, 4.0, 4.0 );
                    break;
                }

                case DecorationButtonType::Shade:
                {

                    if (isChecked())
                    {

                        painter->drawLine( 5, 6, 13, 6 );
                        QPainterPath path;
                        path.moveTo(9, 13);
                        path.lineTo(4, 9);
                        path.lineTo(14, 9);
                        painter->fillPath(path, QBrush(foregroundColor));


                    } else {

                        painter->drawLine( 5, 6, 13, 6 );
                        QPainterPath path;
                        path.moveTo(9, 9);
                        path.lineTo(4, 13);
                        path.lineTo(14, 13);
                        painter->fillPath(path, QBrush(foregroundColor));

                    }

                    break;

                }

                case DecorationButtonType::KeepBelow:
                {

                    painter->setPen( Qt::NoPen );
                    painter->setBrush( foregroundColor );

                    QPainterPath path;
                    path.moveTo(9, 14);
                    path.lineTo(4, 6);
                    path.lineTo(14, 6);
                    painter->fillPath(path, QBrush(foregroundColor));

                    break;

                }

                case DecorationButtonType::KeepAbove:
                {

                    painter->setPen( Qt::NoPen );
                    painter->setBrush( foregroundColor );

                    QPainterPath path;
                    path.moveTo(9, 5);
                    path.lineTo(4, 13);
                    path.lineTo(14, 13);
                    painter->fillPath(path, QBrush(foregroundColor));

                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                    painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                    painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QPainterPath path;
                    path.moveTo( 5, 6 );
                    path.arcTo( QRectF( 5, 3.5, 8, 5 ), 180, -180 );
                    path.cubicTo( QPointF(12.5, 9.5), QPointF( 9, 7.5 ), QPointF( 9, 11.5 ) );
                    painter->drawPath( path );

                    painter->drawPoint( 9, 15 );

                    break;
                }

                default: break;

            }

        }

    }

    //__________________________________________________________________
    void Button::drawIconDarkAuroraeStyle( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        painter->scale( width/20, width/20 );
        painter->translate( 1, 1 );

        auto d = qobject_cast<Decoration*>( decoration() );
        bool inactiveWindow( d && !d->client().data()->isActive() );
        bool useActiveButtonStyle( d && d->internalSettings()->buttonStyle() == 6 );
        bool useInactiveButtonStyle( d && d->internalSettings()->buttonStyle() == 7 );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );

        const QColor darkSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(81, 102, 107) : QColor(34, 45, 50) );
        const QColor lightSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(192, 193, 194) : QColor(250, 251, 252) );

        // symbols color

        QColor symbolColor( this->autoColor( inactiveWindow, useActiveButtonStyle, useInactiveButtonStyle, isMatchTitleBarColor, darkSymbolColor, lightSymbolColor ) );

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        symbol_pen.setWidthF( 1.2*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                const QColor button_color = QColor(242, 80, 86);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && isHovered() )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  // it's a cross
                  painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 13 ) );
                  painter->drawLine( QPointF( 5, 13 ), QPointF( 13, 5 ) );
                }
                break;
            }

            case DecorationButtonType::Maximize:
            {
                const QColor button_color = QColor(19, 209, 61);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && isHovered() )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {

                  painter->setPen( symbol_pen );

                  // solid vs. open rectangle
                  if( isChecked() )
                  {
                      painter->drawLine( QPointF( 4.5, 6 ), QPointF( 13.5, 6 ) );
                      painter->drawLine( QPointF( 13.5, 6 ), QPointF( 13.5, 12 ) );
                      painter->drawLine( QPointF( 4.5, 6 ), QPointF( 4.5, 12 ) );
                      painter->drawLine( QPointF( 4.5, 12 ), QPointF( 13.5, 12 ) );
                  }
                  else
                  {
                      painter->drawLine( QPointF( 4.5, 4.5 ), QPointF( 13.5, 4.5 ) );
                      painter->drawLine( QPointF( 13.5, 4.5 ), QPointF( 13.5, 9 ) );
                      painter->drawLine( QPointF( 4.5, 9 ), QPointF( 4.5, 13.5 ) );
                      painter->drawLine( QPointF( 4.5, 13.5 ), QPointF( 13.5, 13.5 ) );
                  }

                }
                break;
            }

            case DecorationButtonType::Minimize:
            {
                const QColor button_color = QColor(252, 190, 7);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && isHovered() )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                }
                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                const QColor button_color = QColor(125, 209, 200);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( !isChecked() && ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) )
                {
                  painter->setPen( symbol_pen );

                  painter->drawLine( QPointF( 7, 5 ), QPointF( 15, 5 ) );
                  painter->drawLine( QPointF( 15, 5 ), QPointF( 15, 13 ) );
                  painter->drawLine( QPointF( 7, 5 ), QPointF( 7, 13 ) );
                  painter->drawLine( QPointF( 7, 13 ), QPointF( 15, 13 ) );

                  painter->drawLine( QPointF( 3, 5 ), QPointF( 3, 13 ) );
                  painter->drawLine( QPointF( 3, 5 ), QPointF( 4.5, 5 ) );
                  painter->drawLine( QPointF( 3, 13 ), QPointF( 4.5, 13 ) );
                }
                else if ( isChecked() && ( isHovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) )
                {
                  painter->setPen( symbol_pen );

                  painter->drawLine( QPointF( 5, 5 ), QPointF( 11, 5 ) );
                  painter->drawLine( QPointF( 11, 5 ), QPointF( 11, 11 ) );
                  painter->drawLine( QPointF( 5, 5 ), QPointF( 5, 11 ) );
                  painter->drawLine( QPointF( 5, 11 ), QPointF( 11, 11 ) );

                  painter->drawLine( QPointF( 7, 7 ), QPointF( 13, 7 ) );
                  painter->drawLine( QPointF( 13, 7 ), QPointF( 13, 13 ) );
                  painter->drawLine( QPointF( 7, 7 ), QPointF( 7, 13 ) );
                  painter->drawLine( QPointF( 7, 13 ), QPointF( 13, 13 ) );
                }
                break;
            }

            case DecorationButtonType::Shade:
            {
                const QColor button_color = QColor(204, 176, 213);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 4, 6 ), QPointF( 14, 6 ) );

                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 8, 10, 2, 2 ) );
                }
                break;

            }

            case DecorationButtonType::KeepBelow:
            {
                const QColor button_color = QColor(255, 137, 241);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  painter->drawPolyline( QVector<QPointF>{
                           QPointF( 4, 7 ),
                           QPointF( 9, 12 ),
                           QPointF( 14, 7 ) });
                }
                break;

            }

            case DecorationButtonType::KeepAbove:
            {
                const QColor button_color = QColor(135, 206, 249);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  painter->drawPolyline( QVector<QPointF>{
                           QPointF( 4, 11 ),
                           QPointF( 9, 6 ),
                           QPointF( 14, 11 )});
                }
                break;
            }

            case DecorationButtonType::ApplicationMenu:
            {
                auto d = qobject_cast<Decoration*>( decoration() );

                const QColor matchedTitleBarColor(d->client().data()->palette().color(QPalette::Window));

                const QColor titleBarColor ( isMatchTitleBarColor ? matchedTitleBarColor : d->titleBarColor() );

                QColor menuSymbolColor;
                if ( qGray(titleBarColor.rgb()) > 128 )
                    menuSymbolColor = darkSymbolColor;
                else
                    menuSymbolColor = lightSymbolColor;

                QPen menuSymbol_pen( menuSymbolColor );
                menuSymbol_pen.setJoinStyle( Qt::MiterJoin );
                menuSymbol_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                painter->setPen( menuSymbol_pen );

                painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );

                break;
            }

            case DecorationButtonType::ContextHelp:
            {
                const QColor button_color = QColor(102, 156, 246);

                QPen button_pen( button_color );
                button_pen.setJoinStyle( Qt::MiterJoin );
                button_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( isHovered() || isChecked() ) )
                {
                  // ring
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( button_pen );
                }
                else if ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  // nothing
                  painter->setBrush( Qt::NoBrush );
                  painter->setPen( Qt::NoPen );
                }
                else
                {
                  // filled
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                }
                qreal r = static_cast<qreal>(7)
                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( isHovered() || isChecked() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  int startAngle = 260 * 16;
                  int spanAngle = 280 * 16;
                  painter->drawArc( QRectF( 5, 3, 8, 8), startAngle, spanAngle );

                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 8, 14, 2, 2 ) );
                }
                break;
            }

            default: break;

        }
    }

    //__________________________________________________________________
    QColor Button::foregroundColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );
        const QColor matchedTitleBarColor(d->client().data()->palette().color(QPalette::Window));
        const QColor titleBarColor ( isMatchTitleBarColor ? matchedTitleBarColor : d->titleBarColor() );

        if( !d ) {

            return QColor();

        } else if( isPressed() ) {

            return titleBarColor;

        } else if( type() == DecorationButtonType::Close && d->internalSettings()->outlineCloseButton() ) {

            return titleBarColor;

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove || type() == DecorationButtonType::Shade ) && isChecked() ) {

            return titleBarColor;

        } else if( m_animation->state() == QPropertyAnimation::Running ) {

            return KColorUtils::mix( d->fontColor(), titleBarColor, m_opacity );

        } else if( isHovered() ) {

            return titleBarColor;

        } else {

            return d->fontColor();

        }

    }

    //__________________________________________________________________
    QColor Button::backgroundColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        if( !d ) {

            return QColor();

        }

        auto c = d->client().data();
        if( isPressed() ) {

            if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground );
            else return KColorUtils::mix( d->titleBarColor(), d->fontColor(), 0.3 );

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove || type() == DecorationButtonType::Shade ) && isChecked() ) {

            return d->fontColor();

        } else if( m_animation->state() == QPropertyAnimation::Running ) {

            if( type() == DecorationButtonType::Close )
            {
                if( d->internalSettings()->outlineCloseButton() )
                {

                    return KColorUtils::mix( d->fontColor(), c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter(), m_opacity );

                } else {

                    QColor color( c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter() );
                    color.setAlpha( color.alpha()*m_opacity );
                    return color;

                }

            } else {

                QColor color( d->fontColor() );
                color.setAlpha( color.alpha()*m_opacity );
                return color;

            }

        } else if( isHovered() ) {

            if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter();
            else return d->fontColor();

        } else if( type() == DecorationButtonType::Close && d->internalSettings()->outlineCloseButton() ) {

            return d->fontColor();

        } else {

            return QColor();

        }

    }

    //__________________________________________________________________
    QColor Button::autoColor( const bool inactiveWindow, const bool useActiveButtonStyle, const bool useInactiveButtonStyle, const bool isMatchTitleBarColor, const QColor darkSymbolColor, const QColor lightSymbolColor ) const
    {
        QColor col;

        if ( useActiveButtonStyle || ( !inactiveWindow && !useInactiveButtonStyle ) )
            col = darkSymbolColor;
        else
        {
            auto d = qobject_cast<Decoration*>( decoration() );

            const QColor matchedTitleBarColor(d->client().data()->palette().color(QPalette::Window));

            const QColor titleBarColor ( isMatchTitleBarColor ? matchedTitleBarColor : d->titleBarColor() );

            if ( qGray(titleBarColor.rgb()) > 128 )
                col = darkSymbolColor;
            else
                col = lightSymbolColor;
        }
        return col;
    }

    //________________________________________________________________
    void Button::reconfigure()
    {

        // animation
        auto d = qobject_cast<Decoration*>(decoration());
        if( d )  m_animation->setDuration( d->internalSettings()->animationsDuration() );

    }

    //__________________________________________________________________
    void Button::updateAnimationState( bool hovered )
    {

        auto d = qobject_cast<Decoration*>(decoration());
        if( !(d && d->internalSettings()->animationsEnabled() ) ) return;

        QAbstractAnimation::Direction dir = hovered ? QPropertyAnimation::Forward : QPropertyAnimation::Backward;
        if( m_animation->state() == QPropertyAnimation::Running && m_animation->direction() != dir )
            m_animation->stop();
        m_animation->setDirection( dir );
        if( m_animation->state() != QPropertyAnimation::Running ) m_animation->start();

    }

} // namespace
