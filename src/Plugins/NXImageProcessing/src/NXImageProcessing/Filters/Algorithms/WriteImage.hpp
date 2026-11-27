#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

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
struct NXIMAGEPROCESSING_EXPORT WriteImageInputValues
{
  std::filesystem::path output_file_path;
  usize plane_index = 0;
  uint64 index_offset = 0;
  int32 total_index_digits = 3;
  std::string leading_digit_character = "0";
  DataPath image_geometry_path;
  DataPath image_data_array_path;
  bool create_color_table = false;
  std::string preset_name;
  bool use_mask = false;
  DataPath mask_array_path;
  std::vector<uint8> invalid_color;
  ImageFlipTransform flip_mode = ImageFlipTransform::None;
  bool add_scale_bar = false;
};

/**
 * @class WriteImage
 * @brief Selects the direct or bounded writer from the source-array storage.
 */
class NXIMAGEPROCESSING_EXPORT WriteImage
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
  ~WriteImage() noexcept = default;

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
