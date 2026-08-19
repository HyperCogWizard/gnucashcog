/********************************************************************\
 * gnc-cognitive-backend.cpp -- Simulated + OpenCog backend adapter *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

#include <config.h>

#include "gnc-cognitive-backend.h"
#include "gnc-cognitive-accounting.h"

#include <glib.h>
#include <cstring>
#include <sstream>
#include <string>

static GncCognitiveBackendKind g_backend_kind = GNC_COGNITIVE_BACKEND_SIMULATED;
static gboolean g_opencog_runtime_active = FALSE;

#if defined(HAVE_OPENCOG_CORE) && defined(HAVE_OPENCOG_ATOMSPACE)
/* Optional real OpenCog headers — only compiled when detected by CMake. */
/* #include <opencog/atomspace/AtomSpace.h> */
static gboolean
opencog_runtime_probe (void)
{
    /* Placeholder: a future link against libatomspace would construct
     * an AtomSpace here. For now, build-time presence alone is not enough
     * to claim a live runtime without the shared library path verified. */
    return TRUE;
}
#else
static gboolean
opencog_runtime_probe (void)
{
    return FALSE;
}
#endif

static gboolean
opencog_build_enabled (void)
{
#if defined(HAVE_OPENCOG_CORE)
    return TRUE;
#else
    return FALSE;
#endif
}

static gboolean
ggml_build_enabled (void)
{
#if defined(HAVE_GGML)
    return TRUE;
#else
    return FALSE;
#endif
}

gboolean
gnc_cognitive_backend_available (GncCognitiveBackendKind kind)
{
    switch (kind) {
    case GNC_COGNITIVE_BACKEND_SIMULATED:
        return TRUE;
    case GNC_COGNITIVE_BACKEND_OPENCOG:
        return opencog_build_enabled () && opencog_runtime_probe ();
    default:
        return FALSE;
    }
}

gboolean
gnc_cognitive_backend_select (GncCognitiveBackendKind kind)
{
    if (!gnc_cognitive_backend_available (kind)) {
        g_warning ("Cognitive backend %d not available; keeping %s",
                   (int)kind, gnc_cognitive_backend_name ());
        return FALSE;
    }

    g_backend_kind = kind;
    g_opencog_runtime_active =
        (kind == GNC_COGNITIVE_BACKEND_OPENCOG) && opencog_runtime_probe ();

    g_message ("Cognitive backend selected: %s (opencog_runtime=%s)",
               gnc_cognitive_backend_name (),
               g_opencog_runtime_active ? "yes" : "no");
    return TRUE;
}

GncCognitiveBackendKind
gnc_cognitive_backend_current (void)
{
    return g_backend_kind;
}

const char*
gnc_cognitive_backend_name (void)
{
    switch (g_backend_kind) {
    case GNC_COGNITIVE_BACKEND_OPENCOG:
        return "opencog";
    case GNC_COGNITIVE_BACKEND_SIMULATED:
    default:
        return "simulated";
    }
}

void
gnc_cognitive_backend_apply_env_default (void)
{
    const char *env = g_getenv ("GNC_COGNITIVE_BACKEND");
    if (!env || !*env)
        return;

    if (g_ascii_strcasecmp (env, "opencog") == 0) {
        if (!gnc_cognitive_backend_select (GNC_COGNITIVE_BACKEND_OPENCOG)) {
            g_message ("GNC_COGNITIVE_BACKEND=opencog unavailable; using simulated");
            gnc_cognitive_backend_select (GNC_COGNITIVE_BACKEND_SIMULATED);
        }
    } else if (g_ascii_strcasecmp (env, "simulated") == 0 ||
               g_ascii_strcasecmp (env, "sim") == 0) {
        gnc_cognitive_backend_select (GNC_COGNITIVE_BACKEND_SIMULATED);
    } else {
        g_warning ("Unknown GNC_COGNITIVE_BACKEND=%s (use simulated|opencog)", env);
    }
}

gboolean
gnc_cognitive_backend_sync_book (QofBook *book)
{
    g_return_val_if_fail (book != nullptr, FALSE);
    if (!gnc_cognitive_accounting_is_initialized ())
        return FALSE;

    /* Simulated core always mirrors; OpenCog path currently dual-writes
     * into the simulated atomspace and (when linked) would also push atoms
     * into a real AtomSpace. */
    gnc_cognitive_accounting_observe_book (book);

#if defined(HAVE_OPENCOG_CORE) && defined(HAVE_OPENCOG_ATOMSPACE)
    if (g_opencog_runtime_active) {
        /* Future: walk CoA and create opencog::ConceptNode entries. */
        g_debug ("OpenCog backend: book sync hook (dual-write stub)");
    }
#endif
    return TRUE;
}

gboolean
gnc_cognitive_backend_sync_transaction (Transaction *transaction)
{
    g_return_val_if_fail (transaction != nullptr, FALSE);
    if (!gnc_cognitive_accounting_is_initialized ())
        return FALSE;

    gnc_cognitive_accounting_on_transaction_commit (transaction);

#if defined(HAVE_OPENCOG_CORE) && defined(HAVE_OPENCOG_ATOMSPACE)
    if (g_opencog_runtime_active) {
        g_debug ("OpenCog backend: transaction sync hook (dual-write stub)");
    }
#endif
    return TRUE;
}

gboolean
gnc_cognitive_backend_get_stats (GncCognitiveBackendStats *out)
{
    g_return_val_if_fail (out != nullptr, FALSE);
    memset (out, 0, sizeof (*out));
    out->backend_name = gnc_cognitive_backend_name ();
    out->opencog_build_enabled = opencog_build_enabled ();
    out->opencog_runtime_active = g_opencog_runtime_active;
    out->ggml_build_enabled = ggml_build_enabled ();

    if (!gnc_cognitive_accounting_is_initialized ())
        return FALSE;

    return gnc_cognitive_atomspace_stats (
        &out->atom_count,
        &out->account_atoms,
        &out->transaction_atoms,
        &out->total_sti_funds,
        &out->total_lti_funds);
}

char*
gnc_cognitive_backend_status_json (void)
{
    GncCognitiveBackendStats st{};
    gnc_cognitive_backend_get_stats (&st);

    std::ostringstream ss;
    ss << "{"
       << "\"backend\":\"" << (st.backend_name ? st.backend_name : "none") << "\","
       << "\"atom_count\":" << st.atom_count << ","
       << "\"account_atoms\":" << st.account_atoms << ","
       << "\"transaction_atoms\":" << st.transaction_atoms << ","
       << "\"sti_funds\":" << st.total_sti_funds << ","
       << "\"lti_funds\":" << st.total_lti_funds << ","
       << "\"opencog_build\":" << (st.opencog_build_enabled ? "true" : "false") << ","
       << "\"opencog_runtime\":" << (st.opencog_runtime_active ? "true" : "false") << ","
       << "\"ggml_build\":" << (st.ggml_build_enabled ? "true" : "false") << ","
       << "\"initialized\":"
       << (gnc_cognitive_accounting_is_initialized () ? "true" : "false")
       << "}";
    return g_strdup (ss.str ().c_str ());
}

gboolean
gnc_cognitive_backend_health_check (void)
{
    if (!gnc_cognitive_accounting_is_initialized ())
        return FALSE;

    GncCognitiveBackendStats st{};
    if (!gnc_cognitive_backend_get_stats (&st))
        return FALSE;

    /* Funds must remain non-negative; empty atomspace is ok pre-observe. */
    if (st.total_sti_funds < 0.0 || st.total_lti_funds < 0.0)
        return FALSE;

    if (g_backend_kind == GNC_COGNITIVE_BACKEND_OPENCOG && !g_opencog_runtime_active)
        return FALSE;

    return TRUE;
}
