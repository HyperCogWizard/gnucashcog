# GnuCash Cognitive Implementation Report

## Reality check

Earlier drafts of this report overstated completion. This document tracks **what actually works**
in the simulated cognitive core shipped with the engine, plus Phases 5–8 surfaces.

## Implemented (simulated core)

- Clean compile path for cognitive sources without OpenCog/ggml
- In-process AtomSpace with real outgoing/incoming sets and SimpleTV
- Account + transaction mapping; hierarchy InheritanceLinks
- PLN double-entry / n-entry TV formulas; trial balance & P&L proof atoms + `GncProofReport`
- ECAN STI/LTI funds, wage/rent, decay tick, Hebbian co-occurrence boost, top-K API
- MOSES-style feature heuristics emitting JSON strategy strings (no silent tx rewrite)
- URE point estimate + uncertainty bounds (`GncUrePrediction`)
- Tensor network Memory/Task/AI/Autonomy nodes with **k-means** clustering and feature schema v1
- Module communication hub with bounded queues
- Scheme **export** strings only (no untrusted eval)
- Engine init/shutdown hooks; optional auto QOF via `GNC_COGNITIVE_AUTO=1`
- Unit tests: `test-cognitive-accounting`, `test-tensor-network`

## Implemented (Phases 5–8)

### Phase 5 — Guile `(gnucash cognitive)`
- SWIG interface `bindings/cognitive.i` included from `engine.i`
- Scheme module `bindings/guile/cognitive.scm` with friendly wrappers
- CMake target `scm-cognitive`

### Phase 6 — UI / reports
- Transaction badges + attention heat / CSS color C API
- HTML fragment generators for summary, attention table, validation
- Standard report: **Cognitive Accounting** (Experimental menu)
- Register debit/credit hatch when badges enabled (`GNC_COGNITIVE_UI` / AUTO)

### Phase 7 — OpenCog CognitiveBackend adapter
- `gnc-cognitive-backend.{h,cpp}` with simulated default
- OpenCog selection gated on `HAVE_OPENCOG_CORE` + runtime probe
- Book/tx sync hooks (dual-write stub when OpenCog active)
- JSON status + health check API
- Tests: `test-cognitive-backend`

### Phase 8 — Hardening / benchmarks
- `test-cognitive-benchmark` large-book observe/validate/ECAN timings
- Bounded validation HTML on large books (sample ≤500 txs)
- Re-observe stability check (atom count does not explode)
- Env knobs for CI vs local stress sizes

## Partial / future

- Full live OpenCog AtomSpace push (requires linked libatomspace at runtime)
- ggml kernels behind tensor path when `HAVE_GGML`
- Multi-commodity pricedb conversion in all proof paths
- Import (OFX/CSV) categorization assists driven by MOSES JSON

## Verification

```bash
ctest -R 'test-cognitive' --output-on-failure
# covers: accounting, tensor, backend, benchmark
```
