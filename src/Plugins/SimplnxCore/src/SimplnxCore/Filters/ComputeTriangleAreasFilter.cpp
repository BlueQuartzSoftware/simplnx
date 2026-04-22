#include "ComputeTriangleAreasFilter.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <array>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
constexpr nx::core::int32 k_MissingFeatureAttributeMatrix = -75769;

/// Number of triangles processed per bulk I/O chunk. Keeps the connectivity buffer
/// (3 uint64 per triangle) near 1.5 MB and the area output buffer near 512 KB per chunk.
constexpr usize k_ChunkTriangles = 65536;

/// Maximum vertex index span (in vertices) we are willing to bulk-load per triangle chunk.
/// 16M vertices * 12 bytes (x,y,z float32) = ~192 MB upper bound. Filter-generated meshes
/// cluster their vertex references spatially and will stay well under this cap; meshes
/// that exceed it fall back to a safe per-triangle vertex read path.
constexpr uint64 k_MaxVertexSpan = 16ULL * 1024 * 1024;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeTriangleAreasFilter::name() const
{
  return FilterTraits<ComputeTriangleAreasFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeTriangleAreasFilter::className() const
{
  return FilterTraits<ComputeTriangleAreasFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeTriangleAreasFilter::uuid() const
{
  return FilterTraits<ComputeTriangleAreasFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeTriangleAreasFilter::humanName() const
{
  return "Compute Triangle Areas";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeTriangleAreasFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Misc", "Triangle Geometry"};
}

//------------------------------------------------------------------------------
Parameters ComputeTriangleAreasFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryDataPath_Key, "Triangle Geometry", "The complete path to the Geometry for which to calculate the face areas", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));
  params.insertSeparator(Parameters::Separator{"Output Face Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CalculatedAreasDataName_Key, "Created Face Areas", "The complete path to the array storing the calculated face areas", "Face Areas"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeTriangleAreasFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeTriangleAreasFilter::clone() const
{
  return std::make_unique<ComputeTriangleAreasFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeTriangleAreasFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);
  auto pCalculatedAreasName = filterArgs.value<std::string>(k_CalculatedAreasDataName_Key);

  std::vector<PreflightValue> preflightUpdatedValues;

  nx::core::Result<OutputActions> resultOutputActions;

  // The parameter will have validated that the Triangle Geometry exists and is the correct type
  const auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);

  // Get the Face AttributeMatrix from the Geometry (It should have been set at construction of the Triangle Geometry)
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();
  if(faceAttributeMatrix == nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_MissingFeatureAttributeMatrix,
                                           fmt::format("Could not find Triangle Face Attribute Matrix with in the Triangle Geometry '{}'", pTriangleGeometryDataPath.toString()))};
  }
  // Instantiate and move the action that will create the output array
  {
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pCalculatedAreasName);
    // Create the face areas DataArray Action and store it into the resultOutputActions
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{1}, createArrayDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeTriangleAreasFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pCalculatedAreasName = filterArgs.value<std::string>(k_CalculatedAreasDataName_Key);
  auto pTriangleGeometryDataPath = filterArgs.value<DataPath>(k_TriangleGeometryDataPath_Key);

  const TriangleGeom* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);
  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();

  DataPath pCalculatedAreasDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pCalculatedAreasName);
  auto& areaStore = dataStructure.getDataAs<Float64Array>(pCalculatedAreasDataPath)->getDataStoreRef();

  const auto& triStore = triangleGeom->getFaces()->getDataStoreRef();
  const auto& vertStore = triangleGeom->getVertices()->getDataStoreRef();
  const usize numTris = triangleGeom->getNumberOfFaces();

  // Per-chunk scratch. Both buffers are bounded by k_ChunkTriangles regardless of mesh size,
  // so total peak RAM outside vertBuf is O(chunk), not O(n).
  auto triBuf = std::make_unique<uint64[]>(k_ChunkTriangles * 3);
  auto areaBuf = std::make_unique<float64[]>(k_ChunkTriangles);

  // Vertex-coordinate scratch. Grown lazily to fit each chunk's vertex index span; bounded
  // by k_MaxVertexSpan * 3 floats (~192 MB). Allocated here so the allocation is amortized
  // across chunks rather than re-done every iteration.
  std::vector<float32> vertBuf;

  for(usize chunkStart = 0; chunkStart < numTris; chunkStart += k_ChunkTriangles)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize chunkCount = std::min<usize>(k_ChunkTriangles, numTris - chunkStart);

    // 1. Bulk-read triangle connectivity (3 vertex indices per triangle).
    triStore.copyIntoBuffer(chunkStart * 3, nonstd::span<uint64>(triBuf.get(), chunkCount * 3));

    // 2. Determine the vertex-index span referenced by this triangle chunk.
    uint64 minVertIdx = std::numeric_limits<uint64>::max();
    uint64 maxVertIdx = 0;
    for(usize i = 0; i < chunkCount * 3; i++)
    {
      const uint64 v = triBuf[i];
      if(v < minVertIdx)
      {
        minVertIdx = v;
      }
      if(v > maxVertIdx)
      {
        maxVertIdx = v;
      }
    }

    // 3. Bulk-load vertex coordinates for the chunk's index range when the span is bounded.
    //    When the span exceeds k_MaxVertexSpan the mesh's vertex indexing has very poor locality
    //    (rare for filter-generated meshes) — fall through to a safe per-triangle read path.
    const uint64 vertSpan = maxVertIdx - minVertIdx + 1;
    const bool useBulkVerts = (vertSpan <= k_MaxVertexSpan);
    if(useBulkVerts)
    {
      const usize needFloats = static_cast<usize>(vertSpan) * 3;
      if(vertBuf.size() < needFloats)
      {
        vertBuf.resize(needFloats);
      }
      vertStore.copyIntoBuffer(minVertIdx * 3, nonstd::span<float32>(vertBuf.data(), needFloats));

      // 4a. Parallel area compute. Threads read shared plain-array buffers and write disjoint
      //     areaBuf positions — no DataStore access in the parallel region, so this is safe.
      const uint64* triBufPtr = triBuf.get();
      const float32* vertBufPtr = vertBuf.data();
      float64* areaBufPtr = areaBuf.get();
      const uint64 baseVertIdx = minVertIdx;

      ParallelDataAlgorithm dataAlg;
      dataAlg.setRange(0, static_cast<size_t>(chunkCount));
      dataAlg.execute([&](const Range& range) {
        for(size_t i = range.min(); i < range.max(); i++)
        {
          const usize o0 = static_cast<usize>(triBufPtr[i * 3 + 0] - baseVertIdx) * 3;
          const usize o1 = static_cast<usize>(triBufPtr[i * 3 + 1] - baseVertIdx) * 3;
          const usize o2 = static_cast<usize>(triBufPtr[i * 3 + 2] - baseVertIdx) * 3;

          const Point3Df p0{vertBufPtr[o0], vertBufPtr[o0 + 1], vertBufPtr[o0 + 2]};
          const Point3Df p1{vertBufPtr[o1], vertBufPtr[o1 + 1], vertBufPtr[o1 + 2]};
          const Point3Df p2{vertBufPtr[o2], vertBufPtr[o2 + 1], vertBufPtr[o2 + 2]};

          const Point3Df vecA = (p0 - p1).toArray();
          const Point3Df vecB = (p0 - p2).toArray();
          const Point3Df cross = vecA.cross(vecB);
          areaBufPtr[i] = 0.5F * cross.magnitude();
        }
      });
    }
    else
    {
      // 4b. Fallback: per-triangle vertex reads via copyIntoBuffer. These hit the DataStore
      //     directly, which isn't thread-safe for concurrent access, so run serially.
      std::array<float32, 3> v0Buf{};
      std::array<float32, 3> v1Buf{};
      std::array<float32, 3> v2Buf{};
      for(usize i = 0; i < chunkCount; i++)
      {
        if(shouldCancel)
        {
          return {};
        }
        const uint64 v0 = triBuf[i * 3 + 0];
        const uint64 v1 = triBuf[i * 3 + 1];
        const uint64 v2 = triBuf[i * 3 + 2];
        vertStore.copyIntoBuffer(v0 * 3, nonstd::span<float32>(v0Buf.data(), 3));
        vertStore.copyIntoBuffer(v1 * 3, nonstd::span<float32>(v1Buf.data(), 3));
        vertStore.copyIntoBuffer(v2 * 3, nonstd::span<float32>(v2Buf.data(), 3));
        const Point3Df p0{v0Buf[0], v0Buf[1], v0Buf[2]};
        const Point3Df p1{v1Buf[0], v1Buf[1], v1Buf[2]};
        const Point3Df p2{v2Buf[0], v2Buf[1], v2Buf[2]};
        const Point3Df vecA = (p0 - p1).toArray();
        const Point3Df vecB = (p0 - p2).toArray();
        const Point3Df cross = vecA.cross(vecB);
        areaBuf[i] = 0.5F * cross.magnitude();
      }
    }

    // 5. Bulk-write the chunk's areas.
    areaStore.copyFromBuffer(chunkStart, nonstd::span<const float64>(areaBuf.get(), chunkCount));
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceMeshTriangleAreasArrayPathKey = "SurfaceMeshTriangleAreasArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeTriangleAreasFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeTriangleAreasFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshTriangleAreasArrayPathKey,
                                                                                                                                      k_TriangleGeometryDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshTriangleAreasArrayPathKey,
                                                                                                                                  k_CalculatedAreasDataName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
