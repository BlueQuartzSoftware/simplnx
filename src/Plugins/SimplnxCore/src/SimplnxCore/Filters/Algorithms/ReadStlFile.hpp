#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
/**
 * @class ConditionalSetValue

 */
class SIMPLNXCORE_EXPORT ReadStlFile
{
public:
  ReadStlFile(DataStructure& data, fs::path stlFilePath, const DataPath& geometryPath, const DataPath& faceGroupPath, const DataPath& faceNormalsDataPath, bool scaleOutput, float32 scaleFactor,
              const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ReadStlFile() noexcept;

  ReadStlFile(const ReadStlFile&) = delete;
  ReadStlFile(ReadStlFile&&) noexcept = delete;
  ReadStlFile& operator=(const ReadStlFile&) = delete;
  ReadStlFile& operator=(ReadStlFile&&) noexcept = delete;

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
  const bool m_ScaleOutput = false;
  const float m_ScaleFactor = 1.0F;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
