#include "ITKMultiScaleHessianBasedObjectnessImage.hpp"

#define ITK_BASIC_PIXEL_ID_TYPE_LIST 1
#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

#include <itkHessianToObjectnessMeasureImageFilter.h>
#include <itkMultiScaleHessianBasedMeasureImageFilter.h>

namespace
{
struct ITKMultiScaleHessianBasedObjectnessImageFilterCreationFunctor
{
  int32 m_ObjectDimension;
  float64 m_Alpha;
  float64 m_Beta;
  float64 m_Gamma;
  bool m_BrightObject;
  bool m_ScaleObjectnessMeasure;
  float64 m_SigmaMinimum;
  float64 m_SigmaMaximum;
  float64 m_NumberOfSigmaSteps;

  template <typename InputImageType, typename OutputImageType, unsigned int Dimension>
  auto createFilter() const
  {

    using InputPixelType = typename InputImageType::PixelType;
    typedef typename itk::NumericTraits<InputPixelType>::RealType RealType;
    typedef itk::SymmetricSecondRankTensor<RealType, Dimension> HessianPixelType;
    typedef itk::Image<HessianPixelType, Dimension> HessianImageType;

    typedef itk::HessianToObjectnessMeasureImageFilter<HessianImageType, OutputImageType> ObjectnessFilterType;
    typename ObjectnessFilterType::Pointer objectnessFilter = ObjectnessFilterType::New();
    objectnessFilter->SetObjectDimension(static_cast<unsigned int>(m_ObjectDimension));
    objectnessFilter->SetAlpha(static_cast<double>(m_Alpha));
    objectnessFilter->SetBeta(static_cast<double>(m_Beta));
    objectnessFilter->SetGamma(static_cast<double>(m_Gamma));
    objectnessFilter->SetBrightObject(static_cast<bool>(m_BrightObject));
    objectnessFilter->SetScaleObjectnessMeasure(static_cast<bool>(m_ScaleObjectnessMeasure));

    typedef itk::MultiScaleHessianBasedMeasureImageFilter<InputImageType, HessianImageType, OutputImageType> FilterType;

    typename FilterType::Pointer filter = FilterType::New();
    filter->SetSigmaMinimum(static_cast<double>(m_SigmaMinimum));
    filter->SetSigmaMaximum(static_cast<double>(m_SigmaMaximum));
    filter->SetNumberOfSigmaSteps(static_cast<double>(m_NumberOfSigmaSteps));
    filter->SetHessianToMeasureFilter(objectnessFilter);

    return filter;
  }
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKMultiScaleHessianBasedObjectnessImage::name() const
{
  return FilterTraits<ITKMultiScaleHessianBasedObjectnessImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKMultiScaleHessianBasedObjectnessImage::className() const
{
  return FilterTraits<ITKMultiScaleHessianBasedObjectnessImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMultiScaleHessianBasedObjectnessImage::uuid() const
{
  return FilterTraits<ITKMultiScaleHessianBasedObjectnessImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMultiScaleHessianBasedObjectnessImage::humanName() const
{
  return "ITK::Multi-scale Hessian Based Objectness Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMultiScaleHessianBasedObjectnessImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK Edge"};
}

//------------------------------------------------------------------------------
Parameters ITKMultiScaleHessianBasedObjectnessImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_ObjectDimension_Key, "ObjectDimension", "", 1234356));
  params.insert(std::make_unique<Float64Parameter>(k_Alpha_Key, "Alpha", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Beta_Key, "Beta", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_Gamma_Key, "Gamma", "", 2.3456789));
  params.insert(std::make_unique<BoolParameter>(k_BrightObject_Key, "BrightObject", "", false));
  params.insert(std::make_unique<BoolParameter>(k_ScaleObjectnessMeasure_Key, "ScaleObjectnessMeasure", "", false));
  params.insert(std::make_unique<Float64Parameter>(k_SigmaMinimum_Key, "SigmaMinimum", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_SigmaMaximum_Key, "SigmaMaximum", "", 2.3456789));
  params.insert(std::make_unique<Float64Parameter>(k_NumberOfSigmaSteps_Key, "NumberOfSigmaSteps", "", 2.3456789));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeomPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{DataObject::Type::ImageGeom}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_NewCellArrayName_Key, "Filtered Array", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMultiScaleHessianBasedObjectnessImage::clone() const
{
  return std::make_unique<ITKMultiScaleHessianBasedObjectnessImage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMultiScaleHessianBasedObjectnessImage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pObjectDimension = filterArgs.value<int32>(k_ObjectDimension_Key);
  auto pAlpha = filterArgs.value<float64>(k_Alpha_Key);
  auto pBeta = filterArgs.value<float64>(k_Beta_Key);
  auto pGamma = filterArgs.value<float64>(k_Gamma_Key);
  auto pBrightObject = filterArgs.value<bool>(k_BrightObject_Key);
  auto pScaleObjectnessMeasure = filterArgs.value<bool>(k_ScaleObjectnessMeasure_Key);
  auto pSigmaMinimum = filterArgs.value<float64>(k_SigmaMinimum_Key);
  auto pSigmaMaximum = filterArgs.value<float64>(k_SigmaMaximum_Key);
  auto pNumberOfSigmaSteps = filterArgs.value<float64>(k_NumberOfSigmaSteps_Key);
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
Result<> ITKMultiScaleHessianBasedObjectnessImage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode,
                                                               const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pObjectDimension = filterArgs.value<int32>(k_ObjectDimension_Key);
  auto pAlpha = filterArgs.value<float64>(k_Alpha_Key);
  auto pBeta = filterArgs.value<float64>(k_Beta_Key);
  auto pGamma = filterArgs.value<float64>(k_Gamma_Key);
  auto pBrightObject = filterArgs.value<bool>(k_BrightObject_Key);
  auto pScaleObjectnessMeasure = filterArgs.value<bool>(k_ScaleObjectnessMeasure_Key);
  auto pSigmaMinimum = filterArgs.value<float64>(k_SigmaMinimum_Key);
  auto pSigmaMaximum = filterArgs.value<float64>(k_SigmaMaximum_Key);
  auto pNumberOfSigmaSteps = filterArgs.value<float64>(k_NumberOfSigmaSteps_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeomPath_Key);
  auto pSelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pOutputArrayPath = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/
  ::ITKMultiScaleHessianBasedObjectnessImageFilterCreationFunctor itkFunctor{};
  itkFunctor.m_ObjectDimension = pObjectDimension;
  itkFunctor.m_Alpha = pAlpha;
  itkFunctor.m_Beta = pBeta;
  itkFunctor.m_Gamma = pGamma;
  itkFunctor.m_BrightObject = pBrightObject;
  itkFunctor.m_ScaleObjectnessMeasure = pScaleObjectnessMeasure;
  itkFunctor.m_SigmaMinimum = pSigmaMinimum;
  itkFunctor.m_SigmaMaximum = pSigmaMaximum;
  itkFunctor.m_NumberOfSigmaSteps = pNumberOfSigmaSteps;

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeomPath);
  imageGeom.getLinkedGeometryData().addCellData(pOutputArrayPath);

  return ITK::Execute(dataStructure, pSelectedCellArrayPath, pImageGeomPath, pOutputArrayPath, itkFunctor);
}
} // namespace complex
