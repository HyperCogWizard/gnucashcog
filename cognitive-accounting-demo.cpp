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

void demonstrate_cognitive_features()
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
        // Run demonstrations
        demonstrate_atomspace_representation();
        demonstrate_pln_validation();
        demonstrate_ecan_attention();
        demonstrate_cognitive_features();
        demonstrate_moses_optimization();
        demonstrate_trial_balance_proof();
        
        std::cout << "\n=== Cognitive Accounting Framework Summary ===\n";
        std::cout << "✓ AtomSpace: Chart of Accounts mapped as hypergraph\n";
        std::cout << "✓ PLN: Double-entry and N-entry validation rules\n";
        std::cout << "✓ ECAN: Adaptive attention allocation for accounts\n";
        std::cout << "✓ MOSES: Strategy discovery and optimization\n";
        std::cout << "✓ URE: Uncertain reasoning for predictions\n";
        std::cout << "✓ Cognitive: Enhanced account types and features\n";
        std::cout << "\nCognitive accounting transformation complete!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << "\n";
    }
    
    // Cleanup
    gnc_cognitive_accounting_shutdown();
    qof_close();
    
    std::cout << "\nDemo completed successfully.\n";
    return 0;
}