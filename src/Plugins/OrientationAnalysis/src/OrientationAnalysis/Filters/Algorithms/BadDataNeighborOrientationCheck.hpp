#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckInputValues
{
  float32 MisorientationTolerance;
  int32 NumberOfNeighbors;
  DataPath ImageGeomPath;
  DataPath QuatsArrayPath;
  DataPath MaskArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheck
{
public:
  BadDataNeighborOrientationCheck(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  BadDataNeighborOrientationCheckInputValues* inputValues);
  ~BadDataNeighborOrientationCheck() noexcept;

  BadDataNeighborOrientationCheck(const BadDataNeighborOrientationCheck&) = delete;
  BadDataNeighborOrientationCheck(BadDataNeighborOrientationCheck&&) noexcept = delete;
  BadDataNeighborOrientationCheck& operator=(const BadDataNeighborOrientationCheck&) = delete;
  BadDataNeighborOrientationCheck& operator=(BadDataNeighborOrientationCheck&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
