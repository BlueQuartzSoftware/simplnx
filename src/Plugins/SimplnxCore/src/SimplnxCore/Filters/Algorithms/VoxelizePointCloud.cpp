#include "VoxelizePointCloud.hpp"

#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"

using namespace nx::core;

namespace
{
Result<> CalculateImageVoxelMask(const INodeGeometry0D::SharedVertexList& pointCloud, const ImageGeom* image, UInt8AbstractDataStore& voxelMaskStore)
{
  const SizeVec3 dims = image->getDimensions();
  const FloatVec3 origin = image->getOrigin();
  const FloatVec3 spacing = image->getSpacing();

  for(usize i = 0; i < pointCloud.getNumberOfTuples(); i++)
  {
    const float32 xRaw = (pointCloud.getValue(i * 3) - origin[0]) / spacing[0];
    if(xRaw < 0.0f)
    {
      continue;
    }
    const usize xPos = static_cast<usize>(xRaw);
    if(xPos >= dims[0])
    {
      continue;
    }

    const float32 yRaw = (pointCloud.getValue(i * 3 + 1) - origin[1]) / spacing[1];
    if(yRaw < 0.0f)
    {
      continue;
    }
    const usize yPos = static_cast<usize>(yRaw);
    if(yPos >= dims[1])
    {
      continue;
    }

    const float32 zRaw = (pointCloud.getValue(i * 3 + 2) - origin[2]) / spacing[2];
    if(zRaw < 0.0f)
    {
      continue;
    }
    const usize zPos = static_cast<usize>(zRaw);
    if(zPos >= dims[2])
    {
      continue;
    }

    const usize target = (zPos * dims[0] * dims[1]) + (yPos * dims[0]) + xPos;
    voxelMaskStore.setValue(target, 1);
  }

  return {};
}

Result<> CalculateRectGridVoxelMask(const INodeGeometry0D::SharedVertexList& pointCloud, const RectGridGeom* rectGrid, UInt8AbstractDataStore& voxelMaskStore)
{
  const Float32AbstractDataStore& xBounds = rectGrid->getXBoundsRef().getDataStoreRef();
  const Float32AbstractDataStore& yBounds = rectGrid->getYBoundsRef().getDataStoreRef();
  const Float32AbstractDataStore& zBounds = rectGrid->getZBoundsRef().getDataStoreRef();

  const SizeVec3 dims = rectGrid->getDimensions();

  for(usize i = 0; i < pointCloud.getNumberOfTuples(); i++)
  {
    const usize xPos = std::upper_bound(xBounds.begin(), xBounds.end(), pointCloud.getValue(i * 3)) - xBounds.begin();
    const usize yPos = std::upper_bound(yBounds.begin(), yBounds.end(), pointCloud.getValue(i * 3 + 1)) - yBounds.begin();
    const usize zPos = std::upper_bound(zBounds.begin(), zBounds.end(), pointCloud.getValue(i * 3 + 2)) - zBounds.begin();

    if(xPos == 0 || xPos > dims[0] || yPos == 0 || yPos > dims[1] || zPos == 0 || zPos > dims[2])
    {
      continue;
    }

    const usize target = ((zPos - 1) * dims[0] * dims[1]) + ((yPos - 1) * dims[0]) + (xPos - 1);
    voxelMaskStore.setValue(target, 1);
  }

  return {};
}

Result<> ResizeImageGeom(const INodeGeometry0D& pointCloud, ImageGeom* image)
{
  constexpr float32 k_PaddingMult = 0.001; // will add 0.1% of the side lengths of the bounding box
  const FloatVec3 spacing = image->getSpacing();

  const BoundingBox3Df bounds = pointCloud.getBoundingBox();

  if(!bounds.isValid())
  {
    return MakeErrorResult(-45980, "Invalid bounding box calculated for the point cloud.");
  }

  Point3Df distance = bounds.sideLengths();
  const Point3Df padding = distance * k_PaddingMult;

  const Point3Df minPoint = bounds.getMinPoint() - padding;
  const Point3Df maxPoint = bounds.getMaxPoint() + padding;
  distance = maxPoint - minPoint;

  const SizeVec3 dims{static_cast<usize>(std::ceil(distance[0] / spacing[0])), static_cast<usize>(std::ceil(distance[1] / spacing[1])), static_cast<usize>(std::ceil(distance[2] / spacing[2]))};

  if(dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    return MakeErrorResult(-45981, fmt::format("Point cloud has zero extent in at least one dimension "
                                               "(computed dims: {}x{}x{}). A minimum size of 1 is required in each axis. "
                                               "Ensure the point cloud spans a non-zero range in all three dimensions.",
                                               dims[0], dims[1], dims[2]));
  }

  image->setDimensions(dims);
  image->setOrigin(minPoint);

  auto* cellData = image->getCellData();
  cellData->resizeTuples(ShapeType{dims[0], dims[1], dims[2]});

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
VoxelizePointCloud::VoxelizePointCloud(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, VoxelizePointCloudInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
VoxelizePointCloud::~VoxelizePointCloud() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& VoxelizePointCloud::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> VoxelizePointCloud::operator()()
{
  const auto& pointCloud = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->PointCloudGeometryPath);

  if(m_InputValues->UseExistingGeom)
  {
    // Type assured to be RectGrid or Image in parameter gates
    const auto* destGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->OutputGeometryPath);
    auto& voxelMask = m_DataStructure.getDataRefAs<UInt8Array>(destGeom->getCellDataPath().createChildPath(m_InputValues->MaskName));

    if(const auto* image = dynamic_cast<const ImageGeom*>(destGeom); image != nullptr)
    {
      return CalculateImageVoxelMask(pointCloud.getVerticesRef(), image, voxelMask.getDataStoreRef());
    }

    const auto* rectGrid = dynamic_cast<const RectGridGeom*>(destGeom);
    return CalculateRectGridVoxelMask(pointCloud.getVerticesRef(), rectGrid, voxelMask.getDataStoreRef());
  }

  // If we are doing a new geometry we need to do a first pass to determine the proper bounds
  auto* destGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->NewGeometryPath);
  auto& voxelMask = m_DataStructure.getDataRefAs<UInt8Array>(destGeom->getCellDataPath().createChildPath(m_InputValues->MaskName));

  if(Result<> result = ResizeImageGeom(pointCloud, destGeom); result.invalid())
  {
    return result;
  }

  return CalculateImageVoxelMask(pointCloud.getVerticesRef(), destGeom, voxelMask.getDataStoreRef());
}
