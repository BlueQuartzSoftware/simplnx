#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
struct SIMPLNXCORE_EXPORT DataArrayMetadata
{
  DataPath path;
  usize tupleCount;
  usize componentCount;
};

struct SIMPLNXCORE_EXPORT FileCache
{
  fs::path inputFile;
  std::vector<DataArrayMetadata> dataArrays;
  usize vertexAttrMatTupleCount;
  usize cellAttrMatTupleCount;
  fs::file_time_type timeStamp;

  void flush()
  {
    inputFile.clear();
    dataArrays.clear();
    timeStamp = fs::file_time_type();
  }
};

struct SIMPLNXCORE_EXPORT ReadDeformKeyFileV12InputValues
{
  fs::path InputFilePath;
  DataPath QuadGeomPath;
  DataPath VertexAMPath;
  DataPath CellAMPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT ReadDeformKeyFileV12
{
public:
  ReadDeformKeyFileV12(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadDeformKeyFileV12InputValues* inputValues);
  ~ReadDeformKeyFileV12() noexcept;

  ReadDeformKeyFileV12(const ReadDeformKeyFileV12&) = delete;
  ReadDeformKeyFileV12(ReadDeformKeyFileV12&&) noexcept = delete;
  ReadDeformKeyFileV12& operator=(const ReadDeformKeyFileV12&) = delete;
  ReadDeformKeyFileV12& operator=(ReadDeformKeyFileV12&&) noexcept = delete;

  Result<> operator()(bool allocate);

  const std::atomic_bool& getCancel();
  void updateProgress(const std::string& progMessage);
  FileCache& getCache();

private:
  DataStructure& m_DataStructure;
  const ReadDeformKeyFileV12InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  FileCache m_Cache = {};
};
} // namespace nx::core