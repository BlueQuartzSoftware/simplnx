#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <fmt/format.h>

#include <string>

namespace nx::core
{
inline constexpr StringLiteral k_MDFArrayName = "MDF";
inline constexpr StringLiteral k_AngleDistributionAMName = "Angle Distribution";
inline constexpr StringLiteral k_AnglesArrayName = "Angles";
inline constexpr StringLiteral k_MDFDensityArrayName = "MDF Density";
inline constexpr StringLiteral k_RandomDensityArrayName = "Random Density";

inline std::string PhaseGroupName(int32 p)
{
  return fmt::format("Phase-{}", p);
}

struct ORIENTATIONANALYSIS_EXPORT ComputeMDFInputValues
{
  DataPath ImageGeometryPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath OutputGroupPath;
  float32 HalfwidthDegrees;
  int32 NumCurvePoints;
  bool UseAreaWeights;
};

/**
 * @class ComputeMDF
 * @brief Computes the correlated Misorientation Distribution Function (MDF) for each Ensemble/Phase
 * using a de la Vallee Poussin kernel density estimate, as implemented in EbsdLib.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeMDF
{
public:
  ComputeMDF(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeMDFInputValues* inputValues);
  ~ComputeMDF() noexcept;

  ComputeMDF(const ComputeMDF&) = delete;
  ComputeMDF(ComputeMDF&&) noexcept = delete;
  ComputeMDF& operator=(const ComputeMDF&) = delete;
  ComputeMDF& operator=(ComputeMDF&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeMDFInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
