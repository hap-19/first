# DLL Monitor Example

This directory shows how to monitor calls from an executable to a DLL by using a proxy library.

## Building

This example targets Windows and requires the `mingw-w64` toolchain.

```bash
make
```

This will build:

- `original.dll` – the real library with an `add` function.
- `monitor.dll` – a proxy DLL that logs calls to `add` and forwards them to `original.dll`.
- `app.exe` – a tiny program that links against `monitor.dll` and calls the `add` function.

## Running

Place `original.dll`, `monitor.dll`, and `app.exe` in the same directory and run `app.exe`. The console output shows the monitored calls.
