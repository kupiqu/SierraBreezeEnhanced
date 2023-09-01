//////////////////////////////////////////////////////////////////////////////
// breezeexceptiondialog.cpp
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

#include "breezeexceptiondialog.h"
#include "breezedetectwidget.h"
#include "config-breeze.h"

namespace Breeze
{

    //___________________________________________
    ExceptionDialog::ExceptionDialog( QWidget* parent ):
        QDialog( parent )
    {

        m_ui.setupUi( this );

        connect( m_ui.buttonBox->button( QDialogButtonBox::Cancel ), &QAbstractButton::clicked, this, &QWidget::close );

        // store checkboxes from ui into list
        m_checkboxes.insert( BorderSize, m_ui.borderSizeCheckBox );

        // detect window properties
        connect( m_ui.detectDialogButton, &QAbstractButton::clicked, this, &ExceptionDialog::selectWindowProperties );

        // connections
        connect( m_ui.exceptionType, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.exceptionEditor, &QLineEdit::textChanged, this, &ExceptionDialog::updateChanged );
        connect( m_ui.borderSizeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );

        for( CheckBoxMap::iterator iter = m_checkboxes.begin(); iter != m_checkboxes.end(); ++iter )
        { connect( iter.value(), &QAbstractButton::clicked, this, &ExceptionDialog::updateChanged ); }

        connect( m_ui.hideTitleBar, SIGNAL(currentIndexChanged(int)), SLOT(updateChanged()) );
        connect( m_ui.matchColorForTitleBar, &QAbstractButton::clicked, this, &ExceptionDialog::updateChanged );
        connect( m_ui.systemForegroundColor, &QAbstractButton::clicked, this, &ExceptionDialog::updateChanged );
        connect( m_ui.drawTitleBarSeparator, &QAbstractButton::clicked, this, &ExceptionDialog::updateChanged );
        connect( m_ui.drawBackgroundGradient, &QAbstractButton::clicked, this, &ExceptionDialog::updateChanged );
        m_ui.gradientOverrideLabelSpinBox->setSpecialValueText(tr("None"));
        connect( m_ui.gradientOverrideLabelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );
        connect( m_ui.opaqueTitleBar, &QAbstractButton::clicked, this, &ExceptionDialog::updateChanged );
        m_ui.opacityOverrideLabelSpinBox->setSpecialValueText(tr("None"));
        connect( m_ui.opacityOverrideLabelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int /*i*/){updateChanged();} );
        connect( m_ui.isDialog, &QAbstractButton::clicked, this, &ExceptionDialog::updateChanged );
    }

    //___________________________________________
    void ExceptionDialog::setException( InternalSettingsPtr exception )
    {

        // store exception internally
        m_exception = exception;

        // type
        m_ui.exceptionType->setCurrentIndex(m_exception->exceptionType() );
        m_ui.exceptionEditor->setText( m_exception->exceptionPattern() );
        m_ui.borderSizeComboBox->setCurrentIndex( m_exception->borderSize() );
        m_ui.hideTitleBar->setCurrentIndex( m_exception->hideTitleBar() );
        m_ui.matchColorForTitleBar->setChecked( m_exception->matchColorForTitleBar() );
        m_ui.systemForegroundColor->setChecked( m_exception->systemForegroundColor() );
        m_ui.drawTitleBarSeparator->setChecked( m_exception->drawTitleBarSeparator() );
        m_ui.drawBackgroundGradient->setChecked( m_exception->drawBackgroundGradient() );
        m_ui.gradientOverrideLabelSpinBox->setValue( m_exception->gradientOverride() );
        m_ui.opaqueTitleBar->setChecked( m_exception->opaqueTitleBar() );
        m_ui.opacityOverrideLabelSpinBox->setValue( m_exception->opacityOverride() );
        m_ui.isDialog->setChecked( m_exception->isDialog() );

        // mask
        for( CheckBoxMap::iterator iter = m_checkboxes.begin(); iter != m_checkboxes.end(); ++iter )
        { iter.value()->setChecked( m_exception->mask() & iter.key() ); }

        setChanged( false );

    }

    //___________________________________________
    void ExceptionDialog::save()
    {
        m_exception->setExceptionType( m_ui.exceptionType->currentIndex() );
        m_exception->setExceptionPattern( m_ui.exceptionEditor->text() );
        m_exception->setBorderSize( m_ui.borderSizeComboBox->currentIndex() );
        m_exception->setHideTitleBar( m_ui.hideTitleBar->currentIndex() );
        m_exception->setMatchColorForTitleBar( m_ui.matchColorForTitleBar->isChecked() );
        m_exception->setSystemForegroundColor( m_ui.systemForegroundColor->isChecked() );
        m_exception->setDrawTitleBarSeparator( m_ui.drawTitleBarSeparator->isChecked() );
        m_exception->setDrawBackgroundGradient( m_ui.drawBackgroundGradient->isChecked() );
        m_exception->setGradientOverride( m_ui.gradientOverrideLabelSpinBox->value() );
        m_exception->setOpaqueTitleBar( m_ui.opaqueTitleBar->isChecked() );
        m_exception->setOpacityOverride( m_ui.opacityOverrideLabelSpinBox->value() );
        m_exception->setIsDialog( m_ui.isDialog->isChecked() );

        // mask
        unsigned int mask = None;
        for( CheckBoxMap::iterator iter = m_checkboxes.begin(); iter != m_checkboxes.end(); ++iter )
        { if( iter.value()->isChecked() ) mask |= iter.key(); }

        m_exception->setMask( mask );

        setChanged( false );

    }

    //___________________________________________
    void ExceptionDialog::updateChanged()
    {
        bool modified( false );
        if( m_exception->exceptionType() != m_ui.exceptionType->currentIndex() ) modified = true;
        else if( m_exception->exceptionPattern() != m_ui.exceptionEditor->text() ) modified = true;
        else if( m_exception->borderSize() != m_ui.borderSizeComboBox->currentIndex() ) modified = true;
        else if( m_exception->hideTitleBar() != m_ui.hideTitleBar->currentIndex() ) modified = true;
        else if( m_exception->matchColorForTitleBar() != m_ui.matchColorForTitleBar->isChecked() ) modified = true;
        else if( m_exception->systemForegroundColor() != m_ui.systemForegroundColor->isChecked() ) modified = true;
        else if( m_exception->drawTitleBarSeparator() != m_ui.drawTitleBarSeparator->isChecked() ) modified = true;
        else if( m_exception->drawBackgroundGradient() != m_ui.drawBackgroundGradient->isChecked() ) modified = true;
        else if( m_exception->gradientOverride() != m_ui.gradientOverrideLabelSpinBox->value() ) modified = true;
        else if( m_exception->opaqueTitleBar() != m_ui.opaqueTitleBar->isChecked() ) modified = true;
        else if( m_exception->opacityOverride() != m_ui.opacityOverrideLabelSpinBox->value() ) modified = true;
        else if( m_exception->isDialog() != m_ui.isDialog->isChecked() ) modified = true;
        else
        {
            // check mask
            for( CheckBoxMap::iterator iter = m_checkboxes.begin(); iter != m_checkboxes.end(); ++iter )
            {
                if( iter.value()->isChecked() != (bool)( m_exception->mask() & iter.key() ) )
                {
                    modified = true;
                    break;
                }
            }
        }

        setChanged( modified );

    }

    //___________________________________________
    void ExceptionDialog::selectWindowProperties()
    {

        // create widget
        if( !m_detectDialog )
        {
            m_detectDialog = new DetectDialog( this );
            connect( m_detectDialog, &DetectDialog::detectionDone, this, &ExceptionDialog::readWindowProperties );
        }

        m_detectDialog->detect();

    }

    //___________________________________________
    void ExceptionDialog::readWindowProperties( bool valid )
    {
        Q_CHECK_PTR( m_detectDialog );
        if( valid )
        {

            // window info
            const QVariantMap properties = m_detectDialog->properties();

            switch(m_ui.exceptionType->currentIndex())
            {

                default:
                case InternalSettings::ExceptionWindowClassName:
                m_ui.exceptionEditor->setText(properties.value(QStringLiteral("resourceClass")).toString());
                break;

                case InternalSettings::ExceptionWindowTitle:
                m_ui.exceptionEditor->setText(properties.value(QStringLiteral("caption")).toString());
                break;

            }

        }

        delete m_detectDialog;
        m_detectDialog = nullptr;

    }

}
