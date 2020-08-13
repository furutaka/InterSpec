#ifndef EnergyCalTool_h
#define EnergyCalTool_h
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

#include <set>
#include <string>
#include <vector>
#include <memory>

#include <Wt/WContainerWidget>

#include "InterSpec/AuxWindow.h"

class SpecMeas;
class AuxWindow;
class InterSpec;
class PeakModel;
class RowStretchTreeView;

namespace Wt
{
  class WText;
  class WCheckBox;
  class WGridLayout;
  class WPushButton;
  class WStackedWidget;
}//namespace Wt

namespace EnergyCalImp
{
  class CalDisplay;
}

/** This class handles energy calibration by the user.
 
 Energy calibration can be a suprising complicated or nuanced operation, but the user almost never
 cares about the details, and they just want to make a peak lineup with a reference line.  So the
 goal of this class is facilitate the most complex energy calibrations, while defaulting to letting
 the user not worry about the details.
 
 There can be a wide variation of energy calibration scenerious in spectrum files:
 - Single record with defined energy calibration
 - Multiple samples, all from the same detector, all with the same energy calibration
 - Multiple samples, all from the same detector, but calibration changes between samples
 - Multiple samples, each with different detectors, but all detectors and samples use same energy
   calbration
 - Multiple samples, each with different detectors, but each detector uses a different energy
   calibration, but that remains same for all samples.
 - Multiple samples, each with different detectors, and some or all detectors calibration may change
   throughout with sample.
 - Some sample/detectors may be lower channel, while others polynomial or FRF
 - Different deviation pairs for each detector, but constant throughout the file
 - Different deviation pairs for each detector, but may change throughout the file
 
 And the user may want to apply calibration differences in a few ways:
 - Apply change in energy calibration to every detector and every sample in file
 - Apply change to only certain detectors, but for every sample in file
 - Apply change to only the sample numbers currently contributing to spectrum, but all detectors
 - Apply change to only the sample numbers and detectors currently contributing to spectrum
 - Apply change to all detectors, but only visible samples
 Where calibration differences means propogating the change in linear term for detector Aa1, to
 Aa2, and not just simply setting Aa1 and Aa2's linear coefficients equal to each other.
 
 Users may want to:
 - re-bin all spectrum to the same energy calibration before/after doing recalibration
 - linearize spectra
 - change from polynomial to FRF, etc
 - Edit deviation pairs
 - Convert lower channel energies to polynomial/FRF coefficients, or other way
 - Revert energy calibration to original calbiration given in file
 - Revert to a previous energy calibration
 
 The ways users may want to perform a calbiration include:
 - Fit one or more peaks that have an assigned nuclide, and use that to fit coefficients
 - Drag spectrum using mouse to where it should be; they may do this for, offset, gain, or deviation
   pairs
 - Use multiple files to fit coefficients (useful for lower resolution detectors that measured one
   nuclide at a time).
 */

class EnergyCalTool : public Wt::WContainerWidget
{
public:
  EnergyCalTool( InterSpec *viewer, PeakModel *peakModel, Wt::WContainerWidget *parent = nullptr );
  virtual ~EnergyCalTool();
  
  void setWideLayout(){}
  void setTallLayout(AuxWindow* parent){ }
  
  enum LayoutStyle{ kWide, kTall };
  LayoutStyle currentLayoutStyle() const{ return LayoutStyle::kWide; }
  
  
  void handleGraphicalRecalRequest( double xstart, double xfinish ){}
  
  void refreshGuiFromFiles();
  void refreshRecalibrator(){ refreshGuiFromFiles(); }
  
  void fitCoefficientCBChanged();
  void userChangedCoefficient( const size_t coefnum, EnergyCalImp::CalDisplay *display );
  void userChangedDeviationPair( EnergyCalImp::CalDisplay *display );
  
  void displayedSpectrumChanged( const SpecUtils::SpectrumType type,
                                 const std::shared_ptr<SpecMeas> &meas,
                                 const std::set<int> &samples,
                                 const std::vector<std::string> &detectors );
  
  
  void setShowNoCalInfo( const bool nocal );
  
protected:
  void specTypeToDisplayForChanged();
  
  /** \TODO: have #refreshGuiFromFiles only ever get called from #render to avoid current situation
   of calling refreshGuiFromFiles multiple times for a single render (the looping over files can
   get expensive probably)
   
   virtual void render( WFlags<RenderFlag> flags) override
  */
  
  enum ApplyToCbIndex
  {
    ApplyToForeground = 0,
    ApplyToBackground,
    ApplyToSecondary,
    ApplyToDisplayedDetectors,
    ApplyToAllDetectors,
    ApplyToDisplayedSamples,
    ApplyToAllSamples,
    NumApplyToCbIndex
  };//enum ApplyToCbIndex
  
  void applyToCbChanged( const ApplyToCbIndex index );
  
  enum MoreActionsIndex
  {
    Linearize,
    Truncate,
    CombineChannels,
    ConvertToFrf,
    ConvertToPoly,
    NumMoreActionsIndex
  };//enum MoreActionsIndex
  
  void moreActionBtnClicked( const MoreActionsIndex index );
  
  
  /*
  static void shiftPeaksForEnergyCalibration(
  PeakModel *peakmodel,
  const std::vector<float> &new_pars,
  const std::vector< std::pair<float,float> > &new_devpairs,
  SpecUtils::EnergyCalType new_eqn_type,
  std::shared_ptr<SpecMeas> meas,
  const SpecUtils::SpectrumType spectype,
  std::vector<float> old_pars,
  const std::vector< std::pair<float,float> > &old_devpairs,
  SpecUtils::EnergyCalType old_eqn_type )
  {
    
  }
   */
  InterSpec *m_interspec;
  PeakModel *m_peakModel;
  RowStretchTreeView *m_peakTable;  /** \TODO: can maybe remove this memeber variable */
  
  /** Menu that lets you choose which spectrum (For., Back, Sec.) to show the detectors to select */
  Wt::WMenu *m_specTypeMenu;
  
  /** Stack that holds the menus that let you select which detector to show calibration info for. */
  Wt::WStackedWidget *m_specTypeMenuStack;
  
  /** The menus that let you select which detector to show calibration info for.
   These menues are held in the #m_specTypeMenuStack stack.
   
   These entries will be nullptr if there is not a SpecFile for its respective {for, back, sec}
   */
  Wt::WMenu *m_detectorMenu[3];
  
  /** The stack that holds all the calibration info displays for all the spectrum files and
   detectors (i.e., this stack is shared by all m_detectorMenu[]'s).
   */
  Wt::WStackedWidget *m_calInfoDisplayStack;
  
  Wt::WText *m_noCalTxt;
  Wt::WContainerWidget *m_moreActionsColumn;
  Wt::WContainerWidget *m_applyToColumn;
  Wt::WContainerWidget *m_detColumn;
  Wt::WContainerWidget *m_calColumn;
  Wt::WContainerWidget *m_peakTableColumn;
  
  Wt::WGridLayout *m_layout;
  
  Wt::WCheckBox *m_applyToCbs[ApplyToCbIndex::NumApplyToCbIndex];
  
  Wt::WAnchor *m_moreActions[MoreActionsIndex::NumMoreActionsIndex];
  
  std::shared_ptr<SpecMeas> m_currentSpecMeas[3];
  std::set<int> m_currentSampleNumbers[3];
};//class EnergyCalTool

#endif //EnergyCalTool_h
