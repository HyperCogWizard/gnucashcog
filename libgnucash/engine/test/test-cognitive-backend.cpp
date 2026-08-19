/********************************************************************\
 * test-cognitive-backend.cpp -- CognitiveBackend adapter tests    *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

#include <config.h>
#include <glib.h>
#include <gtest/gtest.h>

#include "gnc-cognitive-accounting.h"
#include "gnc-cognitive-backend.h"
#include "Account.h"
#include "Transaction.h"
#include "Split.h"
#include "qof.h"
#include "cashobjects.h"
#include "gnc-commodity.h"

class CognitiveBackendTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        qof_init();
        ASSERT_TRUE(cashobjects_register());
        ASSERT_TRUE(gnc_cognitive_accounting_init());
        book = qof_book_new();
        gnc_commodity_table *table = gnc_commodity_table_get_table(book);
        gnc_commodity *created = gnc_commodity_new(book, "US Dollar", "CURRENCY", "USD", "0", 100);
        currency = gnc_commodity_table_insert(table, created);
        root = gnc_account_create_root(book);
        checking = xaccMallocAccount(book);
        xaccAccountBeginEdit(checking);
        xaccAccountSetName(checking, "Checking");
        xaccAccountSetType(checking, ACCT_TYPE_BANK);
        xaccAccountSetCommodity(checking, currency);
        xaccAccountCommitEdit(checking);
        gnc_account_append_child(root, checking);

        expense = xaccMallocAccount(book);
        xaccAccountBeginEdit(expense);
        xaccAccountSetName(expense, "Food");
        xaccAccountSetType(expense, ACCT_TYPE_EXPENSE);
        xaccAccountSetCommodity(expense, currency);
        xaccAccountCommitEdit(expense);
        gnc_account_append_child(root, expense);
    }

    void TearDown() override
    {
        gnc_cognitive_accounting_shutdown();
        qof_book_destroy(book);
        qof_close();
    }

    QofBook *book = nullptr;
    gnc_commodity *currency = nullptr;
    Account *root = nullptr;
    Account *checking = nullptr;
    Account *expense = nullptr;
};

TEST_F(CognitiveBackendTest, SimulatedAlwaysAvailable)
{
    EXPECT_TRUE(gnc_cognitive_backend_available(GNC_COGNITIVE_BACKEND_SIMULATED));
    EXPECT_TRUE(gnc_cognitive_backend_select(GNC_COGNITIVE_BACKEND_SIMULATED));
    EXPECT_EQ(gnc_cognitive_backend_current(), GNC_COGNITIVE_BACKEND_SIMULATED);
    EXPECT_STREQ(gnc_cognitive_backend_name(), "simulated");
}

TEST_F(CognitiveBackendTest, OpenCogSelectionRespectsAvailability)
{
    if (gnc_cognitive_backend_available(GNC_COGNITIVE_BACKEND_OPENCOG)) {
        EXPECT_TRUE(gnc_cognitive_backend_select(GNC_COGNITIVE_BACKEND_OPENCOG));
        EXPECT_STREQ(gnc_cognitive_backend_name(), "opencog");
        /* Restore default for later tests in this process */
        EXPECT_TRUE(gnc_cognitive_backend_select(GNC_COGNITIVE_BACKEND_SIMULATED));
    } else {
        EXPECT_FALSE(gnc_cognitive_backend_select(GNC_COGNITIVE_BACKEND_OPENCOG));
        EXPECT_EQ(gnc_cognitive_backend_current(), GNC_COGNITIVE_BACKEND_SIMULATED);
    }
}

TEST_F(CognitiveBackendTest, SyncBookAndStats)
{
    ASSERT_TRUE(gnc_cognitive_backend_sync_book(book));
    GncCognitiveBackendStats st{};
    ASSERT_TRUE(gnc_cognitive_backend_get_stats(&st));
    EXPECT_GE(st.account_atoms, 1u);
    EXPECT_GE(st.atom_count, st.account_atoms);
    EXPECT_TRUE(gnc_cognitive_backend_health_check());

    char *json = gnc_cognitive_backend_status_json();
    ASSERT_NE(json, nullptr);
    EXPECT_NE(strstr(json, "\"backend\""), nullptr);
    EXPECT_NE(strstr(json, "simulated"), nullptr);
    g_free(json);
}

TEST_F(CognitiveBackendTest, BadgesAndHeat)
{
    Transaction *tx = xaccMallocTransaction(book);
    xaccTransBeginEdit(tx);
    xaccTransSetCurrency(tx, currency);
    Split *s1 = xaccMallocSplit(book);
    xaccSplitSetAccount(s1, checking);
    xaccSplitSetValue(s1, gnc_numeric_create(-2500, 100));
    xaccSplitSetAmount(s1, gnc_numeric_create(-2500, 100));
    xaccSplitSetParent(s1, tx);
    Split *s2 = xaccMallocSplit(book);
    xaccSplitSetAccount(s2, expense);
    xaccSplitSetValue(s2, gnc_numeric_create(2500, 100));
    xaccSplitSetAmount(s2, gnc_numeric_create(2500, 100));
    xaccSplitSetParent(s2, tx);
    xaccTransCommitEdit(tx);

    ASSERT_TRUE(gnc_cognitive_backend_sync_transaction(tx));

    GncCognitiveBadge badge = gnc_cognitive_transaction_badge(tx);
    EXPECT_TRUE(badge == GNC_COGNITIVE_BADGE_OK || badge == GNC_COGNITIVE_BADGE_WARN);

    char *label = gnc_cognitive_transaction_badge_label(tx);
    ASSERT_NE(label, nullptr);
    EXPECT_GT(strlen(label), 0u);
    g_free(label);

    gnc_ecan_update_account_attention(checking, tx);
    gdouble heat = gnc_cognitive_account_attention_heat(checking);
    EXPECT_GE(heat, 0.0);
    EXPECT_LE(heat, 1.0);
    char *color = gnc_cognitive_account_attention_css_color(checking);
    ASSERT_NE(color, nullptr);
    EXPECT_EQ(color[0], '#');
    EXPECT_EQ(strlen(color), 7u);
    g_free(color);

    char *html = gnc_cognitive_html_summary_for_book(book);
    ASSERT_NE(html, nullptr);
    EXPECT_NE(strstr(html, "Backend"), nullptr);
    g_free(html);

    char *att = gnc_cognitive_attention_table_html(book, 5);
    ASSERT_NE(att, nullptr);
    EXPECT_NE(strstr(att, "<table"), nullptr);
    g_free(att);

    char *val = gnc_cognitive_validation_summary_html(book);
    ASSERT_NE(val, nullptr);
    EXPECT_NE(strstr(val, "badge"), nullptr);
    g_free(val);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
