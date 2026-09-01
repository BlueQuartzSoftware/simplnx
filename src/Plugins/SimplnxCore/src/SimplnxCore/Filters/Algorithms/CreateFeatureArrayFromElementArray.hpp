#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{

/**
 * @struct CreateFeatureArrayFromElementArrayInputValues
 * @brief Collects input, output, and feature paths.
 */
struct SIMPLNXCORE_EXPORT CreateFeatureArrayFromElementArrayInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath;
  DataObjectNameParameter::ValueType CreatedArrayName;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  ArraySelectionParameter::ValueType SelectedCellArrayPath;
};

/**
 * @class CreateFeatureArrayFromElementArray
 * @brief Creates one feature value from the last matching element value.
 *
 * Element data uses fixed-size bulk transfers. First and final values remain in
 * feature-scale memory. The first inconsistent component adds one warning, and
 * the final element value becomes the feature output.
 */
class SIMPLNXCORE_EXPORT CreateFeatureArrayFromElementArray
{
public:
  /**
   * @brief Initializes feature-array creation.
   * @param dataStructure Contains source and destination arrays.
   * @param mesgHandler Supplies the common algorithm message interface.
   * @param shouldCancel Signals cancellation between chunks.
   * @param inputValues Identifies source and destination paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  CreateFeatureArrayFromElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     CreateFeatureArrayFromElementArrayInputValues* inputValues);
  ~CreateFeatureArrayFromElementArray() noexcept;

  CreateFeatureArrayFromElementArray(const CreateFeatureArrayFromElementArray&) = delete;
  CreateFeatureArrayFromElementArray(CreateFeatureArrayFromElementArray&&) noexcept = delete;
  CreateFeatureArrayFromElementArray& operator=(const CreateFeatureArrayFromElementArray&) = delete;
  CreateFeatureArrayFromElementArray& operator=(CreateFeatureArrayFromElementArray&&) noexcept = delete;

  /**
   * @brief Copies the final element value for each feature.
   * @return Success with an optional inconsistency warning, or an input or bulk-transfer error.
   *
   * Negative feature IDs return an error before structural changes. The feature
   * matrix can grow before element transfer starts. Cancellation then returns
   * success without publishing the feature-value buffer.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateFeatureArrayFromElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
