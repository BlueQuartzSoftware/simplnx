#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{
struct ORIENTATIONANALYSIS_EXPORT ComputeTwinBoundariesInputValues
{
  bool FindCoherence;
  float32 AngleTolerance;
  float32 AxisTolerance;
  DataPath FaceLabelsArrayPath;
  DataPath FaceNormalsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath TwinBoundariesArrayPath;
  DataPath TwinBoundaryIncoherenceArrayPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeTwinBoundaries
{
public:
  ComputeTwinBoundaries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeTwinBoundariesInputValues* inputValues);
  ~ComputeTwinBoundaries() noexcept;

  ComputeTwinBoundaries(const ComputeTwinBoundaries&) = delete;
  ComputeTwinBoundaries(ComputeTwinBoundaries&&) noexcept = delete;
  ComputeTwinBoundaries& operator=(const ComputeTwinBoundaries&) = delete;
  ComputeTwinBoundaries& operator=(ComputeTwinBoundaries&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeTwinBoundariesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
