#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  LaplacianSmoothingInputValues inputValues;

  inputValues.IterationSteps = filterArgs.value<int32>(k_IterationSteps_Key);
  inputValues.Lambda = filterArgs.value<float32>(k_Lambda_Key);
  inputValues.UseTaubinSmoothing = filterArgs.value<bool>(k_UseTaubinSmoothing_Key);
  inputValues.MuFactor = filterArgs.value<float32>(k_MuFactor_Key);
  inputValues.TripleLineLambda = filterArgs.value<float32>(k_TripleLineLambda_Key);
  inputValues.QuadPointLambda = filterArgs.value<float32>(k_QuadPointLambda_Key);
  inputValues.SurfacePointLambda = filterArgs.value<float32>(k_SurfacePointLambda_Key);
  inputValues.SurfaceTripleLineLambda = filterArgs.value<float32>(k_SurfaceTripleLineLambda_Key);
  inputValues.SurfaceQuadPointLambda = filterArgs.value<float32>(k_SurfaceQuadPointLambda_Key);
  inputValues.SurfaceMeshNodeTypeArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshNodeTypeArrayPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

  return LaplacianSmoothing(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT LaplacianSmoothingInputValues
{
  int32 IterationSteps;
  float32 Lambda;
  bool UseTaubinSmoothing;
  float32 MuFactor;
  float32 TripleLineLambda;
  float32 QuadPointLambda;
  float32 SurfacePointLambda;
  float32 SurfaceTripleLineLambda;
  float32 SurfaceQuadPointLambda;
  DataPath SurfaceMeshNodeTypeArrayPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT LaplacianSmoothing
{
public:
  LaplacianSmoothing(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LaplacianSmoothingInputValues* inputValues);
  ~LaplacianSmoothing() noexcept;

  LaplacianSmoothing(const LaplacianSmoothing&) = delete;
  LaplacianSmoothing(LaplacianSmoothing&&) noexcept = delete;
  LaplacianSmoothing& operator=(const LaplacianSmoothing&) = delete;
  LaplacianSmoothing& operator=(LaplacianSmoothing&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const LaplacianSmoothingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
