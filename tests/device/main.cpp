// GoogleTest entry point with a Qt event loop: DeviceSession uses QTimer and
// queued signals, which need a QCoreApplication.
#include <QCoreApplication>

#include <gtest/gtest.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
