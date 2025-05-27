#include "ComputeFeatureBounds.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
namespace GeometryGrid
{
std::pair<Point3Df, Point3Df> CalculateFeatureBounds(int32 activeFeature, const ImageGeom& imageGeom, const Int32AbstractDataStore& featureIds)
{
  float32 maxX = std::numeric_limits<float32>::lowest();
  float32 maxY = std::numeric_limits<float32>::lowest();
  float32 maxZ = std::numeric_limits<float32>::lowest();

  float32 minX = std::numeric_limits<float32>::max();
  float32 minY = std::numeric_limits<float32>::max();
  float32 minZ = std::numeric_limits<float32>::max();

  size_t xPoints = imageGeom.getNumXCells();
  size_t yPoints = imageGeom.getNumYCells();
  size_t zPoints = imageGeom.getNumZCells();
  FloatVec3 spacing = imageGeom.getSpacing();
  FloatVec3 origin = imageGeom.getOrigin();

  size_t zStride = 0, yStride = 0;
  for(size_t i = 0; i < zPoints; i++)
  {
    zStride = i * xPoints * yPoints;
    for(size_t j = 0; j < yPoints; j++)
    {
      yStride = j * xPoints;
      for(size_t k = 0; k < xPoints; k++)
      {
        if(featureIds[zStride + yStride + k] == activeFeature)
        {
          // We are inlining the calculations here to leverage the speed of primitives (no Point object or vector from the API)
          float32 xVal = k * spacing[0] + origin[0] + (0.5f * spacing[0]);
          float32 yVal = j * spacing[1] + origin[1] + (0.5f * spacing[1]);
          float32 zVal = i * spacing[2] + origin[2] + (0.5f * spacing[2]);

          minX = std::min(minX, xVal);
          minY = std::min(minY, yVal);
          minZ = std::min(minZ, zVal);

          maxX = std::max(maxX, xVal);
          maxY = std::max(maxY, yVal);
          maxZ = std::max(maxZ, zVal);
        }
      }
    }
  }

  return std::make_pair(Point3Df(minX, minY, minZ), Point3Df(maxX, maxY, maxZ));
}
} // namespace GeometryGrid
namespace Geometry0D
{
std::pair<Point3Df, Point3Df> CalculateFeatureBounds(int32 activeFeature, const IGeometry::SharedVertexList::store_type& verts, const Int32AbstractDataStore& featureIds)
{
  float32 maxX = std::numeric_limits<float32>::lowest();
  float32 maxY = std::numeric_limits<float32>::lowest();
  float32 maxZ = std::numeric_limits<float32>::lowest();

  float32 minX = std::numeric_limits<float32>::max();
  float32 minY = std::numeric_limits<float32>::max();
  float32 minZ = std::numeric_limits<float32>::max();

  for(usize i = 0; i < verts.getNumberOfTuples(); i++)
  {
    if(featureIds[i] == activeFeature)
    {
      float32 xVal = verts[(i * 3) + 0];
      float32 yVal = verts[(i * 3) + 1];
      float32 zVal = verts[(i * 3) + 2];

      minX = std::min(minX, xVal);
      minY = std::min(minY, yVal);
      minZ = std::min(minZ, zVal);

      maxX = std::max(maxX, xVal);
      maxY = std::max(maxY, yVal);
      maxZ = std::max(maxZ, zVal);
    }
  }

  return std::make_pair(Point3Df(minX, minY, minZ), Point3Df(maxX, maxY, maxZ));
}
} // namespace Geometry0D
namespace Geometry1D
{
std::pair<Point3Df, Point3Df> CalculateFeatureBounds(int32 activeFeature, const IGeometry::SharedVertexList::store_type& verts, const IGeometry::SharedEdgeList::store_type& edges,
                                                     const Int32AbstractDataStore& featureIds)
{
  float32 maxX = std::numeric_limits<float32>::lowest();
  float32 maxY = std::numeric_limits<float32>::lowest();
  float32 maxZ = std::numeric_limits<float32>::lowest();

  float32 minX = std::numeric_limits<float32>::max();
  float32 minY = std::numeric_limits<float32>::max();
  float32 minZ = std::numeric_limits<float32>::max();

  usize numComp = edges.getNumberOfComponents();
  for(usize i = 0; i < edges.getNumberOfTuples(); i++)
  {
    if(featureIds[i] == activeFeature)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        const IGeometry::SharedFaceList::value_type activeVertIndex = edges[(i * numComp) + comp];
        float32 xVal = verts[(activeVertIndex * 3) + 0];
        float32 yVal = verts[(activeVertIndex * 3) + 1];
        float32 zVal = verts[(activeVertIndex * 3) + 2];

        minX = std::min(minX, xVal);
        minY = std::min(minY, yVal);
        minZ = std::min(minZ, zVal);

        maxX = std::max(maxX, xVal);
        maxY = std::max(maxY, yVal);
        maxZ = std::max(maxZ, zVal);
      }
    }
  }

  return std::make_pair(Point3Df(minX, minY, minZ), Point3Df(maxX, maxY, maxZ));
}
} // namespace Geometry1D
namespace Geometry2D
{
std::pair<Point3Df, Point3Df> CalculateFeatureBounds(int32 activeFeature, const IGeometry::SharedVertexList::store_type& verts, const IGeometry::SharedFaceList::store_type& faces,
                                                     const Int32AbstractDataStore& featureIds)
{
  float32 maxX = std::numeric_limits<float32>::lowest();
  float32 maxY = std::numeric_limits<float32>::lowest();
  float32 maxZ = std::numeric_limits<float32>::lowest();

  float32 minX = std::numeric_limits<float32>::max();
  float32 minY = std::numeric_limits<float32>::max();
  float32 minZ = std::numeric_limits<float32>::max();

  usize numComp = faces.getNumberOfComponents();
  for(usize i = 0; i < faces.getNumberOfTuples(); i++)
  {
    if(featureIds[i] == activeFeature)
    {
      for(usize comp = 0; comp < numComp; comp++)
      {
        const IGeometry::SharedFaceList::value_type activeVertIndex = faces[(i * numComp) + comp];
        float32 xVal = verts[(activeVertIndex * 3) + 0];
        float32 yVal = verts[(activeVertIndex * 3) + 1];
        float32 zVal = verts[(activeVertIndex * 3) + 2];

        minX = std::min(minX, xVal);
        minY = std::min(minY, yVal);
        minZ = std::min(minZ, zVal);

        maxX = std::max(maxX, xVal);
        maxY = std::max(maxY, yVal);
        maxZ = std::max(maxZ, zVal);
      }
    }
  }

  return std::make_pair(Point3Df(minX, minY, minZ), Point3Df(maxX, maxY, maxZ));
}
} // namespace Geometry2D

template <typename T>
concept GeometryType = std::is_base_of_v<IGeometry, T>;

template <GeometryType GeomT>
class ComputeSplitBoundsImpl
{
public:
  ComputeSplitBoundsImpl(const GeomT& geom, const Int32AbstractDataStore& featureIds, Float32AbstractDataStore& minBounds, Float32AbstractDataStore& maxBounds)
  : m_Geom(geom)
  , m_FeatureIds(featureIds)
  , m_MinBounds(minBounds)
  , m_MaxBounds(maxBounds)
  {
  }
  ~ComputeSplitBoundsImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    std::pair<Point3Df, Point3Df> minMax;
    for(usize feature = start; feature < end; feature++)
    {
      if constexpr(std::is_same_v<ImageGeom, GeomT>)
      {
        minMax = GeometryGrid::CalculateFeatureBounds(feature, m_Geom, m_FeatureIds);
      }
      if constexpr(std::is_same_v<VertexGeom, GeomT>)
      {
        const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
        minMax = Geometry0D::CalculateFeatureBounds(feature, verts, m_FeatureIds);
      }
      if constexpr(std::is_same_v<EdgeGeom, GeomT>)
      {
        const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
        const IGeometry::SharedEdgeList::store_type& edges = m_Geom.getEdgesRef().getDataStoreRef();
        minMax = Geometry1D::CalculateFeatureBounds(feature, verts, edges, m_FeatureIds);
      }
      if constexpr(std::is_same_v<TriangleGeom, GeomT> || std::is_same_v<QuadGeom, GeomT>)
      {
        const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
        const IGeometry::SharedFaceList::store_type& faces = m_Geom.getFacesRef().getDataStoreRef();
        minMax = Geometry2D::CalculateFeatureBounds(feature, verts, faces, m_FeatureIds);
      }

      // Set Min
      m_MinBounds[(feature * 3) + 0] = minMax.first.getX();
      m_MinBounds[(feature * 3) + 1] = minMax.first.getY();
      m_MinBounds[(feature * 3) + 2] = minMax.first.getZ();

      // Set Max
      m_MaxBounds[(feature * 3) + 0] = minMax.second.getX();
      m_MaxBounds[(feature * 3) + 1] = minMax.second.getY();
      m_MaxBounds[(feature * 3) + 2] = minMax.second.getZ();
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const GeomT& m_Geom;
  const Int32AbstractDataStore& m_FeatureIds;
  Float32AbstractDataStore& m_MinBounds;
  Float32AbstractDataStore& m_MaxBounds;
};

template <GeometryType GeomT>
class ComputeUnifiedBoundsImpl
{
public:
  ComputeUnifiedBoundsImpl(const GeomT& geom, const Int32AbstractDataStore& featureIds, Float32AbstractDataStore& unifiedBounds)
  : m_Geom(geom)
  , m_FeatureIds(featureIds)
  , m_UnifiedBounds(unifiedBounds)
  {
  }
  ~ComputeUnifiedBoundsImpl() = default;

  // -----------------------------------------------------------------------------
  void compute(usize start, usize end) const
  {
    static_assert(std::is_same_v<ImageGeom, GeomT> || std::is_base_of_v<INodeGeometry0D, GeomT> || std::is_base_of_v<INodeGeometry1D, GeomT> || std::is_base_of_v<INodeGeometry2D, GeomT>);
    std::pair<Point3Df, Point3Df> minMax;
    for(usize feature = start; feature < end; feature++)
    {
      if constexpr(std::is_same_v<ImageGeom, GeomT>)
      {
        minMax = GeometryGrid::CalculateFeatureBounds(feature, m_Geom, m_FeatureIds);
      }
      if constexpr(std::is_same_v<VertexGeom, GeomT>)
      {
        const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
        minMax = Geometry0D::CalculateFeatureBounds(feature, verts, m_FeatureIds);
      }
      if constexpr(std::is_same_v<EdgeGeom, GeomT>)
      {
        const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
        const IGeometry::SharedEdgeList::store_type& edges = m_Geom.getEdgesRef().getDataStoreRef();
        minMax = Geometry1D::CalculateFeatureBounds(feature, verts, edges, m_FeatureIds);
      }
      if constexpr(std::is_same_v<TriangleGeom, GeomT> || std::is_same_v<QuadGeom, GeomT>)
      {
        const IGeometry::SharedVertexList::store_type& verts = m_Geom.getVerticesRef().getDataStoreRef();
        const IGeometry::SharedFaceList::store_type& faces = m_Geom.getFacesRef().getDataStoreRef();
        minMax = Geometry2D::CalculateFeatureBounds(feature, verts, faces, m_FeatureIds);
      }

      // Set Min
      m_UnifiedBounds[(feature * 6) + 0] = minMax.first.getX();
      m_UnifiedBounds[(feature * 6) + 1] = minMax.first.getY();
      m_UnifiedBounds[(feature * 6) + 2] = minMax.first.getZ();

      // Set Max
      m_UnifiedBounds[(feature * 6) + 3] = minMax.second.getX();
      m_UnifiedBounds[(feature * 6) + 4] = minMax.second.getY();
      m_UnifiedBounds[(feature * 6) + 5] = minMax.second.getZ();
    }
  }

  // -----------------------------------------------------------------------------
  void operator()(const Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  const GeomT& m_Geom;
  const Int32AbstractDataStore& m_FeatureIds;
  Float32AbstractDataStore& m_UnifiedBounds;
};

template <template <class> class BodyT, class... ArgsT>
Result<> ExecuteComputeBounds(const IGeometry& geom, ParallelDataAlgorithm&& dataAlg, ArgsT&&... args)
{
  switch(geom.getGeomType())
  {
  case IGeometry::Type::Image: {
    dataAlg.execute(BodyT<ImageGeom>(dynamic_cast<const ImageGeom&>(geom), std::forward<ArgsT>(args)...));
    break;
  }
  case IGeometry::Type::Triangle: {
    dataAlg.execute(BodyT<TriangleGeom>(dynamic_cast<const TriangleGeom&>(geom), std::forward<ArgsT>(args)...));
    break;
  }
  case IGeometry::Type::Vertex: {
    dataAlg.execute(BodyT<VertexGeom>(dynamic_cast<const VertexGeom&>(geom), std::forward<ArgsT>(args)...));
    break;
  }
  case IGeometry::Type::Edge: {
    dataAlg.execute(BodyT<EdgeGeom>(dynamic_cast<const EdgeGeom&>(geom), std::forward<ArgsT>(args)...));
    break;
  }
  case IGeometry::Type::Quad: {
    dataAlg.execute(BodyT<QuadGeom>(dynamic_cast<const QuadGeom&>(geom), std::forward<ArgsT>(args)...));
    break;
  }
  default: {
    return MakeErrorResult(-89472, fmt::format("Input geometry must be of type(s) [ Image, Triangle, Vertex, Edge, Quad ]. Supplied geometry name {}", geom.getName()));
  }
  }
  return {};
}
} // namespace

// -----------------------------------------------------------------------------
ComputeFeatureBounds::ComputeFeatureBounds(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureBoundsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureBounds::~ComputeFeatureBounds() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureBounds::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& featureAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->FeatureAMPath);

  const int32 numFeatures = (*std::max_element(featureIds.cbegin(), featureIds.cend())) + 1;
  if(numFeatures > featureAM.getNumTuples())
  {
    return MakeErrorResult(-89471, fmt::format("{} Attribute Matrix size ({}) doesn't align with number of features ({}) in {} array", m_InputValues->FeatureAMPath.getTargetName(),
                                               featureAM.getNumTuples(), numFeatures, m_InputValues->FeatureIdsArrayPath.getTargetName()));
  }

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, numFeatures);

  const auto& geom = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->GeometryPath);
  switch(static_cast<OutputDataType>(m_InputValues->OutputType))
  {
  case OutputDataType::Split: {
    auto& minArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MinArrayPath).getDataStoreRef();
    auto& maxArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MaxArrayPath).getDataStoreRef();
    return ExecuteComputeBounds<::ComputeSplitBoundsImpl>(geom, std::move(dataAlg), featureIds, minArray, maxArray);
  }
  case OutputDataType::Unified: {
    auto& unifiedArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->UnifiedArrayPath).getDataStoreRef();
    return ExecuteComputeBounds<::ComputeUnifiedBoundsImpl>(geom, std::move(dataAlg), featureIds, unifiedArray);
  }
  }

  return {};
}
