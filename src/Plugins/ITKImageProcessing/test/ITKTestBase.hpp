#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

namespace fs = std::filesystem;

namespace nx::core
{
namespace ITKTestBase
{
inline constexpr StringLiteral k_ImageGeometryPath = "[ImageGeometry]";
inline constexpr StringLiteral k_ImageCellDataName = "Cell Data";
inline constexpr StringLiteral k_MaskGeometryPath = "[MaskGeometry]";
inline constexpr StringLiteral k_BaselineGeometryPath = "[BaselineGeometry]";
inline constexpr StringLiteral k_InputDataName = "Input";
inline constexpr StringLiteral k_OutputDataPath = "Output";
inline constexpr StringLiteral k_MaskDataPath = "Mask";

inline constexpr StringLiteral k_BaselineDataPath = "Baseline";

Result<> ReadImage(DataStructure& dataStructure, const fs::path& filePath, const DataPath& geometryPath, const std::string& cellAttrMatName, const std::string& imageArrayName);

Result<> WriteImage(DataStructure& dataStructure, const fs::path& filePath, const DataPath& geometryPath, const DataPath& imagePath);

Result<> CompareImages(DataStructure& dataStructure, const DataPath& baselineGeometryPath, const DataPath& baselineDataPath, const DataPath& inputGeometryPath, const DataPath& outputDataPath,
                       float64 tolerance);

std::string ComputeMd5Hash(DataStructure& dataStructure, const DataPath& outputDataPath);

void RemoveFiles(const fs::path& dirPath, const std::string& filePattern);

template <class T>
void WriteDataSetAsBinary(const fs::path& absolutePath, const DataStore<T>& dataStore)
{
  std::cout << "Writing Binary File: " << absolutePath.string() << std::endl;
  FILE* file = std::fopen(absolutePath.c_str(), "wb");

  usize numElements = dataStore.getSize();

  usize numWritten = std::fwrite(dataStore.data(), sizeof(T), numElements, file);

  std::fclose(file);
}
} // namespace ITKTestBase
} // namespace nx::core
