#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Exposes the protected message-routing entry point so tests can drive it directly without
 * needing a filter that emits a particular message during execution.
 */
class TestablePipelineFilter : public PipelineFilter
{
public:
  TestablePipelineFilter()
  : PipelineFilter(nullptr)
  {
  }

  using PipelineFilter::notifyFilterMessage;
};

struct UpdateRecord
{
  int32 index = -1;
  std::string message;
};

struct ProgressRecord
{
  int32 index = -1;
  int32 progress = -1;
  std::string message;
};
} // namespace

TEST_CASE("Simplnx::PipelineFilter::Routes info text to the update signal", "[Simplnx][PipelineFilter]")
{
  TestablePipelineFilter pipelineFilter;

  std::vector<UpdateRecord> updates;
  pipelineFilter.getFilterUpdateSignal().connect([&updates](AbstractPipelineNode*, int32 index, const std::string& message) { updates.push_back(UpdateRecord{index, message}); });

  pipelineFilter.notifyFilterMessage(IFilter::Message{IFilter::Message::Type::Info, "Reading slice 4"});

  REQUIRE(updates.size() == 1);
  REQUIRE(updates[0].message == "Reading slice 4");
}

TEST_CASE("Simplnx::PipelineFilter::Routes progress without a downcast", "[Simplnx][PipelineFilter]")
{
  TestablePipelineFilter pipelineFilter;

  std::vector<ProgressRecord> progressRecords;
  pipelineFilter.getFilterProgressSignal().connect(
      [&progressRecords](AbstractPipelineNode*, int32 index, int32 progress, const std::string& message) { progressRecords.push_back(ProgressRecord{index, progress, message}); });

  pipelineFilter.notifyFilterMessage(IFilter::Message{IFilter::Message::Type::Progress, "Analyzing data", 35});

  REQUIRE(progressRecords.size() == 1);
  REQUIRE(progressRecords[0].progress == 35);
  REQUIRE(progressRecords[0].message == "Analyzing data");
}

TEST_CASE("Simplnx::PipelineFilter::Warning text is not discarded", "[Simplnx][PipelineFilter]")
{
  TestablePipelineFilter pipelineFilter;

  std::vector<UpdateRecord> updates;
  pipelineFilter.getFilterUpdateSignal().connect([&updates](AbstractPipelineNode*, int32 index, const std::string& message) { updates.push_back(UpdateRecord{index, message}); });

  std::vector<FaultState> faults;
  pipelineFilter.getFilterFaultSignal().connect([&faults](AbstractPipelineNode*, int32, FaultState state) { faults.push_back(state); });

  pipelineFilter.notifyFilterMessage(IFilter::Message{IFilter::Message::Type::Warning, "Phase 3 has an unknown crystal structure"});

  // The fault state must still be raised.
  REQUIRE(faults.size() == 1);
  REQUIRE(faults[0] == FaultState::Warnings);

  // ...and the text must reach observers rather than being thrown away.
  REQUIRE(updates.size() == 1);
  REQUIRE(updates[0].message == "Phase 3 has an unknown crystal structure");
}

TEST_CASE("Simplnx::PipelineFilter::Error text is not discarded", "[Simplnx][PipelineFilter]")
{
  TestablePipelineFilter pipelineFilter;

  std::vector<UpdateRecord> updates;
  pipelineFilter.getFilterUpdateSignal().connect([&updates](AbstractPipelineNode*, int32 index, const std::string& message) { updates.push_back(UpdateRecord{index, message}); });

  std::vector<FaultState> faults;
  pipelineFilter.getFilterFaultSignal().connect([&faults](AbstractPipelineNode*, int32, FaultState state) { faults.push_back(state); });

  pipelineFilter.notifyFilterMessage(IFilter::Message{IFilter::Message::Type::Error, "Could not open file"});

  REQUIRE(faults.size() == 1);
  REQUIRE(faults[0] == FaultState::Errors);

  REQUIRE(updates.size() == 1);
  REQUIRE(updates[0].message == "Could not open file");
}
