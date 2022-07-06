#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkStitchImagesInputValues inputValues;

  inputValues.AttributeMatrixName = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  inputValues.StitchedCoordinatesArrayPath = filterArgs.value<DataPath>(k_StitchedCoordinatesArrayPath_Key);
  inputValues.AttributeArrayNamesPath = filterArgs.value<DataPath>(k_AttributeArrayNamesPath_Key);
  inputValues.StitchedVolumeDataContainerName = filterArgs.value<DataPath>(k_StitchedVolumeDataContainerName_Key);
  inputValues.StitchedAttributeMatrixName = filterArgs.value<DataPath>(k_StitchedAttributeMatrixName_Key);
  inputValues.StitchedImagesArrayName = filterArgs.value<DataPath>(k_StitchedImagesArrayName_Key);

  return ItkStitchImages(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkStitchImagesInputValues
{
  DataPath AttributeMatrixName;
  DataPath StitchedCoordinatesArrayPath;
  DataPath AttributeArrayNamesPath;
  DataPath StitchedVolumeDataContainerName;
  DataPath StitchedAttributeMatrixName;
  DataPath StitchedImagesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkStitchImages
{
public:
  ItkStitchImages(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkStitchImagesInputValues* inputValues);
  ~ItkStitchImages() noexcept;

  ItkStitchImages(const ItkStitchImages&) = delete;
  ItkStitchImages(ItkStitchImages&&) noexcept = delete;
  ItkStitchImages& operator=(const ItkStitchImages&) = delete;
  ItkStitchImages& operator=(ItkStitchImages&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkStitchImagesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
