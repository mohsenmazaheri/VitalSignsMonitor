#include "vsm/audit/FileAuditLog.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace vsm;
using namespace std::chrono;

namespace {

// 2026-11-01 09:30:05.250 UTC
audit::TimePoint fixedTime()
{
    return sys_days{year{2026} / November / 1} + hours{9} + minutes{30} + seconds{5} + milliseconds{250};
}

core::AuditEvent raised()
{
    return {core::AuditEventType::AlarmRaised, "HR_HIGH", core::AlarmPriority::Medium, "HR high (> 120 bpm)"};
}

class FileAuditLogTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        folder = std::filesystem::temp_directory_path() / "vsm_audit_tests" / info->name();
        std::filesystem::remove_all(folder);
        file = folder / "audit.csv";
    }

    void TearDown() override { std::filesystem::remove_all(folder); }

    [[nodiscard]] std::vector<std::string> lines() const
    {
        std::ifstream in(file);
        std::vector<std::string> result;
        for (std::string line; std::getline(in, line);) {
            result.push_back(line);
        }
        return result;
    }

    std::filesystem::path folder;
    std::filesystem::path file;
};

} // namespace

TEST(AuditFormat, TimestampIsUtcWithMilliseconds)
{
    EXPECT_EQ(audit::formatTimestampUtc(fixedTime()), "2026-11-01T09:30:05.250Z");
}

TEST(AuditFormat, TimestampPadsSmallValues)
{
    const audit::TimePoint time = sys_days{year{2027} / January / 2} + hours{3} + minutes{4} + seconds{5} + milliseconds{6};
    EXPECT_EQ(audit::formatTimestampUtc(time), "2027-01-02T03:04:05.006Z");
}

TEST(AuditFormat, FileNameContainsTheUtcDay)
{
    EXPECT_EQ(audit::auditFileName(fixedTime()), "audit-2026-11-01.csv");
}

TEST(AuditFormat, PlainFieldsAreNotQuoted)
{
    EXPECT_EQ(audit::csvEscape("HR high (> 120 bpm)"), "HR high (> 120 bpm)");
}

TEST(AuditFormat, SpecialCharactersAreQuoted)
{
    EXPECT_EQ(audit::csvEscape("a,b"), "\"a,b\"");
    EXPECT_EQ(audit::csvEscape("say \"hi\""), "\"say \"\"hi\"\"\"");
    EXPECT_EQ(audit::csvEscape("two\nlines"), "\"two\nlines\"");
}

TEST(AuditFormat, AlarmLineHasAllColumns)
{
    EXPECT_EQ(audit::formatCsvLine(fixedTime(), raised()),
              "2026-11-01T09:30:05.250Z,ALARM_RAISED,system,HR_HIGH,Medium,HR high (> 120 bpm)");
}

TEST(AuditFormat, OperatorEventWithoutAlarmLeavesColumnsEmpty)
{
    const core::AuditEvent event{core::AuditEventType::ConnectRequested, {}, std::nullopt, "127.0.0.1:5555"};
    EXPECT_EQ(audit::formatCsvLine(fixedTime(), event),
              "2026-11-01T09:30:05.250Z,CONNECT_REQUESTED,operator,,,127.0.0.1:5555");
}

TEST_F(FileAuditLogTest, CreatesFolderAndWritesHeaderAndEvents)
{
    audit::FileAuditLog log(file, [] { return fixedTime(); });
    log.record(raised());

    EXPECT_TRUE(log.isOk());
    const auto content = lines();
    ASSERT_EQ(content.size(), 2U);
    EXPECT_EQ(content[0], "time_utc,event,actor,code,priority,detail");
    EXPECT_EQ(content[1], "2026-11-01T09:30:05.250Z,ALARM_RAISED,system,HR_HIGH,Medium,HR high (> 120 bpm)");
}

TEST_F(FileAuditLogTest, EachEventIsOnDiskImmediately)
{
    audit::FileAuditLog log(file, [] { return fixedTime(); });
    log.record(raised());

    // Read while the log is still open: a crash now would lose nothing.
    EXPECT_EQ(lines().size(), 2U);
}

TEST_F(FileAuditLogTest, SecondRunAppendsWithoutSecondHeader)
{
    {
        audit::FileAuditLog log(file, [] { return fixedTime(); });
        log.record(raised());
    }
    {
        audit::FileAuditLog log(file, [] { return fixedTime(); });
        log.record(raised());
    }
    EXPECT_EQ(lines().size(), 3U);
}

TEST_F(FileAuditLogTest, UnwritableFileIsReportedAndDoesNotThrow)
{
    std::filesystem::create_directories(file); // a folder where the file should be

    audit::FileAuditLog log(file);

    EXPECT_FALSE(log.isOk());
    EXPECT_NO_THROW(log.record(raised()));
}

TEST_F(FileAuditLogTest, DefaultClockIsTheSystemClock)
{
    const auto before = floor<seconds>(system_clock::now());
    {
        audit::FileAuditLog log(file);
        log.record(raised());
    }
    const auto content = lines();
    ASSERT_EQ(content.size(), 2U);
    // Same day as "now" (a run across UTC midnight would still match one of the two).
    const std::string day = audit::formatTimestampUtc(before).substr(0, 10);
    const std::string dayAfter = audit::formatTimestampUtc(system_clock::now()).substr(0, 10);
    const std::string logged = content[1].substr(0, 10);
    EXPECT_TRUE(logged == day || logged == dayAfter) << logged;
}
