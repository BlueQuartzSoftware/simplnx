#include "WriteAbaqusHexahedron.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
std::string format_duration(std::chrono::milliseconds ms)
{
  using namespace std::chrono;
  auto secs = duration_cast<seconds>(ms);
  ms -= duration_cast<milliseconds>(secs);
  auto mins = duration_cast<minutes>(secs);
  secs -= duration_cast<seconds>(mins);
  auto hour = duration_cast<hours>(mins);
  mins -= duration_cast<minutes>(hour);

  std::stringstream ss;
  ss << hour.count() << " Hours : " << mins.count() << " Minutes : " << secs.count() << " Seconds : " << ms.count() << " Milliseconds";
  return ss.str();
}

std::vector<int64> getNodeIds(usize x, usize y, usize z, const usize* pDims)
{
  std::vector<int64> nodeId(8, 0);

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

int32 writeNodes(WriteAbaqusHexahedron* filter, const std::string& fileName, usize* cDims, const float32* origin, const float32* spacing, const std::atomic_bool& shouldCancel)
{
  usize pDims[3] = {cDims[0] + 1, cDims[1] + 1, cDims[2] + 1};
  usize nodeIndex = 1;
  usize totalPoints = pDims[0] * pDims[1] * pDims[2];
  auto increment = static_cast<usize>(totalPoints * 0.01f);
  if(increment == 0) // check to prevent divide by 0
  {
    increment = 1;
  }

  int32 err = 0;
  FILE* f = fopen(fileName.c_str(), "wb");
  if(nullptr == f)
  {
    return -1;
  }

  auto initialTime = std::chrono::steady_clock::now();
  fprintf(f, "** ----------------------------------------------------------------\n**\n*Node\n");
  for(usize z = 0; z < pDims[2]; z++)
  {
    for(usize y = 0; y < pDims[1]; y++)
    {
      for(usize x = 0; x < pDims[0]; x++)
      {
        float32 xCoord = origin[0] + (x * spacing[0]);
        float32 yCoord = origin[1] + (y * spacing[1]);
        float32 zCoord = origin[2] + (z * spacing[2]);
        fprintf(f, "%llu, %f, %f, %f\n", static_cast<unsigned long long int>(nodeIndex), xCoord, yCoord, zCoord);
        if(nodeIndex % increment == 0)
        {
          auto now = std::chrono::steady_clock::now();
          int64 milliDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - initialTime).count();
          if(milliDiff > 1000)
          {
            std::string percentage = "Writing Nodes (File 1/5) " + StringUtilities::number(static_cast<int32>((float32)(nodeIndex) / (float32)(totalPoints)*100)) + "% Completed ";
            float32 timeDiff = ((float32)nodeIndex / (float32)(milliDiff));
            int64 estimatedTime = (float32)(totalPoints - nodeIndex) / timeDiff;
            std::string timeRemaining = " || Est. Time Remain: " + format_duration(std::chrono::milliseconds(estimatedTime));
            filter->sendMessage(percentage + timeRemaining);
            initialTime = std::chrono::steady_clock::now();
            if(shouldCancel) // Filter has been cancelled
            {
              fclose(f);
              return 1;
            }
          }
        }
        ++nodeIndex;
      }
    }
  }

  // Write the last node, which is a dummy node used for stress - strain curves.
  fprintf(f, "%d, %f, %f, %f\n", 999999, 0.0f, 0.0f, 0.0f);
  fprintf(f, "**\n** ----------------------------------------------------------------\n**\n");

  // Close the file
  fclose(f);
  return err;
}

int32 writeElems(WriteAbaqusHexahedron* filter, const std::string& fileName, const usize* cDims, usize* pDims, const std::atomic_bool& shouldCancel)
{
  usize totalPoints = cDims[0] * cDims[1] * cDims[2];
  auto increment = static_cast<usize>(totalPoints * 0.01f);
  if(increment == 0) // check to prevent divide by 0
  {
    increment = 1;
  }

  int32 err = 0;
  FILE* f = fopen(fileName.c_str(), "wb");
  if(nullptr == f)
  {
    return -1;
  }

  // Get a typedef to shorten up the printf codes
  using _lli_t_ = long long int;

  auto initialTime = std::chrono::steady_clock::now();
  usize index = 1;
  fprintf(f, "** ----------------------------------------------------------------\n**\n*Element, type=C3D8\n");
  for(usize z = 0; z < cDims[2]; z++)
  {
    for(usize y = 0; y < cDims[1]; y++)
    {
      for(usize x = 0; x < cDims[0]; x++)
      {
        std::vector<int64> nodeId = getNodeIds(x, y, z, pDims);
        fprintf(f, "%llu, %lld, %lld, %lld, %lld, %lld, %lld, %lld, %lld\n", (_lli_t_)index, (_lli_t_)nodeId[5], (_lli_t_)nodeId[1], (_lli_t_)nodeId[0], (_lli_t_)nodeId[4], (_lli_t_)nodeId[7],
                (_lli_t_)nodeId[3], (_lli_t_)nodeId[2], (_lli_t_)nodeId[6]);
        if(index % increment == 0)
        {
          auto now = std::chrono::steady_clock::now();
          int64 milliDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - initialTime).count();
          if(milliDiff > 1000)
          {
            std::string percentage = "Writing Elements (File 2/5) " + StringUtilities::number(static_cast<int32>((float32)(index) / (float32)(totalPoints)*100)) + "% Completed ";
            float32 timeDiff = ((float32)index / (float32)(milliDiff));
            int64 estimatedTime = (float32)(totalPoints - index) / timeDiff;
            std::string timeRemaining = " || Est. Time Remain: " + format_duration(std::chrono::milliseconds(estimatedTime));
            filter->sendMessage(percentage + timeRemaining);
            initialTime = std::chrono::steady_clock::now();
            if(shouldCancel) // Filter has been cancelled
            {
              fclose(f);
              return 1;
            }
          }
        }
        ++index;
      }
    }
  }

  fprintf(f, "**\n** ----------------------------------------------------------------\n**\n");

  // Close the file
  fclose(f);
  return err;
}

int32 writeElset(WriteAbaqusHexahedron* filter, const std::string& fileName, size_t totalPoints, const Int32Array& featureIds, const std::atomic_bool& shouldCancel)
{
  int32 err = 0;
  FILE* f = fopen(fileName.c_str(), "wb");
  if(nullptr == f)
  {
    return -1;
  }

  fprintf(f, "** ----------------------------------------------------------------\n**\n** The element sets\n");
  fprintf(f, "*Elset, elset=cube, generate\n");
  fprintf(f, "1, %llu, 1\n", static_cast<unsigned long long int>(totalPoints));
  fprintf(f, "**\n** Each Grain is made up of multiple elements\n**");

  // find total number of Grain Ids
  int32 maxGrainId = *std::max_element(std::begin(featureIds), std::end(featureIds));

  auto increment = static_cast<int32>(maxGrainId * 0.1f);
  if(increment == 0) // check to prevent divide by 0
  {
    increment = 1;
  }

  auto initialTime = std::chrono::steady_clock::now();
  int32 voxelId = 1;
  while(voxelId <= maxGrainId)
  {
    usize elementPerLine = 0;
    fprintf(f, "\n*Elset, elset=Grain%d_set\n", voxelId);

    for(usize i = 0; i < featureIds.size(); i++)
    {
      if(featureIds[i] == voxelId)
      {
        if(elementPerLine != 0) // no comma at start
        {
          if((elementPerLine % 16) != 0u) // 16 per line
          {
            fprintf(f, ", ");
          }
          else
          {
            fprintf(f, ",\n");
          }
        }
        fprintf(f, "%llu", static_cast<unsigned long long int>(i + 1));
        elementPerLine++;
      }
    }
    if(voxelId % increment == 0)
    {
      auto now = std::chrono::steady_clock::now();
      int64 milliDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - initialTime).count();
      if(milliDiff > 1000)
      {
        std::string percentage = "Writing Element Sets (File 4/5) " + StringUtilities::number(static_cast<int>((float32)(voxelId) / (float32)(maxGrainId)*100)) + "% Completed ";
        float32 timeDiff = ((float32)voxelId / (float32)(milliDiff));
        auto estimatedTime = static_cast<int64>((float32)(maxGrainId - voxelId) / timeDiff);
        std::string timeRemaining = " || Est. Time Remain: " + format_duration(std::chrono::milliseconds(estimatedTime));
        filter->sendMessage(percentage + timeRemaining);
        initialTime = std::chrono::steady_clock::now();
        if(shouldCancel) // Filter has been cancelled
        {
          fclose(f);
          return 1;
        }
      }
    }
    voxelId++;
  }
  fprintf(f, "\n**\n** ----------------------------------------------------------------\n**\n");

  // Close the file
  fclose(f);
  return err;
}

int32 writeMaster(const std::string& file, const std::string& jobName, const std::string& filePrefix)
{
  int32 err = 0;
  FILE* f = fopen(file.c_str(), "wb");
  if(nullptr == f)
  {
    return -1;
  }

  fprintf(f, "*Heading\n");
  fprintf(f, "%s\n", jobName.c_str());
  fprintf(f, "** Job name : %s\n", jobName.c_str());
  fprintf(f, "*Preprint, echo = NO, model = NO, history = NO, contact = NO\n");
  fprintf(f, "**\n** ----------------------------Geometry----------------------------\n**\n");
  fprintf(f, "*Include, Input = %s\n", (filePrefix + "_nodes.inp").c_str());
  fprintf(f, "*Include, Input = %s\n", (filePrefix + "_elems.inp").c_str());
  fprintf(f, "*Include, Input = %s\n", (filePrefix + "_elset.inp").c_str());
  fprintf(f, "*Include, Input = %s\n", (filePrefix + "_sects.inp").c_str());
  fprintf(f, "**\n** ----------------------------------------------------------------\n**\n");

  // Close the file
  fclose(f);
  return err;
}

int32 writeSects(const std::string& file, const Int32Array& featureIds, int32 hourglassStiffness)
{
  int32 err = 0;
  FILE* f = fopen(file.c_str(), "wb");
  if(nullptr == f)
  {
    return -1;
  }
  fprintf(f, "** ----------------------------------------------------------------\n**\n** Each section is a separate grain\n");

  // find total number of Grain Ids
  int32 maxGrainId = *std::max_element(featureIds.cbegin(), featureIds.cend());

  // We are now defining the sections, which is for each grain
  int32 grain = 1;
  while(grain <= maxGrainId)
  {
    fprintf(f, "** Section: Grain%d\n", grain);
    fprintf(f, "*Solid Section, elset=Grain%d_set, material=Grain_Mat%d\n", grain, grain);
    fprintf(f, "*Hourglass Stiffness\n%d\n", hourglassStiffness);
    fprintf(f, "** --------------------------------------\n");
    grain++;
  }
  fprintf(f, "**\n** ----------------------------------------------------------------\n**\n");

  // Close the file
  fclose(f);
  return err;
}

void deleteFile(const std::vector<std::unique_ptr<AtomicFile>>& fileList)
{
  for(const auto& atomicFile : fileList)
  {
    atomicFile->removeTempFile();
  }
}
} // namespace

// -----------------------------------------------------------------------------
WriteAbaqusHexahedron::WriteAbaqusHexahedron(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             WriteAbaqusHexahedronInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteAbaqusHexahedron::~WriteAbaqusHexahedron() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteAbaqusHexahedron::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void WriteAbaqusHexahedron::sendMessage(const std::string& message)
{
  m_MessageHandler(IFilter::Message::Type::Info, message);
}

// -----------------------------------------------------------------------------
Result<> WriteAbaqusHexahedron::operator()()
{
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  Vec3<usize> cDims = imageGeom.getDimensions();
  usize pDims[3] = {cDims[0] + 1, cDims[1] + 1, cDims[2] + 1};
  Vec3<float32> origin = imageGeom.getOrigin();
  Vec3<float32> spacing = imageGeom.getSpacing();
  usize totalPoints = imageGeom.getNumberOfCells();

  // Create file names
  std::vector<std::unique_ptr<AtomicFile>> fileList = {};
  fileList.push_back(std::make_unique<AtomicFile>(m_InputValues->OutputPath.string() + "/" + m_InputValues->FilePrefix + "_nodes.inp"));
  fileList.push_back(std::make_unique<AtomicFile>(m_InputValues->OutputPath.string() + "/" + m_InputValues->FilePrefix + "_elems.inp"));
  fileList.push_back(std::make_unique<AtomicFile>(m_InputValues->OutputPath.string() + "/" + m_InputValues->FilePrefix + "_sects.inp"));
  fileList.push_back(std::make_unique<AtomicFile>(m_InputValues->OutputPath.string() + "/" + m_InputValues->FilePrefix + "_elset.inp"));
  fileList.push_back(std::make_unique<AtomicFile>(m_InputValues->OutputPath.string() + "/" + m_InputValues->FilePrefix + ".inp"));

  int32 err = writeNodes(this, fileList[0]->tempFilePath().string(), cDims.data(), origin.data(), spacing.data(), getCancel()); // Nodes file
  if(err < 0)
  {
    return MakeErrorResult(-1113, fmt::format("Error writing output nodes file '{}'", fileList[0]->tempFilePath().string()));
  }
  if(getCancel()) // Filter has been cancelled
  {
    deleteFile(fileList); // delete files
    return {};
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Writing Sections (File 1/5) Complete");

  err = writeElems(this, fileList[1]->tempFilePath().string(), cDims.data(), pDims, getCancel()); // Elements file
  if(err < 0)
  {
    return MakeErrorResult(-1114, fmt::format("Error writing output elems file '{}'", fileList[1]->tempFilePath().string()));
  }
  if(getCancel()) // Filter has been cancelled
  {
    deleteFile(fileList); // delete files
    return {};
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Writing Sections (File 2/5) Complete");

  err = writeSects(fileList[2]->tempFilePath().string(), featureIds, m_InputValues->HourglassStiffness); // Sections file
  if(err < 0)
  {
    return MakeErrorResult(-1115, fmt::format("Error writing output sects file '{}'", fileList[2]->tempFilePath().string()));
  }
  if(getCancel()) // Filter has been cancelled
  {
    deleteFile(fileList); // delete files
    return {};
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Writing Sections (File 3/5) Complete");

  err = writeElset(this, fileList[3]->tempFilePath().string(), totalPoints, featureIds, getCancel()); // Element set file
  if(err < 0)
  {
    return MakeErrorResult(-1116, fmt::format("Error writing output elset file '{}'", fileList[3]->tempFilePath().string()));
  }
  if(getCancel()) // Filter has been cancelled
  {
    deleteFile(fileList); // delete files
    return {};
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Writing Sections (File 4/5) Complete");

  err = writeMaster(fileList[4]->tempFilePath().string(), m_InputValues->JobName, m_InputValues->FilePrefix); // Master file
  if(err < 0)
  {
    return MakeErrorResult(-1117, fmt::format("Error writing output master file '{}'", fileList[4]->tempFilePath().string()));
  }
  if(getCancel()) // Filter has been cancelled
  {
    deleteFile(fileList); // delete files
    return {};
  }
  m_MessageHandler(IFilter::Message::Type::Info, "Writing Sections (File 5/5) Complete");

  for(auto& file : fileList)
  {
    file->commit();
  }

  return {};
}
