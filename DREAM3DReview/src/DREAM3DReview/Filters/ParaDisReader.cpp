#include "ParaDisReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string ParaDisReader::name() const
{
  return FilterTraits<ParaDisReader>::name.str();
}

std::string ParaDisReader::className() const
{
  return FilterTraits<ParaDisReader>::className;
}

Uuid ParaDisReader::uuid() const
{
  return FilterTraits<ParaDisReader>::uuid;
}

std::string ParaDisReader::humanName() const
{
  return "Import ParaDis File";
}

Parameters ParaDisReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<Float32Parameter>(k_BurgersVector_Key, "Burgers Vector Length (Angstroms)", "", 1.23345f));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_EdgeDataContainerName_Key, "Edge DataContainer Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_VertexAttributeMatrixName_Key, "Vertex AttributeMatrix Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_EdgeAttributeMatrixName_Key, "Edge AttributeMatrix Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NumberOfArmsArrayName_Key, "Number Of Arms Array Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NodeConstraintsArrayName_Key, "Node Constraints Array Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_BurgersVectorsArrayName_Key, "Burgers Vectors Array Name", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_SlipPlaneNormalsArrayName_Key, "Slip Plane Normals Array Name", "", DataPath{}));

  return params;
}

IFilter::UniquePointer ParaDisReader::clone() const
{
  return std::make_unique<ParaDisReader>();
}

Result<OutputActions> ParaDisReader::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pBurgersVectorValue = filterArgs.value<float32>(k_BurgersVector_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pNumberOfArmsArrayNameValue = filterArgs.value<DataPath>(k_NumberOfArmsArrayName_Key);
  auto pNodeConstraintsArrayNameValue = filterArgs.value<DataPath>(k_NodeConstraintsArrayName_Key);
  auto pBurgersVectorsArrayNameValue = filterArgs.value<DataPath>(k_BurgersVectorsArrayName_Key);
  auto pSlipPlaneNormalsArrayNameValue = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ParaDisReaderAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> ParaDisReader::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pBurgersVectorValue = filterArgs.value<float32>(k_BurgersVector_Key);
  auto pEdgeDataContainerNameValue = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  auto pEdgeAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeAttributeMatrixName_Key);
  auto pNumberOfArmsArrayNameValue = filterArgs.value<DataPath>(k_NumberOfArmsArrayName_Key);
  auto pNodeConstraintsArrayNameValue = filterArgs.value<DataPath>(k_NodeConstraintsArrayName_Key);
  auto pBurgersVectorsArrayNameValue = filterArgs.value<DataPath>(k_BurgersVectorsArrayName_Key);
  auto pSlipPlaneNormalsArrayNameValue = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
