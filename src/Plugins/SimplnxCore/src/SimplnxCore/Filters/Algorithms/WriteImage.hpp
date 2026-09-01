#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

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
 * @brief Stores slice selection, naming, color, mask, flip, and scale-bar settings.
 *
 * planeIndex maps 0 to XY, 1 to XZ, and 2 to YZ.
 * invalidColor supplies three RGB bytes for masked color-table pixels.
 */
struct SIMPLNXCORE_EXPORT WriteImageInputValues
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
 * @brief Writes ImageGeom slices through the IImageIO layer.
 *
 * Each slice uses a bounded source buffer. Color-table mode first reduces the
 * global range through 1 MiB pages, then colorizes one scalar slice at a time.
 *
 * Each output uses an AtomicFile. A multi-slice series commits each file separately.
 * Later cancellation or failure does not roll back earlier slice files.
 */
class SIMPLNXCORE_EXPORT WriteImage
{
public:
  /**
   * @brief Creates an image-slice writer.
   * @param dataStructure Provides image metadata and source arrays.
   * @param mesgHandler Receives per-slice messages.
   * @param shouldCancel Stops before later range pages or slices when true.
   * @param inputValues Specifies validated output settings. The caller must keep
   * this object alive for the writer lifetime.
   */
  WriteImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues);
  /**
   * @brief Destroys the non-owning writer.
   */
  ~WriteImage() noexcept;

  WriteImage(const WriteImage&) = delete;
  WriteImage(WriteImage&&) noexcept = delete;
  WriteImage& operator=(const WriteImage&) = delete;
  WriteImage& operator=(WriteImage&&) noexcept = delete;

  /**
   * @brief Extracts, transforms, and writes all selected slices.
   * @return Geometry, source-read, image-encoding, or AtomicFile error, or success after cancellation.
   *
   * Flip operations precede scale-bar padding so the scale bar stays upright.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteImageInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
