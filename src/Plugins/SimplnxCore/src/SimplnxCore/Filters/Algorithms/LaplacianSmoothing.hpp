#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT LaplacianSmoothingInputValues
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
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT LaplacianSmoothing
{
public:
  LaplacianSmoothing(DataStructure& data, LaplacianSmoothingInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~LaplacianSmoothing() noexcept;

  LaplacianSmoothing(const LaplacianSmoothing&) = delete;
  LaplacianSmoothing(LaplacianSmoothing&&) noexcept = delete;
  LaplacianSmoothing& operator=(const LaplacianSmoothing&) = delete;
  LaplacianSmoothing& operator=(LaplacianSmoothing&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const LaplacianSmoothingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::vector<float> generateLambdaArray();
  Result<> edgeBasedSmoothing();
};

} // namespace nx::core
