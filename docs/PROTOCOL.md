# VitalSigns Device Protocol (version 1)

A small binary protocol between the (simulated) bedside device and the monitor. It works the same
over TCP and serial ports. The source of truth is `src/protocol/` - this document explains it.

## Frame

All multi-byte fields are **little-endian**.

| Offset | Size | Field          | Notes                                         |
|-------:|-----:|----------------|-----------------------------------------------|
| 0      | 2    | Sync           | `0xAA 0x55`                                   |
| 2      | 1    | Version        | `1`                                           |
| 3      | 1    | Message type   | see below                                     |
| 4      | 2    | Sequence       | +1 per frame, wraps 65535 -> 0                |
| 6      | 2    | Payload length | N, at most 1024                               |
| 8      | N    | Payload        |                                               |
| 8+N    | 2    | CRC            | CRC-16/CCITT-FALSE over bytes 2 .. 8+N-1      |

CRC-16/CCITT-FALSE: polynomial `0x1021`, initial value `0xFFFF`, no reflection, no final XOR.
Check value for `"123456789"` is `0x29B1`.

## Message types

### `0x01` Heartbeat (1 Hz)

| Size | Field    | Unit |
|-----:|----------|------|
| 4    | uptime   | ms   |

### `0x10` Numerics (1 Hz)

| Size | Field      | Unit | Invalid value |
|-----:|------------|------|---------------|
| 2    | heart rate | bpm  | `0xFFFF`      |
| 1    | SpO2       | %    | `0xFF`        |
| 1    | flags      |      | bit 0 = ECG leads off, bit 1 = SpO2 probe off |

### `0x20` ECG waveform (every 40 ms)

| Size  | Field        | Unit |
|------:|--------------|------|
| 2     | sample rate  | Hz   |
| 2     | sample count | n    |
| 2 x n | samples      | int16, 1 LSB = 1 uV |

The payload length must equal `4 + 2 * n`, otherwise the message is rejected.

## Receiver rules (implemented in `FrameDecoder` and `DeviceSession`)

1. Search for `0xAA 0x55`; bytes before it are discarded and counted.
2. Reject a header with an unknown version or a length above 1024 **before** waiting for the
   payload, so a corrupted length can never stall the stream.
3. On a CRC mismatch, drop **one byte** and search again (re-synchronisation).
4. A jump in the sequence number is counted as a gap, and the skipped numbers as lost frames
   (a frame dropped for a CRC error is also a lost frame); a reconnect starts a new sequence.
5. Unknown message types are ignored (forward compatibility) and counted as malformed.
6. Any valid message resets the 3 s watchdog; if it expires, the link is reported as lost.
