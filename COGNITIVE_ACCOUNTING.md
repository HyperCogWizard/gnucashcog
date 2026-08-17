# GnuCash Cognitive Accounting Framework

## Status (truthful)

The cognitive layer ships as an **always-on simulated core** inside `libgnucash/engine`.
It builds **without** OpenCog or ggml. Optional backends may be linked later when those
libraries are present (`HAVE_OPENCOG_*`, `HAVE_GGML`).

| Area | Simulated core | Optional real backend |
|------|----------------|------------------------|
| AtomSpace CoA/tx mirror | Yes | OpenCog AtomSpace adapter (future) |
| PLN double-entry / proofs | Yes (numeric TV formulas) | OpenCog PLN |
| ECAN attention | Yes (STI/LTI funds, decay, Hebbian) | OpenCog AttentionBank |
| MOSES strategies | Yes (feature heuristics + JSON rules) | AS-MOSES |
| URE prediction | Yes (drift + bounds) | OpenCog URE |
| Tensor Memory/Task/AI/Autonomy | Yes (CPU k-means + features v1) | ggml kernels |
| Guile `(gnucash cognitive)` | Partial (Scheme **export** strings only) | Full SWIG module |
| Register UI badges / reports | Not yet | Phase 6 |

Environment:
- `GNC_COGNITIVE_AUTO=1` enables QOF commit/account listeners after engine init.

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

## Core API (headers)

- `libgnucash/engine/gnc-cognitive-accounting.h` — AtomSpace, PLN, ECAN, MOSES, URE, proofs, lifecycle
- `libgnucash/engine/gnc-cognitive-comms.h` — module hub (`GncCognitiveModuleMessage`)
- `libgnucash/engine/gnc-cognitive-scheme.h` — bootstrap / export helpers
- `libgnucash/engine/gnc-tensor-network.h` — Memory/Task/AI/Autonomy tensor nodes

### PLN truth values

`gnc_pln_validate_double_entry()` returns `strength * confidence` in `[0,1]`.
Balanced transactions typically land in approximately **`[0.70, 0.99]`**, not exact `1.0`.
Use `gnc_pln_validate_double_entry_tv()` for separate strength/confidence.
Trial balance / P&L proofs expose `GncProofReport` with numeric debit/credit totals.

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
ninja -C build gnc-engine test-cognitive-accounting test-tensor-network
ctest --test-dir build -R 'test-cognitive-accounting|test-tensor-network' --output-on-failure
# or
./test-cognitive-accounting.sh   # runs ctest when build/ exists
```

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

See the implementation plan milestones M0–M8 (build → AtomSpace → lifecycle → PLN/ECAN → tensor → Guile/UI → OpenCog adapter → hardening).
