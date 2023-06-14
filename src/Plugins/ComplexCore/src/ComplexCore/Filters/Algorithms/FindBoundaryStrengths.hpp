#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT FindBoundaryStrengthsInputValues
{
  VectorFloat32Parameter::ValueType Loading;
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

} // namespace complex
