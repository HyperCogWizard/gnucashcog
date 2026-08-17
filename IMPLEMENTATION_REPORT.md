# GnuCash Cognitive Implementation Report

## Reality check

Earlier drafts of this report overstated completion. This document tracks **what actually works**
in the simulated cognitive core shipped with the engine.

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

## Partial / not done

- Guile SWIG module `(gnucash cognitive)`
- Register UI validation badges, attention heat, HTML reports
- Import (OFX/CSV) categorization assists
- Real OpenCog / ggml backend adapters behind a `CognitiveBackend` interface
- Multi-commodity pricedb conversion in all proof paths
- Large-book benchmarks, ASAN leak gates in CI

## Verification

Prefer `ctest -R 'test-cognitive-accounting|test-tensor-network'` over presence-only scripts.
