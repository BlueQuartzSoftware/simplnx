#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InitializeImageGeomCellDataInputValues inputValues;
  inputValues.CellArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(cell_arrays);
  inputValues.InitRange = filterArgs.value<VectorFloat64Parameter::ValueType>(init_range);
  inputValues.InitTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(init_type_index);
  inputValues.InitValue = filterArgs.value<Float64Parameter::ValueType>(init_value);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_image_geometry_path);
  inputValues.MaxPoint = filterArgs.value<VectorUInt64Parameter::ValueType>(max_point);
  inputValues.MinPoint = filterArgs.value<VectorUInt64Parameter::ValueType>(min_point);
  inputValues.SeedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(seed_array_name);
  inputValues.SeedValue = filterArgs.value<UInt64Parameter::ValueType>(seed_value);
  inputValues.UseSeed = filterArgs.value<BoolParameter::ValueType>(use_seed);
  return InitializeImageGeomCellData(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT InitializeImageGeomCellDataInputValues
{
  MultiArraySelectionParameter::ValueType CellArrays;
  VectorFloat64Parameter::ValueType InitRange;
  ChoicesParameter::ValueType InitTypeIndex;
  Float64Parameter::ValueType InitValue;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  VectorUInt64Parameter::ValueType MaxPoint;
  VectorUInt64Parameter::ValueType MinPoint;
  DataObjectNameParameter::ValueType SeedArrayName;
  UInt64Parameter::ValueType SeedValue;
  BoolParameter::ValueType UseSeed;
};

/**
 * @class InitializeImageGeomCellData
 * @brief This algorithm implements support code for the InitializeImageGeomCellDataFilter
 */

class SIMPLNXCORE_EXPORT InitializeImageGeomCellData
{
public:
  InitializeImageGeomCellData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InitializeImageGeomCellDataInputValues* inputValues);
  ~InitializeImageGeomCellData() noexcept;

  InitializeImageGeomCellData(const InitializeImageGeomCellData&) = delete;
  InitializeImageGeomCellData(InitializeImageGeomCellData&&) noexcept = delete;
  InitializeImageGeomCellData& operator=(const InitializeImageGeomCellData&) = delete;
  InitializeImageGeomCellData& operator=(InitializeImageGeomCellData&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const InitializeImageGeomCellDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
