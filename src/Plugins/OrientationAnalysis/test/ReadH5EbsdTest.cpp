#include "OrientationAnalysis/Filters/ReadH5EbsdFilter.hpp"
#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"
#include "OrientationAnalysis/Parameters/ReadH5EbsdFileParameter.h"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <EbsdLib/IO/TSL/AngConstants.h>

#include <catch2/catch.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace nx::core;
using namespace nx::core::UnitTest;
namespace fs = std::filesystem;
namespace
{
const std::string k_MaterialName("MaterialName");

// 189 x 201 points at 0.25 spacing span [0,47.25] x [0,50.25].
// A 180-degree Y rotation negates X and Z: minima are -47.25 and -sliceCount * 0.25.
constexpr usize k_XPoints = 189;
constexpr usize k_YPoints = 201;
constexpr int32 k_FirstSlice = 1;
constexpr int32 k_LastSlice = 2;
constexpr float32 k_Resolution = 0.25F;
constexpr float32 k_XMin = -47.25F;
constexpr float32 k_YMax = 50.25F;
constexpr float32 k_Zero = 0.0F;
constexpr float32 k_OriginTolerance = 1.0e-5F;
constexpr float32 k_EulerRotationDegrees = 90.0F;
constexpr float32 k_SampleRotationDegrees = 180.0F;
constexpr ChoicesParameter::ValueType k_AxisAngleRepresentation = 0;

// RotateSampleRefFrame lives in SimplnxCore, so the test addresses it through the filter list
// with the same handle and argument keys that ReadH5Ebsd uses.
constexpr StringLiteral k_SimplnxCorePluginId = "05cc618b-781f-4ac0-b9ac-43f26ce1854f";
constexpr StringLiteral k_RotateSampleRefFrameFilterId = "d2451dc1-a5a1-4ac2-a64d-7991669dcffc";
constexpr StringLiteral k_RotateSelectedImageGeometryPath_Key = "input_image_geometry_path";
constexpr StringLiteral k_RotateCreatedImageGeometryPath_Key = "output_image_geometry_path";
constexpr StringLiteral k_RotateRotationAxisAngle_Key = "rotation_axis_angle";
constexpr StringLiteral k_RotateRotationRepresentation_Key = "rotation_representation_index";
constexpr StringLiteral k_RotateSliceBySlice_Key = "rotate_slice_by_slice";
constexpr StringLiteral k_RotateRemoveOriginalGeometry_Key = "remove_original_geometry";
constexpr StringLiteral k_RotateKeepInputGeometryOrigin_Key = "keep_input_geometry_origin";

template <typename T>
void CompareNumericValues(const IDataArray& generated, const IDataArray& expected, bool bitExact)
{
  CompareDataArrays<T>(generated, expected);
  if(bitExact)
  {
    const auto& generatedStore = generated.getIDataStoreRefAs<AbstractDataStore<T>>();
    const auto& expectedStore = expected.getIDataStoreRefAs<AbstractDataStore<T>>();
    const usize size = generatedStore.getSize();
    auto generatedValues = std::make_unique<T[]>(size);
    auto expectedValues = std::make_unique<T[]>(size);
    const auto generatedCopy = generatedStore.copyIntoBuffer(0, nonstd::span<T>(generatedValues.get(), size));
    SIMPLNX_RESULT_REQUIRE_VALID(generatedCopy);
    const auto expectedCopy = expectedStore.copyIntoBuffer(0, nonstd::span<T>(expectedValues.get(), size));
    SIMPLNX_RESULT_REQUIRE_VALID(expectedCopy);
    REQUIRE(std::memcmp(generatedValues.get(), expectedValues.get(), size * sizeof(T)) == 0);
  }
}

void CompareNumericArrays(const IDataArray& generatedDataArray, const IDataArray& exemplarDataArray, bool bitExact = false)
{
  REQUIRE(generatedDataArray.getDataType() == exemplarDataArray.getDataType());
  if(bitExact)
  {
    REQUIRE(generatedDataArray.getTupleShape() == exemplarDataArray.getTupleShape());
    REQUIRE(generatedDataArray.getComponentShape() == exemplarDataArray.getComponentShape());
  }
  switch(generatedDataArray.getDataType())
  {
  case DataType::boolean: {
    CompareNumericValues<bool>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::int8: {
    CompareNumericValues<int8>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::int16: {
    CompareNumericValues<int16>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::int32: {
    CompareNumericValues<int32>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::int64: {
    CompareNumericValues<int64>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::uint8: {
    CompareNumericValues<uint8>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::uint16: {
    CompareNumericValues<uint16>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::uint32: {
    CompareNumericValues<uint32>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::uint64: {
    CompareNumericValues<uint64>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::float32: {
    CompareNumericValues<float32>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  case DataType::float64: {
    CompareNumericValues<float64>(generatedDataArray, exemplarDataArray, bitExact);
    break;
  }
  default: {
    throw std::runtime_error("Invalid DataType");
  }
  }
}

Arguments SmallArchiveArguments(int32 endSlice, bool useRecommendedTransform)
{
  ReadH5EbsdFileParameter::ValueType input;
  input.inputFilePath = fmt::format("{}/6_5_h5ebsd_exemplar/6_5_h5ebsd_exemplar.h5ebsd", unit_test::k_TestFilesDir);
  input.startSlice = k_FirstSlice;
  input.endSlice = endSlice;
  input.eulerRepresentation = ebsdlib::AngleRepresentation::Radians;
  input.selectedArrayNames = {Constants::k_ConfidenceIndex, Constants::k_EulerAngles, Constants::k_Fit,        Constants::k_ImageQuality,
                              Constants::k_Phases,          Constants::k_SEMSignal,   ebsdlib::Ang::XPosition, ebsdlib::Ang::YPosition};
  input.useRecommendedTransform = useRecommendedTransform;
  Arguments args;
  args.insertOrAssign(ReadH5EbsdFilter::k_ReadH5EbsdParameter_Key, std::make_any<ReadH5EbsdFileParameter::ValueType>(input));
  args.insertOrAssign(ReadH5EbsdFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(ReadH5EbsdFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_CellData));
  args.insertOrAssign(ReadH5EbsdFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_EnsembleAttributeMatrix));
  return args;
}

const ImageGeom& GetImageGeometry(const DataStructure& dataStructure)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(Constants::k_DataContainerPath));
  return dataStructure.getDataRefAs<ImageGeom>(Constants::k_DataContainerPath);
}

void CheckRotatedGeometry(const ImageGeom& geometry, usize sliceCount)
{
  const SizeVec3 expectedDims = {k_XPoints, k_YPoints, sliceCount};
  const FloatVec3 expectedSpacing = {k_Resolution, k_Resolution, k_Resolution};
  const FloatVec3 expectedMin = {k_XMin, k_Zero, -static_cast<float32>(sliceCount) * k_Resolution};
  const FloatVec3 expectedMax = {k_Zero, k_YMax, k_Zero};
  CHECK(geometry.getDimensions() == expectedDims);
  CHECK(geometry.getSpacing() == expectedSpacing);
  const auto origin = geometry.getOrigin();
  const auto bounds = geometry.getBoundingBoxf();
  INFO(fmt::format("Observed origin: [{:.9g}, {:.9g}, {:.9g}]", origin[0], origin[1], origin[2]));
  for(usize axis = 0; axis < expectedMin.size(); ++axis)
  {
    CAPTURE(axis);
    CHECK(origin[axis] == Approx(expectedMin[axis]).epsilon(0).margin(k_OriginTolerance));
    CHECK(bounds.getMinPoint()[axis] == Approx(expectedMin[axis]).epsilon(0).margin(k_OriginTolerance));
    CHECK(bounds.getMaxPoint()[axis] == Approx(expectedMax[axis]).epsilon(0).margin(k_OriginTolerance));
  }
}

void ApplyExplicitTransforms(DataStructure& dataStructure)
{
  RotateEulerRefFrameFilter eulerFilter;
  Arguments eulerArgs;
  eulerArgs.insertOrAssign(RotateEulerRefFrameFilter::k_EulerAnglesArrayPath_Key, std::make_any<DataPath>(Constants::k_CellAttributeMatrix.createChildPath(Constants::k_EulerAngles)));
  eulerArgs.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0F, 0.0F, 1.0F, k_EulerRotationDegrees}));
  const auto eulerResult = eulerFilter.execute(dataStructure, eulerArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(eulerResult.result);

  const auto pluginId = *Uuid::FromString(k_SimplnxCorePluginId.str());
  const auto filterId = *Uuid::FromString(k_RotateSampleRefFrameFilterId.str());
  auto sampleFilter = Application::Instance()->getFilterList()->createFilter(FilterHandle(filterId, pluginId));
  REQUIRE(sampleFilter != nullptr);
  Arguments sampleArgs;
  sampleArgs.insertOrAssign(k_RotateSelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  sampleArgs.insertOrAssign(k_RotateCreatedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  sampleArgs.insertOrAssign(k_RotateRotationAxisAngle_Key, std::make_any<VectorFloat32Parameter::ValueType>({0.0F, 1.0F, 0.0F, k_SampleRotationDegrees}));
  sampleArgs.insertOrAssign(k_RotateRotationRepresentation_Key, std::make_any<ChoicesParameter::ValueType>(k_AxisAngleRepresentation));
  sampleArgs.insertOrAssign(k_RotateSliceBySlice_Key, std::make_any<bool>(true));
  sampleArgs.insertOrAssign(k_RotateRemoveOriginalGeometry_Key, std::make_any<bool>(true));
  sampleArgs.insertOrAssign(k_RotateKeepInputGeometryOrigin_Key, std::make_any<bool>(false));
  const auto sampleResult = sampleFilter->execute(dataStructure, sampleArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(sampleResult.result);
}

void CompareImportedArrays(const DataStructure& automatic, const DataStructure& manual)
{
  for(const auto& groupPath : {Constants::k_CellAttributeMatrix, Constants::k_CellEnsembleAttributeMatrixPath})
  {
    REQUIRE_NOTHROW(automatic.getDataRefAs<AttributeMatrix>(groupPath));
    REQUIRE_NOTHROW(manual.getDataRefAs<AttributeMatrix>(groupPath));
    const auto& automaticGroup = automatic.getDataRefAs<AttributeMatrix>(groupPath);
    const auto& manualGroup = manual.getDataRefAs<AttributeMatrix>(groupPath);
    REQUIRE(automaticGroup.getSize() == manualGroup.getSize());
    for(const auto& [id, child] : automaticGroup)
    {
      const auto arrayPath = groupPath.createChildPath(child->getName());
      INFO(arrayPath.toString());
      if(child->getName() == k_MaterialName)
      {
        REQUIRE_NOTHROW(automatic.getDataRefAs<StringArray>(arrayPath));
        REQUIRE_NOTHROW(manual.getDataRefAs<StringArray>(arrayPath));
        CompareStringArrays(automatic.getDataRefAs<StringArray>(arrayPath), manual.getDataRefAs<StringArray>(arrayPath));
        continue;
      }
      REQUIRE_NOTHROW(automatic.getDataRefAs<IDataArray>(arrayPath));
      REQUIRE_NOTHROW(manual.getDataRefAs<IDataArray>(arrayPath));
      CompareNumericArrays(automatic.getDataRefAs<IDataArray>(arrayPath), manual.getDataRefAs<IDataArray>(arrayPath), true);
    }
  }
}
} // namespace

TEST_CASE("OrientationAnalysis::ReadH5Ebsd: Valid filter execution", "[OrientationAnalysis][ReadH5Ebsd]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_h5ebsd.tar.gz", "Small_IN100.h5ebsd");

  auto* filterList = Application::Instance()->getFilterList();

  auto exemplarFilePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure exemplarDataStructure = LoadDataStructure(exemplarFilePath);

  DataStructure dataStructure;
  {
    ReadH5EbsdFilter filter;
    Arguments args;

    ReadH5EbsdFileParameter::ValueType h5ebsdParamVal;
    h5ebsdParamVal.inputFilePath = fmt::format("{}/Small_IN100.h5ebsd", unit_test::k_TestFilesDir);
    h5ebsdParamVal.startSlice = 1;
    h5ebsdParamVal.endSlice = 117;
    h5ebsdParamVal.eulerRepresentation = ebsdlib::AngleRepresentation::Radians;
    h5ebsdParamVal.selectedArrayNames = {Constants::k_ConfidenceIndex, Constants::k_EulerAngles, Constants::k_Fit, Constants::k_ImageQuality, Constants::k_Phases, Constants::k_SEMSignal};
    h5ebsdParamVal.useRecommendedTransform = true;

    args.insertOrAssign(ReadH5EbsdFilter::k_ReadH5EbsdParameter_Key, std::make_any<ReadH5EbsdFileParameter::ValueType>(h5ebsdParamVal));
    args.insertOrAssign(ReadH5EbsdFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
    args.insertOrAssign(ReadH5EbsdFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_CellData));
    args.insertOrAssign(ReadH5EbsdFilter::k_CellEnsembleAttributeMatrixName_Key, std::make_any<std::string>(Constants::k_EnsembleAttributeMatrix));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/read_h5ebsd_test.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellAttributeMatrix));
    auto& cellDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellAttributeMatrix);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellEnsembleAttributeMatrixPath));
    auto& cellEnsembleDataGroup = dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellEnsembleAttributeMatrixPath);
    std::vector<DataPath> selectedArrays;

    for(auto& child : cellDataGroup)
    {
      selectedArrays.push_back(Constants::k_CellAttributeMatrix.createChildPath(child.second->getName()));
    }
    for(auto& child : cellEnsembleDataGroup)
    {
      selectedArrays.push_back(Constants::k_CellEnsembleAttributeMatrixPath.createChildPath(child.second->getName()));
    }

    for(const auto& arrayPath : selectedArrays)
    {
      if(arrayPath.getTargetName() == ::k_MaterialName)
      {
        // MaterialName is a StringArray. The numeric-array switch cannot
        // compare it.
        continue;
      }
      REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(arrayPath));
      const auto& generatedDataArray = dataStructure.getDataRefAs<IDataArray>(arrayPath);
      DataType type = generatedDataArray.getDataType();
      REQUIRE_NOTHROW(exemplarDataStructure.getDataRefAs<IDataArray>(arrayPath));
      auto& exemplarDataArray = exemplarDataStructure.getDataRefAs<IDataArray>(arrayPath);
      DataType exemplarType = exemplarDataArray.getDataType();

      if(type != exemplarType)
      {
        std::cout << fmt::format("DataArray {} and {} do not have the same type: {} vs {}. Data Will not be compared.", generatedDataArray.getName(), exemplarDataArray.getName(),
                                 fmt::underlying(type), fmt::underlying(exemplarType))
                  << std::endl;
        continue;
      }

      CompareNumericArrays(generatedDataArray, exemplarDataArray);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5EbsdFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ReadH5EbsdFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadH5EbsdFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadH5EbsdFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ReadH5EbsdFilter>::uuid);

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ReadH5EbsdFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(ReadH5EbsdFilter::k_CellAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(ReadH5EbsdFilter::k_CellEnsembleAttributeMatrixName_Key) == "TestName");

      const auto h5EbsdValue = args.value<ReadH5EbsdFileParameter::ValueType>(ReadH5EbsdFilter::k_ReadH5EbsdParameter_Key);
      CHECK(h5EbsdValue.inputFilePath == "/test/path/file.h5ebsd");
      CHECK(h5EbsdValue.startSlice == 0);
      CHECK(h5EbsdValue.endSlice == 10);
      CHECK(h5EbsdValue.useRecommendedTransform == true);
      CHECK(h5EbsdValue.selectedArrayNames == std::vector<std::string>{"Confidence Index", "Image Quality"});
    }
  }
}

TEST_CASE("OrientationAnalysis::ReadH5Ebsd: Single sample rotation origin", "[OrientationAnalysis][ReadH5Ebsd]")
{
  UnitTest::LoadPlugins();
  const TestFileSentinel sentinel(unit_test::k_TestFilesDir, "6_5_h5ebsd_exemplar.tar.gz", "6_5_h5ebsd_exemplar");
  const int32 endSlice = GENERATE(k_FirstSlice, k_LastSlice);
  CAPTURE(endSlice);
  const auto args = SmallArchiveArguments(endSlice, true);
  ReadH5EbsdFilter filter;
  DataStructure dataStructure;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  std::optional<FloatVec3> preflightOrigin;
  for(const auto& action : preflightResult.outputActions.value().actions)
  {
    if(const auto* update = dynamic_cast<const UpdateImageGeomAction*>(action.get()); update != nullptr)
    {
      REQUIRE(update->path() == Constants::k_DataContainerPath);
      REQUIRE(update->origin().has_value());
      preflightOrigin = update->origin();
    }
  }
  REQUIRE(preflightOrigin.has_value());
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto& geometry = GetImageGeometry(dataStructure);
  CheckRotatedGeometry(geometry, static_cast<usize>(endSlice));
  CHECK(geometry.getOrigin() == preflightOrigin.value());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ReadH5Ebsd: Automatic and explicit transformations agree", "[OrientationAnalysis][ReadH5Ebsd]")
{
  UnitTest::LoadPlugins();
  const TestFileSentinel sentinel(unit_test::k_TestFilesDir, "6_5_h5ebsd_exemplar.tar.gz", "6_5_h5ebsd_exemplar");
  const int32 endSlice = GENERATE(k_FirstSlice, k_LastSlice);
  CAPTURE(endSlice);
  ReadH5EbsdFilter filter;
  DataStructure automatic;
  const auto automaticResult = filter.execute(automatic, SmallArchiveArguments(endSlice, true));
  SIMPLNX_RESULT_REQUIRE_VALID(automaticResult.result);
  DataStructure manual;
  const auto manualResult = filter.execute(manual, SmallArchiveArguments(endSlice, false));
  SIMPLNX_RESULT_REQUIRE_VALID(manualResult.result);
  const FloatVec3 untransformedOrigin = {k_Zero, k_Zero, k_Zero};
  REQUIRE(GetImageGeometry(manual).getOrigin() == untransformedOrigin);
  ApplyExplicitTransforms(manual);
  const auto& automaticGeometry = GetImageGeometry(automatic);
  const auto& manualGeometry = GetImageGeometry(manual);
  CheckRotatedGeometry(manualGeometry, static_cast<usize>(endSlice));
  CHECK(automaticGeometry.getOrigin() == manualGeometry.getOrigin());
  CHECK(automaticGeometry.getDimensions() == manualGeometry.getDimensions());
  CHECK(automaticGeometry.getSpacing() == manualGeometry.getSpacing());
  CompareImportedArrays(automatic, manual);
  UnitTest::CheckArraysInheritTupleDims(automatic);
  UnitTest::CheckArraysInheritTupleDims(manual);
}
