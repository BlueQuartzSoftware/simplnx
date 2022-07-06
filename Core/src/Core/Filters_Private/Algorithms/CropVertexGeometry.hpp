#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CropVertexGeometryInputValues inputValues;

  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.XMin = filterArgs.value<float32>(k_XMin_Key);
  inputValues.YMin = filterArgs.value<float32>(k_YMin_Key);
  inputValues.ZMin = filterArgs.value<float32>(k_ZMin_Key);
  inputValues.XMax = filterArgs.value<float32>(k_XMax_Key);
  inputValues.YMax = filterArgs.value<float32>(k_YMax_Key);
  inputValues.ZMax = filterArgs.value<float32>(k_ZMax_Key);
  inputValues.CroppedDataContainerName = filterArgs.value<DataPath>(k_CroppedDataContainerName_Key);

  return CropVertexGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CropVertexGeometryInputValues
{
  DataPath DataContainerName;
  float32 XMin;
  float32 YMin;
  float32 ZMin;
  float32 XMax;
  float32 YMax;
  float32 ZMax;
  DataPath CroppedDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CropVertexGeometry
{
public:
  CropVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropVertexGeometryInputValues* inputValues);
  ~CropVertexGeometry() noexcept;

  CropVertexGeometry(const CropVertexGeometry&) = delete;
  CropVertexGeometry(CropVertexGeometry&&) noexcept = delete;
  CropVertexGeometry& operator=(const CropVertexGeometry&) = delete;
  CropVertexGeometry& operator=(CropVertexGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CropVertexGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
