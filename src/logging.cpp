#include "logging.h"


namespace BazelProjectManager::Internal
{

Q_LOGGING_CATEGORY(BazelPluginLog, "qtc.bazel_plugin")

ScopedStopwatchLogger::ScopedStopwatchLogger(std::string explanation)
  : explanation_{std::move(explanation)},
    startTime_{std::chrono::steady_clock::now()}
{
}

ScopedStopwatchLogger::~ScopedStopwatchLogger() {
  // TODO: Make resolution a class template parameter.
  const auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - startTime_
  );
  qCInfo(BazelPluginLog) << explanation_.c_str() << ": " << elapsedMillis.count() << "ms";
}

}
