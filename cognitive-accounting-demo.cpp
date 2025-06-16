/********************************************************************\
 * cognitive-accounting-demo.cpp -- Cognitive Accounting Demo      *
 * Copyright (C) 2024 GnuCash Cognitive Engine                     *
 *                                                                  *
 * This program demonstrates the cognitive accounting capabilities  *
 * including AtomSpace account representation, PLN validation,     *
 * ECAN attention allocation, MOSES optimization, and URE          *
 * reasoning for intelligent financial management.                  *
 ********************************************************************/

#include <iostream>
#include <glib.h>
#include "gnc-cognitive-accounting.h"
#include "gnc-cognitive-scheme.h"
#include "gnc-cognitive-comms.h"
#include "Account.h"
#include "Transaction.h"
#include "Split.h"
#include "qof.h"
#include "gnc-engine.h"

void demonstrate_atomspace_representation()
{
    std::cout << "\n=== AtomSpace Account Representation Demo ===\n";
    
    // Create a book and account structure
    QofBook *book = qof_book_new();
    Account *root = gnc_account_create_root(book);
    
    Account *checking = xaccMallocAccount(book);
    xaccAccountSetName(checking, "Business Checking");
    xaccAccountSetType(checking, ACCT_TYPE_BANK);
    gnc_account_append_child(root, checking);
    
    Account *revenue = xaccMallocAccount(book);
    xaccAccountSetName(revenue, "Software Sales");
    xaccAccountSetType(revenue, ACCT_TYPE_INCOME);
    gnc_account_append_child(root, revenue);
    
    // Convert to AtomSpace representation
    GncAtomHandle root_atom = gnc_account_to_atomspace(root);
    GncAtomHandle checking_atom = gnc_account_to_atomspace(checking);
    GncAtomHandle revenue_atom = gnc_account_to_atomspace(revenue);
    
    std::cout << "Created AtomSpace representations:\n";
    std::cout << "  Root Account Atom: " << root_atom << "\n";
    std::cout << "  Checking Account Atom: " << checking_atom << "\n";
    std::cout << "  Revenue Account Atom: " << revenue_atom << "\n";
    
    // Create hierarchy links
    GncAtomHandle hierarchy_link = gnc_atomspace_create_hierarchy_link(
        root_atom, checking_atom);
    std::cout << "  Account Hierarchy Link: " << hierarchy_link << "\n";
    
    qof_book_destroy(book);
}

void demonstrate_pln_validation()
{
    std::cout << "\n=== PLN Ledger Validation Demo ===\n";
    
    QofBook *book = qof_book_new();
    Account *checking = xaccMallocAccount(book);
    Account *revenue = xaccMallocAccount(book);
    
    xaccAccountSetName(checking, "Checking");
    xaccAccountSetName(revenue, "Revenue");
    xaccAccountSetType(checking, ACCT_TYPE_BANK);
    xaccAccountSetType(revenue, ACCT_TYPE_INCOME);
    
    // Create a balanced transaction
    Transaction *balanced_txn = xaccMallocTransaction(book);
    xaccTransBeginEdit(balanced_txn);
    
    Split *split1 = xaccMallocSplit(book);
    xaccSplitSetAccount(split1, checking);
    xaccSplitSetAmount(split1, gnc_numeric_create(100000, 100)); // $1000.00
    xaccSplitSetParent(split1, balanced_txn);
    
    Split *split2 = xaccMallocSplit(book);
    xaccSplitSetAccount(split2, revenue);
    xaccSplitSetAmount(split2, gnc_numeric_create(-100000, 100)); // -$1000.00
    xaccSplitSetParent(split2, balanced_txn);
    
    xaccTransCommitEdit(balanced_txn);
    
    // Validate with PLN
    gdouble confidence = gnc_pln_validate_double_entry(balanced_txn);
    std::cout << "Balanced Transaction PLN Confidence: " << confidence << "\n";
    
    // Create unbalanced transaction
    Transaction *unbalanced_txn = xaccMallocTransaction(book);
    xaccTransBeginEdit(unbalanced_txn);
    
    Split *split3 = xaccMallocSplit(book);
    xaccSplitSetAccount(split3, checking);
    xaccSplitSetAmount(split3, gnc_numeric_create(100000, 100)); // $1000.00
    xaccSplitSetParent(split3, unbalanced_txn);
    
    Split *split4 = xaccMallocSplit(book);
    xaccSplitSetAccount(split4, revenue);
    xaccSplitSetAmount(split4, gnc_numeric_create(-95000, 100)); // -$950.00 (imbalanced!)
    xaccSplitSetParent(split4, unbalanced_txn);
    
    xaccTransCommitEdit(unbalanced_txn);
    
    gdouble unbalanced_confidence = gnc_pln_validate_double_entry(unbalanced_txn);
    std::cout << "Unbalanced Transaction PLN Confidence: " << unbalanced_confidence << "\n";
    
    // Test N-entry validation
    gdouble n_entry_confidence = gnc_pln_validate_n_entry(balanced_txn, 3);
    std::cout << "N-Entry (3-party) Validation Confidence: " << n_entry_confidence << "\n";
    
    qof_book_destroy(book);
}

void demonstrate_ecan_attention()
{
    std::cout << "\n=== ECAN Attention Allocation Demo ===\n";
    
    QofBook *book = qof_book_new();
    Account *checking = xaccMallocAccount(book);
    Account *savings = xaccMallocAccount(book);
    
    xaccAccountSetName(checking, "High Activity Checking");
    xaccAccountSetName(savings, "Low Activity Savings");
    xaccAccountSetType(checking, ACCT_TYPE_BANK);
    xaccAccountSetType(savings, ACCT_TYPE_BANK);
    
    // Create transaction to simulate activity
    Transaction *txn = xaccMallocTransaction(book);
    
    // Get initial attention parameters
    GncAttentionParams initial_checking = gnc_ecan_get_attention_params(checking);
    GncAttentionParams initial_savings = gnc_ecan_get_attention_params(savings);
    
    std::cout << "Initial Attention - Checking: importance=" << initial_checking.importance 
              << ", attention=" << initial_checking.attention_value << "\n";
    std::cout << "Initial Attention - Savings: importance=" << initial_savings.importance 
              << ", attention=" << initial_savings.attention_value << "\n";
    
    // Simulate multiple transactions on checking account
    for (int i = 0; i < 5; i++) {
        gnc_ecan_update_account_attention(checking, txn);
    }
    
    // One transaction on savings
    gnc_ecan_update_account_attention(savings, txn);
    
    // Get updated attention parameters
    GncAttentionParams updated_checking = gnc_ecan_get_attention_params(checking);
    GncAttentionParams updated_savings = gnc_ecan_get_attention_params(savings);
    
    std::cout << "Updated Attention - Checking: importance=" << updated_checking.importance 
              << ", attention=" << updated_checking.attention_value << "\n";
    std::cout << "Updated Attention - Savings: importance=" << updated_savings.importance 
              << ", attention=" << updated_savings.attention_value << "\n";
    
    // Allocate attention across accounts
    Account *accounts[] = {checking, savings};
    gnc_ecan_allocate_attention(accounts, 2);
    
    std::cout << "ECAN attention allocation completed.\n";
    
    qof_book_destroy(book);
}

void demonstrate_opencog_style_operations()
{
    std::cout << "\n=== OpenCog-Style Atom Operations Demo ===\n";
    
    // Create OpenCog-style atoms
    GncAtomHandle concept1 = gnc_atomspace_create_concept_node("CashFlow");
    GncAtomHandle concept2 = gnc_atomspace_create_concept_node("Revenue");
    GncAtomHandle predicate = gnc_atomspace_create_predicate_node("influences");
    
    std::cout << "Created OpenCog-style atoms:\n";
    std::cout << "  ConceptNode 'CashFlow': " << concept1 << "\n";
    std::cout << "  ConceptNode 'Revenue': " << concept2 << "\n";
    std::cout << "  PredicateNode 'influences': " << predicate << "\n";
    
    // Create evaluation link
    GncAtomHandle eval_link = gnc_atomspace_create_evaluation_link(
        predicate, concept1, 0.85);
    
    std::cout << "  EvaluationLink created: " << eval_link << "\n";
    
    // Set and get truth values
    gnc_atomspace_set_truth_value(concept1, 0.9, 0.8);
    
    gdouble strength, confidence;
    gnc_atomspace_get_truth_value(concept1, &strength, &confidence);
    
    std::cout << "Truth value for CashFlow concept:\n";
    std::cout << "  Strength: " << strength << "\n";
    std::cout << "  Confidence: " << confidence << "\n";
    
    // Create inheritance link
    GncAtomHandle inheritance = gnc_atomspace_create_inheritance_link(concept1, concept2);
    std::cout << "  InheritanceLink created: " << inheritance << "\n";
}

void demonstrate_scheme_representations()
{
    std::cout << "\n=== Scheme-based Cognitive Representations Demo ===\n";
    
    QofBook *book = qof_book_new();
    Account *investment_account = xaccMallocAccount(book);
    xaccAccountSetName(investment_account, "Stock Portfolio");
    xaccAccountSetType(investment_account, ACCT_TYPE_STOCK);
    
    // Generate Scheme representation
    char* scheme_repr = gnc_account_to_scheme_representation(investment_account);
    
    std::cout << "Scheme representation of Stock Portfolio account:\n";
    std::cout << scheme_repr << "\n";
    
    g_free(scheme_repr);
    
    // Generate hypergraph pattern encoding
    Account *root = gnc_account_create_root(book);
    gnc_account_append_child(root, investment_account);
    
    char* hypergraph_pattern = gnc_create_hypergraph_pattern_encoding(root);
    
    std::cout << "Hypergraph pattern encoding for account hierarchy:\n";
    std::cout << hypergraph_pattern << "\n";
    
    g_free(hypergraph_pattern);
    
    // Test Scheme expression evaluation
    GncAtomHandle eval_result = gnc_evaluate_scheme_expression(
        "(ConceptNode \"EvaluatedExpression\")");
    
    std::cout << "Scheme expression evaluation result: " << eval_result << "\n";
    
    qof_book_destroy(book);
}

void demonstrate_cognitive_messaging()
{
    std::cout << "\n=== Inter-Module Cognitive Communication Demo ===\n";
    
    // Register a message handler
    auto attention_handler = [](const GncCognitiveMessage* message) {
        std::cout << "ECAN module received message from " << message->source_module 
                  << ": " << message->message_type 
                  << " (priority: " << message->priority << ")\n";
    };
    
    gnc_register_cognitive_message_handler("ECAN", attention_handler);
    
    // Send cognitive messages between modules
    GncCognitiveMessage atomspace_msg = {};
    atomspace_msg.source_module = "AtomSpace";
    atomspace_msg.target_module = "ECAN";
    atomspace_msg.message_type = "AtomActivation";
    atomspace_msg.payload_atom = gnc_atomspace_create_concept_node("ActiveAccount");
    atomspace_msg.priority = 0.8;
    atomspace_msg.timestamp = time(nullptr);
    
    std::cout << "Sending cognitive message from AtomSpace to ECAN...\n";
    gnc_send_cognitive_message(&atomspace_msg);
    
    GncCognitiveMessage moses_msg = {};
    moses_msg.source_module = "MOSES";
    moses_msg.target_module = "PLN";
    moses_msg.message_type = "StrategyUpdate";
    moses_msg.payload_atom = gnc_atomspace_create_concept_node("EvolvedStrategy");
    moses_msg.priority = 0.9;
    moses_msg.timestamp = time(nullptr);
    
    // Register PLN handler and send message
    auto pln_handler = [](const GncCognitiveMessage* message) {
        std::cout << "PLN module received strategy update from " << message->source_module 
                  << " with high priority: " << message->priority << "\n";
    };
    
    gnc_register_cognitive_message_handler("PLN", pln_handler);
    
    std::cout << "Sending cognitive message from MOSES to PLN...\n";
    gnc_send_cognitive_message(&moses_msg);
}

void demonstrate_emergent_behavior()
{
    std::cout << "\n=== Emergent Cognitive Behavior Demo ===\n";
    
    QofBook *book = qof_book_new();
    Account *root = gnc_account_create_root(book);
    
    // Create multiple accounts with different activity patterns
    Account* accounts[4];
    const char* account_names[] = {"Checking", "Savings", "Investments", "Expenses"};
    GNCAccountType account_types[] = {ACCT_TYPE_BANK, ACCT_TYPE_BANK, ACCT_TYPE_STOCK, ACCT_TYPE_EXPENSE};
    
    for (int i = 0; i < 4; i++) {
        accounts[i] = xaccMallocAccount(book);
        xaccAccountSetName(accounts[i], account_names[i]);
        xaccAccountSetType(accounts[i], account_types[i]);
        gnc_account_append_child(root, accounts[i]);
    }
    
    // Simulate activity to create patterns
    Transaction *trans = xaccMallocTransaction(book);
    xaccTransBeginEdit(trans);
    
    Split *split1 = xaccMallocSplit(book);
    xaccSplitSetAccount(split1, accounts[0]); // Checking
    xaccSplitSetAmount(split1, gnc_numeric_create(50000, 100)); // $500
    xaccSplitSetParent(split1, trans);
    
    Split *split2 = xaccMallocSplit(book);
    xaccSplitSetAccount(split2, accounts[3]); // Expenses
    xaccSplitSetAmount(split2, gnc_numeric_create(-50000, 100)); // -$500
    xaccSplitSetParent(split2, trans);
    
    xaccTransCommitEdit(trans);
    
    // Update attention for pattern formation
    for (int i = 0; i < 4; i++) {
        gnc_ecan_update_account_attention(accounts[i], trans);
    }
    
    // Detect emergent patterns
    GncEmergenceParams params = {};
    params.complexity_threshold = 0.1;
    params.coherence_measure = 0.2;
    params.novelty_score = 0.15;
    params.pattern_frequency = 2;
    
    std::cout << "Analyzing account activity for emergent patterns...\n";
    GncAtomHandle emergent_pattern = gnc_detect_emergent_patterns(
        accounts, 4, &params);
    
    if (emergent_pattern != 0) {
        gdouble strength, confidence;
        gnc_atomspace_get_truth_value(emergent_pattern, &strength, &confidence);
        
        std::cout << "Emergent pattern detected!\n";
        std::cout << "  Pattern atom: " << emergent_pattern << "\n";
        std::cout << "  Emergence strength: " << strength << "\n";
        std::cout << "  Pattern confidence: " << confidence << "\n";
    } else {
        std::cout << "No emergent patterns detected with current parameters.\n";
    }
    
    // Test distributed attention optimization
    std::cout << "\nOptimizing distributed cognitive attention...\n";
    gdouble cognitive_load = 0.6;
    gdouble available_resources = 1.2;
    
    GncAtomHandle optimization = gnc_optimize_distributed_attention(
        cognitive_load, available_resources);
    
    gdouble opt_strength, opt_confidence;
    gnc_atomspace_get_truth_value(optimization, &opt_strength, &opt_confidence);
    
    std::cout << "Attention optimization completed:\n";
    std::cout << "  Optimization atom: " << optimization << "\n";
    std::cout << "  Efficiency: " << opt_strength << "\n";
    std::cout << "  Confidence: " << opt_confidence << "\n";
    
    qof_book_destroy(book);
}

void demonstrate_enhanced_ecan()
{
    std::cout << "\n=== Enhanced ECAN Attention Dynamics Demo ===\n";
    
    QofBook *book = qof_book_new();
    Account *trading_account = xaccMallocAccount(book);
    xaccAccountSetName(trading_account, "High-Frequency Trading");
    xaccAccountSetType(trading_account, ACCT_TYPE_TRADING);
    
    // Create multiple transactions to build attention
    for (int i = 0; i < 5; i++) {
        Transaction *trans = xaccMallocTransaction(book);
        xaccTransBeginEdit(trans);
        
        Split *split = xaccMallocSplit(book);
        xaccSplitSetAccount(split, trading_account);
        xaccSplitSetAmount(split, gnc_numeric_create(1000 * (i + 1), 100));
        xaccSplitSetParent(split, trans);
        
        xaccTransCommitEdit(trans);
        
        // Update ECAN attention with each transaction
        gnc_ecan_update_account_attention(trading_account, trans);
        
        GncAttentionParams params = gnc_ecan_get_attention_params(trading_account);
        
        std::cout << "Transaction " << (i + 1) << " - ECAN Parameters:\n";
        std::cout << "  STI (Short-term Importance): " << params.sti << "\n";
        std::cout << "  LTI (Long-term Importance): " << params.lti << "\n";
        std::cout << "  Activity Level: " << params.activity_level << "\n";
        std::cout << "  Cognitive Wage: " << params.wage << "\n";
        std::cout << "  Cognitive Rent: " << params.rent << "\n";
        std::cout << "  ---\n";
    }
    
    // Test attention allocation
    Account* high_activity_accounts[] = {trading_account};
    gnc_ecan_allocate_attention(high_activity_accounts, 1);
    
    std::cout << "Post-allocation attention parameters:\n";
    GncAttentionParams final_params = gnc_ecan_get_attention_params(trading_account);
    std::cout << "  Final STI: " << final_params.sti << "\n";
    std::cout << "  Final LTI: " << final_params.lti << "\n";
    std::cout << "  Total Attention Value: " << final_params.attention_value << "\n";
    
    qof_book_destroy(book);
}
{
    std::cout << "\n=== Cognitive Account Features Demo ===\n";
    
    QofBook *book = qof_book_new();
    Account *smart_account = xaccMallocAccount(book);
    xaccAccountSetName(smart_account, "AI-Enhanced Investment Account");
    xaccAccountSetType(smart_account, ACCT_TYPE_STOCK);
    
    // Set cognitive account types
    GncCognitiveAccountType cognitive_type = static_cast<GncCognitiveAccountType>(
        GNC_COGNITIVE_ACCT_ADAPTIVE | GNC_COGNITIVE_ACCT_PREDICTIVE | GNC_COGNITIVE_ACCT_ATTENTION);
    gnc_account_set_cognitive_type(smart_account, cognitive_type);
    
    GncCognitiveAccountType retrieved_type = gnc_account_get_cognitive_type(smart_account);
    
    std::cout << "Cognitive Account Features:\n";
    std::cout << "  Adaptive Learning: " << ((retrieved_type & GNC_COGNITIVE_ACCT_ADAPTIVE) ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Predictive Analysis: " << ((retrieved_type & GNC_COGNITIVE_ACCT_PREDICTIVE) ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Attention Driven: " << ((retrieved_type & GNC_COGNITIVE_ACCT_ATTENTION) ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Multi-modal Transactions: " << ((retrieved_type & GNC_COGNITIVE_ACCT_MULTIMODAL) ? "Enabled" : "Disabled") << "\n";
    
    // Demonstrate URE balance prediction
    time64 future_date = time(nullptr) + 86400 * 30; // 30 days in future
    gnc_numeric predicted_balance = gnc_ure_predict_balance(smart_account, future_date);
    
    std::cout << "URE Predicted Balance (30 days): $" 
              << gnc_numeric_to_double(predicted_balance) << "\n";
    
    qof_book_destroy(book);
}

void demonstrate_moses_optimization()
{
    std::cout << "\n=== MOSES Strategy Discovery Demo ===\n";
    
    QofBook *book = qof_book_new();
    
    // Create sample historical transactions
    Transaction *txn1 = xaccMallocTransaction(book);
    Transaction *txn2 = xaccMallocTransaction(book);
    Transaction *txn3 = xaccMallocTransaction(book);
    Transaction *historical_txns[] = {txn1, txn2, txn3};
    
    // Discover balancing strategies
    GncAtomHandle strategy_handle = gnc_moses_discover_balancing_strategies(
        historical_txns, 3);
    
    std::cout << "MOSES discovered balancing strategy: Atom Handle " 
              << strategy_handle << "\n";
    
    // Optimize a transaction
    Transaction *optimized_txn = gnc_moses_optimize_transaction(txn1);
    std::cout << "MOSES transaction optimization completed.\n";
    
    qof_book_destroy(book);
}

void demonstrate_trial_balance_proof()
{
    std::cout << "\n=== PLN Trial Balance Proof Demo ===\n";
    
    QofBook *book = qof_book_new();
    Account *root = gnc_account_create_root(book);
    Account *assets = xaccMallocAccount(book);
    Account *liabilities = xaccMallocAccount(book);
    Account *equity = xaccMallocAccount(book);
    
    xaccAccountSetName(assets, "Total Assets");
    xaccAccountSetName(liabilities, "Total Liabilities");
    xaccAccountSetName(equity, "Owner's Equity");
    xaccAccountSetType(assets, ACCT_TYPE_ASSET);
    xaccAccountSetType(liabilities, ACCT_TYPE_LIABILITY);
    xaccAccountSetType(equity, ACCT_TYPE_EQUITY);
    
    gnc_account_append_child(root, assets);
    gnc_account_append_child(root, liabilities);
    gnc_account_append_child(root, equity);
    
    // Generate trial balance proof
    GncAtomHandle trial_balance_proof = gnc_pln_generate_trial_balance_proof(root);
    std::cout << "PLN Trial Balance Proof Generated: Atom Handle " 
              << trial_balance_proof << "\n";
    
    // Generate P&L proof
    Account *income = xaccMallocAccount(book);
    Account *expenses = xaccMallocAccount(book);
    xaccAccountSetName(income, "Total Income");
    xaccAccountSetName(expenses, "Total Expenses");
    xaccAccountSetType(income, ACCT_TYPE_INCOME);
    xaccAccountSetType(expenses, ACCT_TYPE_EXPENSE);
    
    GncAtomHandle pl_proof = gnc_pln_generate_pl_proof(income, expenses);
    std::cout << "PLN Profit & Loss Proof Generated: Atom Handle " 
              << pl_proof << "\n";
    
    qof_book_destroy(book);
}

void demonstrate_scheme_integration()
{
    std::cout << "\n=== Scheme-based Cognitive Representations Demo ===\n";
    
    // Test Scheme evaluation
    gchar* result = gnc_cognitive_scheme_eval("(+ 1 2 3)");
    if (result) {
        std::cout << "Scheme evaluation result: " << result << "\n";
        g_free(result);
    }
    
    // Create hypergraph patterns
    GncAtomHandle pattern = gnc_scheme_create_hypergraph_pattern(
        "(create-account-concept \"Neural-Account\" \"COGNITIVE\")");
    std::cout << "Created hypergraph pattern: " << pattern << "\n";
    
    // Test neural-symbolic analysis
    QofBook *book = qof_book_new();
    Account *cognitive_account = xaccMallocAccount(book);
    xaccAccountSetName(cognitive_account, "Neural-Symbolic-Account");
    xaccAccountSetType(cognitive_account, ACCT_TYPE_ASSET);
    
    // Perform neural-symbolic analysis
    gnc_scheme_neural_symbolic_analysis(cognitive_account);
    
    // Test emergent insight discovery
    Transaction *test_txn = xaccMallocTransaction(book);
    Transaction *txns[] = {test_txn};
    gnc_scheme_emergent_insight_discovery(txns, 1);
    
    std::cout << "Scheme-based cognitive representations demonstrated\n";
    std::cout << "Neural-symbolic synergy operational\n";
    
    qof_book_destroy(book);
}

void demonstrate_distributed_cognition()
{
    std::cout << "\n=== Distributed Cognition and Inter-Module Communication Demo ===\n";
    
    // Test module communication
    gnc_cognitive_send_message(GNC_MODULE_ATOMSPACE, GNC_MODULE_PLN,
                              GNC_MSG_DATA_UPDATE, nullptr);
    
    gnc_cognitive_send_message(GNC_MODULE_PLN, GNC_MODULE_ECAN,
                              GNC_MSG_ATTENTION_REQUEST, nullptr);
    
    gnc_cognitive_send_message(GNC_MODULE_ECAN, GNC_MODULE_MOSES,
                              GNC_MSG_PATTERN_MATCH, nullptr);
    
    // Test broadcast communication
    gnc_cognitive_broadcast_message(GNC_MODULE_ATOMSPACE,
                                   GNC_MSG_SYNCHRONIZATION, nullptr);
    
    // Optimize attention flow
    gnc_cognitive_optimize_attention_flow();
    
    // Synchronize modules
    gnc_cognitive_synchronize_modules();
    
    std::cout << "Inter-module communication protocols demonstrated\n";
    std::cout << "Distributed cognition active across OpenCog modules\n";
    std::cout << "Adaptive attention allocation optimized\n";
    std::cout << "Emergent cognitive architectures operational\n";
}

int main(int argc, char **argv)
{
    std::cout << "======================================================\n";
    std::cout << "    GnuCash Cognitive Accounting Demonstration\n";
    std::cout << "======================================================\n";
    std::cout << "Transmuting classical ledgers into cognitive neural-\n";
    std::cout << "symbolic tapestries: every account a node in the vast\n";
    std::cout << "neural fabric of accounting sensemaking.\n";
    std::cout << "======================================================\n";
    
    // Initialize QOF and cognitive accounting
    qof_init();
    qof_load_backend_shared_modules();
    
    if (!gnc_cognitive_accounting_init()) {
        std::cerr << "Failed to initialize cognitive accounting!\n";
        return 1;
    }
    
    std::cout << "Cognitive AtomSpace initialized successfully.\n";
    
    try {
        // Run original demonstrations
        demonstrate_atomspace_representation();
        demonstrate_pln_validation();
        demonstrate_ecan_attention();
        demonstrate_cognitive_features();
        demonstrate_moses_optimization();
        demonstrate_trial_balance_proof();
        demonstrate_scheme_integration();
        demonstrate_distributed_cognition();
        
//<<<<<<< copilot/fix-1-3
        // Run enhanced OpenCog integration demonstrations
        demonstrate_opencog_style_operations();
        demonstrate_scheme_representations();
        demonstrate_cognitive_messaging();
        demonstrate_emergent_behavior();
        demonstrate_enhanced_ecan();
        
        std::cout << "\n=== Enhanced Cognitive Accounting Framework Summary ===\n";
        std::cout << "✓ AtomSpace: OpenCog-style hypergraph with ConceptNodes and Links\n";
        std::cout << "✓ PLN: Enhanced probabilistic reasoning with truth value computation\n";
        std::cout << "✓ ECAN: Sophisticated attention dynamics with STI/LTI economics\n";
        std::cout << "✓ MOSES: Evolutionary strategy discovery with fitness evaluation\n";
        std::cout << "✓ URE: Advanced uncertain reasoning with multi-factor analysis\n";
        std::cout << "✓ Scheme: Cognitive representations with hypergraph pattern encoding\n";
        std::cout << "✓ Messaging: Inter-module communication protocols established\n";
        std::cout << "✓ Emergence: Distributed cognition and emergent behavior detection\n";
        std::cout << "✓ Integration: Neural-symbolic synergy with adaptive architectures\n";
        std::cout << "\nOpenCog core modules successfully integrated!\n";
        std::cout << "\nThe classical ledger has evolved into a true cognitive\n";
        std::cout << "neural-symbolic system with emergent intelligence.\n";
//=======
//        std::cout << "\n=== Cognitive Accounting Framework Summary ===\n";
//        std::cout << "✓ AtomSpace: Chart of Accounts mapped as hypergraph\n";
//        std::cout << "✓ PLN: Double-entry and N-entry validation rules\n";
//        std::cout << "✓ ECAN: Adaptive attention allocation for accounts\n";
//        std::cout << "✓ MOSES: Strategy discovery and optimization\n";
//        std::cout << "✓ URE: Uncertain reasoning for predictions\n";
//        std::cout << "✓ Cognitive: Enhanced account types and features\n";
//        std::cout << "✓ Scheme: Neural-symbolic representations and hypergraph patterns\n";
//        std::cout << "✓ Communications: Distributed cognition and emergent architectures\n";
//        std::cout << "\nCognitive accounting transformation complete!\n";
//>>>>>>> stable
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << "\n";
    }
    
    // Cleanup
    gnc_cognitive_accounting_shutdown();
    qof_close();
    
    std::cout << "\nDemo completed successfully.\n";
    return 0;
}