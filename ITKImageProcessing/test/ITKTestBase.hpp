#pragma once

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

namespace fs = std::filesystem;

namespace complex
{
namespace ITKTestBase
{
inline constexpr StringLiteral k_ImageGeometryPath = "[ImageGeometry]";
inline constexpr StringLiteral k_MaskGeometryPath = "[MaskGeometry]";
inline constexpr StringLiteral k_BaselineGeometryPath = "[BaselineGeometry]";
inline constexpr StringLiteral k_InputDataPath = "Input";
inline constexpr StringLiteral k_OutputDataPath = "Output";
inline constexpr StringLiteral k_MaskDataPath = "Mask";
inline constexpr StringLiteral k_BaselineDataPath = "Baseline";

Result<> ReadImage(DataStructure& ds, const fs::path& filePath, const DataPath& geometryPath, const DataPath& imagePath);

int32_t WriteImage(DataStructure& ds, const fs::path& filePath, const DataPath& geometryPath, const DataPath& imagePath);

Result<> CompareImages(DataStructure& ds, const DataPath& baselineGeometryPath, const DataPath& baselineDataPath, const DataPath& inputGeometryPath, const DataPath& outputDataPath, double tolerance);

std::string ComputeMd5Hash(DataStructure& ds, const DataPath& outputDataPath);

void RemoveFiles(fs::path& dirPath, const std::string& filePattern);

template <typename T>
void WriteDataSetAsBinary(const fs::path& absolutePath, const DataStore<T>& dataStore)
{
  std::cout << "Writing Binary File: " << absolutePath.string() << std::endl;
  FILE* file = fopen(absolutePath.c_str(), "wb");

  size_t numElements = dataStore.getSize();

  size_t numWritten = fwrite(dataStore.data(), sizeof(T), numElements, file);

  fclose(file);
}
} // namespace ITKTestBase
} // namespace complex
