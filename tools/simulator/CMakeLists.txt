# Synthetic device simulators:
#   engine/  vsm_simulation library (patient model, scenarios, frames, TCP/serial output)
#   cli/     VitalSignsSimulator         - console program
#   gui/     VitalSignsPatientSimulator  - front panel that looks like a hardware patient simulator
add_subdirectory(engine)
add_subdirectory(cli)
add_subdirectory(gui)
