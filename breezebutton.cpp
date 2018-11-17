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

            const QRectF iconRect( geometry().topLeft(), m_iconSize );
            decoration()->client().data()->icon().paint(painter, iconRect.toRect());


        } else {

            drawIcon( painter );

        }

        painter->restore();

    }

    //__________________________________________________________________
    void Button::drawIcon( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        painter->scale( width/20, width/20 );
        painter->translate( 1, 1 );

        auto d = qobject_cast<Decoration*>( decoration() );
        bool isInactive(d && !d->client().data()->isActive());
        const QColor inactiveCol = "transparent"; // no color

        // render mark
        const QColor foregroundColor( this->foregroundColor(inactiveCol) );
        if( foregroundColor.isValid() )
        {

            // setup painter

            QPen pen( foregroundColor );
            pen.setJoinStyle( Qt::MiterJoin );
            pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

            const QColor hover_hint_color( this->autoColor() );
            QPen hint_pen(hover_hint_color);
            hint_pen.setJoinStyle( Qt::MiterJoin );
            hint_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

            switch( type() )
            {

                case DecorationButtonType::Close:
                {
                    QColor button_color = QColor(242, 80, 86);
                    if ( isInactive && isHovered() )
                    {
                      // ring
                      painter->setBrush( inactiveCol );
                      painter->setPen( button_color );
                    }
                    else if ( isInactive )
                    {
                      // nothing
                      painter->setBrush( inactiveCol );
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
                    if ( isHovered() || isInactive )
                    {
                      painter->setPen( hint_pen );
                      // it's a cross
                      painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
                      painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 6 ) );
                    }
                    painter->setPen( pen );
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                    QColor button_color = QColor(19, 209, 61);
                    if ( isInactive && isHovered() )
                    {
                      // ring
                      painter->setBrush( inactiveCol );
                      painter->setPen( button_color );
                    }
                    else if ( isInactive )
                    {
                      // nothing
                      painter->setBrush( inactiveCol );
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
                    // if ( isHovered() )
                    if ( isHovered() || isInactive )
                    {
                      painter->setPen( Qt::NoPen );

                      // two triangles
                      QPainterPath path1, path2;
                      path1.moveTo(5, 13);
                      path1.lineTo(11, 13);
                      path1.lineTo(5, 7);

                      path2.moveTo(13, 5);
                      path2.lineTo(7, 5);
                      path2.lineTo(13, 11);


                      painter->fillPath(path1, QBrush(hover_hint_color));
                      painter->fillPath(path2, QBrush(hover_hint_color));
                    }
                    painter->setPen( pen );
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                    QColor button_color = QColor(252, 190, 7);
                    if ( isInactive && isHovered() )
                    {
                      // ring
                      painter->setBrush( inactiveCol );
                      painter->setPen( button_color );
                    }
                    else if ( isInactive )
                    {
                      // nothing
                      painter->setBrush( inactiveCol );
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
                    if ( isHovered() || isInactive )
                    {
                      painter->setPen( hint_pen );
                      painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                    }
                    painter->setPen( pen );
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    QColor button_color = QColor(125, 209, 200);
                    if ( isInactive && ( isHovered() || isChecked() ) )
                    {
                      // ring
                      painter->setBrush( inactiveCol );
                      painter->setPen( button_color );
                    }
                    else if ( isInactive )
                    {
                      // nothing
                      painter->setBrush( inactiveCol );
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
                    if ( isHovered() || isChecked() || isInactive )
                    {
                      painter->setPen( Qt::NoPen );
                      painter->setBrush(QBrush(hover_hint_color));
                      painter->drawEllipse( QRectF( 6, 6, 6, 6 ) );
                    }
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );
                    break;
                }

                case DecorationButtonType::Shade:
                {
                    QColor button_color = QColor(135, 206, 249);
                    if ( isInactive && ( isHovered() || isChecked() ) )
                    {
                      // ring
                      painter->setBrush( inactiveCol );
                      painter->setPen( button_color );
                    }
                    else if ( isInactive )
                    {
                      // nothing
                      painter->setBrush( inactiveCol );
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
                        painter->setPen( hint_pen );
                        painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 12 ) );
                        painter->setPen( Qt::NoPen );
                        QPainterPath path;
                        path.moveTo(9, 11);
                        path.lineTo(5, 6);
                        path.lineTo(13, 6);
                        painter->fillPath(path, QBrush(hover_hint_color));

                    }
                    else if (isHovered() || isInactive) {
                        painter->setPen( hint_pen );
                        painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 6 ) );
                        painter->setPen( Qt::NoPen );
                        QPainterPath path;
                        path.moveTo(9, 7);
                        path.lineTo(5, 12);
                        path.lineTo(13, 12);
                        painter->fillPath(path, QBrush(hover_hint_color));
                    }
                    painter->setPen( pen );
                    break;

                }

                case DecorationButtonType::KeepBelow:
                {
                    QColor button_color = QColor(255, 137, 241);
                    if ( isInactive && ( isHovered() || isChecked() ) )
                    {
                      // ring
                      painter->setBrush( inactiveCol );
                      painter->setPen( button_color );
                    }
                    else if ( isInactive )
                    {
                      // nothing
                      painter->setBrush( inactiveCol );
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
                    if ( isHovered() || isChecked() || isInactive )
                    {
                      painter->setPen( Qt::NoPen );

                      QPainterPath path;
                      path.moveTo(9, 12);
                      path.lineTo(5, 6);
                      path.lineTo(13, 6);
                      painter->fillPath(path, QBrush(hover_hint_color));
                    }
                    painter->setPen( pen );
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    QColor button_color = QColor(204, 176, 213);
                    if ( isInactive && ( isHovered() || isChecked() ) )
                    {
                      // ring
                      painter->setBrush( inactiveCol );
                      painter->setPen( button_color );
                    }
                    else if ( isInactive )
                    {
                      // nothing
                      painter->setBrush( inactiveCol );
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
                    if ( isHovered() || isChecked() || isInactive )
                    {
                      painter->setPen( Qt::NoPen );

                      QPainterPath path;
                      path.moveTo(9, 6);
                      path.lineTo(5, 12);
                      path.lineTo(13, 12);
                      painter->fillPath(path, QBrush(hover_hint_color));
                    }
                    painter->setPen( pen );
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
                    QColor button_color = QColor(102, 156, 246);
                    if ( isInactive && ( isHovered() || isChecked() ) )
                    {
                      // ring
                      painter->setBrush( inactiveCol );
                      painter->setPen( button_color );
                    }
                    else if ( isInactive )
                    {
                      // nothing
                      painter->setBrush( inactiveCol );
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
                    if ( isHovered() || isChecked() || isInactive )
                    {
                      painter->setPen( hint_pen );
                      QPainterPath path;
                      path.moveTo( 6, 6 );
                      path.arcTo( QRectF( 5.5, 4, 7.5, 4.5 ), 180, -180 );
                      path.cubicTo( QPointF(11, 9), QPointF( 9, 6 ), QPointF( 9, 10 ) );
                      painter->drawPath( path );
                      painter->drawPoint( 9, 13 );
                    }
                    painter->setPen( pen );
                    break;
                }

                default: break;

            }

        }

    }

    //__________________________________________________________________
    QColor Button::autoColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        QColor col;
        if (d && !d->client().data()->isActive())
        {
          if (qGray(d->titleBarColor().rgb()) > 100)
            col = QColor(41, 43, 50, 200);
          else
            col = QColor(214, 212, 205, 200);
        }
        return col;
    }

    //__________________________________________________________________
    QColor Button::foregroundColor(const QColor& inactiveCol) const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        QColor col;
        if (d && !d->client().data()->isActive()
            && !isHovered() && !isPressed()
            && m_animation->state() != QPropertyAnimation::Running)
        {
            int v = qGray(inactiveCol.rgb());
            if (v > 127) v -= 127;
            else v += 128;
            col = QColor(v, v, v);
        }
        else
        {
            if (d && qGray(d->titleBarColor().rgb()) > 100)
                col = QColor(250, 250, 250);
            else
                col = QColor(40, 40, 40);
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
