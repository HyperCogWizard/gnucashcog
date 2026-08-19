/********************************************************************\
 * gnc-cognitive-accounting.h -- Cognitive accounting API           *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
\********************************************************************/

/** @addtogroup Engine
    @{ */
/** @addtogroup CognitiveAccounting
    Simulated (always-on) cognitive accounting core with optional
    OpenCog/ggml backends behind adapters. Chart of Accounts is
    mirrored as an in-process AtomSpace-style hypergraph; PLN validates
    ledgers; ECAN allocates attention; MOSES discovers strategies; URE
    predicts balances under uncertainty.
    @{ */

#ifndef GNC_COGNITIVE_ACCOUNTING_H
#define GNC_COGNITIVE_ACCOUNTING_H

#include "Account.h"
#include "Transaction.h"
#include "gnc-engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/** AtomSpace handle type for representing accounts as atoms */
typedef guint64 GncAtomHandle;

/** Account node types in AtomSpace hierarchy */
typedef enum {
    GNC_ATOM_CONCEPT_NODE = 0,
    GNC_ATOM_PREDICATE_NODE,
    GNC_ATOM_SCHEMA_NODE,
    GNC_ATOM_GROUNDED_SCHEMA,
    GNC_ATOM_INHERITANCE_LINK,
    GNC_ATOM_SIMILARITY_LINK,
    GNC_ATOM_MEMBER_LINK,
    GNC_ATOM_EVALUATION_LINK,
    GNC_ATOM_EXECUTION_LINK,
    GNC_ATOM_IMPLICATION_LINK,
    GNC_ATOM_AND_LINK,
    GNC_ATOM_OR_LINK,
    GNC_ATOM_COMBO_NODE,
    /* Legacy aliases */
    GNC_ATOM_ACCOUNT_CONCEPT = GNC_ATOM_CONCEPT_NODE,
    GNC_ATOM_ACCOUNT_CATEGORY = GNC_ATOM_CONCEPT_NODE,
    GNC_ATOM_ACCOUNT_HIERARCHY = GNC_ATOM_INHERITANCE_LINK,
    GNC_ATOM_ACCOUNT_BALANCE = GNC_ATOM_PREDICATE_NODE,
    GNC_ATOM_TRANSACTION_RULE = GNC_ATOM_SCHEMA_NODE,
    GNC_ATOM_DOUBLE_ENTRY_RULE = GNC_ATOM_IMPLICATION_LINK,
    GNC_ATOM_N_ENTRY_RULE = GNC_ATOM_IMPLICATION_LINK
} GncAtomType;

/** OpenCog ECAN-style attention allocation parameters */
typedef struct {
    gdouble sti;
    gdouble sti_funds;
    gdouble lti;
    gdouble lti_funds;
    gdouble vlti;
    gdouble confidence;
    gdouble strength;
    gdouble activity_level;
    gdouble wage;
    gdouble rent;
    /* Legacy compatibility */
    gdouble importance;
    gdouble attention_value;
} GncAttentionParams;

/** Simple truth value (PLN-style) */
typedef struct {
    gdouble strength;
    gdouble confidence;
} GncTruthValue;

/** URE balance prediction with uncertainty bounds */
typedef struct {
    gnc_numeric point_estimate;
    gnc_numeric lower_bound;
    gnc_numeric upper_bound;
    gdouble confidence;
} GncUrePrediction;

/** Trial balance / P&L proof report (auditable numerics) */
typedef struct {
    GncAtomHandle proof_atom;
    gnc_numeric total_debits;
    gnc_numeric total_credits;
    gnc_numeric imbalance;
    gdouble strength;
    gdouble confidence;
    gboolean balanced;
} GncProofReport;

/** Enhanced account types for cognitive accounting (bit flags) */
typedef enum {
    GNC_COGNITIVE_ACCT_TRADITIONAL = 0x0000,
    GNC_COGNITIVE_ACCT_ADAPTIVE    = 0x0001,
    GNC_COGNITIVE_ACCT_PREDICTIVE  = 0x0002,
    GNC_COGNITIVE_ACCT_MULTIMODAL  = 0x0004,
    GNC_COGNITIVE_ACCT_ATTENTION   = 0x0008
} GncCognitiveAccountType;

/** String-routed atom payload message (distinct from module-hub messages) */
typedef struct {
    const char* source_module;
    const char* target_module;
    const char* message_type;
    GncAtomHandle payload_atom;
    gdouble priority;
    time64 timestamp;
} GncCognitiveAtomMessage;

/* Backward-compatible alias used by existing tests/demos */
typedef GncCognitiveAtomMessage GncCognitiveMessage;

typedef void (*GncCognitiveMessageHandler)(const GncCognitiveAtomMessage* message);

/** Emergence detection parameters */
typedef struct {
    gdouble complexity_threshold;
    gdouble coherence_measure;
    gdouble novelty_score;
    gint pattern_frequency;
} GncEmergenceParams;

/** @name Lifecycle / feature flag */
/** @{ */

gboolean gnc_cognitive_accounting_init(void);
void gnc_cognitive_accounting_shutdown(void);
gboolean gnc_cognitive_accounting_is_initialized(void);

/**
 * Enable or disable automatic QOF lifecycle integration.
 * Default: disabled unless env GNC_COGNITIVE_AUTO=1.
 */
void gnc_cognitive_accounting_set_auto_enabled(gboolean enabled);
gboolean gnc_cognitive_accounting_get_auto_enabled(void);

/** @} */

/** @name AtomSpace operations */
/** @{ */

GncAtomHandle gnc_atomspace_create_concept_node(const char* name);
GncAtomHandle gnc_atomspace_create_predicate_node(const char* name);
GncAtomHandle gnc_atomspace_create_evaluation_link(GncAtomHandle predicate_atom,
                                                   GncAtomHandle account_atom,
                                                   gdouble truth_value);
GncAtomHandle gnc_atomspace_create_inheritance_link(GncAtomHandle child_atom,
                                                    GncAtomHandle parent_atom);
void gnc_atomspace_set_truth_value(GncAtomHandle atom_handle,
                                   gdouble strength, gdouble confidence);
gboolean gnc_atomspace_get_truth_value(GncAtomHandle atom_handle,
                                       gdouble* strength, gdouble* confidence);

GncAtomType gnc_atomspace_get_atom_type(GncAtomHandle atom_handle);
const char* gnc_atomspace_get_atom_name(GncAtomHandle atom_handle);
guint gnc_atomspace_get_outgoing_size(GncAtomHandle atom_handle);
GncAtomHandle gnc_atomspace_get_outgoing(GncAtomHandle atom_handle, guint index);
guint gnc_atomspace_get_incoming_size(GncAtomHandle atom_handle);
GncAtomHandle gnc_atomspace_get_incoming(GncAtomHandle atom_handle, guint index);

GncAtomHandle gnc_account_to_atomspace(const Account *account);
GncAtomHandle gnc_atomspace_create_hierarchy_link(GncAtomHandle parent_atom,
                                                  GncAtomHandle child_atom);
GncAtomHandle gnc_transaction_to_atomspace(const Transaction *transaction);
void gnc_atomspace_remove_account(const Account *account);

/** @} */

/** @name PLN ledger rules */
/** @{ */

/**
 * Validate double-entry using PLN-style truth values.
 * Returns strength*confidence in [0,1].
 * Balanced transactions typically score in approximately [0.70, 0.99].
 */
gdouble gnc_pln_validate_double_entry(const Transaction *transaction);
gboolean gnc_pln_validate_double_entry_tv(const Transaction *transaction,
                                          GncTruthValue *tv_out);
gdouble gnc_pln_validate_n_entry(const Transaction *transaction, gint n_parties);
GncAtomHandle gnc_pln_generate_trial_balance_proof(const Account *root_account);
GncAtomHandle gnc_pln_generate_pl_proof(const Account *income_account,
                                        const Account *expense_account);
gboolean gnc_pln_trial_balance_report(const Account *root_account,
                                      GncProofReport *report_out);
gboolean gnc_pln_pl_report(const Account *income_account,
                           const Account *expense_account,
                           GncProofReport *report_out);
gdouble gnc_pln_get_last_validation_score(const Transaction *transaction);

/** @} */

/** @name ECAN attention allocation */
/** @{ */

void gnc_ecan_update_account_attention(Account *account,
                                       const Transaction *transaction);
GncAttentionParams gnc_ecan_get_attention_params(const Account *account);
void gnc_ecan_allocate_attention(Account **accounts, gint n_accounts);
void gnc_ecan_decay_tick(void);
gint gnc_ecan_top_accounts(Account **out_accounts, gint max_accounts);

/** @} */

/** @name MOSES integration */
/** @{ */

GncAtomHandle gnc_moses_discover_balancing_strategies(Transaction **historical_transactions,
                                                      gint n_transactions);
Transaction* gnc_moses_optimize_transaction(const Transaction *transaction);
char* gnc_moses_last_strategies_json(void);

/** @} */

/** @name URE uncertain reasoning */
/** @{ */

gnc_numeric gnc_ure_predict_balance(const Account *account, time64 future_date);
gboolean gnc_ure_predict_balance_ex(const Account *account, time64 future_date,
                                    GncUrePrediction *prediction_out);
gdouble gnc_ure_transaction_validity(const Transaction *transaction);

/** @} */

/** @name Scheme export strings (not untrusted eval of book data) */
/** @{ */

char* gnc_account_to_scheme_representation(const Account *account);
char* gnc_transaction_to_scheme_pattern(const Transaction *transaction);
GncAtomHandle gnc_evaluate_scheme_expression(const char* scheme_expr);
char* gnc_create_hypergraph_pattern_encoding(const Account *root_account);

/** @} */

/** @name Atom-routed messaging */
/** @{ */

gboolean gnc_send_cognitive_message(const GncCognitiveAtomMessage* message);
gboolean gnc_register_cognitive_message_handler(const char* module_name,
                                               GncCognitiveMessageHandler handler_func);

/** @} */

/** @name Emergence */
/** @{ */

GncAtomHandle gnc_detect_emergent_patterns(Account** accounts, gint n_accounts,
                                           const GncEmergenceParams* params);
GncAtomHandle gnc_optimize_distributed_attention(gdouble cognitive_load,
                                                 gdouble available_resources);

/** @} */

/** @name Cognitive account types (KVP-backed flags) */
/** @{ */

void gnc_account_set_cognitive_type(Account *account, GncCognitiveAccountType cognitive_type);
GncCognitiveAccountType gnc_account_get_cognitive_type(const Account *account);
gboolean gnc_account_has_cognitive_behavior(const Account *account, GncCognitiveAccountType behavior);
void gnc_account_adapt_cognitive_behavior(Account *account, const Transaction *transaction);

/** @} */

/** @name Query helpers for GUI/reports */
/** @{ */

void gnc_cognitive_accounting_observe_book(QofBook *book);
void gnc_cognitive_accounting_on_transaction_commit(Transaction *transaction);

/** @} */

/** @name UI badges / attention heat (Phase 6) */
/** @{ */

/** Register / report validation badge for a transaction */
typedef enum {
    GNC_COGNITIVE_BADGE_OK = 0,
    GNC_COGNITIVE_BADGE_WARN,
    GNC_COGNITIVE_BADGE_FAIL,
    GNC_COGNITIVE_BADGE_UNKNOWN
} GncCognitiveBadge;

/**
 * Enable register hatch / badge surfacing.
 * Default: on when GNC_COGNITIVE_AUTO=1 or GNC_COGNITIVE_UI=1.
 */
void gnc_cognitive_ui_set_badges_enabled(gboolean enabled);
gboolean gnc_cognitive_ui_badges_enabled(void);

/** PLN-based badge for a transaction (does not mutate the book). */
GncCognitiveBadge gnc_cognitive_transaction_badge(const Transaction *transaction);

/** Short label suitable for tooltips ("OK" / "Warn" / "Fail" / "?"). Caller g_free. */
char* gnc_cognitive_transaction_badge_label(const Transaction *transaction);

/** Attention heat in [0,1] from STI/LTI blend. */
gdouble gnc_cognitive_account_attention_heat(const Account *account);

/** CSS hex color (#RRGGBB) for heat map cells. Caller g_free. */
char* gnc_cognitive_account_attention_css_color(const Account *account);

/** Convenience STI/LTI accessors for Guile without struct marshalling. */
gdouble gnc_ecan_account_sti(const Account *account);
gdouble gnc_ecan_account_lti(const Account *account);

/** TRUE when trial-balance proof reports balanced. */
gboolean gnc_pln_trial_balance_balanced(const Account *root_account);

/** @} */

/** @name HTML fragments for Scheme reports */
/** @{ */

/** Backend + atomspace summary as a small HTML snippet. Caller g_free. */
char* gnc_cognitive_html_summary_for_book(QofBook *book);

/** Top-N attention accounts as an HTML table. Caller g_free. */
char* gnc_cognitive_attention_table_html(QofBook *book, gint top_n);

/** Recent / sample validation summary HTML. Caller g_free. */
char* gnc_cognitive_validation_summary_html(QofBook *book);

/** @} */

/** @name AtomSpace stats (used by CognitiveBackend) */
/** @{ */

gboolean gnc_cognitive_atomspace_stats(guint64 *atom_count,
                                       guint64 *account_atoms,
                                       guint64 *transaction_atoms,
                                       gdouble *sti_funds,
                                       gdouble *lti_funds);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* GNC_COGNITIVE_ACCOUNTING_H */
/** @} */
/** @} */
