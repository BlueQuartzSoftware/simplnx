#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"

#include "OrientationAnalysis/Filters/GenerateFZQuaternions.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;
using namespace complex::Constants;

namespace
{
DataStructure CreateDataStructure()
{
  DataStructure dataGraph;

  DataGroup* group = complex::DataGroup::Create(dataGraph, complex::Constants::k_SmallIN100);
  DataGroup* scanData = complex::DataGroup::Create(dataGraph, complex::Constants::k_EbsdScanData, group->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, k_SmallIn100ImageGeom, scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  complex::SizeVec3 imageGeomDims = {100, 100, 2};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  std::vector<size_t> compDims = {4};
  std::vector<size_t> tupleDims = {100, 100, 2};

  std::string filePath = complex::unit_test::k_DataDir.str();

  std::string fileName = "/quats.raw";
  complex::ImportFromBinaryFile<float>(filePath + fileName, k_Quats, dataGraph, tupleDims, compDims, scanData->getId());

  fileName = "/fz_quats.raw";
  complex::ImportFromBinaryFile<float>(filePath + fileName, "FZ_QUATS_EXEMPLAR", dataGraph, tupleDims, compDims, scanData->getId());

  Int32Array* phases_data = complex::UnitTest::CreateTestDataArray<int32>(dataGraph, k_Phases, tupleDims, {1}, scanData->getId());
  phases_data->fill(1);

  // Add in another group that is just information about the grid data.
  DataGroup* phaseGroup = complex::DataGroup::Create(dataGraph, k_PhaseData, group->getId());
  UInt32Array* laueClass = UInt32Array::CreateWithStore<UInt32DataStore>(dataGraph, k_LaueClass, {2}, {1}, phaseGroup->getId());
  (*laueClass)[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  (*laueClass)[1] = EbsdLib::CrystalStructure::Cubic_High;

  return dataGraph;
}

void MessageHandlerFunction(const IFilter::Message& message)
{
}

} // namespace

TEST_CASE("OrientationAnalysis::GenerateFZQuaternions: Instantiation and Parameter Check", "[OrientationAnalysis][GenerateFZQuaternions]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  GenerateFZQuaternions filter;
  DataStructure dataStructure = CreateDataStructure();
  Arguments args;

  DataPath scanDataPath = DataPath({complex::Constants::k_SmallIN100, complex::Constants::k_EbsdScanData});
  // Create default Parameters for the filter.

  args.insertOrAssign(GenerateFZQuaternions::k_QuatsArrayPath_Key, std::make_any<DataPath>(scanDataPath.createChildPath(k_Quats)));
  args.insertOrAssign(GenerateFZQuaternions::k_FZQuatsArrayPath_Key, std::make_any<DataPath>(scanDataPath.createChildPath(k_FZQuats)));
  args.insertOrAssign(GenerateFZQuaternions::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(scanDataPath.createChildPath(k_Phases)));
  args.insertOrAssign(GenerateFZQuaternions::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({k_SmallIN100, k_PhaseData, k_LaueClass})));

  args.insertOrAssign(GenerateFZQuaternions::k_UseGoodVoxels_Key, std::make_any<bool>(false));
  args.insertOrAssign(GenerateFZQuaternions::k_GoodVoxelsArrayPath_Key, std::make_any<DataPath>(DataPath{}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  if(preflightResult.outputActions.invalid())
  {
    for(const auto& error : preflightResult.outputActions.errors())
    {
      std::cout << error.code << ": " << error.message << std::endl;
    }
  }
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  // Compare Results
  auto generatedFZQuats = dataStructure.getDataRefAs<Float32Array>(scanDataPath.createChildPath(k_FZQuats));
  auto exemplarFZQuats = dataStructure.getDataRefAs<Float32Array>(scanDataPath.createChildPath("FZ_QUATS_EXEMPLAR"));
  std::vector<size_t> misMatchIndices;
  for(size_t idx = 0; idx < generatedFZQuats.getNumberOfTuples(); idx++)
  {
    if(generatedFZQuats[idx * 4] != exemplarFZQuats[idx * 4])
    {
      misMatchIndices.push_back(idx);
      continue;
    }
    if(generatedFZQuats[idx * 4 + 1] != exemplarFZQuats[idx * 4 + 1])
    {
      misMatchIndices.push_back(idx);
      continue;
    }
    if(generatedFZQuats[idx * 4 + 2] != exemplarFZQuats[idx * 4 + 2])
    {
      misMatchIndices.push_back(idx);
      continue;
    }
    if(generatedFZQuats[idx * 4 + 3] != exemplarFZQuats[idx * 4 + 3])
    {
      misMatchIndices.push_back(idx);
      continue;
    }
  }
  REQUIRE(misMatchIndices.size() == 0);
}
