#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include <vector>

namespace complex
{

struct LaplacianSmoothingInputValues
{
  DataPath pTriangleGeometryDataPath;
  int32 pIterationSteps;
  float32 pLambda;
  bool pUseTaubinSmoothing;
  float32 pMuFactor;
  float32 pTripleLineLambda;
  float32 pQuadPointLambda;
  float32 pSurfacePointLambda;
  float32 pSurfaceTripleLineLambda;
  float32 pSurfaceQuadPointLambda;
  DataPath pSurfaceMeshNodeTypeArrayPath;
  DataPath pSurfaceMeshFaceLabelsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IFilter;

class COMPLEXCORE_EXPORT LaplacianSmoothing
{
public:
  LaplacianSmoothing(DataStructure& data, LaplacianSmoothingInputValues* inputValues, const IFilter* filter);
  ~LaplacianSmoothing() noexcept;

  LaplacianSmoothing(const LaplacianSmoothing&) = delete;
  LaplacianSmoothing(LaplacianSmoothing&&) noexcept = delete;
  LaplacianSmoothing& operator=(const LaplacianSmoothing&) = delete;
  LaplacianSmoothing& operator=(LaplacianSmoothing&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const LaplacianSmoothingInputValues* m_InputValues = nullptr;
  const IFilter* m_Filter = nullptr;

  std::vector<float> generateLambdaArray();
  Result<> edgeBasedSmoothing();
};

} // namespace complex
