#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace complex
{
//enum class DataArrayType : uint8
//{
//  VERTEX,
//  CELL
//};
//
//struct DataArrayMetadata
//{
//  std::string name;
//  usize tupleCount;
//  usize componentCount;
//  DataArrayType type;
//};
//
//struct FileCache
//{
//  std::string inputFile;
//  std::vector<DataArrayMetadata> dataArrays;
//  usize vertexAttrMatTupleCount;
//  usize cellAttrMatTupleCount;
//  fs::file_time_type timeStamp;
//
//  void flush()
//  {
//    inputFile.clear();
//    dataArrays.clear();
//    timeStamp = fs::file_time_type();
//  }
//};

/**
 * @class ImportDeformKeyFileV12Filter
 * @brief This filter will...
 */
class COMPLEXCORE_EXPORT ImportDeformKeyFileV12Filter : public IFilter
{
public:
  ImportDeformKeyFileV12Filter() = default;
  ~ImportDeformKeyFileV12Filter() noexcept override = default;

  ImportDeformKeyFileV12Filter(const ImportDeformKeyFileV12Filter&) = delete;
  ImportDeformKeyFileV12Filter(ImportDeformKeyFileV12Filter&&) noexcept = delete;

  ImportDeformKeyFileV12Filter& operator=(const ImportDeformKeyFileV12Filter&) = delete;
  ImportDeformKeyFileV12Filter& operator=(ImportDeformKeyFileV12Filter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFilePath_Key = "input_file_path";
  static inline constexpr StringLiteral k_UseVerboseOutput_Key = "use_verbose_output";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ImportDeformKeyFileV12Filter, "22c421c3-573c-4125-883c-4d95bd6e3bcb");