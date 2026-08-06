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

struct SIMPLNXCORE_EXPORT ReadStlFileInputValues
{
  fs::path stlFilePath;
  DataPath geometryPath;
  DataPath faceGroupPath;
  DataPath faceNormalsDataPath;
  bool scaleOutput;
  float32 scaleFactor;
};

/**
 * @class ReadStlFile
 */
class SIMPLNXCORE_EXPORT ReadStlFile
{
public:
  ReadStlFile(DataStructure& dataStructure, ReadStlFileInputValues& inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
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
  const ReadStlFileInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
