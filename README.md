# Pulse

A Linux system resource monitor written in **C++23**. Pulse reads `/proc` directly to display overall CPU utilization, system memory usage, and the processes using the most resident memory.

The project focuses on Linux system programming, explicit ownership, testable components, and a separation between metric collection and presentation. The current front end is a CLI; the architecture is intended to support a TUI or GUI later.

## Features

- Overall CPU utilization calculated from consecutive `/proc/stat` samples.
- Used and total system memory in GiB, with a usage percentage.
- Up to ten processes ranked by resident memory, showing PID, name, state, CPU percentage, and memory in MiB.
- Per-process CPU accounting with PID/start-time checks to handle PID reuse between samples.
- Periodic updates with a default interval of one second.
- Terminal refresh once per successful update.
- Cooperative shutdown on `Ctrl+C` (`SIGINT`) or `SIGTERM`.

## Requirements

- Linux with `/proc` mounted and readable. Ubuntu 24.04 is the target environment.
- A C++ compiler and standard library supporting C++23, including `std::expected`.
- Bazel **9.3.0**, as specified in `.bazelversion`. A version-aware launcher such as Bazelisk can select it.
- An ANSI-compatible terminal for in-place display updates.

Bazel resolves `rules_cc` and GoogleTest/GoogleMock through Bzlmod using `MODULE.bazel`. The first build needs access to any dependencies that are not already cached.

## Build and run

From the repository root:

```bash
bazel build //src:pulse
bazel run //src:pulse
```

After building, you can also run the executable directly:

```bash
./bazel-bin/src/pulse
```

Press **Ctrl+C** to stop. The application currently runs continuously; command-line options for the interval, process limit, or a one-time snapshot are not implemented.

### Reading the display

Illustrative output from a system with three collected processes:

```text
CPU Usage: 12.50%
Memory Usage: 6.00 / 16.00 GiB (37.50%)

TOP 3 PROCESSES BY MEMORY
PID       NAME                  STATE    CPU%       MEMORY
1240      browser               S        12.50%     850.00 MiB
2050      editor                S        3.00%      320.00 MiB
3100      pulse                 R        1.00%      12.50 MiB
```

The first successful update omits overall CPU usage because calculating it requires two readings. Process CPU is `N/A` until a valid comparison exists, including for newly observed processes. Missing process memory also appears as `N/A`; a measured zero remains distinct from unavailable data.

The presenter clears the visible screen and moves the cursor home before each successful update. Terminal scrollback may retain previous output. Run Pulse in a terminal rather than a plain output panel; redirected output includes ANSI escape sequences.

## How metrics are calculated

| Metric | Calculation / meaning |
| --- | --- |
| Overall CPU | `100 × (total_delta - idle_delta) / total_delta`, where total includes the eight stored CPU counters and idle includes `idle` plus `iowait` |
| Used memory | `MemTotal - MemAvailable` from `/proc/meminfo` |
| Memory percentage | `100 × used / total` |
| Process CPU | `100 × ((user_tick_delta + system_tick_delta) / ticks_per_second) / elapsed_seconds` |
| Process memory | Optional `VmRSS` from `/proc/<pid>/status` |

Overall CPU is normalized against aggregate CPU capacity. Process CPU uses one fully occupied logical CPU as 100%, so a multithreaded process can exceed 100%. The process tick rate comes from `sysconf(_SC_CLK_TCK)`, and elapsed time uses `std::chrono::steady_clock`.

Memory values reported as `kB` by these readers are treated as KiB. The CLI converts them to GiB for system memory and MiB for process memory. RSS can include shared pages; it is not a process's unique memory cost.

## Architecture

```text
PulseApplication                     periodic execution and cancellation
    |
MonitoringServiceImpl                collect metrics, then begin and present a frame
    |
    +-- CPU / memory / process monitors
    |       |
    |       +-- reader interfaces --> Linux /proc readers --> IFileReader
    |       +-- parsers, calculators, and sampling state
    |
    +-- IMetricsPresenter --> CliMetricsPresenterImpl --> terminal
```

`ApplicationFactory` constructs the concrete dependency graph and injects it into the application. Interfaces sit at collection and presentation boundaries; parsers and calculators operate on supplied text or typed values. Stateful monitors retain CPU baselines independently of display formatting.

Most dependencies use exclusive ownership through `std::unique_ptr`; the factory shares its file reader across Linux adapters. Recoverable sampling failures use `std::expected`, and invalid constructor arguments throw exceptions. Monitoring runs on the main thread while a `std::jthread` waits for shutdown signals and requests cooperative cancellation.

### Repository layout

```text
app/                 application lifecycle and dependency assembly
core/
  cpu/               CPU counters, parser, calculator, monitor, and interfaces
  memory/            memory data, parser, monitor, and interfaces
  process/           process metadata, CPU calculation, history, and ranking
  monitoring/        orchestration and presentation interfaces
  io/                file-reading interface
platform/linux/      file access and Linux /proc adapters
presentation/cli/    terminal formatting and frame refresh
src/                 executable entry point and signal handling
tests/               GoogleTest tests and GoogleMock collaborators
```

## Tests

Run all tests:

```bash
bazel test //tests/...
```

Run a focused component test:

```bash
bazel test //tests/cpu:cpu_usage_calculator_test
bazel test //tests/process:process_monitor_impl_test
bazel test //tests/monitoring:monitoring_service_impl_test
```

Tests use controlled inputs and mocked readers rather than asserting against live `/proc` values. Coverage includes parsing and numeric boundaries, sampling state, PID reuse, process ranking, error propagation, presentation, and cancellation. File access and process directory enumeration also use temporary filesystem fixtures. The process monitor accepts an injectable clock for deterministic elapsed-time tests.

## Code quality

The repository includes `.clang-format` and `.clang-tidy` configurations. Bazel enables C++23 with `-Wall`, `-Wextra`, and `-Wpedantic`.

To check formatting across project C++ files:

```bash
rg --files src app core platform presentation tests -g '*.cpp' -g '*.hpp' \
  | xargs clang-format --dry-run --Werror
```

Clang-Tidy 20 was used for the recent production-code checks. For example, from the repository root:

```bash
clang-tidy-20 src/main.cpp -- -std=c++23 -I.
```

Use a Clang-Tidy toolchain compatible with the selected C++23 standard library. A clean production-code analysis does not imply that test sources, sanitizers, or Coverity have also been checked.

## Current limitations

- Collection is sequential, so a displayed update is an approximation over a collection window rather than an atomic snapshot.
- The application stops on a monitor error. Some individual monitors can recover across calls, but the application does not currently retry failed ticks.
- The CPU calculator rejects decreasing counters, including I/O wait; accounting anomalies can therefore stop monitoring.
- Unreadable or malformed individual process records are skipped. Process visibility depends on permissions and the mounted `/proc` view.
- An in-progress scan or blocked output can delay shutdown; cancellation interrupts the wait between samples.
- Metrics are not designed to reflect container resource limits or quotas specifically.

## Possible next steps

- Command-line configuration and one-time snapshots with explicit CPU warm-up behavior.
- More detailed diagnostics and a policy for transient sampling failures.
- A structured snapshot model and additional presenters, such as JSON or a TUI.
- Additional metrics and performance measurements before introducing parallel collection.

For a deeper walkthrough and interview preparation, see [the project interview guide](PROJECT_INTERVIEW_GUIDE.md).
