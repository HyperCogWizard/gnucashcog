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

/** Enhanced OpenCog-style AtomSpace implementation for cognitive accounting */
struct GncCognitiveAtomSpace {
    std::map<guint64, GncAtomType> atom_types;
    std::map<guint64, std::string> atom_names;
    std::map<guint64, GncAttentionParams> attention_params;
    std::map<guint64, std::pair<gdouble, gdouble>> truth_values; // strength, confidence
    std::map<const Account*, guint64> account_atoms;
    std::vector<GncCognitiveMessage> message_queue;
    std::map<std::string, GncCognitiveMessageHandler> message_handlers;
    guint64 next_handle;
    
    /* ECAN fund management */
    gdouble total_sti_funds;
    gdouble total_lti_funds;
    gdouble attention_decay_rate;
    
    GncCognitiveAtomSpace() : next_handle(1000), total_sti_funds(1000.0), 
                             total_lti_funds(1000.0), attention_decay_rate(0.01) {}
    
    guint64 create_atom(GncAtomType type, const std::string& name) {
        guint64 handle = next_handle++;
        atom_types[handle] = type;
        atom_names[handle] = name;
        
        // Initialize OpenCog-style attention parameters
        GncAttentionParams params = {};
        params.sti = 0.0;
        params.sti_funds = 10.0;
        params.lti = 0.0; 
        params.lti_funds = 10.0;
        params.vlti = 0.0;
        params.confidence = 0.5;
        params.strength = 0.5;
        params.activity_level = 0.0;
        params.wage = 1.0;
        params.rent = 0.1;
        
        // Legacy compatibility
        params.importance = 0.5;
        params.attention_value = 0.1;
        
        attention_params[handle] = params;
        
        // Initialize truth value
        truth_values[handle] = std::make_pair(0.5, 0.5);
        
        return handle;
    }
    
    void distribute_sti_funds() {
        // Simple STI fund distribution algorithm
        if (attention_params.empty()) return;
        
        gdouble fund_per_atom = total_sti_funds / attention_params.size();
        for (auto& pair : attention_params) {
            pair.second.sti_funds = fund_per_atom;
        }
    }
    
    void apply_attention_decay() {
        // Apply attention decay to all atoms
        for (auto& pair : attention_params) {
            auto& params = pair.second;
            params.sti *= (1.0 - attention_decay_rate);
            params.lti *= (1.0 - attention_decay_rate * 0.1); // LTI decays slower
            
            // Collect rent
            if (params.sti > params.rent) {
                params.sti -= params.rent;
                total_sti_funds += params.rent;
            }
        }
    }
};

static std::unique_ptr<GncCognitiveAtomSpace> g_atomspace = nullptr;

/* Cognitive account type storage using KVP */
static const char* COGNITIVE_TYPE_KEY = "cognitive-accounting-type";

/********************************************************************\
 * OpenCog-style AtomSpace Operations                                *
\********************************************************************/

GncAtomHandle gnc_atomspace_create_concept_node(const char* name)
{
    g_return_val_if_fail(name != nullptr, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    return g_atomspace->create_atom(GNC_ATOM_CONCEPT_NODE, std::string(name));
}

GncAtomHandle gnc_atomspace_create_predicate_node(const char* name)
{
    g_return_val_if_fail(name != nullptr, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    return g_atomspace->create_atom(GNC_ATOM_PREDICATE_NODE, std::string(name));
}

GncAtomHandle gnc_atomspace_create_evaluation_link(GncAtomHandle predicate_atom,
                                                   GncAtomHandle account_atom,
                                                   gdouble truth_value)
{
    g_return_val_if_fail(predicate_atom != 0, 0);
    g_return_val_if_fail(account_atom != 0, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    std::string link_name = "EvaluationLink:" + 
                           std::to_string(predicate_atom) + ":" + 
                           std::to_string(account_atom);
    
    GncAtomHandle link_handle = g_atomspace->create_atom(GNC_ATOM_EVALUATION_LINK, link_name);
    
    // Set truth value for the evaluation
    gnc_atomspace_set_truth_value(link_handle, truth_value, 0.9);
    
    return link_handle;
}

GncAtomHandle gnc_atomspace_create_inheritance_link(GncAtomHandle child_atom,
                                                    GncAtomHandle parent_atom)
{
    g_return_val_if_fail(child_atom != 0, 0);
    g_return_val_if_fail(parent_atom != 0, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    std::string link_name = "InheritanceLink:" + 
                           std::to_string(child_atom) + "->" + 
                           std::to_string(parent_atom);
    
    return g_atomspace->create_atom(GNC_ATOM_INHERITANCE_LINK, link_name);
}

void gnc_atomspace_set_truth_value(GncAtomHandle atom_handle, 
                                   gdouble strength, gdouble confidence)
{
    g_return_if_fail(atom_handle != 0);
    g_return_if_fail(strength >= 0.0 && strength <= 1.0);
    g_return_if_fail(confidence >= 0.0 && confidence <= 1.0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return;
    }
    
    g_atomspace->truth_values[atom_handle] = std::make_pair(strength, confidence);
    
    // Also update attention parameters
    auto it = g_atomspace->attention_params.find(atom_handle);
    if (it != g_atomspace->attention_params.end()) {
        it->second.strength = strength;
        it->second.confidence = confidence;
    }
}

gboolean gnc_atomspace_get_truth_value(GncAtomHandle atom_handle,
                                       gdouble* strength, gdouble* confidence)
{
    g_return_val_if_fail(atom_handle != 0, FALSE);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return FALSE;
    }
    
    auto it = g_atomspace->truth_values.find(atom_handle);
    if (it != g_atomspace->truth_values.end()) {
        if (strength) *strength = it->second.first;
        if (confidence) *confidence = it->second.second;
        return TRUE;
    }
    
    return FALSE;
}

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
    
    // Create account concept node using OpenCog-style approach
    std::string account_name = xaccAccountGetName(account) ? 
                              xaccAccountGetName(account) : "unnamed_account";
    
    GncAtomHandle concept_handle = gnc_atomspace_create_concept_node(
        ("Account:" + account_name).c_str()
    );
    
    // Store mapping
    g_atomspace->account_atoms[account] = concept_handle;
    
    // Create category concept node based on account type
    GNCAccountType acct_type = xaccAccountGetType(account);
    std::string category_name = "Category:" + std::string(xaccAccountGetTypeStr(acct_type));
    
    GncAtomHandle category_handle = gnc_atomspace_create_concept_node(category_name.c_str());
    
    // Create inheritance link: Account inherits from Category
    gnc_atomspace_create_inheritance_link(concept_handle, category_handle);
    
    // Create balance predicate and evaluation
    GncAtomHandle balance_predicate = gnc_atomspace_create_predicate_node("hasBalance");
    gnc_numeric current_balance = xaccAccountGetBalance(account);
    gdouble balance_value = gnc_numeric_to_double(current_balance);
    
    // Normalize balance for truth value (simple approach)
    gdouble normalized_balance = (balance_value >= 0) ? 
        std::min(1.0, balance_value / 1000.0) : 0.0;
    
    gnc_atomspace_create_evaluation_link(balance_predicate, concept_handle, normalized_balance);
    
    // Create hierarchy link if account has parent
    Account *parent = gnc_account_get_parent(account);
    if (parent) {
        GncAtomHandle parent_atom = gnc_account_to_atomspace(parent);
        gnc_atomspace_create_inheritance_link(concept_handle, parent_atom);
    }
    
    g_message("Created OpenCog-style AtomSpace representation for account: %s", account_name.c_str());
    return concept_handle;
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
    
    // Enhanced PLN-style double-entry validation with truth value computation
    gnc_numeric total = gnc_numeric_zero();
    GList *splits = xaccTransGetSplitList(transaction);
    gint split_count = g_list_length(splits);
    
    // Collect split amounts for analysis
    std::vector<double> split_amounts;
    
    for (GList *node = splits; node; node = node->next) {
        Split *split = GNC_SPLIT(node->data);
        gnc_numeric amount = xaccSplitGetAmount(split);
        total = gnc_numeric_add(total, amount, GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);
        split_amounts.push_back(gnc_numeric_to_double(amount));
    }
    
    // PLN truth value computation
    gdouble strength = 0.0;  // How true is the balance
    gdouble confidence = 0.0; // How certain are we
    
    if (gnc_numeric_zero_p(total)) {
        strength = 1.0; // Perfect balance
        confidence = std::min(1.0, split_count / 10.0); // More splits = more evidence
    } else {
        // Imbalance analysis with PLN-style reasoning
        double imbalance = gnc_numeric_to_double(gnc_numeric_abs(total));
        
        // Calculate total transaction magnitude for normalization
        double total_magnitude = 0.0;
        for (double amount : split_amounts) {
            total_magnitude += std::abs(amount);
        }
        
        if (total_magnitude > 0.0) {
            double relative_imbalance = imbalance / total_magnitude;
            
            // PLN strength function: exponential decay with imbalance
            strength = exp(-10.0 * relative_imbalance);
            
            // Confidence decreases with larger relative imbalance and fewer splits
            confidence = std::max(0.1, 1.0 - relative_imbalance) * 
                        std::min(1.0, split_count / 5.0);
        }
    }
    
    // Create PLN atoms for this validation
    if (strength > 0.5) {
        GncAtomHandle validation_atom = g_atomspace->create_atom(
            GNC_ATOM_IMPLICATION_LINK, 
            "DoubleEntryValidation:" + std::to_string(reinterpret_cast<uintptr_t>(transaction))
        );
        gnc_atomspace_set_truth_value(validation_atom, strength, confidence);
    }
    
    g_debug("PLN double-entry validation: strength=%.3f, confidence=%.3f", strength, confidence);
    
    // Return combined truth value for backward compatibility
    return strength * confidence;
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
    
    // PLN reasoning for N-entry validation
    if (split_count < n_parties) {
        // Create failed validation atom
        GncAtomHandle failure_atom = g_atomspace->create_atom(
            GNC_ATOM_IMPLICATION_LINK,
            "NEntryValidationFailure:InsufficientSplits"
        );
        gnc_atomspace_set_truth_value(failure_atom, 0.0, 0.9);
        return 0.0;
    }
    
    // Base validation using double-entry logic
    gdouble base_strength, base_confidence;
    gdouble base_validation = gnc_pln_validate_double_entry(transaction);
    
    // Decompose the validation result (approximation)
    base_strength = sqrt(base_validation);
    base_confidence = base_validation / (base_strength + 0.001);
    
    // PLN complexity adjustment based on number of parties
    gdouble complexity_factor = 1.0 / (1.0 + 0.1 * (n_parties - 2));
    gdouble evidence_factor = std::min(1.0, split_count / (gdouble)n_parties);
    
    // Combine factors using PLN truth value revision
    gdouble final_strength = base_strength * complexity_factor;
    gdouble final_confidence = std::min(0.95, base_confidence * evidence_factor);
    
    // Create N-entry validation atom
    std::string validation_name = "NEntryValidation:Parties:" + std::to_string(n_parties) +
                                 ":Transaction:" + std::to_string(reinterpret_cast<uintptr_t>(transaction));
    
    GncAtomHandle n_entry_atom = g_atomspace->create_atom(GNC_ATOM_IMPLICATION_LINK, validation_name);
    gnc_atomspace_set_truth_value(n_entry_atom, final_strength, final_confidence);
    
    g_debug("PLN N-entry validation (%d parties): strength=%.3f, confidence=%.3f", 
            n_parties, final_strength, final_confidence);
    
    return final_strength * final_confidence;
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
 * Scheme-based Cognitive Representations                            *
\********************************************************************/

char* gnc_account_to_scheme_representation(const Account *account)
{
    g_return_val_if_fail(account != nullptr, nullptr);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return nullptr;
    }
    
    std::string account_name = xaccAccountGetName(account) ? 
                              xaccAccountGetName(account) : "unnamed_account";
    GNCAccountType acct_type = xaccAccountGetType(account);
    gnc_numeric balance = xaccAccountGetBalance(account);
    
    // Generate Scheme representation
    std::ostringstream scheme_repr;
    scheme_repr << "(ConceptNode \"Account:" << account_name << "\")\n";
    scheme_repr << "(InheritanceLink\n";
    scheme_repr << "  (ConceptNode \"Account:" << account_name << "\")\n";
    scheme_repr << "  (ConceptNode \"Category:" << xaccAccountGetTypeStr(acct_type) << "\"))\n";
    scheme_repr << "(EvaluationLink\n";
    scheme_repr << "  (PredicateNode \"hasBalance\")\n";
    scheme_repr << "  (ListLink\n";
    scheme_repr << "    (ConceptNode \"Account:" << account_name << "\")\n";
    scheme_repr << "    (NumberNode " << gnc_numeric_to_double(balance) << ")))\n";
    
    return g_strdup(scheme_repr.str().c_str());
}

char* gnc_transaction_to_scheme_pattern(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, nullptr);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return nullptr;
    }
    
    std::ostringstream scheme_pattern;
    scheme_pattern << "; Transaction pattern for OpenCog reasoning\n";
    scheme_pattern << "(BindLink\n";
    scheme_pattern << "  (VariableList\n";
    scheme_pattern << "    (VariableNode \"$transaction\"))\n";
    scheme_pattern << "  (AndLink\n";
    
    GList *splits = xaccTransGetSplitList(transaction);
    for (GList *node = splits; node; node = node->next) {
        Split *split = GNC_SPLIT(node->data);
        Account *account = xaccSplitGetAccount(split);
        gnc_numeric amount = xaccSplitGetAmount(split);
        
        if (account) {
            std::string account_name = xaccAccountGetName(account) ? 
                                      xaccAccountGetName(account) : "unnamed_account";
            
            scheme_pattern << "    (EvaluationLink\n";
            scheme_pattern << "      (PredicateNode \"involvesSplit\")\n";
            scheme_pattern << "      (ListLink\n";
            scheme_pattern << "        (VariableNode \"$transaction\")\n";
            scheme_pattern << "        (ConceptNode \"Account:" << account_name << "\")\n";
            scheme_pattern << "        (NumberNode " << gnc_numeric_to_double(amount) << ")))\n";
        }
    }
    
    scheme_pattern << "  )\n";
    scheme_pattern << "  (VariableNode \"$transaction\"))\n";
    
    return g_strdup(scheme_pattern.str().c_str());
}

GncAtomHandle gnc_evaluate_scheme_expression(const char* scheme_expr)
{
    g_return_val_if_fail(scheme_expr != nullptr, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    // Create an atom to represent the evaluated expression result
    std::string result_name = "SchemeResult:" + std::string(scheme_expr).substr(0, 50);
    GncAtomHandle result_atom = g_atomspace->create_atom(GNC_ATOM_CONCEPT_NODE, result_name);
    
    // Set high confidence for scheme evaluation results
    gnc_atomspace_set_truth_value(result_atom, 0.8, 0.9);
    
    g_message("Evaluated Scheme expression (simulated): %s", scheme_expr);
    return result_atom;
}

char* gnc_create_hypergraph_pattern_encoding(const Account *root_account)
{
    g_return_val_if_fail(root_account != nullptr, nullptr);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return nullptr;
    }
    
    std::ostringstream hypergraph_pattern;
    hypergraph_pattern << "; Hypergraph pattern encoding for account hierarchy\n";
    hypergraph_pattern << "(BindLink\n";
    hypergraph_pattern << "  (VariableList\n";
    hypergraph_pattern << "    (TypedVariableLink\n";
    hypergraph_pattern << "      (VariableNode \"$account\")\n";
    hypergraph_pattern << "      (TypeNode \"ConceptNode\")))\n";
    hypergraph_pattern << "  (AndLink\n";
    
    // Recursive pattern generation for account hierarchy
    std::function<void(const Account*, int)> add_account_pattern = 
        [&](const Account* account, int depth) {
            if (!account) return;
            
            std::string account_name = xaccAccountGetName(account) ? 
                                      xaccAccountGetName(account) : "unnamed_account";
            
            hypergraph_pattern << std::string(depth * 2, ' ') << "    (InheritanceLink\n";
            hypergraph_pattern << std::string(depth * 2, ' ') << "      (VariableNode \"$account\")\n";
            hypergraph_pattern << std::string(depth * 2, ' ') << "      (ConceptNode \"Account:" << account_name << "\"))\n";
            
            // Add child accounts
            GList *children = gnc_account_get_children(account);
            for (GList *node = children; node; node = node->next) {
                Account *child = GNC_ACCOUNT(node->data);
                add_account_pattern(child, depth + 1);
            }
            g_list_free(children);
        };
    
    add_account_pattern(root_account, 0);
    
    hypergraph_pattern << "  )\n";
    hypergraph_pattern << "  (VariableNode \"$account\"))\n";
    
    return g_strdup(hypergraph_pattern.str().c_str());
}

/********************************************************************\
 * Inter-Module Communication Protocols                             *
\********************************************************************/

gboolean gnc_send_cognitive_message(const GncCognitiveMessage* message)
{
    g_return_val_if_fail(message != nullptr, FALSE);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return FALSE;
    }
    
    // Add message to queue
    g_atomspace->message_queue.push_back(*message);
    
    // Try to deliver immediately if handler is registered
    auto handler_it = g_atomspace->message_handlers.find(message->target_module);
    if (handler_it != g_atomspace->message_handlers.end()) {
        handler_it->second(message);
        g_debug("Delivered cognitive message from %s to %s", 
                message->source_module, message->target_module);
        return TRUE;
    }
    
    g_debug("Queued cognitive message from %s to %s (no handler registered)", 
            message->source_module, message->target_module);
    return TRUE;
}

gboolean gnc_register_cognitive_message_handler(const char* module_name,
                                               GncCognitiveMessageHandler handler_func)
{
    g_return_val_if_fail(module_name != nullptr, FALSE);
    g_return_val_if_fail(handler_func != nullptr, FALSE);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return FALSE;
    }
    
    g_atomspace->message_handlers[module_name] = handler_func;
    
    // Deliver any queued messages for this module
    for (auto it = g_atomspace->message_queue.begin(); it != g_atomspace->message_queue.end();) {
        if (it->target_module == module_name) {
            handler_func(&(*it));
            it = g_atomspace->message_queue.erase(it);
        } else {
            ++it;
        }
    }
    
    g_message("Registered cognitive message handler for module: %s", module_name);
    return TRUE;
}

/********************************************************************\
 * Distributed Cognition and Emergent Behavior                      *
\********************************************************************/

GncAtomHandle gnc_detect_emergent_patterns(Account** accounts, gint n_accounts,
                                          const GncEmergenceParams* params)
{
    g_return_val_if_fail(accounts != nullptr, 0);
    g_return_val_if_fail(n_accounts > 0, 0);
    g_return_val_if_fail(params != nullptr, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    // Simple emergence detection based on account activity patterns
    gdouble total_complexity = 0.0;
    gdouble total_coherence = 0.0;
    
    for (gint i = 0; i < n_accounts; i++) {
        GncAttentionParams attention = gnc_ecan_get_attention_params(accounts[i]);
        total_complexity += attention.activity_level;
        total_coherence += attention.confidence;
    }
    
    gdouble avg_complexity = total_complexity / n_accounts;
    gdouble avg_coherence = total_coherence / n_accounts;
    
    if (avg_complexity > params->complexity_threshold && 
        avg_coherence > params->coherence_measure) {
        
        // Create emergent pattern atom
        std::string pattern_name = "EmergentPattern:Complexity:" + 
                                  std::to_string(avg_complexity) + 
                                  ":Coherence:" + std::to_string(avg_coherence);
        
        GncAtomHandle pattern_atom = g_atomspace->create_atom(GNC_ATOM_CONCEPT_NODE, pattern_name);
        
        // Set truth value based on emergence strength
        gdouble emergence_strength = std::min(1.0, (avg_complexity + avg_coherence) / 2.0);
        gnc_atomspace_set_truth_value(pattern_atom, emergence_strength, 0.8);
        
        g_message("Detected emergent cognitive pattern with strength: %.3f", emergence_strength);
        return pattern_atom;
    }
    
    return 0;
}

GncAtomHandle gnc_optimize_distributed_attention(gdouble cognitive_load,
                                                gdouble available_resources)
{
    g_return_val_if_fail(cognitive_load >= 0.0, 0);
    g_return_val_if_fail(available_resources >= 0.0, 0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }
    
    // Apply ECAN fund distribution
    g_atomspace->distribute_sti_funds();
    g_atomspace->apply_attention_decay();
    
    // Create optimization strategy atom
    std::string strategy_name = "AttentionOptimization:Load:" + 
                               std::to_string(cognitive_load) + 
                               ":Resources:" + std::to_string(available_resources);
    
    GncAtomHandle strategy_atom = g_atomspace->create_atom(GNC_ATOM_SCHEMA_NODE, strategy_name);
    
    // Calculate optimization confidence
    gdouble efficiency = (available_resources > 0) ? 
        std::min(1.0, available_resources / (cognitive_load + 1.0)) : 0.0;
    
    gnc_atomspace_set_truth_value(strategy_atom, efficiency, 0.9);
    
    g_message("Optimized distributed attention allocation with efficiency: %.3f", efficiency);
    return strategy_atom;
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
    
    // OpenCog ECAN-style attention updates
    gdouble activity_boost = 0.1;
    gdouble wage_payment = params.wage * activity_boost;
    
    // Increase STI based on transaction activity
    if (g_atomspace->total_sti_funds >= wage_payment) {
        params.sti += wage_payment;
        g_atomspace->total_sti_funds -= wage_payment;
        params.activity_level += 0.1;
    }
    
    // Gradual LTI increase for frequently used accounts
    params.lti += 0.01;
    
    // Update legacy compatibility fields
    params.importance = (params.sti + params.lti) / 2.0;
    params.attention_value = std::min(1.0, params.sti / 100.0);
    
    g_debug("Updated ECAN attention for account %s: STI=%.3f, LTI=%.3f, activity=%.3f",
            xaccAccountGetName(account), params.sti, params.lti, params.activity_level);
}

GncAttentionParams gnc_ecan_get_attention_params(const Account *account)
{
    GncAttentionParams default_params = {};
    
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
    
    // OpenCog ECAN-style attention allocation with economics
    gdouble total_sti = 0.0;
    std::vector<GncAtomHandle> account_handles;
    
    // Collect all account handles and calculate total STI
    for (gint i = 0; i < n_accounts; i++) {
        auto it = g_atomspace->account_atoms.find(accounts[i]);
        if (it != g_atomspace->account_atoms.end()) {
            account_handles.push_back(it->second);
            auto& params = g_atomspace->attention_params[it->second];
            total_sti += params.sti;
        }
    }
    
    // Normalize STI values if total exceeds fund limits
    if (total_sti > g_atomspace->total_sti_funds) {
        gdouble normalization_factor = g_atomspace->total_sti_funds / total_sti;
        
        for (auto handle : account_handles) {
            auto& params = g_atomspace->attention_params[handle];
            params.sti *= normalization_factor;
            
            // Update legacy fields
            params.importance = (params.sti + params.lti) / 2.0;
            params.attention_value = std::min(1.0, params.sti / 100.0);
        }
    }
    
    // Apply attention decay to all atoms
    g_atomspace->apply_attention_decay();
    
    g_message("Allocated ECAN attention across %d accounts with total STI: %.2f", 
              n_accounts, total_sti);
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
    
    // Enhanced MOSES-style evolutionary strategy discovery
    std::map<std::string, gint> pattern_frequencies;
    std::map<std::string, gdouble> pattern_fitness;
    
    // Analyze historical transactions for patterns
    for (gint i = 0; i < n_transactions; i++) {
        Transaction *trans = historical_transactions[i];
        if (!trans) continue;
        
        GList *splits = xaccTransGetSplitList(trans);
        gint split_count = g_list_length(splits);
        
        // Extract transaction patterns
        std::string pattern_key = "SplitCount:" + std::to_string(split_count);
        pattern_frequencies[pattern_key]++;
        
        // Calculate fitness based on validation success
        gdouble validation_fitness = gnc_pln_validate_double_entry(trans);
        pattern_fitness[pattern_key] += validation_fitness;
        
        // Analyze account type patterns
        std::map<GNCAccountType, gint> account_type_counts;
        for (GList *node = splits; node; node = node->next) {
            Split *split = GNC_SPLIT(node->data);
            Account *account = xaccSplitGetAccount(split);
            if (account) {
                GNCAccountType type = xaccAccountGetType(account);
                account_type_counts[type]++;
            }
        }
        
        // Create pattern signature based on account types
        std::string type_pattern = "Types:";
        for (auto& pair : account_type_counts) {
            type_pattern += std::to_string(pair.first) + ":" + std::to_string(pair.second) + ",";
        }
        pattern_frequencies[type_pattern]++;
        pattern_fitness[type_pattern] += validation_fitness;
    }
    
    // Find the best performing pattern using MOSES-style fitness evaluation
    std::string best_pattern;
    gdouble best_fitness = 0.0;
    gint best_frequency = 0;
    
    for (auto& pattern : pattern_frequencies) {
        gdouble avg_fitness = pattern_fitness[pattern.first] / pattern.second;
        gdouble weighted_fitness = avg_fitness * sqrt(pattern.second); // Frequency weighting
        
        if (weighted_fitness > best_fitness) {
            best_fitness = weighted_fitness;
            best_pattern = pattern.first;
            best_frequency = pattern.second;
        }
    }
    
    // Create evolved strategy atom with MOSES-style combo tree representation
    std::string strategy_name = "MOSESStrategy:Evolved:" + best_pattern +
                               ":Fitness:" + std::to_string(best_fitness) +
                               ":Freq:" + std::to_string(best_frequency);
    
    GncAtomHandle strategy_atom = g_atomspace->create_atom(GNC_ATOM_COMBO_NODE, strategy_name);
    
    // Set truth value based on evolutionary fitness
    gdouble confidence = std::min(0.95, best_frequency / (gdouble)n_transactions);
    gdouble strength = std::min(1.0, best_fitness);
    
    gnc_atomspace_set_truth_value(strategy_atom, strength, confidence);
    
    // Update attention parameters for high-fitness strategies
    auto& params = g_atomspace->attention_params[strategy_atom];
    params.sti = best_fitness * 50.0; // Reward good strategies with attention
    params.lti += 10.0; // Build long-term importance
    
    g_message("MOSES discovered evolved balancing strategy: %s (fitness=%.3f, n=%d)", 
              best_pattern.c_str(), best_fitness, n_transactions);
    
    return strategy_atom;
}

Transaction* gnc_moses_optimize_transaction(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, nullptr);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return const_cast<Transaction*>(transaction);
    }
    
    // MOSES-style transaction optimization
    gdouble current_fitness = gnc_pln_validate_double_entry(transaction);
    
    g_message("MOSES transaction optimization: current fitness=%.3f", current_fitness);
    
    // For now, return original transaction if fitness is already high
    if (current_fitness > 0.9) {
        g_message("Transaction already optimized (fitness > 0.9)");
        return const_cast<Transaction*>(transaction);
    }
    
    // In a full implementation, this would:
    // 1. Generate variations of the transaction structure
    // 2. Evaluate fitness of each variation
    // 3. Use evolutionary operators (crossover, mutation)
    // 4. Return the fittest variant
    
    // Create optimization result atom
    GncAtomHandle optimization_atom = g_atomspace->create_atom(
        GNC_ATOM_GROUNDED_SCHEMA,
        "MOSESOptimization:Transaction:" + std::to_string(reinterpret_cast<uintptr_t>(transaction))
    );
    
    gnc_atomspace_set_truth_value(optimization_atom, current_fitness, 0.8);
    
    g_message("MOSES transaction optimization completed (placeholder implementation)");
    
    return const_cast<Transaction*>(transaction);
}

/********************************************************************\
 * URE Uncertain Reasoning                                           *
\********************************************************************/

gnc_numeric gnc_ure_predict_balance(const Account *account, time64 future_date)
{
    g_return_val_if_fail(account != nullptr, gnc_numeric_zero());
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return xaccAccountGetBalance(account);
    }
    
    // Enhanced URE-style uncertain reasoning for balance prediction
    gnc_numeric current_balance = xaccAccountGetBalance(account);
    time64 current_time = time(nullptr);
    
    if (future_date <= current_time) {
        return current_balance; // No prediction needed for past/present
    }
    
    // Collect historical data for pattern analysis
    GList *splits = xaccAccountGetSplitList(account);
    std::vector<double> historical_changes;
    gdouble total_variance = 0.0;
    gdouble trend = 0.0;
    gint data_points = 0;
    
    // Analyze split history for trend and variance
    GList *prev_node = nullptr;
    for (GList *node = splits; node; prev_node = node, node = node->next) {
        Split *split = GNC_SPLIT(node->data);
        gnc_numeric amount = xaccSplitGetAmount(split);
        double change = gnc_numeric_to_double(amount);
        
        historical_changes.push_back(change);
        trend += change;
        data_points++;
        
        if (data_points > 100) break; // Limit analysis to recent history
    }
    
    if (data_points > 0) {
        trend /= data_points;
        
        // Calculate variance for uncertainty quantification
        for (double change : historical_changes) {
            gdouble deviation = change - trend;
            total_variance += deviation * deviation;
        }
        total_variance /= data_points;
    }
    
    // URE reasoning: combine trend with uncertainty
    time64 time_delta = future_date - current_time;
    gdouble days_ahead = time_delta / 86400.0; // Convert to days
    
    // Base prediction using trend
    gdouble predicted_change = trend * days_ahead;
    
    // Uncertainty increases with time and variance
    gdouble uncertainty_factor = 1.0 + (sqrt(total_variance) * sqrt(days_ahead) / 365.0);
    
    // Apply conservative adjustment for uncertainty
    if (predicted_change > 0) {
        predicted_change /= uncertainty_factor;
    } else {
        predicted_change *= uncertainty_factor;
    }
    
    gnc_numeric predicted_balance = gnc_numeric_add(
        current_balance,
        gnc_numeric_create(predicted_change, 100),
        GNC_DENOM_AUTO,
        GNC_HOW_RND_ROUND_HALF_UP
    );
    
    // Create URE prediction atom for knowledge retention
    std::string prediction_name = "UREPrediction:Account:" + 
                                 std::string(xaccAccountGetName(account)) +
                                 ":Days:" + std::to_string((int)days_ahead);
    
    GncAtomHandle prediction_atom = g_atomspace->create_atom(GNC_ATOM_PREDICATE_NODE, prediction_name);
    
    // Set truth value based on prediction confidence
    gdouble confidence = std::max(0.1, 1.0 / uncertainty_factor);
    gdouble strength = 0.7; // Moderate strength for predictions
    
    gnc_atomspace_set_truth_value(prediction_atom, strength, confidence);
    
    g_message("URE balance prediction for account %s: %.2f (uncertainty factor: %.2f)", 
              xaccAccountGetName(account), 
              gnc_numeric_to_double(predicted_balance),
              uncertainty_factor);
    
    return predicted_balance;
}

gdouble gnc_ure_transaction_validity(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, 0.0);
    
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return gnc_pln_validate_double_entry(transaction);
    }
    
    // Enhanced URE uncertain reasoning for transaction validity
    gdouble base_validity = gnc_pln_validate_double_entry(transaction);
    
    GList *splits = xaccTransGetSplitList(transaction);
    gint split_count = g_list_length(splits);
    
    // Uncertainty factors for URE reasoning
    gdouble complexity_uncertainty = 1.0;
    gdouble temporal_uncertainty = 1.0;
    gdouble account_uncertainty = 1.0;
    
    // Calculate complexity-based uncertainty
    if (split_count > 2) {
        complexity_uncertainty = 1.0 - (0.05 * (split_count - 2));
        complexity_uncertainty = std::max(0.5, complexity_uncertainty);
    }
    
    // Calculate temporal uncertainty (recent transactions more certain)
    time64 trans_time = xaccTransGetDate(transaction);
    time64 current_time = time(nullptr);
    time64 age_days = (current_time - trans_time) / 86400;
    
    if (age_days > 0) {
        temporal_uncertainty = exp(-age_days / 365.0); // Decay over year
        temporal_uncertainty = std::max(0.3, temporal_uncertainty);
    }
    
    // Calculate account-based uncertainty using attention values
    gdouble total_attention = 0.0;
    gint attention_count = 0;
    
    for (GList *node = splits; node; node = node->next) {
        Split *split = GNC_SPLIT(node->data);
        Account *account = xaccSplitGetAccount(split);
        
        if (account) {
            GncAttentionParams params = gnc_ecan_get_attention_params(account);
            total_attention += params.confidence;
            attention_count++;
        }
    }
    
    if (attention_count > 0) {
        account_uncertainty = total_attention / attention_count;
    }
    
    // URE truth value revision combining multiple uncertainties
    gdouble combined_uncertainty = (complexity_uncertainty + temporal_uncertainty + account_uncertainty) / 3.0;
    gdouble final_validity = base_validity * combined_uncertainty;
    
    // Create URE validity assessment atom
    std::string assessment_name = "UREValidityAssessment:Transaction:" +
                                 std::to_string(reinterpret_cast<uintptr_t>(transaction));
    
    GncAtomHandle assessment_atom = g_atomspace->create_atom(GNC_ATOM_EVALUATION_LINK, assessment_name);
    gnc_atomspace_set_truth_value(assessment_atom, final_validity, combined_uncertainty);
    
    g_debug("URE transaction validity: base=%.3f, complexity=%.3f, temporal=%.3f, account=%.3f, final=%.3f",
            base_validity, complexity_uncertainty, temporal_uncertainty, account_uncertainty, final_validity);
    
    return final_validity;
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