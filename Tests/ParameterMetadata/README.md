# Isolated parameter regression harness

This macOS arm64 host queries the actual VST3 component/controller interfaces.
It processes silent offline buffers without creating an editor or opening an audio
device. The plug-in source and bundle are read-only inputs to this harness.

The fixtures use only ordinary `[param]` create, range and mode messages.
The host does not require any custom plug-in API or installed plug-in bundle.

## Minimal manual fixtures

The `fixtures/` directory contains three independent, ordinary `[param]` patches:
`create-default-change.pd`, `range-change.pd`, and `curve-change.pd`. Each waits one
second after initialization before its isolated edit. Copy the unmodified
`Resources/Patches/param.pd` from the same checkout beside a fixture if the test
host does not provide plugdata's standard abstraction search path.

Query the VST3 controller immediately before and after that second. The first
fixture should change the advertised default from normalized `0.25` to `0.75`.
The range fixture should change the integer step count from 4 to 8 and controller
current from `0.5` to `0.25` while retaining raw value 2. The curve fixture should
change normalized current from about `0.1` to `0.56234133`, retaining raw value 4.
The latter current-value failures are independent of the command-notification
fix; `--expect notifications` explicitly keeps them visible as known failures.

## Preparation and isolation

Run from this directory. Preparation is the default and does **not** execute any
plug-in:

```sh
python3 run.py --source /path/to/plugdata --output prepared-review
```

This compiles a host against the source checkout's exact VST3 SDK (detecting its
old or `juce_audio_processors_headless` module location). It also compiles
`probe_paths.mm` into a dylib linked only to the test host. The dylib redirects
`NSHomeDirectory`, `NSTemporaryDirectory`, and the `getenv("TMPDIR")` query through
task-specific `PLUGDATA_PARAMETER_TEST_HOME` and
`PLUGDATA_PARAMETER_TEST_TMPDIR` variables. It does not change `HOME` or `TMPDIR`
in the environment, or modify the plug-in. The host refuses to load a plug-in
unless all three redirected paths match the expected locations.

Each child runs with a Seatbelt profile denying all filesystem writes except
that run's output directory and `/dev/null`, and denying network access. The
plug-in's extracted standard resources, preferences and temporary files therefore
live within its isolated run directory. Reads remain allowed so macOS frameworks,
the source SDK, and the explicitly provided bundle can load normally.

After reviewing the profile and source, run the self-test. It does not load a
plug-in. It verifies redirected paths, an allowed temporary write and denial of
a write to a sibling path still within the task workspace:

```sh
python3 run.py --source /path/to/plugdata --output isolation-review --self-test-isolation
```

`sandbox-exec` may need to run outside an outer tool sandbox. A nested-sandbox
failure is a harness launch failure, never a plug-in regression result. The
test's own Seatbelt restrictions must remain enabled.

## Native runs

Use a new output directory for each bundle. Completed run results are never
overwritten. All output directories must be children of this tests directory.

```sh
python3 run.py --source /path/to/plugdata \
  --bundle /path/to/baseline/plugdata-fx.vst3 \
  --output results-baseline --expect baseline --execute

python3 run.py --source /path/to/plugdata \
  --bundle /path/to/notification-fix/plugdata-fx.vst3 \
  --output results-notifications --expect notifications \
  --baseline results-baseline --execute

python3 run.py --source /path/to/plugdata \
  --bundle /path/to/combined-fixes/plugdata-fx.vst3 \
  --output results-fixed --expect fixed \
  --baseline results-baseline --execute
```

Each invocation executes an isolation self-test before loading any bundle. The
five cases are fresh and nondefault saved recall in both controller-first and
processor-configured-first orders, plus controller-only pending automation while
the processor reports playing. The recall file comes from that invocation's own configured-first
initial snapshot, with enabled values set to float32 `0.8`.

The fixture has distinct late commands after initial callbacks drain:

| Pd time | Command | Check |
|---|---|---|
| 250 ms | Existing integer `create 3 0 4` | Default-only metadata refresh |
| 600 ms | Existing integer `range 0 8` | Step count and normalized current; raw value retained |
| 900 ms | Existing float `mode 4` | Curve-only normalized current; raw value retained |
| 1250 ms | Identical `range 0 8` | No value refresh or synthetic edit |
| 1600 ms | New parameter `create` | First enable without a default argument still notifies |

An initial curved parameter checks mapping synchronization across both controller
creation orders. The pending case sets `held_float` controller current to `0.9`
after entering playing state, while withholding a process parameter queue so the
processor remains at `0.75`. Unrelated changes must preserve that separation.

Assertions compare controller current **exactly** with the float32 processor value
serialized in state (apart from the intentionally pending value). Raw-value
retention is decoded from normalized state and uses a small tolerance because
inverse curve arithmetic rounds. Saved enabled-parameter attributes and values
are compared exactly against the supplied clean baseline. IDs and gesture/edit
counts are checked, including controller recreation on the same processor.

## Expected failures

`regression.json` retains every assertion, including known failures. A successful
`--expect baseline` run means both the notification and mapping failure categories
were reproduced and no invariant failed. It does not mean the plug-in passed the
fixed behavior. `--expect notifications` requires notification assertions to pass
while mapping failures remain. `--expect fixed` permits no failed assertions.

Recheck already captured results without executing the plug-in:

```sh
python3 run.py --source /path/to/plugdata --output results-fixed --expect fixed \
  --baseline results-baseline --validate-only
```

Each run records bundle/host/fixture/abstraction hashes and source checkout
observations. Source status is captured **when tested**; build provenance must
also be supplied by the separate build manifest, especially when several
preserved bundles were built sequentially from the same checkout.

This is native arm64 VST3 API evidence. It establishes no DAW GUI, real-time audio,
x86 execution, or listening outcome.
