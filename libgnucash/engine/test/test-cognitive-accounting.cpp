/********************************************************************\
 * test-cognitive-accounting.cpp -- Test cognitive accounting      *
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

#include <glib.h>
#include <gtest/gtest.h>
#include "gnc-cognitive-accounting.h"
#include "Account.h"
#include "Transaction.h"
#include "Split.h"
#include "qof.h"
#include "gnc-engine.h"

class CognitiveAccountingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize QOF and engine
        qof_init();
        qof_load_backend_shared_modules();
        
        // Initialize cognitive accounting
        gnc_cognitive_accounting_init();
        
        // Create test book
        book = qof_book_new();
        
        // Create test accounts
        root_account = gnc_account_create_root(book);
        
        checking_account = xaccMallocAccount(book);
        xaccAccountSetName(checking_account, "Checking");
        xaccAccountSetType(checking_account, ACCT_TYPE_BANK);
        gnc_account_append_child(root_account, checking_account);
        
        expense_account = xaccMallocAccount(book);
        xaccAccountSetName(expense_account, "Groceries");
        xaccAccountSetType(expense_account, ACCT_TYPE_EXPENSE);
        gnc_account_append_child(root_account, expense_account);
        
        income_account = xaccMallocAccount(book);
        xaccAccountSetName(income_account, "Salary");
        xaccAccountSetType(income_account, ACCT_TYPE_INCOME);
        gnc_account_append_child(root_account, income_account);
    }
    
    void TearDown() override
    {
        gnc_cognitive_accounting_shutdown();
        qof_book_destroy(book);
        qof_close();
    }
    
    QofBook *book;
    Account *root_account;
    Account *checking_account;
    Account *expense_account;
    Account *income_account;
};

TEST_F(CognitiveAccountingTest, InitializationTest)
{
    // Test that cognitive accounting can be initialized and shutdown
    gnc_cognitive_accounting_shutdown();
    EXPECT_TRUE(gnc_cognitive_accounting_init());
    gnc_cognitive_accounting_shutdown();
    
    // Re-initialize for other tests
    gnc_cognitive_accounting_init();
}

TEST_F(CognitiveAccountingTest, AtomSpaceAccountRepresentation)
{
    // Test converting accounts to AtomSpace representation
    GncAtomHandle checking_atom = gnc_account_to_atomspace(checking_account);
    EXPECT_NE(checking_atom, 0);
    
    GncAtomHandle expense_atom = gnc_account_to_atomspace(expense_account);
    EXPECT_NE(expense_atom, 0);
    
    // Atoms should be different
    EXPECT_NE(checking_atom, expense_atom);
    
    // Converting same account should return same atom
    GncAtomHandle checking_atom2 = gnc_account_to_atomspace(checking_account);
    EXPECT_EQ(checking_atom, checking_atom2);
}

TEST_F(CognitiveAccountingTest, AccountHierarchyLinks)
{
    // Create hierarchy links
    GncAtomHandle root_atom = gnc_account_to_atomspace(root_account);
    GncAtomHandle checking_atom = gnc_account_to_atomspace(checking_account);
    
    GncAtomHandle hierarchy_link = gnc_atomspace_create_hierarchy_link(
        root_atom, checking_atom);
    
    EXPECT_NE(hierarchy_link, 0);
}

TEST_F(CognitiveAccountingTest, PLNDoubleEntryValidation)
{
    // Create a balanced transaction
    Transaction *transaction = xaccMallocTransaction(book);
    xaccTransBeginEdit(transaction);
    
    // Split 1: Debit checking account $100
    Split *split1 = xaccMallocSplit(book);
    xaccSplitSetAccount(split1, checking_account);
    xaccSplitSetAmount(split1, gnc_numeric_create(10000, 100)); // $100.00
    xaccSplitSetParent(split1, transaction);
    
    // Split 2: Credit income account $100
    Split *split2 = xaccMallocSplit(book);
    xaccSplitSetAccount(split2, income_account);
    xaccSplitSetAmount(split2, gnc_numeric_create(-10000, 100)); // -$100.00
    xaccSplitSetParent(split2, transaction);
    
    xaccTransCommitEdit(transaction);
    
    // Test PLN validation
    gdouble confidence = gnc_pln_validate_double_entry(transaction);
    EXPECT_DOUBLE_EQ(confidence, 1.0); // Perfect balance should have confidence 1.0
}

TEST_F(CognitiveAccountingTest, PLNUnbalancedTransaction)
{
    // Create an unbalanced transaction
    Transaction *transaction = xaccMallocTransaction(book);
    xaccTransBeginEdit(transaction);
    
    // Split 1: Debit checking account $100
    Split *split1 = xaccMallocSplit(book);
    xaccSplitSetAccount(split1, checking_account);
    xaccSplitSetAmount(split1, gnc_numeric_create(10000, 100)); // $100.00
    xaccSplitSetParent(split1, transaction);
    
    // Split 2: Credit income account $90 (unbalanced!)
    Split *split2 = xaccMallocSplit(book);
    xaccSplitSetAccount(split2, income_account);
    xaccSplitSetAmount(split2, gnc_numeric_create(-9000, 100)); // -$90.00
    xaccSplitSetParent(split2, transaction);
    
    xaccTransCommitEdit(transaction);
    
    // Test PLN validation
    gdouble confidence = gnc_pln_validate_double_entry(transaction);
    EXPECT_LT(confidence, 1.0); // Unbalanced should have less than perfect confidence
}

TEST_F(CognitiveAccountingTest, PLNNEntryValidation)
{
    // Create a 3-party transaction
    Transaction *transaction = xaccMallocTransaction(book);
    xaccTransBeginEdit(transaction);
    
    // Split 1: Debit checking account $100
    Split *split1 = xaccMallocSplit(book);
    xaccSplitSetAccount(split1, checking_account);
    xaccSplitSetAmount(split1, gnc_numeric_create(10000, 100));
    xaccSplitSetParent(split1, transaction);
    
    // Split 2: Credit income account $60
    Split *split2 = xaccMallocSplit(book);
    xaccSplitSetAccount(split2, income_account);
    xaccSplitSetAmount(split2, gnc_numeric_create(-6000, 100));
    xaccSplitSetParent(split2, transaction);
    
    // Split 3: Credit expense account $40
    Split *split3 = xaccMallocSplit(book);
    xaccSplitSetAccount(split3, expense_account);
    xaccSplitSetAmount(split3, gnc_numeric_create(-4000, 100));
    xaccSplitSetParent(split3, transaction);
    
    xaccTransCommitEdit(transaction);
    
    // Test n-entry validation for 3 parties
    gdouble confidence = gnc_pln_validate_n_entry(transaction, 3);
    EXPECT_GT(confidence, 0.0);
    EXPECT_LE(confidence, 1.0);
}

TEST_F(CognitiveAccountingTest, TrialBalanceProof)
{
    // Generate trial balance proof
    GncAtomHandle proof_handle = gnc_pln_generate_trial_balance_proof(root_account);
    EXPECT_NE(proof_handle, 0);
}

TEST_F(CognitiveAccountingTest, PLProof)
{
    // Generate P&L proof
    GncAtomHandle proof_handle = gnc_pln_generate_pl_proof(income_account, expense_account);
    EXPECT_NE(proof_handle, 0);
}

TEST_F(CognitiveAccountingTest, ECANAttentionAllocation)
{
    // Create a transaction to trigger attention update
    Transaction *transaction = xaccMallocTransaction(book);
    xaccTransBeginEdit(transaction);
    
    Split *split1 = xaccMallocSplit(book);
    xaccSplitSetAccount(split1, checking_account);
    xaccSplitSetAmount(split1, gnc_numeric_create(5000, 100));
    xaccSplitSetParent(split1, transaction);
    
    Split *split2 = xaccMallocSplit(book);
    xaccSplitSetAccount(split2, expense_account);
    xaccSplitSetAmount(split2, gnc_numeric_create(-5000, 100));
    xaccSplitSetParent(split2, transaction);
    
    xaccTransCommitEdit(transaction);
    
    // Get initial attention parameters
    GncAttentionParams initial_params = gnc_ecan_get_attention_params(checking_account);
    
    // Update attention based on transaction
    gnc_ecan_update_account_attention(checking_account, transaction);
    
    // Get updated attention parameters
    GncAttentionParams updated_params = gnc_ecan_get_attention_params(checking_account);
    
    // Attention should have increased
    EXPECT_GE(updated_params.attention_value, initial_params.attention_value);
    EXPECT_GE(updated_params.importance, initial_params.importance);
}

TEST_F(CognitiveAccountingTest, AttentionAllocationAcrossAccounts)
{
    Account *accounts[] = {checking_account, expense_account, income_account};
    
    // Update attention for all accounts
    Transaction *transaction = xaccMallocTransaction(book);
    gnc_ecan_update_account_attention(checking_account, transaction);
    gnc_ecan_update_account_attention(expense_account, transaction);
    gnc_ecan_update_account_attention(income_account, transaction);
    
    // Allocate attention across accounts
    gnc_ecan_allocate_attention(accounts, 3);
    
    // Verify attention parameters are set
    for (int i = 0; i < 3; i++) {
        GncAttentionParams params = gnc_ecan_get_attention_params(accounts[i]);
        EXPECT_GE(params.attention_value, 0.0);
        EXPECT_LE(params.attention_value, 1.0);
    }
}

TEST_F(CognitiveAccountingTest, MOSESBalancingStrategies)
{
    // Create array of historical transactions
    Transaction *transaction1 = xaccMallocTransaction(book);
    Transaction *transaction2 = xaccMallocTransaction(book);
    Transaction *transactions[] = {transaction1, transaction2};
    
    // Discover balancing strategies
    GncAtomHandle strategy_handle = gnc_moses_discover_balancing_strategies(transactions, 2);
    EXPECT_NE(strategy_handle, 0);
}

TEST_F(CognitiveAccountingTest, MOSESTransactionOptimization)
{
    Transaction *transaction = xaccMallocTransaction(book);
    
    // Optimize transaction
    Transaction *optimized = gnc_moses_optimize_transaction(transaction);
    EXPECT_NE(optimized, nullptr);
}

TEST_F(CognitiveAccountingTest, UREBalancePrediction)
{
    // Predict future balance
    time64 future_date = time(nullptr) + 86400; // Tomorrow
    gnc_numeric predicted_balance = gnc_ure_predict_balance(checking_account, future_date);
    
    // Should return a valid numeric
    EXPECT_FALSE(gnc_numeric_error(predicted_balance));
}

TEST_F(CognitiveAccountingTest, URETransactionValidity)
{
    // Create balanced transaction
    Transaction *transaction = xaccMallocTransaction(book);
    xaccTransBeginEdit(transaction);
    
    Split *split1 = xaccMallocSplit(book);
    xaccSplitSetAccount(split1, checking_account);
    xaccSplitSetAmount(split1, gnc_numeric_create(5000, 100));
    xaccSplitSetParent(split1, transaction);
    
    Split *split2 = xaccMallocSplit(book);
    xaccSplitSetAccount(split2, expense_account);
    xaccSplitSetAmount(split2, gnc_numeric_create(-5000, 100));
    xaccSplitSetParent(split2, transaction);
    
    xaccTransCommitEdit(transaction);
    
    // Test URE validity assessment
    gdouble validity = gnc_ure_transaction_validity(transaction);
    EXPECT_GT(validity, 0.0);
    EXPECT_LE(validity, 1.0);
}

TEST_F(CognitiveAccountingTest, CognitiveAccountTypes)
{
    // Test setting and getting cognitive account types
    gnc_account_set_cognitive_type(checking_account, GNC_COGNITIVE_ACCT_ADAPTIVE);
    
    GncCognitiveAccountType type = gnc_account_get_cognitive_type(checking_account);
    EXPECT_EQ(type, GNC_COGNITIVE_ACCT_ADAPTIVE);
    
    // Test multiple flags
    GncCognitiveAccountType multi_type = static_cast<GncCognitiveAccountType>(
        GNC_COGNITIVE_ACCT_PREDICTIVE | GNC_COGNITIVE_ACCT_ATTENTION);
    gnc_account_set_cognitive_type(expense_account, multi_type);
    
    GncCognitiveAccountType retrieved_type = gnc_account_get_cognitive_type(expense_account);
    EXPECT_EQ(retrieved_type, multi_type);
}

TEST_F(CognitiveAccountingTest, CognitiveAccountDefaults)
{
    // New accounts should have traditional type by default
    Account *new_account = xaccMallocAccount(book);
    GncCognitiveAccountType default_type = gnc_account_get_cognitive_type(new_account);
    EXPECT_EQ(default_type, GNC_COGNITIVE_ACCT_TRADITIONAL);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    g_test_init(&argc, &argv, nullptr);
    
    return RUN_ALL_TESTS();
}