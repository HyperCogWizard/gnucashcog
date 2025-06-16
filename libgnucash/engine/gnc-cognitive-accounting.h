/********************************************************************\
 * gnc-cognitive-accounting.h -- OpenCog integration for accounting *
 * Copyright (C) 2024 GnuCash Cognitive Engine                     *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 ********************************************************************/

/** @addtogroup Engine
    @{ */
/** @addtogroup CognitiveAccounting
    Cognitive accounting functionality using OpenCog AtomSpace for
    representing Chart of Accounts as hypergraph structures and PLN
    for reasoning about ledger rules and balance validation.
    @{ */

/** @file gnc-cognitive-accounting.h
    @brief Cognitive accounting with AtomSpace and PLN integration
    @author Copyright (C) 2024 GnuCash Cognitive Engine
*/

#ifndef GNC_COGNITIVE_ACCOUNTING_H
#define GNC_COGNITIVE_ACCOUNTING_H

#include "Account.h"
#include "Transaction.h"
#include "gnc-engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name AtomSpace Account Representation */
/** @{ */

/** AtomSpace handle type for representing accounts as atoms */
typedef guint64 GncAtomHandle;

/** Account node types in AtomSpace hierarchy */
typedef enum {
    GNC_ATOM_ACCOUNT_CONCEPT,     /**< Basic account concept atom */
    GNC_ATOM_ACCOUNT_CATEGORY,    /**< Account category/type atom */
    GNC_ATOM_ACCOUNT_HIERARCHY,   /**< Account hierarchy link */
    GNC_ATOM_ACCOUNT_BALANCE,     /**< Account balance predicate */
    GNC_ATOM_TRANSACTION_RULE,    /**< Transaction validation rule */
    GNC_ATOM_DOUBLE_ENTRY_RULE,   /**< Double-entry constraint */
    GNC_ATOM_N_ENTRY_RULE         /**< N-entry extension rule */
} GncAtomType;

/** Cognitive attention allocation parameters */
typedef struct {
    gdouble importance;           /**< ECAN importance value */
    gdouble confidence;           /**< PLN confidence level */
    gdouble attention_value;      /**< Current attention allocation */
    gdouble activity_level;       /**< Account activity level */
} GncAttentionParams;

/** @} */

/** @name AtomSpace Integration Functions */
/** @{ */

/** Initialize cognitive accounting AtomSpace 
 * @return TRUE on success, FALSE on failure
 */
gboolean gnc_cognitive_accounting_init(void);

/** Shutdown cognitive accounting and cleanup AtomSpace */
void gnc_cognitive_accounting_shutdown(void);

/** Convert traditional account to AtomSpace representation
 * @param account The GnuCash account to convert
 * @return AtomSpace handle for the account atom
 */
GncAtomHandle gnc_account_to_atomspace(const Account *account);

/** Create account hierarchy links in AtomSpace
 * @param parent_atom Parent account atom handle
 * @param child_atom Child account atom handle
 * @return Handle to hierarchy link atom
 */
GncAtomHandle gnc_atomspace_create_hierarchy_link(GncAtomHandle parent_atom, 
                                                  GncAtomHandle child_atom);

/** @} */

/** @name PLN Ledger Rules */
/** @{ */

/** Validate double-entry transaction using PLN
 * @param transaction The transaction to validate
 * @return Confidence level (0.0-1.0) of double-entry validity
 */
gdouble gnc_pln_validate_double_entry(const Transaction *transaction);

/** Validate n-entry transaction for multi-party scenarios
 * @param transaction The transaction to validate
 * @param n_parties Number of parties involved
 * @return Confidence level of n-entry validity
 */
gdouble gnc_pln_validate_n_entry(const Transaction *transaction, gint n_parties);

/** Generate trial balance proof using PLN reasoning
 * @param root_account Root account for balance calculation
 * @return PLN proof handle for trial balance
 */
GncAtomHandle gnc_pln_generate_trial_balance_proof(const Account *root_account);

/** Generate profit & loss proof using PLN
 * @param income_account Income account root
 * @param expense_account Expense account root
 * @return PLN proof handle for P&L statement
 */
GncAtomHandle gnc_pln_generate_pl_proof(const Account *income_account,
                                        const Account *expense_account);

/** @} */

/** @name ECAN Attention Allocation */
/** @{ */

/** Update attention allocation for account based on activity
 * @param account The account to update
 * @param transaction Recent transaction affecting the account
 */
void gnc_ecan_update_account_attention(Account *account, 
                                       const Transaction *transaction);

/** Get current attention parameters for account
 * @param account The account to query
 * @return Attention allocation parameters
 */
GncAttentionParams gnc_ecan_get_attention_params(const Account *account);

/** Allocate cognitive resources based on attention dynamics
 * @param accounts Array of accounts to consider
 * @param n_accounts Number of accounts in array
 */
void gnc_ecan_allocate_attention(Account **accounts, gint n_accounts);

/** @} */

/** @name MOSES Integration */
/** @{ */

/** Discover novel ledger balancing strategies using MOSES
 * @param historical_transactions Array of historical transactions
 * @param n_transactions Number of transactions
 * @return Handle to evolved balancing rule set
 */
GncAtomHandle gnc_moses_discover_balancing_strategies(Transaction **historical_transactions,
                                                      gint n_transactions);

/** Apply MOSES-evolved rules to optimize transaction patterns
 * @param transaction Transaction to optimize
 * @return Optimized transaction structure
 */
Transaction* gnc_moses_optimize_transaction(const Transaction *transaction);

/** @} */

/** @name URE Uncertain Reasoning */
/** @{ */

/** Apply uncertain reasoning to account balance predictions
 * @param account Account for prediction
 * @param future_date Date for balance prediction
 * @return Predicted balance with uncertainty bounds
 */
gnc_numeric gnc_ure_predict_balance(const Account *account, time64 future_date);

/** Reason about transaction validity under uncertainty
 * @param transaction Transaction to analyze
 * @return Validity probability with uncertainty quantification
 */
gdouble gnc_ure_transaction_validity(const Transaction *transaction);

/** @} */

/** @name Cognitive Account Types */
/** @{ */

/** Enhanced account types for cognitive accounting */
typedef enum {
    GNC_COGNITIVE_ACCT_TRADITIONAL = 0x0000,  /**< Standard account */
    GNC_COGNITIVE_ACCT_ADAPTIVE    = 0x0001,  /**< Adaptive learning account */
    GNC_COGNITIVE_ACCT_PREDICTIVE  = 0x0002,  /**< Predictive account */
    GNC_COGNITIVE_ACCT_MULTIMODAL  = 0x0004,  /**< Multi-modal transaction account */
    GNC_COGNITIVE_ACCT_ATTENTION   = 0x0008   /**< Attention-driven account */
} GncCognitiveAccountType;

/** Set cognitive account type flags
 * @param account Account to modify
 * @param cognitive_type Cognitive type flags
 */
void gnc_account_set_cognitive_type(Account *account, GncCognitiveAccountType cognitive_type);

/** Get cognitive account type flags
 * @param account Account to query
 * @return Current cognitive type flags
 */
GncCognitiveAccountType gnc_account_get_cognitive_type(const Account *account);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* GNC_COGNITIVE_ACCOUNTING_H */
/** @} */
/** @} */