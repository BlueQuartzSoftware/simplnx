#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"

#include <filesystem>

namespace nx::core
{
/**
 * @struct ReadMhaFileInputValues
 * @brief Plain-data inputs consumed by the ReadMhaFile algorithm (the voxel-read
 *        half). The filter's executeImpl populates this from its Arguments; the
 *        transform feature is handled by the filter, not this algorithm.
 */
struct IMAGEPROCESSING_EXPORT ReadMhaFileInputValues
{
  std::filesystem::path InputFilePath;
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string ImageDataArrayName;
  CropGeometryParameter::ValueType CroppingOptions;
};

/**
 * @class ReadMhaFile
 * @brief Streams voxel data out of a MetaImage file into the DataArray that
 *        ReadMhaFileFilter::preflightImpl created. Single pass: parse header,
 *        resolve crop bounds, open a MetaImageDataReader, stream one source
 *        scan-line at a time into a destination scratch buffer (byteswap if
 *        needed), then bulk-copy the cropped subrange via CopyFromArray::CopyData
 *        (OOC-friendly). OutputT == the file's native type.
 */
class IMAGEPROCESSING_EXPORT ReadMhaFile
{
public:
  ReadMhaFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadMhaFileInputValues* inputValues);
  ~ReadMhaFile() noexcept;

  ReadMhaFile(const ReadMhaFile&) = delete;
  ReadMhaFile(ReadMhaFile&&) noexcept = delete;
  ReadMhaFile& operator=(const ReadMhaFile&) = delete;
  ReadMhaFile& operator=(ReadMhaFile&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadMhaFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
