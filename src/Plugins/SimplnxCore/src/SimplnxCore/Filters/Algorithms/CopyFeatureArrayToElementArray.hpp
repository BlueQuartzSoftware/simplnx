#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

/**
 * @struct CopyFeatureArrayToElementArrayInputValues
 * @brief Stores selected feature-array paths and output naming.
 */
struct SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArrayInputValues
{
  StringParameter::ValueType CreatedArraySuffix;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  MultiArraySelectionParameter::ValueType SelectedFeatureArrayPaths;
};

/**
 * @class CopyFeatureArrayToElementArray
 * @brief Dispatches feature-to-cell array broadcasts by storage.
 *
 * Every cell receives the tuple of its Feature Id. Feature Id, selected feature,
 * and created cell arrays all drive dispatch because mixed storage is valid.
 *
 * @see CopyFeatureArrayToElementArrayDirect, CopyFeatureArrayToElementArrayScanline, DispatchAlgorithm
 */
class SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArray
{
public:
  /**
   * @brief Creates a feature-to-cell dispatcher.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later arrays or chunks when true.
   * @param inputValues Specifies validated paths and naming. The caller must
   * keep this object alive for the dispatcher lifetime.
   */
  CopyFeatureArrayToElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 const CopyFeatureArrayToElementArrayInputValues* inputValues);
  /**
   * @brief Destroys the non-owning dispatcher.
   */
  ~CopyFeatureArrayToElementArray() noexcept;

  CopyFeatureArrayToElementArray(const CopyFeatureArrayToElementArray&) = delete;
  CopyFeatureArrayToElementArray(CopyFeatureArrayToElementArray&&) noexcept = delete;
  CopyFeatureArrayToElementArray& operator=(const CopyFeatureArrayToElementArray&) = delete;
  CopyFeatureArrayToElementArray& operator=(CopyFeatureArrayToElementArray&&) noexcept = delete;

  /**
   * @brief Broadcasts every selected feature array.
   * @return Error from Feature Id validation or the selected implementation.
   *
   * Cancellation can retain output from completed arrays or chunks.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CopyFeatureArrayToElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
