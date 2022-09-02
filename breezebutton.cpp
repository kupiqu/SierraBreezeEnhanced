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
#include <KIconLoader>

#include <QPainter>
#include <QPainterPath>

namespace Breeze
{

    using KDecoration2::ColorRole;
    using KDecoration2::ColorGroup;
    using KDecoration2::DecorationButtonType;


    //__________________________________________________________________
    Button::Button(DecorationButtonType type, Decoration* decoration, QObject* parent)
        : DecorationButton(type, decoration, parent)
        , m_animation( new QVariantAnimation( this ) )
    {

        // setup animation
        // It is important start and end value are of the same type, hence 0.0 and not just 0
        m_animation->setStartValue( 0.0 );
        m_animation->setEndValue( 1.0 );
        // Linear to have the same easing as Breeze animations
        m_animation->setEasingCurve( QEasingCurve::Linear );
        connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            setOpacity(value.toReal());
        });

        // setup default geometry
        const int height = decoration->buttonHeight();
        setGeometry(QRect(0, 0, height, height));
        setIconSize(QSize( height, height ));

        // connections
        connect(decoration->client().toStrongRef().data(), SIGNAL(iconChanged(QIcon)), this, SLOT(update()));
        connect(decoration->settings().data(), &KDecoration2::DecorationSettings::reconfigured, this, &Button::reconfigure);
        connect( this, &KDecoration2::DecorationButton::hoveredChanged, this, &Button::updateAnimationState );

        if (decoration->objectName() == "applet-window-buttons") {
            connect( this, &Button::hoveredChanged, [=](bool hovered){
                    decoration->setButtonHovered(hovered);
                    });
        }
        connect(decoration, SIGNAL(buttonHoveredChanged()), this, SLOT(update()));

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
                b->setVisible( d->client().toStrongRef().data()->isCloseable() );
                QObject::connect(d->client().toStrongRef().data(), &KDecoration2::DecoratedClient::closeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Maximize:
                b->setVisible( d->client().toStrongRef().data()->isMaximizeable() );
                QObject::connect(d->client().toStrongRef().data(), &KDecoration2::DecoratedClient::maximizeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Minimize:
                b->setVisible( d->client().toStrongRef().data()->isMinimizeable() );
                QObject::connect(d->client().toStrongRef().data(), &KDecoration2::DecoratedClient::minimizeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::ContextHelp:
                b->setVisible( d->client().toStrongRef().data()->providesContextHelp() );
                QObject::connect(d->client().toStrongRef().data(), &KDecoration2::DecoratedClient::providesContextHelpChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Shade:
                b->setVisible( d->client().toStrongRef().data()->isShadeable() );
                QObject::connect(d->client().toStrongRef().data(), &KDecoration2::DecoratedClient::shadeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Menu:
                QObject::connect(d->client().toStrongRef().data(), &KDecoration2::DecoratedClient::iconChanged, b, [b]() { b->update(); });
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

        if( !m_iconSize.isValid() || isStandAlone() ) m_iconSize = geometry().size().toSize();

        // menu button
        if (type() == DecorationButtonType::Menu)
        {

            const QRectF iconRect( geometry().topLeft(), 0.8*m_iconSize );
            const qreal width( m_iconSize.width() );
            painter->translate( 0.1*width, 0.1*width );
            if (auto deco =  qobject_cast<Decoration*>(decoration())) {
              const QPalette activePalette = KIconLoader::global()->customPalette();
              QPalette palette = decoration()->client().toStrongRef().data()->palette();
              palette.setColor(QPalette::Foreground, deco->fontColor());
              KIconLoader::global()->setCustomPalette(palette);
              decoration()->client().toStrongRef().data()->icon().paint(painter, iconRect.toRect());
              if (activePalette == QPalette()) {
                KIconLoader::global()->resetPalette();
              }    else {
                KIconLoader::global()->setCustomPalette(palette);
              }
            } else {
              decoration()->client().toStrongRef().data()->icon().paint(painter, iconRect.toRect());
            }

        } else {

            auto d = qobject_cast<Decoration*>( decoration() );

            if ( d && d->internalSettings()->buttonStyle() == 0 )
                drawIconPlasma( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 1 )
                drawIconGnome( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 2 )
                drawIconMacSierra( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 3 )
                drawIconMacDarkAurorae( painter );
            else if ( d && ( d->internalSettings()->buttonStyle() == 4 || d->internalSettings()->buttonStyle() == 5 || d->internalSettings()->buttonStyle() == 6 ) )
                drawIconSBEsierra( painter );
            else if ( d && ( d->internalSettings()->buttonStyle() == 7 || d->internalSettings()->buttonStyle() == 8 || d->internalSettings()->buttonStyle() == 9 ) )
                drawIconSBEdarkAurorae( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 10 )
                drawIconSierraColorSymbols( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 11 )
                drawIconDarkAuroraeColorSymbols( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 12 )
                drawIconSierraMonochromeSymbols( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 13 )
                drawIconDarkAuroraeMonochromeSymbols( painter );

        }

        painter->restore();

    }

    //__________________________________________________________________
    void Button::drawIconPlasma( QPainter *painter ) const
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
        QColor backgroundColor( this->backgroundColor() );
        if( backgroundColor.isValid() )
        {
            painter->setPen( Qt::NoPen );
            painter->setBrush( backgroundColor );
            painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
        }

        // render mark
        QColor foregroundColor( this->foregroundColor() );
        if( foregroundColor.isValid() )
        {

            // setup painter
            QPen pen( foregroundColor );
            pen.setCapStyle( Qt::RoundCap );
            pen.setJoinStyle( Qt::MiterJoin );
            pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

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

                        painter->drawLine( QPointF( 4, 5.5 ), QPointF( 14, 5.5 ) ); // painter->drawLine( 4, 5, 14, 5 );
                        painter->drawPolyline( QVector<QPointF> {
                            QPointF( 4, 8 ),
                            QPointF( 9, 13 ),
                            QPointF( 14, 8 )} );

                    } else {

                        painter->drawLine( QPointF( 4, 5.5 ), QPointF( 14, 5.5 ) ); // painter->drawLine( 4, 5, 14, 5 );
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
                    painter->drawRect( QRectF( 3.5, 4.5, 11, 1 ) ); // painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                    painter->drawRect( QRectF( 3.5, 8.5, 11, 1 ) ); // painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                    painter->drawRect( QRectF( 3.5, 12.5, 11, 1 ) ); // painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QPainterPath path;
                    path.moveTo( 5, 6 );
                    path.arcTo( QRectF( 5, 3.5, 8, 5 ), 180, -180 );
                    path.cubicTo( QPointF(12.5, 9.5), QPointF( 9, 7.5 ), QPointF( 9, 11.5 ) );
                    painter->drawPath( path );

                    painter->drawRect( QRectF( 9, 15, 0.5, 0.5 ) ); // painter->drawPoint( 9, 15 );

                    break;
                }

                default: break;
            }

        }

    }

    //__________________________________________________________________
    void Button::drawIconGnome( QPainter *painter ) const
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

        // render background
        QColor backgroundColor;
        if ( isChecked() || this->hovered() || isHovered() )
            backgroundColor = d->titleBarColor();
        else
            backgroundColor = QColor();

        if( backgroundColor.isValid() )
        {
          if ( qGray(backgroundColor.rgb()) > 69 ) {
            painter->setPen(backgroundColor.darker(150));

            QLinearGradient gradient( 0, 0, 0, width );
            int b = 10;
            if ( isChecked() && isHovered() ) {
              backgroundColor = backgroundColor.darker(115);
              gradient.setColorAt(0.0, backgroundColor.lighter( 100 + 2*b ));
              gradient.setColorAt(1.0, backgroundColor);
            }
            else if ( isChecked() ) {
              backgroundColor = backgroundColor.darker(115);
              gradient.setColorAt(0.0, backgroundColor.lighter( 100 + b ));
              gradient.setColorAt(1.0, backgroundColor);
            }
            else if ( this->hovered() ) {
              backgroundColor = backgroundColor.darker(115);
              gradient.setColorAt(0.0, backgroundColor.lighter( 100 + 3*b ));
              gradient.setColorAt(1.0, backgroundColor);
            }
            painter->setBrush(gradient);
            painter->drawRoundedRect( QRectF( -1, -1, 19, 19 ), 1, 1);
          }
          else{
            painter->setPen(backgroundColor.lighter(180));

            QLinearGradient gradient( 0, 0, 0, width );
            int b = 40;
            if ( isChecked() && isHovered() ) {
              backgroundColor = backgroundColor.lighter(130);
              gradient.setColorAt(0.0, backgroundColor.lighter( 100 + b ));
              gradient.setColorAt(1.0, backgroundColor.darker ( 120));
            }
            else if ( isChecked() ) {
              backgroundColor = backgroundColor.lighter(110);
              gradient.setColorAt(0.0, backgroundColor.lighter( 100 + b ));
              gradient.setColorAt(1.0, backgroundColor.darker ( 120 ));
            }
            else if ( this->hovered() ) {
              backgroundColor = backgroundColor.lighter(150);
              gradient.setColorAt(0.0, backgroundColor.lighter( 100 + b ));
              gradient.setColorAt(1.0, backgroundColor.darker ( 120 ));
            }
            painter->setBrush(gradient);
            painter->drawRoundedRect( QRectF( -1, -1, 19, 19 ), 1, 1);
          }
        }

        // render mark
        QColor foregroundColor = d->fontColor();
        if( foregroundColor.isValid() )
        {
            // setup painter
            QPen pen( foregroundColor );
            pen.setJoinStyle( Qt::MiterJoin );
            pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

            switch( type() )
            {

                case DecorationButtonType::Close:
                {
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

                    painter->drawLine( QPointF( 6.5, 6.5 ), QPointF( 11.5, 11.5 ) );
                    painter->drawLine( QPointF( 11.5, 6.5 ), QPointF( 6.5, 11.5 ) );
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

                    if( isChecked() )
                        painter->drawRect(QRectF(7.5, 7.5, 3, 3));
                    else
                        painter->drawRect(QRectF(6.5, 6.5, 5, 5));
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

                    painter->drawLine( QPointF( 6.5, 11.5 ), QPointF( 11.5, 11.5 ) );
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {

                    painter->setPen( Qt::NoPen );
                    painter->setBrush( foregroundColor );

                    QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                    if( isChecked()) {
                        painter->drawEllipse( c, 1.0, 1.0 );
                    }
                    else {
                        painter->drawEllipse( c, 2.0, 2.0 );
                    }
                    break;
                }

                case DecorationButtonType::Shade:
                {

                    painter->setPen( pen );

                    if (isChecked())
                    {

                        painter->drawLine( 6, 12.5, 12, 12.5 );

                        QPainterPath path;
                        path.moveTo(9, 12.5);
                        path.lineTo(5, 6.5);
                        path.lineTo(13, 6.5);
                        painter->fillPath(path, QBrush(foregroundColor));


                    } else {

                        painter->drawLine( 6, 7.5, 12, 7.5 );

                        QPainterPath path;
                        path.moveTo(9, 7.5);
                        path.lineTo(5, 12.5);
                        path.lineTo(13, 12.5);
                        painter->fillPath(path, QBrush(foregroundColor));

                    }

                    break;

                }

                case DecorationButtonType::KeepBelow:
                {

                    painter->setPen( Qt::NoPen );

                    QPainterPath path;
                    path.moveTo(9, 11.5);
                    path.lineTo(5, 6.5);
                    path.lineTo(13, 6.5);
                    painter->fillPath(path, QBrush(foregroundColor));

                    break;

                }

                case DecorationButtonType::KeepAbove:
                {

                    painter->setPen( Qt::NoPen );

                    QPainterPath path;
                    path.moveTo(9, 6.5);
                    path.lineTo(5, 11.5);
                    path.lineTo(13, 11.5);
                    painter->fillPath(path, QBrush(foregroundColor));

                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    painter->setPen( pen );
                    painter->setBrush( Qt::NoBrush );

                    painter->drawLine( QPointF( 6.5, 6.5 ), QPointF( 11.5, 6.5 ) );
                    painter->drawLine( QPointF( 6.5, 9 ), QPointF( 11.5, 9 ) );
                    painter->drawLine( QPointF( 6.5, 11.5 ), QPointF( 11.5, 11.5 ) );
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    painter->setPen( pen );

                    int startAngle = 260 * 16;
                    int spanAngle = 280 * 16;
                    painter->drawArc( QRectF( 7, 5.5, 4, 4), startAngle, spanAngle );

                    painter->setBrush( foregroundColor );

                    QPointF c = QPointF (static_cast<qreal>(9), static_cast<qreal>(12));
                    painter->drawEllipse( c, 0.5, 0.5 );

                    break;
                }

                default: break;
            }

        }

    }

    //__________________________________________________________________
    void Button::drawIconMacSierra( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        auto d = qobject_cast<Decoration*>( decoration() );
        if ( d->internalSettings()->animationsEnabled() ) {
          painter->scale( width/20, width/20 );
          painter->translate( 1, 1 );
        }
        else {
          painter->scale( 7./9.*width/20, 7./9.*width/20 );
          painter->translate( 4, 4 );
        }

        bool inactiveWindow( d && !d->client().toStrongRef().data()->isActive() );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );

        QColor darkSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(81, 102, 107) : QColor(34, 45, 50) );
        QColor lightSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(192, 193, 194) : QColor(250, 251, 252) );

        QColor titleBarColor (d->titleBarColor());

        // symbols color

        QColor symbolColor;
        bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
        if (isSystemForegroundColor)
          symbolColor = this->fontColor();
        else {
          if ( inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
            symbolColor = lightSymbolColor;
          else if ( inactiveWindow && qGray(titleBarColor.rgb()) > 128 )
            symbolColor = darkSymbolColor;
          else
            symbolColor = this->autoColor( false, true, false, darkSymbolColor, lightSymbolColor );
        }

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        if ( d->internalSettings()->animationsEnabled() )
          symbol_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );
        else
          symbol_pen.setWidthF( 9./7.*1.7*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                QColor button_color;
                if ( !inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(238, 102, 90);
                else if( !inactiveWindow )
                  button_color = QColor(255, 94, 88);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() )
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
                QColor button_color;
                if ( !inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 196, 86);
                else if( !inactiveWindow )
                  button_color = QColor(40, 200, 64);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() )
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
                QColor button_color;
                if ( !inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(223, 192, 76);
                else if( !inactiveWindow )
                  button_color = QColor(255, 188, 48);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() )
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                }
                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(125, 209, 200);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() )
                {
                  painter->setPen( Qt::NoPen );
                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 6, 6, 6, 6 ) );
                }
                break;
            }

            case DecorationButtonType::Shade:
            {
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(204, 176, 213);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( isChecked() )
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
                else if ( this->hovered() ) {
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
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(255, 137, 241);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() )
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
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(135, 206, 249);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() )
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
                QColor menuSymbolColor;
                bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
                if (isSystemForegroundColor)
                  menuSymbolColor = this->fontColor();
                else {
                  uint r = qRed(titleBarColor.rgb());
                  uint g = qGreen(titleBarColor.rgb());
                  uint b = qBlue(titleBarColor.rgb());
                  // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
                  // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
                  // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
                  qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
                  if ( colorConditional > 186 || g > 186 )
                    menuSymbolColor = darkSymbolColor;
                  else
                    menuSymbolColor = lightSymbolColor;
                }

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
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(102, 156, 246);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() )
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
    void Button::drawIconMacDarkAurorae( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        auto d = qobject_cast<Decoration*>( decoration() );
        if ( d->internalSettings()->animationsEnabled() ) {
          painter->scale( width/20, width/20 );
          painter->translate( 1, 1 );
        }
        else {
          painter->scale( 7./9.*width/20, 7./9.*width/20 );
          painter->translate( 4, 4 );
        }

        bool inactiveWindow( d && !d->client().toStrongRef().data()->isActive() );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );

        QColor darkSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(81, 102, 107) : QColor(34, 45, 50) );
        QColor lightSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(192, 193, 194) : QColor(250, 251, 252) );

        QColor titleBarColor (d->titleBarColor());

        // symbols color

        QColor symbolColor;
        bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
        if (isSystemForegroundColor)
          symbolColor = this->fontColor();
        else {
          if ( inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
              symbolColor = lightSymbolColor;
          else if ( inactiveWindow && qGray(titleBarColor.rgb()) > 128 )
              symbolColor = darkSymbolColor;
          else
              symbolColor = this->autoColor( false, true, false, darkSymbolColor, lightSymbolColor );
        }

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        if ( d->internalSettings()->animationsEnabled() )
          symbol_pen.setWidthF( 1.2*qMax((qreal)1.0, 20/width ) );
        else
          symbol_pen.setWidthF( 9./7.*1.2*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                QColor button_color;
                if ( !inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(238, 102, 90);
                else if( !inactiveWindow )
                  button_color = QColor(255, 94, 88);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() )
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
                QColor button_color;
                if ( !inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 196, 86);
                else if( !inactiveWindow )
                  button_color = QColor(40, 200, 64);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() )
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
                QColor button_color;
                if ( !inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(223, 192, 76);
                else if( !inactiveWindow )
                  button_color = QColor(255, 188, 48);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() )
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                }
                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(125, 209, 200);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() && !isChecked() )
                {
                  painter->setPen( symbol_pen );

                  if ( !isHovered() && d->internalSettings()->animationsEnabled() ) {
                    painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 5 ) );
                    painter->drawLine( QPointF( 13, 5 ), QPointF( 13, 13 ) );
                    painter->drawLine( QPointF( 5, 5 ), QPointF( 5, 13 ) );
                    painter->drawLine( QPointF( 5, 13 ), QPointF( 13, 13 ) );

                    painter->setBrush(QBrush(symbolColor));
                    painter->drawEllipse( c, 0.5, 0.5 );
                  }
                  else {
                    painter->drawLine( QPointF( 7, 5 ), QPointF( 15, 5 ) );
                    painter->drawLine( QPointF( 15, 5 ), QPointF( 15, 13 ) );
                    painter->drawLine( QPointF( 7, 5 ), QPointF( 7, 13 ) );
                    painter->drawLine( QPointF( 7, 13 ), QPointF( 15, 13 ) );

                    painter->drawLine( QPointF( 3, 5 ), QPointF( 3, 13 ) );
                    painter->drawLine( QPointF( 3, 5 ), QPointF( 4.5, 5 ) );
                    painter->drawLine( QPointF( 3, 13 ), QPointF( 4.5, 13 ) );
                  }

                }
                else if ( isChecked() )
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
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(204, 176, 213);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( isChecked() )
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 4, 12 ), QPointF( 14, 12 ) );

                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 8, 6, 2, 2 ) );
                }
                else if ( this->hovered() )
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
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(255, 137, 241);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() )
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
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(135, 206, 249);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() )
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
                QColor menuSymbolColor;
                bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
                if (isSystemForegroundColor)
                  menuSymbolColor = this->fontColor();
                else {
                  uint r = qRed(titleBarColor.rgb());
                  uint g = qGreen(titleBarColor.rgb());
                  uint b = qBlue(titleBarColor.rgb());
                  // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
                  // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
                  // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
                  qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
                  if ( colorConditional > 186 || g > 186 )
                    menuSymbolColor = darkSymbolColor;
                  else
                    menuSymbolColor = lightSymbolColor;
                }

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
                QColor button_color;
                if ( !inactiveWindow )
                  button_color = QColor(102, 156, 246);
                else if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 100, 100);
                else
                  button_color = QColor(200, 200, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                painter->setBrush( button_color );
                painter->setPen( button_pen );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() )
                {
                  painter->setPen( symbol_pen );
                  int startAngle = 260 * 16;
                  int spanAngle = 280 * 16;
                  painter->drawArc( QRectF( 6, 4, 6, 6), startAngle, spanAngle );

                  painter->setBrush(QBrush(symbolColor));
                  r = static_cast<qreal>(1);
                  c = QPointF (static_cast<qreal>(9), static_cast<qreal>(13));
                  painter->drawEllipse( c, r, r );
                }
                break;
            }

            default: break;
        }
    }

    //__________________________________________________________________
    void Button::drawIconSBEsierra( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        auto d = qobject_cast<Decoration*>( decoration() );
        if ( d->internalSettings()->animationsEnabled() ) {
          painter->scale( width/20, width/20 );
          painter->translate( 1, 1 );
        }
        else {
          painter->scale( 7./9.*width/20, 7./9.*width/20 );
          painter->translate( 4, 4 );
        }

        bool inactiveWindow( d && !d->client().toStrongRef().data()->isActive() );
        bool useActiveButtonStyle( d && d->internalSettings()->buttonStyle() == 5 );
        bool useInactiveButtonStyle( d && d->internalSettings()->buttonStyle() == 6 );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );

        QColor darkSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(81, 102, 107) : QColor(34, 45, 50) );
        QColor lightSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(192, 193, 194) : QColor(250, 251, 252) );

        QColor titleBarColor (d->titleBarColor());

        // symbols color

        QColor symbolColor;
        bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
        if (isSystemForegroundColor)
          symbolColor = this->fontColor();
        else
          symbolColor = this->autoColor( inactiveWindow, useActiveButtonStyle, useInactiveButtonStyle, darkSymbolColor, lightSymbolColor );

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        if ( d->internalSettings()->animationsEnabled() )
          symbol_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );
        else
          symbol_pen.setWidthF( 9./7.*1.7*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                QColor button_color;
                if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(238, 102, 90);
                else
                  button_color = QColor(255, 94, 88);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && this->hovered() )
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
                  painter->setPen( button_pen );
                }
                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor button_color;
                if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 196, 86);
                else
                  button_color = QColor(40, 200, 64);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && this->hovered() )
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
                  painter->setPen( button_pen );
                }
                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor button_color;
                if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(223, 192, 76);
                else
                  button_color = QColor(255, 188, 48);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && this->hovered() )
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
                  painter->setPen( button_pen );
                }
                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                }
                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                QColor button_color = QColor(125, 209, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( Qt::NoPen );
                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 6, 6, 6, 6 ) );
                }
                break;
            }

            case DecorationButtonType::Shade:
            {
                QColor button_color = QColor(204, 176, 213);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
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
                else if ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) {
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
                QColor button_color = QColor(255, 137, 241);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor button_color = QColor(135, 206, 249);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor menuSymbolColor;
                bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
                if (isSystemForegroundColor)
                  menuSymbolColor = this->fontColor();
                else {
                  uint r = qRed(titleBarColor.rgb());
                  uint g = qGreen(titleBarColor.rgb());
                  uint b = qBlue(titleBarColor.rgb());
                  // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
                  // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
                  // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
                  qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
                  if ( colorConditional > 186 || g > 186 )
                    menuSymbolColor = darkSymbolColor;
                  else
                    menuSymbolColor = lightSymbolColor;
                }

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
                QColor button_color = QColor(102, 156, 246);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
    void Button::drawIconSBEdarkAurorae( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        auto d = qobject_cast<Decoration*>( decoration() );
        if ( d->internalSettings()->animationsEnabled() ) {
          painter->scale( width/20, width/20 );
          painter->translate( 1, 1 );
        }
        else {
          painter->scale( 7./9.*width/20, 7./9.*width/20 );
          painter->translate( 4, 4 );
        }

        bool inactiveWindow( d && !d->client().toStrongRef().data()->isActive() );
        bool useActiveButtonStyle( d && d->internalSettings()->buttonStyle() == 8 );
        bool useInactiveButtonStyle( d && d->internalSettings()->buttonStyle() == 9 );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );

        QColor darkSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(81, 102, 107) : QColor(34, 45, 50) );
        QColor lightSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(192, 193, 194) : QColor(250, 251, 252) );

        QColor titleBarColor (d->titleBarColor());

        // symbols color

        QColor symbolColor;
        bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
        if (isSystemForegroundColor)
          symbolColor = this->fontColor();
        else
          symbolColor = this->autoColor( inactiveWindow, useActiveButtonStyle, useInactiveButtonStyle, darkSymbolColor, lightSymbolColor );

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        if ( d->internalSettings()->animationsEnabled() )
          symbol_pen.setWidthF( 1.2*qMax((qreal)1.0, 20/width ) );
        else
          symbol_pen.setWidthF( 9./7.*1.2*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                QColor button_color;
                if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(238, 102, 90);
                else
                  button_color = QColor(255, 94, 88);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && this->hovered() )
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
                  painter->setPen( button_pen );
                }
                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor button_color;
                if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(100, 196, 86);
                else
                  button_color = QColor(40, 200, 64);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && this->hovered() )
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
                  painter->setPen( button_pen );
                }
                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                if ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor button_color;
                if ( qGray(titleBarColor.rgb()) < 128 )
                  button_color = QColor(223, 192, 76);
                else
                  button_color = QColor(255, 188, 48);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && this->hovered() )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                }
                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                QColor button_color = QColor(125, 209, 200);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( !isChecked() && ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) )
                {
                  painter->setPen( symbol_pen );

                  if ( !isHovered() && d->internalSettings()->animationsEnabled() ) {
                    painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 5 ) );
                    painter->drawLine( QPointF( 13, 5 ), QPointF( 13, 13 ) );
                    painter->drawLine( QPointF( 5, 5 ), QPointF( 5, 13 ) );
                    painter->drawLine( QPointF( 5, 13 ), QPointF( 13, 13 ) );

                    painter->setBrush(QBrush(symbolColor));
                    painter->drawEllipse( c, 0.5, 0.5 );
                  }
                  else {
                    painter->drawLine( QPointF( 7, 5 ), QPointF( 15, 5 ) );
                    painter->drawLine( QPointF( 15, 5 ), QPointF( 15, 13 ) );
                    painter->drawLine( QPointF( 7, 5 ), QPointF( 7, 13 ) );
                    painter->drawLine( QPointF( 7, 13 ), QPointF( 15, 13 ) );

                    painter->drawLine( QPointF( 3, 5 ), QPointF( 3, 13 ) );
                    painter->drawLine( QPointF( 3, 5 ), QPointF( 4.5, 5 ) );
                    painter->drawLine( QPointF( 3, 13 ), QPointF( 4.5, 13 ) );
                  }
                }
                else if ( isChecked() && ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) )
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
                QColor button_color = QColor(204, 176, 213);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if (isChecked())
                {
                  painter->setPen( symbol_pen );
                  painter->drawLine( QPointF( 4, 12 ), QPointF( 14, 12 ) );

                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 8, 6, 2, 2 ) );
                }
                else if ( this->hovered() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor button_color = QColor(255, 137, 241);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor button_color = QColor(135, 206, 249);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() ||  ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
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
                QColor menuSymbolColor;
                bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
                if (isSystemForegroundColor)
                  menuSymbolColor = this->fontColor();
                else {
                  uint r = qRed(titleBarColor.rgb());
                  uint g = qGreen(titleBarColor.rgb());
                  uint b = qBlue(titleBarColor.rgb());
                  // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
                  // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
                  // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
                  qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
                  if ( colorConditional > 186 || g > 186 )
                    menuSymbolColor = darkSymbolColor;
                  else
                    menuSymbolColor = lightSymbolColor;
                }

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
                QColor button_color = QColor(102, 156, 246);
                QPen button_pen( qGray(titleBarColor.rgb()) < 69 ? button_color.lighter(115) : button_color.darker(115) );
                button_pen.setJoinStyle( Qt::MiterJoin );
                if ( d->internalSettings()->animationsEnabled() )
                  button_pen.setWidthF( PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );
                else
                  button_pen.setWidthF( 9./7.*PenWidth::Symbol*qMax((qreal)1.0, 20/width ) );

                if ( ( ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle ) && ( this->hovered() || isChecked() ) )
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
                  painter->setPen( button_pen );
                }

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                if ( this->hovered() || isChecked() || ( inactiveWindow && !useActiveButtonStyle ) || useInactiveButtonStyle )
                {
                  painter->setPen( symbol_pen );
                  int startAngle = 260 * 16;
                  int spanAngle = 280 * 16;
                  painter->drawArc( QRectF( 6, 4, 6, 6), startAngle, spanAngle );

                  painter->setBrush(QBrush(symbolColor));
                  r = static_cast<qreal>(1);
                  c = QPointF (static_cast<qreal>(9), static_cast<qreal>(13));
                  painter->drawEllipse( c, r, r );
                }
                break;
            }

            default: break;
        }
    }

    //__________________________________________________________________
    void Button::drawIconSierraColorSymbols( QPainter *painter ) const
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

        bool inactiveWindow( d && !d->client().toStrongRef().data()->isActive() );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );

        QColor darkSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(81, 102, 107) : QColor(34, 45, 50) );
        QColor lightSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(192, 193, 194) : QColor(250, 251, 252) );

        QColor titleBarColor (d->titleBarColor());

        // symbols color

        QColor symbolColor;
        bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
        if (isSystemForegroundColor)
          symbolColor = this->fontColor();
        else {
          if ( inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
              symbolColor = lightSymbolColor;
          else if ( inactiveWindow && qGray(titleBarColor.rgb()) > 128 )
              symbolColor = darkSymbolColor;
          else
              symbolColor = this->autoColor( false, true, false, darkSymbolColor, lightSymbolColor );
        }

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        symbol_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                QColor  button_color = QColor(238, 102, 90);
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                // it's a cross
                painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
                painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 6 ) );

                break;
            }

            case DecorationButtonType::Maximize:
            {
                QColor button_color = QColor(100, 196, 86);
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );
                painter->setPen( Qt::NoPen );

                button_color.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);

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

                painter->fillPath(path1, QBrush(mycolor));
                painter->fillPath(path2, QBrush(mycolor));

                break;
            }

            case DecorationButtonType::Minimize:
            {
                QColor button_color = QColor(223, 192, 76);
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                // it's a horizontal line
                painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );

                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                QColor button_color = QColor(125, 209, 200);
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( !isChecked() )
                    mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                // painter->setPen( symbol_pen );
                painter->setPen( Qt::NoPen );
                painter->setBrush(QBrush(mycolor));
                painter->drawEllipse( QRectF( 6, 6, 6, 6 ) );

                break;
            }

            case DecorationButtonType::Shade:
            {
                QColor button_color = QColor(204, 176, 213);
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( !isChecked() )
                  mycolor = this->mixColors(button_color.darker (100), symbolColor, m_opacity);
                painter->setPen( mycolor );

                // it's a triangle with a dash
                if (isChecked())
                {
                    painter->setPen( symbol_pen );
                    painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 12 ) );
                    painter->setPen( Qt::NoPen );
                    QPainterPath path;
                    path.moveTo(9, 11);
                    path.lineTo(5, 6);
                    path.lineTo(13, 6);
                    painter->fillPath(path, QBrush(mycolor));

                }
                else {
                    painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 6 ) );
                    painter->setPen( Qt::NoPen );
                    QPainterPath path;
                    path.moveTo(9, 7);
                    path.lineTo(5, 12);
                    path.lineTo(13, 12);
                    painter->fillPath(path, QBrush(mycolor));
                }

                break;

            }

            case DecorationButtonType::KeepBelow:
            {
                QColor button_color = QColor(255, 137, 241);
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( !isChecked() )
                    mycolor = this->mixColors(button_color.darker (100), symbolColor, m_opacity);
                painter->setPen( Qt::NoPen );

                // it's a downward pointing triangle
                QPainterPath path;
                path.moveTo(9, 12);
                path.lineTo(5, 6);
                path.lineTo(13, 6);
                painter->fillPath(path, QBrush(mycolor));

                break;

            }

            case DecorationButtonType::KeepAbove:
            {
                QColor button_color = QColor(135, 206, 249);
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( !isChecked() )
                    mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                painter->setPen( Qt::NoPen );

                // it's a upward pointing triangle
                QPainterPath path;
                path.moveTo(9, 6);
                path.lineTo(5, 12);
                path.lineTo(13, 12);
                painter->fillPath(path, QBrush(mycolor));

                break;
            }

            case DecorationButtonType::ApplicationMenu:
            {
                QColor menuSymbolColor;
                bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
                if (isSystemForegroundColor)
                  menuSymbolColor = this->fontColor();
                else {
                  uint r = qRed(titleBarColor.rgb());
                  uint g = qGreen(titleBarColor.rgb());
                  uint b = qBlue(titleBarColor.rgb());
                  // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
                  // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
                  // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
                  qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
                  if ( colorConditional > 186 || g > 186 )
                    menuSymbolColor = darkSymbolColor;
                  else
                    menuSymbolColor = lightSymbolColor;
                }

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
                QColor button_color = QColor(102, 156, 246);
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                // it's a question mark
                QPainterPath path;
                path.moveTo( 6, 6 );
                path.arcTo( QRectF( 5.5, 4, 7.5, 4.5 ), 180, -180 );
                path.cubicTo( QPointF(11, 9), QPointF( 9, 6 ), QPointF( 9, 10 ) );
                painter->drawPath( path );
                painter->drawPoint( 9, 13 );

                break;
            }

            default: break;
        }
    }

    //__________________________________________________________________
    void Button::drawIconDarkAuroraeColorSymbols( QPainter *painter ) const
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

        bool inactiveWindow( d && !d->client().toStrongRef().data()->isActive() );
        bool isMatchTitleBarColor( d && d->internalSettings()->matchColorForTitleBar() );

        QColor darkSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(81, 102, 107) : QColor(34, 45, 50) );
        QColor lightSymbolColor( ( inactiveWindow && isMatchTitleBarColor ) ? QColor(192, 193, 194) : QColor(250, 251, 252) );

        QColor titleBarColor (d->titleBarColor());

        // symbols color

        QColor symbolColor;
        bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
        if (isSystemForegroundColor)
          symbolColor = this->fontColor();
        else {
          if ( inactiveWindow && qGray(titleBarColor.rgb()) < 128 )
              symbolColor = lightSymbolColor;
          else if ( inactiveWindow && qGray(titleBarColor.rgb()) > 128 )
              symbolColor = darkSymbolColor;
          else
              symbolColor = this->autoColor( false, true, false, darkSymbolColor, lightSymbolColor );
        }

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        symbol_pen.setWidthF( 1.2*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                QColor button_color = QColor(238, 102, 90);
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                // it's a cross
                painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 13 ) );
                painter->drawLine( QPointF( 5, 13 ), QPointF( 13, 5 ) );

                break;
            }

            case DecorationButtonType::Maximize:
            {
                QColor button_color = QColor(100, 196, 86);
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
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

                break;
            }

            case DecorationButtonType::Minimize:
            {
                QColor button_color = QColor(223, 192, 76);

                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                // it's a horizontal line
                painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );

                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                QColor button_color = QColor(125, 209, 200);
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( !isChecked() )
                  mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );

                if (isChecked())
                {
                  painter->drawLine( QPointF( 5, 5 ), QPointF( 11, 5 ) );
                  painter->drawLine( QPointF( 11, 5 ), QPointF( 11, 11 ) );
                  painter->drawLine( QPointF( 5, 5 ), QPointF( 5, 11 ) );
                  painter->drawLine( QPointF( 5, 11 ), QPointF( 11, 11 ) );

                  painter->drawLine( QPointF( 7, 7 ), QPointF( 13, 7 ) );
                  painter->drawLine( QPointF( 13, 7 ), QPointF( 13, 13 ) );
                  painter->drawLine( QPointF( 7, 7 ), QPointF( 7, 13 ) );
                  painter->drawLine( QPointF( 7, 13 ), QPointF( 13, 13 ) );
                }
                else {
                  painter->drawLine( QPointF( 7, 5 ), QPointF( 15, 5 ) );
                  painter->drawLine( QPointF( 15, 5 ), QPointF( 15, 13 ) );
                  painter->drawLine( QPointF( 7, 5 ), QPointF( 7, 13 ) );
                  painter->drawLine( QPointF( 7, 13 ), QPointF( 15, 13 ) );

                  painter->drawLine( QPointF( 3, 5 ), QPointF( 3, 13 ) );
                  painter->drawLine( QPointF( 3, 5 ), QPointF( 4.5, 5 ) );
                  painter->drawLine( QPointF( 3, 13 ), QPointF( 4.5, 13 ) );
                }
                break;
            }

            case DecorationButtonType::Shade:
            {
                QColor button_color = QColor(204, 176, 213);
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( !isChecked() )
                  mycolor = this->mixColors(button_color.darker (100), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                if (isChecked())
                {
                  painter->drawLine( QPointF( 4, 12 ), QPointF( 14, 12 ) );
                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 8, 6, 2, 2 ) );
                }
                else {
                  painter->drawLine( QPointF( 4, 6 ), QPointF( 14, 6 ) );
                  painter->setBrush(QBrush(mycolor));
                  painter->drawEllipse( QRectF( 8, 10, 2, 2 ) );
                }

                break;

            }

            case DecorationButtonType::KeepBelow:
            {
                QColor button_color = QColor(255, 137, 241);
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( !isChecked() )
                  mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                painter->drawPolyline( QVector<QPointF>{
                           QPointF( 4, 7 ),
                           QPointF( 9, 12 ),
                           QPointF( 14, 7 ) });

                break;

            }

            case DecorationButtonType::KeepAbove:
            {
                QColor button_color = QColor(135, 206, 249);
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( !isChecked() )
                  mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                painter->drawPolyline( QVector<QPointF>{
                           QPointF( 4, 11 ),
                           QPointF( 9, 6 ),
                           QPointF( 14, 11 )});

                break;
            }

            case DecorationButtonType::ApplicationMenu:
            {
                QColor menuSymbolColor;
                bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
                if (isSystemForegroundColor)
                  menuSymbolColor = this->fontColor();
                else {
                  uint r = qRed(titleBarColor.rgb());
                  uint g = qGreen(titleBarColor.rgb());
                  uint b = qBlue(titleBarColor.rgb());
                  // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
                  // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
                  // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
                  qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
                  if ( colorConditional > 186 || g > 186 )
                    menuSymbolColor = darkSymbolColor;
                  else
                    menuSymbolColor = lightSymbolColor;
                }

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
                QColor button_color = QColor(102, 156, 246);
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color.darker( 100 ), symbolColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                // it's a question mark

                painter->setPen( symbol_pen );
                int startAngle = 260 * 16;
                int spanAngle = 280 * 16;
                painter->drawArc( QRectF( 6, 4, 6, 6), startAngle, spanAngle );

                painter->setBrush(QBrush(symbolColor));
                r = static_cast<qreal>(1);
                c = QPointF (static_cast<qreal>(9), static_cast<qreal>(13));
                painter->drawEllipse( c, r, r );

                break;
            }

            default: break;
        }
    }

    //__________________________________________________________________
    void Button::drawIconSierraMonochromeSymbols( QPainter *painter ) const
    {
        painter->setRenderHints( QPainter::Antialiasing );

        /*
         *        scale painter so that its window matches QRect( -1, -1, 20, 20 )
         *        this makes all further rendering and scaling simpler
         *        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
         */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        painter->scale( width/20, width/20 );
        painter->translate( 1, 1 );

        QColor darkSymbolColor = QColor(34, 45, 50);
        QColor lightSymbolColor = QColor(250, 251, 252);

        auto d = qobject_cast<Decoration*>( decoration() );
        QColor titleBarColor (d->titleBarColor());

        QColor symbolColor;
        QColor symbolBgdColor;
        bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
        if (isSystemForegroundColor) {
          symbolColor = this->foregroundColor();
          symbolBgdColor = this->backgroundColor();
        } else {
          uint r = qRed(titleBarColor.rgb());
          uint g = qGreen(titleBarColor.rgb());
          uint b = qBlue(titleBarColor.rgb());

          // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
          // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
          // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
          qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
          if ( colorConditional > 186 || g > 186 ) {
            symbolColor = darkSymbolColor;
            symbolBgdColor = lightSymbolColor;
          }
          else {
            symbolColor = lightSymbolColor;
            symbolBgdColor = darkSymbolColor;
          }
        }

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        symbol_pen.setWidthF( 1.7*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

          case DecorationButtonType::Close:
          {
            QColor button_color = symbolColor;
            button_color.setAlpha( button_color.alpha()*m_opacity );
            painter->setPen( Qt::NoPen );
            painter->setBrush( button_color );

            qreal r = static_cast<qreal>(7)
            + static_cast<qreal>(2) * m_animation->currentValue().toReal();
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
            painter->drawEllipse( c, r, r );
            painter->setBrush( Qt::NoBrush );

            button_color.setAlpha( 255 );
            symbolBgdColor.setAlpha( 255 );
            QColor mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
            symbol_pen.setColor(mycolor);
            painter->setPen( symbol_pen );
            // it's a cross
            painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
            painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 6 ) );

            break;
          }

          case DecorationButtonType::Maximize:
          {
            QColor button_color = symbolColor;
            button_color.setAlpha( button_color.alpha()*m_opacity );
            painter->setPen( Qt::NoPen );
            painter->setBrush( button_color );

            qreal r = static_cast<qreal>(7)
            + static_cast<qreal>(2) * m_animation->currentValue().toReal();
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
            painter->drawEllipse( c, r, r );
            painter->setBrush( Qt::NoBrush );
            painter->setPen( Qt::NoPen );

            button_color.setAlpha( 255 );
            symbolBgdColor.setAlpha( 255 );
            QColor mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);

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

            painter->fillPath(path1, QBrush(mycolor));
            painter->fillPath(path2, QBrush(mycolor));

            break;
        }

        case DecorationButtonType::Minimize:
        {
          QColor button_color = symbolColor;
          button_color.setAlpha( button_color.alpha()*m_opacity );
          painter->setPen( Qt::NoPen );
          painter->setBrush( button_color );

          qreal r = static_cast<qreal>(7)
          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
          QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
          painter->drawEllipse( c, r, r );
          painter->setBrush( Qt::NoBrush );

          button_color.setAlpha( 255 );
          symbolBgdColor.setAlpha( 255 );
          QColor mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
          symbol_pen.setColor(mycolor);
          painter->setPen( symbol_pen );
          // it's a horizontal line
          painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );

          break;
        }

        case DecorationButtonType::OnAllDesktops:
        {
          QColor button_color = symbolColor;
          if ( !isChecked() )
            button_color.setAlpha( button_color.alpha()*m_opacity );
          painter->setPen( Qt::NoPen );
          painter->setBrush( button_color );

          if ( !isChecked() ) {
            qreal r = static_cast<qreal>(7)
                      + static_cast<qreal>(2) * m_animation->currentValue().toReal();
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
            painter->drawEllipse( c, r, r );
          }
          else {
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                  painter->drawEllipse( c, 9, 9 );
          }
          painter->setBrush( Qt::NoBrush );

          button_color.setAlpha( 255 );
          symbolBgdColor.setAlpha( 255 );
          QColor mycolor = symbolColor;
          if ( isChecked() && !this->hovered() )
            mycolor = this->mixColors(symbolBgdColor, button_color, m_opacity);
          else
            mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
          symbol_pen.setColor(mycolor);
          painter->setPen( Qt::NoPen );
          painter->setBrush(QBrush(mycolor));
          painter->drawEllipse( QRectF( 6, 6, 6, 6 ) );

          break;
        }

        case DecorationButtonType::Shade:
        {
          QColor button_color = symbolColor;
          if ( !isChecked() )
              button_color.setAlpha( button_color.alpha()*m_opacity );
          painter->setPen( Qt::NoPen );
          painter->setBrush( button_color );

          if ( !isChecked() ) {
            qreal r = static_cast<qreal>(7)
                      + static_cast<qreal>(2) * m_animation->currentValue().toReal();
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
            painter->drawEllipse( c, r, r );
          }
          else {
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                  painter->drawEllipse( c, 9, 9 );
          }
          painter->setBrush( Qt::NoBrush );

          button_color.setAlpha( 255 );
          symbolBgdColor.setAlpha( 255 );
          QColor mycolor = symbolColor;
          if ( isChecked() && !this->hovered() )
            mycolor = this->mixColors(symbolBgdColor, button_color, m_opacity);
          else
            mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
          symbol_pen.setColor(mycolor);
          painter->setPen( symbol_pen );
          // it's a triangle with a dash
          if (isChecked())
          {
            painter->setPen( symbol_pen );
            painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 12 ) );
            painter->setPen( Qt::NoPen );
            QPainterPath path;
            path.moveTo(9, 11);
            path.lineTo(5, 6);
            path.lineTo(13, 6);
            painter->fillPath(path, QBrush(mycolor));
          }
          else {
            painter->setPen( symbol_pen );
            painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 6 ) );
            painter->setPen( Qt::NoPen );
            QPainterPath path;
            path.moveTo(9, 7);
            path.lineTo(5, 12);
            path.lineTo(13, 12);
            painter->fillPath(path, QBrush(mycolor));
          }

          break;

        }

        case DecorationButtonType::KeepBelow:
        {
          QColor button_color = symbolColor;
          if ( !isChecked() )
            button_color.setAlpha( button_color.alpha()*m_opacity );
          painter->setPen( Qt::NoPen );
          painter->setBrush( button_color );

          if ( !isChecked() ) {
            qreal r = static_cast<qreal>(7)
                      + static_cast<qreal>(2) * m_animation->currentValue().toReal();
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
            painter->drawEllipse( c, r, r );
          }
          else {
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                  painter->drawEllipse( c, 9, 9 );
          }
          painter->setBrush( Qt::NoBrush );

          button_color.setAlpha( 255 );
          symbolBgdColor.setAlpha( 255 );
          QColor mycolor = symbolColor;
          if ( isChecked() && !this->hovered() )
            mycolor = this->mixColors(symbolBgdColor, button_color, m_opacity);
          else
            mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
          painter->setPen( Qt::NoPen );

          // it's a downward pointing triangle
          QPainterPath path;
          path.moveTo(9, 12);
          path.lineTo(5, 6);
          path.lineTo(13, 6);
          painter->fillPath(path, QBrush(mycolor));

          break;

        }

        case DecorationButtonType::KeepAbove:
        {
          QColor button_color = symbolColor;
          if ( !isChecked() )
            button_color.setAlpha( button_color.alpha()*m_opacity );
          painter->setPen( Qt::NoPen );
          painter->setBrush( button_color );

          if ( !isChecked() ) {
            qreal r = static_cast<qreal>(7)
                      + static_cast<qreal>(2) * m_animation->currentValue().toReal();
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
            painter->drawEllipse( c, r, r );
          }
          else {
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                  painter->drawEllipse( c, 9, 9 );
          }
          painter->setBrush( Qt::NoBrush );

          button_color.setAlpha( 255 );
          symbolBgdColor.setAlpha( 255 );
          QColor mycolor = symbolColor;
          if ( isChecked() && !this->hovered() )
            mycolor = this->mixColors(symbolBgdColor, button_color, m_opacity);
          else
            mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
          painter->setPen( Qt::NoPen );

          // it's a upward pointing triangle
          QPainterPath path;
          path.moveTo(9, 6);
          path.lineTo(5, 12);
          path.lineTo(13, 12);
          painter->fillPath(path, QBrush(mycolor));

          break;
        }

        case DecorationButtonType::ApplicationMenu:
        {
          painter->setPen( symbol_pen );

          painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
          painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
          painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );

          break;
        }

        case DecorationButtonType::ContextHelp:
        {
          QColor button_color = symbolColor;
          button_color.setAlpha( button_color.alpha()*m_opacity );
          painter->setPen( Qt::NoPen );
          painter->setBrush( button_color );

          if ( !isChecked() ) {
            qreal r = static_cast<qreal>(7)
                      + static_cast<qreal>(2) * m_animation->currentValue().toReal();
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
            painter->drawEllipse( c, r, r );
          }
          else {
            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                  painter->drawEllipse( c, 9, 9 );
          }
          painter->setBrush( Qt::NoBrush );

          button_color.setAlpha( 255 );
          symbolBgdColor.setAlpha( 255 );
          QColor mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
          symbol_pen.setColor(mycolor);
          painter->setPen( symbol_pen );

          // it's a question mark

          QPainterPath path;
          path.moveTo( 6, 6 );
          path.arcTo( QRectF( 5.5, 4, 7.5, 4.5 ), 180, -180 );
          path.cubicTo( QPointF(11, 9), QPointF( 9, 6 ), QPointF( 9, 10 ) );
          painter->drawPath( path );
          painter->drawPoint( 9, 13 );

          break;
        }

        default: break;
      }
    }

    //__________________________________________________________________
    void Button::drawIconDarkAuroraeMonochromeSymbols( QPainter *painter ) const
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

        QColor darkSymbolColor = QColor(34, 45, 50);
        QColor lightSymbolColor = QColor(250, 251, 252);

        auto d = qobject_cast<Decoration*>( decoration() );
        QColor titleBarColor (d->titleBarColor());

        QColor symbolColor;
        QColor symbolBgdColor;

        bool isSystemForegroundColor( d && d->internalSettings()->systemForegroundColor() );
        if (isSystemForegroundColor) {
          symbolColor = this->foregroundColor();
          symbolBgdColor = this->backgroundColor();
        } else {
          uint r = qRed(titleBarColor.rgb());
          uint g = qGreen(titleBarColor.rgb());
          uint b = qBlue(titleBarColor.rgb());

          // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
          // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
          // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
          qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
          if ( colorConditional > 186 || g > 186 ) {
            symbolColor = darkSymbolColor;
            symbolBgdColor = lightSymbolColor;
          }
          else {
            symbolColor = lightSymbolColor;
            symbolBgdColor = darkSymbolColor;
          }
        }

        // symbols pen

        QPen symbol_pen( symbolColor );
        symbol_pen.setJoinStyle( Qt::MiterJoin );
        symbol_pen.setWidthF( 1.2*qMax((qreal)1.0, 20/width ) );

        switch( type() )
        {

            case DecorationButtonType::Close:
            {
                QColor button_color = symbolColor;
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                symbolBgdColor.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                // it's a cross
                painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 13 ) );
                painter->drawLine( QPointF( 5, 13 ), QPointF( 13, 5 ) );

                break;
            }

            case DecorationButtonType::Maximize:
            {
                QColor button_color = symbolColor;
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                symbolBgdColor.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
                symbol_pen.setColor(mycolor);
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

                break;
            }

            case DecorationButtonType::Minimize:
            {
                QColor button_color = symbolColor;
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                symbolBgdColor.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                // it's a horizontal line
                painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );

                break;
            }

            case DecorationButtonType::OnAllDesktops:
            {
                QColor button_color = symbolColor;
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                symbolBgdColor.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( isChecked() && !this->hovered() )
                  mycolor = this->mixColors(symbolBgdColor, button_color, m_opacity);
                else
                  mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );

                if (isChecked())
                {
                  painter->drawLine( QPointF( 5, 5 ), QPointF( 11, 5 ) );
                  painter->drawLine( QPointF( 11, 5 ), QPointF( 11, 11 ) );
                  painter->drawLine( QPointF( 5, 5 ), QPointF( 5, 11 ) );
                  painter->drawLine( QPointF( 5, 11 ), QPointF( 11, 11 ) );

                  painter->drawLine( QPointF( 7, 7 ), QPointF( 13, 7 ) );
                  painter->drawLine( QPointF( 13, 7 ), QPointF( 13, 13 ) );
                  painter->drawLine( QPointF( 7, 7 ), QPointF( 7, 13 ) );
                  painter->drawLine( QPointF( 7, 13 ), QPointF( 13, 13 ) );
                }
                else {
                  painter->drawLine( QPointF( 7, 5 ), QPointF( 15, 5 ) );
                  painter->drawLine( QPointF( 15, 5 ), QPointF( 15, 13 ) );
                  painter->drawLine( QPointF( 7, 5 ), QPointF( 7, 13 ) );
                  painter->drawLine( QPointF( 7, 13 ), QPointF( 15, 13 ) );

                  painter->drawLine( QPointF( 3, 5 ), QPointF( 3, 13 ) );
                  painter->drawLine( QPointF( 3, 5 ), QPointF( 4.5, 5 ) );
                  painter->drawLine( QPointF( 3, 13 ), QPointF( 4.5, 13 ) );
                }
                break;
            }

            case DecorationButtonType::Shade:
            {
                QColor button_color = symbolColor;
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                symbolBgdColor.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( isChecked() && !this->hovered() )
                  mycolor = this->mixColors(symbolBgdColor, button_color, m_opacity);
                else
                  mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                if (isChecked())
                {
                  painter->drawLine( QPointF( 4, 12 ), QPointF( 14, 12 ) );
                  painter->setBrush(QBrush(symbolColor));
                  painter->drawEllipse( QRectF( 8, 6, 2, 2 ) );
                }
                else {
                  painter->drawLine( QPointF( 4, 6 ), QPointF( 14, 6 ) );
                  painter->setBrush(QBrush(mycolor));
                  painter->drawEllipse( QRectF( 8, 10, 2, 2 ) );
                }

                break;

            }

            case DecorationButtonType::KeepBelow:
            {
                QColor button_color = symbolColor;
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                symbolBgdColor.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( isChecked() && !this->hovered() )
                  mycolor = this->mixColors(symbolBgdColor, button_color, m_opacity);
                else
                  mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                painter->drawPolyline( QVector<QPointF>{
                           QPointF( 4, 7 ),
                           QPointF( 9, 12 ),
                           QPointF( 14, 7 ) });

                break;

            }

            case DecorationButtonType::KeepAbove:
            {
                QColor button_color = symbolColor;
                if ( !isChecked() )
                    button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                symbolBgdColor.setAlpha( 255 );
                QColor mycolor = symbolColor;
                if ( isChecked() && !this->hovered() )
                  mycolor = this->mixColors(symbolBgdColor, button_color, m_opacity);
                else
                  mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );
                painter->drawPolyline( QVector<QPointF>{
                           QPointF( 4, 11 ),
                           QPointF( 9, 6 ),
                           QPointF( 14, 11 )});

                break;
            }

            case DecorationButtonType::ApplicationMenu:
            {
                painter->setPen( symbol_pen );

                painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );

                break;
            }

            case DecorationButtonType::ContextHelp:
            {
                QColor button_color = symbolColor;
                button_color.setAlpha( button_color.alpha()*m_opacity );
                painter->setPen( Qt::NoPen );
                painter->setBrush( button_color );

                qreal r = this->buttonRadius();
                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                painter->drawEllipse( c, r, r );
                painter->setBrush( Qt::NoBrush );

                button_color.setAlpha( 255 );
                symbolBgdColor.setAlpha( 255 );
                QColor mycolor = this->mixColors(button_color, symbolBgdColor, m_opacity);
                symbol_pen.setColor(mycolor);
                painter->setPen( symbol_pen );

                // it's a question mark

                painter->setPen( symbol_pen );
                int startAngle = 260 * 16;
                int spanAngle = 280 * 16;
                painter->drawArc( QRectF( 6, 4, 6, 6), startAngle, spanAngle );

                painter->setBrush(QBrush(symbolColor));
                r = static_cast<qreal>(1);
                c = QPointF (static_cast<qreal>(9), static_cast<qreal>(13));
                painter->drawEllipse( c, r, r );

                break;
            }

            default: break;
        }
    }

    //__________________________________________________________________
    // https://stackoverflow.com/questions/25514812/how-to-animate-color-of-qbrush
    QColor Button::mixColors(const QColor &cstart, const QColor &cend, qreal progress) const
    {
        int sh = cstart.hsvHue();
        int eh = cend.hsvHue();
        int ss = cstart.hsvSaturation();
        int es = cend.hsvSaturation();
        int sv = cstart.value();
        int ev = cend.value();
        int hr = qAbs( sh - eh );
        int sr = qAbs( ss - es );
        int vr = qAbs( sv - ev );
        int dirh =  sh > eh ? -1 : 1;
        int dirs =  ss > es ? -1 : 1;
        int dirv =  sv > ev ? -1 : 1;

        return QColor::fromHsv( sh + dirh * progress * hr,
                                ss + dirs * progress * sr,
                                sv + dirv * progress * vr );
    }

    //__________________________________________________________________
    QColor Button::fontColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        QColor titleBarColor ( d->titleBarColor() );

        if( !d ) {

            return QColor();

        } else {

            return d->fontColor();

        }

    }

    //__________________________________________________________________
    QColor Button::foregroundColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        QColor titleBarColor ( d->titleBarColor() );

        if( !d ) {

            return QColor();

        } else if( isPressed() ) {

            return titleBarColor;

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove || type() == DecorationButtonType::Shade ) && isChecked() ) {

            return titleBarColor;

        } else if( m_animation->state() == QAbstractAnimation::Running ) {

            return KColorUtils::mix( d->fontColor(), titleBarColor, m_opacity );

        } else if( this->hovered() ) {

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

        auto c = d->client().toStrongRef().data();
        if( isPressed() ) {

            if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground );
            else return KColorUtils::mix( d->titleBarColor(), d->fontColor(), 0.3 );

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove || type() == DecorationButtonType::Shade ) && isChecked() ) {

            return d->fontColor();

        } else if( m_animation->state() == QAbstractAnimation::Running ) {

            if( type() == DecorationButtonType::Close )
            {
                QColor color( c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter() );
                color.setAlpha( color.alpha()*m_opacity );
                return color;

            } else {

                QColor color( d->fontColor() );
                color.setAlpha( color.alpha()*m_opacity );
                return color;

            }

        } else if( this->hovered() ) {

            if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter();
            else return d->fontColor();

        } else {

            return QColor();

        }

    }

    //__________________________________________________________________
    qreal Button::buttonRadius() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );

        if ( d->internalSettings()->animationsEnabled() && ( !isChecked() || ( isChecked() && type() == DecorationButtonType::Maximize ) ) ) {
          return static_cast<qreal>(7) + static_cast<qreal>(2) * m_animation->currentValue().toReal();
        }
        else
          return static_cast<qreal>(9);
    }


    //__________________________________________________________________
    QColor Button::autoColor( const bool inactiveWindow, const bool useActiveButtonStyle, const bool useInactiveButtonStyle, const QColor darkSymbolColor, const QColor lightSymbolColor ) const
    {
        QColor col;

        if ( useActiveButtonStyle || ( !inactiveWindow && !useInactiveButtonStyle ) )
            col = darkSymbolColor;
        else
        {
            auto d = qobject_cast<Decoration*>( decoration() );
            QColor titleBarColor ( d->titleBarColor() );

            uint r = qRed(titleBarColor.rgb());
            uint g = qGreen(titleBarColor.rgb());
            uint b = qBlue(titleBarColor.rgb());

            // modified from https://stackoverflow.com/questions/3942878/how-to-decide-font-color-in-white-or-black-depending-on-background-color
            // qreal titleBarLuminance = (0.2126 * static_cast<qreal>(r) + 0.7152 * static_cast<qreal>(g) + 0.0722 * static_cast<qreal>(b)) / 255.;
            // if ( titleBarLuminance >  sqrt(1.05 * 0.05) - 0.05 )
            qreal colorConditional = 0.299 * static_cast<qreal>(r) + 0.587 * static_cast<qreal>(g) + 0.114 * static_cast<qreal>(b);
            if ( colorConditional > 186 || g > 186 )
                col = darkSymbolColor;
            else
                col = lightSymbolColor;
        }
        return col;
    }

    //__________________________________________________________________
    bool Button::hovered() const
    {
      auto d = qobject_cast<Decoration*>( decoration() );
      return isHovered() || ( d->buttonHovered() && d->internalSettings()->unisonHovering() );
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
        if( !d || !d->internalSettings()->animationsEnabled() || (d->internalSettings()->buttonStyle() == 1) ) return;

        QAbstractAnimation::Direction dir = hovered ? QAbstractAnimation::Forward : QAbstractAnimation::Backward;
        if( m_animation->state() == QAbstractAnimation::Running && m_animation->direction() != dir )
            m_animation->stop();
        m_animation->setDirection( dir );
        if( m_animation->state() != QAbstractAnimation::Running ) m_animation->start();

    }

} // namespace
