#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

namespace fs = std::filesystem;

namespace complex
{

struct COMPLEXCORE_EXPORT ReadBinaryCTNorthstarInputValues
{
  FileSystemPathParameter::ValueType InputHeaderFile;
  DataPath ImageGeometryPath;
  DataPath DensityArrayPath;
  std::vector<std::pair<fs::path, usize>> DataFilePaths;
  SizeVec3 OriginalGeometryDims;
  SizeVec3 ImportedGeometryDims;
  bool ImportSubvolume;
  IntVec3 StartVoxelCoord;
  IntVec3 EndVoxelCoord;
  ChoicesParameter::ValueType LengthUnit;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT ReadBinaryCTNorthstar
{
public:
  ReadBinaryCTNorthstar(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ReadBinaryCTNorthstarInputValues* inputValues);
  ~ReadBinaryCTNorthstar() noexcept;

  ReadBinaryCTNorthstar(const ReadBinaryCTNorthstar&) = delete;
  ReadBinaryCTNorthstar(ReadBinaryCTNorthstar&&) noexcept = delete;
  ReadBinaryCTNorthstar& operator=(const ReadBinaryCTNorthstar&) = delete;
  ReadBinaryCTNorthstar& operator=(ReadBinaryCTNorthstar&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadBinaryCTNorthstarInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
