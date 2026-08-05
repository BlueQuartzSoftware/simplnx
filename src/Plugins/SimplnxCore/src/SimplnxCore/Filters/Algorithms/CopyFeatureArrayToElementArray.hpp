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

struct SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArrayInputValues
{
  StringParameter::ValueType CreatedArraySuffix;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  MultiArraySelectionParameter::ValueType SelectedFeatureArrayPaths;
};

/**
 * @class CopyFeatureArrayToElementArray
 * @brief Copies each selected Feature-level array down to the Element (cell) level: for every
 * Element i, the created array's tuple is the source array's tuple at index FeatureIds[i].
 */
class SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArray
{
public:
  CopyFeatureArrayToElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                 const CopyFeatureArrayToElementArrayInputValues* inputValues);
  ~CopyFeatureArrayToElementArray() noexcept;

  CopyFeatureArrayToElementArray(const CopyFeatureArrayToElementArray&) = delete;
  CopyFeatureArrayToElementArray(CopyFeatureArrayToElementArray&&) noexcept = delete;
  CopyFeatureArrayToElementArray& operator=(const CopyFeatureArrayToElementArray&) = delete;
  CopyFeatureArrayToElementArray& operator=(CopyFeatureArrayToElementArray&&) noexcept = delete;

  /**
   * @brief Runs the copy for every selected feature array.
   * @return Invalid Result on FeatureIds range-validation failure (-5355 / -5351).
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CopyFeatureArrayToElementArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
