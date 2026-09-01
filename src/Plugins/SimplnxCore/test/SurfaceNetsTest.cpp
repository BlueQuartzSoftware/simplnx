#include "SimplnxCore/Filters/Algorithms/SurfaceNets.hpp"
#include "SimplnxCore/Filters/Algorithms/TupleTransfer.hpp"
#include "SimplnxCore/Filters/SurfaceNetsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <limits>
#include <memory>
#include <string_view>

using namespace nx::core;
using namespace nx::core::UnitTest;
using namespace nx::core::Constants;

#if SIMPLNX_TEST_ALGORITHM_PATH != 2
namespace
{
constexpr std::string_view k_SmallGeomName = "SurfaceNets Small Image";
constexpr std::string_view k_SmallCellName = "Cell Data";
constexpr std::string_view k_SmallFeatureName = "Feature Data";
const DataPath k_SmallGeomPath({std::string(k_SmallGeomName)});
const DataPath k_SmallCellPath = k_SmallGeomPath.createChildPath(std::string(k_SmallCellName));
const DataPath k_SmallFeaturePath = k_SmallGeomPath.createChildPath(std::string(k_SmallFeatureName));
const DataPath k_SmallFeatureIdsPath = k_SmallCellPath.createChildPath("FeatureIds");
const DataPath k_SmallCellIntPath = k_SmallCellPath.createChildPath("CellInt");
const DataPath k_SmallCellFloatPath = k_SmallCellPath.createChildPath("CellFloat");
const DataPath k_SmallCellBoolPath = k_SmallCellPath.createChildPath("CellBool");
const DataPath k_SmallFeatureVectorPath = k_SmallFeaturePath.createChildPath("FeatureVector");
const DataPath k_SmallOutputPath({"SurfaceNets Output"});

template <typename T>
class SurfaceNetsFailingReadStore : public DataStore<T>
{
public:
  SurfaceNetsFailingReadStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> value, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, value)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyIntoBuffer(usize, nonstd::span<T>) const override
  {
    return MakeErrorResult(m_ErrorCode, "Injected SurfaceNets bulk-read failure");
  }

private:
  int32 m_ErrorCode;
};

template <typename T>
class SurfaceNetsFailingWriteStore : public DataStore<T>
{
public:
  SurfaceNetsFailingWriteStore(const ShapeType& tupleShape, const ShapeType& componentShape, std::optional<T> value, int32 errorCode)
  : DataStore<T>(tupleShape, componentShape, value)
  , m_ErrorCode(errorCode)
  {
  }

  Result<> copyFromBuffer(usize, nonstd::span<const T>) override
  {
    return MakeErrorResult(m_ErrorCode, "Injected SurfaceNets bulk-write failure");
  }

private:
  int32 m_ErrorCode;
};

struct SurfaceNetsFixture
{
  inline static const ShapeType k_CellShape = {2, 3, 4};
  inline static const ShapeType k_FeatureShape = {4};

  static void Create(DataStructure& dataStructure, bool useConfiguredStores, bool featureIdsInMemory = false, bool selectedCellOnly = false)
  {
    auto* image = ImageGeom::Create(dataStructure, std::string(k_SmallGeomName));
    REQUIRE(image != nullptr);
    image->setDimensions(SizeVec3{4, 3, 2});
    auto* cellData = AttributeMatrix::Create(dataStructure, std::string(k_SmallCellName), k_CellShape, image->getId());
    REQUIRE(cellData != nullptr);
    image->setCellData(*cellData);
    auto* featureData = AttributeMatrix::Create(dataStructure, std::string(k_SmallFeatureName), k_FeatureShape, image->getId());
    REQUIRE(featureData != nullptr);

    std::shared_ptr<AbstractDataStore<int32>> featureIdsStore;
    if(useConfiguredStores && !featureIdsInMemory)
    {
      featureIdsStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_SmallFeatureIdsPath, k_CellShape, {1}, IDataAction::Mode::Execute);
    }
    else
    {
      featureIdsStore = std::make_shared<DataStore<int32>>(k_CellShape, ShapeType{1}, std::optional<int32>{});
    }
    REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", featureIdsStore, cellData->getId()) != nullptr);
    auto cellIntStore = useConfiguredStores ? DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_SmallCellIntPath, k_CellShape, {2}, IDataAction::Mode::Execute) :
                                              std::make_shared<DataStore<int32>>(k_CellShape, ShapeType{2}, std::optional<int32>{});
    auto cellFloatStore = useConfiguredStores && !selectedCellOnly ? DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_SmallCellFloatPath, k_CellShape, {3}, IDataAction::Mode::Execute) :
                                                                     std::make_shared<DataStore<float32>>(k_CellShape, ShapeType{3}, std::optional<float32>{});
    auto cellBoolStore = useConfiguredStores && !selectedCellOnly ? DataStoreUtilities::CreateDataStore<bool>(dataStructure, k_SmallCellBoolPath, k_CellShape, {1}, IDataAction::Mode::Execute) :
                                                                    std::make_shared<DataStore<bool>>(k_CellShape, ShapeType{1}, std::optional<bool>{});
    auto featureStore = useConfiguredStores && !selectedCellOnly ?
                            DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_SmallFeatureVectorPath, k_FeatureShape, {3}, IDataAction::Mode::Execute) :
                            std::make_shared<DataStore<float32>>(k_FeatureShape, ShapeType{3}, std::optional<float32>{});
    REQUIRE(Int32Array::Create(dataStructure, "CellInt", cellIntStore, cellData->getId()) != nullptr);
    REQUIRE(Float32Array::Create(dataStructure, "CellFloat", cellFloatStore, cellData->getId()) != nullptr);
    REQUIRE(BoolArray::Create(dataStructure, "CellBool", cellBoolStore, cellData->getId()) != nullptr);
    REQUIRE(Float32Array::Create(dataStructure, "FeatureVector", featureStore, featureData->getId()) != nullptr);

    std::array<int32, 24> featureIds{};
    std::array<int32, 48> cellInts{};
    std::array<float32, 72> cellFloats{};
    auto cellBools = std::make_unique<bool[]>(24);
    std::array<float32, 12> featureValues{};
    for(usize i = 0; i < featureIds.size(); i++)
    {
      featureIds[i] = static_cast<int32>((i % 4) == 0 ? 1 : ((i / 4) % 3) + 1);
      cellInts[2 * i] = static_cast<int32>(i);
      cellInts[2 * i + 1] = -static_cast<int32>(i);
      cellFloats[3 * i] = static_cast<float32>(i) + 0.25F;
      cellFloats[3 * i + 1] = -static_cast<float32>(i) - 0.5F;
      cellFloats[3 * i + 2] = static_cast<float32>(i % 5) * 1.25F;
      cellBools[i] = (i % 2) == 0;
    }
    for(usize i = 0; i < featureValues.size(); i++)
    {
      featureValues[i] = static_cast<float32>(i) + 0.125F;
    }
    SIMPLNX_RESULT_REQUIRE_VALID(featureIdsStore->copyFromBuffer(0, nonstd::span<const int32>(featureIds.data(), featureIds.size())));
    SIMPLNX_RESULT_REQUIRE_VALID(cellIntStore->copyFromBuffer(0, nonstd::span<const int32>(cellInts.data(), cellInts.size())));
    SIMPLNX_RESULT_REQUIRE_VALID(cellFloatStore->copyFromBuffer(0, nonstd::span<const float32>(cellFloats.data(), cellFloats.size())));
    SIMPLNX_RESULT_REQUIRE_VALID(cellBoolStore->copyFromBuffer(0, nonstd::span<const bool>(cellBools.get(), featureIds.size())));
    SIMPLNX_RESULT_REQUIRE_VALID(featureStore->copyFromBuffer(0, nonstd::span<const float32>(featureValues.data(), featureValues.size())));
  }

  static Arguments ArgumentsFor(bool smoothing, bool repairTriangleWinding = false)
  {
    Arguments args;
    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(repairTriangleWinding));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(smoothing));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0F));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5F));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(3));
    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(k_SmallGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_SmallFeatureIdsPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_SmallCellIntPath, k_SmallCellFloatPath, k_SmallCellBoolPath}));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key,
                        std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{k_SmallFeatureVectorPath}));
    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(k_SmallOutputPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));
    return args;
  }
};

void CompareSmallOutputs(const DataStructure& expected, const DataStructure& actual)
{
  const auto facePath = k_SmallOutputPath.createChildPath(k_FaceDataGroupName);
  const auto vertexPath = k_SmallOutputPath.createChildPath(k_VertexDataGroupName);
  CompareDataArrays<IGeometry::MeshIndexType>(expected.getDataRefAs<IDataArray>(k_SmallOutputPath.createChildPath(TriangleGeom::k_SharedFacesListName)),
                                              actual.getDataRefAs<IDataArray>(k_SmallOutputPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
  CompareDataArrays<float32>(expected.getDataRefAs<IDataArray>(k_SmallOutputPath.createChildPath(INodeGeometry0D::k_SharedVertexListName)),
                             actual.getDataRefAs<IDataArray>(k_SmallOutputPath.createChildPath(INodeGeometry0D::k_SharedVertexListName)));
  CompareDataArrays<int8>(expected.getDataRefAs<IDataArray>(vertexPath.createChildPath(k_NodeTypeArrayName)), actual.getDataRefAs<IDataArray>(vertexPath.createChildPath(k_NodeTypeArrayName)));
  CompareDataArrays<int32>(expected.getDataRefAs<IDataArray>(facePath.createChildPath(k_Face_Labels)), actual.getDataRefAs<IDataArray>(facePath.createChildPath(k_Face_Labels)));
  CompareDataArrays<int32>(expected.getDataRefAs<IDataArray>(facePath.createChildPath("CellInt")), actual.getDataRefAs<IDataArray>(facePath.createChildPath("CellInt")));
  CompareDataArrays<float32>(expected.getDataRefAs<IDataArray>(facePath.createChildPath("CellFloat")), actual.getDataRefAs<IDataArray>(facePath.createChildPath("CellFloat")));
  CompareDataArrays<bool>(expected.getDataRefAs<IDataArray>(facePath.createChildPath("CellBool")), actual.getDataRefAs<IDataArray>(facePath.createChildPath("CellBool")));
  CompareDataArrays<float32>(expected.getDataRefAs<IDataArray>(facePath.createChildPath("FeatureVector")), actual.getDataRefAs<IDataArray>(facePath.createChildPath("FeatureVector")));
}

SurfaceNetsInputValues CreateInputValues(const Arguments& args)
{
  SurfaceNetsInputValues values;
  values.ApplySmoothing = args.value<bool>(SurfaceNetsFilter::k_ApplySmoothing_Key);
  values.RepairTriangleWinding = args.value<bool>(SurfaceNetsFilter::k_RepairTriangleWinding_Key);
  values.SmoothingIterations = args.value<int32>(SurfaceNetsFilter::k_SmoothingIterations_Key);
  values.MaxDistanceFromVoxel = args.value<float32>(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key);
  values.RelaxationFactor = args.value<float32>(SurfaceNetsFilter::k_RelaxationFactor_Key);
  values.GridGeomDataPath = args.value<DataPath>(SurfaceNetsFilter::k_GridGeometryDataPath_Key);
  values.FeatureIdsArrayPath = args.value<DataPath>(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key);
  values.SelectedCellDataArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key);
  values.SelectedFeatureDataArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key);
  values.TriangleGeometryPath = args.value<DataPath>(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key);
  values.VertexGroupDataPath = values.TriangleGeometryPath.createChildPath(args.value<std::string>(SurfaceNetsFilter::k_VertexDataGroupName_Key));
  values.NodeTypesDataPath = values.VertexGroupDataPath.createChildPath(args.value<std::string>(SurfaceNetsFilter::k_NodeTypesArrayName_Key));
  values.FaceGroupDataPath = values.TriangleGeometryPath.createChildPath(args.value<std::string>(SurfaceNetsFilter::k_FaceDataGroupName_Key));
  values.FaceLabelsDataPath = values.FaceGroupDataPath.createChildPath(args.value<std::string>(SurfaceNetsFilter::k_FaceLabelsArrayName_Key));
  for(const auto& path : values.SelectedCellDataArrayPaths)
  {
    values.CreatedDataArrayPaths.push_back(values.FaceGroupDataPath.createChildPath(path.getTargetName()));
  }
  for(const auto& path : values.SelectedFeatureDataArrayPaths)
  {
    values.CreatedDataArrayPaths.push_back(values.FaceGroupDataPath.createChildPath(path.getTargetName()));
  }
  return values;
}

void CreateOutputs(DataStructure& dataStructure, const Arguments& args)
{
  SurfaceNetsFilter filter;
  auto preflight = filter.preflight(dataStructure, args);
  if(preflight.outputActions.invalid())
  {
    for(const auto& error : preflight.outputActions.errors())
    {
      INFO("Preflight error code: " << error.code);
    }
  }
  REQUIRE(preflight.outputActions.valid());
  auto applyResult = preflight.outputActions.value().applyAll(dataStructure, IDataAction::Mode::Execute);
  if(applyResult.invalid())
  {
    for(const auto& error : applyResult.errors())
    {
      INFO("Output-action error code: " << error.code);
    }
  }
  REQUIRE(applyResult.valid());
}

void RequireSmallOutputStores(const DataStructure& dataStructure, const bool expectedOutOfCore)
{
  const auto faceDataPath = k_SmallOutputPath.createChildPath(k_FaceDataGroupName);
  const auto vertexDataPath = k_SmallOutputPath.createChildPath(k_VertexDataGroupName);
  const std::array<DataPath, 8> paths = {
      k_SmallOutputPath.createChildPath(INodeGeometry0D::k_SharedVertexListName),
      k_SmallOutputPath.createChildPath(TriangleGeom::k_SharedFacesListName),
      vertexDataPath.createChildPath(k_NodeTypeArrayName),
      faceDataPath.createChildPath(k_Face_Labels),
      faceDataPath.createChildPath("CellInt"),
      faceDataPath.createChildPath("CellFloat"),
      faceDataPath.createChildPath("CellBool"),
      faceDataPath.createChildPath("FeatureVector"),
  };
  for(const auto& path : paths)
  {
    REQUIRE(IsOutOfCore(dataStructure.getDataRefAs<IDataArray>(path)) == expectedOutOfCore);
  }
}

void RequireLargeTransferValue(AbstractDataStore<int32>& store, const usize offset, const int32 expected)
{
  int32 value = 0;
  SIMPLNX_RESULT_REQUIRE_VALID(store.copyIntoBuffer(offset, nonstd::span<int32>(&value, 1)));
  REQUIRE(value == expected);
}
} // namespace

TEST_CASE("SimplnxCore::SurfaceNetsFilter: generated direct and scanline parity", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();
  for(const bool smoothing : {false, true})
  {
    CAPTURE(smoothing);
    DataStructure directData;
    {
      AlgorithmTestScope directScope(AlgorithmTestScenario::InCoreAlgorithmOnInMemoryStore);
      SurfaceNetsFixture::Create(directData, false);
      SurfaceNetsFilter filter;
      auto result = directScope.executeFilter(filter, directData, SurfaceNetsFixture::ArgumentsFor(smoothing));
      SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    }
    DataStructure scanlineData;
    {
      AlgorithmTestScope scanlineScope(AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
      SurfaceNetsFixture::Create(scanlineData, false);
      SurfaceNetsFilter filter;
      auto result = scanlineScope.executeFilter(filter, scanlineData, SurfaceNetsFixture::ArgumentsFor(smoothing));
      SIMPLNX_RESULT_REQUIRE_VALID(result.result);
    }
    CompareSmallOutputs(directData, scanlineData);
  }
}

TEST_CASE("SimplnxCore::SurfaceNets: scanline bulk failures propagate", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();
  constexpr int32 k_ReadError = -73101;
  constexpr int32 k_WriteError = -73102;

  SECTION("FeatureIds classification read")
  {
    AlgorithmTestScope scope(AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    DataStructure dataStructure;
    SurfaceNetsFixture::Create(dataStructure, false);
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(k_SmallCellPath);
    dataStructure.removeData(k_SmallFeatureIdsPath);
    auto store = std::make_shared<SurfaceNetsFailingReadStore<int32>>(SurfaceNetsFixture::k_CellShape, ShapeType{1}, int32{1}, k_ReadError);
    REQUIRE(Int32Array::Create(dataStructure, "FeatureIds", store, cellData.getId()) != nullptr);
    const Arguments args = SurfaceNetsFixture::ArgumentsFor(false);
    CreateOutputs(dataStructure, args);
    auto values = CreateInputValues(args);
    std::atomic_bool shouldCancel = false;
    auto result = scope.execute([&] { return SurfaceNets(dataStructure, {}, shouldCancel, &values)(); });
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == k_ReadError);
  }

  SECTION("selected cell source read")
  {
    AlgorithmTestScope scope(AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    DataStructure dataStructure;
    SurfaceNetsFixture::Create(dataStructure, false);
    const auto& cellData = dataStructure.getDataRefAs<AttributeMatrix>(k_SmallCellPath);
    dataStructure.removeData(k_SmallCellIntPath);
    auto store = std::make_shared<SurfaceNetsFailingReadStore<int32>>(SurfaceNetsFixture::k_CellShape, ShapeType{2}, int32{1}, k_ReadError);
    REQUIRE(Int32Array::Create(dataStructure, "CellInt", store, cellData.getId()) != nullptr);
    const Arguments args = SurfaceNetsFixture::ArgumentsFor(false);
    CreateOutputs(dataStructure, args);
    auto values = CreateInputValues(args);
    std::atomic_bool shouldCancel = false;
    auto result = scope.execute([&] { return SurfaceNets(dataStructure, {}, shouldCancel, &values)(); });
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == k_ReadError);
  }

  SECTION("created transfer output write")
  {
    AlgorithmTestScope scope(AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
    DataStructure dataStructure;
    SurfaceNetsFixture::Create(dataStructure, false);
    const Arguments args = SurfaceNetsFixture::ArgumentsFor(false);
    CreateOutputs(dataStructure, args);
    const auto outputPath = k_SmallOutputPath.createChildPath(k_FaceDataGroupName).createChildPath("CellInt");
    const auto& faceData = dataStructure.getDataRefAs<AttributeMatrix>(k_SmallOutputPath.createChildPath(k_FaceDataGroupName));
    dataStructure.removeData(outputPath);
    auto store = std::make_shared<SurfaceNetsFailingWriteStore<int32>>(ShapeType{0}, ShapeType{2, 2}, int32{0}, k_WriteError);
    REQUIRE(Int32Array::Create(dataStructure, "CellInt", store, faceData.getId()) != nullptr);
    auto values = CreateInputValues(args);
    std::atomic_bool shouldCancel = false;
    auto result = scope.execute([&] { return SurfaceNets(dataStructure, {}, shouldCancel, &values)(); });
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == k_WriteError);
  }
}

TEST_CASE("SimplnxCore::SurfaceNets: pre-cancelled scanline does not write", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();
  AlgorithmTestScope scope(AlgorithmTestScenario::OutOfCoreAlgorithmOnInMemoryStore);
  DataStructure dataStructure;
  SurfaceNetsFixture::Create(dataStructure, false);
  const Arguments args = SurfaceNetsFixture::ArgumentsFor(false);
  CreateOutputs(dataStructure, args);
  auto values = CreateInputValues(args);
  std::atomic_bool shouldCancel = true;
  auto result = scope.execute([&] { return SurfaceNets(dataStructure, {}, shouldCancel, &values)(); });
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto& geometry = dataStructure.getDataRefAs<TriangleGeom>(k_SmallOutputPath);
  REQUIRE(geometry.getVertices()->getNumberOfTuples() == 1);
  REQUIRE(geometry.getFaces()->getNumberOfTuples() == 0);
}

TEST_CASE("SimplnxCore::TupleTransfer: large-component direct fallback", "[SimplnxCore][SurfaceNetsFilter][OOC]")
{
  constexpr usize kComponents = detail::kTransferPageValues + 1;
  constexpr usize kValuesPerFace = kComponents * 2;
  DataStructure dataStructure;
  const DataPath sourcePath({"Direct Source"});
  const DataPath destinationPath({"Direct Destination"});
  auto* source = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, sourcePath.getTargetName(), ShapeType{3}, ShapeType{kComponents});
  auto* destination = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, destinationPath.getTargetName(), ShapeType{2}, ShapeType{2, kComponents});
  REQUIRE(source != nullptr);
  REQUIRE(destination != nullptr);

  auto sourceValues = std::make_unique<int32[]>(3 * kComponents);
  for(usize tuple = 0; tuple < 3; tuple++)
  {
    for(usize component = 0; component < kComponents; component++)
    {
      sourceValues[tuple * kComponents + component] = static_cast<int32>(tuple * 1000000 + component);
    }
  }
  SIMPLNX_RESULT_REQUIRE_VALID(source->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(sourceValues.get(), 3 * kComponents)));

  const std::array<SurfaceNetsTransferData, 2> records = {
      SurfaceNetsTransferData{0, {0, 1}},
      SurfaceNetsTransferData{1, {2, std::numeric_limits<usize>::max()}},
  };
  TransferTuple<int32> transfer(dataStructure, sourcePath, destinationPath);
  SIMPLNX_RESULT_REQUIRE_VALID(transfer.surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData>(records.data(), records.size())));

  for(const usize component : {usize{0}, detail::kTransferPageValues - 1, detail::kTransferPageValues})
  {
    RequireLargeTransferValue(destination->getDataStoreRef(), component, static_cast<int32>(component));
    RequireLargeTransferValue(destination->getDataStoreRef(), kComponents + component, static_cast<int32>(1000000 + component));
    RequireLargeTransferValue(destination->getDataStoreRef(), kValuesPerFace + component, static_cast<int32>(2000000 + component));
    RequireLargeTransferValue(destination->getDataStoreRef(), kValuesPerFace + kComponents + component, 0);
  }
}

TEST_CASE("SimplnxCore::TupleTransfer: large-component feature fallback", "[SimplnxCore][SurfaceNetsFilter][OOC]")
{
  constexpr usize kComponents = detail::kTransferPageValues + 1;
  constexpr usize kValuesPerFace = kComponents * 2;
  DataStructure dataStructure;
  const DataPath featureIdsPath({"Feature Ids"});
  const DataPath sourcePath({"Feature Source"});
  const DataPath destinationPath({"Feature Destination"});
  auto* featureIds = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, featureIdsPath.getTargetName(), ShapeType{3}, ShapeType{1});
  auto* source = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, sourcePath.getTargetName(), ShapeType{3}, ShapeType{kComponents});
  auto* destination = Int32Array::CreateWithStore<DataStore<int32>>(dataStructure, destinationPath.getTargetName(), ShapeType{2}, ShapeType{2, kComponents});
  REQUIRE(featureIds != nullptr);
  REQUIRE(source != nullptr);
  REQUIRE(destination != nullptr);

  const std::array<int32, 3> featureIdsValues = {2, 0, 1};
  SIMPLNX_RESULT_REQUIRE_VALID(featureIds->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(featureIdsValues.data(), featureIdsValues.size())));
  auto sourceValues = std::make_unique<int32[]>(3 * kComponents);
  for(usize feature = 0; feature < 3; feature++)
  {
    for(usize component = 0; component < kComponents; component++)
    {
      sourceValues[feature * kComponents + component] = static_cast<int32>(feature * 1000000 + component);
    }
  }
  SIMPLNX_RESULT_REQUIRE_VALID(source->getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(sourceValues.get(), 3 * kComponents)));

  const std::array<SurfaceNetsTransferData, 2> records = {
      SurfaceNetsTransferData{0, {0, 1}},
      SurfaceNetsTransferData{1, {2, std::numeric_limits<usize>::max()}},
  };
  TransferFeatureTuple<int32, int32> transfer(dataStructure, sourcePath, destinationPath, featureIdsPath);
  SIMPLNX_RESULT_REQUIRE_VALID(transfer.surfaceNetsTransferBatch(nonstd::span<const SurfaceNetsTransferData>(records.data(), records.size())));

  for(const usize component : {usize{0}, detail::kTransferPageValues - 1, detail::kTransferPageValues})
  {
    RequireLargeTransferValue(destination->getDataStoreRef(), component, static_cast<int32>(2000000 + component));
    RequireLargeTransferValue(destination->getDataStoreRef(), kComponents + component, static_cast<int32>(component));
    RequireLargeTransferValue(destination->getDataStoreRef(), kValuesPerFace + component, static_cast<int32>(1000000 + component));
    RequireLargeTransferValue(destination->getDataStoreRef(), kValuesPerFace + kComponents + component, 0);
  }
}
#endif

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Default", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v4.tar.gz", "SurfaceNetsTest_v4");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v4/SurfaceNetsTest_v4.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath celDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(featureIdsDataPath));

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed SurfaceNets"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar SurfaceNets"});
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(celDataPath));
    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(celDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      // fmt::print("Adding Cell Array: {}\n", child.second->getName());
      selectedCellArrayPaths.push_back(celDataPath.createChildPath(child.second->getName()));
    }

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath));
    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      // fmt::print("Adding Feature Array: {}\n", child.second->getName());
      selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child.second->getName()));
    }

    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));

    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName)));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(faceLabelsDataPath));

    // The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/SurfaceNetsFilterTest_default.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Verify the generated data.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath));
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex and triangle lists.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath));
  auto& exemplarDataArray = dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
  auto& computedDataArray = dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<IGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Smoothing", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v4.tar.gz", "SurfaceNetsTest_v4");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v4/SurfaceNetsTest_v4.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath celDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed SurfaceNets"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar SurfaceNets Smoothing"});
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(featureIdsDataPath));
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(celDataPath));
    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(celDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(celDataPath.createChildPath(child.second->getName()));
    }

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath));
    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child.second->getName()));
    }

    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));

    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName)));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(faceLabelsDataPath));

    // The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/SurfaceNetsFilterTest_Smoothing.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Verify the generated data.
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath));
    TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
    IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
    IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();
    REQUIRE(triangle->getNumberOfTuples() == 668786);
    REQUIRE(vertices->getNumberOfTuples() == 319447);
  }

  // Compare the shared vertex and triangle lists.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath));
  auto& exemplarDataArray = dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
  auto& computedDataArray = dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<IGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Winding", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v4.tar.gz", "SurfaceNetsTest_v4");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v4/SurfaceNetsTest_v4.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath celDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed SurfaceNets"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar SurfaceNets Winding"});
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(featureIdsDataPath));
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(celDataPath));
    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(celDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(celDataPath.createChildPath(child.second->getName()));
    }

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath));
    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child.second->getName()));
    }

    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(false));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));

    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName)));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(faceLabelsDataPath));

    // The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/SurfaceNetsFilterTest_winding.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Verify the generated data.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath));
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex and triangle lists.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath));
  auto& exemplarDataArray = dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
  auto& computedDataArray = dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<IGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::SurfaceNetsFilter: Winding Smoothing", "[SimplnxCore][SurfaceNetsFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "SurfaceNetsTest_v4.tar.gz", "SurfaceNetsTest_v4");

  // Load the Small IN100 input.
  auto baseDataFilePath = fs::path(fmt::format("{}/SurfaceNetsTest_v4/SurfaceNetsTest_v4.dream3d", nx::core::unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_DataContainer});
  DataPath gridGeomDataPath({k_DataContainer});
  DataPath featureIdsDataPath({k_DataContainer, k_CellData, k_FeatureIds});
  DataPath celDataPath({k_DataContainer, k_CellData});
  DataPath featureDataPath({k_DataContainer, k_CellFeatureData});

  // DataPath triangleParentGroup({k_DataContainer});
  DataPath computedTriangleGeomPath({"Computed SurfaceNets"});
  DataPath vertexGroupDataPath = computedTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  DataPath nodeTypeDataPath = vertexGroupDataPath.createChildPath(k_NodeTypeArrayName);
  DataPath faceGroupDataPath = computedTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  DataPath faceLabelsDataPath = faceGroupDataPath.createChildPath(k_Face_Labels);

  DataPath exemplarTriangleGeomPath({"Exemplar SurfaceNets Winding Smoothing"});
  scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(featureIdsDataPath));
  DataPath exemplarSharedTriPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry2D::k_SharedFacesListName);
  DataPath exemplarSharedVertexPath = exemplarTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName);

  {
    Arguments args;
    SurfaceNetsFilter const filter;

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(celDataPath));
    auto voxelCellAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(celDataPath);
    MultiArraySelectionParameter::ValueType selectedCellArrayPaths;
    for(const auto& child : voxelCellAttrMat)
    {
      selectedCellArrayPaths.push_back(celDataPath.createChildPath(child.second->getName()));
    }

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath));
    auto voxelFeatureAttrMat = dataStructure.getDataRefAs<AttributeMatrix>(featureDataPath);
    MultiArraySelectionParameter::ValueType selectedFeatureArrayPaths;
    for(const auto& child : voxelFeatureAttrMat)
    {
      selectedFeatureArrayPaths.push_back(featureDataPath.createChildPath(child.second->getName()));
    }

    args.insertOrAssign(SurfaceNetsFilter::k_RepairTriangleWinding_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_ApplySmoothing_Key, std::make_any<bool>(true));
    args.insertOrAssign(SurfaceNetsFilter::k_MaxDistanceFromVoxelCenter_Key, std::make_any<float32>(1.0f));
    args.insertOrAssign(SurfaceNetsFilter::k_RelaxationFactor_Key, std::make_any<float32>(0.5f));
    args.insertOrAssign(SurfaceNetsFilter::k_SmoothingIterations_Key, std::make_any<int32>(20));

    args.insertOrAssign(SurfaceNetsFilter::k_GridGeometryDataPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsDataPath));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedCellArrayPaths));
    args.insertOrAssign(SurfaceNetsFilter::k_SelectedFeatureDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(selectedFeatureArrayPaths));

    args.insertOrAssign(SurfaceNetsFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(computedTriangleGeomPath));
    args.insertOrAssign(SurfaceNetsFilter::k_VertexDataGroupName_Key, std::make_any<std::string>(k_VertexDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_NodeTypesArrayName_Key, std::make_any<std::string>(k_NodeTypeArrayName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceDataGroupName_Key, std::make_any<std::string>(k_FaceDataGroupName));
    args.insertOrAssign(SurfaceNetsFilter::k_FaceLabelsArrayName_Key, std::make_any<std::string>(k_Face_Labels));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(INodeGeometry0D::k_SharedVertexListName)));
    scope.requireExpectedStore(dataStructure.getDataRefAs<IDataArray>(faceLabelsDataPath));

    // The optional output supports manual inspection.
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
    WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/SurfaceNetsFilterTest_winding_smoothing.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif
  }
  // Verify the generated data.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath));
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(computedTriangleGeomPath);
  IGeometry::SharedTriList* triangle = triangleGeom.getFaces();
  IGeometry::SharedVertexList* vertices = triangleGeom.getVertices();

  REQUIRE(triangle->getNumberOfTuples() == 668786);
  REQUIRE(vertices->getNumberOfTuples() == 319447);

  // Compare the shared vertex and triangle lists.
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath));
  auto& exemplarDataArray = dataStructure.getDataRefAs<IDataArray>(exemplarSharedTriPath);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName)));
  auto& computedDataArray = dataStructure.getDataRefAs<IDataArray>(computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedFacesListName));
  CompareDataArrays<IGeometry::MeshIndexType>(exemplarDataArray, computedDataArray);
  CompareArrays<float32>(dataStructure, exemplarSharedVertexPath, computedTriangleGeomPath.createChildPath(TriangleGeom::k_SharedVertexListName));

  DataPath exemplarFaceAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_FaceDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarFaceAttrMatPath, dataStructure, faceGroupDataPath, true);

  DataPath exemplarVertexAttrMatPath = exemplarTriangleGeomPath.createChildPath(k_VertexDataGroupName);
  CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarVertexAttrMatPath, dataStructure, vertexGroupDataPath, true);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
