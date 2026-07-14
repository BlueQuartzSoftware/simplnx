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

struct SIMPLNXCORE_EXPORT WriteImageInputValues
{
  std::filesystem::path outputFilePath;
  usize planeIndex = 0; ///< 0=XY, 1=XZ, 2=YZ
  uint64 indexOffset = 0;
  int32 totalIndexDigits = 3;
  std::string leadingDigitCharacter = "0";
  DataPath imageGeometryPath;
  DataPath imageDataArrayPath;
  bool createColorTable = false;
  std::string presetName;
  bool useMask = false;
  DataPath maskArrayPath;
  std::vector<uint8> invalidColor; ///< size 3, RGB
  ImageFlipTransform flipMode = ImageFlipTransform::None;
};

/**
 * @class WriteImage
 * @brief Extracts 2D slices from a 3D ImageGeometry and writes them as
 * individual image files via the IImageIO layer.
 */
class SIMPLNXCORE_EXPORT WriteImage
{
public:
  WriteImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues);
  ~WriteImage() noexcept;

  WriteImage(const WriteImage&) = delete;
  WriteImage(WriteImage&&) noexcept = delete;
  WriteImage& operator=(const WriteImage&) = delete;
  WriteImage& operator=(WriteImage&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteImageInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
