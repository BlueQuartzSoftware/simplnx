#include "AlignSections.hpp"

#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <chrono>

using namespace nx::core;

namespace
{
// -----------------------------------------------------------------------------
template <typename T>
class AlignSectionsTransferDataImpl
{
public:
  AlignSectionsTransferDataImpl() = delete;
  AlignSectionsTransferDataImpl(const AlignSectionsTransferDataImpl&) = default;     // Copy Constructor Default Implemented
  AlignSectionsTransferDataImpl(AlignSectionsTransferDataImpl&&) noexcept = default; // Move Constructor Default Implemented

  AlignSectionsTransferDataImpl(AlignSections* filter, SizeVec3 dims, std::vector<int64_t> xShifts, std::vector<int64_t> yShifts, IDataArray& dataArray)
  : m_Filter(filter)
  , m_Dims(std::move(dims))
  , m_Xshifts(std::move(xShifts))
  , m_Yshifts(std::move(yShifts))
  , m_DataArray(static_cast<DataArray<T>&>(dataArray))
  {
  }

  ~AlignSectionsTransferDataImpl() = default;

  AlignSectionsTransferDataImpl& operator=(const AlignSectionsTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  AlignSectionsTransferDataImpl& operator=(AlignSectionsTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  void operator()() const
  {
    T var = static_cast<T>(0);

    auto start = std::chrono::steady_clock::now();

    for(size_t i = 1; i < m_Dims[2]; i++)
    {
      auto now = std::chrono::steady_clock::now();
      // Only send updates every 1 second
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        std::string message = fmt::format("Processing {}: {}% completed", m_DataArray.getName(), static_cast<int32>(100 * (static_cast<float>(i) / static_cast<float>(m_Dims[2]))));
        m_Filter->updateProgress(message);
        start = std::chrono::steady_clock::now();
      }
      if(m_Filter->getCancel())
      {
        return;
      }
      size_t slice = (m_Dims[2] - 1) - i;
      for(size_t yIndex = 0; yIndex < m_Dims[1]; yIndex++)
      {
        for(size_t xIndex = 0; xIndex < m_Dims[0]; xIndex++)
        {
          int64_t xspot = 0;
          int64_t yspot = 0;
          if(m_Yshifts[i] >= 0)
          {
            yspot = static_cast<int64_t>(yIndex);
          }
          else if(m_Yshifts[i] < 0)
          {
            yspot = static_cast<int64_t>(m_Dims[1]) - 1 - static_cast<int64_t>(yIndex);
          }
          if(m_Xshifts[i] >= 0)
          {
            xspot = static_cast<int64_t>(xIndex);
          }
          else if(m_Xshifts[i] < 0)
          {
            xspot = static_cast<int64_t>(m_Dims[0]) - 1 - static_cast<int64_t>(xIndex);
          }
          int64_t newPosition = (slice * m_Dims[0] * m_Dims[1]) + (yspot * m_Dims[0]) + xspot;
          int64_t currentPosition = (slice * m_Dims[0] * m_Dims[1]) + ((yspot + m_Yshifts[i]) * m_Dims[0]) + (xspot + m_Xshifts[i]);
          if((yspot + m_Yshifts[i]) >= 0 && (yspot + m_Yshifts[i]) <= static_cast<int64_t>(m_Dims[1]) - 1 && (xspot + m_Xshifts[i]) >= 0 &&
             (xspot + m_Xshifts[i]) <= static_cast<int64_t>(m_Dims[0]) - 1)
          {
            m_DataArray.copyTuple(static_cast<size_t>(currentPosition), static_cast<size_t>(newPosition));
          }
          if((yspot + m_Yshifts[i]) < 0 || (yspot + m_Yshifts[i]) > static_cast<int64_t>(m_Dims[1] - 1) || (xspot + m_Xshifts[i]) < 0 || (xspot + m_Xshifts[i]) > static_cast<int64_t>(m_Dims[0]) - 1)
          {
            m_DataArray.initializeTuple(newPosition, var);
          }
        }
      }
    }
  }

private:
  AlignSections* m_Filter = nullptr;
  SizeVec3 m_Dims;
  std::vector<int64_t> m_Xshifts;
  std::vector<int64_t> m_Yshifts;
  nx::core::DataArray<T>& m_DataArray;
};
} // namespace

// -----------------------------------------------------------------------------
AlignSections::AlignSections(DataStructure& data, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(data)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignSections::~AlignSections() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AlignSections::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void AlignSections::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}

// -----------------------------------------------------------------------------
Result<> AlignSections::execute(const SizeVec3& udims)
{
  std::array<int64, 3> dims = {static_cast<int64_t>(udims[0]), static_cast<int64_t>(udims[1]), static_cast<int64_t>(udims[2])};
  std::vector<int64_t> xShifts(dims[2], 0);
  std::vector<int64_t> yShifts(dims[2], 0);

  // Find the voxel shifts that need to happen
  Result<> foundShiftsResults = findShifts(xShifts, yShifts);
  if(foundShiftsResults.invalid())
  {
    return foundShiftsResults;
  }

  if(getCancel())
  {
    return {};
  }

  // Now Adjust the actual DataArrays
  std::vector<DataPath> selectedCellArrays = getSelectedDataPaths();

  ParallelTaskAlgorithm taskRunner;

  for(const auto& cellArrayPath : selectedCellArrays)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHandler(fmt::format("Updating DataArray '{}'", cellArrayPath.toString()));
    auto& cellArray = m_DataStructure.getDataRefAs<IDataArray>(cellArrayPath);

    ExecuteParallelFunction<AlignSectionsTransferDataImpl>(cellArray.getDataType(), taskRunner, this, udims, xShifts, yShifts, cellArray);
  }
  // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.
  taskRunner.wait();

  return {};
}

// -----------------------------------------------------------------------------
Result<> AlignSections::readDream3dShiftsFile(const std::filesystem::path& file, int64 zDim, std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) const
{
  std::ifstream inFile;
  inFile.open(file);

  int64 slice = 0;
  int64 newXShift = 0, newYShift = 0;
  // These are ignored from the input file since DREAM.3D wrote the file
  int64 slice2 = 0;
  float32 xShift = 0.0f;
  float32 yShift = 0.0f;

  for(int64 iter = 1; iter < zDim; iter++)
  {
    std::string line;
    std::getline(inFile, line);
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while(iss >> token)
    {
      tokens.push_back(token);
    }
    if(tokens.size() < 6)
    {
      std::string message = fmt::format(
          "Error reading line {} of Input Shifts File with file path '{}'. 6 columns in the format <Slice_A,Slice_B,New X Shift,New Y Shift,X Shift, Y Shift> are required but only {} were found",
          iter, file.string(), tokens.size());
      inFile.close();
      return MakeErrorResult(-84750, message);
    }
    std::istringstream temp(line);
    iss.swap(temp); // reset the stream to beginning so we can read in the formatted tokens
    iss >> slice >> slice2 >> newXShift >> newYShift >> xShift >> yShift;
    xShifts[iter] = xShifts[iter - 1] + newXShift;
    yShifts[iter] = yShifts[iter - 1] + newYShift;
  }
  inFile.close();
  return {};
}

// -----------------------------------------------------------------------------
Result<> AlignSections::readUserShiftsFile(const std::filesystem::path& file, int64 zDim, std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts) const
{
  int64 slice = 0;
  int64 newXShift = 0, newYShift = 0;

  std::ifstream inFile;
  inFile.open(file);
  for(int64 iter = 1; iter < zDim; iter++)
  {
    std::string line;
    std::getline(inFile, line);
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;
    while(iss >> token)
    {
      tokens.push_back(token);
    }
    if(tokens.size() < 3)
    {
      std::string message = fmt::format("Error reading line {} of Input Shifts File with file path '{}'. 3 columns in the format <Slice_Number,X Shift,Y Shift> are required but only {} were found",
                                        iter, file.string(), tokens.size());
      inFile.close();
      return MakeErrorResult(-84750, message);
    }
    std::istringstream temp(line);
    iss.swap(temp); // reset the stream to beginning so we can read in the formatted tokens
    inFile >> slice >> newXShift >> newYShift;
    xShifts[iter] = xShifts[iter - 1] + newXShift;
    yShifts[iter] = yShifts[iter - 1] + newYShift;
  }
  inFile.close();
  return {};
}
