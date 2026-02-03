#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ExtractFeatureBoundaries2DInputValues
{
  DataPath InputImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath OutputEdgeGeometryPath;
  uint64 ZValueChoice = 0;
  float32 CustomZValue = 0.0f;
  bool ExtractVirtualSampleEdges = false;
};

/**
 * @class ExtractFeatureBoundaries2D
 * @brief This algorithm extracts 2D feature boundaries from an Image Geometry and creates an Edge Geometry.
 */
class SIMPLNXCORE_EXPORT ExtractFeatureBoundaries2D
{
public:
  ExtractFeatureBoundaries2D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractFeatureBoundaries2DInputValues* inputValues);
  ~ExtractFeatureBoundaries2D() noexcept;

  ExtractFeatureBoundaries2D(const ExtractFeatureBoundaries2D&) = delete;
  ExtractFeatureBoundaries2D(ExtractFeatureBoundaries2D&&) noexcept = delete;
  ExtractFeatureBoundaries2D& operator=(const ExtractFeatureBoundaries2D&) = delete;
  ExtractFeatureBoundaries2D& operator=(ExtractFeatureBoundaries2D&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractFeatureBoundaries2DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
