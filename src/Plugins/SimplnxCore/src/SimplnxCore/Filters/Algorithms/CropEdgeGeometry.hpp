#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace
{
const std::string k_TempGeometryName = ".cropped_edge_geometry";
}

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CropEdgeGeometryInputValues
{
  DataPath srcEdgeGeomPath;
  DataPath destEdgeGeomPath;
  std::vector<float32> minCoords;
  std::vector<float32> maxCoords;
  bool removeOriginalGeometry;
  bool cropXDim;
  bool cropYDim;
  bool cropZDim;
  uint64 boundaryIntersectionBehavior;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT CropEdgeGeometry
{
public:
  CropEdgeGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, CropEdgeGeometryInputValues* inputValues);
  ~CropEdgeGeometry() noexcept;

  CropEdgeGeometry(const CropEdgeGeometry&) = delete;
  CropEdgeGeometry(CropEdgeGeometry&&) noexcept = delete;
  CropEdgeGeometry& operator=(const CropEdgeGeometry&) = delete;
  CropEdgeGeometry& operator=(CropEdgeGeometry&&) noexcept = delete;

  enum class BoundaryIntersectionBehavior : uint64
  {
    InterpolateOutsideVertex = 0,
    IgnoreEdge = 1,
    FilterError = 2
  };

  enum class ErrorCodes : int64
  {
    XMinLargerThanXMax = -1210,
    YMinLargerThanYMax = -1211,
    ZMinLargerThanZMax = -1212,
    NoDimensionsChosen = -1213,
    InvalidVertexIndex = -1220,
    OutsideVertexError = -1221,
    InvalidVertexMapping = -1222
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CropEdgeGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
