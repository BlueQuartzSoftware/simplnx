#include "VoxelizePointCloud.hpp"

#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"

#include "simplnx/Utilities/MessageHelper.hpp"

#include <limits>
#include <new>

using namespace nx::core;

namespace
{
constexpr usize k_ProgressInterval = 65536;

Result<usize> CalculateImageVoxelMask(const INodeGeometry0D::SharedVertexList& pointCloud, const ImageGeom* imageGeom, UInt8AbstractDataStore& voxelMaskStore,
                                      const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const Float32AbstractDataStore& vertices = pointCloud.getDataStoreRef();
  const usize numTup = pointCloud.getNumberOfTuples();

  const SizeVec3 dims = imageGeom->getDimensions();
  const FloatVec3 origin = imageGeom->getOrigin();
  const FloatVec3 spacing = imageGeom->getSpacing();

  const float32 xInv = 1.0f / spacing[0];
  const float32 yInv = 1.0f / spacing[1];
  const float32 zInv = 1.0f / spacing[2];

  const usize sliceSize = dims[0] * dims[1];

  const auto dimsXf = static_cast<float32>(dims[0]);
  const auto dimsYf = static_cast<float32>(dims[1]);
  const auto dimsZf = static_cast<float32>(dims[2]);

  usize skipped = 0;

  MessageHelper msgHelper(messageHandler);
  auto progressHelper = msgHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(numTup);
  progressHelper.setProgressMessageTemplate("Voxelizing points: {:.1f}%");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(usize i = 0; i < numTup; i++)
  {
    if(shouldCancel)
    {
      return {skipped};
    }
    if((i & (k_ProgressInterval - 1)) == 0)
    {
      progressMessenger.sendProgressMessage(k_ProgressInterval);
    }

    const float32 xRaw = (vertices.getValue(i * 3) - origin[0]) * xInv;
    if(!(xRaw >= 0.0f && xRaw < dimsXf))
    {
      skipped++;
      continue;
    }
    const auto xPos = static_cast<usize>(xRaw);

    const float32 yRaw = (vertices.getValue(i * 3 + 1) - origin[1]) * yInv;
    if(!(yRaw >= 0.0f && yRaw < dimsYf))
    {
      skipped++;
      continue;
    }
    const auto yPos = static_cast<usize>(yRaw);

    const float32 zRaw = (vertices.getValue(i * 3 + 2) - origin[2]) * zInv;
    if(!(zRaw >= 0.0f && zRaw < dimsZf))
    {
      skipped++;
      continue;
    }
    const auto zPos = static_cast<usize>(zRaw);

    const usize target = (zPos * sliceSize) + (yPos * dims[0]) + xPos;
    voxelMaskStore.setValue(target, 1);
  }

  return {skipped};
}

Result<usize> CalculateRectGridVoxelMask(const INodeGeometry0D::SharedVertexList& pointCloud, const RectGridGeom* rectGridGeom, UInt8AbstractDataStore& voxelMaskStore,
                                         const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const Float32AbstractDataStore& vertices = pointCloud.getDataStoreRef();

  // Copy bounds into local vectors so std::upper_bound iterates over concrete
  // float32* rather than calling getValue virtually through the abstract store.
  // Cost is O(dims[i]) — typically a few hundred floats — not O(numPoints).
  const std::vector<float32> xBounds(rectGridGeom->getXBoundsRef().getDataStoreRef().begin(), rectGridGeom->getXBoundsRef().getDataStoreRef().end());
  const std::vector<float32> yBounds(rectGridGeom->getYBoundsRef().getDataStoreRef().begin(), rectGridGeom->getYBoundsRef().getDataStoreRef().end());
  const std::vector<float32> zBounds(rectGridGeom->getZBoundsRef().getDataStoreRef().begin(), rectGridGeom->getZBoundsRef().getDataStoreRef().end());

  const SizeVec3 dims = rectGridGeom->getDimensions();
  const usize numTup = pointCloud.getNumberOfTuples();

  const usize sliceSize = dims[0] * dims[1];

  usize skipped = 0;

  MessageHelper msgHelper(messageHandler);
  auto progressHelper = msgHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(numTup);
  progressHelper.setProgressMessageTemplate("Voxelizing points: {:.1f}%");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(usize i = 0; i < numTup; i++)
  {
    if(shouldCancel)
    {
      return {skipped};
    }
    if((i & (k_ProgressInterval - 1)) == 0)
    {
      progressMessenger.sendProgressMessage(k_ProgressInterval);
    }
    const usize xPos = std::upper_bound(xBounds.begin(), xBounds.end(), vertices.getValue(i * 3)) - xBounds.begin();
    if(xPos == 0 || xPos > dims[0])
    {
      skipped++;
      continue;
    }
    const usize yPos = std::upper_bound(yBounds.begin(), yBounds.end(), vertices.getValue(i * 3 + 1)) - yBounds.begin();
    if(yPos == 0 || yPos > dims[1])
    {
      skipped++;
      continue;
    }
    const usize zPos = std::upper_bound(zBounds.begin(), zBounds.end(), vertices.getValue(i * 3 + 2)) - zBounds.begin();
    if(zPos == 0 || zPos > dims[2])
    {
      skipped++;
      continue;
    }

    const usize target = ((zPos - 1) * sliceSize) + ((yPos - 1) * dims[0]) + (xPos - 1);
    voxelMaskStore.setValue(target, 1);
  }

  return {skipped};
}

Result<> ResizeImageGeom(const INodeGeometry0D& pointCloud, ImageGeom* imageGeom)
{
  constexpr float32 k_PaddingMult = 0.001f; // will add 0.1% of the side lengths of the bounding box
  const FloatVec3 spacing = imageGeom->getSpacing();

  const BoundingBox3Df bounds = pointCloud.getBoundingBox();

  if(!bounds.isValid())
  {
    return MakeErrorResult(-45980, "Invalid bounding box calculated for the point cloud.");
  }

  Point3Df distance = bounds.sideLengths();
  const Point3Df padding = distance * k_PaddingMult;

  const Point3Df rawMinPoint = bounds.getMinPoint() - padding;
  const Point3Df rawMaxPoint = bounds.getMaxPoint() + padding;

  // When proportional padding falls below float32 ULP at large coordinate magnitudes
  // (e.g. coordinates ~1e7 with a sub-millimetre extent), the addition has no effect and
  // boundary points can be silently excluded. nextafter guarantees at least 1 ULP of
  // expansion beyond the original bounding box faces regardless of coordinate magnitude.
  constexpr float32 k_Inf = std::numeric_limits<float32>::infinity();
  const Point3Df origMin = bounds.getMinPoint();
  const Point3Df origMax = bounds.getMaxPoint();
  const Point3Df minPoint{std::min(rawMinPoint[0], std::nextafter(origMin[0], -k_Inf)), std::min(rawMinPoint[1], std::nextafter(origMin[1], -k_Inf)),
                          std::min(rawMinPoint[2], std::nextafter(origMin[2], -k_Inf))};
  const Point3Df maxPoint{std::max(rawMaxPoint[0], std::nextafter(origMax[0], k_Inf)), std::max(rawMaxPoint[1], std::nextafter(origMax[1], k_Inf)),
                          std::max(rawMaxPoint[2], std::nextafter(origMax[2], k_Inf))};
  distance = maxPoint - minPoint;

  const SizeVec3 dims{std::max(usize{1}, static_cast<usize>(std::ceil(distance[0] / spacing[0]))), std::max(usize{1}, static_cast<usize>(std::ceil(distance[1] / spacing[1]))),
                      std::max(usize{1}, static_cast<usize>(std::ceil(distance[2] / spacing[2])))};

  imageGeom->setDimensions(dims);
  imageGeom->setOrigin(minPoint);

  auto* cellData = imageGeom->getCellData();
  try
  {
    cellData->resizeTuples(ShapeType{dims[2], dims[1], dims[0]});
  } catch(const std::bad_alloc&)
  {
    return MakeErrorResult(-45982, fmt::format("Failed to allocate voxel grid of {}x{}x{} voxels. "
                                               "The point cloud extent relative to the current spacing is too large.",
                                               dims[0], dims[1], dims[2]));
  }

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
Result<> VoxelizePointCloud::operator()()
{
  const auto& pointCloud = m_DataStructure.getDataRefAs<INodeGeometry0D>(m_InputValues->PointCloudGeometryPath);
  const usize numPoints = pointCloud.getVerticesRef().getNumberOfTuples();

  auto emitSkipWarning = [&](usize skipped) {
    if(skipped > 0)
    {
      m_MessageHandler(IFilter::Message{IFilter::Message::Type::Warning,
                                        fmt::format("{} of {} point(s) had non-finite coordinates or fell outside the destination geometry and were not voxelized.", skipped, numPoints)});
    }
  };

  if(m_InputValues->UseExistingGeom)
  {
    const auto* destGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->OutputGeometryPath);
    auto& voxelMask = m_DataStructure.getDataRefAs<UInt8Array>(destGeom->getCellDataPath().createChildPath(m_InputValues->MaskName));

    if(const auto* image = dynamic_cast<const ImageGeom*>(destGeom); image != nullptr)
    {
      auto result = CalculateImageVoxelMask(pointCloud.getVerticesRef(), image, voxelMask.getDataStoreRef(), m_MessageHandler, m_ShouldCancel);
      emitSkipWarning(result.value());
      return ConvertResult(std::move(result));
    }

    const auto* rectGrid = dynamic_cast<const RectGridGeom*>(destGeom);
    if(rectGrid == nullptr)
    {
      return MakeErrorResult(-45988, fmt::format("Unsupported grid geometry type at '{}'. Only Image Geometry and RectGrid Geometry are supported.", m_InputValues->OutputGeometryPath.toString()));
    }
    auto result = CalculateRectGridVoxelMask(pointCloud.getVerticesRef(), rectGrid, voxelMask.getDataStoreRef(), m_MessageHandler, m_ShouldCancel);
    emitSkipWarning(result.value());
    return ConvertResult(std::move(result));
  }

  // If we are doing a new geometry we need to do a first pass to determine the proper bounds
  auto* destGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->NewGeometryPath);

  if(Result<> result = ResizeImageGeom(pointCloud, destGeom); result.invalid())
  {
    return result;
  }

  auto& voxelMask = m_DataStructure.getDataRefAs<UInt8Array>(destGeom->getCellDataPath().createChildPath(m_InputValues->MaskName));

  auto result = CalculateImageVoxelMask(pointCloud.getVerticesRef(), destGeom, voxelMask.getDataStoreRef(), m_MessageHandler, m_ShouldCancel);
  emitSkipWarning(result.value());
  return ConvertResult(std::move(result));
}
