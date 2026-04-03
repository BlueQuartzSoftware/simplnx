#include "ReadH5OimData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ReadH5OimData::ReadH5OimData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues)
: IEbsdOemReader<ebsdlib::H5OIMReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
{
}

// -----------------------------------------------------------------------------
ReadH5OimData::~ReadH5OimData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadH5OimData::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Reading H5 OIM data...");

  return execute();
}

// -----------------------------------------------------------------------------
Result<> ReadH5OimData::copyRawEbsdData(int sliceIndex)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize tuplesPerScan = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize sliceTupleStart = sliceIndex * tuplesPerScan;

  // Adjust the values of the 'phase' data to correct for invalid values
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::Phases));
  auto* phasePtr = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::Ang::PhaseData));

  const auto* phi1Ptr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::Phi1));
  const auto* phiPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::Phi));
  const auto* phi2Ptr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::Phi2));
  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::EulerAngles));

  const auto* imageQualPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::ImageQuality));
  auto& imageQuality = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ImageQuality));

  const auto* confIndexPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::ConfidenceIndex));
  auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ConfidenceIndex));

  const auto* semSigPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::SEMSignal));
  auto& semSignal = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::SEMSignal));

  const auto* fitPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::Ang::Fit));
  auto& fit = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::Fit));

  for(size_t i = 0; i < tuplesPerScan; i++)
  {
    if(phasePtr[i] < 1)
    {
      phasePtr[i] = 1;
    }
    phases[sliceTupleStart + i] = phasePtr[i];

    // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
    eulerAngles[(sliceTupleStart + i) * 3] = phi1Ptr[i];
    eulerAngles[(sliceTupleStart + i) * 3 + 1] = phiPtr[i];
    eulerAngles[(sliceTupleStart + i) * 3 + 2] = phi2Ptr[i];

    imageQuality[sliceTupleStart + i] = imageQualPtr[i];

    confidenceIndex[sliceTupleStart + i] = confIndexPtr[i];

    semSignal[sliceTupleStart + i] = semSigPtr[i];

    fit[sliceTupleStart + i] = fitPtr[i];
  }

  if(m_InputValues->ReadPatternData)
  {
    const uint8* patternDataPtr = m_Reader->getPatternData();
    if(patternDataPtr == nullptr)
    {
      return MakeErrorResult(-34880, "Pattern data was requested but no pattern data was found in the data file");
    }
    std::array<int32, 2> pDims = {{0, 0}};
    m_Reader->getPatternDims(pDims);
    if(pDims[0] != 0 && pDims[1] != 0)
    {
      std::vector<usize> pDimsV(2);
      pDimsV[0] = pDims[0];
      pDimsV[1] = pDims[1];
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::PatternData));
      const usize numComponents = patternData.getNumberOfComponents();
      for(usize i = 0; i < tuplesPerScan; i++)
      {
        for(usize j = 0; j < numComponents; ++j)
        {
          patternData[sliceTupleStart + numComponents * i + j] = patternDataPtr[numComponents * i + j];
        }
      }
    }
  }

  return {};
}
