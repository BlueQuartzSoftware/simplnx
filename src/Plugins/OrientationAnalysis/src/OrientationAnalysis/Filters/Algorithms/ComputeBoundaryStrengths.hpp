#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeBoundaryStrengthsInputValues
{
  VectorFloat64Parameter::ValueType Loading;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath SurfaceMeshF1sArrayName;
  DataPath SurfaceMeshF1sptsArrayName;
  DataPath SurfaceMeshF7sArrayName;
  DataPath SurfaceMeshmPrimesArrayName;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeBoundaryStrengths
{
public:
  ComputeBoundaryStrengths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBoundaryStrengthsInputValues* inputValues);
  ~ComputeBoundaryStrengths() noexcept;

  ComputeBoundaryStrengths(const ComputeBoundaryStrengths&) = delete;
  ComputeBoundaryStrengths(ComputeBoundaryStrengths&&) noexcept = delete;
  ComputeBoundaryStrengths& operator=(const ComputeBoundaryStrengths&) = delete;
  ComputeBoundaryStrengths& operator=(ComputeBoundaryStrengths&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryStrengthsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
