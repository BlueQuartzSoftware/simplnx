#include "ITKStitchMontage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/MontageSelectionFilterParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkStitchMontageFilter.h>

namespace
{
struct ITKStitchMontageFilterCreationFunctor
{
  <<<NOT_IMPLEMENTED>>> m_MontageSelection;
  StringParameter::ValueType m_CommonAttributeMatrixName;
  StringParameter::ValueType m_CommonDataArrayName;
  StringParameter::ValueType m_MontageDataContainerName;
  StringParameter::ValueType m_MontageAttributeMatrixName;
  StringParameter::ValueType m_MontageDataArrayName;
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKStitchMontage::name() const
{
  return FilterTraits<ITKStitchMontage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKStitchMontage::className() const
{
  return FilterTraits<ITKStitchMontage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKStitchMontage::uuid() const
{
  return FilterTraits<ITKStitchMontage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKStitchMontage::humanName() const
{
  return "ITK Stitch Montage";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKStitchMontage::defaultTags() const
{
  return {"IO", "Generation"};
}

//------------------------------------------------------------------------------
Parameters ITKStitchMontage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<MontageSelectionFilterParameter>(k_MontageSelection_Key, "Montage Selection", "", {}));
  params.insert(std::make_unique<StringParameter>(k_CommonAttributeMatrixName_Key, "Common Attribute Matrix", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_CommonDataArrayName_Key, "Common Data Array", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MontageDataContainerName_Key, "Montage Data Container Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MontageAttributeMatrixName_Key, "Montage Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<StringParameter>(k_MontageDataArrayName_Key, "Montage Data Array Name", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKStitchMontage::clone() const
{
  return std::make_unique<ITKStitchMontage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKStitchMontage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pMontageSelection = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  auto pCommonAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayName = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);
  auto pMontageDataContainerName = filterArgs.value<StringParameter::ValueType>(k_MontageDataContainerName_Key);
  auto pMontageAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_MontageAttributeMatrixName_Key);
  auto pMontageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_MontageDataArrayName_Key);

  PreflightResult preflightResult;
  std::vector<PreflightValue> preflightUpdatedValues;

  complex::Result<OutputActions> resultOutputActions;

  resultOutputActions = ITK::DataCheck(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath);

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKStitchMontage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMontageSelection = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_MontageSelection_Key);
  auto pCommonAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  auto pCommonDataArrayName = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);
  auto pMontageDataContainerName = filterArgs.value<StringParameter::ValueType>(k_MontageDataContainerName_Key);
  auto pMontageAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_MontageAttributeMatrixName_Key);
  auto pMontageDataArrayName = filterArgs.value<StringParameter::ValueType>(k_MontageDataArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKStitchMontageFilterCreationFunctor itkFunctor;
  itkFunctor.m_MontageSelection = pMontageSelection;
  itkFunctor.m_CommonAttributeMatrixName = pCommonAttributeMatrixName;
  itkFunctor.m_CommonDataArrayName = pCommonDataArrayName;
  itkFunctor.m_MontageDataContainerName = pMontageDataContainerName;
  itkFunctor.m_MontageAttributeMatrixName = pMontageAttributeMatrixName;
  itkFunctor.m_MontageDataArrayName = pMontageDataArrayName;

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(pOutputArrayPath);

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
