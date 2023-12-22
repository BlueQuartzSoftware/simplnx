#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT FindCAxisLocationsInputValues
{
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CAxisLocationsArrayName;
};

/**
 * @class FindCAxisLocations
 * @brief This filter determines the direction of the C-axis for each Element by applying the quaternion of the Element to the <001> direction, which is the C-axis for Hexagonal materials. This will
 * tell where the C-axis of the Element sits in the sample reference frame.
 */

class ORIENTATIONANALYSIS_EXPORT FindCAxisLocations
{
public:
  FindCAxisLocations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindCAxisLocationsInputValues* inputValues);
  ~FindCAxisLocations() noexcept;

  FindCAxisLocations(const FindCAxisLocations&) = delete;
  FindCAxisLocations(FindCAxisLocations&&) noexcept = delete;
  FindCAxisLocations& operator=(const FindCAxisLocations&) = delete;
  FindCAxisLocations& operator=(FindCAxisLocations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindCAxisLocationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
