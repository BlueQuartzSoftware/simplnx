#pragma once

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"
#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

namespace complex
{
namespace ITKTestBase
{
// using DataStructurePointer = std::shared_ptr<DataStructure>;

static inline constexpr StringLiteral k_ImageGeometryPath = "[ImageGeometry]";
static inline constexpr StringLiteral k_BaselineGeometryPath = "[BaselineGeometry]";
static inline constexpr StringLiteral k_InputDataPath = "Input";
static inline constexpr StringLiteral k_OutputDataPath = "Output";
static inline constexpr StringLiteral k_BaselineDataPath = "Baseline";

int32_t ReadImage(DataStructure& ds, const fs::path& filePath, const DataPath& geometryPath, const DataPath& imagePath);

int32_t WriteImage(DataStructure& ds, const fs::path& filePath, const DataPath& geometryPath, const DataPath& imagePath);

Result<> CompareImages(DataStructure& ds, const DataPath& baselineGeometryPath, const DataPath& baselineDataPath, const DataPath& inputGeometryPath, const DataPath& outputDataPath, double tolerance);

std::string ComputeMd5Hash(DataStructure& ds, const DataPath& outputDataPath);

} // namespace ITKTestBase
} // namespace complex
