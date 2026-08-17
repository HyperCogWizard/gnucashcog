/********************************************************************\
 * gnc-cognitive-comms.cpp -- Inter-module communication hub        *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

#include "gnc-cognitive-comms.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <vector>

struct GncCognitiveCommHub {
    std::map<GncCognitiveModule, std::vector<GncCognitiveModuleMessage>> module_queues;
    std::map<GncCognitiveModule, gboolean> module_status;
    std::vector<GncCognitivePattern> active_patterns;
    static constexpr size_t MAX_QUEUE = 500;
    static constexpr size_t MAX_PATTERNS = 200;

    GncCognitiveCommHub()
    {
        module_status[GNC_MODULE_ATOMSPACE] = FALSE;
        module_status[GNC_MODULE_PLN] = FALSE;
        module_status[GNC_MODULE_ECAN] = FALSE;
        module_status[GNC_MODULE_MOSES] = FALSE;
        module_status[GNC_MODULE_URE] = FALSE;
        module_status[GNC_MODULE_COGSERVER] = FALSE;
        module_status[GNC_MODULE_SCHEME] = FALSE;
    }
};

static std::unique_ptr<GncCognitiveCommHub> g_comm_hub;

gboolean
gnc_cognitive_comms_init(void)
{
    if (g_comm_hub) {
        g_warning("Cognitive communications already initialized");
        return FALSE;
    }
    g_comm_hub = std::make_unique<GncCognitiveCommHub>();
    g_message("Cognitive communication hub initialized");
    return TRUE;
}

void
gnc_cognitive_comms_shutdown(void)
{
    if (!g_comm_hub) {
        g_warning("Cognitive communications not initialized");
        return;
    }
    g_comm_hub.reset();
    g_message("Cognitive communication hub shutdown");
}

const gchar*
gnc_cognitive_module_name(GncCognitiveModule module)
{
    switch (module) {
    case GNC_MODULE_ATOMSPACE: return "AtomSpace";
    case GNC_MODULE_PLN: return "PLN";
    case GNC_MODULE_ECAN: return "ECAN";
    case GNC_MODULE_MOSES: return "MOSES";
    case GNC_MODULE_URE: return "URE";
    case GNC_MODULE_COGSERVER: return "CogServer";
    case GNC_MODULE_SCHEME: return "Scheme";
    default: return "Unknown";
    }
}

gboolean
gnc_cognitive_register_module(GncCognitiveModule module)
{
    if (!g_comm_hub) return FALSE;
    g_comm_hub->module_status[module] = TRUE;
    g_message("Registered cognitive module: %s", gnc_cognitive_module_name(module));
    gnc_cognitive_trigger_emergence(module);
    return TRUE;
}

void
gnc_cognitive_send_message(GncCognitiveModule from_module,
                           GncCognitiveModule to_module,
                           GncCognitiveMessageType msg_type,
                           gpointer data)
{
    if (!g_comm_hub) return;
    GncCognitiveModuleMessage msg;
    msg.from_module = from_module;
    msg.to_module = to_module;
    msg.message_type = msg_type;
    msg.data = data;
    msg.timestamp = g_get_real_time();
    auto &q = g_comm_hub->module_queues[to_module];
    if (q.size() >= GncCognitiveCommHub::MAX_QUEUE)
        q.erase(q.begin());
    q.push_back(msg);
    gnc_cognitive_process_emergent_patterns();
}

GArray*
gnc_cognitive_receive_messages(GncCognitiveModule module)
{
    GArray *arr = g_array_new(FALSE, FALSE, sizeof(GncCognitiveModuleMessage));
    if (!g_comm_hub) return arr;
    auto &queue = g_comm_hub->module_queues[module];
    for (const auto &m : queue)
        g_array_append_val(arr, m);
    queue.clear();
    return arr;
}

std::vector<GncCognitiveModuleMessage>
gnc_cognitive_receive_messages_cpp(GncCognitiveModule module)
{
    std::vector<GncCognitiveModuleMessage> out;
    if (!g_comm_hub) return out;
    auto &queue = g_comm_hub->module_queues[module];
    out.swap(queue);
    return out;
}

void
gnc_cognitive_broadcast_message(GncCognitiveModule from_module,
                                GncCognitiveMessageType msg_type,
                                gpointer data)
{
    if (!g_comm_hub) return;
    for (auto &pair : g_comm_hub->module_status) {
        if (pair.second && pair.first != from_module)
            gnc_cognitive_send_message(from_module, pair.first, msg_type, data);
    }
}

void
gnc_cognitive_trigger_emergence(GncCognitiveModule trigger_module)
{
    if (!g_comm_hub) return;
    GncCognitivePattern pattern;
    pattern.trigger_module = trigger_module;
    pattern.pattern_type = GNC_PATTERN_ACTIVATION;
    pattern.strength = 0.7;
    pattern.timestamp = g_get_real_time();
    if (g_comm_hub->active_patterns.size() >= GncCognitiveCommHub::MAX_PATTERNS)
        g_comm_hub->active_patterns.erase(g_comm_hub->active_patterns.begin());
    g_comm_hub->active_patterns.push_back(pattern);
    gnc_cognitive_broadcast_message(trigger_module, GNC_MSG_EMERGENCE_ACTIVATION, nullptr);
}

void
gnc_cognitive_process_emergent_patterns(void)
{
    if (!g_comm_hub) return;
    for (auto &p : g_comm_hub->active_patterns)
        p.strength *= 0.99;
    g_comm_hub->active_patterns.erase(
        std::remove_if(g_comm_hub->active_patterns.begin(),
                       g_comm_hub->active_patterns.end(),
                       [](const GncCognitivePattern &p) { return p.strength < 0.05; }),
        g_comm_hub->active_patterns.end());
}

void
gnc_cognitive_amplify_pattern(const GncCognitivePattern* pattern)
{
    if (!g_comm_hub || !pattern) return;
    GncCognitivePattern p = *pattern;
    p.strength = std::min(1.0, p.strength * 1.2);
    p.timestamp = g_get_real_time();
    g_comm_hub->active_patterns.push_back(p);
}

void
gnc_cognitive_detect_emergent_insights(void)
{
    if (!g_comm_hub) return;
    for (const auto &p : g_comm_hub->active_patterns) {
        if (p.strength > 0.8)
            gnc_cognitive_generate_insight(&p);
    }
}

void
gnc_cognitive_generate_insight(const GncCognitivePattern* pattern)
{
    if (!pattern) return;
    g_debug("Cognitive insight from module %s strength=%.3f",
            gnc_cognitive_module_name(pattern->trigger_module),
            pattern->strength);
}

void
gnc_cognitive_optimize_attention_flow(void)
{
    if (!g_comm_hub) return;
    gnc_cognitive_broadcast_message(GNC_MODULE_ECAN, GNC_MSG_ATTENTION_REALLOCATION, nullptr);
}

void
gnc_cognitive_synchronize_modules(void)
{
    if (!g_comm_hub) return;
    gnc_cognitive_broadcast_message(GNC_MODULE_ATOMSPACE, GNC_MSG_SYNCHRONIZATION, nullptr);
}
