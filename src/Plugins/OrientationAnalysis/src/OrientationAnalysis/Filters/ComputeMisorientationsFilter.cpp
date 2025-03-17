#include "ComputeMisorientationsFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/ComputeMisorientations.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{

// Error Code constants
constexpr nx::core::int32 k_InputRepresentationTypeError = -68001;
constexpr nx::core::int32 k_OutputRepresentationTypeError = -68002;
constexpr nx::core::int32 k_InputComponentDimensionError = -68003;
constexpr nx::core::int32 k_InputComponentCountError = -68004;
constexpr nx::core::int32 k_InconsistentTupleCount = -68063;
constexpr nx::core::int32 k_OutputFilePathEmpty = -68063;

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeMisorientationsFilter::name() const
{
  return FilterTraits<ComputeMisorientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeMisorientationsFilter::className() const
{
  return FilterTraits<ComputeMisorientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeMisorientationsFilter::uuid() const
{
  return FilterTraits<ComputeMisorientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeMisorientationsFilter::humanName() const
{
  return "Compute Misorientation";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeMisorientationsFilter::defaultTags() const
{
  return {className(), "Misorientation", "Eulers"};
}

//------------------------------------------------------------------------------
Parameters ComputeMisorientationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_ComputationType_Key, "Computation Type", "The type of computation to perform",
                                                                    compute_misorientations_constants::k_UseArraysIndex, compute_misorientations_constants::k_ComputationTypeStrings));

  params.insert(std::make_unique<VectorFloat32Parameter>(k_ReferenceOrientation_Key, "Reference Axis-Angle", "<xyz> w (w in degrees)", std::vector<float32>{0.0f, 0.0f, 1.0f, 0.0f},
                                                         std::vector<std::string>{"x", "y", "z", "w (Deg)"}));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputOrientationArrayPath1_Key, "Euler Angles 1", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputOrientationArrayPath2_Key, "Euler Angles 2", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_PhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputMisorientationArrayName_Key, "Output Misorientations",
                                                          "The DataPath to the created output array that will hold the computed misorientations as Axis-Angles", "Misorientations"));

  params.linkParameters(k_ComputationType_Key, k_InputOrientationArrayPath2_Key, std::make_any<ChoicesParameter::ValueType>(compute_misorientations_constants::k_UseArraysIndex));
  params.linkParameters(k_ComputationType_Key, k_ReferenceOrientation_Key, std::make_any<ChoicesParameter::ValueType>(compute_misorientations_constants::k_UseReferenceAxesIndex));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeMisorientationsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeMisorientationsFilter::clone() const
{
  return std::make_unique<ComputeMisorientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputPath1 = filterArgs.value<DataPath>(k_InputOrientationArrayPath1_Key);
  auto inputPath2 = filterArgs.value<DataPath>(k_InputOrientationArrayPath2_Key);
  auto pReferenceDirValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ReferenceOrientation_Key);
  auto computationType = filterArgs.value<ChoicesParameter::ValueType>(k_ComputationType_Key);
  auto outputMisorientationPath = inputPath1.replaceName(filterArgs.value<std::string>(k_OutputMisorientationArrayName_Key));

  if(computationType == compute_misorientations_constants::k_UseArraysIndex)
  {
    std::vector<DataPath> dataPaths;

    dataPaths.push_back(inputPath1);

    dataPaths.push_back(inputPath2);

    auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
    if(!tupleValidityCheck)
    {
      return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
    }
  }

  // Get the number of tuples
  auto* eulersArray = dataStructure.getDataAs<Float32Array>(inputPath1);

  // Create output DataStructure Items
  auto createDataAction = std::make_unique<CreateArrayAction>(DataType::float32, eulersArray->getIDataStore()->getTupleShape(), std::vector<usize>{4}, outputMisorientationPath);

  OutputActions actions;
  actions.appendAction(std::move(createDataAction));

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ComputeMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeMisorientationsInputValues inputValues;

  inputValues.InputOrientationPath1 = filterArgs.value<DataPath>(k_InputOrientationArrayPath1_Key);
  inputValues.InputOrientationPath2 = filterArgs.value<DataPath>(k_InputOrientationArrayPath2_Key);
  inputValues.ComputationType = filterArgs.value<ChoicesParameter::ValueType>(k_ComputationType_Key);
  inputValues.ReferenceOrientation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ReferenceOrientation_Key);
  inputValues.OutputMisorientationsPath = inputValues.InputOrientationPath1.replaceName(filterArgs.value<std::string>(k_OutputMisorientationArrayName_Key));
  inputValues.InputPhasesArrayPath = filterArgs.value<DataPath>(k_PhasesArrayPath_Key);
  inputValues.InputCrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return ComputeMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

} // namespace nx::core
