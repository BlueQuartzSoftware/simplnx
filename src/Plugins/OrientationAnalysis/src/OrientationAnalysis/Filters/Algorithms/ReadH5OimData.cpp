#include "ReadH5OimData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ReadH5OimData::ReadH5OimData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues)
: IEbsdOemReader<H5OIMReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
{
}

// -----------------------------------------------------------------------------
ReadH5OimData::~ReadH5OimData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadH5OimData::operator()()
{
  return execute();
}

// -----------------------------------------------------------------------------
Result<> ReadH5OimData::copyRawEbsdData(int index)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize offset = index * totalPoints;

  // Adjust the values of the 'phase' data to correct for invalid values
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::AngFile::Phases));
  auto* phasePtr = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::Ang::PhaseData));

  const auto* phi1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::Phi1));
  const auto* phi = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::Phi));
  const auto* phi2 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::Phi2));
  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::AngFile::EulerAngles));

  const auto* imageQual = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::ImageQuality));
  auto& imageQuality = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::ImageQuality));

  const auto* confIndex = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::ConfidenceIndex));
  auto& confidenceIndex = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::ConfidenceIndex));

  const auto* semSig = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::SEMSignal));
  auto& semSignal = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::SEMSignal));

  const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::Ang::Fit));
  auto& fit = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::Fit));

  for(size_t i = 0; i < totalPoints; i++)
  {
    if(phasePtr[i] < 1)
    {
      phasePtr[i] = 1;
    }
    phases[offset + i] = phasePtr[i];

    // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
    eulerAngles[offset + 3 * i] = phi1[i];
    eulerAngles[offset + 3 * i + 1] = phi[i];
    eulerAngles[offset + 3 * i + 2] = phi2[i];

    imageQuality[offset + i] = imageQual[i];

    confidenceIndex[offset + i] = confIndex[i];

    semSignal[offset + i] = semSig[i];

    fit[offset + i] = f1[i];
  }

  if(m_InputValues->ReadPatternData)
  {
    const uint8* patternDataPtr = m_Reader->getPatternData();
    std::array<int32, 2> pDims = {{0, 0}};
    m_Reader->getPatternDims(pDims);
    if(pDims[0] != 0 && pDims[1] != 0)
    {
      std::vector<usize> pDimsV(2);
      pDimsV[0] = pDims[0];
      pDimsV[1] = pDims[1];
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::PatternData));
      const usize numComponents = patternData.getNumberOfComponents();
      for(usize i = 0; i < totalPoints; i++)
      {
        for(usize j = 0; j < numComponents; ++j)
        {
          patternData[offset + numComponents * i + j] = patternDataPtr[numComponents * i + j];
        }
      }
    }
  }

  return {};
}
