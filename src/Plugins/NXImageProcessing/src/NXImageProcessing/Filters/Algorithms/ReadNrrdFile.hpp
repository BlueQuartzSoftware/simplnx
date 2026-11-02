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
 * @struct ReadNrrdFileInputValues
 * @brief Plain-data inputs consumed by the ReadNrrdFile algorithm. The filter's
 *        executeImpl populates this from its Arguments. The DataPaths must match
 *        what the filter's preflight created.
 */
struct IMAGEPROCESSING_EXPORT ReadNrrdFileInputValues
{
  std::filesystem::path InputFilePath;
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string ImageDataArrayName;
  CropGeometryParameter::ValueType CroppingOptions;
};

/**
 * @class ReadNrrdFile
 * @brief Streams voxel data out of a NRRD file into the DataArray that
 *        ReadImageFilter's NRRD preflight created. Single pass: parse header,
 *        resolve crop bounds, open a NrrdDataReader, stream one source scan-line
 *        at a time into a destination scratch buffer (byteswap if needed), then
 *        bulk-copy the cropped subrange via CopyFromArray::CopyData (OOC-friendly).
 */
class IMAGEPROCESSING_EXPORT ReadNrrdFile
{
public:
  ReadNrrdFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadNrrdFileInputValues* inputValues);
  ~ReadNrrdFile() noexcept;

  ReadNrrdFile(const ReadNrrdFile&) = delete;
  ReadNrrdFile(ReadNrrdFile&&) noexcept = delete;
  ReadNrrdFile& operator=(const ReadNrrdFile&) = delete;
  ReadNrrdFile& operator=(ReadNrrdFile&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadNrrdFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
