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

std::array<float, 6> copyBoundsForFeature(const ComputeFeatureBoundsInputValues* inputValues, const DataStructure& datatStructure, size_t featureIndex)
{

  switch(auto outputType = static_cast<ComputeFeatureBounds::OutputDataType>(inputValues->OutputType))
  {
  case ComputeFeatureBounds::OutputDataType::Split: {
    auto& minArray = datatStructure.getDataRefAs<Float32Array>(inputValues->MinArrayPath).getDataStoreRef();
    auto& maxArray = datatStructure.getDataRefAs<Float32Array>(inputValues->MaxArrayPath).getDataStoreRef();

    return {minArray[featureIndex * 3], minArray[featureIndex * 3 + 1], minArray[featureIndex * 3 + 2], maxArray[featureIndex * 3], maxArray[featureIndex * 3 + 1], maxArray[featureIndex * 3 + 2]};
  }
  case ComputeFeatureBounds::OutputDataType::Unified: {
    auto& unifiedArray = datatStructure.getDataRefAs<Float32Array>(inputValues->UnifiedArrayPath).getDataStoreRef();
    return {unifiedArray[featureIndex * 3],     unifiedArray[featureIndex * 3 + 1], unifiedArray[featureIndex * 3 + 2],
            unifiedArray[featureIndex * 3 + 3], unifiedArray[featureIndex * 3 + 4], unifiedArray[featureIndex * 3 + 5]};
  }
  }
  return {};
}

namespace GeometryGrid
{

std::pair<Point3Df, Point3Df> CalculateFeatureBounds(int32 activeFeature, const ImageGeom& imageGeom, const Int32AbstractDataStore& featureIds)
{

  float32 maxX = std::numeric_limits<float32>::quiet_NaN();
  float32 maxY = std::numeric_limits<float32>::quiet_NaN();
  float32 maxZ = std::numeric_limits<float32>::quiet_NaN();

  float32 minX = std::numeric_limits<float32>::quiet_NaN();
  float32 minY = std::numeric_limits<float32>::quiet_NaN();
  float32 minZ = std::numeric_limits<float32>::quiet_NaN();

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
          float32 xValMin = k * spacing[0] + origin[0];
          float32 yValMin = j * spacing[1] + origin[1];
          float32 zValMin = i * spacing[2] + origin[2];

          float32 xValMax = k * spacing[0] + origin[0] + spacing[0];
          float32 yValMax = j * spacing[1] + origin[1] + spacing[1];
          float32 zValMax = i * spacing[2] + origin[2] + spacing[2];

          minX = std::isnan(minX) ? xValMin : std::min(minX, xValMin);
          minY = std::isnan(minY) ? yValMin : std::min(minY, yValMin);
          minZ = std::isnan(minZ) ? zValMin : std::min(minZ, zValMin);

          maxX = std::isnan(maxX) ? xValMax : std::max(maxX, xValMax);
          maxY = std::isnan(maxY) ? yValMax : std::max(maxY, yValMax);
          maxZ = std::isnan(maxZ) ? zValMax : std::max(maxZ, zValMax);
        }
      }
    }
  }

  return std::make_pair(Point3Df(minX, minY, minZ), Point3Df(maxX, maxY, maxZ));
}

using MinMaxPairType = std::pair<Point3Df, Point3Df>;
std::vector<MinMaxPairType> CalculateFeatureBounds2(int32 numFeatures, const ImageGeom& imageGeom, const Int32AbstractDataStore& featureIds)
{
  auto quietNan = std::numeric_limits<float32>::quiet_NaN();
  std::vector<MinMaxPairType> features(numFeatures, MinMaxPairType{{quietNan, quietNan, quietNan}, {quietNan, quietNan, quietNan}});

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
        const int32 currentFeatureId = featureIds[zStride + yStride + k];
        if(currentFeatureId < 0)
        {
          continue;
        }
        auto& minPoint = features[currentFeatureId].first;
        auto& maxPoint = features[currentFeatureId].second;

        float32 xValMin = k * spacing[0] + origin[0];
        float32 yValMin = j * spacing[1] + origin[1];
        float32 zValMin = i * spacing[2] + origin[2];

        float32 xValMax = k * spacing[0] + origin[0] + spacing[0];
        float32 yValMax = j * spacing[1] + origin[1] + spacing[1];
        float32 zValMax = i * spacing[2] + origin[2] + spacing[2];

        float32 minX = std::isnan(minPoint[0]) ? xValMin : std::min(minPoint[0], xValMin);
        float32 minY = std::isnan(minPoint[1]) ? yValMin : std::min(minPoint[1], yValMin);
        float32 minZ = std::isnan(minPoint[2]) ? zValMin : std::min(minPoint[2], zValMin);

        float32 maxX = std::isnan(maxPoint[0]) ? xValMax : std::max(maxPoint[0], xValMax);
        float32 maxY = std::isnan(maxPoint[1]) ? yValMax : std::max(maxPoint[1], yValMax);
        float32 maxZ = std::isnan(maxPoint[2]) ? zValMax : std::max(maxPoint[2], zValMax);

        features[currentFeatureId] = {{minX, minY, minZ}, {maxX, maxY, maxZ}};
      }
    }
  }

  return features;
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

  const auto& geom = m_DataStructure.getDataRefAs<IGeometry>(m_InputValues->GeometryPath);

  // Specialize on the Image Geometry to flip the loops so we loop over the entire
  // image geometry only once updating the min and max as we go. This equates to
  // about a 10x speed up in release mode.
  if(geom.getGeomType() == IGeometry::Type::Image)
  {
    auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->GeometryPath);
    std::vector<GeometryGrid::MinMaxPairType> output = GeometryGrid::CalculateFeatureBounds2(numFeatures, imageGeom, featureIds);
    switch(static_cast<OutputDataType>(m_InputValues->OutputType))
    {
    case OutputDataType::Split: {
      auto& minArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MinArrayPath).getDataStoreRef();
      auto& maxArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MaxArrayPath).getDataStoreRef();
      for(size_t i = 0; i < numFeatures; i++)
      {
        minArray.setTuple(i, output[i].first.data());
        maxArray.setTuple(i, output[i].second.data());
      }
      break;
    }
    case OutputDataType::Unified: {
      auto& unifiedArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->UnifiedArrayPath).getDataStoreRef();
      for(size_t i = 0; i < numFeatures; i++)
      {
        unifiedArray.setComponent(i, 0, output[i].first[0]);
        unifiedArray.setComponent(i, 1, output[i].first[1]);
        unifiedArray.setComponent(i, 2, output[i].first[2]);
        unifiedArray.setComponent(i, 3, output[i].second[0]);
        unifiedArray.setComponent(i, 4, output[i].second[1]);
        unifiedArray.setComponent(i, 5, output[i].second[2]);
      }
      break;
    }
    }
  }
  else
  {
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, numFeatures);

    switch(static_cast<OutputDataType>(m_InputValues->OutputType))
    {
    case OutputDataType::Split: {
      auto& minArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MinArrayPath).getDataStoreRef();
      minArray.fill(std::numeric_limits<float>::quiet_NaN());
      auto& maxArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->MaxArrayPath).getDataStoreRef();
      maxArray.fill(std::numeric_limits<float>::quiet_NaN());
      auto result = ExecuteComputeBounds<::ComputeSplitBoundsImpl>(geom, std::move(dataAlg), featureIds, minArray, maxArray);
      if(result.invalid())
      {
        return result;
      }
      break;
    }
    case OutputDataType::Unified: {
      auto& unifiedArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->UnifiedArrayPath).getDataStoreRef();
      unifiedArray.fill(std::numeric_limits<float>::quiet_NaN());
      auto result = ExecuteComputeBounds<::ComputeUnifiedBoundsImpl>(geom, std::move(dataAlg), featureIds, unifiedArray);
      if(result.invalid())
      {
        return result;
      }
      break;
    }
    }
  }

  if(m_InputValues->CreateEdgeGeometry)
  {
    // define all 12 cube edges as pairs of vertex indices
    static constexpr std::array<std::pair<int, int>, 12> cubeEdges = {{// bottom face
                                                                       {0, 1},
                                                                       {1, 2},
                                                                       {2, 3},
                                                                       {3, 0},
                                                                       // top face
                                                                       {4, 5},
                                                                       {5, 6},
                                                                       {6, 7},
                                                                       {7, 4},
                                                                       // vertical sides
                                                                       {0, 4},
                                                                       {1, 5},
                                                                       {2, 6},
                                                                       {3, 7}}};
    std::array<usize, 2> vertPair = {0, 0};

    // Compute the number of features which will tell use the number of vertices and edges
    size_t numVerts = numFeatures * 8;
    size_t numEdges = numFeatures * 12;

    auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->EdgeGeometryDataPath);
    edgeGeom.resizeVertexList(numVerts);
    edgeGeom.resizeEdgeList(numEdges);
    edgeGeom.getEdgeAttributeMatrix()->resizeTuples({numEdges});
    edgeGeom.getVertexAttributeMatrix()->resizeTuples({numVerts});

    DataPath edgeAmPath = m_InputValues->EdgeGeometryDataPath.createChildPath(m_InputValues->EdgeAttributeMatrixName);
    auto& edgeFeatureIds = m_DataStructure.getDataRefAs<Int32Array>(edgeAmPath.createChildPath(m_InputValues->FeatureIdsArrayName)).getDataStoreRef();
    edgeFeatureIds.fill(-1);
    size_t currentOffset = 0;
    for(size_t idx = 0; idx < numFeatures; ++idx)
    {
      std::array<float, 6> bounds = copyBoundsForFeature(m_InputValues, m_DataStructure, idx);
      // NaN values mean that there was something wrong with the bounding min/max points.
      if(std::any_of(bounds.begin(), bounds.end(), [](float v) { return std::isnan(v); }))
      {
        continue;
      }

      // Create the 8 Vertices
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 0, {bounds[0], bounds[1], bounds[2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 1, {bounds[3], bounds[1], bounds[2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 2, {bounds[3], bounds[4], bounds[2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 3, {bounds[0], bounds[4], bounds[2]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 4, {bounds[0], bounds[1], bounds[5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 5, {bounds[3], bounds[1], bounds[5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 6, {bounds[3], bounds[4], bounds[5]});
      edgeGeom.setVertexCoordinate(currentOffset * 8 + 7, {bounds[0], bounds[4], bounds[5]});

      // Create the 12 Edges
      for(size_t edgeIdx = 0; edgeIdx < cubeEdges.size(); ++edgeIdx)
      {
        vertPair[0] = currentOffset * 8 + (cubeEdges[edgeIdx].first);
        vertPair[1] = currentOffset * 8 + (cubeEdges[edgeIdx].second);

        edgeGeom.setEdgePointIds(currentOffset * 12 + edgeIdx, vertPair);
        edgeFeatureIds[currentOffset * 12 + edgeIdx] = currentOffset;
      }
      currentOffset++;
    }

    currentOffset--;
    // Update the Edge Geometry sizes
    edgeGeom.resizeVertexList(currentOffset * 8);
    edgeGeom.getVertexAttributeMatrix()->resizeTuples({currentOffset * 8});
    edgeGeom.resizeEdgeList(currentOffset * 12);
    edgeGeom.getEdgeAttributeMatrix()->resizeTuples({currentOffset * 12});
  }

  return {};
}
