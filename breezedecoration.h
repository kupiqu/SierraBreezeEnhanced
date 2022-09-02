#ifndef BREEZE_DECORATION_H
#define BREEZE_DECORATION_H

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

#include "breeze.h"
#include "breezesettings.h"

#include <KDecoration2/Decoration>
#include <KDecoration2/DecoratedClient>
#include <KDecoration2/DecorationSettings>

#include <QPalette>
#include <QVariant>
#include <QVariantAnimation>
#include <QPainterPath>

class QVariantAnimation;

namespace KDecoration2
{
    class DecorationButton;
    class DecorationButtonGroup;
}

namespace Breeze
{
    class SizeGrip;
    class Button;
    class Decoration : public KDecoration2::Decoration
    {
        Q_OBJECT

        public:

        //* constructor
        explicit Decoration(QObject *parent = nullptr, const QVariantList &args = QVariantList());

        //* destructor
        virtual ~Decoration();

        //* paint
        void paint(QPainter *painter, const QRect &repaintRegion) override;

        //* internal settings
        InternalSettingsPtr internalSettings() const
        { return m_internalSettings; }

        //* caption height
        int captionHeight() const;

        //* button height
        int buttonHeight() const;

        //*@name active state change animation
        //@{
        void setOpacity( qreal );

        qreal opacity() const
        { return m_opacity; }

        //@}

        //*@name colors
        //@{
        QColor titleBarColor() const;
        QColor outlineColor() const;
        QColor rawTitleBarColor() const;
        QColor fontColor() const;
        //@}

        //*@name maximization modes
        //@{
        inline bool isMaximized() const;
        inline bool isMaximizedHorizontally() const;
        inline bool isMaximizedVertically() const;

        inline bool isLeftEdge() const;
        inline bool isRightEdge() const;
        inline bool isTopEdge() const;
        inline bool isBottomEdge() const;

        inline bool hideTitleBar() const;
        inline int titleBarAlpha() const;
        inline bool matchColorForTitleBar() const;
        inline bool drawBackgroundGradient() const;
        inline bool systemForegroundColor() const;
        //@}

        //*@Decoration has a hovered button
        //@{
        bool m_buttonHovered = false;
        bool buttonHovered() const
        { return m_buttonHovered; }

        signals:
        void buttonHoveredChanged();

        public Q_SLOTS:
        void setButtonHovered(bool value);

        protected:
        void hoverMoveEvent(QHoverEvent *event) override;
        //@}


        public Q_SLOTS:
        void init() override;

        private Q_SLOTS:
        void reconfigure();
        void recalculateBorders();
        void updateButtonsGeometry();
        void updateButtonsGeometryDelayed();
        void updateTitleBar();
        void updateAnimationState();
        void updateSizeGripVisibility();
        void updateBlur();
        void createShadow();

        private:

        //* return the rect in which caption will be drawn
        QPair<QRect,Qt::Alignment> captionRect() const;

        void createButtons();
        void paintTitleBar(QPainter *painter, const QRect &repaintRegion);
        void updateShadow();
        void updateActiveShadow();
        void updateInactiveShadow();
        void calculateWindowAndTitleBarShapes(const bool windowShapeOnly=false);

        //*@name border size
        //@{
        int borderSize(bool bottom = false) const;
        inline bool hasBorders() const;
        inline bool hasNoBorders() const;
        inline bool hasNoSideBorders() const;
        //@}

        //*@name size grip
        //@{
        void createSizeGrip();
        void deleteSizeGrip();
        SizeGrip* sizeGrip() const
        { return m_sizeGrip; }
        //@}

        InternalSettingsPtr m_internalSettings;
        KDecoration2::DecorationButtonGroup *m_leftButtons = nullptr;
        KDecoration2::DecorationButtonGroup *m_rightButtons = nullptr;

        //* size grip widget
        SizeGrip *m_sizeGrip = nullptr;

        //* active state change animation
        QVariantAnimation *m_animation;

        //* active state change opacity
        qreal m_opacity = 0;

        //* Rectangular area of titlebar without clipped corners
        QRect m_titleRect;
        
        //* Exact titlebar path, with clipped rounded corners
        std::shared_ptr<QPainterPath> m_titleBarPath = std::make_shared<QPainterPath>();
        //* Exact window path, with clipped rounded corners
        std::shared_ptr<QPainterPath> m_windowPath = std::make_shared<QPainterPath>();
    };

    bool Decoration::hasBorders() const
    {
        if( m_internalSettings && m_internalSettings->mask() & BorderSize ) return m_internalSettings->borderSize() > InternalSettings::BorderNoSides;
        else return settings()->borderSize() > KDecoration2::BorderSize::NoSides;
    }

    bool Decoration::hasNoBorders() const
    {
        if( m_internalSettings && m_internalSettings->mask() & BorderSize ) return m_internalSettings->borderSize() == InternalSettings::BorderNone;
        else return settings()->borderSize() == KDecoration2::BorderSize::None;
    }

    bool Decoration::hasNoSideBorders() const
    {
        if( m_internalSettings && m_internalSettings->mask() & BorderSize ) return m_internalSettings->borderSize() == InternalSettings::BorderNoSides;
        else return settings()->borderSize() == KDecoration2::BorderSize::NoSides;
    }

    bool Decoration::isMaximized() const
    { return client().toStrongRef().data()->isMaximized() && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isMaximizedHorizontally() const
    { return client().toStrongRef().data()->isMaximizedHorizontally() && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isMaximizedVertically() const
    { return client().toStrongRef().data()->isMaximizedVertically() && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isLeftEdge() const
    { return (client().toStrongRef().data()->isMaximizedHorizontally() || client().toStrongRef().data()->adjacentScreenEdges().testFlag( Qt::LeftEdge ) ) && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isRightEdge() const
    { return (client().toStrongRef().data()->isMaximizedHorizontally() || client().toStrongRef().data()->adjacentScreenEdges().testFlag( Qt::RightEdge ) ) && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isTopEdge() const
    { return (client().toStrongRef().data()->isMaximizedVertically() || client().toStrongRef().data()->adjacentScreenEdges().testFlag( Qt::TopEdge ) ) && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::isBottomEdge() const
    { return (client().toStrongRef().data()->isMaximizedVertically() || client().toStrongRef().data()->adjacentScreenEdges().testFlag( Qt::BottomEdge ) ) && !m_internalSettings->drawBorderOnMaximizedWindows(); }

    bool Decoration::hideTitleBar() const
    { return m_internalSettings->hideTitleBar() == 3 || ( m_internalSettings->hideTitleBar() == 1 && client().toStrongRef().data()->isMaximized() ) || ( m_internalSettings->hideTitleBar() == 2 && ( client().toStrongRef().data()->isMaximized() || client().toStrongRef().data()->isMaximizedVertically()  || client().toStrongRef().data()->isMaximizedHorizontally()) ); }

    int Decoration::titleBarAlpha() const
    {
        if (m_internalSettings->opaqueTitleBar())
            return 255;
        int a = m_internalSettings->opacityOverride() > -1 ? m_internalSettings->opacityOverride() : m_internalSettings->backgroundOpacity();
        a =  qBound(0, a, 100);
        return qRound(static_cast<qreal>(a) * static_cast<qreal>(2.55));
    }

    bool Decoration::matchColorForTitleBar() const
    { return m_internalSettings->matchColorForTitleBar(); }

    bool Decoration::drawBackgroundGradient() const
    { return m_internalSettings->drawBackgroundGradient(); }

    bool Decoration::systemForegroundColor() const
    { return m_internalSettings->systemForegroundColor(); }
}

#endif
