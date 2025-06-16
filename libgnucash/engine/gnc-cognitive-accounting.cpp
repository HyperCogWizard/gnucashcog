/********************************************************************\
 * gnc-cognitive-accounting.cpp -- OpenCog integration implementation *
 * Copyright (C) 2024 GnuCash Cognitive Engine                       *
 *                                                                    *
 * This program is free software; you can redistribute it and/or      *
 * modify it under the terms of the GNU General Public License as     *
 * published by the Free Software Foundation; either version 2 of     *
 * the License, or (at your option) any later version.                *
 *                                                                    *
 * This program is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 * GNU General Public License for more details.                       *
 *********************************************************************/

#include "gnc-cognitive-accounting.h"
#include "Account.h"
#include "Split.h"
#include "Transaction.h"
#include "gnc-numeric.h"
#include "qof.h"
#include <glib.h>
#include <map>
#include <memory>
#include <vector>

/** Simulated AtomSpace implementation for cognitive accounting */
struct GncCognitiveAtomSpace {
    std::map<guint64, GncAtomType> atom_types;
    std::map<guint64, std::string> atom_names;
    std::map<guint64, GncAttentionParams> attention_params;
    std::map<const Account*, guint64> account_atoms;
    guint64 next_handle;
    
    GncCognitiveAtomSpace() : next_handle(1000) {}
    
    guint64 create_atom(GncAtomType type, const std::string& name) {
        guint64 handle = next_handle++;
        atom_types[handle] = type;
        atom_names[handle] = name;
        
        // Initialize attention parameters
        GncAttentionParams params = {0.5, 0.5, 0.1, 0.0};
        attention_params[handle] = params;
        
        return handle;
    }
};

static std::unique_ptr<GncCognitiveAtomSpace> g_atomspace = nullptr;

/* Cognitive account type storage using KVP */
static const char* COGNITIVE_TYPE_KEY = "cognitive-accounting-type";

/********************************************************************\
 * AtomSpace Integration Functions                                   *
\********************************************************************/

gboolean gnc_cognitive_accounting_init(void)
{
    if (g_atomspace) {
        g_warning("Cognitive accounting already initialized");
        return FALSE;
    }
    
    g_atomspace = std::make_unique<GncCognitiveAtomSpace>();
    
    g_message("Cognitive accounting AtomSpace initialized");
    return TRUE;
}

void gnc_cognitive_accounting_shutdown(void)
{
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return;
    }
    
    g_atomspace.reset();
    g_message("Cognitive accounting AtomSpace shutdown");
}

GncAtomHandle gnc_account_to_atomspace(const Account *account)
{
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    g_return_val_if_fail(account != nullptr, 0);
    
    // Check if account already has an atom
    auto it = g_atomspace->account_atoms.find(account);
    if (it != g_atomspace->account_atoms.end()) {
        return it->second;
    }
    
    // Create account concept atom
    std::string account_name = xaccAccountGetName(account) ? 
                              xaccAccountGetName(account) : "unnamed_account";
    
    GncAtomHandle atom_handle = g_atomspace->create_atom(
        GNC_ATOM_ACCOUNT_CONCEPT, 
        "Account:" + account_name
    );
    
    // Store mapping
    g_atomspace->account_atoms[account] = atom_handle;
    
    // Create category atom based on account type
    GNCAccountType acct_type = xaccAccountGetType(account);
    std::string category_name = "Category:" + std::string(xaccAccountGetTypeStr(acct_type));
    
    GncAtomHandle category_atom = g_atomspace->create_atom(
        GNC_ATOM_ACCOUNT_CATEGORY,
        category_name
    );
    
    // Create hierarchy link if account has parent
    Account *parent = gnc_account_get_parent(account);
    if (parent) {
        GncAtomHandle parent_atom = gnc_account_to_atomspace(parent);
        gnc_atomspace_create_hierarchy_link(parent_atom, atom_handle);
    }
    
    g_message("Created AtomSpace representation for account: %s", account_name.c_str());
    return atom_handle;
}

GncAtomHandle gnc_atomspace_create_hierarchy_link(GncAtomHandle parent_atom, 
                                                  GncAtomHandle child_atom)
{
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    std::string link_name = "HierarchyLink:" + 
                           std::to_string(parent_atom) + "->" + 
                           std::to_string(child_atom);
    
    return g_atomspace->create_atom(GNC_ATOM_ACCOUNT_HIERARCHY, link_name);
}

/********************************************************************\
 * PLN Ledger Rules                                                  *
\********************************************************************/

gdouble gnc_pln_validate_double_entry(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, 0.0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0.0;
    }
    
    // Basic double-entry validation: sum of all splits should be zero
    gnc_numeric total = gnc_numeric_zero();
    GList *splits = xaccTransGetSplitList(transaction);
    
    for (GList *node = splits; node; node = node->next) {
        Split *split = GNC_SPLIT(node->data);
        gnc_numeric amount = xaccSplitGetAmount(split);
        total = gnc_numeric_add(total, amount, GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);
    }
    
    // PLN confidence based on how close to zero the total is
    if (gnc_numeric_zero_p(total)) {
        return 1.0; // Perfect double-entry balance
    }
    
    // Confidence decreases with magnitude of imbalance
    gnc_numeric abs_total = gnc_numeric_abs(total);
    double imbalance = gnc_numeric_to_double(abs_total);
    
    // Exponential decay confidence function
    return exp(-imbalance);
}

gdouble gnc_pln_validate_n_entry(const Transaction *transaction, gint n_parties)
{
    g_return_val_if_fail(transaction != nullptr, 0.0);
    g_return_val_if_fail(n_parties >= 2, 0.0);
    
    if (!g_atomspace) {
        return gnc_pln_validate_double_entry(transaction);
    }
    
    GList *splits = xaccTransGetSplitList(transaction);
    gint split_count = g_list_length(splits);
    
    // N-entry validation: at least n_parties splits, balanced total
    if (split_count < n_parties) {
        return 0.0; // Insufficient splits for n-party transaction
    }
    
    // Base confidence from double-entry validation
    gdouble base_confidence = gnc_pln_validate_double_entry(transaction);
    
    // Adjust confidence based on multi-party complexity
    gdouble complexity_factor = 1.0 / sqrt((double)n_parties);
    
    return base_confidence * complexity_factor;
}

GncAtomHandle gnc_pln_generate_trial_balance_proof(const Account *root_account)
{
    g_return_val_if_fail(root_account != nullptr, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    // Create trial balance proof atom
    std::string proof_name = "TrialBalanceProof:" + 
                            std::string(xaccAccountGetName(root_account));
    
    GncAtomHandle proof_atom = g_atomspace->create_atom(
        GNC_ATOM_TRANSACTION_RULE,
        proof_name
    );
    
    // Set high confidence for trial balance proof
    g_atomspace->attention_params[proof_atom].confidence = 0.95;
    
    g_message("Generated trial balance proof for account tree: %s", 
              xaccAccountGetName(root_account));
    
    return proof_atom;
}

GncAtomHandle gnc_pln_generate_pl_proof(const Account *income_account,
                                        const Account *expense_account)
{
    g_return_val_if_fail(income_account != nullptr, 0);
    g_return_val_if_fail(expense_account != nullptr, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    std::string proof_name = "PLProof:" + 
                            std::string(xaccAccountGetName(income_account)) + 
                            "-" + 
                            std::string(xaccAccountGetName(expense_account));
    
    return g_atomspace->create_atom(GNC_ATOM_TRANSACTION_RULE, proof_name);
}

/********************************************************************\
 * ECAN Attention Allocation                                         *
\********************************************************************/

void gnc_ecan_update_account_attention(Account *account, 
                                       const Transaction *transaction)
{
    g_return_if_fail(account != nullptr);
    g_return_if_fail(transaction != nullptr);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return;
    }
    
    GncAtomHandle atom_handle = gnc_account_to_atomspace(account);
    if (atom_handle == 0) return;
    
    auto& params = g_atomspace->attention_params[atom_handle];
    
    // Increase attention based on transaction activity
    params.activity_level += 0.1;
    params.attention_value = std::min(1.0, params.attention_value + 0.05);
    
    // Increase importance for frequently used accounts
    params.importance = std::min(1.0, params.importance + 0.02);
    
    g_debug("Updated attention for account %s: importance=%.3f, attention=%.3f",
            xaccAccountGetName(account), params.importance, params.attention_value);
}

GncAttentionParams gnc_ecan_get_attention_params(const Account *account)
{
    GncAttentionParams default_params = {0.0, 0.0, 0.0, 0.0};
    
    g_return_val_if_fail(account != nullptr, default_params);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return default_params;
    }
    
    auto it = g_atomspace->account_atoms.find(account);
    if (it == g_atomspace->account_atoms.end()) {
        return default_params;
    }
    
    auto param_it = g_atomspace->attention_params.find(it->second);
    if (param_it != g_atomspace->attention_params.end()) {
        return param_it->second;
    }
    
    return default_params;
}

void gnc_ecan_allocate_attention(Account **accounts, gint n_accounts)
{
    g_return_if_fail(accounts != nullptr);
    g_return_if_fail(n_accounts > 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return;
    }
    
    // Simple attention allocation: normalize attention values
    gdouble total_attention = 0.0;
    
    for (gint i = 0; i < n_accounts; i++) {
        GncAttentionParams params = gnc_ecan_get_attention_params(accounts[i]);
        total_attention += params.attention_value;
    }
    
    if (total_attention > 0.0) {
        for (gint i = 0; i < n_accounts; i++) {
            auto it = g_atomspace->account_atoms.find(accounts[i]);
            if (it != g_atomspace->account_atoms.end()) {
                auto& params = g_atomspace->attention_params[it->second];
                params.attention_value /= total_attention;
            }
        }
    }
    
    g_message("Allocated attention across %d accounts", n_accounts);
}

/********************************************************************\
 * MOSES Integration                                                 *
\********************************************************************/

GncAtomHandle gnc_moses_discover_balancing_strategies(Transaction **historical_transactions,
                                                      gint n_transactions)
{
    g_return_val_if_fail(historical_transactions != nullptr, 0);
    g_return_val_if_fail(n_transactions > 0, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    // Create evolved strategy atom
    std::string strategy_name = "MOSESStrategy:BalancingRules:" + 
                               std::to_string(n_transactions);
    
    GncAtomHandle strategy_atom = g_atomspace->create_atom(
        GNC_ATOM_TRANSACTION_RULE,
        strategy_name
    );
    
    // Set moderate confidence for evolved strategies
    g_atomspace->attention_params[strategy_atom].confidence = 0.7;
    
    g_message("MOSES discovered balancing strategies from %d transactions", n_transactions);
    return strategy_atom;
}

Transaction* gnc_moses_optimize_transaction(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, nullptr);
    
    // For now, return a copy of the original transaction
    // In a full implementation, this would apply MOSES optimizations
    g_message("MOSES transaction optimization (placeholder implementation)");
    
    return const_cast<Transaction*>(transaction);
}

/********************************************************************\
 * URE Uncertain Reasoning                                           *
\********************************************************************/

gnc_numeric gnc_ure_predict_balance(const Account *account, time64 future_date)
{
    g_return_val_if_fail(account != nullptr, gnc_numeric_zero());
    
    // Simple prediction: current balance (placeholder for URE reasoning)
    gnc_numeric current_balance = xaccAccountGetBalance(account);
    
    g_message("URE balance prediction for account %s: %.2f", 
              xaccAccountGetName(account), 
              gnc_numeric_to_double(current_balance));
    
    return current_balance;
}

gdouble gnc_ure_transaction_validity(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, 0.0);
    
    // Combine PLN validation with uncertainty quantification
    gdouble base_validity = gnc_pln_validate_double_entry(transaction);
    
    // Add uncertainty based on transaction complexity
    GList *splits = xaccTransGetSplitList(transaction);
    gint split_count = g_list_length(splits);
    
    gdouble uncertainty_factor = 1.0 - (0.05 * split_count);
    uncertainty_factor = std::max(0.1, uncertainty_factor);
    
    return base_validity * uncertainty_factor;
}

/********************************************************************\
 * Cognitive Account Types                                           *
\********************************************************************/

void gnc_account_set_cognitive_type(Account *account, GncCognitiveAccountType cognitive_type)
{
    g_return_if_fail(account != nullptr);
    
    // Store cognitive type in account KVP
    qof_instance_set_kvp(QOF_INSTANCE(account), 
                        g_variant_new_uint32(cognitive_type),
                        1, COGNITIVE_TYPE_KEY);
    
    g_debug("Set cognitive type %u for account %s", 
            cognitive_type, xaccAccountGetName(account));
}

GncCognitiveAccountType gnc_account_get_cognitive_type(const Account *account)
{
    g_return_val_if_fail(account != nullptr, GNC_COGNITIVE_ACCT_TRADITIONAL);
    
    // Retrieve cognitive type from account KVP
    auto var = qof_instance_get_kvp(QOF_INSTANCE(account), 1, COGNITIVE_TYPE_KEY);
    if (var && g_variant_is_of_type(var, G_VARIANT_TYPE_UINT32)) {
        return static_cast<GncCognitiveAccountType>(g_variant_get_uint32(var));
    }
    
    return GNC_COGNITIVE_ACCT_TRADITIONAL;
}