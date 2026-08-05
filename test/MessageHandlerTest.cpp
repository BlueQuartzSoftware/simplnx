#include "simplnx/Filter/IFilter.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Records every message emitted through a MessageHandler so tests can assert on them.
 */
class MessageRecorder
{
public:
  IFilter::MessageHandler createHandler()
  {
    return IFilter::MessageHandler{[this](const IFilter::Message& message) { m_Messages.push_back(message); }};
  }

  usize size() const
  {
    return m_Messages.size();
  }

  const IFilter::Message& at(usize index) const
  {
    return m_Messages.at(index);
  }

  const IFilter::Message& only() const
  {
    REQUIRE(m_Messages.size() == 1);
    return m_Messages.front();
  }

private:
  std::vector<IFilter::Message> m_Messages;
};
} // namespace

TEST_CASE("Simplnx::MessageHandler::Info", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  handler.sendInfoMessage("Initializing working grid");

  const IFilter::Message& message = recorder.only();
  REQUIRE(message.type == IFilter::Message::Type::Info);
  REQUIRE(message.message == "Initializing working grid");
  REQUIRE(message.progress == -1);
}

TEST_CASE("Simplnx::MessageHandler::Debug Warning Error", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  handler.sendDebugMessage("cache miss");
  handler.sendWarningMessage("Phase 3 has an unknown crystal structure");
  handler.sendErrorMessage("Could not open file");

  REQUIRE(recorder.size() == 3);

  REQUIRE(recorder.at(0).type == IFilter::Message::Type::Debug);
  REQUIRE(recorder.at(0).message == "cache miss");

  REQUIRE(recorder.at(1).type == IFilter::Message::Type::Warning);
  REQUIRE(recorder.at(1).message == "Phase 3 has an unknown crystal structure");

  REQUIRE(recorder.at(2).type == IFilter::Message::Type::Error);
  REQUIRE(recorder.at(2).message == "Could not open file");
}

TEST_CASE("Simplnx::MessageHandler::sendProgressCount renders counts", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  handler.sendProgressCount("Processing tuples", 35, 100);

  const IFilter::Message& message = recorder.only();
  REQUIRE(message.type == IFilter::Message::Type::Progress);
  REQUIRE(message.message == "Processing tuples: 35/100");
  // The bar still receives an integer percent even though the text shows counts.
  REQUIRE(message.progress == 35);
}

TEST_CASE("Simplnx::MessageHandler::sendProgressPercent renders decimals", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  // Two decimal places by default, for loops whose counts are too large to read.
  handler.sendProgressPercent("Analyzing voxels", 1, 3);

  const IFilter::Message& message = recorder.only();
  REQUIRE(message.type == IFilter::Message::Type::Progress);
  REQUIRE(message.message == "Analyzing voxels: 33.33%");
  REQUIRE(message.progress == 33);
}

TEST_CASE("Simplnx::MessageHandler::sendProgressPercent honors the decimal count", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  handler.sendProgressPercent("Analyzing voxels", 1, 3, 1);
  REQUIRE(recorder.at(0).message == "Analyzing voxels: 33.3%");

  handler.sendProgressPercent("Analyzing voxels", 1, 3, 0);
  REQUIRE(recorder.at(1).message == "Analyzing voxels: 33%");
}

TEST_CASE("Simplnx::MessageHandler::Progress with a zero denominator is safe", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  REQUIRE_NOTHROW(handler.sendProgressCount("Nothing to do", 0, 0));
  REQUIRE(recorder.at(0).message == "Nothing to do: 0/0");
  REQUIRE(recorder.at(0).progress == 0);

  REQUIRE_NOTHROW(handler.sendProgressPercent("Nothing to do", 0, 0));
  REQUIRE(recorder.at(1).message == "Nothing to do: 0.00%");
  REQUIRE(recorder.at(1).progress == 0);
}

TEST_CASE("Simplnx::MessageHandler::Progress clamps an overshooting numerator", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  handler.sendProgressPercent("Overshooting", 15, 10);

  // The text reports what actually happened; the bar value stays in range.
  REQUIRE(recorder.at(0).message == "Overshooting: 150.00%");
  REQUIRE(recorder.at(0).progress == 100);
}

TEST_CASE("Simplnx::MessageHandler::sendProgressMessage passes rendered text through", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  // The low-level primitive: caller has already rendered the text and computed the bar value.
  handler.sendProgressMessage("Iteration 4 of 9 (converging)", 44);

  const IFilter::Message& message = recorder.only();
  REQUIRE(message.type == IFilter::Message::Type::Progress);
  REQUIRE(message.message == "Iteration 4 of 9 (converging)");
  REQUIRE(message.progress == 44);
}

TEST_CASE("Simplnx::MessageHandler::Generic sendMessage", "[Simplnx][MessageHandler]")
{
  MessageRecorder recorder;
  IFilter::MessageHandler handler = recorder.createHandler();

  handler.sendMessage(IFilter::Message::Type::Warning, "explicit type");

  const IFilter::Message& message = recorder.only();
  REQUIRE(message.type == IFilter::Message::Type::Warning);
  REQUIRE(message.message == "explicit type");
  REQUIRE(message.progress == -1);
}

TEST_CASE("Simplnx::MessageHandler::Empty handler does not crash", "[Simplnx][MessageHandler]")
{
  IFilter::MessageHandler handler;

  REQUIRE_NOTHROW(handler.sendInfoMessage("no callback installed"));
  REQUIRE_NOTHROW(handler.sendProgressCount("no callback installed", 1, 2));
  REQUIRE_NOTHROW(handler.sendProgressPercent("no callback installed", 1, 2));
  REQUIRE_NOTHROW(handler.sendMessage(IFilter::Message::Type::Error, "no callback installed"));
}
