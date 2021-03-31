#ifndef EnergyCalPreserveWindow_h
#define EnergyCalPreserveWindow_h
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

#include  <memory>

#include "InterSpec/AuxWindow.h"

//Forward Declarations
class SpecMeas;
class EnergyCalTool;
namespace SpecUtils{ enum class EnergyCalType : int; }
namespace SpecUtils{ enum class SpectrumType : int; }

/** Prompts user if energy calibration from one SpecMeas should be propogated to another.
 
 Used when loading a new spectrum into InterSpec, and it looks to be the same detector, but a
 different energy calibration.
 */
class EnergyCalPreserveWindow : public AuxWindow
{
public:
  static bool candidate( std::shared_ptr<SpecMeas> newmeas, std::shared_ptr<SpecMeas> oldmeas );
  
public:
  EnergyCalPreserveWindow( std::shared_ptr<SpecMeas> newmeas, const SpecUtils::SpectrumType newtype,
                           std::shared_ptr<SpecMeas> oldmeas, const SpecUtils::SpectrumType oldtype,
                           EnergyCalTool *calibrator );
  
  virtual ~EnergyCalPreserveWindow();
  
protected:
  void propogateCalibrations();
  
  
  EnergyCalTool *m_calibrator;
  std::shared_ptr<SpecMeas> m_newmeas;
  std::shared_ptr<SpecMeas> m_oldmeas;
  const SpecUtils::SpectrumType m_newtype, m_oldtype;
};//class EnergyCalPreserveWindow

#endif //EnergyCalPreserveWindow_h
