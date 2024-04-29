#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace fs = std::filesystem;

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadBinaryCTNorthstarInputValues
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
 * @class
 */
class SIMPLNXCORE_EXPORT ReadBinaryCTNorthstar
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

} // namespace nx::core
