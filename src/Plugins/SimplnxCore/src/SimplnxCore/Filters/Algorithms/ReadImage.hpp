#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOEnums.hpp"

#include <filesystem>

namespace nx::core
{

/**
 * @struct ReadImageInputValues
 * @brief Stores file, output, spatial, type-conversion, and crop settings.
 */
struct SIMPLNXCORE_EXPORT ReadImageInputValues
{
  std::filesystem::path inputFilePath;
  DataPath imageGeometryPath;
  DataPath imageDataArrayPath;
  std::string cellDataName;
  bool changeOrigin = false;
  bool centerOrigin = false;
  FloatVec3 origin;
  bool changeSpacing = false;
  FloatVec3 spacing;
  OriginSpacingProcessing originSpacingProcessing = OriginSpacingProcessing::Postprocessed;
  bool changeDataType = false;
  DataType imageDataType = DataType::uint8;
  CropGeometryParameter::ValueType croppingOptions;
};

/**
 * @class ReadImage
 * @brief Streams one two-dimensional image into a preallocated DataArray.
 *
 * IImageIO supplies decoded row segments. Each segment is cropped, optionally
 * normalized to another scalar type, and written directly to the destination store.
 */
class SIMPLNXCORE_EXPORT ReadImage
{
public:
  /**
   * @brief Creates a single-image reader.
   * @param dataStructure Receives decoded pixels.
   * @param mesgHandler Receives file and conversion messages.
   * @param shouldCancel Stops the decoder callback when true.
   * @param inputValues Specifies validated file and output settings. The caller
   * must keep this object alive for the reader lifetime.
   */
  ReadImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadImageInputValues& inputValues);
  /**
   * @brief Destroys the non-owning reader.
   */
  ~ReadImage() noexcept;

  ReadImage(const ReadImage&) = delete;
  ReadImage(ReadImage&&) noexcept = delete;
  ReadImage& operator=(const ReadImage&) = delete;
  ReadImage& operator=(ReadImage&&) noexcept = delete;

  /**
   * @brief Reads, crops, converts, and writes decoded row segments.
   * @return Decoder, metadata, crop, or destination-write error, or success after cancellation.
   *
   * Cancellation returns success and retains destination segments written before the callback stops.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadImageInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
