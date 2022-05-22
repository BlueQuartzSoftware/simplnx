#include "ReadH5Ebsd.hpp"

#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/EbsdMacros.h"
#include "EbsdLib/IO/H5EbsdVolumeInfo.h"
#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/HKL/H5CtfVolumeReader.h"
#include "EbsdLib/IO/TSL/AngFields.h"
#include "EbsdLib/IO/TSL/H5AngVolumeReader.h"

namespace RotateSampleRefFrame
{
// Parameter Keys
static inline constexpr complex::StringLiteral k_RotationRepresentation_Key = "rotation_representation";
static inline constexpr complex::StringLiteral k_RotationAngle_Key = "rotation_angle";
static inline constexpr complex::StringLiteral k_RotationAxis_Key = "rotation_axis";
static inline constexpr complex::StringLiteral k_RotationMatrix_Key = "rotation_matrix";
static inline constexpr complex::StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";
static inline constexpr complex::StringLiteral k_SelectedCellArrays_Key = "selected_cell_arrays";
static inline constexpr complex::StringLiteral k_CreatedImageGeometry_Key = "created_image_geometry";

enum class RotationRepresentation : uint64_t
{
  AxisAngle = 0,
  RotationMatrix = 1
};

} // namespace RotateSampleRefFrame

namespace
{

/**
 * @brief loadInfo Reads the values for the phase type, crystal structure
 * and precipitate fractions from the EBSD file.
 * @param m_InputValues
 * @param m_DataStructure
 * @param reader EbsdReader instance pointer
 * @return
 */
template <typename EbsdReaderType, typename EbsdPhase>
complex::Result<> LoadInfo(const complex::ReadH5EbsdInputValues* m_InputValues, complex::DataStructure& m_DataStructure, std::shared_ptr<EbsdReaderType>& reader)
{
  reader->setFileName(m_InputValues->inputFilePath);
  reader->setSliceStart(m_InputValues->startSlice);
  reader->setSliceEnd(m_InputValues->endSlice);

  std::vector<typename EbsdPhase::Pointer> phases = reader->getPhases();
  if(phases.size() == 0)
  {
    return {complex::MakeErrorResult(-50027, fmt::format("Error reading phase information from file '{}'.", m_InputValues->inputFilePath))};
  }

  // Resize the Ensemble Attribute Matrix to be the correct number of phases.
  std::vector<size_t> tDims = {phases.size() + 1};

  complex::DataPath cellEnsembleMatrixPath = m_InputValues->cellEnsembleMatrixPath;

  complex::DataPath xtalDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::CrystalStructures);
  complex::Int32Array& xtalData = m_DataStructure.getDataRefAs<complex::Int32Array>(xtalDataPath);
  xtalData.getIDataStore()->reshapeTuples(tDims);

  complex::DataPath latticeDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::LatticeConstants);
  complex::Float32Array& latticData = m_DataStructure.getDataRefAs<complex::Float32Array>(latticeDataPath);
  latticData.getIDataStore()->reshapeTuples(tDims);

  // Reshape the Material Names here also.

  // Initialize the zero'th element to unknowns. The other elements will
  // be filled in based on values from the data file
  xtalData[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  // materialNames->setValue(0, QString("Invalid Phase"));
  latticData[0] = 0.0f;
  latticData[1] = 0.0f;
  latticData[2] = 0.0f;
  latticData[3] = 0.0f;
  latticData[4] = 0.0f;
  latticData[5] = 0.0f;

  for(size_t i = 0; i < phases.size(); i++)
  {
    int32_t phaseID = phases[i]->getPhaseIndex();
    xtalData[phaseID] = phases[i]->determineLaueGroup();
    // materialNames[phaseID] = phases[i]->getMaterialName();
    std::vector<float> latticeConstant = phases[i]->getLatticeConstants();

    latticData[phaseID * 6ULL] = latticeConstant[0];
    latticData[phaseID * 6ULL + 1] = latticeConstant[1];
    latticData[phaseID * 6ULL + 2] = latticeConstant[2];
    latticData[phaseID * 6ULL + 3] = latticeConstant[3];
    latticData[phaseID * 6ULL + 4] = latticeConstant[4];
    latticData[phaseID * 6ULL + 5] = latticeConstant[5];
  }

  return {};
}

/**
 * @brief LoadEbsdData
 * @param m_InputValues
 * @param m_DataStructure
 * @param eulerNames
 * @param m_MessageHandler
 * @param m_SelectedArrayNames
 * @param dcDims
 * @param floatArrays
 * @param intArrays
 * @return
 */
template <typename H5EbsdReaderType, typename PhaseType>
complex::Result<> LoadEbsdData(const complex::ReadH5EbsdInputValues* m_InputValues, complex::DataStructure& m_DataStructure, const std::vector<std::string>& eulerNames,
                               const complex::IFilter::MessageHandler& m_MessageHandler, std::set<std::string> m_SelectedArrayNames, const std::array<size_t, 3>& dcDims,
                               const std::vector<std::string>& floatArrays, const std::vector<std::string>& intArrays)
{
  int32_t err = 0;
  std::shared_ptr<H5EbsdReaderType> ebsdReader = std::dynamic_pointer_cast<H5EbsdReaderType>(H5EbsdReaderType::New());
  if(nullptr == ebsdReader)
  {
    return {complex::MakeErrorResult(-50006, fmt::format("Error instantiating H5EbsdVolumeReader"))};
  }
  ebsdReader->setFileName(m_InputValues->inputFilePath);
  complex::Result<> result = LoadInfo<H5EbsdReaderType, PhaseType>(m_InputValues, m_DataStructure, ebsdReader);
  if(result.invalid())
  {
    return result;
  }

  std::string manufacturer = ebsdReader->getManufacturer();

  if(m_SelectedArrayNames.find(EbsdLib::CellData::EulerAngles) != m_SelectedArrayNames.end())
  {
    m_SelectedArrayNames.insert(eulerNames[0]);
    m_SelectedArrayNames.insert(eulerNames[1]);
    m_SelectedArrayNames.insert(eulerNames[2]);
  }
  if(m_SelectedArrayNames.find(EbsdLib::CellData::Phases) != m_SelectedArrayNames.end())
  {
    m_SelectedArrayNames.insert(eulerNames[3]);
  }

  // Initialize all the arrays with some default values
  m_MessageHandler(complex::IFilter::ProgressMessage{complex::IFilter::Message::Type::Progress, fmt::format("Reading Ebsd Data from file {}", m_InputValues->inputFilePath)});
  uint32_t m_RefFrameZDir = ebsdReader->getStackingOrder();

  ebsdReader->setSliceStart(m_InputValues->startSlice);
  ebsdReader->setSliceEnd(m_InputValues->endSlice);
  ebsdReader->readAllArrays(false);
  ebsdReader->setArraysToRead(m_SelectedArrayNames);
  err = ebsdReader->loadData(dcDims[0], dcDims[1], dcDims[2], m_RefFrameZDir);
  if(err < 0)
  {
    return {complex::MakeErrorResult(-50003, fmt::format("Error loading data from H5Ebsd file '{}'", m_InputValues->inputFilePath))};
  }

  complex::DataPath geometryPath = m_InputValues->dataContainerPath;
  complex::DataPath cellAttributeMatrixPath = m_InputValues->cellAttributeMatrixPath;

  size_t totalPoints = dcDims[0] * dcDims[1] * dcDims[2];

  // Get the Crystal Structure data which should have already been read from the file and copied to the array
  complex::DataPath cellEnsembleMatrixPath = m_InputValues->cellEnsembleMatrixPath;
  complex::DataPath xtalDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::CrystalStructures);
  complex::Int32Array& xtalData = m_DataStructure.getDataRefAs<complex::Int32Array>(xtalDataPath);

  // Copy the Phase Values from the EBSDReader to the DataStructure
  auto* phasePtr = reinterpret_cast<int32_t*>(ebsdReader->getPointerByName(eulerNames[3]));           // get the phase data from the EbsdReader
  complex::DataPath phaseDataPath = cellAttributeMatrixPath.createChildPath(EbsdLib::H5Ebsd::Phases); // get the phase data from the DataStructure
  complex::Int32Array& phaseData = m_DataStructure.getDataRefAs<complex::Int32Array>(phaseDataPath);
  // Copy the data. Just now sure what to do here. Iterators? ::memcpy()? Out-of-Core may have to change the copy code here.
  // And this probably isn't as fast as the old ::memcpy()
  for(size_t tupleIndex = 0; tupleIndex < totalPoints; tupleIndex++)
  {
    phaseData[tupleIndex] = phasePtr[tupleIndex];
  }

  if(m_SelectedArrayNames.find(EbsdLib::CellData::EulerAngles) != m_SelectedArrayNames.end())
  {
    //  radianconversion = M_PI / 180.0;
    float* euler0 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[0]));
    float* euler1 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[1]));
    float* euler2 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[2]));
    //  std::vector<size_t> cDims = {3};
    complex::DataPath eulerDataPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CellData::EulerAngles); // get the Euler data from the DataStructure
    complex::Float32Array& eulerData = m_DataStructure.getDataRefAs<complex::Float32Array>(eulerDataPath);

    float degToRad = 1.0f;
    if(m_InputValues->eulerRepresentation != EbsdLib::AngleRepresentation::Radians && m_InputValues->useRecommendedTransform)
    {
      degToRad = complex::numbers::pi_v<float> / 180.0F;
    }
    for(size_t elementIndex = 0; elementIndex < totalPoints; elementIndex++)
    {
      eulerData[3 * elementIndex] = euler0[elementIndex] * degToRad;
      eulerData[3 * elementIndex + 1] = euler1[elementIndex] * degToRad;
      eulerData[3 * elementIndex + 2] = euler2[elementIndex] * degToRad;
    }
    // THIS IS ONLY TO BRING OXFORD DATA INTO THE SAME HEX REFERENCE AS EDAX HEX REFERENCE
    if(manufacturer == EbsdLib::Ctf::Manufacturer)
    {
      for(size_t elementIndex = 0; elementIndex < totalPoints; elementIndex++)
      {
        if(xtalData[phaseData[elementIndex]] == EbsdLib::CrystalStructure::Hexagonal_High)
        {
          eulerData[3 * elementIndex + 2] = eulerData[3 * elementIndex + 2] + (30.0F * degToRad);
        }
      }
    }
  }

  // These could probably be templated
  // Loop over the Float32 arrays
  for(const auto& arrayName : floatArrays)
  {
    // cDims[0] = 1;
    if(m_SelectedArrayNames.find(arrayName) != m_SelectedArrayNames.end())
    {
      float* source = reinterpret_cast<float*>(ebsdReader->getPointerByName(arrayName));
      complex::DataPath dataPath = cellAttributeMatrixPath.createChildPath(arrayName); // get the data from the DataStructure
      complex::Float32Array& destination = m_DataStructure.getDataRefAs<complex::Float32Array>(dataPath);
      // Copy the data. Just now sure what to do here. Iterators? ::memcpy()? Out-of-Core may have to change the copy code here.
      // And this probably isn't as fast as the old ::memcpy()
      for(size_t tupleIndex = 0; tupleIndex < totalPoints; tupleIndex++)
      {
        destination[tupleIndex] = source[tupleIndex];
      }
    }
  }

  // Loop over the Int32 arrays
  for(const auto& arrayName : intArrays)
  {
    // cDims[0] = 1;
    if(m_SelectedArrayNames.find(arrayName) != m_SelectedArrayNames.end())
    {
      int32_t* source = reinterpret_cast<int32_t*>(ebsdReader->getPointerByName(arrayName));
      complex::DataPath dataPath = cellAttributeMatrixPath.createChildPath(arrayName); // get the data from the DataStructure
      complex::Int32Array& destination = m_DataStructure.getDataRefAs<complex::Int32Array>(dataPath);
      // Copy the data. Just now sure what to do here. Iterators? ::memcpy()? Out-of-Core may have to change the copy code here.
      // And this probably isn't as fast as the old ::memcpy()
      for(size_t tupleIndex = 0; tupleIndex < totalPoints; tupleIndex++)
      {
        destination[tupleIndex] = source[tupleIndex];
      }
    }
  }

  return {};
}

} // namespace

using namespace complex;

// -----------------------------------------------------------------------------
ReadH5Ebsd::ReadH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5EbsdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadH5Ebsd::~ReadH5Ebsd() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadH5Ebsd::operator()()
{
  using FloatVec3Type = std::array<float, 3>;

  // Get the Size and Spacing of the Volume
  H5EbsdVolumeInfo::Pointer volumeInfoReader = H5EbsdVolumeInfo::New();
  volumeInfoReader->setFileName(m_InputValues->inputFilePath);
  int err = volumeInfoReader->readVolumeInfo();
  if(err < 0)
  {
    return {MakeErrorResult(-50000, fmt::format("Could not read H5EbsdVolumeInfo from file '{}", m_InputValues->inputFilePath))};
  }
  std::array<int64_t, 3> dims = {0, 0, 0};
  std::array<float, 3> res = {0.0f, 0.0f, 0.0f};
  volumeInfoReader->getDimsAndResolution(dims[0], dims[1], dims[2], res[0], res[1], res[2]);

  std::array<size_t, 3> dcDims = {static_cast<size_t>(dims[0]), static_cast<size_t>(dims[1]), static_cast<size_t>(dims[2])};

  // The dimensions/spacing/origin of the ImageGeometry should have already been set during preflight.
  complex::ImageGeom& imageGeom = m_DataStructure.getDataRefAs<complex::ImageGeom>(m_InputValues->dataContainerPath);

  // Now Calculate our "subvolume" of slices, ie, those start and end values that the user selected from the GUI
  dcDims[2] = m_InputValues->endSlice - m_InputValues->startSlice + 1;

  std::string manufacturer = volumeInfoReader->getManufacturer();
  uint32_t m_RefFrameZDir = volumeInfoReader->getStackingOrder();

  std::array<float, 3> sampleTransAxis = volumeInfoReader->getSampleTransformationAxis();
  float sampleTransAngle = volumeInfoReader->getSampleTransformationAngle();

  std::array<float, 3> eulerTransAxis = volumeInfoReader->getEulerTransformationAxis();
  float eulerTransAngle = volumeInfoReader->getEulerTransformationAngle();

  // This will effectively close the reader
  volumeInfoReader = H5EbsdVolumeInfo::NullPointer();

  // Now create the specific reader that we need.
  H5EbsdVolumeReader::Pointer ebsdReader;

  std::set<std::string> m_SelectedArrayNames;
  for(const auto& selectedArrayName : m_InputValues->hdf5DataPaths)
  {
    m_SelectedArrayNames.insert(selectedArrayName);
  }

  if(manufacturer == EbsdLib::Ang::Manufacturer)
  {
    std::vector<std::string> eulerPhaseArrays = {EbsdLib::Ang::Phi1, EbsdLib::Ang::Phi, EbsdLib::Ang::Phi2, EbsdLib::Ang::PhaseData};
    std::vector<std::string> floatArrays = {EbsdLib::Ang::ImageQuality, EbsdLib::Ang::ConfidenceIndex, EbsdLib::Ang::SEMSignal, EbsdLib::Ang::Fit, EbsdLib::Ang::XPosition, EbsdLib::Ang::YPosition};
    std::vector<std::string> intArrays = {};
    Result<> result = LoadEbsdData<H5AngVolumeReader, AngPhase>(m_InputValues, m_DataStructure, eulerPhaseArrays, m_MessageHandler, m_SelectedArrayNames, dcDims, floatArrays, intArrays);
    if(result.invalid())
    {
      return result;
    }
  }
  else if(manufacturer == EbsdLib::Ctf::Manufacturer)
  {
    std::vector<std::string> eulerPhaseArrays = {EbsdLib::Ang::Phi1, EbsdLib::Ang::Phi, EbsdLib::Ang::Phi2, EbsdLib::Ctf::Phase};
    std::vector<std::string> floatArrays = {EbsdLib::Ctf::MAD, EbsdLib::Ctf::X, EbsdLib::Ctf::Y};
    std::vector<std::string> intArrays = {EbsdLib::Ctf::Bands, EbsdLib::Ctf::Error, EbsdLib::Ctf::BC, EbsdLib::Ctf::BS};
    Result<> result = LoadEbsdData<H5CtfVolumeReader, CtfPhase>(m_InputValues, m_DataStructure, eulerPhaseArrays, m_MessageHandler, m_SelectedArrayNames, dcDims, floatArrays, intArrays);
    if(result.invalid())
    {
      return result;
    }
  }
  else
  {
    return {MakeErrorResult(-50001, fmt::format("Could not determine or match a supported manufacturer from the data file. Supported manufacturer codes are: '{}' and '{}'", EbsdLib::Ctf::Manufacturer,
                                                EbsdLib::Ang::Manufacturer))};
  }

  // Sanity Check the Error Condition or the state of the EBSD Reader Object.
  if(m_InputValues->useRecommendedTransform)
  {

    if(eulerTransAngle > 0)
    {
      RotateEulerRefFrameFilter rot_Euler;
      Arguments args;
      args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAngle_Key, std::make_any<Float32Parameter::ValueType>(eulerTransAngle));
      args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxis_Key,
                          std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{eulerTransAxis[0], eulerTransAxis[1], eulerTransAxis[2]}));

      complex::DataPath eulerDataPath = m_InputValues->cellAttributeMatrixPath.createChildPath(EbsdLib::CellData::EulerAngles); // get the Euler data from the DataStructure
      args.insertOrAssign(RotateEulerRefFrameFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(eulerDataPath));
      // Preflight the filter and check result
      auto preflightResult = rot_Euler.preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
      }

      // Execute the filter and check the result
      auto executeResult = rot_Euler.execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
    }

    if(sampleTransAngle > 0)
    {
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Warning, "Sample Reference Frame Rotation NOT applied."});

#if 0

      const Uuid k_CorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
      const Uuid k_RotateSampleRefFrameFilterId = *Uuid::FromString("5efdf395-33fb-4dc0-986e-0dc0ae990f6a");
      const FilterHandle k_RotateSampleRefFrameFilterHandle(k_RotateSampleRefFrameFilterId, k_CorePluginId);

      auto* filterList = Application::Instance()->getFilterList();
      auto filter = filterList->createFilter(k_RotateSampleRefFrameFilterHandle);
      if(nullptr == filter)
      {
        return {MakeErrorResult(-50010, fmt::format("Error creating RotateSampleRefFrame filter"))};
      }
      Arguments args;

      args.insertOrAssign(RotateSampleRefFrame::k_RotationRepresentation_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::AxisAngle)));
      args.insertOrAssign(RotateSampleRefFrame::k_SelectedImageGeometry_Key, std::make_any<DataPath>(m_InputValues->dataContainerPath));
      DataPath rotatedGeometryDataPath({".RotatedGeometry"});
      args.insertOrAssign(RotateSampleRefFrame::k_CreatedImageGeometry_Key, std::make_any<DataPath>(rotatedGeometryDataPath));

      std::vector<DataPath> rotatedDataPaths;
      for(const auto& path : m_InputValues->hdf5DataPaths)
      {
        rotatedDataPaths.push_back(m_InputValues->cellAttributeMatrixPath.createChildPath(path));
      }
      args.insertOrAssign(RotateSampleRefFrame::k_SelectedCellArrays_Key, std::make_any<std::vector<DataPath>>(rotatedDataPaths));

      args.insertOrAssign(RotateSampleRefFrame::k_RotationAngle_Key, std::make_any<Float32Parameter::ValueType>(sampleTransAngle));
      args.insertOrAssign(RotateSampleRefFrame::k_RotationAxis_Key, std::make_any<VectorFloat32Parameter::ValueType>({sampleTransAxis[0], sampleTransAxis[1], sampleTransAxis[2]}));

      // Preflight the filter and check result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, "Preflighting Rotate Sample Ref Frame..."});
      complex::IFilter::PreflightResult preflightResult = filter->preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        return {MakeErrorResult(-50010, fmt::format("Error creating RotateSampleRefFrame filter"))};

        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
      }

      // Execute the filter and check the result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, "Executing Rotate Sample Ref Frame..."});
      auto executeResult = filter->execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
      }
      /********************    *************************************/
      // Delete the original DataContainer
      const Uuid k_DeleteDataFilterId = *Uuid::FromString("bf286740-e987-49fe-a7c8-6e566e3a0606");
      const FilterHandle k_DeleteDataFilterHandle(k_DeleteDataFilterId, k_CorePluginId);
      filter = filterList->createFilter(k_DeleteDataFilterHandle);
      if(nullptr == filter)
      {
        return {MakeErrorResult(-50010, fmt::format("Error creating 'Delete Data Object' filter"))};
      }
      // Preflight the filter and check result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, "Preflighting Delete Data Object Filter ..."});
      preflightResult = filter->preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
      }

      // Execute the filter and check the result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, "Executing Delete Data Object Filter..."});
      executeResult = filter->execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
      }

      /********************    *************************************/
      // Rename the rotated back to the original
      const Uuid k_RenameDataObjectFilterId = *Uuid::FromString("d53c808f-004d-5fac-b125-0fffc8cc78d6");
      const FilterHandle k_RenameDataObjectFilterHandle(k_RenameDataObjectFilterId, k_CorePluginId);
      filter = filterList->createFilter(k_RenameDataObjectFilterHandle);
      if(nullptr == filter)
      {
        return {MakeErrorResult(-50010, fmt::format("Error creating 'Rename Data Object' filter"))};
      }
      // Preflight the filter and check result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, "Preflighting Rename Data Object Filter ..."});
      preflightResult = filter->preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
      }

      // Execute the filter and check the result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, "Executing Rename Data Object Filter..."});
      executeResult = filter->execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
      }
#endif
    }
  }

  return {};
}
