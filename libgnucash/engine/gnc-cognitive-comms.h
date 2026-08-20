/********************************************************************\
 * gnc-cognitive-comms.h -- Inter-module communication protocols   *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

#ifndef GNC_COGNITIVE_COMMS_H
#define GNC_COGNITIVE_COMMS_H

#include "gnc-engine.h"
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GNC_MODULE_ATOMSPACE = 0,
    GNC_MODULE_PLN,
    GNC_MODULE_ECAN,
    GNC_MODULE_MOSES,
    GNC_MODULE_URE,
    GNC_MODULE_COGSERVER,
    GNC_MODULE_SCHEME
} GncCognitiveModule;

typedef enum {
    GNC_MSG_DATA_UPDATE = 0,
    GNC_MSG_ATTENTION_REQUEST,
    GNC_MSG_ATTENTION_REALLOCATION,
    GNC_MSG_PATTERN_MATCH,
    GNC_MSG_PATTERN_RESONANCE,
    GNC_MSG_INFERENCE_REQUEST,
    GNC_MSG_EMERGENCE_ACTIVATION,
    GNC_MSG_SYNCHRONIZATION
} GncCognitiveMessageType;

typedef enum {
    GNC_PATTERN_ACTIVATION = 0,
    GNC_PATTERN_RESONANCE,
    GNC_PATTERN_EMERGENCE,
    GNC_PATTERN_ATTENTION
} GncCognitivePatternType;

/** Module-hub message (enum-routed). Distinct from GncCognitiveAtomMessage. */
typedef struct {
    GncCognitiveModule from_module;
    GncCognitiveModule to_module;
    GncCognitiveMessageType message_type;
    gpointer data;
    gint64 timestamp;
} GncCognitiveModuleMessage;

typedef struct {
    GncCognitiveModule trigger_module;
    GncCognitivePatternType pattern_type;
    gdouble strength;
    gint64 timestamp;
} GncCognitivePattern;

gboolean gnc_cognitive_comms_init(void);
void gnc_cognitive_comms_shutdown(void);
gboolean gnc_cognitive_register_module(GncCognitiveModule module);

void gnc_cognitive_send_message(GncCognitiveModule from_module,
                               GncCognitiveModule to_module,
                               GncCognitiveMessageType msg_type,
                               gpointer data);

/**
 * Drain pending module messages into a caller-owned GArray of
 * GncCognitiveModuleMessage. Caller must g_array_free(..., TRUE).
 * Returns empty array if none.
 */
GArray* gnc_cognitive_receive_messages(GncCognitiveModule module);

void gnc_cognitive_broadcast_message(GncCognitiveModule from_module,
                                    GncCognitiveMessageType msg_type,
                                    gpointer data);

void gnc_cognitive_trigger_emergence(GncCognitiveModule trigger_module);
void gnc_cognitive_process_emergent_patterns(void);
void gnc_cognitive_amplify_pattern(const GncCognitivePattern* pattern);
void gnc_cognitive_detect_emergent_insights(void);
void gnc_cognitive_generate_insight(const GncCognitivePattern* pattern);
void gnc_cognitive_optimize_attention_flow(void);
void gnc_cognitive_synchronize_modules(void);
const gchar* gnc_cognitive_module_name(GncCognitiveModule module);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <vector>
/** C++ convenience wrapper around gnc_cognitive_receive_messages. */
std::vector<GncCognitiveModuleMessage>
gnc_cognitive_receive_messages_cpp(GncCognitiveModule module);
#endif

#endif /* GNC_COGNITIVE_COMMS_H */
