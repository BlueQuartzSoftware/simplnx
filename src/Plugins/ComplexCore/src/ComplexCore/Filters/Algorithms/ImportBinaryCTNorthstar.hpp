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

struct COMPLEXCORE_EXPORT ImportBinaryCTNorthstarInputValues
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

class COMPLEXCORE_EXPORT ImportBinaryCTNorthstar
{
public:
  ImportBinaryCTNorthstar(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ImportBinaryCTNorthstarInputValues* inputValues);
  ~ImportBinaryCTNorthstar() noexcept;

  ImportBinaryCTNorthstar(const ImportBinaryCTNorthstar&) = delete;
  ImportBinaryCTNorthstar(ImportBinaryCTNorthstar&&) noexcept = delete;
  ImportBinaryCTNorthstar& operator=(const ImportBinaryCTNorthstar&) = delete;
  ImportBinaryCTNorthstar& operator=(ImportBinaryCTNorthstar&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportBinaryCTNorthstarInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
