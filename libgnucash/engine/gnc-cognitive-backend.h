/********************************************************************\
 * gnc-cognitive-backend.h -- Pluggable cognitive backend adapter  *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

/** @addtogroup Engine
    @{ */
/** @addtogroup CognitiveBackend
    Pluggable backend for the cognitive accounting core.
    Default is the always-on simulated AtomSpace. When OpenCog libraries
    are present at build time (HAVE_OPENCOG_CORE), the OpenCog adapter
    may be selected; otherwise selection falls back to simulated.
    @{ */

#ifndef GNC_COGNITIVE_BACKEND_H
#define GNC_COGNITIVE_BACKEND_H

#include "Account.h"
#include "Transaction.h"
#include "gnc-engine.h"
#include "gnc-cognitive-accounting.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Backend implementation kind */
typedef enum {
    GNC_COGNITIVE_BACKEND_SIMULATED = 0,
    GNC_COGNITIVE_BACKEND_OPENCOG   = 1
} GncCognitiveBackendKind;

/** Snapshot of backend / atomspace health metrics */
typedef struct {
    guint64 atom_count;
    guint64 account_atoms;
    guint64 transaction_atoms;
    gdouble total_sti_funds;
    gdouble total_lti_funds;
    const char *backend_name;
    gboolean opencog_build_enabled;
    gboolean opencog_runtime_active;
    gboolean ggml_build_enabled;
} GncCognitiveBackendStats;

/** @name Backend selection */
/** @{ */

/** Return TRUE if @a kind can be activated in this build/runtime. */
gboolean gnc_cognitive_backend_available (GncCognitiveBackendKind kind);

/**
 * Select active backend. Safe to call before or after cognitive init.
 * If OpenCog is requested but unavailable, returns FALSE and keeps
 * the current backend (default simulated).
 */
gboolean gnc_cognitive_backend_select (GncCognitiveBackendKind kind);

/** Currently selected backend kind. */
GncCognitiveBackendKind gnc_cognitive_backend_current (void);

/** Human-readable name of the active backend ("simulated" / "opencog"). */
const char* gnc_cognitive_backend_name (void);

/** Prefer OpenCog when available (env GNC_COGNITIVE_BACKEND=opencog). */
void gnc_cognitive_backend_apply_env_default (void);

/** @} */

/** @name Sync / health */
/** @{ */

/** Mirror whole book CoA into the active backend. */
gboolean gnc_cognitive_backend_sync_book (QofBook *book);

/** Mirror one committed transaction into the active backend. */
gboolean gnc_cognitive_backend_sync_transaction (Transaction *transaction);

/** Fill @a out with current stats; returns FALSE if cognitive not init. */
gboolean gnc_cognitive_backend_get_stats (GncCognitiveBackendStats *out);

/**
 * JSON status blob for reports/diagnostics. Caller g_free()'s.
 * Always returns a valid JSON object string when cognitive is up.
 */
char* gnc_cognitive_backend_status_json (void);

/** Lightweight health check (atomspace non-empty after observe, funds ok). */
gboolean gnc_cognitive_backend_health_check (void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* GNC_COGNITIVE_BACKEND_H */
/** @} */
/** @} */
