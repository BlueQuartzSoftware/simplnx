#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Geometry/GeometryTransformation.hpp"
#include "simplnx/Utilities/ImageProcessing/ColorToGrayScale.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

using namespace nx::core;

TEST_CASE("simplnx::AppendUnchangedAttributeMatricesWarning: reports source paths and excludes aliases by ID", "[simplnx][FilterUtilities]")
{
  DataStructure dataStructure;
  const auto* source = DataGroup::Create(dataStructure, "Source");
  REQUIRE(source != nullptr);
  const auto* nested = DataGroup::Create(dataStructure, "Nested", source->getId());
  REQUIRE(nested != nullptr);
  const auto* selected = AttributeMatrix::Create(dataStructure, "Selected Features", {3}, source->getId());
  REQUIRE(selected != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(selected->getId(), nested->getId()));

  const DataPath selectedPath({"Source", "Selected Features"});
  const DataPath nestedPath({"Source", "Nested"});
  Result<OutputActions> result;

  SECTION("Copied matrices appear in path order")
  {
    REQUIRE(AttributeMatrix::Create(dataStructure, "Other Features", {2}, nested->getId()) != nullptr);
    REQUIRE(AttributeMatrix::Create(dataStructure, "Ensemble", {2}, source->getId()) != nullptr);
    AppendUnchangedAttributeMatricesWarning(dataStructure, {selectedPath, nestedPath, DataPath({"Source", "Ensemble"})}, {selectedPath}, "cropped geometry", result);
    REQUIRE(result.warnings().size() == 1);
    CHECK(result.warnings().front().code == -4041);
    CHECK(result.warnings().front().message == "These Attribute Matrices are copied to the cropped geometry unchanged:\n"
                                               "Source/Ensemble\n"
                                               "Source/Nested/Other Features\n"
                                               "If one holds Feature data, its values describe the original geometry. Recompute them. Ensemble data is not affected.");
  }

  SECTION("Only excluded matrices are present")
  {
    AppendUnchangedAttributeMatricesWarning(dataStructure, {nestedPath}, {selectedPath}, "cropped geometry", result);
    CHECK(result.warnings().empty());
  }
}

TEST_CASE("simplnx::PreflightColorToGrayScale: rejects inputs with fewer than three color components", "[simplnx][PreflightColorToGrayScale]")
{
  const auto algorithm = GENERATE(ConvertColorToGrayScale::ConversionType::Luminosity, ConvertColorToGrayScale::ConversionType::Average, ConvertColorToGrayScale::ConversionType::Lightness);
  const usize components = GENERATE(usize{1}, usize{2});
  CAPTURE(components);
  DataStructure dataStructure;
  REQUIRE(UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Input", {2}, {components}) != nullptr);
  ConvertColorToGrayScaleInputValues inputValues{};
  inputValues.ConversionAlgorithm = to_underlying(algorithm);
  inputValues.ColorWeights = {0.25F, 0.5F, 0.25F};
  inputValues.InputDataArrayPaths = {DataPath({"Input"})};
  inputValues.OutputArrayPrefix = "Gray";
  auto result = PreflightColorToGrayScale(dataStructure, inputValues);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == -10710);
  REQUIRE(result.outputActions.errors().front().message.find("Input") != std::string::npos);
}

TEST_CASE("simplnx::PreflightColorToGrayScale: rejects channels outside the component range", "[simplnx][PreflightColorToGrayScale]")
{
  const usize components = GENERATE(usize{1}, usize{2}, usize{3}, usize{4});
  const bool negative = GENERATE(false, true);
  const int32 channel = negative ? -1 : static_cast<int32>(components);
  CAPTURE(components, channel);
  DataStructure dataStructure;
  REQUIRE(UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Input", {2}, {components}) != nullptr);
  ConvertColorToGrayScaleInputValues inputValues{};
  inputValues.ConversionAlgorithm = to_underlying(ConvertColorToGrayScale::ConversionType::SingleChannel);
  inputValues.ColorChannel = channel;
  inputValues.InputDataArrayPaths = {DataPath({"Input"})};
  inputValues.OutputArrayPrefix = "Gray";
  auto result = PreflightColorToGrayScale(dataStructure, inputValues);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == -10711);
  REQUIRE(result.outputActions.errors().front().message.find(fmt::format("Select a channel from 0 to {}.", components - 1)) != std::string::npos);
}

TEST_CASE("simplnx::PreflightGeometryTransformation: rejects node matrices without sixteen elements", "[simplnx][PreflightGeometryTransformation]")
{
  const usize elements = GENERATE(usize{3}, usize{17});
  CAPTURE(elements);
  DataStructure dataStructure;
  REQUIRE(TriangleGeom::Create(dataStructure, "Geometry") != nullptr);
  REQUIRE(Float32Array::CreateWithStore<DataStore<float32>>(dataStructure, "Matrix", {elements}, {1}) != nullptr);
  ApplyTransformationToGeometryInputValues inputValues{};
  inputValues.SelectedGeometryPath = DataPath({"Geometry"});
  inputValues.TransformationSelection = detail::k_PrecomputedTransformationMatrixIdx;
  inputValues.ComputedTransformationMatrix = DataPath({"Matrix"});
  auto result = PreflightGeometryTransformation(dataStructure, inputValues);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == -82019);
  REQUIRE(result.outputActions.errors().front().message.find("Matrix") != std::string::npos);
}

TEST_CASE("simplnx::PreflightColorToGrayScale: rejects arrays with no components", "[simplnx][PreflightColorToGrayScale]")
{
  DataStructure dataStructure;
  REQUIRE(UInt8Array::CreateWithStore<DataStore<uint8>>(dataStructure, "Input", {2}, {0}) != nullptr);
  ConvertColorToGrayScaleInputValues inputValues{};
  inputValues.ConversionAlgorithm = to_underlying(ConvertColorToGrayScale::ConversionType::SingleChannel);
  inputValues.ColorChannel = 0;
  inputValues.InputDataArrayPaths = {DataPath({"Input"})};
  auto result = PreflightColorToGrayScale(dataStructure, inputValues);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == -10711);
  const auto& message = result.outputActions.errors().front().message;
  REQUIRE(message.find("Input") != std::string::npos);
  REQUIRE(message.find("has no components") != std::string::npos);
  REQUIRE(message.find("Select an array with at least one component.") != std::string::npos);
  REQUIRE(message.find("from 0 to") == std::string::npos);
}
