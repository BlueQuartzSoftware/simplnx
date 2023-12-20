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

struct ORIENTATIONANALYSIS_EXPORT FindBoundaryStrengthsInputValues
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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindBoundaryStrengths
{
public:
  FindBoundaryStrengths(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundaryStrengthsInputValues* inputValues);
  ~FindBoundaryStrengths() noexcept;

  FindBoundaryStrengths(const FindBoundaryStrengths&) = delete;
  FindBoundaryStrengths(FindBoundaryStrengths&&) noexcept = delete;
  FindBoundaryStrengths& operator=(const FindBoundaryStrengths&) = delete;
  FindBoundaryStrengths& operator=(FindBoundaryStrengths&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindBoundaryStrengthsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
