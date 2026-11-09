#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Geometry/GeometryTransformation.hpp"
#include "simplnx/Utilities/ImageProcessing/ColorToGrayScale.hpp"

#include <catch2/catch.hpp>
#include <fmt/format.h>

using namespace nx::core;

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
