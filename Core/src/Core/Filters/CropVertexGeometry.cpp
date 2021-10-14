#include "CropVertexGeometry.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
{
std::string CropVertexGeometry::name() const
{
  return FilterTraits<CropVertexGeometry>::name.str();
}

std::string CropVertexGeometry::className() const
{
  return FilterTraits<CropVertexGeometry>::className;
}

Uuid CropVertexGeometry::uuid() const
{
  return FilterTraits<CropVertexGeometry>::uuid;
}

std::string CropVertexGeometry::humanName() const
{
  return "Crop Geometry (Vertex)";
}

Parameters CropVertexGeometry::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataContainerName_Key, "Vertex Geometry to Crop", "", DataPath{}));
  params.insert(std::make_unique<Float32Parameter>(k_XMin_Key, "X Min", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_YMin_Key, "Y Min", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_ZMin_Key, "Z Min", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_XMax_Key, "X Max", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_YMax_Key, "Y Max", "", 1.23345f));
  params.insert(std::make_unique<Float32Parameter>(k_ZMax_Key, "Z Max", "", 1.23345f));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CroppedDataContainerName_Key, "Cropped Data Container", "", DataPath{}));

  return params;
}

IFilter::UniquePointer CropVertexGeometry::clone() const
{
  return std::make_unique<CropVertexGeometry>();
}

Result<OutputActions> CropVertexGeometry::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pXMinValue = filterArgs.value<float32>(k_XMin_Key);
  auto pYMinValue = filterArgs.value<float32>(k_YMin_Key);
  auto pZMinValue = filterArgs.value<float32>(k_ZMin_Key);
  auto pXMaxValue = filterArgs.value<float32>(k_XMax_Key);
  auto pYMaxValue = filterArgs.value<float32>(k_YMax_Key);
  auto pZMaxValue = filterArgs.value<float32>(k_ZMax_Key);
  auto pCroppedDataContainerNameValue = filterArgs.value<DataPath>(k_CroppedDataContainerName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<CropVertexGeometryAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> CropVertexGeometry::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pDataContainerNameValue = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pXMinValue = filterArgs.value<float32>(k_XMin_Key);
  auto pYMinValue = filterArgs.value<float32>(k_YMin_Key);
  auto pZMinValue = filterArgs.value<float32>(k_ZMin_Key);
  auto pXMaxValue = filterArgs.value<float32>(k_XMax_Key);
  auto pYMaxValue = filterArgs.value<float32>(k_YMax_Key);
  auto pZMaxValue = filterArgs.value<float32>(k_ZMax_Key);
  auto pCroppedDataContainerNameValue = filterArgs.value<DataPath>(k_CroppedDataContainerName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
