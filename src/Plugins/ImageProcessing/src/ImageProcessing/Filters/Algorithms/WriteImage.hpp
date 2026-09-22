#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOEnums.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nx::core
{

/**
 * @struct WriteImageInputValues
 * @brief Stores the values that control image slice output.
 */
struct IMAGEPROCESSING_EXPORT WriteImageInputValues
{
  std::filesystem::path outputFilePath;
  usize planeIndex = 0;
  uint64 indexOffset = 0;
  int32 totalIndexDigits = 3;
  std::string leadingDigitCharacter = "0";
  DataPath imageGeometryPath;
  DataPath imageDataArrayPath;
  bool createColorTable = false;
  std::string presetName;
  bool useMask = false;
  DataPath maskArrayPath;
  std::vector<uint8> invalidColor;
  ImageFlipTransform flipMode = ImageFlipTransform::None;
  bool addScaleBar = false;
};

/**
 * @class WriteImage
 * @brief Selects the direct or bounded writer from the source-array storage.
 */
class IMAGEPROCESSING_EXPORT WriteImage
{
public:
  /**
   * @brief Creates an image writer.
   * @param dataStructure Contains the source Image Geometry and Data Array.
   * @param messageHandler Receives slice progress messages.
   * @param shouldCancel Stops output between slices when set.
   * @param inputValues Contains the output settings and source paths.
   */
  WriteImage(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues);

  /** @brief Destroys the image writer. */
  ~WriteImage() noexcept;

  WriteImage(const WriteImage&) = delete;
  WriteImage(WriteImage&&) noexcept = delete;
  WriteImage& operator=(const WriteImage&) = delete;
  WriteImage& operator=(WriteImage&&) noexcept = delete;

  /**
   * @brief Writes all slices for the selected plane.
   * @return An error if slice extraction, compression, or file output fails.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteImageInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
