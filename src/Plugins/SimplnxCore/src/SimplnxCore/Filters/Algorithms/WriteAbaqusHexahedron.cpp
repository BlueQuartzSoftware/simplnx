#include "WriteAbaqusHexahedron.hpp"
#include "SimplnxCore/utils/AbaqusElementSets.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <array>
#include <filesystem>
#include <fstream>

using namespace nx::core;

namespace
{
/**
 * @brief Gets the eight one-based node IDs for one hexahedral cell.
 * @param x Cell X index.
 * @param y Cell Y index.
 * @param z Cell Z index.
 * @param pDims Node-grid dimensions in X, Y, and Z order.
 * @return Abaqus node IDs in the writer's local corner order.
 * @pre pDims contains three values and all index products fit int64.
 */
std::array<int64, 8> GetNodeIds(usize x, usize y, usize z, const usize* pDims)
{
  std::array<int64, 8> nodeId{};

  nodeId[0] = static_cast<int64>(1 + (pDims[0] * pDims[1] * z) + (pDims[0] * y) + x);
  nodeId[1] = static_cast<int64>(1 + (pDims[0] * pDims[1] * z) + (pDims[0] * y) + (x + 1));
  nodeId[2] = static_cast<int64>(1 + (pDims[0] * pDims[1] * z) + (pDims[0] * (y + 1)) + x);
  nodeId[3] = static_cast<int64>(1 + (pDims[0] * pDims[1] * z) + (pDims[0] * (y + 1)) + (x + 1));

  nodeId[4] = static_cast<int64>(1 + (pDims[0] * pDims[1] * (z + 1)) + (pDims[0] * y) + x);
  nodeId[5] = static_cast<int64>(1 + (pDims[0] * pDims[1] * (z + 1)) + (pDims[0] * y) + (x + 1));
  nodeId[6] = static_cast<int64>(1 + (pDims[0] * pDims[1] * (z + 1)) + (pDims[0] * (y + 1)) + x);
  nodeId[7] = static_cast<int64>(1 + (pDims[0] * pDims[1] * (z + 1)) + (pDims[0] * (y + 1)) + (x + 1));

#if 0
  {
    printf("           %lld-------%lld  \n", static_cast<long long int>(nodeId[4]), static_cast<long long int>(nodeId[5]));
    printf("            /|        /|   \n");
    printf("           / |       / |   \n");
    printf("          /  |      /  |   \n");
    printf("       %lld--------%lld  |   \n", static_cast<long long int>(nodeId[6]), static_cast<long long int>(nodeId[7]));
    printf("         |   |      |  |   \n");
    printf("         | %lld------|-%lld  \n", static_cast<long long int>(nodeId[0]), static_cast<long long int>(nodeId[1]));
    printf("         |  /       | /    \n");
    printf("         | /        |/     \n");
    printf("        %lld--------%lld     \n", static_cast<long long int>(nodeId[2]), static_cast<long long int>(nodeId[3]));
#endif
  return nodeId;
}

/**
 * @brief Writes all ImageGeom nodes and an optional dummy node.
 * @param progressThrottle Receives progress messages.
 * @param fileName Temporary node-file path.
 * @param cDims Cell dimensions in X, Y, and Z order.
 * @param origin ImageGeom origin.
 * @param spacing ImageGeom spacing.
 * @param shouldCancel Signals cancellation between Z planes.
 * @param writeDummyNode True to append a zero-coordinate node.
 * @return Zero on completion, one on cancellation, or -1 when fopen fails.
 * @pre Pointers contain three values and dimension products fit usize.
 *
 * C stdio return values are not inspected. Cancellation is checked per Z plane.
 */
int32 WriteNodes(ThrottledMessageHandler& progressThrottle, const std::string& fileName, usize* cDims, const float32* origin, const float32* spacing, const std::atomic_bool& shouldCancel,
                 bool writeDummyNode)
{
  usize pDims[3] = {cDims[0] + 1, cDims[1] + 1, cDims[2] + 1};
  usize nodeIndex = 1;

  int32 err = 0;
  FILE* fPtr = fopen(fileName.c_str(), "wb");
  if(nullptr == fPtr)
  {
    return -1;
  }

  progressThrottle.reset(pDims[2], "Writing Nodes (File 1/5)");
  fprintf(fPtr, "** ----------------------------------------------------------------\n**\n*Node\n");
  for(usize z = 0; z < pDims[2]; z++)
  {
    if(shouldCancel)
    {
      fclose(fPtr);
      return 1;
    }
    for(usize y = 0; y < pDims[1]; y++)
    {
      for(usize x = 0; x < pDims[0]; x++)
      {
        const float64 xCoord = static_cast<float64>(origin[0]) + static_cast<float64>(x) * static_cast<float64>(spacing[0]);
        const float64 yCoord = static_cast<float64>(origin[1]) + static_cast<float64>(y) * static_cast<float64>(spacing[1]);
        const float64 zCoord = static_cast<float64>(origin[2]) + static_cast<float64>(z) * static_cast<float64>(spacing[2]);
        fmt::print(fPtr, "{}, {:.9g}, {:.9g}, {:.9g}\n", nodeIndex, xCoord, yCoord, zCoord);

        ++nodeIndex;
      }
    }
    progressThrottle.updateCount(z + 1);
  }

  if(writeDummyNode)
  {
    // The dummy node supports stress-strain curve workflows.
    fprintf(fPtr, "%llu, %f, %f, %f\n", static_cast<unsigned long long int>(nodeIndex), 0.0f, 0.0f, 0.0f);
  }
  fprintf(fPtr, "**\n** ----------------------------------------------------------------\n**\n");

  fclose(fPtr);
  return err;
}

/**
 * @brief Writes one C3D8 or C3D8R element for every ImageGeom cell.
 * @param progressThrottle Receives progress messages.
 * @param fileName Temporary element-file path.
 * @param cDims Cell dimensions in X, Y, and Z order.
 * @param pDims Node-grid dimensions in X, Y, and Z order.
 * @param shouldCancel Signals cancellation between Z planes.
 * @param useReducedIntegration Writes C3D8R elements when true and C3D8 elements otherwise.
 * @return Zero on completion, one on cancellation, or -1 when fopen fails.
 * @pre Dimension products and generated IDs fit the output integer types.
 *
 * C stdio return values are not inspected. Cancellation is checked per Z plane.
 */
int32 WriteElems(ThrottledMessageHandler& progressThrottle, const std::string& fileName, const usize* cDims, usize* pDims, const std::atomic_bool& shouldCancel, bool useReducedIntegration)
{

  int32 err = 0;
  FILE* fPtr = fopen(fileName.c_str(), "wb");
  if(nullptr == fPtr)
  {
    return -1;
  }

  // Keep the format casts consistent across platforms.
  using LliTType = long long int;

  progressThrottle.reset(cDims[2], "Writing Elements (File 2/5)");
  usize index = 1;
  fprintf(fPtr, "** ----------------------------------------------------------------\n**\n*Element, type=%s\n", useReducedIntegration ? "C3D8R" : "C3D8");
  for(usize z = 0; z < cDims[2]; z++)
  {
    if(shouldCancel)
    {
      fclose(fPtr);
      return 1;
    }
    for(usize y = 0; y < cDims[1]; y++)
    {
      for(usize x = 0; x < cDims[0]; x++)
      {
        const std::array<int64, 8> nodeId = GetNodeIds(x, y, z, pDims);
        fprintf(fPtr, "%llu, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n", (LliTType)index, (LliTType)nodeId[5], (LliTType)nodeId[1], (LliTType)nodeId[0], (LliTType)nodeId[4],
                (LliTType)nodeId[7], (LliTType)nodeId[3], (LliTType)nodeId[2], (LliTType)nodeId[6]);

        ++index;
      }
    }
    progressThrottle.updateCount(z + 1);
  }

  fprintf(fPtr, "**\n** ----------------------------------------------------------------\n**\n");

  fclose(fPtr);
  return err;
}

/**
 * @brief Opens and writes the element-set file with the selected grouping path.
 * @param progressThrottle Receives grouping progress messages.
 * @param fileName Temporary ELSET path.
 * @param totalPoints Number of ImageGeom cells for the generated cube set.
 * @param featureIdsRef Supplies cell Feature IDs.
 * @param maxGrainId Largest grain ID to emit.
 * @param useOocAlgorithm True to select external sort or repeated scans.
 * @param shouldCancel Signals cancellation during grouping and sorting.
 * @return File-open, source-I/O, sorter, or provider-capability result.
 *
 * The function checks stream errors before cancellation. A valid cancellation result leaves a partial temporary file.
 */
Result<> WriteElset(ThrottledMessageHandler& progressThrottle, const std::string& fileName, size_t totalPoints, const Int32AbstractDataStoreType& featureIdsRef, int32 maxGrainId, bool useOocAlgorithm,
                    const std::atomic_bool& shouldCancel)
{
  std::ofstream output(fileName, std::ios::binary);
  if(!output.is_open())
  {
    return MakeErrorResult(-1116, fmt::format("Could not open Abaqus element-set output file '{}'.", fileName));
  }
  output << "** ----------------------------------------------------------------\n**\n** The element sets\n"
         << "*Elset, elset=cube, generate\n"
         << fmt::format("1, {}, 1\n", totalPoints) << "**\n** Each Grain is made up of multiple elements\n**";
  Result<> result = abaqus_element_sets::Write(
      progressThrottle, output, [](int32 grainId) { return fmt::format("\n*Elset, elset=Grain{}_set\n", grainId); }, featureIdsRef, maxGrainId, useOocAlgorithm, shouldCancel);
  if(result.valid())
  {
    output << "\n**\n** ----------------------------------------------------------------\n**\n";
  }
  output.close();
  if(!output)
  {
    return MakeErrorResult(-12013, fmt::format("Writing Abaqus element sets to '{}' failed. Check available disk space.", fileName));
  }
  return result;
}

/**
 * @brief Writes the master Abaqus include file.
 * @param file Temporary master-file path.
 * @param jobName Heading and job name.
 * @param filePrefix Prefix of the four included files.
 * @return Zero on completion or -1 when fopen fails.
 *
 * C stdio return values are not inspected.
 */
int32 WriteMaster(const std::string& file, const std::string& jobName, const std::string& filePrefix)
{
  int32 err = 0;
  FILE* fPtr = fopen(file.c_str(), "wb");
  if(nullptr == fPtr)
  {
    return -1;
  }

  fprintf(fPtr, "*Heading\n");
  fprintf(fPtr, "%s\n", jobName.c_str());
  fprintf(fPtr, "** Job name : %s\n", jobName.c_str());
  fprintf(fPtr, "*Preprint, echo = NO, model = NO, history = NO, contact = NO\n");
  fprintf(fPtr, "**\n** ----------------------------Geometry----------------------------\n**\n");
  fprintf(fPtr, "*Include, Input = %s\n", (filePrefix + "_nodes.inp").c_str());
  fprintf(fPtr, "*Include, Input = %s\n", (filePrefix + "_elems.inp").c_str());
  fprintf(fPtr, "*Include, Input = %s\n", (filePrefix + "_elset.inp").c_str());
  fprintf(fPtr, "*Include, Input = %s\n", (filePrefix + "_sects.inp").c_str());
  fprintf(fPtr, "**\n** ----------------------------------------------------------------\n**\n");

  fclose(fPtr);
  return err;
}

/**
 * @brief Writes one solid section for each positive grain ID through the maximum.
 * @param file Temporary section-file path.
 * @param maxGrainId Largest grain ID to emit.
 * @param useReducedIntegration Writes hourglass stiffness when true.
 * @param hourglassStiffness Hourglass stiffness written for reduced-integration sections.
 * @return Zero on completion or -1 when fopen fails.
 *
 * C stdio return values are not inspected.
 */
int32 WriteSects(const std::string& file, int32 maxGrainId, bool useReducedIntegration, int32 hourglassStiffness)
{
  int32 err = 0;
  FILE* fPtr = fopen(file.c_str(), "wb");
  if(nullptr == fPtr)
  {
    return -1;
  }
  fprintf(fPtr, "** ----------------------------------------------------------------\n**\n** Each section is a separate grain\n");

  // Emit sections for IDs that can include gaps in the input Feature IDs.
  int32 grain = 1;
  while(grain <= maxGrainId)
  {
    fprintf(fPtr, "** Section: Grain%d\n", grain);
    fprintf(fPtr, "*Solid Section, elset=Grain%d_set, material=Grain_Mat%d\n", grain, grain);
    if(useReducedIntegration)
    {
      fprintf(fPtr, "*Hourglass Stiffness\n%d\n", hourglassStiffness);
    }
    fprintf(fPtr, "** --------------------------------------\n");
    grain++;
  }
  fprintf(fPtr, "**\n** ----------------------------------------------------------------\n**\n");

  fclose(fPtr);
  return err;
}

/**
 * @brief Removes temporary files for all successfully created AtomicFile objects.
 * @param fileList Atomic-file creation results.
 *
 * Final files that were already committed are not restored.
 */
void DeleteFiles(const std::vector<Result<AtomicFile>>& fileList)
{
  for(const auto& atomicFile : fileList)
  {
    if(atomicFile.valid())
    {
      atomicFile.value().removeTempFile();
    }
  }
}
} // namespace

WriteAbaqusHexahedron::WriteAbaqusHexahedron(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             WriteAbaqusHexahedronInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& WriteAbaqusHexahedron::getCancel()
{
  return m_ShouldCancel;
}

Result<> WriteAbaqusHexahedron::operator()()
{
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->image_geometry_path);
  auto& featureIdsRef = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->feature_ids_array_path).getDataStoreRef();
  Vec3<usize> cDims = imageGeom.getDimensions();
  usize pDims[3] = {cDims[0] + 1, cDims[1] + 1, cDims[2] + 1};
  Vec3<float32> origin = imageGeom.getOrigin();
  Vec3<float32> spacing = imageGeom.getSpacing();
  usize totalPoints = imageGeom.getNumberOfCells();

  Result<int32> maxGrainResult = abaqus_element_sets::FindMaximumGrainId(featureIdsRef, getCancel());
  if(maxGrainResult.invalid())
  {
    return ConvertResult(std::move(maxGrainResult));
  }
  const int32 maxGrainId = maxGrainResult.value();
  if(getCancel())
  {
    return {};
  }
  const bool usesOutOfCoreStore = featureIdsRef.getStoreType() == IDataStore::StoreType::OutOfCore;
  const bool useOocAlgorithm = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
  RecordAlgorithmPathExecution(useOocAlgorithm ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

  // Each output gets a separate recoverable temporary path.
  std::vector<Result<AtomicFile>> fileList;
  fileList.push_back(AtomicFile::Create(m_InputValues->output_path / fmt::format("{}_nodes.inp", m_InputValues->file_prefix)));
  fileList.push_back(AtomicFile::Create(m_InputValues->output_path / fmt::format("{}_elems.inp", m_InputValues->file_prefix)));
  fileList.push_back(AtomicFile::Create(m_InputValues->output_path / fmt::format("{}_sects.inp", m_InputValues->file_prefix)));
  fileList.push_back(AtomicFile::Create(m_InputValues->output_path / fmt::format("{}_elset.inp", m_InputValues->file_prefix)));
  fileList.push_back(AtomicFile::Create(m_InputValues->output_path / fmt::format("{}.inp", m_InputValues->file_prefix)));

  for(auto& file : fileList)
  {
    if(file.invalid())
    {
      return ConvertResult(std::move(file));
    }
  }

  ThrottledMessageHandler progressThrottle(m_MessageHandler);
  int32 err = WriteNodes(progressThrottle, fileList[0].value().tempFilePath().string(), cDims.data(), origin.data(), spacing.data(), getCancel(), m_InputValues->write_dummy_node);
  progressThrottle.flush();
  if(err < 0)
  {
    return MakeErrorResult(-1113, fmt::format("Error writing output nodes file '{}'", fileList[0].value().tempFilePath().string()));
  }
  if(getCancel())
  {
    DeleteFiles(fileList);
    return {};
  }
  m_MessageHandler.sendInfoMessage("Wrote Nodes (File 1/5)");

  err = WriteElems(progressThrottle, fileList[1].value().tempFilePath().string(), cDims.data(), pDims, getCancel(), m_InputValues->use_reduced_integration);
  progressThrottle.flush();
  if(err < 0)
  {
    return MakeErrorResult(-1114, fmt::format("Error writing output elems file '{}'", fileList[1].value().tempFilePath().string()));
  }
  if(getCancel())
  {
    DeleteFiles(fileList);
    return {};
  }
  m_MessageHandler.sendInfoMessage("Wrote Elements (File 2/5)");

  err = WriteSects(fileList[2].value().tempFilePath().string(), maxGrainId, m_InputValues->use_reduced_integration, m_InputValues->hourglass_stiffness); // Sections file
  if(err < 0)
  {
    return MakeErrorResult(-1115, fmt::format("Error writing output sects file '{}'", fileList[2].value().tempFilePath().string()));
  }
  if(getCancel())
  {
    DeleteFiles(fileList);
    return {};
  }
  m_MessageHandler.sendInfoMessage("Wrote Sections (File 3/5)");

  Result<> elsetResult = WriteElset(progressThrottle, fileList[3].value().tempFilePath().string(), totalPoints, featureIdsRef, maxGrainId, useOocAlgorithm, getCancel());
  progressThrottle.flush();
  if(elsetResult.invalid())
  {
    DeleteFiles(fileList);
    return elsetResult;
  }
  if(getCancel())
  {
    DeleteFiles(fileList);
    return {};
  }
  m_MessageHandler.sendInfoMessage("Wrote Element Sets (File 4/5)");

  err = WriteMaster(fileList[4].value().tempFilePath().string(), m_InputValues->job_name, m_InputValues->file_prefix);
  if(err < 0)
  {
    return MakeErrorResult(-1117, fmt::format("Error writing output master file '{}'", fileList[4].value().tempFilePath().string()));
  }
  if(getCancel())
  {
    DeleteFiles(fileList);
    return {};
  }
  m_MessageHandler.sendInfoMessage("Wrote Master (File 5/5)");

  // Commits are atomic per file but are not one transaction for the file set.
  for(auto& file : fileList)
  {
    if(m_ShouldCancel)
    {
      return MakeErrorResult(-1, "Filter cancelled");
    }
    Result<> commitResult = file.value().commit();
    if(commitResult.invalid())
    {
      return commitResult;
    }
  }

  return {};
}
