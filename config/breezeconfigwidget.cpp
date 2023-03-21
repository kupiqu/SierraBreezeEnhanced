 //////////////////////////////////////////////////////////////////////////////
// breezeconfigurationui.cpp
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

#include "breezeconfigwidget.h"
#include "breezeexceptionlist.h"
#include "breezesettings.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>

namespace Breeze
{

    //_________________________________________________________
    ConfigWidget::ConfigWidget( QWidget* parent, const QVariantList &args ):
        KCModule(parent, args),
        m_configuration( KSharedConfig::openConfig( QStringLiteral( "sierrabreezeenhancedrc" ) ) ),
        m_changed( false )
    {

        // configuration
        m_ui.setupUi( this );

        // track ui changes
        connect( m_ui.titleAlignment, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.buttonSize, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.buttonSpacing, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );
        connect( m_ui.buttonPadding, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.hOffset, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.unisonHovering, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.cornerRadiusSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );
        connect( m_ui.drawBorderOnMaximizedWindows, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.drawSizeGrip, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.opaqueTitleBar, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.drawBackgroundGradient, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.buttonStyle, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.opacitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );
        connect( m_ui.gradientSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );
        connect( m_ui.drawTitleBarSeparator, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.hideTitleBar, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.matchColorForTitleBar, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.systemForegroundColor, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );

        // track animations changes
        connect( m_ui.animationsEnabled, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.animationsDuration, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );

        // track shadows changes
        connect( m_ui.shadowSize, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.shadowStrength, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.shadowColor, &KColorButton::changed, this, &ConfigWidget::updateChanged );
        connect( m_ui.specificShadowsInactiveWindows, &QAbstractButton::clicked, this, &ConfigWidget::updateChanged );
        connect( m_ui.shadowSizeInactiveWindows, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.shadowStrengthInactiveWindows, SIGNAL(valueChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.shadowColorInactiveWindows, &KColorButton::changed, this, &ConfigWidget::updateChanged );

        // track exception changes
        connect( m_ui.exceptions, &ExceptionListWidget::changed, this, &ConfigWidget::updateChanged );

    }

    //_________________________________________________________
    void ConfigWidget::load()
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->load();

        // assign to ui
        m_ui.titleAlignment->setCurrentIndex( m_internalSettings->titleAlignment() );
        m_ui.buttonSize->setCurrentIndex( m_internalSettings->buttonSize() );
        m_ui.buttonSpacing->setValue( m_internalSettings->buttonSpacing() );
        m_ui.buttonPadding->setValue( m_internalSettings->buttonPadding() );
        m_ui.hOffset->setValue( m_internalSettings->hOffset() );
        m_ui.unisonHovering->setChecked( m_internalSettings->unisonHovering() );
        m_ui.cornerRadiusSpinBox->setValue( m_internalSettings->cornerRadius() );
        m_ui.drawBorderOnMaximizedWindows->setChecked( m_internalSettings->drawBorderOnMaximizedWindows() );
        m_ui.drawSizeGrip->setChecked( m_internalSettings->drawSizeGrip() );
        m_ui.opaqueTitleBar->setChecked( m_internalSettings->opaqueTitleBar() );
        m_ui.drawBackgroundGradient->setChecked( m_internalSettings->drawBackgroundGradient() );
        m_ui.animationsEnabled->setChecked( m_internalSettings->animationsEnabled() );
        m_ui.animationsDuration->setValue( m_internalSettings->animationsDuration() );
        m_ui.buttonStyle->setCurrentIndex ( m_internalSettings->buttonStyle() );
        m_ui.opacitySpinBox->setValue( m_internalSettings->backgroundOpacity() );
        m_ui.gradientSpinBox->setValue( m_internalSettings->backgroundGradientIntensity() );
        m_ui.drawTitleBarSeparator->setChecked( m_internalSettings->drawTitleBarSeparator() );
        m_ui.hideTitleBar->setCurrentIndex( m_internalSettings->hideTitleBar() );
        m_ui.matchColorForTitleBar->setChecked( m_internalSettings->matchColorForTitleBar() );
        m_ui.systemForegroundColor->setChecked( m_internalSettings->systemForegroundColor() );

        // load shadows
        if( m_internalSettings->shadowSize() <= InternalSettings::ShadowVeryLarge ) m_ui.shadowSize->setCurrentIndex( m_internalSettings->shadowSize() );
        else m_ui.shadowSize->setCurrentIndex( InternalSettings::ShadowLarge );
        m_ui.shadowStrength->setValue( qRound(qreal(m_internalSettings->shadowStrength()*100)/255 ) );
        m_ui.shadowColor->setColor( m_internalSettings->shadowColor() );

        m_ui.specificShadowsInactiveWindows->setChecked( m_internalSettings->specificShadowsInactiveWindows() );

        if( m_internalSettings->shadowSizeInactiveWindows() <= InternalSettings::ShadowVeryLargeInactiveWindows ) m_ui.shadowSizeInactiveWindows->setCurrentIndex( m_internalSettings->shadowSizeInactiveWindows() );
        else m_ui.shadowSizeInactiveWindows->setCurrentIndex( InternalSettings::ShadowLargeInactiveWindows );
        m_ui.shadowStrengthInactiveWindows->setValue( qRound(qreal(m_internalSettings->shadowStrengthInactiveWindows()*100)/255 ) );
        m_ui.shadowColorInactiveWindows->setColor( m_internalSettings->shadowColorInactiveWindows() );

        // load exceptions
        ExceptionList exceptions;
        exceptions.readConfig( m_configuration );
        m_ui.exceptions->setExceptions( exceptions.get() );
        setChanged( false );

    }

    //_________________________________________________________
    void ConfigWidget::save()
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->load();

        // apply modifications from ui
        m_internalSettings->setTitleAlignment( m_ui.titleAlignment->currentIndex() );
        m_internalSettings->setButtonSize( m_ui.buttonSize->currentIndex() );
        m_internalSettings->setButtonSpacing( m_ui.buttonSpacing->value() );
        m_internalSettings->setButtonPadding( m_ui.buttonPadding->value() );
        m_internalSettings->setHOffset( m_ui.hOffset->value() );
        m_internalSettings->setUnisonHovering( m_ui.unisonHovering->isChecked() );
        m_internalSettings->setCornerRadius( m_ui.cornerRadiusSpinBox->value() );
        m_internalSettings->setDrawBorderOnMaximizedWindows( m_ui.drawBorderOnMaximizedWindows->isChecked() );
        m_internalSettings->setDrawSizeGrip( m_ui.drawSizeGrip->isChecked() );
        m_internalSettings->setOpaqueTitleBar( m_ui.opaqueTitleBar->isChecked() );
        m_internalSettings->setDrawBackgroundGradient( m_ui.drawBackgroundGradient->isChecked() );
        m_internalSettings->setAnimationsEnabled( m_ui.animationsEnabled->isChecked() );
        m_internalSettings->setAnimationsDuration( m_ui.animationsDuration->value() );
        m_internalSettings->setButtonStyle( m_ui.buttonStyle->currentIndex() );
        m_internalSettings->setBackgroundOpacity(m_ui.opacitySpinBox->value());
        m_internalSettings->setBackgroundGradientIntensity(m_ui.gradientSpinBox->value());
        m_internalSettings->setDrawTitleBarSeparator(m_ui.drawTitleBarSeparator->isChecked());
        m_internalSettings->setHideTitleBar( m_ui.hideTitleBar->currentIndex() );
        m_internalSettings->setMatchColorForTitleBar( m_ui.matchColorForTitleBar->isChecked() );
        m_internalSettings->setSystemForegroundColor( m_ui.systemForegroundColor->isChecked() );

        m_internalSettings->setShadowSize( m_ui.shadowSize->currentIndex() );
        m_internalSettings->setShadowStrength( qRound( qreal(m_ui.shadowStrength->value()*255)/100 ) );
        m_internalSettings->setShadowColor( m_ui.shadowColor->color() );

        m_internalSettings->setSpecificShadowsInactiveWindows( m_ui.specificShadowsInactiveWindows->isChecked() );

        m_internalSettings->setShadowSizeInactiveWindows( m_ui.shadowSizeInactiveWindows->currentIndex() );
        m_internalSettings->setShadowStrengthInactiveWindows( qRound( qreal(m_ui.shadowStrengthInactiveWindows->value()*255)/100 ) );
        m_internalSettings->setShadowColorInactiveWindows( m_ui.shadowColorInactiveWindows->color() );

        // save configuration
        m_internalSettings->save();

        // get list of exceptions and write
        InternalSettingsList exceptions( m_ui.exceptions->exceptions() );
        ExceptionList( exceptions ).writeConfig( m_configuration );

        // sync configuration
        m_configuration->sync();
        setChanged( false );

        // needed to tell kwin to reload when running from external kcmshell
        {
            QDBusMessage message = QDBusMessage::createSignal("/KWin", "org.kde.KWin", "reloadConfig");
            QDBusConnection::sessionBus().send(message);
        }

        // needed for breeze style to reload shadows
        {
            QDBusMessage message( QDBusMessage::createSignal("/BreezeDecoration",  "org.kde.Breeze.Style", "reparseConfiguration") );
            QDBusConnection::sessionBus().send(message);
        }

    }

    //_________________________________________________________
    void ConfigWidget::defaults()
    {

        // create internal settings and load from rc files
        m_internalSettings = InternalSettingsPtr( new InternalSettings() );
        m_internalSettings->setDefaults();

        // assign to ui
        m_ui.titleAlignment->setCurrentIndex( m_internalSettings->titleAlignment() );
        m_ui.buttonSize->setCurrentIndex( m_internalSettings->buttonSize() );
        m_ui.buttonSpacing->setValue( m_internalSettings->buttonSpacing() );
        m_ui.buttonPadding->setValue( m_internalSettings->buttonPadding() );
        m_ui.hOffset->setValue( m_internalSettings->hOffset() );
        m_ui.unisonHovering->setChecked( m_internalSettings->unisonHovering() );
        m_ui.cornerRadiusSpinBox->setValue( m_internalSettings->cornerRadius() );
        m_ui.drawBorderOnMaximizedWindows->setChecked( m_internalSettings->drawBorderOnMaximizedWindows() );
        m_ui.drawSizeGrip->setChecked( m_internalSettings->drawSizeGrip() );
        m_ui.opaqueTitleBar->setChecked( m_internalSettings->opaqueTitleBar() );
        m_ui.drawBackgroundGradient->setChecked( m_internalSettings->drawBackgroundGradient() );
        m_ui.drawTitleBarSeparator->setChecked( m_internalSettings->drawTitleBarSeparator() );
        m_ui.hideTitleBar->setCurrentIndex( m_internalSettings->hideTitleBar() );
        m_ui.matchColorForTitleBar->setChecked( m_internalSettings->matchColorForTitleBar() );
        m_ui.systemForegroundColor->setChecked( m_internalSettings->systemForegroundColor() );

        m_ui.animationsEnabled->setChecked( m_internalSettings->animationsEnabled() );
        m_ui.animationsDuration->setValue( m_internalSettings->animationsDuration() );
        m_ui.buttonStyle->setCurrentIndex( m_internalSettings->buttonStyle() );
        m_ui.opacitySpinBox->setValue( m_internalSettings->backgroundOpacity() );
        m_ui.gradientSpinBox->setValue( m_internalSettings->backgroundGradientIntensity() );

        m_ui.shadowSize->setCurrentIndex( m_internalSettings->shadowSize() );
        m_ui.shadowStrength->setValue( qRound(qreal(m_internalSettings->shadowStrength()*100)/255 ) );
        m_ui.shadowColor->setColor( m_internalSettings->shadowColor() );

        m_ui.specificShadowsInactiveWindows->setChecked( m_internalSettings->specificShadowsInactiveWindows() );

        m_ui.shadowSizeInactiveWindows->setCurrentIndex( m_internalSettings->shadowSizeInactiveWindows() );
        m_ui.shadowStrengthInactiveWindows->setValue( qRound(qreal(m_internalSettings->shadowStrengthInactiveWindows()*100)/255 ) );
        m_ui.shadowColorInactiveWindows->setColor( m_internalSettings->shadowColorInactiveWindows() );

    }

    //_______________________________________________
    void ConfigWidget::updateChanged()
    {

        // check configuration
        if( !m_internalSettings ) return;

        // track modifications
        bool modified( false );

        if( m_ui.titleAlignment->currentIndex() != m_internalSettings->titleAlignment() ) modified = true;
        else if( m_ui.buttonSize->currentIndex() != m_internalSettings->buttonSize() ) modified = true;
        else if( m_ui.buttonSpacing->value() != m_internalSettings->buttonSpacing() ) modified = true;
        else if ( m_ui.buttonPadding->value() != m_internalSettings->buttonPadding() ) modified = true;
        else if ( m_ui.hOffset->value() != m_internalSettings->hOffset() ) modified = true;
        else if( m_ui.unisonHovering->isChecked() != m_internalSettings->unisonHovering() ) modified = true;
        else if( m_ui.cornerRadiusSpinBox->value() != m_internalSettings->cornerRadius() ) modified = true;
        else if( m_ui.drawBorderOnMaximizedWindows->isChecked() !=  m_internalSettings->drawBorderOnMaximizedWindows() ) modified = true;
        else if( m_ui.drawSizeGrip->isChecked() !=  m_internalSettings->drawSizeGrip() ) modified = true;
        else if( m_ui.opaqueTitleBar->isChecked() !=  m_internalSettings->opaqueTitleBar() ) modified = true;
        else if( m_ui.drawBackgroundGradient->isChecked() !=  m_internalSettings->drawBackgroundGradient() ) modified = true;
        else if( m_ui.buttonStyle->currentIndex() != m_internalSettings->buttonStyle() ) modified = true;
        else if( m_ui.opacitySpinBox->value() != m_internalSettings->backgroundOpacity() ) modified = true;
        else if( m_ui.gradientSpinBox->value() != m_internalSettings->backgroundGradientIntensity() ) modified = true;
        else if (m_ui.drawTitleBarSeparator->isChecked() != m_internalSettings->drawTitleBarSeparator()) modified = true;
        else if ( m_ui.hideTitleBar->currentIndex() != m_internalSettings->hideTitleBar() ) modified = true;
        else if ( m_ui.matchColorForTitleBar->isChecked() != m_internalSettings->matchColorForTitleBar() ) modified = true;
        else if ( m_ui.systemForegroundColor->isChecked() != m_internalSettings->systemForegroundColor() ) modified = true;

        // animations
        else if( m_ui.animationsEnabled->isChecked() !=  m_internalSettings->animationsEnabled() ) modified = true;
        else if( m_ui.animationsDuration->value() != m_internalSettings->animationsDuration() ) modified = true;

        // shadows
        else if( m_ui.shadowSize->currentIndex() !=  m_internalSettings->shadowSize() ) modified = true;
        else if( qRound( qreal(m_ui.shadowStrength->value()*255)/100 ) != m_internalSettings->shadowStrength() ) modified = true;
        else if( m_ui.shadowColor->color() != m_internalSettings->shadowColor() ) modified = true;

        else if( m_ui.specificShadowsInactiveWindows->isChecked() != m_internalSettings->specificShadowsInactiveWindows() ) modified = true;

        else if( m_ui.shadowSizeInactiveWindows->currentIndex() !=  m_internalSettings->shadowSizeInactiveWindows() ) modified = true;
        else if( qRound( qreal(m_ui.shadowStrengthInactiveWindows->value()*255)/100 ) != m_internalSettings->shadowStrengthInactiveWindows() ) modified = true;
        else if( m_ui.shadowColorInactiveWindows->color() != m_internalSettings->shadowColorInactiveWindows() ) modified = true;

        // exceptions
        else if( m_ui.exceptions->isChanged() ) modified = true;


        setChanged( modified );

    }

    //_______________________________________________
    void ConfigWidget::setChanged( bool value )
    {
        emit changed( value );
    }

}
