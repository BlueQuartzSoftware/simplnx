
#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ScalarSegmentFeatures.hpp"

using namespace complex;
using namespace complex::UnitTest;
using namespace complex::Constants;

namespace
{
DataStructure CreateEbsdTestDataStructure()
{
  DataStructure dataGraph;

  DataGroup* group = complex::DataGroup::Create(dataGraph, complex::Constants::k_SmallIN100);
  DataGroup* scanData = complex::DataGroup::Create(dataGraph, complex::Constants::k_EbsdScanData, group->getId());
  DataGroup* featureGroup = complex::DataGroup::Create(dataGraph, complex::Constants::k_FeatureGroupName, group->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, k_SmallIn100ImageGeom, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  complex::SizeVec3 imageGeomDims = {100, 100, 100};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  std::vector<size_t> compDims = {1};
  std::vector<size_t> tupleDims = {100, 100, 100};

  std::string filePath = complex::unit_test::k_SourceDir.str() + "/data/";

  // std::string fileName = "ConfidenceIndex.raw";
  // complex::ImportFromBinaryFile<float>(filePath + fileName, k_ConfidenceIndex, dataGraph, tupleDims, compDims, scanData->getId());

  std::string fileName = "FeatureIds.raw";
  complex::ImportFromBinaryFile<int32_t>(filePath + fileName, k_FeatureIds, dataGraph, tupleDims, compDims, scanData->getId());

  // fileName = "ImageQuality.raw";
  // complex::ImportFromBinaryFile<float>(filePath + fileName, k_ImageQuality, dataGraph, tupleDims, compDims, scanData->getId());

  // fileName = "Phases.raw";
  // complex::ImportFromBinaryFile<int32_t>(filePath + fileName, k_Phases, dataGraph, tupleDims, compDims, scanData->getId());

  // fileName = "IPFColors.raw";
  // compDims = {3};
  // complex::ImportFromBinaryFile<uint8_t>(filePath + fileName, k_IpfColors, dataGraph, tupleDims, compDims, scanData->getId());

  // Add in another group that is just information about the grid data.
  // DataGroup* phaseGroup = complex::DataGroup::Create(dataGraph, k_PhaseData, group->getId());
  // Int32Array::CreateWithStore<Int32DataStore>(dataGraph, k_LaueClass, {2}, compDims, phaseGroup->getId());

  return dataGraph;
}
} // namespace

TEST_CASE("Reconstruction::ScalarSegmentFeatures", "[Reconstruction][ScalarSegmentFeatures]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object

  DataStructure dataGraph = ::CreateEbsdTestDataStructure();

  std::vector<size_t> imageDims = {100, 100, 100};
  size_t numTuples = imageDims[0] * imageDims[1] * imageDims[2];
  FloatVec3 imageSpacing = {0.10F, 2.0F, 33.0F};
  FloatVec3 imageOrigin = {
      0.0f,
      22.0f,
      77.0f,
  };

  {
    Arguments args;
    ScalarSegmentFeatures filter;

    DataPath smallIn100Group({k_SmallIN100});
    DataPath ebsdScanDataPath = smallIn100Group.createChildPath(k_EbsdScanData);
    DataPath inputDataArrayPath = ebsdScanDataPath.createChildPath(k_FeatureIds);
    DataPath outputFeatureIds = ebsdScanDataPath.createChildPath("Output_Feature_Ids");

    DataPath featureDataGroupPath = smallIn100Group.createChildPath(k_FeatureGroupName);
    DataPath activeArrayDataPath = featureDataGroupPath.createChildPath(k_ActiveName);

    DataPath gridGeomDataPath({k_SmallIN100, k_EbsdScanData, k_SmallIn100ImageGeom});
    int scalarTolerance = 0;

    // Create default Parameters for the filter.
    args.insertOrAssign(SegmentFeatures::k_GridGeomPath_Key, std::make_any<DataPath>(gridGeomDataPath));
    // Turn off the use of a Mask Array
    args.insertOrAssign(ScalarSegmentFeatures::k_UseGoodVoxelsKey, std::make_any<bool>(false));
    args.insertOrAssign(ScalarSegmentFeatures::k_GoodVoxelsPathKey, std::make_any<DataPath>(DataPath{}));
    // Set the input array and the tolerance
    args.insertOrAssign(ScalarSegmentFeatures::k_InputArrayPathKey, std::make_any<DataPath>(inputDataArrayPath));
    args.insertOrAssign(ScalarSegmentFeatures::k_ScalarToleranceKey, std::make_any<int>(scalarTolerance));
    // Set the paths to the created arrays
    args.insertOrAssign(ScalarSegmentFeatures::k_FeatureIdsPathKey, std::make_any<DataPath>(outputFeatureIds));
    args.insertOrAssign(ScalarSegmentFeatures::k_ActiveArrayPathKey, std::make_any<DataPath>(activeArrayDataPath));
    // Are we going to randomize the featureIds when completed.
    args.insertOrAssign(ScalarSegmentFeatures::k_RandomizeFeaturesKey, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataGraph, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataGraph, args);
    REQUIRE(executeResult.result.valid());

    // Let's sum up all the areas.
    UInt8Array& actives = dataGraph.getDataRefAs<UInt8Array>(activeArrayDataPath);
    size_t numFeatures = actives.getNumberOfTuples();
    std::cout << "NumFeatures: " << numFeatures << std::endl;
    REQUIRE(numFeatures == 847);
  }

  // Write out the DataStructure for later viewing/debugging
  std::string filePath = fmt::format("{}/ScalarSegmentFeatures.dream3d", unit_test::k_BinaryDir);
  std::cout << "Writing file to: " << filePath << std::endl;
  Result<H5::FileWriter> result = H5::FileWriter::CreateFile(filePath);
  H5::FileWriter fileWriter = std::move(result.value());

  herr_t err = dataGraph.writeHdf5(fileWriter);
  REQUIRE(err >= 0);
}
