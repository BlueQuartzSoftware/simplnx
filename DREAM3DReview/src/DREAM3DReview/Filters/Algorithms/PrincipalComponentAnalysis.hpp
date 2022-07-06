#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  PrincipalComponentAnalysisInputValues inputValues;

  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.MatrixApproach = filterArgs.value<ChoicesParameter::ValueType>(k_MatrixApproach_Key);
  inputValues.ProjectDataSpace = filterArgs.value<bool>(k_ProjectDataSpace_Key);
  inputValues.NumberOfDimensionsForProjection = filterArgs.value<int32>(k_NumberOfDimensionsForProjection_Key);
  inputValues.ProjectedDataSpaceArrayPath = filterArgs.value<DataPath>(k_ProjectedDataSpaceArrayPath_Key);
  inputValues.PCAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_PCAttributeMatrixName_Key);
  inputValues.PCEigenvaluesName = filterArgs.value<StringParameter::ValueType>(k_PCEigenvaluesName_Key);
  inputValues.PCEigenvectorsName = filterArgs.value<StringParameter::ValueType>(k_PCEigenvectorsName_Key);

  return PrincipalComponentAnalysis(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT PrincipalComponentAnalysisInputValues
{
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  ChoicesParameter::ValueType MatrixApproach;
  bool ProjectDataSpace;
  int32 NumberOfDimensionsForProjection;
  DataPath ProjectedDataSpaceArrayPath;
  StringParameter::ValueType PCAttributeMatrixName;
  StringParameter::ValueType PCEigenvaluesName;
  StringParameter::ValueType PCEigenvectorsName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT PrincipalComponentAnalysis
{
public:
  PrincipalComponentAnalysis(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PrincipalComponentAnalysisInputValues* inputValues);
  ~PrincipalComponentAnalysis() noexcept;

  PrincipalComponentAnalysis(const PrincipalComponentAnalysis&) = delete;
  PrincipalComponentAnalysis(PrincipalComponentAnalysis&&) noexcept = delete;
  PrincipalComponentAnalysis& operator=(const PrincipalComponentAnalysis&) = delete;
  PrincipalComponentAnalysis& operator=(PrincipalComponentAnalysis&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PrincipalComponentAnalysisInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
