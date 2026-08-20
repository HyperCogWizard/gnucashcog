/********************************************************************\
 * test-cognitive-benchmark.cpp -- Large-book cognitive benchmarks *
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

#include <chrono>
#include <string>
#include <vector>

namespace {

static gint env_int(const char *name, gint fallback)
{
    const char *v = g_getenv(name);
    if (!v || !*v) return fallback;
    return static_cast<gint>(g_ascii_strtoll(v, nullptr, 10));
}

} // namespace

class CognitiveBenchmarkTest : public ::testing::Test
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
    }

    void TearDown() override
    {
        gnc_cognitive_accounting_shutdown();
        qof_book_destroy(book);
        qof_close();
    }

    Account *make_account(const char *name, GNCAccountType type)
    {
        Account *acc = xaccMallocAccount(book);
        xaccAccountBeginEdit(acc);
        xaccAccountSetName(acc, name);
        xaccAccountSetType(acc, type);
        xaccAccountSetCommodity(acc, currency);
        xaccAccountCommitEdit(acc);
        gnc_account_append_child(root, acc);
        return acc;
    }

    Transaction *make_tx(Account *debit, Account *credit, gint64 cents)
    {
        Transaction *tx = xaccMallocTransaction(book);
        xaccTransBeginEdit(tx);
        xaccTransSetCurrency(tx, currency);
        gnc_numeric amt = gnc_numeric_create(cents, 100);
        Split *s1 = xaccMallocSplit(book);
        xaccSplitSetAccount(s1, debit);
        xaccSplitSetValue(s1, amt);
        xaccSplitSetAmount(s1, amt);
        xaccSplitSetParent(s1, tx);
        Split *s2 = xaccMallocSplit(book);
        gnc_numeric neg = gnc_numeric_neg(amt);
        xaccSplitSetAccount(s2, credit);
        xaccSplitSetValue(s2, neg);
        xaccSplitSetAmount(s2, neg);
        xaccSplitSetParent(s2, tx);
        xaccTransCommitEdit(tx);
        return tx;
    }

    QofBook *book = nullptr;
    gnc_commodity *currency = nullptr;
    Account *root = nullptr;
};

TEST_F(CognitiveBenchmarkTest, LargeBookObserveValidateEcan)
{
    /* Defaults keep CI light; override with GNC_COG_BENCH_ACCOUNTS / _TXNS. */
    const gint n_accounts = std::max(10, env_int("GNC_COG_BENCH_ACCOUNTS", 80));
    const gint n_txns = std::max(20, env_int("GNC_COG_BENCH_TXNS", 400));
    const gint max_ms_observe = env_int("GNC_COG_BENCH_MAX_MS_OBSERVE", 15000);
    const gint max_ms_validate = env_int("GNC_COG_BENCH_MAX_MS_VALIDATE", 30000);

    std::vector<Account*> banks;
    std::vector<Account*> expenses;
    banks.reserve(static_cast<size_t>(n_accounts / 2));
    expenses.reserve(static_cast<size_t>(n_accounts / 2));

    for (gint i = 0; i < n_accounts; ++i) {
        gchar *name = g_strdup_printf("Acct-%d", i);
        if (i % 2 == 0)
            banks.push_back(make_account(name, ACCT_TYPE_BANK));
        else
            expenses.push_back(make_account(name, ACCT_TYPE_EXPENSE));
        g_free(name);
    }
    ASSERT_FALSE(banks.empty());
    ASSERT_FALSE(expenses.empty());

    std::vector<Transaction*> txs;
    txs.reserve(static_cast<size_t>(n_txns));
    for (gint i = 0; i < n_txns; ++i) {
        Account *b = banks[static_cast<size_t>(i) % banks.size()];
        Account *e = expenses[static_cast<size_t>(i) % expenses.size()];
        txs.push_back(make_tx(e, b, 1000 + (i % 50) * 17));
    }

    auto t0 = std::chrono::steady_clock::now();
    ASSERT_TRUE(gnc_cognitive_backend_sync_book(book));
    for (Transaction *tx : txs)
        ASSERT_TRUE(gnc_cognitive_backend_sync_transaction(tx));
    auto t1 = std::chrono::steady_clock::now();
    auto observe_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_message("benchmark observe: %ld ms for %d accounts + %d txns",
              static_cast<long>(observe_ms), n_accounts, n_txns);
    EXPECT_LT(observe_ms, max_ms_observe);

    t0 = std::chrono::steady_clock::now();
    gint ok = 0;
    for (Transaction *tx : txs) {
        gdouble score = gnc_pln_validate_double_entry(tx);
        EXPECT_GE(score, 0.0);
        EXPECT_LE(score, 1.0);
        if (gnc_cognitive_transaction_badge(tx) == GNC_COGNITIVE_BADGE_OK ||
            gnc_cognitive_transaction_badge(tx) == GNC_COGNITIVE_BADGE_WARN)
            ++ok;
        gnc_ecan_update_account_attention(xaccSplitGetAccount(xaccTransGetSplit(tx, 0)), tx);
    }
    gnc_ecan_decay_tick();
    Account *top[16] = {};
    gint ntop = gnc_ecan_top_accounts(top, 16);
    t1 = std::chrono::steady_clock::now();
    auto validate_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    g_message("benchmark validate+ecan: %ld ms, okish=%d/%d, top=%d",
              static_cast<long>(validate_ms), ok, n_txns, ntop);
    EXPECT_LT(validate_ms, max_ms_validate);
    EXPECT_GT(ok, n_txns / 2);
    EXPECT_GT(ntop, 0);

    GncCognitiveBackendStats st{};
    ASSERT_TRUE(gnc_cognitive_backend_get_stats(&st));
    EXPECT_GE(st.account_atoms, static_cast<guint64>(n_accounts));
    EXPECT_GE(st.transaction_atoms, static_cast<guint64>(n_txns));
    /* Atom count should grow roughly with accounts+txns (plus link atoms). */
    EXPECT_GE(st.atom_count, st.account_atoms + st.transaction_atoms);
    EXPECT_TRUE(gnc_cognitive_backend_health_check());

    /* HTML generators must stay bounded on large books */
    t0 = std::chrono::steady_clock::now();
    char *html = gnc_cognitive_validation_summary_html(book);
    t1 = std::chrono::steady_clock::now();
    auto html_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    ASSERT_NE(html, nullptr);
    g_free(html);
    g_message("benchmark validation html: %ld ms", static_cast<long>(html_ms));
    EXPECT_LT(html_ms, 10000);
}

TEST_F(CognitiveBenchmarkTest, RepeatedObserveIsStable)
{
    Account *bank = make_account("Bank", ACCT_TYPE_BANK);
    Account *exp = make_account("Exp", ACCT_TYPE_EXPENSE);
    for (int i = 0; i < 25; ++i)
        make_tx(exp, bank, 500 + i);

    ASSERT_TRUE(gnc_cognitive_backend_sync_book(book));
    GncCognitiveBackendStats a{}, b{};
    ASSERT_TRUE(gnc_cognitive_backend_get_stats(&a));
    ASSERT_TRUE(gnc_cognitive_backend_sync_book(book));
    ASSERT_TRUE(gnc_cognitive_backend_get_stats(&b));
    /* Re-observe should not explode atom counts unboundedly. */
    EXPECT_LE(b.atom_count, a.atom_count + 50);
    EXPECT_EQ(b.account_atoms, a.account_atoms);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
