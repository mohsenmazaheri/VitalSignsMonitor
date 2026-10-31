# vsm_core - domain model and alarm logic.
# Pure C++20: no Qt and no protocol dependency, so the safety-relevant alarm
# logic can be tested in isolation.
add_library(vsm_core STATIC
    include/vsm/core/VitalSigns.h
    include/vsm/core/Alarm.h
    include/vsm/core/AlarmLimits.h
    include/vsm/core/IAlarmRule.h
    include/vsm/core/AlarmRules.h
    include/vsm/core/AlarmEvaluator.h
    include/vsm/core/SweepBuffer.h
    Alarm.cpp
    AlarmLimits.cpp
    AlarmRules.cpp
    AlarmEvaluator.cpp
)
add_library(vsm::core ALIAS vsm_core)

target_include_directories(vsm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(vsm_core PUBLIC cxx_std_20)
vsm_set_warnings(vsm_core)
