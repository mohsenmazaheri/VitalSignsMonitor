#pragma once

#include "vsm/core/AuditLog.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>

namespace vsm::audit {

using TimePoint = std::chrono::system_clock::time_point;

/// "2026-11-01T09:30:05.250Z" - always UTC, so files from different time zones compare.
[[nodiscard]] std::string formatTimestampUtc(TimePoint time);

/// "audit-2026-11-01.csv" - the UTC day the application started.
[[nodiscard]] std::string auditFileName(TimePoint time);

/// Quotes a CSV field if it contains a comma, a quote or a line break.
[[nodiscard]] std::string csvEscape(std::string_view field);

/// "time_utc,event,actor,code,priority,detail" for one event, without line end.
[[nodiscard]] std::string formatCsvLine(TimePoint time, const core::AuditEvent& event);

/// Appends audit events to a CSV file. Each line is flushed at once, so a crash
/// loses nothing that was already reported. The clock is injected for the tests.
///
/// A file that cannot be written never stops the monitor: record() then does
/// nothing and isOk() returns false, so that the user interface can warn.
class FileAuditLog final : public core::IAuditLog {
public:
    using WallClock = std::function<TimePoint()>;

    explicit FileAuditLog(std::filesystem::path file, WallClock clock = {});

    void record(const core::AuditEvent& event) override;

    [[nodiscard]] bool isOk() const noexcept { return m_stream.good(); }
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return m_path; }

private:
    std::filesystem::path m_path;
    WallClock m_clock;
    std::ofstream m_stream;
};

} // namespace vsm::audit
