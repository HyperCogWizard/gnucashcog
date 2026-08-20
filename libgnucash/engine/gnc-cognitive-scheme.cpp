/********************************************************************\
 * gnc-cognitive-scheme.cpp -- Scheme export / bootstrap            *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

#include "gnc-cognitive-scheme.h"
#include "Account.h"
#include "Transaction.h"
#include "qof.h"
#include <glib.h>
#include <string>

/* Bootstrap is documentation/export only — never eval untrusted book data. */
static const gchar *cognitive_accounting_scheme_init = R"scheme(
;; GnuCash Cognitive Accounting Scheme Interface (export DSL)
;; Hypergraph pattern helpers for reports and OpenCog export.

(define (create-account-concept name type)
  (list 'InheritanceLink
        (list 'ConceptNode name)
        (list 'ConceptNode type)))

(define (create-transaction-pattern split-count)
  (list 'transaction-pattern split-count))

(define (cognitive-balance-validation transaction)
  (list 'pln-truth-value 0.8 0.9))

(define (attention-allocation-update account activity-level)
  (list 'ecan-attention-update account activity-level))

(define (evolutionary-strategy-discovery n)
  (list 'moses-evolved-strategy n))

(define (uncertain-reasoning-prediction account future-date)
  (list 'ure-prediction account future-date))

(define (neural-symbolic-account-analysis account)
  (list 'neural-symbolic-analysis account))

(define (emergent-cognitive-insight n)
  (list 'emergent-cognitive-insight n))
)scheme";

static gboolean g_scheme_ready = FALSE;

gboolean
gnc_cognitive_scheme_init(void)
{
    g_scheme_ready = TRUE;
    g_message("Cognitive accounting Scheme interface initialized (export mode)");
    g_debug("%s", cognitive_accounting_scheme_init);
    return TRUE;
}

gchar*
gnc_cognitive_scheme_eval(const gchar* scheme_code)
{
    g_return_val_if_fail(scheme_code != nullptr, nullptr);
    /* Safe fallback: echo sanitized acknowledgment, no code execution. */
    g_message("Scheme evaluation (export-only fallback): %.120s", scheme_code);
    return g_strdup("scheme-evaluation-fallback");
}

GncAtomHandle
gnc_scheme_create_hypergraph_pattern(const gchar* pattern_scheme)
{
    g_return_val_if_fail(pattern_scheme != nullptr, 0);
    return gnc_evaluate_scheme_expression(pattern_scheme);
}

void
gnc_scheme_register_account_patterns(Account *account)
{
    g_return_if_fail(account != nullptr);
    const gchar* account_name = xaccAccountGetName(account);
    GNCAccountType account_type = xaccAccountGetType(account);
    const gchar* type_str = xaccAccountGetTypeStr(account_type);
    gchar* account_pattern = g_strdup_printf(
        "(create-account-concept \"%s\" \"%s\")",
        account_name ? account_name : "unnamed",
        type_str ? type_str : "UNKNOWN");
    gnc_scheme_create_hypergraph_pattern(account_pattern);
    g_free(account_pattern);
}

void
gnc_scheme_register_transaction_patterns(Transaction *transaction)
{
    g_return_if_fail(transaction != nullptr);
    GList *splits = xaccTransGetSplitList(transaction);
    gint split_count = g_list_length(splits);
    gchar* transaction_pattern = g_strdup_printf(
        "(create-transaction-pattern %d)", split_count);
    gnc_scheme_create_hypergraph_pattern(transaction_pattern);
    g_free(transaction_pattern);
}

void
gnc_scheme_trigger_attention_update(Account *account, gdouble activity_level)
{
    g_return_if_fail(account != nullptr);
    const gchar* account_name = xaccAccountGetName(account);
    gchar* attention_scheme = g_strdup_printf(
        "(attention-allocation-update \"%s\" %.3f)",
        account_name ? account_name : "unnamed", activity_level);
    gchar* result = gnc_cognitive_scheme_eval(attention_scheme);
    g_free(result);
    g_free(attention_scheme);
}

void
gnc_scheme_evolutionary_optimization(Transaction **transactions, gint n_transactions)
{
    g_return_if_fail(transactions != nullptr);
    g_return_if_fail(n_transactions > 0);
    gchar* evolution_scheme = g_strdup_printf(
        "(evolutionary-strategy-discovery %d)", n_transactions);
    gchar* result = gnc_cognitive_scheme_eval(evolution_scheme);
    g_free(result);
    g_free(evolution_scheme);
}

gchar*
gnc_scheme_uncertain_prediction(Account *account, time64 future_date)
{
    g_return_val_if_fail(account != nullptr, nullptr);
    const gchar* account_name = xaccAccountGetName(account);
    gchar* prediction_scheme = g_strdup_printf(
        "(uncertain-reasoning-prediction \"%s\" %ld)",
        account_name ? account_name : "unnamed",
        static_cast<long>(future_date));
    gchar* result = gnc_cognitive_scheme_eval(prediction_scheme);
    g_free(prediction_scheme);
    return result;
}

void
gnc_scheme_neural_symbolic_analysis(Account *account)
{
    g_return_if_fail(account != nullptr);
    const gchar* account_name = xaccAccountGetName(account);
    gchar* analysis_scheme = g_strdup_printf(
        "(neural-symbolic-account-analysis \"%s\")",
        account_name ? account_name : "unnamed");
    gchar* result = gnc_cognitive_scheme_eval(analysis_scheme);
    g_free(result);
    g_free(analysis_scheme);
}

void
gnc_scheme_emergent_insight_discovery(Transaction **transactions, gint n_transactions)
{
    g_return_if_fail(transactions != nullptr);
    g_return_if_fail(n_transactions > 0);
    gchar* insight_scheme = g_strdup_printf(
        "(emergent-cognitive-insight %d)", n_transactions);
    gchar* result = gnc_cognitive_scheme_eval(insight_scheme);
    g_free(result);
    g_free(insight_scheme);
}
