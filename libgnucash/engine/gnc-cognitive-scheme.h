/********************************************************************\
 * gnc-cognitive-scheme.h -- Scheme export helpers for cognitive    *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

#ifndef GNC_COGNITIVE_SCHEME_H
#define GNC_COGNITIVE_SCHEME_H

#include "gnc-cognitive-accounting.h"
#include "Account.h"
#include "Transaction.h"
#include "gnc-engine.h"

#ifdef __cplusplus
extern "C" {
#endif

gboolean gnc_cognitive_scheme_init(void);
gchar* gnc_cognitive_scheme_eval(const gchar* scheme_code);
GncAtomHandle gnc_scheme_create_hypergraph_pattern(const gchar* pattern_scheme);
void gnc_scheme_register_account_patterns(Account *account);
void gnc_scheme_register_transaction_patterns(Transaction *transaction);
void gnc_scheme_trigger_attention_update(Account *account, gdouble activity_level);
void gnc_scheme_evolutionary_optimization(Transaction **transactions, gint n_transactions);
gchar* gnc_scheme_uncertain_prediction(Account *account, time64 future_date);
void gnc_scheme_neural_symbolic_analysis(Account *account);
void gnc_scheme_emergent_insight_discovery(Transaction **transactions, gint n_transactions);

#ifdef __cplusplus
}
#endif

#endif /* GNC_COGNITIVE_SCHEME_H */
