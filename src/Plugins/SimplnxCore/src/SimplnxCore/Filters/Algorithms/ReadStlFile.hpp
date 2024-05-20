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
 * @class ConditionalSetValueFilter

 */
class SIMPLNXCORE_EXPORT ReadStlFile
{
public:
  ReadStlFile(DataStructure& dataStructure, fs::path stlFilePath, const DataPath& geometryPath, const DataPath& faceGroupPath, const DataPath& faceNormalsDataPath, bool scaleOutput,
              float32 scaleFactor, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
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

private:
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
