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
        : DecorationButton(args.at(0).value<DecorationButtonType>(), args.at(1).value<Decoration*>(), parent)
        , m_flag(FlagStandalone)
        , m_animation( new QPropertyAnimation( this ) )
    {}

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

        // render mark
        const QColor foregroundColor( this->foregroundColor() );
        if( foregroundColor.isValid() )
        {

            // setup painter
            QPen pen( foregroundColor );
            pen.setCapStyle( Qt::RoundCap );
            pen.setJoinStyle( Qt::MiterJoin );
            pen.setWidthF( 1.1*qMax((qreal)1.0, 20/width ) );

            auto d = qobject_cast<Decoration*>( decoration() );

            switch( type() )
            {

                case DecorationButtonType::Close:
                {
                    if (!d || d->internalSettings()->macOSButtons()) {
                        int a = (d && !d->client().data()->isActive())
                                ? qRound(200.0 * (1.0 - m_animation->currentValue().toReal()))
                                : 255;
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        {
                            grad.setColorAt(0, QColor(255, 92, 87, a));
                            grad.setColorAt(1, QColor(233, 84, 79, a));
                        }
                        else
                        {
                            grad.setColorAt(0, QColor(250, 100, 102, a));
                            grad.setColorAt(1, QColor(230, 92, 94, a));
                        }
                        painter->setBrush( QBrush(grad) );
                        painter->setPen( Qt::NoPen );
                        painter->drawEllipse( QRectF( 2, 2, 14, 14 ) );
                        if( backgroundColor.isValid() )
                        {
                            painter->setPen( Qt::NoPen );
                            painter->setBrush( backgroundColor );
                            qreal r = static_cast<qreal>(7)
                                      + (isPressed() ? 0.0
                                         : static_cast<qreal>(2) * m_animation->currentValue().toReal());
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse( c, r, r );
                        }
                    }
                    else {
                        if( backgroundColor.isValid() )
                        {
                            painter->setPen( Qt::NoPen );
                            painter->setBrush( backgroundColor );
                            painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                        }
                        painter->setPen( pen );
                        painter->setBrush( Qt::NoBrush );

                        painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 13 ) );
                        painter->drawLine( QPointF( 5, 13 ), QPointF( 13, 5 ) );
                    }
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                    if (!d || d->internalSettings()->macOSButtons()) {
                        int a = (d && !d->client().data()->isActive())
                                ? qRound(200.0 * (1.0 - m_animation->currentValue().toReal()))
                                : 255;
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        {
                            grad.setColorAt(0, isChecked() ? QColor(67, 198, 176, a)
                                                           : QColor(40, 211, 63, a));
                            grad.setColorAt(1, isChecked() ? QColor(60, 178, 159, a)
                                                           : QColor(36, 191, 57, a));
                        }
                        else
                        {
                            grad.setColorAt(0, isChecked() ? QColor(67, 198, 176, a)
                                                        : QColor(124, 198, 67, a));
                            grad.setColorAt(1, isChecked() ? QColor(60, 178, 159, a)
                                                        : QColor(111, 178, 60, a));
                        }
                        painter->setBrush( QBrush(grad) );
                        painter->setPen( Qt::NoPen );
                        painter->drawEllipse( QRectF( 2, 2, 14, 14 ) );
                        if( backgroundColor.isValid() )
                        {
                            painter->setPen( Qt::NoPen );
                            painter->setBrush( backgroundColor );
                            qreal r = static_cast<qreal>(7)
                                      + (isPressed() ? 0.0
                                         : static_cast<qreal>(2) * m_animation->currentValue().toReal());
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse( c, r, r );
                        }
                    }
                    else {
                        if( backgroundColor.isValid() )
                        {
                            painter->setPen( Qt::NoPen );
                            painter->setBrush( backgroundColor );
                            painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                        }
                        painter->setPen( pen );
                        painter->setBrush( Qt::NoBrush );

                        painter->drawPolyline(QPolygonF()
                                                << QPointF(5, 8) << QPointF(5, 13) << QPointF(10, 13));
                        if (isChecked())
                            painter->drawRect(QRectF(8.0, 5.0, 5.0, 5.0));
                        else {
                            painter->drawPolyline(QPolygonF()
                                                  << QPointF(8, 5) << QPointF(13, 5) << QPointF(13, 10));
                        }
                    }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                    if (!d || d->internalSettings()->macOSButtons()) {
                        int a = (d && !d->client().data()->isActive())
                                ? qRound(200.0 * (1.0 - m_animation->currentValue().toReal()))
                                : 255;
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        { // yellow isn't good with light backgrounds
                            grad.setColorAt(0, QColor(243, 176, 43, a));
                            grad.setColorAt(1, QColor(223, 162, 39, a));
                        }
                        else
                        {
                            grad.setColorAt(0, QColor(237, 198, 81, a));
                            grad.setColorAt(1, QColor(217, 181, 74, a));
                        }
                        painter->setBrush( QBrush(grad) );
                        painter->setPen( Qt::NoPen );
                        painter->drawEllipse( QRectF( 2, 2, 14, 14 ) );
                        if( backgroundColor.isValid() )
                        {
                            painter->setPen( Qt::NoPen );
                            painter->setBrush( backgroundColor );
                            qreal r = static_cast<qreal>(7)
                                      + (isPressed() ? 0.0
                                         : static_cast<qreal>(2) * m_animation->currentValue().toReal());
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse( c, r, r );
                        }
                    }
                    else {
                        if( backgroundColor.isValid() )
                        {
                            painter->setPen( Qt::NoPen );
                            painter->setBrush( backgroundColor );
                            painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                        }
                        painter->setPen( pen );
                        painter->setBrush( Qt::NoBrush );

                        painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                    }
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    painter->setPen( Qt::NoPen );
                    if( backgroundColor.isValid() )
                    {
                        painter->setBrush( backgroundColor );
                        painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                    }
                    painter->setBrush( foregroundColor );

                    if( isChecked())
                    {

                        // outer ring
                        painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );

                        // center dot
                        QColor backgroundColor( this->backgroundColor() );
                        if( !backgroundColor.isValid() && d ) backgroundColor = d->titleBarColor();

                        if( backgroundColor.isValid() )
                        {
                            painter->setBrush( backgroundColor );
                            painter->drawEllipse( QRectF( 8, 8, 2, 2 ) );
                        }

                    } else {

                        painter->drawPolygon( QPolygonF()
                            << QPointF( 6.5, 8.5 )
                            << QPointF( 12, 3 )
                            << QPointF( 15, 6 )
                            << QPointF( 9.5, 11.5 ) );

                        painter->setPen( pen );
                        painter->drawLine( QPointF( 5.5, 7.5 ), QPointF( 10.5, 12.5 ) );
                        painter->drawLine( QPointF( 12, 6 ), QPointF( 4.5, 13.5 ) );
                    }
                    break;
                }

                case DecorationButtonType::Shade:
                {
                    if( backgroundColor.isValid() )
                    {
                        painter->setPen( Qt::NoPen );
                        painter->setBrush( backgroundColor );
                        painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                    }
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

                    if (isChecked())
                    {

                        painter->drawLine( 4, 5, 14, 5 );
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 4, 8 )
                            << QPointF( 9, 13 )
                            << QPointF( 14, 8 ) );

                    } else {
                        if (!d || d->internalSettings()->macOSButtons()) {
                            painter->drawLine( 4, 5, 14, 5 );
                            painter->drawPolyline( QPolygonF()
                                << QPointF( 4, 13 )
                                << QPointF( 9, 8 )
                                << QPointF( 14, 13 ) );
                        }
                        else { // make it smaller
                            painter->drawLine( 5, 5, 13, 5 );
                            painter->drawPolyline( QPolygonF()
                                << QPointF( 5, 12 )
                                << QPointF( 9, 8 )
                                << QPointF( 13, 12 ) );
                        }
                    }

                    break;

                }

                case DecorationButtonType::KeepBelow:
                {
                    if( backgroundColor.isValid() )
                    {
                        painter->setPen( Qt::NoPen );
                        painter->setBrush( backgroundColor );
                        painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                    }
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

                    if (!d || d->internalSettings()->macOSButtons()) {
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 4, 5 )
                            << QPointF( 9, 10 )
                            << QPointF( 14, 5 ) );

                        painter->drawPolyline( QPolygonF()
                            << QPointF( 4, 9 )
                            << QPointF( 9, 14 )
                            << QPointF( 14, 9 ) );
                    }
                    else { // make it smaller
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 5, 5 )
                            << QPointF( 9, 9 )
                            << QPointF( 13, 5 ) );

                        painter->drawPolyline( QPolygonF()
                            << QPointF( 5, 9 )
                            << QPointF( 9, 13 )
                            << QPointF( 13, 9 ) );
                    }
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    if( backgroundColor.isValid() )
                    {
                        painter->setPen( Qt::NoPen );
                        painter->setBrush( backgroundColor );
                        painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                    }
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

                    if (!d || d->internalSettings()->macOSButtons()) {
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 4, 9 )
                            << QPointF( 9, 4 )
                            << QPointF( 14, 9 ) );

                        painter->drawPolyline( QPolygonF()
                            << QPointF( 4, 13 )
                            << QPointF( 9, 8 )
                            << QPointF( 14, 13 ) );
                    }
                    else { // make it smaller
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 5, 9 )
                            << QPointF( 9, 5 )
                            << QPointF( 13, 9 ) );

                        painter->drawPolyline( QPolygonF()
                            << QPointF( 5, 13 )
                            << QPointF( 9, 9 )
                            << QPointF( 13, 13 ) );
                    }
                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    if( backgroundColor.isValid() )
                    {
                        painter->setPen( Qt::NoPen );
                        painter->setBrush( backgroundColor );
                        painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                    }
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

                    painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                    painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                    painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    if( backgroundColor.isValid() )
                    {
                        painter->setPen( Qt::NoPen );
                        painter->setBrush( backgroundColor );
                        painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                    }
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

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
    QColor Button::foregroundColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        if( !d ) {

            return QColor();

        } else if( isPressed() ) {

            return d->titleBarColor();

        } else if( type() == DecorationButtonType::Close && d->internalSettings()->outlineCloseButton() ) {

            return d->titleBarColor();

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove ) && isChecked() ) {

            return d->titleBarColor();

        } else if( m_animation->state() == QPropertyAnimation::Running ) {

            return KColorUtils::mix( d->fontColor(), d->titleBarColor(), m_opacity );

        } else if( isHovered() ) {

            return d->titleBarColor();

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

        if (d->internalSettings()->macOSButtons()) {
            if( isPressed() ) {

                QColor col;
                if( type() == DecorationButtonType::Close )
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(254, 73, 66);
                    else
                        col = QColor(240, 77, 80);
                }
                else if( type() == DecorationButtonType::Maximize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = isChecked() ? QColor(0, 188, 154) : QColor(7, 201, 33);
                    else
                        col = isChecked() ? QColor(0, 188, 154) : QColor(101, 188, 34);
                }
                else if( type() == DecorationButtonType::Minimize )
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(233, 160, 13);
                    else
                        col = QColor(227, 185, 59);
                }
                if (col.isValid()) {
                    if (!d->client().data()->isActive()) col.setAlpha(200);
                    return col;
                }
                else return KColorUtils::mix( d->titleBarColor(), d->fontColor(), 0.3 );

            } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove ) && isChecked() ) {

                return d->fontColor();

            } else if( m_animation->state() == QPropertyAnimation::Running ) {

                QColor col;
                if( type() == DecorationButtonType::Close )
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(254, 95, 87);
                    else
                        col = QColor(240, 96, 97);
                }
                else if( type() == DecorationButtonType::Maximize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = isChecked() ? QColor(64, 188, 168) : QColor(39, 201, 63);
                    else
                        col = isChecked() ? QColor(64, 188, 168) : QColor(116, 188, 64);
                }
                else if( type() == DecorationButtonType::Minimize )
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(233, 172, 41);
                    else
                        col = QColor(227, 191, 78);
                }
                if (col.isValid()) {
                    if (!d->client().data()->isActive()) col.setAlpha(200);
                    return col;
                } else {

                    col = d->fontColor();
                    col.setAlpha( col.alpha()*m_opacity );
                    return col;

                }

            } else if( isHovered() ) {

                QColor col;
                if( type() == DecorationButtonType::Close )
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(254, 95, 87);
                    else
                        col = QColor(240, 96, 97);
                }
                else if( type() == DecorationButtonType::Maximize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = isChecked() ? QColor(64, 188, 168) : QColor(39, 201, 63);
                    else
                        col = isChecked() ? QColor(64, 188, 168) : QColor(116, 188, 64);
                }
                else if( type() == DecorationButtonType::Minimize )
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(233, 172, 41);
                    else
                        col = QColor(227, 191, 78);
                }
                if (col.isValid()) {
                    if (!d->client().data()->isActive()) col.setAlpha(200);
                    return col;
                }
                else return d->fontColor();

            } else if( type() == DecorationButtonType::Close && d->internalSettings()->outlineCloseButton() ) {

                QColor col(240, 96, 97);
                if (!d->client().data()->isActive()) col.setAlpha(200);
                return col;

            } else {

                return QColor();

            }
        }
        else { // as in Breeze
            auto c = d->client().data();
            if( isPressed() ) {

                if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground );
                else return KColorUtils::mix( d->titleBarColor(), d->fontColor(), 0.3 );

            } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove ) && isChecked() ) {

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
