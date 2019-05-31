#ifndef MakeDrf_h
#define MakeDrf_h
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

#include <vector>

#include <Wt/WContainerWidget>

#include "InterSpec/AuxWindow.h"
#include "InterSpec/MakeDrfFit.h"

class MakeDrf;
class PeakDef;
class InterSpec;
class MaterialDB;
class MakeDrfChart;

namespace Wt
{
  class WText;
  class WGroupBox;
  class WComboBox;
  class WLineEdit;
  class WCheckBox;
  class WDoubleSpinBox;
  class WSuggestionPopup;
}

namespace { class DrfPeak; }


class MakeDrf : public Wt::WContainerWidget
{
public:
  MakeDrf( InterSpec *viewer,
           MaterialDB *materialDB,
           Wt::WSuggestionPopup *materialSuggest,
           Wt::WContainerWidget *parent = nullptr );
  
  virtual ~MakeDrf();
  
  /** Create a AuxWindow with the MakeDrf widget as the primary content.
      Returns pointer to the created AuxWindow, but is will already be shown,
      and have the signals to delete it when closed hooked up, so you probably
      wont need the window.
   */
  static AuxWindow *makeDrfWindow( InterSpec *viewer, MaterialDB *materialDB,
                                   Wt::WSuggestionPopup *materialSuggest );
  
  /** Creates a "Save As..." dialog.  If the current DRF is not valid, the
     dialog that will pop up will state as such.
   */
  void startSaveAs();
  
  /** Returns whether it is worthwhile to call #startSaveAs */
  bool isIntrinsicEfficiencyValid();
  
  /** Signal emmitted wheneever the intrinsic efficiency becomes valid, or
     invalid.  Intended for enabling/disabling save button.
   */
  Wt::Signal<bool> &intrinsicEfficiencyIsValid();
  
  /** Assembles a SpecMeas with a single (summed, if need be) spectrum for
   each sampel number that has a peak being used; has peaks as well.
   
   Will return nullptr on error.
   
   Currently does not save source information to returned result (if we do this
   then re-creating this widget is maybe complete?)
   */
  std::shared_ptr<SpecMeas> assembleCalFile();
  
protected:
  void handleSourcesUpdates();
  void handleSqrtEqnOrderChange();
  void handleFwhmTypeChanged();
  void handleShowFwhmPointsToggled();
  void chartEnergyRangeChangedCallback( double lower, double upper );
  
  void peakPreviewShown( DrfPeak *peak );
  
  void fitFwhmEqn( std::vector< std::shared_ptr<const PeakDef> > peaks,
                   const size_t num_gamma_channels );
  void updateFwhmEqn( std::vector<float> coefs, std::vector<float> uncerts,
                      const int functionalForm, //see DetectorPeakResponse::ResolutionFnctForm
                      const int fitid );
  
  void fitEffEqn( std::vector<MakeDrfFit::DetEffDataPoint> data );
  
  /** Error message is not empty, only when there is an error. */
  void updateEffEqn( std::vector<float> coefs, std::vector<float> uncerts, const int fitid, const std::string errormsg );

  
  InterSpec *m_interspec;
  MaterialDB *m_materialDB;
  Wt::WSuggestionPopup *m_materialSuggest;
  
  Wt::Signal<bool> m_intrinsicEfficiencyIsValid;
  
  /** Signal emmitted when the user saves the DRF (or in the future otherwise
   indicates they are done with the tool) and the widget should now be deleted.
   */
  Wt::Signal<> m_finished;
  
  MakeDrfChart *m_chart;
  
  Wt::WContainerWidget *m_files;
  
  Wt::WLineEdit *m_detDiameter;
  
  Wt::WCheckBox *m_showFwhmPoints;
  
  Wt::WGroupBox *m_fwhmOptionGroup;
  
  Wt::WComboBox *m_fwhmEqnType;
  
  Wt::WComboBox *m_sqrtEqnOrder;
  
  Wt::WComboBox *m_effEqnOrder;
  
  Wt::WComboBox *m_effEqnUnits;
  
  Wt::WGroupBox *m_effOptionGroup;
  
  /** ToDo: make chart properly interactive so user doesnt need to input the
   energy range manually.
   */
  Wt::WDoubleSpinBox *m_chartLowerE;
  Wt::WDoubleSpinBox *m_chartUpperE;
  
  Wt::WText *m_errorMsg;
  Wt::WText *m_intrinsicEffAnswer;
  
  /** Identifies current fit for FWHM.  Fits are done in an auxilary thread,
   and the user may change things before the fit is done, and some fits take a
   lot longer than others, so to make sure the the correct one is displayed,
   we will use this variable (which is only touched from main Wt thread).
   */
  int m_fwhmFitId;
  int m_effEqnFitId;
  
  std::vector<float> m_fwhmCoefs, m_fwhmCoefUncerts;
  std::vector<float> m_effEqnCoefs, m_effEqnCoefUncerts;
  
  friend class MakeDrfWindow;
};//class MakeDrf

#endif //MakeDrf_h
