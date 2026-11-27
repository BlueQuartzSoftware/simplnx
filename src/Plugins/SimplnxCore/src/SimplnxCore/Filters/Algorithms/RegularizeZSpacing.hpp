#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <filesystem>
#include <vector>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RegularizeZSpacingInputValues
{
  DataPath selected_image_geometry_path;
  std::filesystem::path input_file;
  float32 new_z_res = 0.0F;
  bool remove_original_image_geom = true;
  DataPath created_image_geometry_path;
  BoolParameter::ValueType clear_feature_attribute_matrix = false;
  BoolParameter::ValueType renumber_features = false;
  ArraySelectionParameter::ValueType feature_ids_array_path;
  AttributeMatrixSelectionParameter::ValueType cell_feature_attribute_matrix;
};

/**
 * @class RegularizeZSpacing
 * @brief Copies cell data onto a regularly Z-spaced Image Geometry and optionally renumbers
 * Feature Ids and resizes the cleared Feature Attribute Matrix.
 */
class SIMPLNXCORE_EXPORT RegularizeZSpacing
{
public:
  RegularizeZSpacing(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, RegularizeZSpacingInputValues* inputValues);
  ~RegularizeZSpacing() noexcept;

  RegularizeZSpacing(const RegularizeZSpacing&) = delete;
  RegularizeZSpacing(RegularizeZSpacing&&) noexcept = delete;
  RegularizeZSpacing& operator=(const RegularizeZSpacing&) = delete;
  RegularizeZSpacing& operator=(RegularizeZSpacing&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendThreadSafeProgressMessage(usize completedPlanes);

private:
  DataStructure& m_DataStructure;
  const RegularizeZSpacingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessageMutex;

  ThrottledMessageHandler m_Throttle;
};

/**
 * @brief Reads the Z boundary positions file into a vector of floats.
 * @param inputFile The path to the whitespace-delimited text file.
 * @param count The number of values to read (should be ZPoints + 1).
 * @return The parsed values, or an error Result if the file could not be opened or did not contain enough values.
 */
SIMPLNXCORE_EXPORT Result<std::vector<float32>> ReadZBoundsFile(const std::filesystem::path& inputFile, usize count);

/**
 * @brief Computes the number of Z planes for the resampled geometry.
 * @param lastZBound The total Z extent (last value in the Z bounds file).
 * @param newZRes The new (regular) Z spacing.
 * @return The number of Z planes, always at least 1.
 */
SIMPLNXCORE_EXPORT usize ComputeRegularizedZDim(float32 lastZBound, float32 newZRes);

} // namespace nx::core
