#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeVertexToTriangleDistancesInputValues
{
  DataPath VertexDataContainer;
  DataPath TriangleDataContainer;
  DataPath TriangleNormalsArrayPath;
  DataPath DistancesArrayPath;
  DataPath ClosestTriangleIdArrayPath;
  DataPath TriBoundsDataPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeVertexToTriangleDistances
{
public:
  ComputeVertexToTriangleDistances(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   ComputeVertexToTriangleDistancesInputValues* inputValues);
  ~ComputeVertexToTriangleDistances() noexcept;

  ComputeVertexToTriangleDistances(const ComputeVertexToTriangleDistances&) = delete;
  ComputeVertexToTriangleDistances(ComputeVertexToTriangleDistances&&) noexcept = delete;
  ComputeVertexToTriangleDistances& operator=(const ComputeVertexToTriangleDistances&) = delete;
  ComputeVertexToTriangleDistances& operator=(ComputeVertexToTriangleDistances&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeVertexToTriangleDistancesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
