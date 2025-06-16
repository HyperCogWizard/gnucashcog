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

/** Account node types in AtomSpace hierarchy - aligned with OpenCog atom types */
typedef enum {
    /* Basic node types */
    GNC_ATOM_CONCEPT_NODE,        /**< ConceptNode - basic account concepts */
    GNC_ATOM_PREDICATE_NODE,      /**< PredicateNode - balance and validation predicates */
    GNC_ATOM_SCHEMA_NODE,         /**< SchemaNode - transaction schemas and rules */
    GNC_ATOM_GROUNDED_SCHEMA,     /**< GroundedSchemaNode - executable accounting procedures */
    
    /* Link types for relationships */
    GNC_ATOM_INHERITANCE_LINK,    /**< InheritanceLink - account type hierarchy */
    GNC_ATOM_SIMILARITY_LINK,     /**< SimilarityLink - account pattern relationships */
    GNC_ATOM_MEMBER_LINK,         /**< MemberLink - account membership in categories */
    GNC_ATOM_EVALUATION_LINK,     /**< EvaluationLink - balance and rule evaluations */
    GNC_ATOM_EXECUTION_LINK,      /**< ExecutionLink - transaction execution */
    
    /* PLN-specific types */
    GNC_ATOM_IMPLICATION_LINK,    /**< ImplicationLink - ledger rules and constraints */
    GNC_ATOM_AND_LINK,            /**< AndLink - conjunction of conditions */
    GNC_ATOM_OR_LINK,             /**< OrLink - disjunction of alternatives */
    
    /* MOSES-specific types */
    GNC_ATOM_COMBO_NODE,          /**< ComboNode - evolved accounting strategies */
    
    /* Legacy compatibility */
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
    /* Short-term importance (STI) */
    gdouble sti;                  /**< Short-term importance value */
    gdouble sti_funds;            /**< Available STI funds for allocation */
    
    /* Long-term importance (LTI) */ 
    gdouble lti;                  /**< Long-term importance value */
    gdouble lti_funds;            /**< Available LTI funds for allocation */
    
    /* Very long-term importance (VLTI) */
    gdouble vlti;                 /**< Very long-term importance value */
    
    /* Confidence and truth values */
    gdouble confidence;           /**< PLN confidence level */
    gdouble strength;             /**< PLN strength/truth value */
    
    /* Activity and wages */
    gdouble activity_level;       /**< Account activity level */
    gdouble wage;                 /**< Cognitive wage for attention allocation */
    gdouble rent;                 /**< Cognitive rent for maintaining attention */
    
    /* Legacy compatibility */
    gdouble importance;           /**< Legacy: maps to STI */
    gdouble attention_value;      /**< Legacy: maps to total attention */
} GncAttentionParams;

/** @} */

/** @name OpenCog-style AtomSpace Operations */
/** @{ */

/** Create OpenCog-style ConceptNode for account
 * @param name Concept name
 * @return Handle to concept node atom
 */
GncAtomHandle gnc_atomspace_create_concept_node(const char* name);

/** Create OpenCog-style PredicateNode for balance evaluation
 * @param name Predicate name  
 * @return Handle to predicate node atom
 */
GncAtomHandle gnc_atomspace_create_predicate_node(const char* name);

/** Create OpenCog-style EvaluationLink for account balance
 * @param predicate_atom Predicate atom handle
 * @param account_atom Account atom handle
 * @param truth_value Truth value for the evaluation
 * @return Handle to evaluation link atom
 */
GncAtomHandle gnc_atomspace_create_evaluation_link(GncAtomHandle predicate_atom,
                                                   GncAtomHandle account_atom,
                                                   gdouble truth_value);

/** Create OpenCog-style InheritanceLink for account hierarchy
 * @param child_atom Child account atom handle
 * @param parent_atom Parent account atom handle  
 * @return Handle to inheritance link atom
 */
GncAtomHandle gnc_atomspace_create_inheritance_link(GncAtomHandle child_atom,
                                                    GncAtomHandle parent_atom);

/** Set truth value for atom (OpenCog-style)
 * @param atom_handle Atom to update
 * @param strength Truth strength (0.0-1.0)
 * @param confidence Truth confidence (0.0-1.0)
 */
void gnc_atomspace_set_truth_value(GncAtomHandle atom_handle, 
                                   gdouble strength, gdouble confidence);

/** Get truth value for atom (OpenCog-style)
 * @param atom_handle Atom to query
 * @param strength Output parameter for truth strength
 * @param confidence Output parameter for truth confidence
 * @return TRUE if truth value exists, FALSE otherwise
 */
gboolean gnc_atomspace_get_truth_value(GncAtomHandle atom_handle,
                                       gdouble* strength, gdouble* confidence);

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

/** @name Scheme-based Cognitive Representations */
/** @{ */

/** Generate Scheme representation of account in AtomSpace
 * @param account The account to represent
 * @return Allocated string containing Scheme code (caller must free)
 */
char* gnc_account_to_scheme_representation(const Account *account);

/** Generate Scheme representation of transaction pattern
 * @param transaction The transaction to represent
 * @return Allocated string containing Scheme code (caller must free)
 */
char* gnc_transaction_to_scheme_pattern(const Transaction *transaction);

/** Evaluate Scheme expression in cognitive context
 * @param scheme_expr Scheme expression to evaluate
 * @return Result of evaluation (implementation-dependent)
 */
GncAtomHandle gnc_evaluate_scheme_expression(const char* scheme_expr);

/** Create hypergraph pattern encoding for account hierarchy
 * @param root_account Root of account hierarchy
 * @return Allocated string with hypergraph pattern (caller must free)
 */
char* gnc_create_hypergraph_pattern_encoding(const Account *root_account);

/** @} */

/** @name Inter-Module Communication Protocols */
/** @{ */

/** Cognitive message for inter-module communication */
typedef struct {
    const char* source_module;    /**< Source module name */
    const char* target_module;    /**< Target module name */
    const char* message_type;     /**< Type of cognitive message */
    GncAtomHandle payload_atom;   /**< Atom containing message payload */
    gdouble priority;             /**< Message priority for attention allocation */
    time64 timestamp;             /**< Message timestamp */
} GncCognitiveMessage;

/** Send cognitive message between modules
 * @param message Cognitive message to send
 * @return TRUE on successful delivery, FALSE otherwise
 */
gboolean gnc_send_cognitive_message(const GncCognitiveMessage* message);

/** Register cognitive message handler for module
 * @param module_name Name of module to register for
 * @param handler_func Function to handle received messages
 * @return TRUE on successful registration, FALSE otherwise
 */
typedef void (*GncCognitiveMessageHandler)(const GncCognitiveMessage* message);
gboolean gnc_register_cognitive_message_handler(const char* module_name,
                                               GncCognitiveMessageHandler handler_func);

/** @} */

/** @name Distributed Cognition and Emergent Behavior */
/** @{ */

/** Cognitive emergence detection parameters */
typedef struct {
    gdouble complexity_threshold;  /**< Minimum complexity for emergence detection */
    gdouble coherence_measure;     /**< Coherence in cognitive patterns */
    gdouble novelty_score;         /**< Novelty of emerging patterns */
    gint pattern_frequency;        /**< Frequency of pattern occurrence */
} GncEmergenceParams;

/** Detect emergent cognitive patterns in account activity
 * @param accounts Array of accounts to analyze
 * @param n_accounts Number of accounts
 * @param params Parameters for emergence detection
 * @return Handle to atom representing detected emergent pattern
 */
GncAtomHandle gnc_detect_emergent_patterns(Account** accounts, gint n_accounts,
                                          const GncEmergenceParams* params);

/** Optimize attention allocation for distributed cognition
 * @param cognitive_load Current cognitive load across modules
 * @param available_resources Available cognitive resources
 * @return Optimized attention allocation strategy
 */
GncAtomHandle gnc_optimize_distributed_attention(gdouble cognitive_load,
                                                gdouble available_resources);

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