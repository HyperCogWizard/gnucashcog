# GnuCash Cognitive Accounting Framework

## Status (truthful)

The cognitive layer ships as an **always-on simulated core** inside `libgnucash/engine`.
It builds **without** OpenCog or ggml. Optional backends may be linked later when those
libraries are present (`HAVE_OPENCOG_*`, `HAVE_GGML`).

| Area | Simulated core | Optional real backend |
|------|----------------|------------------------|
| AtomSpace CoA/tx mirror | Yes | OpenCog AtomSpace adapter (`CognitiveBackend`) |
| PLN double-entry / proofs | Yes (numeric TV formulas) | OpenCog PLN |
| ECAN attention | Yes (STI/LTI funds, decay, Hebbian) | OpenCog AttentionBank |
| MOSES strategies | Yes (feature heuristics + JSON rules) | AS-MOSES |
| URE prediction | Yes (drift + bounds) | OpenCog URE |
| Tensor Memory/Task/AI/Autonomy | Yes (CPU k-means + features v1) | ggml kernels |
| Guile `(gnucash cognitive)` | Yes (SWIG + scheme wrappers) | — |
| Register UI badges / reports | Yes (hatch + HTML report) | — |
| Large-book benchmarks | Yes (`test-cognitive-benchmark`) | — |

Environment:
- `GNC_COGNITIVE_AUTO=1` enables QOF commit/account listeners after engine init.
- `GNC_COGNITIVE_UI=1` enables register hatch badges (also implied by AUTO).
- `GNC_COGNITIVE_BACKEND=simulated|opencog` selects backend (OpenCog only if built+available).
- Benchmark knobs: `GNC_COG_BENCH_ACCOUNTS`, `GNC_COG_BENCH_TXNS`, `GNC_COG_BENCH_MAX_MS_*`.

## Overview

Chart of Accounts and transactions are mirrored as an in-process hypergraph
(`GncCognitiveAtom` records with outgoing/incoming handles, SimpleTV, STI/LTI).
PLN validates ledgers; ECAN ranks attention; MOSES emits ranked categorization
heuristics; URE predicts balances with uncertainty; the tensor network clusters
transaction feature vectors.

### Design principles

1. **Correct simulated core first** — unit tests must pass with no external cognitive deps.
2. **Engine lifecycle integration** — `gnc_engine_init` / `gnc_engine_shutdown` own cognitive init.
3. **No silent mutation** of user transactions.
4. **No untrusted Scheme eval** of book data (export-only strings; eval records ConceptNodes).
5. **Pluggable backend** — simulated default; OpenCog adapter dual-writes when available.

## Core API (headers)

- `libgnucash/engine/gnc-cognitive-accounting.h` — AtomSpace, PLN, ECAN, MOSES, URE, proofs, lifecycle, UI badges, HTML fragments
- `libgnucash/engine/gnc-cognitive-backend.h` — `CognitiveBackend` selection / sync / health / JSON status
- `libgnucash/engine/gnc-cognitive-comms.h` — module hub (`GncCognitiveModuleMessage`)
- `libgnucash/engine/gnc-cognitive-scheme.h` — bootstrap / export helpers
- `libgnucash/engine/gnc-tensor-network.h` — Memory/Task/AI/Autonomy tensor nodes

### Guile module

```scheme
(use-modules (gnucash cognitive))
(gnc-cognitive-init)
(cognitive-observe-book! book)
(cognitive-pln-validate txn)
(cognitive-tx-badge txn)           ; => ok | warn | fail | unknown
(cognitive-backend-status-json)
(cognitive-html-summary book)
```

SWIG exports live in `bindings/cognitive.i` (included from `engine.i`).

### HTML report

**Reports → Experimental → Cognitive Accounting**
(`gnucash/report/reports/standard/cognitive-accounting.scm`)

### Register UI

When badges are enabled, debit/credit cells hatch on PLN warn/fail in addition to
classic imbalance hatching (`split-register-model.c`).

### PLN truth values

`gnc_pln_validate_double_entry()` returns `strength * confidence` in `[0,1]`.
Balanced transactions typically land in approximately **`[0.70, 0.99]`**, not exact `1.0`.
Use `gnc_pln_validate_double_entry_tv()` for separate strength/confidence.
Trial balance / P&L proofs expose `GncProofReport` with numeric debit/credit totals.

### UI badges

| Badge | Meaning |
|-------|---------|
| OK | balanced and score ≥ 0.70 with confidence ≥ 0.55 |
| Warn | weaker TV / lower confidence |
| Fail | imbalanced or score < 0.45 |
| ? | cognitive not ready / missing data |

### Tensor feature schema v1 (8 dims)

0. date  
1. amount magnitude  
2. split count  
3. validity (row encoded)  
4. log1p(magnitude)  
5. imbalance flag  
6. day-of-week  
7. normalized imbalance  

Clustering: CPU k-means (and Cogfluence path). ggml used only when `HAVE_GGML`.

## Building & testing

```bash
cmake -G Ninja -B build -DWITH_PYTHON=OFF  # plus your usual GnuCash options
ninja -C build gnc-engine \
  test-cognitive-accounting test-tensor-network \
  test-cognitive-backend test-cognitive-benchmark
ctest --test-dir build -R 'test-cognitive' --output-on-failure
# or
./test-cognitive-accounting.sh   # runs ctest when build/ exists
```

Large-book local run:

```bash
GNC_COG_BENCH_ACCOUNTS=200 GNC_COG_BENCH_TXNS=2000 \
  ctest --test-dir build -R test-cognitive-benchmark --output-on-failure
```

ASAN: use the existing `ci_tests_ASAN` workflow / `-DCMAKE_BUILD_TYPE=Asan`.

## Messaging

Two distinct message types:

1. **`GncCognitiveAtomMessage`** (alias `GncCognitiveMessage`) — string-routed atom payloads in the accounting module.
2. **`GncCognitiveModuleMessage`** — enum-routed hub messages in `gnc-cognitive-comms`.

C API receive returns `GArray*`; C++ helper is `gnc_cognitive_receive_messages_cpp`.

## Non-goals (current milestones)

- Full AGI / consciousness claims without measurable metrics  
- Requiring OpenCog/ggml for basic features  
- Silent mutation of committed transactions  
- Blockchain / multi-agent ledgers  

## Roadmap

| Phase | Status |
|-------|--------|
| M0–M4 simulated core + tensor | Done |
| M5 Guile `(gnucash cognitive)` | Done |
| M6 UI badges + HTML report | Done |
| M7 OpenCog `CognitiveBackend` adapter | Done (stub dual-write; live AtomSpace when linked) |
| M8 Hardening / large-book benchmarks | Done (gtest benchmarks + health JSON) |
