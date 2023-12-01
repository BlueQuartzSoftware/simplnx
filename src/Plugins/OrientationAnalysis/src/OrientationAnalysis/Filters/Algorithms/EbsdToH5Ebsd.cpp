#include "EbsdToH5Ebsd.hpp"

#include "complex/Common/AtomicFile.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/IO/HKL/CtfFields.h"
#include "EbsdLib/IO/HKL/H5CtfImporter.h"
#include "EbsdLib/IO/TSL/AngFields.h"
#include "EbsdLib/IO/TSL/H5AngImporter.h"

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

using namespace complex;

// -----------------------------------------------------------------------------
EbsdToH5Ebsd::EbsdToH5Ebsd(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EbsdToH5EbsdInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
EbsdToH5Ebsd::~EbsdToH5Ebsd() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& EbsdToH5Ebsd::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> EbsdToH5Ebsd::operator()()
{
  auto absPath = m_InputValues->OutputPath;
  if(!absPath.is_absolute())
  {
    try
    {
      absPath = fs::absolute(absPath);
    } catch(const std::filesystem::filesystem_error& error)
    {
      return MakeErrorResult(-15000,
                             fmt::format("EbsdToH5Ebsd::operator()() threw an error when creating absolute path from '{}'. Reported error is '{}'", m_InputValues->OutputPath.string(), error.what()));
    }
  }

  AtomicFile atomicFile(absPath, false);

  auto dirResult = atomicFile.createOutputDirectories();
  if(dirResult.invalid())
  {
    return dirResult;
  }

  // Create output H5Ebsd File
  hid_t fileId = H5Support::H5Utilities::createFile(absPath.string());
  if(fileId < 0)
  {
    return MakeErrorResult(-99501, fmt::format("The output HDF5 file could not be created. Check permissions or if the file is in use by another program"));
  }

  H5Support::H5ScopedFileSentinel sentinel(fileId, true);

  herr_t err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::ZResolution, m_InputValues->ZSpacing);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Z Spacing Scalar to the HDF5 File");
    return MakeErrorResult(-99502, ss);
  }

  err = H5Support::H5Lite::writeScalarDataset<uint32>(fileId, EbsdLib::H5Ebsd::StackingOrder, static_cast<uint32>(m_InputValues->StackingOrder));
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Stacking Order Scalar to the HDF5 File");
    return MakeErrorResult(-99503, ss);
  }

  err = H5Support::H5Lite::writeStringAttribute(fileId, EbsdLib::H5Ebsd::StackingOrder, "Name", EbsdToH5EbsdInputConstants::k_StackingChoices[m_InputValues->StackingOrder]);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Stacking Order Name Attribute to the HDF5 File");
    return MakeErrorResult(-99504, ss);
  }

  auto eulerTransformation = EbsdToH5EbsdInputConstants::k_NoEulerTransform;
  auto sampleTransformation = EbsdToH5EbsdInputConstants::k_NoSampleTransform;
  switch(m_InputValues->ReferenceFrame)
  {
  case EbsdToH5EbsdInputConstants::k_Edax:
    eulerTransformation = EbsdToH5EbsdInputConstants::k_EdaxEulerTransform;
    sampleTransformation = EbsdToH5EbsdInputConstants::k_EdaxSampleTransform;
    break;
  case EbsdToH5EbsdInputConstants::k_Oxford:
    eulerTransformation = EbsdToH5EbsdInputConstants::k_OxfordEulerTransform;
    sampleTransformation = EbsdToH5EbsdInputConstants::k_OxfordSampleTransform;
    break;
  case EbsdToH5EbsdInputConstants::k_Hedm:
    eulerTransformation = EbsdToH5EbsdInputConstants::k_HedmEulerTransform;
    sampleTransformation = EbsdToH5EbsdInputConstants::k_HedmSampleTransform;
    break;
  default:
    break;
  }

  err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::SampleTransformationAngle, sampleTransformation[EbsdToH5EbsdInputConstants::k_AngleIndex]);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Sample Transformation Angle to the HDF5 File");
    return MakeErrorResult(-99505, ss);
  }

  int32_t rank = 1;
  hsize_t dims[3] = {3, 0, 0};
  err = H5Support::H5Lite::writePointerDataset<float>(fileId, EbsdLib::H5Ebsd::SampleTransformationAxis, rank, dims, sampleTransformation.data());
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Sample Transformation Axis to the HDF5 File");
    return MakeErrorResult(-99506, ss);
  }

  err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::EulerTransformationAngle, eulerTransformation[EbsdToH5EbsdInputConstants::k_AngleIndex]);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Euler Transformation Angle to the HDF5 File");
    return MakeErrorResult(-99507, ss);
  }

  err = H5Support::H5Lite::writePointerDataset<float>(fileId, EbsdLib::H5Ebsd::EulerTransformationAxis, rank, dims, eulerTransformation.data());
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Euler Transformation Axis to the HDF5 File");
    return MakeErrorResult(-99508, ss);
  }

  // Now generate all the file names the user is asking for and populate the table
  std::vector<std::string> fileList = m_InputValues->InputFileListInfo.generate();
  if(fileList.empty())
  {
    return MakeErrorResult(-99509, fmt::format("Generated File List was empty. Parent path to files is '{}'", m_InputValues->InputFileListInfo.inputPath));
  }
  EbsdImporter::Pointer fileImporter;

  int32 zStartIndex = m_InputValues->InputFileListInfo.startIndex;

  // Write the Manufacturer of the OIM file here
  // This list will grow to be the number of EBSD file formats we support
  auto firstFilePath = fs::path(fileList[0]);
  std::string ext = firstFilePath.extension().string();
  ext.erase(0, 1); // Remove the '.' from the string
  if(ext == EbsdLib::Ang::FileExt)
  {
    err = H5Support::H5Lite::writeStringDataset(fileId, EbsdLib::H5Ebsd::Manufacturer, EbsdLib::Ang::Manufacturer);
    if(err < 0)
    {

      std::string ss = fmt::format("Could not write the Manufacturer Data to the HDF5 File");
      return MakeErrorResult(-99509, ss);
    }
    fileImporter = H5AngImporter::New();
  }
  else if(ext == EbsdLib::Ctf::FileExt)
  {
    err = H5Support::H5Lite::writeStringDataset(fileId, EbsdLib::H5Ebsd::Manufacturer, EbsdLib::Ctf::Manufacturer);
    if(err < 0)
    {
      std::string ss = fmt::format("Could not write the Manufacturer Data to the HDF5 File");
      return MakeErrorResult(-99510, ss);
    }
    fileImporter = H5CtfImporter::New();
    CtfReader ctfReader;
    ctfReader.setFileName(fileList.front());
    err = ctfReader.readHeaderOnly();
    if(ctfReader.getZCells() > 1 && fileList.size() == 1)
    {
      zStartIndex = 0;
    }
    if(err < 0)
    {
      std::string ss = fmt::format("Error reading CTF file header");
      return MakeErrorResult(-99511, ss);
    }
  }
  else
  {
    std::string ss = fmt::format("The file extension was not detected correctly");
    return MakeErrorResult(-99512, ss);
  }

  std::vector<int32_t> indices;
  // Loop on Each EBSD File
  int64_t z = zStartIndex;
  int64_t xDim = 0, yDim = 0;
  float xRes = 0.0f, yRes = 0.0f;
  /* There is a frailness about the z index and the file list. The programmer
   * using this code MUST ensure that the list of files that is sent into this
   * class is in the appropriate order to match up with the z index (slice index)
   * otherwise the import will have subtle errors. The programmer is urged NOT to
   * simply gather a list from the file system as those lists are sorted in such
   * a way that if the number of digits appearing in the filename are NOT the same
   * then the list will be wrong, ie, this example:
   *
   * slice_1.ang
   * slice_2.ang
   * ....
   * slice_10.ang
   *
   * Most, if not ALL C++ libraries when asked for that list will return the list
   * sorted like the following:
   *
   * slice_1.ang
   * slice_10.ang
   * slice_2.ang
   *
   * which is going to cause problems because the data is going to be placed
   * into the HDF5 file at the wrong index. YOU HAVE BEEN WARNED.
   */
  int64_t biggestXDim = 0;
  int64_t biggestYDim = 0;
  int32_t totalSlicesImported = 0;
  for(const auto& ebsdFName : fileList)
  {
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Converting File: '{}'", ebsdFName));

    err = fileImporter->importFile(fileId, z, ebsdFName);
    if(err < 0)
    {
      return MakeErrorResult(err, std::string(fileImporter->getPipelineMessage()));
    }
    totalSlicesImported = totalSlicesImported + fileImporter->numberOfSlicesImported();

    fileImporter->getDims(xDim, yDim);
    fileImporter->getSpacing(xRes, yRes);
    if(xDim > biggestXDim)
    {
      biggestXDim = xDim;
    }
    if(yDim > biggestYDim)
    {
      biggestYDim = yDim;
    }

    indices.push_back(static_cast<int32_t>(z));
    ++z;
    if(getCancel())
    {
      return {};
    }
  }

  // Write Z index start, Z index end and Z Spacing to the HDF5 file
  err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::ZStartIndex, zStartIndex);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Z Start Index Scalar to the HDF5 File");
    return MakeErrorResult(-99514, ss);
  }

  auto zEndIndex = zStartIndex + totalSlicesImported - 1;
  err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::ZEndIndex, zEndIndex);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the Z End Index Scalar to the HDF5 File");
    return MakeErrorResult(-99515, ss);
  }

  err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::XPoints, biggestXDim);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the XPoints Scalar to HDF5 file");
    return MakeErrorResult(-99516, ss);
  }

  err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::YPoints, biggestYDim);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the YPoints Scalar to HDF5 file");
    return MakeErrorResult(-99517, ss);
  }

  err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::XResolution, xRes);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the XResolution Scalar to HDF5 file");
    return MakeErrorResult(-99518, ss);
  }

  err = H5Support::H5Lite::writeScalarDataset(fileId, EbsdLib::H5Ebsd::YResolution, yRes);
  if(err < 0)
  {
    std::string ss = fmt::format("Could not write the YResolution Scalar to HDF5 file");
    return MakeErrorResult(-99519, ss);
  }

  if(!getCancel())
  {
    // Write an Index data set which contains all the z index values which
    // should help speed up the reading side of this file
    std::vector<hsize_t> dimsL = {static_cast<hsize_t>(indices.size())};
    err = H5Support::H5Lite::writeVectorDataset(fileId, std::string(EbsdLib::H5Ebsd::Index), dimsL, indices);
    if(err < 0)
    {
      std::string ss = fmt::format("Error writing index dataset to H5Ebsd");
      return MakeErrorResult(-99520, ss);
    }
  }

  atomicFile.commit();
  return {};
}
