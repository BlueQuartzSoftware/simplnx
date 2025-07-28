#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AlignGeometriesInputValues inputValues;
  inputValues.AlignmentTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(alignment_type_index);
  inputValues.InputMovingGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_moving_geometry_path);
  inputValues.InputTargetGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_target_geometry_path);
  return AlignGeometries(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT AlignGeometriesInputValues
{
  ChoicesParameter::ValueType AlignmentTypeIndex;
  GeometrySelectionParameter::ValueType InputMovingGeometryPath;
  GeometrySelectionParameter::ValueType InputTargetGeometryPath;
};

/**
 * @class AlignGeometries
 * @brief This algorithm implements support code for the AlignGeometriesFilter
 */

class SIMPLNXCORE_EXPORT AlignGeometries
{
public:
  AlignGeometries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignGeometriesInputValues* inputValues);
  ~AlignGeometries() noexcept;

  AlignGeometries(const AlignGeometries&) = delete;
  AlignGeometries(AlignGeometries&&) noexcept = delete;
  AlignGeometries& operator=(const AlignGeometries&) = delete;
  AlignGeometries& operator=(AlignGeometries&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const AlignGeometriesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
