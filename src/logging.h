#pragma once


#include <chrono>

#include <QLoggingCategory>

namespace BazelProjectManager::Internal
{

Q_DECLARE_LOGGING_CATEGORY(BazelPluginLog)

class ScopedStopwatchLogger {
public:
  ScopedStopwatchLogger(std::string explanation);
  ~ScopedStopwatchLogger();

private:
  std::string explanation_;
  std::chrono::steady_clock::time_point startTime_;
};

}  // namespace BazelProjectManager::Internal
