#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ResampleRectGridToImageGeomInputValues inputValues;

  inputValues.RectilinearGridPath = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.RectGridGeometryDesc = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RectGridGeometryDesc_Key);
  inputValues.Dimensions = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  inputValues.CreatedGeometryDescription = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_CreatedGeometryDescription_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.ImageGeomCellAttributeMatrix = filterArgs.value<DataPath>(k_ImageGeomCellAttributeMatrix_Key);

  return ResampleRectGridToImageGeom(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT ResampleRectGridToImageGeomInputValues
{
  DataPath RectilinearGridPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  <<<NOT_IMPLEMENTED>>> RectGridGeometryDesc;
  VectorInt32Parameter::ValueType Dimensions;
  <<<NOT_IMPLEMENTED>>> CreatedGeometryDescription;
  DataPath ImageGeometryPath;
  DataPath ImageGeomCellAttributeMatrix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT ResampleRectGridToImageGeom
{
public:
  ResampleRectGridToImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ResampleRectGridToImageGeomInputValues* inputValues);
  ~ResampleRectGridToImageGeom() noexcept;

  ResampleRectGridToImageGeom(const ResampleRectGridToImageGeom&) = delete;
  ResampleRectGridToImageGeom(ResampleRectGridToImageGeom&&) noexcept = delete;
  ResampleRectGridToImageGeom& operator=(const ResampleRectGridToImageGeom&) = delete;
  ResampleRectGridToImageGeom& operator=(ResampleRectGridToImageGeom&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ResampleRectGridToImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
