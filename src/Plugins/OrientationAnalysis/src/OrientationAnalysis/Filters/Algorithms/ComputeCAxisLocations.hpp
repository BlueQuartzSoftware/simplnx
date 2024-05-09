#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeCAxisLocationsInputValues
{
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CAxisLocationsArrayName;
};

/**
 * @class ComputeCAxisLocations
 * @brief This filter determines the direction of the C-axis for each Element by applying the quaternion of the Element to the <001> direction, which is the C-axis for Hexagonal materials. This will
 * tell where the C-axis of the Element sits in the sample reference frame.
 */

class ORIENTATIONANALYSIS_EXPORT ComputeCAxisLocations
{
public:
  ComputeCAxisLocations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeCAxisLocationsInputValues* inputValues);
  ~ComputeCAxisLocations() noexcept;

  ComputeCAxisLocations(const ComputeCAxisLocations&) = delete;
  ComputeCAxisLocations(ComputeCAxisLocations&&) noexcept = delete;
  ComputeCAxisLocations& operator=(const ComputeCAxisLocations&) = delete;
  ComputeCAxisLocations& operator=(ComputeCAxisLocations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeCAxisLocationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
