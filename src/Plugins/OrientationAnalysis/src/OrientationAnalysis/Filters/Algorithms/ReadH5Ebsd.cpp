#include "ReadH5Ebsd.hpp"

#include "OrientationAnalysis/Filters/RotateEulerRefFrameFilter.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/Common/StringLiteral.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
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
static inline constexpr complex::StringLiteral k_RotationAxisAngle_Key = "rotation_axis";
static inline constexpr complex::StringLiteral k_RotationMatrix_Key = "rotation_matrix";
static inline constexpr complex::StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";
static inline constexpr complex::StringLiteral k_CreatedImageGeometry_Key = "created_image_geometry";
static inline constexpr complex::StringLiteral k_RotateSliceBySlice_Key = "rotate_slice_by_slice";
static inline constexpr complex::StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";

// static inline constexpr complex::StringLiteral k_RotatedGeometryName = ".RotatedGeometry";

enum class RotationRepresentation : uint64_t
{
  AxisAngle = 0,
  RotationMatrix = 1
};

} // namespace RotateSampleRefFrame

namespace RenameDataObject
{
static inline constexpr complex::StringLiteral k_DataObject_Key = "data_object";
static inline constexpr complex::StringLiteral k_NewName_Key = "new_name";
} // namespace RenameDataObject

namespace DeleteData
{
static inline constexpr complex::StringLiteral k_DataPath_Key = "removed_data_path";

}

namespace
{

/**
 * @brief loadInfo Reads the values for the phase type, crystal structure
 * and precipitate fractions from the EBSD file.
 * @param mInputValues
 * @param mDataStructure
 * @param reader EbsdReader instance pointer
 * @return
 */
template <typename EbsdReaderType, typename EbsdPhase>
complex::Result<> LoadInfo(const complex::ReadH5EbsdInputValues* mInputValues, complex::DataStructure& mDataStructure, std::shared_ptr<EbsdReaderType>& reader)
{
  reader->setFileName(mInputValues->inputFilePath);
  reader->setSliceStart(mInputValues->startSlice);
  reader->setSliceEnd(mInputValues->endSlice);

  std::vector<typename EbsdPhase::Pointer> phases = reader->getPhases();
  if(phases.size() == 0)
  {
    return {complex::MakeErrorResult(-50027, fmt::format("Error reading phase information from file '{}'.", mInputValues->inputFilePath))};
  }

  // Resize the Ensemble Attribute Matrix to be the correct number of phases.
  std::vector<size_t> tDims = {phases.size() + 1};

  complex::DataPath cellEnsembleMatrixPath = mInputValues->cellEnsembleMatrixPath;

  complex::DataPath xtalDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::CrystalStructures);
  complex::UInt32Array& xtalData = mDataStructure.getDataRefAs<complex::UInt32Array>(xtalDataPath);
  xtalData.getIDataStore()->resizeTuples(tDims);

  complex::DataPath latticeDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::LatticeConstants);
  complex::Float32Array& latticData = mDataStructure.getDataRefAs<complex::Float32Array>(latticeDataPath);
  latticData.getIDataStore()->resizeTuples(tDims);

  // Reshape the Material Names here also.
  complex::DataPath matNamesDataath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::MaterialName);
  complex::StringArray& matNameData = mDataStructure.getDataRefAs<complex::StringArray>(matNamesDataath);
  matNameData.resizeTuples(tDims);

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
    matNameData[phaseID] = phases[i]->getMaterialName();
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

template <typename H5EbsdReaderType, typename T>
void CopyData(complex::DataStructure& dataStructure, H5EbsdReaderType* ebsdReader, const std::vector<std::string>& arrayNames, std::set<std::string> selectedArrayNames,
              complex::DataPath cellAttributeMatrixPath, size_t totalPoints)
{
  using DataArrayType = complex::DataArray<T>;
  for(const auto& arrayName : arrayNames)
  {
    if(selectedArrayNames.find(arrayName) != selectedArrayNames.end())
    {
      T* source = reinterpret_cast<T*>(ebsdReader->getPointerByName(arrayName));
      complex::DataPath dataPath = cellAttributeMatrixPath.createChildPath(arrayName); // get the data from the DataStructure
      DataArrayType& destination = dataStructure.getDataRefAs<DataArrayType>(dataPath);
      for(size_t tupleIndex = 0; tupleIndex < totalPoints; tupleIndex++)
      {
        destination[tupleIndex] = source[tupleIndex];
      }
    }
  }
}

/**
 * @brief LoadEbsdData
 * @param mInputValues
 * @param dataStructure
 * @param eulerNames
 * @param mMessageHandler
 * @param selectedArrayNames
 * @param dcDims
 * @param floatArrays
 * @param intArrays
 * @return
 */
template <typename H5EbsdReaderType, typename PhaseType>
complex::Result<> LoadEbsdData(const complex::ReadH5EbsdInputValues* mInputValues, complex::DataStructure& dataStructure, const std::vector<std::string>& eulerNames,
                               const complex::IFilter::MessageHandler& mMessageHandler, std::set<std::string> selectedArrayNames, const std::array<size_t, 3>& dcDims,
                               const std::vector<std::string>& floatArrayNames, const std::vector<std::string>& intArrayNames)
{
  int32_t err = 0;
  std::shared_ptr<H5EbsdReaderType> ebsdReader = std::dynamic_pointer_cast<H5EbsdReaderType>(H5EbsdReaderType::New());
  if(nullptr == ebsdReader)
  {
    return {complex::MakeErrorResult(-50006, fmt::format("Error instantiating H5EbsdVolumeReader"))};
  }
  ebsdReader->setFileName(mInputValues->inputFilePath);
  complex::Result<> result = LoadInfo<H5EbsdReaderType, PhaseType>(mInputValues, dataStructure, ebsdReader);
  if(result.invalid())
  {
    return result;
  }

  std::string manufacturer = ebsdReader->getManufacturer();

  if(selectedArrayNames.find(EbsdLib::CellData::EulerAngles) != selectedArrayNames.end())
  {
    selectedArrayNames.insert(eulerNames[0]);
    selectedArrayNames.insert(eulerNames[1]);
    selectedArrayNames.insert(eulerNames[2]);
  }
  if(selectedArrayNames.find(EbsdLib::CellData::Phases) != selectedArrayNames.end())
  {
    selectedArrayNames.insert(eulerNames[3]);
  }

  // Initialize all the arrays with some default values
  mMessageHandler(complex::IFilter::Message{complex::IFilter::Message::Type::Info, fmt::format("Reading EBSD Data from file {}", mInputValues->inputFilePath)});
  uint32_t mRefFrameZDir = ebsdReader->getStackingOrder();

  ebsdReader->setSliceStart(mInputValues->startSlice);
  ebsdReader->setSliceEnd(mInputValues->endSlice);
  ebsdReader->readAllArrays(false);
  ebsdReader->setArraysToRead(selectedArrayNames);
  err = ebsdReader->loadData(dcDims[0], dcDims[1], dcDims[2], mRefFrameZDir);
  if(err < 0)
  {
    return {complex::MakeErrorResult(-50003, fmt::format("Error loading data from H5Ebsd file '{}'", mInputValues->inputFilePath))};
  }

  complex::DataPath geometryPath = mInputValues->dataContainerPath;
  complex::DataPath cellAttributeMatrixPath = mInputValues->cellAttributeMatrixPath;

  size_t totalPoints = dcDims[0] * dcDims[1] * dcDims[2];

  // Get the Crystal Structure data which should have already been read from the file and copied to the array
  complex::DataPath cellEnsembleMatrixPath = mInputValues->cellEnsembleMatrixPath;
  complex::DataPath xtalDataPath = cellEnsembleMatrixPath.createChildPath(EbsdLib::EnsembleData::CrystalStructures);
  complex::UInt32Array& xtalData = dataStructure.getDataRefAs<complex::UInt32Array>(xtalDataPath);

  // Copy the Phase Values from the EBSDReader to the DataStructure
  auto* phasePtr = reinterpret_cast<int32_t*>(ebsdReader->getPointerByName(eulerNames[3]));           // get the phase data from the EbsdReader
  complex::DataPath phaseDataPath = cellAttributeMatrixPath.createChildPath(EbsdLib::H5Ebsd::Phases); // get the phase data from the DataStructure
  complex::Int32Array* phaseDataArrayPtr = nullptr;

  if(selectedArrayNames.find(eulerNames[3]) != selectedArrayNames.end())
  {
    phaseDataArrayPtr = dataStructure.getDataAs<complex::Int32Array>(phaseDataPath);
    for(size_t tupleIndex = 0; tupleIndex < totalPoints; tupleIndex++)
    {
      (*phaseDataArrayPtr)[tupleIndex] = phasePtr[tupleIndex];
    }
  }

  if(selectedArrayNames.find(EbsdLib::CellData::EulerAngles) != selectedArrayNames.end())
  {
    //  radian conversion = M_PI / 180.0;
    auto* euler0 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[0]));
    auto* euler1 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[1]));
    auto* euler2 = reinterpret_cast<float*>(ebsdReader->getPointerByName(eulerNames[2]));
    //  std::vector<size_t> cDims = {3};
    complex::DataPath eulerDataPath = cellAttributeMatrixPath.createChildPath(EbsdLib::CellData::EulerAngles); // get the Euler data from the DataStructure
    auto& eulerData = dataStructure.getDataRefAs<complex::Float32Array>(eulerDataPath);

    float degToRad = 1.0f;
    if(mInputValues->eulerRepresentation != EbsdLib::AngleRepresentation::Radians && mInputValues->useRecommendedTransform)
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
    if(manufacturer == EbsdLib::Ctf::Manufacturer && phaseDataArrayPtr != nullptr)
    {
      for(size_t elementIndex = 0; elementIndex < totalPoints; elementIndex++)
      {
        if(xtalData[(*phaseDataArrayPtr)[elementIndex]] == EbsdLib::CrystalStructure::Hexagonal_High)
        {
          eulerData[3 * elementIndex + 2] = eulerData[3 * elementIndex + 2] + (30.0F * degToRad);
        }
      }
    }
  }

  // Copy the EBSD Data from its temp location into the final DataStructure location.
  ::CopyData<H5EbsdReaderType, float>(dataStructure, ebsdReader.get(), floatArrayNames, selectedArrayNames, cellAttributeMatrixPath, totalPoints);
  ::CopyData<H5EbsdReaderType, int>(dataStructure, ebsdReader.get(), intArrayNames, selectedArrayNames, cellAttributeMatrixPath, totalPoints);

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

  // Now Calculate our "subvolume" of slices, ie, those start and end values that the user selected from the GUI
  dcDims[2] = m_InputValues->endSlice - m_InputValues->startSlice + 1;

  std::string manufacturer = volumeInfoReader->getManufacturer();

  std::array<float, 3> sampleTransAxis = volumeInfoReader->getSampleTransformationAxis();
  float sampleTransAngle = volumeInfoReader->getSampleTransformationAngle();

  std::array<float, 3> eulerTransAxis = volumeInfoReader->getEulerTransformationAxis();
  float eulerTransAngle = volumeInfoReader->getEulerTransformationAngle();

  // This will effectively close the reader
  volumeInfoReader = H5EbsdVolumeInfo::NullPointer();

  // Now create the specific reader that we need.
  H5EbsdVolumeReader::Pointer ebsdReader;

  std::set<std::string> mSelectedArrayNames;
  for(const auto& selectedArrayName : m_InputValues->hdf5DataPaths)
  {
    mSelectedArrayNames.insert(selectedArrayName);
  }

  if(manufacturer == EbsdLib::Ang::Manufacturer)
  {
    std::vector<std::string> eulerPhaseArrays = {EbsdLib::Ang::Phi1, EbsdLib::Ang::Phi, EbsdLib::Ang::Phi2, EbsdLib::Ang::PhaseData};
    std::vector<std::string> floatArrays = {EbsdLib::Ang::ImageQuality, EbsdLib::Ang::ConfidenceIndex, EbsdLib::Ang::SEMSignal, EbsdLib::Ang::Fit, EbsdLib::Ang::XPosition, EbsdLib::Ang::YPosition};
    std::vector<std::string> intArrays = {};
    Result<> result = LoadEbsdData<H5AngVolumeReader, AngPhase>(m_InputValues, m_DataStructure, eulerPhaseArrays, m_MessageHandler, mSelectedArrayNames, dcDims, floatArrays, intArrays);
    if(result.invalid())
    {
      return result;
    }
  }
  else if(manufacturer == EbsdLib::Ctf::Manufacturer)
  {
    std::vector<std::string> eulerPhaseArrays = {EbsdLib::Ctf::Euler1, EbsdLib::Ctf::Euler2, EbsdLib::Ctf::Euler3, EbsdLib::Ctf::Phase};
    std::vector<std::string> floatArrays = {EbsdLib::Ctf::MAD, EbsdLib::Ctf::X, EbsdLib::Ctf::Y};
    std::vector<std::string> intArrays = {EbsdLib::Ctf::Bands, EbsdLib::Ctf::Error, EbsdLib::Ctf::BC, EbsdLib::Ctf::BS};
    Result<> result = LoadEbsdData<H5CtfVolumeReader, CtfPhase>(m_InputValues, m_DataStructure, eulerPhaseArrays, m_MessageHandler, mSelectedArrayNames, dcDims, floatArrays, intArrays);
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
      RotateEulerRefFrameFilter rotEuler;
      Arguments args;
      args.insertOrAssign(RotateEulerRefFrameFilter::k_RotationAxisAngle_Key,
                          std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{eulerTransAxis[0], eulerTransAxis[1], eulerTransAxis[2], eulerTransAngle}));

      complex::DataPath eulerDataPath = m_InputValues->cellAttributeMatrixPath.createChildPath(EbsdLib::CellData::EulerAngles); // get the Euler data from the DataStructure
      args.insertOrAssign(RotateEulerRefFrameFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(eulerDataPath));
      // Preflight the filter and check result
      auto preflightResult = rotEuler.preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
      }

      // Execute the filter and check the result
      auto executeResult = rotEuler.execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
        return {MakeErrorResult(-50011, fmt::format("Error executing {}", rotEuler.humanName()))};
      }
    }

    if(sampleTransAngle > 0)
    {
      const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
      auto* filterList = Application::Instance()->getFilterList();

      /*************************************************************************
       * Rotate Sample Ref Frame
       ************************************************************************/
      const Uuid k_RotateSampleRefFrameFilterId = *Uuid::FromString("d2451dc1-a5a1-4ac2-a64d-7991669dcffc");
      const FilterHandle k_RotateSampleRefFrameFilterHandle(k_RotateSampleRefFrameFilterId, k_ComplexCorePluginId);

      auto filter = filterList->createFilter(k_RotateSampleRefFrameFilterHandle);
      if(nullptr == filter)
      {
        return {MakeErrorResult(-50010, fmt::format("Error creating RotateSampleRefFrame filter"))};
      }
      Arguments args;

      args.insertOrAssign(RotateSampleRefFrame::k_SelectedImageGeometry_Key, std::make_any<DataPath>(m_InputValues->dataContainerPath));
      args.insertOrAssign(RotateSampleRefFrame::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));

      args.insertOrAssign(RotateSampleRefFrame::k_RotationRepresentation_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::AxisAngle)));
      args.insertOrAssign(RotateSampleRefFrame::k_RotationAxisAngle_Key,
                          std::make_any<VectorFloat32Parameter::ValueType>({sampleTransAxis[0], sampleTransAxis[1], sampleTransAxis[2], sampleTransAngle}));
      args.insertOrAssign(RotateSampleRefFrame::k_RotateSliceBySlice_Key, std::make_any<bool>(true));

      // Preflight the filter and check result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Preflighting {}...", filter->humanName())});
      complex::IFilter::PreflightResult preflightResult = filter->preflight(m_DataStructure, args);
      if(preflightResult.outputActions.invalid())
      {
        for(const auto& error : preflightResult.outputActions.errors())
        {
          std::cout << error.code << ": " << error.message << std::endl;
        }
        return {MakeErrorResult(-50012, fmt::format("Error preflighting {}", filter->humanName()))};
      }

      // Execute the filter and check the result
      m_MessageHandler(complex::IFilter::Message{IFilter::Message::Type::Info, fmt::format("Executing {}", filter->humanName())});
      auto executeResult = filter->execute(m_DataStructure, args, nullptr, m_MessageHandler, m_ShouldCancel);
      if(executeResult.result.invalid())
      {
        return {{nonstd::make_unexpected(executeResult.result.errors())}};
      }
    }
  }

  return {};
}
