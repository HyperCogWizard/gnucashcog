/********************************************************************\
 * gnc-cognitive-accounting.cpp -- Simulated cognitive accounting   *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

#include <config.h>

#include "gnc-cognitive-accounting.h"
#include "gnc-cognitive-scheme.h"
#include "gnc-cognitive-comms.h"
#include "gnc-cognitive-backend.h"
#include "gnc-tensor-network.h"
#include "Account.h"
#include "Split.h"
#include "Transaction.h"
#include "gnc-numeric.h"
#include "qof.h"
#include "qofevent.h"
#include "qofinstance-p.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/* ------------------------------------------------------------------ */
/* In-process AtomSpace model                                         */
/* ------------------------------------------------------------------ */

struct GncCognitiveAtom {
    GncAtomHandle handle = 0;
    GncAtomType type = GNC_ATOM_CONCEPT_NODE;
    std::string name;
    std::vector<GncAtomHandle> outgoing;
    std::vector<GncAtomHandle> incoming;
    GncAttentionParams attention{};
    gdouble tv_strength = 0.5;
    gdouble tv_confidence = 0.5;
};

struct GncCognitiveAtomSpace {
    std::unordered_map<GncAtomHandle, GncCognitiveAtom> atoms;
    std::map<const Account*, GncAtomHandle> account_atoms;
    std::map<const Transaction*, GncAtomHandle> transaction_atoms;
    std::map<const Transaction*, gdouble> last_validation;
    std::map<std::string, GncCognitiveMessageHandler> message_handlers;
    std::vector<GncCognitiveAtomMessage> message_queue;
    std::string last_moses_json = "[]";
    GncAtomHandle next_handle = 1000;
    gdouble total_sti_funds = 1000.0;
    gdouble total_lti_funds = 1000.0;
    gdouble attention_decay_rate = 0.01;
    gint event_handler_id = 0;
    gboolean auto_enabled = FALSE;

    GncCognitiveAtomSpace()
    {
        const char *env = g_getenv("GNC_COGNITIVE_AUTO");
        if (env && env[0] == '1')
            auto_enabled = TRUE;
    }

    GncAtomHandle create_atom(GncAtomType type, const std::string& name,
                              const std::vector<GncAtomHandle>& outgoing = {})
    {
        GncAtomHandle h = next_handle++;
        GncCognitiveAtom atom;
        atom.handle = h;
        atom.type = type;
        atom.name = name;
        atom.outgoing = outgoing;
        atom.attention.sti = 0.0;
        atom.attention.sti_funds = 10.0;
        atom.attention.lti = 0.0;
        atom.attention.lti_funds = 10.0;
        atom.attention.vlti = 0.0;
        atom.attention.confidence = 0.5;
        atom.attention.strength = 0.5;
        atom.attention.activity_level = 0.0;
        atom.attention.wage = 1.0;
        atom.attention.rent = 0.1;
        atom.attention.importance = 0.5;
        atom.attention.attention_value = 0.1;
        atom.tv_strength = 0.5;
        atom.tv_confidence = 0.5;
        atoms[h] = atom;

        for (GncAtomHandle out : outgoing) {
            auto it = atoms.find(out);
            if (it != atoms.end())
                it->second.incoming.push_back(h);
        }
        return h;
    }

    GncCognitiveAtom* get(GncAtomHandle h)
    {
        auto it = atoms.find(h);
        return it == atoms.end() ? nullptr : &it->second;
    }

    const GncCognitiveAtom* get(GncAtomHandle h) const
    {
        auto it = atoms.find(h);
        return it == atoms.end() ? nullptr : &it->second;
    }

    void set_tv(GncAtomHandle h, gdouble s, gdouble c)
    {
        auto *a = get(h);
        if (!a) return;
        a->tv_strength = CLAMP(s, 0.0, 1.0);
        a->tv_confidence = CLAMP(c, 0.0, 1.0);
        a->attention.strength = a->tv_strength;
        a->attention.confidence = a->tv_confidence;
    }

    void refresh_legacy_attention(GncAttentionParams& p)
    {
        p.importance = (p.sti + p.lti * 10.0) / 11.0;
        p.attention_value = std::min(1.0, (p.sti + p.lti + p.vlti * 100.0) / 200.0);
        if (p.attention_value < 0.0) p.attention_value = 0.0;
    }
};

static std::unique_ptr<GncCognitiveAtomSpace> g_atomspace;

static const char* COGNITIVE_TYPE_KEY = "cognitive-accounting-type";

static gdouble clamp01(gdouble v)
{
    return std::max(0.0, std::min(1.0, v));
}

static std::string sanitize_scheme_string(const char* raw)
{
    std::string out;
    if (!raw) return out;
    for (const char* p = raw; *p; ++p) {
        char c = *p;
        if (c == '\\' || c == '"')
            out.push_back('\\');
        if (static_cast<unsigned char>(c) >= 32 && c != 127)
            out.push_back(c);
        else
            out.push_back(' ');
    }
    return out;
}

/* ------------------------------------------------------------------ */
/* QOF event integration                                              */
/* ------------------------------------------------------------------ */

static void
cognitive_qof_event_handler(QofInstance *entity, QofEventId event_type,
                            gpointer user_data, gpointer event_data)
{
    (void)user_data;
    (void)event_data;
    if (!g_atomspace || !g_atomspace->auto_enabled)
        return;

    if (GNC_IS_TRANSACTION(entity)) {
        Transaction *tx = GNC_TRANSACTION(entity);
        if (event_type == QOF_EVENT_MODIFY || event_type == QOF_EVENT_CREATE) {
            if (!xaccTransIsOpen(tx))
                gnc_cognitive_accounting_on_transaction_commit(tx);
        }
    } else if (GNC_IS_ACCOUNT(entity)) {
        Account *acc = GNC_ACCOUNT(entity);
        if (event_type == QOF_EVENT_DESTROY)
            gnc_atomspace_remove_account(acc);
        else if (event_type == QOF_EVENT_CREATE || event_type == QOF_EVENT_MODIFY)
            gnc_account_to_atomspace(acc);
    }
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                          */
/* ------------------------------------------------------------------ */

gboolean
gnc_cognitive_accounting_init(void)
{
    if (g_atomspace) {
        g_message("Cognitive accounting already initialized");
        return TRUE;
    }

    g_atomspace = std::make_unique<GncCognitiveAtomSpace>();

    /* Backend selection from env before any optional dual-write hooks. */
    gnc_cognitive_backend_apply_env_default();

    if (!gnc_cognitive_scheme_init())
        g_warning("Failed to initialize Scheme cognitive interface");

    if (!gnc_cognitive_comms_init())
        g_warning("Failed to initialize cognitive communication hub");

    if (!gnc_tensor_network_init())
        g_warning("Failed to initialize tensor network");

    gnc_cognitive_register_module(GNC_MODULE_ATOMSPACE);
    gnc_cognitive_register_module(GNC_MODULE_PLN);
    gnc_cognitive_register_module(GNC_MODULE_ECAN);
    gnc_cognitive_register_module(GNC_MODULE_MOSES);
    gnc_cognitive_register_module(GNC_MODULE_URE);
    gnc_cognitive_register_module(GNC_MODULE_SCHEME);

    g_atomspace->event_handler_id =
        qof_event_register_handler(cognitive_qof_event_handler, nullptr);

    /* UI badges follow AUTO or explicit GNC_COGNITIVE_UI=1 */
    {
        const char *ui = g_getenv("GNC_COGNITIVE_UI");
        if ((ui && ui[0] == '1') || g_atomspace->auto_enabled)
            gnc_cognitive_ui_set_badges_enabled(TRUE);
    }

    g_message("Cognitive accounting framework initialized (backend=%s)",
              gnc_cognitive_backend_name());
    return TRUE;
}

void
gnc_cognitive_accounting_shutdown(void)
{
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return;
    }

    if (g_atomspace->event_handler_id)
        qof_event_unregister_handler(g_atomspace->event_handler_id);

    gnc_cognitive_comms_shutdown();
    gnc_tensor_network_shutdown();
    g_atomspace.reset();
    g_message("Cognitive accounting shutdown complete");
}

gboolean
gnc_cognitive_accounting_is_initialized(void)
{
    return g_atomspace != nullptr;
}

void
gnc_cognitive_accounting_set_auto_enabled(gboolean enabled)
{
    if (g_atomspace)
        g_atomspace->auto_enabled = enabled;
}

gboolean
gnc_cognitive_accounting_get_auto_enabled(void)
{
    return g_atomspace ? g_atomspace->auto_enabled : FALSE;
}

/* ------------------------------------------------------------------ */
/* AtomSpace primitives                                               */
/* ------------------------------------------------------------------ */

GncAtomHandle
gnc_atomspace_create_concept_node(const char* name)
{
    g_return_val_if_fail(name != nullptr, 0);
    if (!g_atomspace) return 0;
    return g_atomspace->create_atom(GNC_ATOM_CONCEPT_NODE, name);
}

GncAtomHandle
gnc_atomspace_create_predicate_node(const char* name)
{
    g_return_val_if_fail(name != nullptr, 0);
    if (!g_atomspace) return 0;
    return g_atomspace->create_atom(GNC_ATOM_PREDICATE_NODE, name);
}

GncAtomHandle
gnc_atomspace_create_evaluation_link(GncAtomHandle predicate_atom,
                                     GncAtomHandle account_atom,
                                     gdouble truth_value)
{
    g_return_val_if_fail(predicate_atom != 0 && account_atom != 0, 0);
    if (!g_atomspace) return 0;
    std::string name = "EvaluationLink:" + std::to_string(predicate_atom) +
                       ":" + std::to_string(account_atom);
    GncAtomHandle h = g_atomspace->create_atom(
        GNC_ATOM_EVALUATION_LINK, name, {predicate_atom, account_atom});
    g_atomspace->set_tv(h, clamp01(truth_value), 0.9);
    return h;
}

GncAtomHandle
gnc_atomspace_create_inheritance_link(GncAtomHandle child_atom,
                                      GncAtomHandle parent_atom)
{
    g_return_val_if_fail(child_atom != 0 && parent_atom != 0, 0);
    if (!g_atomspace) return 0;
    std::string name = "InheritanceLink:" + std::to_string(child_atom) +
                       "->" + std::to_string(parent_atom);
    return g_atomspace->create_atom(
        GNC_ATOM_INHERITANCE_LINK, name, {child_atom, parent_atom});
}

void
gnc_atomspace_set_truth_value(GncAtomHandle atom_handle,
                              gdouble strength, gdouble confidence)
{
    g_return_if_fail(atom_handle != 0);
    if (!g_atomspace) return;
    g_atomspace->set_tv(atom_handle, strength, confidence);
}

gboolean
gnc_atomspace_get_truth_value(GncAtomHandle atom_handle,
                              gdouble* strength, gdouble* confidence)
{
    g_return_val_if_fail(atom_handle != 0, FALSE);
    if (!g_atomspace) return FALSE;
    const auto *a = g_atomspace->get(atom_handle);
    if (!a) return FALSE;
    if (strength) *strength = a->tv_strength;
    if (confidence) *confidence = a->tv_confidence;
    return TRUE;
}

GncAtomType
gnc_atomspace_get_atom_type(GncAtomHandle atom_handle)
{
    if (!g_atomspace) return GNC_ATOM_CONCEPT_NODE;
    const auto *a = g_atomspace->get(atom_handle);
    return a ? a->type : GNC_ATOM_CONCEPT_NODE;
}

const char*
gnc_atomspace_get_atom_name(GncAtomHandle atom_handle)
{
    if (!g_atomspace) return nullptr;
    const auto *a = g_atomspace->get(atom_handle);
    return a ? a->name.c_str() : nullptr;
}

guint
gnc_atomspace_get_outgoing_size(GncAtomHandle atom_handle)
{
    if (!g_atomspace) return 0;
    const auto *a = g_atomspace->get(atom_handle);
    return a ? static_cast<guint>(a->outgoing.size()) : 0;
}

GncAtomHandle
gnc_atomspace_get_outgoing(GncAtomHandle atom_handle, guint index)
{
    if (!g_atomspace) return 0;
    const auto *a = g_atomspace->get(atom_handle);
    if (!a || index >= a->outgoing.size()) return 0;
    return a->outgoing[index];
}

guint
gnc_atomspace_get_incoming_size(GncAtomHandle atom_handle)
{
    if (!g_atomspace) return 0;
    const auto *a = g_atomspace->get(atom_handle);
    return a ? static_cast<guint>(a->incoming.size()) : 0;
}

GncAtomHandle
gnc_atomspace_get_incoming(GncAtomHandle atom_handle, guint index)
{
    if (!g_atomspace) return 0;
    const auto *a = g_atomspace->get(atom_handle);
    if (!a || index >= a->incoming.size()) return 0;
    return a->incoming[index];
}

GncAtomHandle
gnc_atomspace_create_hierarchy_link(GncAtomHandle parent_atom,
                                    GncAtomHandle child_atom)
{
    /* API historically takes parent then child; inheritance is child->parent */
    return gnc_atomspace_create_inheritance_link(child_atom, parent_atom);
}

GncAtomHandle
gnc_account_to_atomspace(const Account *account)
{
    g_return_val_if_fail(account != nullptr, 0);
    if (!g_atomspace) {
        g_warning("Cognitive accounting not initialized");
        return 0;
    }

    auto it = g_atomspace->account_atoms.find(account);
    if (it != g_atomspace->account_atoms.end())
        return it->second;

    const char *aname = xaccAccountGetName(account);
    std::string name = std::string("Account:") + (aname ? aname : "unnamed");
    GncAtomHandle concept_atom = g_atomspace->create_atom(GNC_ATOM_CONCEPT_NODE, name);
    g_atomspace->account_atoms[account] = concept_atom;

    GNCAccountType atype = xaccAccountGetType(account);
    const char *type_str = xaccAccountTypeEnumAsString(atype);
    GncAtomHandle type_node = g_atomspace->create_atom(
        GNC_ATOM_CONCEPT_NODE,
        std::string("AccountType:") + (type_str ? type_str : "UNKNOWN"));
    gnc_atomspace_create_inheritance_link(concept_atom, type_node);

    Account *parent = gnc_account_get_parent(const_cast<Account*>(account));
    if (parent && parent != account) {
        GncAtomHandle parent_atom = gnc_account_to_atomspace(parent);
        if (parent_atom)
            gnc_atomspace_create_inheritance_link(concept_atom, parent_atom);
    }

    gnc_numeric bal = xaccAccountGetBalance(account);
    gdouble bal_d = gnc_numeric_to_double(bal);
    gdouble tv = clamp01(1.0 / (1.0 + std::abs(bal_d) / 10000.0));
    GncAtomHandle pred = gnc_atomspace_create_predicate_node("hasBalance");
    gnc_atomspace_create_evaluation_link(pred, concept_atom, tv);

    return concept_atom;
}

void
gnc_atomspace_remove_account(const Account *account)
{
    if (!g_atomspace || !account) return;
    g_atomspace->account_atoms.erase(account);
}

GncAtomHandle
gnc_transaction_to_atomspace(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, 0);
    if (!g_atomspace) return 0;

    auto it = g_atomspace->transaction_atoms.find(transaction);
    if (it != g_atomspace->transaction_atoms.end())
        return it->second;

    std::string name = "Transaction:" +
        std::to_string(reinterpret_cast<uintptr_t>(transaction));
    GncAtomHandle tx_atom = g_atomspace->create_atom(GNC_ATOM_CONCEPT_NODE, name);

    GList *splits = xaccTransGetSplitList(const_cast<Transaction*>(transaction));
    std::vector<GncAtomHandle> split_atoms;
    for (GList *n = splits; n; n = n->next) {
        Split *split = GNC_SPLIT(n->data);
        Account *acc = xaccSplitGetAccount(split);
        if (!acc) continue;
        GncAtomHandle acc_atom = gnc_account_to_atomspace(acc);
        gdouble amt = gnc_numeric_to_double(xaccSplitGetAmount(split));
        std::string sname = "Split:" + std::to_string(acc_atom) +
                            ":" + std::to_string(amt);
        GncAtomHandle s_atom = g_atomspace->create_atom(
            GNC_ATOM_EVALUATION_LINK, sname, {tx_atom, acc_atom});
        g_atomspace->set_tv(s_atom, clamp01(1.0 - std::abs(amt) / 1e6), 0.8);
        split_atoms.push_back(s_atom);
    }

    GncTruthValue tv{};
    gnc_pln_validate_double_entry_tv(transaction, &tv);
    GncAtomHandle rule = g_atomspace->create_atom(
        GNC_ATOM_IMPLICATION_LINK,
        "DoubleEntry:" + name,
        split_atoms);
    g_atomspace->set_tv(rule, tv.strength, tv.confidence);
    g_atomspace->transaction_atoms[transaction] = tx_atom;
    g_atomspace->last_validation[transaction] = tv.strength * tv.confidence;
    return tx_atom;
}

/* ------------------------------------------------------------------ */
/* PLN                                                                */
/* ------------------------------------------------------------------ */

gboolean
gnc_pln_validate_double_entry_tv(const Transaction *transaction,
                                 GncTruthValue *tv_out)
{
    g_return_val_if_fail(transaction != nullptr, FALSE);
    g_return_val_if_fail(tv_out != nullptr, FALSE);

    tv_out->strength = 0.0;
    tv_out->confidence = 0.0;

    if (!g_atomspace)
        return FALSE;

    GList *splits = xaccTransGetSplitList(const_cast<Transaction*>(transaction));
    gint split_count = g_list_length(splits);
    if (split_count == 0)
        return TRUE;

    gnc_numeric total = gnc_numeric_zero();
    gdouble total_magnitude = 0.0;
    gdouble total_attention = 0.0;
    gint valid_accounts = 0;

    for (GList *node = splits; node; node = node->next) {
        Split *split = GNC_SPLIT(node->data);
        gnc_numeric amount = xaccSplitGetAmount(split);
        total = gnc_numeric_add(total, amount, GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);
        total_magnitude += std::abs(gnc_numeric_to_double(amount));
        Account *account = xaccSplitGetAccount(split);
        if (account) {
            /* Ensure mapped */
            gnc_account_to_atomspace(account);
            GncAttentionParams params = gnc_ecan_get_attention_params(account);
            total_attention += params.sti + params.lti;
            valid_accounts++;
        }
    }

    gdouble account_reliability = 0.5;
    if (valid_accounts > 0)
        account_reliability = clamp01(total_attention / (valid_accounts * 100.0) + 0.5);

    time64 tx_time = xaccTransGetDate(const_cast<Transaction*>(transaction));
    time64 current_time = gnc_time(nullptr);
    gdouble age_days = 0.0;
    if (tx_time > 0 && current_time > tx_time)
        age_days = static_cast<gdouble>(current_time - tx_time) / (24.0 * 3600.0);
    gdouble temporal = std::exp(-age_days / 365.0);
    gdouble complexity = std::log1p(static_cast<gdouble>(split_count)) / std::log(10.0);

    if (gnc_numeric_zero_p(total)) {
        tv_out->strength = 0.98;
        gdouble evidence = std::min(0.99, 0.55 + 0.05 * split_count);
        gdouble complexity_factor = 1.0 - 0.10 * std::min(1.0, complexity);
        gdouble reliability_factor = 0.85 + 0.15 * account_reliability;
        gdouble temporal_factor = 0.90 + 0.10 * temporal;
        tv_out->confidence = clamp01(evidence * complexity_factor *
                                     reliability_factor * temporal_factor);
        tv_out->confidence = std::max(0.70, tv_out->confidence);
    } else {
        gdouble imbalance = gnc_numeric_to_double(gnc_numeric_abs(total));
        if (total_magnitude <= 0.0)
            total_magnitude = imbalance;
        gdouble relative = imbalance / total_magnitude;
        tv_out->strength = clamp01(std::exp(-8.0 * relative) * account_reliability * temporal);
        gdouble base_c = 1.0 - relative;
        gdouble evidence = std::min(1.0, split_count / 4.0);
        tv_out->confidence = clamp01(base_c * evidence * (1.0 - 0.1 * complexity) *
                                     account_reliability * temporal);
        tv_out->confidence = std::max(0.05, std::min(0.95, tv_out->confidence));
    }

    if (g_atomspace) {
        std::string vname = "DoubleEntryValidation:" +
            std::to_string(reinterpret_cast<uintptr_t>(transaction));
        GncAtomHandle vatom = g_atomspace->create_atom(GNC_ATOM_IMPLICATION_LINK, vname);
        g_atomspace->set_tv(vatom, tv_out->strength, tv_out->confidence);
        g_atomspace->last_validation[transaction] = tv_out->strength * tv_out->confidence;
    }
    return TRUE;
}

gdouble
gnc_pln_validate_double_entry(const Transaction *transaction)
{
    GncTruthValue tv{};
    if (!gnc_pln_validate_double_entry_tv(transaction, &tv))
        return 0.0;
    return clamp01(tv.strength * tv.confidence);
}

gdouble
gnc_pln_validate_n_entry(const Transaction *transaction, gint n_parties)
{
    g_return_val_if_fail(transaction != nullptr, 0.0);
    g_return_val_if_fail(n_parties >= 2, 0.0);
    if (!g_atomspace)
        return gnc_pln_validate_double_entry(transaction);

    GList *splits = xaccTransGetSplitList(const_cast<Transaction*>(transaction));
    gint split_count = g_list_length(splits);
    if (split_count < n_parties)
        return 0.0;

    GncTruthValue tv{};
    gnc_pln_validate_double_entry_tv(transaction, &tv);
    gdouble complexity_factor = 1.0 / (1.0 + 0.1 * (n_parties - 2));
    gdouble evidence_factor = std::min(1.0, split_count / static_cast<gdouble>(n_parties));
    gdouble strength = tv.strength * complexity_factor;
    gdouble confidence = std::min(0.95, tv.confidence * evidence_factor);

    std::string name = "NEntryValidation:Parties:" + std::to_string(n_parties);
    GncAtomHandle atom = g_atomspace->create_atom(GNC_ATOM_IMPLICATION_LINK, name);
    g_atomspace->set_tv(atom, strength, confidence);
    return clamp01(strength * confidence);
}

static void
accumulate_account_tree_balances(const Account *account,
                                 gnc_numeric *debits, gnc_numeric *credits)
{
    if (!account) return;
    gnc_numeric bal = xaccAccountGetBalance(account);
    gdouble d = gnc_numeric_to_double(bal);
    if (d >= 0.0)
        *debits = gnc_numeric_add(*debits, bal, GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);
    else
        *credits = gnc_numeric_add(*credits, gnc_numeric_neg(bal),
                                   GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);

    GList *children = gnc_account_get_children(const_cast<Account*>(account));
    for (GList *n = children; n; n = n->next)
        accumulate_account_tree_balances(GNC_ACCOUNT(n->data), debits, credits);
    g_list_free(children);
}

gboolean
gnc_pln_trial_balance_report(const Account *root_account, GncProofReport *report_out)
{
    g_return_val_if_fail(root_account != nullptr, FALSE);
    g_return_val_if_fail(report_out != nullptr, FALSE);
    if (!g_atomspace) return FALSE;

    memset(report_out, 0, sizeof(*report_out));
    report_out->total_debits = gnc_numeric_zero();
    report_out->total_credits = gnc_numeric_zero();
    accumulate_account_tree_balances(root_account,
                                     &report_out->total_debits,
                                     &report_out->total_credits);
    report_out->imbalance = gnc_numeric_sub(report_out->total_debits,
                                            report_out->total_credits,
                                            GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);
    report_out->balanced = gnc_numeric_zero_p(report_out->imbalance);

    gdouble imb = std::abs(gnc_numeric_to_double(report_out->imbalance));
    gdouble mag = std::abs(gnc_numeric_to_double(report_out->total_debits)) +
                  std::abs(gnc_numeric_to_double(report_out->total_credits));
    if (report_out->balanced) {
        report_out->strength = 0.98;
        report_out->confidence = 0.95;
    } else if (mag > 0.0) {
        gdouble rel = imb / mag;
        report_out->strength = clamp01(std::exp(-8.0 * rel));
        report_out->confidence = clamp01(1.0 - rel);
    } else {
        report_out->strength = 0.5;
        report_out->confidence = 0.5;
    }

    const char *rname = xaccAccountGetName(root_account);
    std::string proof_name = std::string("TrialBalanceProof:") + (rname ? rname : "root");
    report_out->proof_atom = g_atomspace->create_atom(GNC_ATOM_SCHEMA_NODE, proof_name);
    g_atomspace->set_tv(report_out->proof_atom, report_out->strength, report_out->confidence);
    return TRUE;
}

GncAtomHandle
gnc_pln_generate_trial_balance_proof(const Account *root_account)
{
    GncProofReport report{};
    if (!gnc_pln_trial_balance_report(root_account, &report))
        return 0;
    return report.proof_atom;
}

gboolean
gnc_pln_pl_report(const Account *income_account, const Account *expense_account,
                  GncProofReport *report_out)
{
    g_return_val_if_fail(income_account && expense_account && report_out, FALSE);
    if (!g_atomspace) return FALSE;

    memset(report_out, 0, sizeof(*report_out));
    gnc_numeric income = xaccAccountGetBalance(income_account);
    gnc_numeric expense = xaccAccountGetBalance(expense_account);
    /* Income accounts typically negative in GnuCash sign convention; use abs nets */
    gdouble inc = std::abs(gnc_numeric_to_double(income));
    gdouble exp = std::abs(gnc_numeric_to_double(expense));
    report_out->total_debits = gnc_numeric_create(static_cast<gint64>(exp * 100), 100);
    report_out->total_credits = gnc_numeric_create(static_cast<gint64>(inc * 100), 100);
    report_out->imbalance = gnc_numeric_sub(report_out->total_credits,
                                            report_out->total_debits,
                                            GNC_DENOM_AUTO, GNC_HOW_RND_ROUND_HALF_UP);
    report_out->balanced = TRUE; /* P&L is not required to zero */
    report_out->strength = 0.9;
    report_out->confidence = 0.85;

    std::string name = std::string("PLProof:") +
        (xaccAccountGetName(income_account) ? xaccAccountGetName(income_account) : "I") +
        "-" +
        (xaccAccountGetName(expense_account) ? xaccAccountGetName(expense_account) : "E");
    report_out->proof_atom = g_atomspace->create_atom(GNC_ATOM_SCHEMA_NODE, name);
    g_atomspace->set_tv(report_out->proof_atom, report_out->strength, report_out->confidence);
    return TRUE;
}

GncAtomHandle
gnc_pln_generate_pl_proof(const Account *income_account,
                          const Account *expense_account)
{
    GncProofReport report{};
    if (!gnc_pln_pl_report(income_account, expense_account, &report))
        return 0;
    return report.proof_atom;
}

gdouble
gnc_pln_get_last_validation_score(const Transaction *transaction)
{
    if (!g_atomspace || !transaction) return 0.0;
    auto it = g_atomspace->last_validation.find(transaction);
    return it == g_atomspace->last_validation.end() ? 0.0 : it->second;
}

/* ------------------------------------------------------------------ */
/* ECAN                                                               */
/* ------------------------------------------------------------------ */

void
gnc_ecan_update_account_attention(Account *account,
                                  const Transaction *transaction)
{
    g_return_if_fail(account != nullptr);
    g_return_if_fail(transaction != nullptr);
    if (!g_atomspace) return;

    GncAtomHandle h = gnc_account_to_atomspace(account);
    auto *atom = g_atomspace->get(h);
    if (!atom) return;

    GList *splits = xaccTransGetSplitList(const_cast<Transaction*>(transaction));
    gint split_count = g_list_length(splits);
    gdouble magnitude = 0.0;
    std::vector<Account*> co_accounts;

    for (GList *node = splits; node; node = node->next) {
        Split *split = GNC_SPLIT(node->data);
        Account *acc = xaccSplitGetAccount(split);
        if (!acc) continue;
        if (acc == account)
            magnitude += std::abs(gnc_numeric_to_double(xaccSplitGetAmount(split)));
        else
            co_accounts.push_back(acc);
    }

    auto& params = atom->attention;
    gdouble activity_boost = 0.05 + (split_count * 0.02) + (magnitude / 10000.0);
    activity_boost = std::min(0.5, activity_boost);
    gdouble wage_payment = params.wage * (1.0 + params.lti / 100.0) *
                           (1.0 + params.activity_level) * activity_boost;

    if (g_atomspace->total_sti_funds >= wage_payment) {
        params.sti += wage_payment;
        g_atomspace->total_sti_funds -= wage_payment;
        params.activity_level += activity_boost;
        gdouble rent_payment = params.rent * (1.0 + params.sti / 100.0);
        if (params.sti > rent_payment)
            params.sti -= rent_payment;
    }

    gdouble lti_growth = activity_boost * 0.1;
    if (g_atomspace->total_lti_funds >= lti_growth) {
        params.lti += lti_growth;
        g_atomspace->total_lti_funds -= lti_growth;
    }
    if (params.lti > 50.0 && params.activity_level > 1.0)
        params.vlti += 0.001;

    params.sti *= (1.0 - g_atomspace->attention_decay_rate);
    params.activity_level *= 0.98;
    g_atomspace->refresh_legacy_attention(params);

    /* Hebbian-style co-occurrence boost */
    for (Account *other : co_accounts) {
        GncAtomHandle oh = gnc_account_to_atomspace(other);
        auto *oatom = g_atomspace->get(oh);
        if (!oatom) continue;
        oatom->attention.sti += activity_boost * 0.1;
        g_atomspace->refresh_legacy_attention(oatom->attention);
        std::string link_name = "Hebbian:" + std::to_string(h) + "-" + std::to_string(oh);
        GncAtomHandle link = g_atomspace->create_atom(
            GNC_ATOM_SIMILARITY_LINK, link_name, {h, oh});
        g_atomspace->set_tv(link, clamp01(activity_boost), 0.6);
    }
}

GncAttentionParams
gnc_ecan_get_attention_params(const Account *account)
{
    GncAttentionParams def{};
    g_return_val_if_fail(account != nullptr, def);
    if (!g_atomspace) return def;

    auto it = g_atomspace->account_atoms.find(account);
    if (it == g_atomspace->account_atoms.end()) {
        /* Map lazily so subsequent updates work */
        GncAtomHandle h = gnc_account_to_atomspace(account);
        auto *a = g_atomspace->get(h);
        return a ? a->attention : def;
    }
    auto *a = g_atomspace->get(it->second);
    return a ? a->attention : def;
}

void
gnc_ecan_allocate_attention(Account **accounts, gint n_accounts)
{
    g_return_if_fail(accounts != nullptr && n_accounts > 0);
    if (!g_atomspace) return;

    gdouble total_activity = 0.0;
    std::vector<GncAtomHandle> handles;
    std::vector<gdouble> scores;

    for (gint i = 0; i < n_accounts; i++) {
        if (!accounts[i]) continue;
        GncAtomHandle h = gnc_account_to_atomspace(accounts[i]);
        auto *a = g_atomspace->get(h);
        if (!a) continue;
        handles.push_back(h);
        gdouble score = a->attention.activity_level + a->attention.sti / 100.0 +
                        a->attention.lti / 50.0;
        scores.push_back(std::max(0.01, score));
        total_activity += scores.back();
    }
    if (handles.empty() || total_activity <= 0.0) return;

    gdouble sti_pool = g_atomspace->total_sti_funds * 0.1;
    gdouble lti_pool = g_atomspace->total_lti_funds * 0.05;
    for (size_t i = 0; i < handles.size(); i++) {
        auto *a = g_atomspace->get(handles[i]);
        if (!a) continue;
        gdouble ratio = scores[i] / total_activity;
        gdouble ds = sti_pool * ratio;
        gdouble dl = lti_pool * ratio;
        a->attention.sti += ds;
        a->attention.lti += dl;
        g_atomspace->total_sti_funds -= ds;
        g_atomspace->total_lti_funds -= dl;
        gdouble rent = a->attention.rent * (1.0 + a->attention.sti / 200.0);
        if (a->attention.sti > rent) a->attention.sti -= rent;
        a->attention.sti *= (1.0 - g_atomspace->attention_decay_rate);
        a->attention.activity_level *= 0.95;
        g_atomspace->refresh_legacy_attention(a->attention);
    }

    g_atomspace->total_sti_funds = std::min(2000.0, g_atomspace->total_sti_funds + 50.0);
    g_atomspace->total_lti_funds = std::min(1000.0, g_atomspace->total_lti_funds + 10.0);
}

void
gnc_ecan_decay_tick(void)
{
    if (!g_atomspace) return;
    for (auto &pair : g_atomspace->atoms) {
        auto &p = pair.second.attention;
        p.sti *= (1.0 - g_atomspace->attention_decay_rate);
        p.lti *= (1.0 - g_atomspace->attention_decay_rate * 0.1);
        if (p.sti > p.rent) {
            p.sti -= p.rent;
            g_atomspace->total_sti_funds += p.rent * 0.5;
        }
        g_atomspace->refresh_legacy_attention(p);
    }
}

gint
gnc_ecan_top_accounts(Account **out_accounts, gint max_accounts)
{
    g_return_val_if_fail(out_accounts != nullptr && max_accounts > 0, 0);
    if (!g_atomspace) return 0;

    std::vector<std::pair<gdouble, const Account*>> ranked;
    for (const auto &pair : g_atomspace->account_atoms) {
        auto *a = g_atomspace->get(pair.second);
        if (!a) continue;
        ranked.emplace_back(a->attention.attention_value, pair.first);
    }
    std::sort(ranked.begin(), ranked.end(),
              [](const auto &x, const auto &y) { return x.first > y.first; });

    gint n = std::min(max_accounts, static_cast<gint>(ranked.size()));
    for (gint i = 0; i < n; i++)
        out_accounts[i] = const_cast<Account*>(ranked[i].second);
    return n;
}

/* ------------------------------------------------------------------ */
/* MOSES                                                              */
/* ------------------------------------------------------------------ */

GncAtomHandle
gnc_moses_discover_balancing_strategies(Transaction **historical_transactions,
                                        gint n_transactions)
{
    g_return_val_if_fail(historical_transactions != nullptr && n_transactions > 0, 0);
    if (!g_atomspace) return 0;

    std::map<std::string, gint> freq;
    std::map<std::string, gdouble> fitness;
    std::ostringstream json;
    json << "[";

    for (gint i = 0; i < n_transactions; i++) {
        Transaction *trans = historical_transactions[i];
        if (!trans) continue;
        GList *splits = xaccTransGetSplitList(trans);
        gint split_count = g_list_length(splits);
        gint dow = 0;
        time64 t = xaccTransGetDate(trans);
        if (t > 0) {
            GDate date;
            g_date_clear(&date, 1);
            g_date_set_time_t(&date, static_cast<time_t>(t));
            dow = g_date_get_weekday(&date);
        }
        gdouble mag = 0.0;
        std::map<GNCAccountType, gint> type_counts;
        for (GList *n = splits; n; n = n->next) {
            Split *s = GNC_SPLIT(n->data);
            mag += std::abs(gnc_numeric_to_double(xaccSplitGetAmount(s)));
            Account *a = xaccSplitGetAccount(s);
            if (a) type_counts[xaccAccountGetType(a)]++;
        }
        gint bucket = static_cast<gint>(std::log1p(mag));
        std::string key = "sc:" + std::to_string(split_count) +
                          "|dow:" + std::to_string(dow) +
                          "|amt:" + std::to_string(bucket);
        for (auto &tc : type_counts)
            key += "|t" + std::to_string(tc.first) + ":" + std::to_string(tc.second);

        gdouble v = gnc_pln_validate_double_entry(trans);
        freq[key]++;
        fitness[key] += v;
    }

    std::string best;
    gdouble best_fit = -1.0;
    gboolean first = TRUE;
    for (auto &p : freq) {
        gdouble avg = fitness[p.first] / p.second;
        gdouble weighted = avg * std::sqrt(static_cast<gdouble>(p.second));
        if (!first) json << ",";
        first = FALSE;
        json << "{\"pattern\":\"" << p.first << "\",\"frequency\":" << p.second
             << ",\"avg_fitness\":" << avg << ",\"weighted\":" << weighted << "}";
        if (weighted > best_fit) {
            best_fit = weighted;
            best = p.first;
        }
    }
    json << "]";
    g_atomspace->last_moses_json = json.str();

    std::string strategy_name = "MOSESStrategy:" + best +
                                ":Fitness:" + std::to_string(best_fit);
    GncAtomHandle strategy_atom =
        g_atomspace->create_atom(GNC_ATOM_COMBO_NODE, strategy_name);
    gdouble confidence = best.empty() ? 0.3 :
        std::min(0.95, freq[best] / static_cast<gdouble>(n_transactions));
    gdouble strength = clamp01(best_fit > 0 ? best_fit : 0.3);
    g_atomspace->set_tv(strategy_atom, strength, confidence);

    auto *a = g_atomspace->get(strategy_atom);
    if (a) {
        a->attention.sti = strength * 50.0;
        a->attention.lti += 10.0;
        g_atomspace->refresh_legacy_attention(a->attention);
    }

    gnc_scheme_evolutionary_optimization(historical_transactions, n_transactions);
    return strategy_atom;
}

Transaction*
gnc_moses_optimize_transaction(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, nullptr);
    if (!g_atomspace)
        return const_cast<Transaction*>(transaction);

    gdouble fitness = gnc_pln_validate_double_entry(transaction);
    GncAtomHandle opt = g_atomspace->create_atom(
        GNC_ATOM_GROUNDED_SCHEMA,
        "MOSESOptimization:" + std::to_string(reinterpret_cast<uintptr_t>(transaction)));
    g_atomspace->set_tv(opt, clamp01(fitness), 0.8);

    /* Never mutate committed transactions; return original pointer. */
    return const_cast<Transaction*>(transaction);
}

char*
gnc_moses_last_strategies_json(void)
{
    if (!g_atomspace)
        return g_strdup("[]");
    return g_strdup(g_atomspace->last_moses_json.c_str());
}

/* ------------------------------------------------------------------ */
/* URE                                                                */
/* ------------------------------------------------------------------ */

gboolean
gnc_ure_predict_balance_ex(const Account *account, time64 future_date,
                           GncUrePrediction *prediction_out)
{
    g_return_val_if_fail(account != nullptr && prediction_out != nullptr, FALSE);
    memset(prediction_out, 0, sizeof(*prediction_out));

    gnc_numeric current = xaccAccountGetBalance(account);
    time64 now = gnc_time(nullptr);
    if (future_date <= now) {
        prediction_out->point_estimate = current;
        prediction_out->lower_bound = current;
        prediction_out->upper_bound = current;
        prediction_out->confidence = 1.0;
        return TRUE;
    }

    /* Mean drift from recent split activity on this account */
    gdouble sum_flow = 0.0;
    gdouble sum_sq = 0.0;
    gint n_flow = 0;
    time64 earliest = now;
    for (GList *n = xaccAccountGetSplitList(const_cast<Account*>(account)); n; n = n->next) {
        Split *s = GNC_SPLIT(n->data);
        Transaction *tx = xaccSplitGetParent(s);
        if (!tx) continue;
        time64 td = xaccTransGetDate(tx);
        if (td <= 0) continue;
        if (td < earliest) earliest = td;
        gdouble amt = gnc_numeric_to_double(xaccSplitGetAmount(s));
        sum_flow += amt;
        sum_sq += amt * amt;
        n_flow++;
        if (n_flow >= 64) break; /* bound work */
    }

    gdouble days_hist = std::max(1.0, static_cast<gdouble>(now - earliest) / 86400.0);
    gdouble daily_drift = (n_flow > 0) ? (sum_flow / days_hist) : 0.0;
    gdouble mean = (n_flow > 0) ? (sum_flow / n_flow) : 0.0;
    gdouble var = (n_flow > 1) ? (sum_sq / n_flow - mean * mean) : 0.0;
    if (var < 0.0) var = 0.0;
    gdouble stdev = std::sqrt(var);

    gdouble days_future = static_cast<gdouble>(future_date - now) / 86400.0;
    gdouble cur = gnc_numeric_to_double(current);
    gdouble point = cur + daily_drift * days_future;
    gdouble uncertainty = stdev * std::sqrt(std::max(1.0, days_future / 30.0)) +
                          std::abs(daily_drift) * 0.1 * days_future;

    GncAttentionParams att = gnc_ecan_get_attention_params(account);
    gdouble conf = clamp01(0.5 + 0.3 * att.confidence + 0.2 * std::min(1.0, n_flow / 20.0));
    conf *= std::exp(-days_future / 365.0);
    conf = clamp01(conf);

    prediction_out->point_estimate = double_to_gnc_numeric(point, 100, GNC_HOW_RND_ROUND_HALF_UP);
    prediction_out->lower_bound = double_to_gnc_numeric(point - 1.96 * uncertainty, 100,
                                                        GNC_HOW_RND_ROUND_HALF_UP);
    prediction_out->upper_bound = double_to_gnc_numeric(point + 1.96 * uncertainty, 100,
                                                        GNC_HOW_RND_ROUND_HALF_UP);
    prediction_out->confidence = conf;

    if (g_atomspace) {
        GncAtomHandle h = g_atomspace->create_atom(
            GNC_ATOM_EVALUATION_LINK,
            std::string("UREPrediction:") +
                (xaccAccountGetName(account) ? xaccAccountGetName(account) : "?"));
        g_atomspace->set_tv(h, conf, conf);
    }
    return TRUE;
}

gnc_numeric
gnc_ure_predict_balance(const Account *account, time64 future_date)
{
    GncUrePrediction pred{};
    if (!gnc_ure_predict_balance_ex(account, future_date, &pred))
        return account ? xaccAccountGetBalance(account) : gnc_numeric_zero();
    return pred.point_estimate;
}

gdouble
gnc_ure_transaction_validity(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, 0.0);
    if (!g_atomspace)
        return gnc_pln_validate_double_entry(transaction);

    GncTruthValue tv{};
    gnc_pln_validate_double_entry_tv(transaction, &tv);
    gdouble base = tv.strength * tv.confidence;

    GList *splits = xaccTransGetSplitList(const_cast<Transaction*>(transaction));
    gint split_count = g_list_length(splits);
    gdouble complexity_u = 1.0;
    if (split_count > 2)
        complexity_u = std::max(0.5, 1.0 - 0.05 * (split_count - 2));

    time64 trans_time = xaccTransGetDate(const_cast<Transaction*>(transaction));
    time64 current_time = gnc_time(nullptr);
    gdouble temporal_u = 1.0;
    if (trans_time > 0 && current_time > trans_time) {
        gdouble age_days = static_cast<gdouble>(current_time - trans_time) / 86400.0;
        temporal_u = std::max(0.3, std::exp(-age_days / 365.0));
    }

    gdouble account_u = 0.5;
    gint ac = 0;
    gdouble att_sum = 0.0;
    for (GList *node = splits; node; node = node->next) {
        Account *account = xaccSplitGetAccount(GNC_SPLIT(node->data));
        if (!account) continue;
        GncAttentionParams p = gnc_ecan_get_attention_params(account);
        att_sum += p.confidence;
        ac++;
    }
    if (ac > 0) account_u = att_sum / ac;

    gdouble combined = (complexity_u + temporal_u + account_u) / 3.0;
    gdouble final_v = clamp01(base * combined);

    GncAtomHandle atom = g_atomspace->create_atom(
        GNC_ATOM_EVALUATION_LINK,
        "UREValidity:" + std::to_string(reinterpret_cast<uintptr_t>(transaction)));
    g_atomspace->set_tv(atom, final_v, combined);
    return final_v;
}

/* ------------------------------------------------------------------ */
/* Scheme export helpers                                              */
/* ------------------------------------------------------------------ */

char*
gnc_account_to_scheme_representation(const Account *account)
{
    g_return_val_if_fail(account != nullptr, nullptr);
    const char *aname = xaccAccountGetName(account);
    std::string safe = sanitize_scheme_string(aname ? aname : "unnamed");
    GNCAccountType t = xaccAccountGetType(account);
    const char *ts = xaccAccountTypeEnumAsString(t);
    std::ostringstream ss;
    ss << "; Account hypergraph export\n"
       << "(define account-repr\n"
       << "  (list\n"
       << "    (ConceptNode \"Account:" << safe << "\")\n"
       << "    (InheritanceLink\n"
       << "      (ConceptNode \"Account:" << safe << "\")\n"
       << "      (ConceptNode \"AccountType:" << (ts ? ts : "UNKNOWN") << "\"))\n"
       << "    (EvaluationLink\n"
       << "      (PredicateNode \"hasBalance\")\n"
       << "      (ConceptNode \"Account:" << safe << "\"))))\n";
    return g_strdup(ss.str().c_str());
}

char*
gnc_transaction_to_scheme_pattern(const Transaction *transaction)
{
    g_return_val_if_fail(transaction != nullptr, nullptr);
    GList *splits = xaccTransGetSplitList(const_cast<Transaction*>(transaction));
    gint sc = g_list_length(splits);
    std::ostringstream ss;
    ss << "(BindLink\n"
       << "  (VariableNode \"$split\")\n"
       << "  (AndLink\n"
       << "    (EvaluationLink (PredicateNode \"inTransaction\") "
       << "(ListLink (ConceptNode \"TX\") (VariableNode \"$split\")))\n"
       << "    (EqualLink (ArityOf (ConceptNode \"TX\")) "
       << "(NumberNode \"" << sc << "\"))))\n";
    return g_strdup(ss.str().c_str());
}

GncAtomHandle
gnc_evaluate_scheme_expression(const char* scheme_expr)
{
    g_return_val_if_fail(scheme_expr != nullptr, 0);
    if (!g_atomspace) return 0;
    /* Do not eval untrusted book data; record as concept only. */
    std::string safe = sanitize_scheme_string(scheme_expr);
    if (safe.size() > 200) safe.resize(200);
    return g_atomspace->create_atom(GNC_ATOM_CONCEPT_NODE, "SchemeExpr:" + safe);
}

char*
gnc_create_hypergraph_pattern_encoding(const Account *root_account)
{
    g_return_val_if_fail(root_account != nullptr, nullptr);
    std::ostringstream hypergraph_pattern;
    hypergraph_pattern << "; Hypergraph pattern encoding for account hierarchy\n";
    hypergraph_pattern << "(BindLink\n";
    hypergraph_pattern << "  (VariableList\n";
    hypergraph_pattern << "    (TypedVariableLink\n";
    hypergraph_pattern << "      (VariableNode \"$account\")\n";
    hypergraph_pattern << "      (TypeNode \"ConceptNode\")))\n";
    hypergraph_pattern << "  (AndLink\n";

    std::function<void(const Account*, int)> add_account_pattern =
        [&](const Account* account, int depth) {
            if (!account) return;
            std::string account_name = sanitize_scheme_string(
                xaccAccountGetName(account) ? xaccAccountGetName(account) : "unnamed_account");
            hypergraph_pattern << std::string(depth * 2, ' ')
                               << "    (InheritanceLink\n";
            hypergraph_pattern << std::string(depth * 2, ' ')
                               << "      (VariableNode \"$account\")\n";
            hypergraph_pattern << std::string(depth * 2, ' ')
                               << "      (ConceptNode \"Account:" << account_name << "\"))\n";
            GList *children = gnc_account_get_children(const_cast<Account*>(account));
            for (GList *node = children; node; node = node->next)
                add_account_pattern(GNC_ACCOUNT(node->data), depth + 1);
            g_list_free(children);
        };

    add_account_pattern(root_account, 0);
    hypergraph_pattern << "  )\n";
    hypergraph_pattern << "  (VariableNode \"$account\"))\n";
    return g_strdup(hypergraph_pattern.str().c_str());
}

/* ------------------------------------------------------------------ */
/* Messaging                                                          */
/* ------------------------------------------------------------------ */

gboolean
gnc_send_cognitive_message(const GncCognitiveAtomMessage* message)
{
    g_return_val_if_fail(message != nullptr, FALSE);
    if (!g_atomspace) return FALSE;

    /* Bound queue */
    if (g_atomspace->message_queue.size() > 1000)
        g_atomspace->message_queue.erase(g_atomspace->message_queue.begin());

    g_atomspace->message_queue.push_back(*message);
    auto handler_it = g_atomspace->message_handlers.find(
        message->target_module ? message->target_module : "");
    if (handler_it != g_atomspace->message_handlers.end()) {
        handler_it->second(message);
        return TRUE;
    }
    return TRUE;
}

gboolean
gnc_register_cognitive_message_handler(const char* module_name,
                                       GncCognitiveMessageHandler handler_func)
{
    g_return_val_if_fail(module_name != nullptr && handler_func != nullptr, FALSE);
    if (!g_atomspace) return FALSE;
    g_atomspace->message_handlers[module_name] = handler_func;
    for (auto it = g_atomspace->message_queue.begin();
         it != g_atomspace->message_queue.end();) {
        if (it->target_module && std::string(it->target_module) == module_name) {
            handler_func(&(*it));
            it = g_atomspace->message_queue.erase(it);
        } else {
            ++it;
        }
    }
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* Emergence                                                          */
/* ------------------------------------------------------------------ */

GncAtomHandle
gnc_detect_emergent_patterns(Account** accounts, gint n_accounts,
                             const GncEmergenceParams* params)
{
    g_return_val_if_fail(accounts && n_accounts > 0 && params, 0);
    if (!g_atomspace) return 0;

    gdouble total_activity = 0.0;
    gdouble total_attention = 0.0;
    for (gint i = 0; i < n_accounts; i++) {
        if (!accounts[i]) continue;
        GncAttentionParams a = gnc_ecan_get_attention_params(accounts[i]);
        total_activity += a.activity_level;
        total_attention += a.attention_value;
    }
    gdouble complexity = total_activity / std::max(1, n_accounts);
    gdouble coherence = total_attention / std::max(1, n_accounts);
    gboolean emergent = (complexity >= params->complexity_threshold &&
                         coherence >= params->coherence_measure);

    GncAtomHandle h = g_atomspace->create_atom(
        GNC_ATOM_CONCEPT_NODE,
        emergent ? "EmergentPattern:Detected" : "EmergentPattern:None");
    g_atomspace->set_tv(h, clamp01(complexity), clamp01(coherence));
    return h;
}

GncAtomHandle
gnc_optimize_distributed_attention(gdouble cognitive_load,
                                   gdouble available_resources)
{
    if (!g_atomspace) return 0;
    gdouble efficiency = available_resources > 0.0
        ? clamp01(1.0 - cognitive_load / available_resources) : 0.0;
    GncAtomHandle h = g_atomspace->create_atom(
        GNC_ATOM_SCHEMA_NODE, "DistributedAttentionStrategy");
    g_atomspace->set_tv(h, efficiency, 0.8);
    if (efficiency < 0.5)
        g_atomspace->attention_decay_rate = std::min(0.05, g_atomspace->attention_decay_rate * 1.1);
    else
        g_atomspace->attention_decay_rate = std::max(0.005, g_atomspace->attention_decay_rate * 0.95);
    return h;
}

/* ------------------------------------------------------------------ */
/* Cognitive account types                                            */
/* ------------------------------------------------------------------ */

void
gnc_account_set_cognitive_type(Account *account, GncCognitiveAccountType cognitive_type)
{
    g_return_if_fail(account != nullptr);
    GValue v = G_VALUE_INIT;
    g_value_init(&v, G_TYPE_INT64);
    g_value_set_int64(&v, static_cast<gint64>(cognitive_type));
    qof_instance_set_kvp(QOF_INSTANCE(account), &v, 1, COGNITIVE_TYPE_KEY);
    g_value_unset(&v);

    if (!g_atomspace) return;
    GncAtomHandle h = gnc_account_to_atomspace(account);
    auto *atom = g_atomspace->get(h);
    if (!atom) return;
    auto &params = atom->attention;
    if (cognitive_type & GNC_COGNITIVE_ACCT_ADAPTIVE) {
        params.wage *= 1.2;
        params.activity_level += 0.1;
        params.lti += 10.0;
    }
    if (cognitive_type & GNC_COGNITIVE_ACCT_PREDICTIVE) {
        params.sti += 25.0;
        params.confidence = clamp01(params.confidence + 0.1);
    }
    if (cognitive_type & GNC_COGNITIVE_ACCT_MULTIMODAL) {
        params.wage *= 1.5;
        params.rent *= 1.3;
        params.vlti += 1.0;
    }
    if (cognitive_type & GNC_COGNITIVE_ACCT_ATTENTION) {
        params.sti += 50.0;
        params.lti += 25.0;
        params.activity_level += 0.3;
    }
    g_atomspace->refresh_legacy_attention(params);
}

GncCognitiveAccountType
gnc_account_get_cognitive_type(const Account *account)
{
    g_return_val_if_fail(account != nullptr, GNC_COGNITIVE_ACCT_TRADITIONAL);
    GValue v = G_VALUE_INIT;
    qof_instance_get_kvp(QOF_INSTANCE(account), &v, 1, COGNITIVE_TYPE_KEY);
    GncCognitiveAccountType t = GNC_COGNITIVE_ACCT_TRADITIONAL;
    if (G_VALUE_HOLDS_INT64(&v))
        t = static_cast<GncCognitiveAccountType>(g_value_get_int64(&v));
    else if (G_VALUE_HOLDS_UINT(&v))
        t = static_cast<GncCognitiveAccountType>(g_value_get_uint(&v));
    else if (G_VALUE_HOLDS_INT(&v))
        t = static_cast<GncCognitiveAccountType>(g_value_get_int(&v));
    if (G_IS_VALUE(&v))
        g_value_unset(&v);
    return t;
}

gboolean
gnc_account_has_cognitive_behavior(const Account *account, GncCognitiveAccountType behavior)
{
    g_return_val_if_fail(account != nullptr, FALSE);
    return (gnc_account_get_cognitive_type(account) & behavior) != 0;
}

void
gnc_account_adapt_cognitive_behavior(Account *account, const Transaction *transaction)
{
    g_return_if_fail(account && transaction);
    if (!(gnc_account_get_cognitive_type(account) & GNC_COGNITIVE_ACCT_ADAPTIVE))
        return;
    gdouble score = gnc_pln_validate_double_entry(transaction);
    if (!g_atomspace) return;
    GncAtomHandle h = gnc_account_to_atomspace(account);
    auto *atom = g_atomspace->get(h);
    if (!atom) return;
    auto &params = atom->attention;
    if (score > 0.8) {
        params.confidence = std::min(1.0, params.confidence + 0.01);
        params.lti += 1.0;
    } else if (score < 0.3) {
        params.confidence *= 0.99;
        params.sti += 5.0;
    }
    params.activity_level = params.activity_level * 0.9 + score * 0.1;
    g_atomspace->refresh_legacy_attention(params);
}

/* ------------------------------------------------------------------ */
/* Book observation / commit hook                                     */
/* ------------------------------------------------------------------ */

void
gnc_cognitive_accounting_observe_book(QofBook *book)
{
    g_return_if_fail(book != nullptr);
    if (!g_atomspace) return;
    Account *root = gnc_book_get_root_account(book);
    if (!root) return;
    GList *accts = gnc_account_get_descendants_sorted(root);
    for (GList *n = accts; n; n = n->next)
        gnc_account_to_atomspace(GNC_ACCOUNT(n->data));
    g_list_free(accts);
}

void
gnc_cognitive_accounting_on_transaction_commit(Transaction *transaction)
{
    g_return_if_fail(transaction != nullptr);
    if (!g_atomspace) return;

    gnc_transaction_to_atomspace(transaction);
    gdouble score = gnc_pln_validate_double_entry(transaction);

    GList *splits = xaccTransGetSplitList(transaction);
    for (GList *n = splits; n; n = n->next) {
        Account *acc = xaccSplitGetAccount(GNC_SPLIT(n->data));
        if (!acc) continue;
        gnc_ecan_update_account_attention(acc, transaction);
        gnc_account_adapt_cognitive_behavior(acc, transaction);
    }

    /* Notify module hub with typed atom payload */
    GncCognitiveAtomMessage msg{};
    msg.source_module = "PLN";
    msg.target_module = "ECAN";
    msg.message_type = "ValidationResult";
    msg.payload_atom = g_atomspace->transaction_atoms.count(transaction)
        ? g_atomspace->transaction_atoms[transaction] : 0;
    msg.priority = score;
    msg.timestamp = gnc_time(nullptr);
    gnc_send_cognitive_message(&msg);

    gnc_cognitive_send_message(GNC_MODULE_PLN, GNC_MODULE_ECAN,
                               GNC_MSG_DATA_UPDATE, GUINT_TO_POINTER((guint)(score * 1000)));
}

/* ------------------------------------------------------------------ */
/* AtomSpace stats                                                    */
/* ------------------------------------------------------------------ */

gboolean
gnc_cognitive_atomspace_stats(guint64 *atom_count,
                              guint64 *account_atoms,
                              guint64 *transaction_atoms,
                              gdouble *sti_funds,
                              gdouble *lti_funds)
{
    if (!g_atomspace)
        return FALSE;
    if (atom_count)
        *atom_count = static_cast<guint64>(g_atomspace->atoms.size());
    if (account_atoms)
        *account_atoms = static_cast<guint64>(g_atomspace->account_atoms.size());
    if (transaction_atoms)
        *transaction_atoms = static_cast<guint64>(g_atomspace->transaction_atoms.size());
    if (sti_funds)
        *sti_funds = g_atomspace->total_sti_funds;
    if (lti_funds)
        *lti_funds = g_atomspace->total_lti_funds;
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* UI badges / attention heat                                         */
/* ------------------------------------------------------------------ */

static gboolean g_ui_badges_enabled = FALSE;

void
gnc_cognitive_ui_set_badges_enabled(gboolean enabled)
{
    g_ui_badges_enabled = enabled ? TRUE : FALSE;
}

gboolean
gnc_cognitive_ui_badges_enabled(void)
{
    return g_ui_badges_enabled;
}

GncCognitiveBadge
gnc_cognitive_transaction_badge(const Transaction *transaction)
{
    if (!transaction)
        return GNC_COGNITIVE_BADGE_UNKNOWN;
    if (!g_atomspace)
        return GNC_COGNITIVE_BADGE_UNKNOWN;

    GncTruthValue tv{};
    if (!gnc_pln_validate_double_entry_tv(transaction, &tv))
        return GNC_COGNITIVE_BADGE_UNKNOWN;

    gdouble score = tv.strength * tv.confidence;
    if (!xaccTransIsBalanced(transaction) || score < 0.45)
        return GNC_COGNITIVE_BADGE_FAIL;
    if (score < 0.70 || tv.confidence < 0.55)
        return GNC_COGNITIVE_BADGE_WARN;
    return GNC_COGNITIVE_BADGE_OK;
}

char*
gnc_cognitive_transaction_badge_label(const Transaction *transaction)
{
    switch (gnc_cognitive_transaction_badge(transaction)) {
    case GNC_COGNITIVE_BADGE_OK: return g_strdup("OK");
    case GNC_COGNITIVE_BADGE_WARN: return g_strdup("Warn");
    case GNC_COGNITIVE_BADGE_FAIL: return g_strdup("Fail");
    case GNC_COGNITIVE_BADGE_UNKNOWN:
    default: return g_strdup("?");
    }
}

gdouble
gnc_ecan_account_sti(const Account *account)
{
    return gnc_ecan_get_attention_params(account).sti;
}

gdouble
gnc_ecan_account_lti(const Account *account)
{
    return gnc_ecan_get_attention_params(account).lti;
}

gdouble
gnc_cognitive_account_attention_heat(const Account *account)
{
    if (!account || !g_atomspace)
        return 0.0;
    GncAttentionParams p = gnc_ecan_get_attention_params(account);
    /* Soft-max style blend of STI and LTI into [0,1]. */
    gdouble raw = 0.7 * p.sti + 0.3 * p.lti;
    gdouble heat = 1.0 - std::exp(-raw / 80.0);
    if (heat < 0.0) return 0.0;
    if (heat > 1.0) return 1.0;
    return heat;
}

char*
gnc_cognitive_account_attention_css_color(const Account *account)
{
    gdouble h = gnc_cognitive_account_attention_heat(account);
    /* Cool blue (low) -> hot amber (high) */
    int r = static_cast<int>(40 + h * 200);
    int g = static_cast<int>(80 + h * 100);
    int b = static_cast<int>(200 - h * 160);
    r = std::max(0, std::min(255, r));
    g = std::max(0, std::min(255, g));
    b = std::max(0, std::min(255, b));
    return g_strdup_printf("#%02x%02x%02x", r, g, b);
}

gboolean
gnc_pln_trial_balance_balanced(const Account *root_account)
{
    GncProofReport report{};
    if (!gnc_pln_trial_balance_report(root_account, &report))
        return FALSE;
    return report.balanced;
}

/* ------------------------------------------------------------------ */
/* HTML fragments for reports                                         */
/* ------------------------------------------------------------------ */

static void
html_escape_append(std::ostringstream& ss, const char *text)
{
    if (!text) return;
    for (const char *p = text; *p; ++p) {
        switch (*p) {
        case '&': ss << "&amp;"; break;
        case '<': ss << "&lt;"; break;
        case '>': ss << "&gt;"; break;
        case '"': ss << "&quot;"; break;
        default: ss << *p; break;
        }
    }
}

char*
gnc_cognitive_html_summary_for_book(QofBook *book)
{
    std::ostringstream ss;
    ss << "<div class=\"gnc-cognitive-summary\">";
    if (!gnc_cognitive_accounting_is_initialized()) {
        ss << "<p>Cognitive accounting is not initialized.</p></div>";
        return g_strdup(ss.str().c_str());
    }

    if (book)
        gnc_cognitive_backend_sync_book(book);

    char *status = gnc_cognitive_backend_status_json();
    ss << "<p><b>Backend:</b> ";
    html_escape_append(ss, gnc_cognitive_backend_name());
    ss << " &nbsp; <b>Health:</b> "
       << (gnc_cognitive_backend_health_check() ? "OK" : "DEGRADED")
       << "</p>";
    ss << "<pre class=\"gnc-cognitive-status\">";
    html_escape_append(ss, status ? status : "{}");
    ss << "</pre></div>";
    g_free(status);
    return g_strdup(ss.str().c_str());
}

char*
gnc_cognitive_attention_table_html(QofBook *book, gint top_n)
{
    if (top_n <= 0)
        top_n = 10;
    if (top_n > 100)
        top_n = 100;

    std::ostringstream ss;
    ss << "<table class=\"gnc-cognitive-attention\">"
       << "<thead><tr><th>Account</th><th>STI</th><th>LTI</th>"
       << "<th>Heat</th><th>Color</th></tr></thead><tbody>";

    if (!gnc_cognitive_accounting_is_initialized()) {
        ss << "<tr><td colspan=\"5\">Cognitive accounting not initialized.</td></tr>"
           << "</tbody></table>";
        return g_strdup(ss.str().c_str());
    }

    if (book)
        gnc_cognitive_accounting_observe_book(book);

    std::vector<Account*> buf(static_cast<size_t>(top_n), nullptr);
    gint n = gnc_ecan_top_accounts(buf.data(), top_n);
    for (gint i = 0; i < n; ++i) {
        Account *acc = buf[static_cast<size_t>(i)];
        if (!acc) continue;
        const char *name = xaccAccountGetName(acc);
        GncAttentionParams p = gnc_ecan_get_attention_params(acc);
        gdouble heat = gnc_cognitive_account_attention_heat(acc);
        char *color = gnc_cognitive_account_attention_css_color(acc);
        ss << "<tr><td>";
        html_escape_append(ss, name ? name : "(unnamed)");
        ss << "</td><td>" << p.sti << "</td><td>" << p.lti
           << "</td><td>" << heat
           << "</td><td style=\"background:" << (color ? color : "#ccc")
           << "\">&nbsp;&nbsp;&nbsp;</td></tr>";
        g_free(color);
    }
    if (n == 0)
        ss << "<tr><td colspan=\"5\">No attention-ranked accounts yet.</td></tr>";
    ss << "</tbody></table>";
    return g_strdup(ss.str().c_str());
}

char*
gnc_cognitive_validation_summary_html(QofBook *book)
{
    std::ostringstream ss;
    ss << "<div class=\"gnc-cognitive-validation\">";

    if (!gnc_cognitive_accounting_is_initialized()) {
        ss << "<p>Cognitive accounting not initialized.</p></div>";
        return g_strdup(ss.str().c_str());
    }

    Account *root = book ? gnc_book_get_root_account(book) : nullptr;
    if (root) {
        GncProofReport report{};
        if (gnc_pln_trial_balance_report(root, &report)) {
            ss << "<p><b>Trial balance proof:</b> "
               << (report.balanced ? "balanced" : "imbalanced")
               << " (strength=" << report.strength
               << ", confidence=" << report.confidence << ")</p>";
        }
    }

    /* Sample recent mapped transactions for badge histogram */
    gint ok = 0, warn = 0, fail = 0, unknown = 0, total = 0;
    if (g_atomspace) {
        for (const auto &kv : g_atomspace->transaction_atoms) {
            const Transaction *tx = kv.first;
            if (!tx) continue;
            ++total;
            switch (gnc_cognitive_transaction_badge(tx)) {
            case GNC_COGNITIVE_BADGE_OK: ++ok; break;
            case GNC_COGNITIVE_BADGE_WARN: ++warn; break;
            case GNC_COGNITIVE_BADGE_FAIL: ++fail; break;
            default: ++unknown; break;
            }
            if (total >= 500) break; /* bound work for large books */
        }
    }
    ss << "<p><b>Transaction badges</b> (sample up to 500 mapped): "
       << "OK=" << ok << ", Warn=" << warn << ", Fail=" << fail
       << ", ?=" << unknown << ", n=" << total << "</p>";

    char *moses = gnc_moses_last_strategies_json();
    ss << "<p><b>MOSES strategies:</b></p><pre>";
    html_escape_append(ss, moses ? moses : "[]");
    ss << "</pre></div>";
    g_free(moses);
    return g_strdup(ss.str().c_str());
}
