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

// Each chunk holds 65,536 triangles. Connectivity uses about 1.5 MiB and areas use about 512 KiB.
constexpr usize k_ChunkTriangles = 65536;

// Bulk vertex reads use no more than 16 million vertices, or about 192 MiB.
// Larger spans use serial per-triangle reads to bound memory and avoid concurrent DataStore access.
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

  const auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(pTriangleGeometryDataPath);

  const AttributeMatrix* faceAttributeMatrix = triangleGeom->getFaceAttributeMatrix();
  if(faceAttributeMatrix == nullptr)
  {
    return {MakeErrorResult<OutputActions>(k_MissingFeatureAttributeMatrix,
                                           fmt::format("Could not find Triangle Face Attribute Matrix with in the Triangle Geometry '{}'", pTriangleGeometryDataPath.toString()))};
  }
  {
    DataPath createArrayDataPath = pTriangleGeometryDataPath.createChildPath(faceAttributeMatrix->getName()).createChildPath(pCalculatedAreasName);
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{1}, createArrayDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

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

  // Connectivity and area buffers stay bounded by the triangle chunk size.
  auto triBuf = std::make_unique<uint64[]>(k_ChunkTriangles * 3);
  auto areaBuf = std::make_unique<float64[]>(k_ChunkTriangles);

  // Vertex scratch grows once and remains bounded by k_MaxVertexSpan.
  std::vector<float32> vertBuf;

  // The algorithm does not inspect bulk-I/O Result values. A storage error can
  // leave partial area output.
  for(usize chunkStart = 0; chunkStart < numTris; chunkStart += k_ChunkTriangles)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize chunkCount = std::min<usize>(k_ChunkTriangles, numTris - chunkStart);

    triStore.copyIntoBuffer(chunkStart * 3, nonstd::span<uint64>(triBuf.get(), chunkCount * 3));

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

    // Bounded index spans preserve a bulk coordinate read. Sparse spans use the serial fallback.
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

      // Workers access only local buffers and write disjoint area positions.
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
      // Direct vertex reads stay serial because generic DataStore access has no concurrent guarantee.
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
