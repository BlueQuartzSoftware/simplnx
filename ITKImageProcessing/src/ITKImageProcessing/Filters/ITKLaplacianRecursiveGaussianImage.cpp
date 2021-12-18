#include "ITKLaplacianRecursiveGaussianImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

using namespace complex;

#include <itkLaplacianRecursiveGaussianImageFilter.h>

namespace
{
struct ITKLaplacianRecursiveGaussianImageFilterCreationFunctor
{
  float64 m_Sigma;
  bool m_NormalizeAcrossScale;
  template <typename InputImageType, typename OutputImageType, unsigned int Dimension>
  auto operator()() const
  {
    typedef itk::LaplacianRecursiveGaussianImageFilter<InputImageType, OutputImageType> FilterType;
    typename FilterType::Pointer filter = FilterType::New();
    filter->SetSigma(static_cast<double>(m_Sigma));
    filter->SetNormalizeAcrossScale(static_cast<bool>(m_NormalizeAcrossScale));
    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImage::name() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImage::className() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKLaplacianRecursiveGaussianImage::uuid() const
{
  return FilterTraits<ITKLaplacianRecursiveGaussianImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKLaplacianRecursiveGaussianImage::humanName() const
{
  return "ITK::Laplacian Recursive Gaussian Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKLaplacianRecursiveGaussianImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Smoothing"};
}

//------------------------------------------------------------------------------
Parameters ITKLaplacianRecursiveGaussianImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_Sigma_Key, "Sigma", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeAcrossScale_Key, "NormalizeAcrossScale", "", false));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Filtered Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKLaplacianRecursiveGaussianImage::clone() const
{
  return std::make_unique<ITKLaplacianRecursiveGaussianImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKLaplacianRecursiveGaussianImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pSigma = filterArgs.value<float64>(k_Sigma_Key);
  auto pNormalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;
  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
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
Result<> ITKLaplacianRecursiveGaussianImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pSigma = filterArgs.value<float64>(k_Sigma_Key);
  auto pNormalizeAcrossScale = filterArgs.value<bool>(k_NormalizeAcrossScale_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKLaplacianRecursiveGaussianImageFilterCreationFunctor itkFunctor;
  itkFunctor.m_Sigma = pSigma;
  itkFunctor.m_NormalizeAcrossScale = pNormalizeAcrossScale;

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
