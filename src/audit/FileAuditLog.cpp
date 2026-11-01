#include "vsm/audit/FileAuditLog.h"

#include <iomanip>
#include <sstream>
#include <system_error>

namespace vsm::audit {

namespace {

constexpr std::string_view kCsvHeader = "time_utc,event,actor,code,priority,detail";

[[nodiscard]] std::chrono::year_month_day utcDay(TimePoint time)
{
    return std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(time)};
}

void writeDate(std::ostringstream& out, std::chrono::year_month_day day)
{
    out << std::setfill('0') << std::setw(4) << static_cast<int>(day.year()) << '-' << std::setw(2)
        << static_cast<unsigned>(day.month()) << '-' << std::setw(2) << static_cast<unsigned>(day.day());
}

} // namespace

std::string formatTimestampUtc(TimePoint time)
{
    using namespace std::chrono;
    const auto dayStart = floor<days>(time);
    const hh_mm_ss timeOfDay{floor<milliseconds>(time - dayStart)};

    std::ostringstream out;
    writeDate(out, year_month_day{dayStart});
    out << 'T' << std::setw(2) << timeOfDay.hours().count() << ':' << std::setw(2) << timeOfDay.minutes().count() << ':'
        << std::setw(2) << timeOfDay.seconds().count() << '.' << std::setw(3) << timeOfDay.subseconds().count() << 'Z';
    return out.str();
}

std::string auditFileName(TimePoint time)
{
    std::ostringstream out;
    out << "audit-";
    writeDate(out, utcDay(time));
    out << ".csv";
    return out.str();
}

std::string csvEscape(std::string_view field)
{
    if (field.find_first_of(",\"\r\n") == std::string_view::npos) {
        return std::string(field);
    }
    std::string escaped = "\"";
    for (const char c : field) {
        if (c == '"') {
            escaped += '"';
        }
        escaped += c;
    }
    escaped += '"';
    return escaped;
}

std::string formatCsvLine(TimePoint time, const core::AuditEvent& event)
{
    std::string line = formatTimestampUtc(time);
    line += ',';
    line += core::toString(event.type);
    line += ',';
    line += core::isOperatorAction(event.type) ? "operator" : "system";
    line += ',';
    line += csvEscape(event.code);
    line += ',';
    if (event.priority) {
        line += core::toString(*event.priority);
    }
    line += ',';
    line += csvEscape(event.detail);
    return line;
}

FileAuditLog::FileAuditLog(std::filesystem::path file, WallClock clock)
    : m_path(std::move(file))
    , m_clock(clock ? std::move(clock) : WallClock([] { return std::chrono::system_clock::now(); }))
{
    std::error_code ignored; // a failed directory creation shows up as a failed open below
    if (m_path.has_parent_path()) {
        std::filesystem::create_directories(m_path.parent_path(), ignored);
    }
    const bool isNewFile = !std::filesystem::exists(m_path, ignored) || std::filesystem::file_size(m_path, ignored) == 0;

    m_stream.open(m_path, std::ios::binary | std::ios::app);
    if (m_stream && isNewFile) {
        m_stream << kCsvHeader << '\n' << std::flush;
    }
}

void FileAuditLog::record(const core::AuditEvent& event)
{
    if (m_stream) {
        m_stream << formatCsvLine(m_clock(), event) << '\n' << std::flush;
    }
}

} // namespace vsm::audit
