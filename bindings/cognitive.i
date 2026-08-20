/********************************************************************\
 * cognitive.i -- SWIG bindings for cognitive accounting            *
 * Copyright (C) 2024-2026 GnuCash Cognitive Engine                 *
\********************************************************************/

/* Included from engine.i (Guile) and optionally python. */

%{
#include "gnc-cognitive-accounting.h"
#include "gnc-cognitive-backend.h"
#include "gnc-cognitive-scheme.h"
#include "gnc-tensor-network.h"
%}

%ignore GncCognitiveMessageHandler;
%ignore gnc_register_cognitive_message_handler;
%ignore gnc_cognitive_receive_messages_cpp;

%newobject gnc_moses_last_strategies_json;
%newobject gnc_account_to_scheme_representation;
%newobject gnc_transaction_to_scheme_pattern;
%newobject gnc_create_hypergraph_pattern_encoding;
%newobject gnc_cognitive_backend_status_json;
%newobject gnc_cognitive_transaction_badge_label;
%newobject gnc_cognitive_account_attention_css_color;
%newobject gnc_cognitive_html_summary_for_book;
%newobject gnc_cognitive_attention_table_html;
%newobject gnc_cognitive_validation_summary_html;
%newobject gnc_scheme_uncertain_prediction;
%newobject gnc_cognitive_scheme_eval;

%include <gnc-cognitive-accounting.h>
%include <gnc-cognitive-backend.h>

/* Scheme helpers used by (gnucash cognitive) — export-only eval. */
gboolean gnc_cognitive_scheme_init(void);
gchar* gnc_cognitive_scheme_eval(const gchar* scheme_code);
