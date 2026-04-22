#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"

#include <filesystem>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadNIfTIFileInputValues
{
  std::filesystem::path InputFilePath;
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string ImageDataArrayName;
  bool UseAffineIfPresent{true};
  bool ApplyScalingTransform{true};
  CropGeometryParameter::ValueType CroppingOptions;
};

/**
 * @class ReadNIfTIFile
 * @brief Streams the voxel data out of a NIfTI-1 (.nii/.nii.gz) file into the
 * pre-allocated DataArray created by ReadNIfTIFileFilter::preflightImpl.
 */
class SIMPLNXCORE_EXPORT ReadNIfTIFile
{
public:
  ReadNIfTIFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadNIfTIFileInputValues* inputValues);
  ~ReadNIfTIFile() noexcept;

  ReadNIfTIFile(const ReadNIfTIFile&) = delete;
  ReadNIfTIFile(ReadNIfTIFile&&) noexcept = delete;
  ReadNIfTIFile& operator=(const ReadNIfTIFile&) = delete;
  ReadNIfTIFile& operator=(ReadNIfTIFile&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadNIfTIFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
