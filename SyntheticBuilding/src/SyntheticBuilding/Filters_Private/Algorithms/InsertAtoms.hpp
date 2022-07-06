#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  InsertAtomsInputValues inputValues;

  inputValues.LatticeConstants = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LatticeConstants_Key);
  inputValues.Basis = filterArgs.value<ChoicesParameter::ValueType>(k_Basis_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.VertexDataContainerName = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  inputValues.AtomFeatureLabelsArrayName = filterArgs.value<DataPath>(k_AtomFeatureLabelsArrayName_Key);

  return InsertAtoms(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT InsertAtomsInputValues
{
  VectorFloat32Parameter::ValueType LatticeConstants;
  ChoicesParameter::ValueType Basis;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath VertexDataContainerName;
  DataPath VertexAttributeMatrixName;
  DataPath AtomFeatureLabelsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT InsertAtoms
{
public:
  InsertAtoms(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, InsertAtomsInputValues* inputValues);
  ~InsertAtoms() noexcept;

  InsertAtoms(const InsertAtoms&) = delete;
  InsertAtoms(InsertAtoms&&) noexcept = delete;
  InsertAtoms& operator=(const InsertAtoms&) = delete;
  InsertAtoms& operator=(InsertAtoms&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const InsertAtomsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
