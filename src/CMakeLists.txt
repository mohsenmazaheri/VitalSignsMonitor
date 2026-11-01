# Layered architecture - each layer only depends on the layers below it:
#
#   app       (Qt Quick UI, view models, sound)   -> ui, device, audit, core
#   audit     (audit trail file)                  -> core, C++20 standard library
#   ui        (shared Qt Quick items: waveform)   -> core, Qt Quick
#   device    (transports, I/O thread, sessions)  -> protocol, core, Qt Network/SerialPort
#   core      (domain: vitals, alarms, buffers)   -> C++20 standard library only
#   protocol  (wire format: frames, CRC, codec)   -> C++20 standard library only
add_subdirectory(protocol)
add_subdirectory(core)
add_subdirectory(audit)
add_subdirectory(device)
add_subdirectory(ui)
add_subdirectory(app)
