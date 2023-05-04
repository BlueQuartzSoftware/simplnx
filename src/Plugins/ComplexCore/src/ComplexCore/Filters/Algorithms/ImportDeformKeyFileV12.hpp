#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/IFilter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace complex
{
enum class COMPLEXCORE_EXPORT DataArrayType : uint8
{
  VERTEX,
  CELL
};

struct COMPLEXCORE_EXPORT DataArrayMetadata
{
  std::string name;
  usize tupleCount;
  usize componentCount;
  DataArrayType type;
};

struct COMPLEXCORE_EXPORT FileCache
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

struct COMPLEXCORE_EXPORT ImportDeformKeyFileV12InputValues
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

class COMPLEXCORE_EXPORT ImportDeformKeyFileV12
{
public:
  ImportDeformKeyFileV12(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportDeformKeyFileV12InputValues* inputValues);
  ~ImportDeformKeyFileV12() noexcept;

  ImportDeformKeyFileV12(const ImportDeformKeyFileV12&) = delete;
  ImportDeformKeyFileV12(ImportDeformKeyFileV12&&) noexcept = delete;
  ImportDeformKeyFileV12& operator=(const ImportDeformKeyFileV12&) = delete;
  ImportDeformKeyFileV12& operator=(ImportDeformKeyFileV12&&) noexcept = delete;

  Result<> operator()(bool allocate);

  const std::atomic_bool& getCancel();
  void updateProgress(const std::string& progMessage);
  FileCache getCache();

private:
  DataStructure& m_DataStructure;
  const ImportDeformKeyFileV12InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  FileCache m_Cache = {};
  std::vector<std::string> m_UserDefinedVariables;
  std::vector<DataPath> m_UserDefinedArrays;
};
}