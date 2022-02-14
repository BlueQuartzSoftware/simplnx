#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/IFilter.hpp"

#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace complex
{
/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */
class COMPLEXCORE_EXPORT StlFileReader
{
public:
  StlFileReader(DataStructure& data, fs::path stlFilePath, const DataPath& geometryPath, const DataPath& faceGroupPath, const DataPath& faceNormalsDataPath, const std::atomic_bool& shouldCancel);
  ~StlFileReader() noexcept;

  StlFileReader(const StlFileReader&) = delete;
  StlFileReader(StlFileReader&&) noexcept = delete;
  StlFileReader& operator=(const StlFileReader&) = delete;
  StlFileReader& operator=(StlFileReader&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief readFile Reads the .stl file
   */
  Result<> readFile();

  /**
   * @brief eliminate_duplicate_nodes Removes duplicate nodes to ensure the
   * created vertex list is shared
   */
  Result<> eliminate_duplicate_nodes();

private:
  // Holds the min/max coordinates for X, Y, Z laid out at { X_Min, X_Max, Y_Min, Y_Max, Z_Min, Z_Max}
  std::array<float, 6> m_MinMaxCoords = {std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                                         -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),  -std::numeric_limits<float>::max()};

  DataStructure& m_DataStructure;
  const fs::path m_FilePath;
  const DataPath& m_GeometryDataPath;
  const DataPath& m_FaceGroupPath;
  const DataPath m_FaceNormalsDataPath;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace complex
