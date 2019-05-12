/* InterSpec: an application to analyze spectral gamma radiation data.
 
 Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC
 (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
 Government retains certain rights in this software.
 For questions contact William Johnson via email at wcjohns@sandia.gov, or
 alternative emails of interspec@sandia.gov.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "InterSpec_config.h"

#include <Wt/WDate>
#include <Wt/WTable>
#include <Wt/WLabel>
#include <Wt/WComboBox>
#include <Wt/WDateEdit>
#include <Wt/WLineEdit>
#include <Wt/WCheckBox>
#include <Wt/WTableCell>
#include <Wt/WApplication>
#include <Wt/WDoubleSpinBox>
#include <Wt/WDoubleValidator>
#include <Wt/WRegExpValidator>
#include <Wt/WContainerWidget>
#include <Wt/WSuggestionPopup>

#include "InterSpec/PeakDef.h"
#include "InterSpec/MaterialDB.h"
#include "InterSpec/HelpSystem.h"
#include "InterSpec/MakeDrfSrcDef.h"
#include "SandiaDecay/SandiaDecay.h"
#include "InterSpec/PhysicalUnits.h"
#include "SpecUtils/UtilityFunctions.h"
#include "InterSpec/IsotopeSelectionAids.h"
#include "InterSpec/ShieldingSourceDisplay.h"
//#include "InterSpec/IsotopeNameFilterModel.h"

using namespace std;
using namespace Wt;

namespace
{
  const int sm_distance_row        = 1;
  const int sm_activity_row        = 2;
  const int sm_activity_uncert_row = 3;
  const int sm_assay_date_row      = 4;
  const int sm_spec_date_row       = 5;
  const int sm_age_at_assay_row    = 6;
  const int sm_decayed_info_row    = 7;
  const int sm_shield_material_row = 8;
  const int sm_options_row         = 9;
}//namespace


MakeDrfSrcDef::MakeDrfSrcDef( const SandiaDecay::Nuclide *nuc,
              const boost::posix_time::ptime &measDate,
              MaterialDB *materialDB,
              Wt::WSuggestionPopup *materialSuggest,
              Wt::WContainerWidget *parent )
: WContainerWidget( parent ),
  m_table( nullptr ),
  m_nuclide( nuc ),
  m_materialDB( materialDB ),
  m_materialSuggest( materialSuggest ),
  m_nuclideLabel( nullptr ),
  m_distanceEdit( nullptr ),
  m_activityEdit( nullptr ),
  m_activityUncertainty( nullptr ),
  m_useAgeInfo( nullptr ),
  m_assayDate( nullptr ),
  m_drfMeasurementDate( nullptr ),
  m_sourceInfoAtMeasurement( nullptr ),
  m_sourceAgeAtAssay( nullptr ),
  m_useShielding( nullptr ),
  m_shieldingSelect( nullptr )
{
  wApp->useStyleSheet( "InterSpec_resources/MakeDrfSrcDef.css" );
  
  addStyleClass( "MakeDrfSrcDef" );
  
  create();
  
  setNuclide( m_nuclide );
  
  if( !measDate.is_special() )
    m_drfMeasurementDate->setDate( WDateTime::fromPosixTime(measDate).date() );
}//MakeDrfSrcDef constructor


MakeDrfSrcDef::~MakeDrfSrcDef()
{
}


Wt::Signal<> &MakeDrfSrcDef::updated()
{
  return m_updated;
}

void MakeDrfSrcDef::setNuclide( const SandiaDecay::Nuclide *nuc )
{
  m_nuclide = nuc;
  const bool notMuchEvolution = (!nuc || PeakDef::ageFitNotAllowed(nuc));
  
  m_table->rowAt(sm_age_at_assay_row)->setHidden( notMuchEvolution );
  
  if( notMuchEvolution || !nuc )
  {
    m_sourceAgeAtAssay->setText( "0s" );
  }else
  {
    double ageval = 5.0*nuc->halfLife;
    if( nuc->canObtainPromptEquilibrium() )
      ageval = log(10000.0)/log(2.0) * nuc->promptEquilibriumHalfLife();
    if( ageval > 20*PhysicalUnits::year )
      ageval = 20*PhysicalUnits::year;
    m_sourceAgeAtAssay->setText( PhysicalUnits::printToBestTimeUnits(ageval) );
  }
  
  if( nuc )
  {
    const string label = "<span class=\"SrcTitleNuc\">" + nuc->symbol + "</span>, <span class=\"SrcTitleHl\">&lambda;<sub>&frac12;</sub>="
                         + PhysicalUnits::printToBestTimeUnits(nuc->halfLife) + "</span>";
    m_nuclideLabel->setText( WString::fromUTF8(label) );
  }else
  {
    m_nuclideLabel->setText( "" );
  }
  
  useAgeInfoUserToggled();
}//setNuclide(...)


void MakeDrfSrcDef::create()
{
  m_table = new WTable( this );
  m_table->addStyleClass( "SrcInputTable" );
  
  WTableCell *cell = m_table->elementAt(0,0);
  cell->setColumnSpan( 2 );
  cell->addStyleClass( "SrcNuclideTitle" );
  m_nuclideLabel = new WText( cell );
  
/*
  //Code to put a nuclide suggestion into a WLineEdit so user could select nuclide.
  string replacerJs, matcherJs;
  PhotopeakDelegate::EditWidget::replacerJs( replacerJs );
  PhotopeakDelegate::EditWidget::nuclideNameMatcherJs( matcherJs );
  
  WSuggestionPopup *nucSuggest = new WSuggestionPopup( matcherJs, replacerJs );
  nucSuggest->setJavaScriptMember("wtNoReparent", "true");
  
  nucSuggest->setMaximumSize( WLength::Auto, WLength(15, WLength::FontEm) );
  nucSuggest->forEdit( m_nuclideEdit,
                       WSuggestionPopup::Editing | WSuggestionPopup::DropDownIcon );
  
  IsotopeNameFilterModel *filterModel = new IsotopeNameFilterModel( this );
  
  filterModel->excludeNuclides( false );
  filterModel->excludeXrays( true );
  filterModel->excludeEscapes( true );
  filterModel->excludeReactions( true );
  
  filterModel->filter( "" );
  nucSuggest->setFilterLength( -1 );
  nucSuggest->setModel( filterModel );
  nucSuggest->filterModel().connect( filterModel, &IsotopeNameFilterModel::filter );
*/
  
  cell = m_table->elementAt(sm_distance_row,0);
  WLabel *label = new WLabel( "Distance", cell );
  cell = m_table->elementAt(sm_distance_row,1);
  m_distanceEdit = new WLineEdit( cell );
  m_distanceEdit->setTextSize( 16 );
  m_distanceEdit->setAutoComplete( false );
  label->setBuddy( m_distanceEdit );
  WRegExpValidator *distValidator = new WRegExpValidator( PhysicalUnits::sm_distanceUnitOptionalRegex, this );
  distValidator->setFlags( Wt::MatchCaseInsensitive );
  m_distanceEdit->setValidator( distValidator );
  m_distanceEdit->setText( "50 cm" );
  m_distanceEdit->changed().connect( this, &MakeDrfSrcDef::handleUserChangedDistance );
  m_distanceEdit->enterPressed().connect( this, &MakeDrfSrcDef::handleUserChangedDistance );
  
  
  cell = m_table->elementAt(sm_activity_row,0);
  label = new WLabel( "Activity", cell );
  
  cell = m_table->elementAt(sm_activity_row,1);
  m_activityEdit = new WLineEdit( cell );
  m_activityEdit->setAutoComplete( false );
  m_activityEdit->setTextSize( 16 );
  label->setBuddy( m_activityEdit );
  
  WRegExpValidator *val = new WRegExpValidator( PhysicalUnits::sm_activityRegex, this );
  val->setFlags( Wt::MatchCaseInsensitive );
  m_activityEdit->setValidator( val );
  m_activityEdit->setText( "100 uCi" );
  m_activityEdit->changed().connect( this, &MakeDrfSrcDef::handleUserChangedActivity );
  m_activityEdit->enterPressed().connect( this, &MakeDrfSrcDef::handleUserChangedActivity );

  cell = m_table->elementAt(sm_activity_uncert_row,0);
  label = new WLabel( "Act. Uncert.&nbsp;", cell );  //The nbsp is to make this the longest label so when acti ity or shielding is shown, the width doesnt get changed
  cell = m_table->elementAt(sm_activity_uncert_row,1);
  m_activityUncertainty = new WDoubleSpinBox( cell );
  m_activityUncertainty->setValue( 0.0 );
  m_activityUncertainty->setTextSize( 14 );
  m_activityUncertainty->setAutoComplete( false );
  m_activityUncertainty->setSuffix( " %" );
  label->setBuddy( m_activityUncertainty );
  
  m_activityUncertainty->setDecimals( 1 );
  m_activityUncertainty->setRange( 0.0, 100.0 );
  m_activityUncertainty->setSingleStep( 1.0 );
  
  m_activityUncertainty->changed().connect( this, &MakeDrfSrcDef::handleUserChangedActivityUncertainty );
  m_activityUncertainty->enterPressed().connect( this, &MakeDrfSrcDef::handleUserChangedActivityUncertainty );
  
  //WDoubleValidator *percentVal = new WDoubleValidator( this );
  //percentVal->setRange( 0.0, 100.0 );
  //m_activityUncertainty->setValidator( percentVal );
  
  cell = m_table->elementAt(sm_assay_date_row,0);
  label = new WLabel( "Assay Date", cell );
  cell = m_table->elementAt(sm_assay_date_row,1);
  m_assayDate = new WDateEdit( cell );
  //m_assayDate->setTextSize( 9 );
  label->setBuddy( m_assayDate );
  m_assayDate->changed().connect( this, &MakeDrfSrcDef::handleEnteredDatesUpdated );
  
  cell = m_table->elementAt(sm_spec_date_row,0);
  label = new WLabel( "Spec. Date", cell );
  cell = m_table->elementAt(sm_spec_date_row,1);
  m_drfMeasurementDate = new WDateEdit( cell );
  //m_drfMeasurementDate->setTextSize( 10 );
  //The right padding is 40px, could reduce down to 30.
  label->setBuddy( m_drfMeasurementDate );
  m_drfMeasurementDate->changed().connect( this, &MakeDrfSrcDef::handleEnteredDatesUpdated );
  
  
  cell = m_table->elementAt(sm_age_at_assay_row,0);
  label = new WLabel( "Age@Assay", cell );
  cell = m_table->elementAt(sm_age_at_assay_row,1);
  m_sourceAgeAtAssay = new WLineEdit( cell );
  m_sourceAgeAtAssay->setAutoComplete( false );
  label->setBuddy( m_sourceAgeAtAssay );
  m_sourceAgeAtAssay->changed().connect( this, &MakeDrfSrcDef::handleUserChangedAgeAtAssay );
  m_sourceAgeAtAssay->enterPressed().connect( this, &MakeDrfSrcDef::handleUserChangedAgeAtAssay );
  
  val = new WRegExpValidator( PhysicalUnits::sm_timeDurationRegex, this );
  val->setFlags( Wt::MatchCaseInsensitive );
  m_sourceAgeAtAssay->setValidator( val );
  m_sourceAgeAtAssay->setText( "0s" );
  
  
  cell = m_table->elementAt(sm_decayed_info_row,0);
  label = new WLabel( "Aging Res.", cell );
  cell = m_table->elementAt(sm_decayed_info_row,1);
  m_sourceInfoAtMeasurement = new WText( cell );
  
  //Wt::WDateEdit *m_sourceCreationDate;
  
  cell = m_table->elementAt(sm_options_row,0);
  m_useAgeInfo = new WCheckBox( "Age?", cell );
  m_useAgeInfo->setFloatSide( Wt::Right );
  m_useAgeInfo->setChecked( false );
  m_useAgeInfo->changed().connect( this, &MakeDrfSrcDef::useAgeInfoUserToggled );
  
  cell = m_table->elementAt(sm_options_row,1);
  m_useShielding = new WCheckBox( "Shielded?", cell );
  m_useShielding->setFloatSide( Wt::Right );
  m_useShielding->setChecked( false );
  m_useShielding->changed().connect( this, &MakeDrfSrcDef::useShieldingInfoUserToggled );
  
  
  cell = m_table->elementAt(sm_shield_material_row,0);
  cell->setColumnSpan( 2 );
  m_shieldingSelect = new ShieldingSelect( m_materialDB, nullptr, m_materialSuggest, false, cell );
  m_shieldingSelect->hide();
  
  useAgeInfoUserToggled();
}//void create()


void MakeDrfSrcDef::useAgeInfoUserToggled()
{
  const bool useAge = m_useAgeInfo->isChecked();
  m_activityEdit->label()->setText( useAge ? "Assay Act." : "Activity" );
  
  m_table->rowAt(sm_assay_date_row)->setHidden( !useAge );
  m_table->rowAt(sm_spec_date_row)->setHidden( !useAge );
  
  const bool notMuchEvolution = (!m_nuclide || PeakDef::ageFitNotAllowed(m_nuclide));
  m_table->rowAt(sm_age_at_assay_row)->setHidden( !useAge || notMuchEvolution );
  
  m_table->rowAt(sm_decayed_info_row)->setHidden( !useAge );
  
  m_assayDate->setDisabled( !useAge );
  m_drfMeasurementDate->setDisabled( !useAge );
  m_sourceInfoAtMeasurement->setDisabled( !useAge );
  m_sourceAgeAtAssay->setDisabled( !useAge );
  
  m_updated.emit();
}//void useAgeInfoUserToggled()


void MakeDrfSrcDef::updateAgedText()
{
  try
  {
    if( !m_nuclide )
      throw runtime_error( "" );
    
    double activity = activityAtSpectrumTime();
    double age = ageAtSpectrumTime();
    const string agestr = PhysicalUnits::printToBestTimeUnits(age);
    
    const string enteredAct = m_activityEdit->text().toUTF8();
    
    const bool useCurrie = (enteredAct.find_first_of( "cC" ) != string::npos);
    const string actstr = PhysicalUnits::printToBestActivityUnits(activity, 1, useCurrie );
    
    string txt = actstr;
    if( !PeakDef::ageFitNotAllowed(m_nuclide) )
      txt += ", " + agestr;

    m_sourceInfoAtMeasurement->setText( WString::fromUTF8(txt) );
  }catch( std::exception & )
  {
    m_sourceInfoAtMeasurement->setText( "" );
  }
}//void updateAgedText()


void MakeDrfSrcDef::handleUserChangedDistance()
{
  try
  {
    distance();
    if( m_distanceEdit->hasStyleClass( "SrcInputError" ) )
      m_distanceEdit->removeStyleClass( "SrcInputError" );
  }catch( std::exception & )
  {
    if( !m_distanceEdit->hasStyleClass( "SrcInputError" ) )
      m_distanceEdit->addStyleClass( "SrcInputError" );
  }
  
  m_updated.emit();
}//void handleUserChangedDistance()


void MakeDrfSrcDef::handleUserChangedActivity()
{
  try
  {
    enteredActivity();
    if( m_activityEdit->hasStyleClass( "SrcInputError" ) )
      m_activityEdit->removeStyleClass( "SrcInputError" );
  }catch( std::exception & )
  {
    if( !m_activityEdit->hasStyleClass( "SrcInputError" ) )
      m_activityEdit->addStyleClass( "SrcInputError" );
  }
  
  if( m_useAgeInfo->isChecked() )
    updateAgedText();
  
  m_updated.emit();
}//void handleUserChangedActivity()


void MakeDrfSrcDef::handleUserChangedActivityUncertainty()
{
  if( WValidator::State::Valid == m_activityUncertainty->validate() )
  {
    if( m_activityUncertainty->hasStyleClass( "SrcInputError" ) )
      m_activityUncertainty->removeStyleClass( "SrcInputError" );
  }else
  {
    if( !m_activityUncertainty->hasStyleClass( "SrcInputError" ) )
      m_activityUncertainty->addStyleClass( "SrcInputError" );
  }
  
  m_updated.emit();
}//void handleUserChangedActivityUncertainty();


void MakeDrfSrcDef::handleUserChangedAgeAtAssay()
{
  string agestr = m_sourceAgeAtAssay->text().toUTF8();
  UtilityFunctions::trim( agestr );
  
  double age = 0.0;
  if( agestr.empty() || (agestr.find_first_not_of("+-0.")==string::npos) )
  {
    m_sourceAgeAtAssay->setText( "0 uCi" );
  }else
  {
    try
    {
      const double hl = m_nuclide ? m_nuclide->halfLife : -1.0;
      age = PhysicalUnits::stringToTimeDurationPossibleHalfLife( agestr, hl );
      if( m_activityEdit->hasStyleClass( "SrcInputError" ) )
        m_activityEdit->removeStyleClass( "SrcInputError" );
    }catch( std::exception &e )
    {
      if( !m_activityEdit->hasStyleClass( "SrcInputError" ) )
        m_activityEdit->addStyleClass( "SrcInputError" );
    }
  }//if( zero / else )
  
  updateAgedText();
  
  m_updated.emit();
}//void handleUserChangedAgeAtAssay()


void MakeDrfSrcDef::handleEnteredDatesUpdated()
{
  
  if( m_drfMeasurementDate->validate() == Wt::WValidator::Valid )
  {
    if( m_drfMeasurementDate->hasStyleClass( "SrcInputError" ) )
      m_drfMeasurementDate->removeStyleClass( "SrcInputError" );
  }else
  {
    if( !m_drfMeasurementDate->hasStyleClass( "SrcInputError" ) )
      m_drfMeasurementDate->addStyleClass( "SrcInputError" );
  }
  
  if( m_assayDate->validate() == Wt::WValidator::Valid )
  {
    if( m_assayDate->hasStyleClass( "SrcInputError" ) )
      m_assayDate->removeStyleClass( "SrcInputError" );
  }else
  {
    if( !m_assayDate->hasStyleClass( "SrcInputError" ) )
      m_assayDate->addStyleClass( "SrcInputError" );
  }
  
  updateAgedText();
  
  m_updated.emit();
}//void handleEnteredDatesUpdated()


void MakeDrfSrcDef::useShieldingInfoUserToggled()
{
  m_shieldingSelect->setHidden( !m_useShielding->isChecked() );
  m_updated.emit();
}//void useShieldingInfoUserToggled()


double MakeDrfSrcDef::enteredActivity() const
{
  string activitystr = m_activityEdit->text().toUTF8();
  
  UtilityFunctions::trim( activitystr );
  
  return PhysicalUnits::stringToActivity( activitystr );
}//double enteredActivity()


double MakeDrfSrcDef::distance() const
{
  const string txt = m_distanceEdit->text().toUTF8();
  return PhysicalUnits::stringToDistance(txt);
}

const SandiaDecay::Nuclide *MakeDrfSrcDef::nuclide() const
{
  return m_nuclide;
}


double MakeDrfSrcDef::activityAtSpectrumTime() const
{
  const double userActivity = enteredActivity();
  if( !m_useAgeInfo->isChecked() || !m_nuclide )
    return userActivity;
  
  const WDate measDate = m_drfMeasurementDate->date();
  const WDate assayDate = m_assayDate->date();
  
  if( !measDate.isValid() )
    throw runtime_error( "Measurement date invalid" );
  
  if( !assayDate.isValid() )
    throw runtime_error( "Assay date invalid" );
  
  if( assayDate > measDate )
    throw runtime_error( "Assay date must be before measurement date" );
  
  const int numDays = assayDate.daysTo( measDate );
  
  SandiaDecay::NuclideMixture mix;
  mix.addNuclideByActivity( m_nuclide, userActivity );
  
  return mix.activity( 24.0*3600.0*numDays, m_nuclide );
}//double activityAtSpectrumTime() const


double MakeDrfSrcDef::fractionalActivityUncertainty() const
{
  switch( m_activityUncertainty->validate() )
  {
    case WValidator::State::Invalid:
      throw runtime_error( "Activity Uncertainty Invalid" );
      
    case WValidator::State::InvalidEmpty:
      return 0.0;
      
    case WValidator::State::Valid:
      break;
  }//switch( m_activityUncertainty->validate() )
  
  return m_activityUncertainty->value() / 100.0;
}//double fractionalActivityUncertainty() const


double MakeDrfSrcDef::ageAtSpectrumTime() const
{
  if( !m_nuclide )
    return 0.0;

  if( !m_useAgeInfo->isChecked() || PeakDef::ageFitNotAllowed(m_nuclide) )
    return PeakDef::defaultDecayTime( m_nuclide, nullptr );
  
  string ageAtAssaystr = m_sourceAgeAtAssay->text().toUTF8();
  UtilityFunctions::trim( ageAtAssaystr );
  
  double ageAtAssay = 0.0;
  if( !ageAtAssaystr.empty() && (ageAtAssaystr.find_first_not_of("+-0.")!=string::npos) )
    ageAtAssay = PhysicalUnits::stringToTimeDurationPossibleHalfLife( ageAtAssaystr, m_nuclide->halfLife );
  
  const WDate measDate = m_drfMeasurementDate->date();
  const WDate assayDate = m_assayDate->date();
  
  if( !measDate.isValid() )
    throw runtime_error( "Measurement date invalid" );
  
  if( !assayDate.isValid() )
    throw runtime_error( "Assay date invalid" );
  
  if( assayDate > measDate )
    throw runtime_error( "Assay date must be before measurement date" );
  
  const double numDays = assayDate.daysTo( measDate );
  
  return ageAtAssay + 24.0*3600.0*numDays;
}//double ageAtSpectrumTime() const


ShieldingSelect *MakeDrfSrcDef::shielding()
{
  if( !m_useShielding->isChecked() )
    return nullptr;
  return m_shieldingSelect;
}//ShieldingSelect *shielding()


void MakeDrfSrcDef::setDistance( const double dist )
{
  m_distanceEdit->setText( PhysicalUnits::printToBestLengthUnits(dist) );
}//void setDistance( const double dist );


void MakeDrfSrcDef::setActivity( const double act )
{
  m_activityEdit->setText( PhysicalUnits::printToBestActivityUnits(act) );
  updateAgedText();
}//void MakeDrfSrcDef::setActivity( const double dist )


void MakeDrfSrcDef::setAssayInfo( const double activity,
                                  const boost::posix_time::ptime &assay_date )
{
  m_useAgeInfo->setChecked( !assay_date.is_special() );
  useAgeInfoUserToggled();
  m_assayDate->setDate( WDateTime::fromPosixTime(assay_date).date() );
  
  if( activity > 0.0 )
    m_activityEdit->setText( PhysicalUnits::printToBestActivityUnits(activity) );
  
  updateAgedText();
}//void setAssayInfo(..);


void MakeDrfSrcDef::setShielding( const float atomic_number, const float areal_density )
{
  m_useShielding->setChecked();
  m_shieldingSelect->setHidden( false );
  m_shieldingSelect->setAtomicNumberAndArealDensity( atomic_number, areal_density );
}//void setShielding( const float atomic_number, const float areal_density )
