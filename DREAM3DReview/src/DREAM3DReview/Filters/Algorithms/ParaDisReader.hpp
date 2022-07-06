#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ParaDisReaderInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.BurgersVector = filterArgs.value<float32>(k_BurgersVector_Key);
  inputValues.EdgeDataContainerName = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  inputValues.EdgeAttributeMatrixName = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  inputValues.NumberOfArmsArrayName = filterArgs.value<DataPath>(k_NumberOfArmsArrayName_Key);
  inputValues.NodeConstraintsArrayName = filterArgs.value<DataPath>(k_NodeConstraintsArrayName_Key);
  inputValues.BurgersVectorsArrayName = filterArgs.value<DataPath>(k_BurgersVectorsArrayName_Key);
  inputValues.SlipPlaneNormalsArrayName = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayName_Key);

  return ParaDisReader(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ParaDisReaderInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  float32 BurgersVector;
  DataPath EdgeDataContainerName;
  DataPath VertexAttributeMatrixName;
  DataPath EdgeAttributeMatrixName;
  DataPath NumberOfArmsArrayName;
  DataPath NodeConstraintsArrayName;
  DataPath BurgersVectorsArrayName;
  DataPath SlipPlaneNormalsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ParaDisReader
{
public:
  ParaDisReader(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ParaDisReaderInputValues* inputValues);
  ~ParaDisReader() noexcept;

  ParaDisReader(const ParaDisReader&) = delete;
  ParaDisReader(ParaDisReader&&) noexcept = delete;
  ParaDisReader& operator=(const ParaDisReader&) = delete;
  ParaDisReader& operator=(ParaDisReader&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ParaDisReaderInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
