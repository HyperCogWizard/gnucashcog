/********************************************************************\
 * gnc-tensor-network.cpp -- Distributed ggml tensor network      *
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

#include "gnc-tensor-network.h"
#include "gnc-cognitive-comms.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <math.h>
#include <string.h>

/** Static tensor network instance */
static GncTensorNetwork *g_tensor_network = NULL;
static gboolean g_tensor_network_initialized = FALSE;

/********************************************************************\
 * Tensor Network Management                                        *
\********************************************************************/

gboolean gnc_tensor_network_init(void)
{
    if (g_tensor_network_initialized) {
        return TRUE;
    }
    
    g_message("Initializing distributed ggml tensor network");
    
    g_tensor_network = gnc_tensor_network_create();
    if (!g_tensor_network) {
        g_warning("Failed to create tensor network context");
        return FALSE;
    }
    
    g_tensor_network_initialized = TRUE;
    g_message("Tensor network initialization complete");
    return TRUE;
}

void gnc_tensor_network_shutdown(void)
{
    if (!g_tensor_network_initialized) {
        return;
    }
    
    g_message("Shutting down tensor network");
    
    if (g_tensor_network) {
        gnc_tensor_network_destroy(g_tensor_network);
        g_tensor_network = NULL;
    }
    
    g_tensor_network_initialized = FALSE;
    g_message("Tensor network shutdown complete");
}

GncTensorNetwork* gnc_tensor_network_create(void)
{
    GncTensorNetwork *network = g_new0(GncTensorNetwork, 1);
    
    network->nodes = g_hash_table_new_full(g_str_hash, g_str_equal, 
                                          g_free, (GDestroyNotify)gnc_tensor_node_destroy);
    network->message_queue = g_queue_new();
    network->network_active = TRUE;
    network->total_attention = 100.0;  // Initial attention pool
    network->network_timestamp = g_get_real_time();
    
    g_message("Created tensor network context");
    return network;
}

void gnc_tensor_network_destroy(GncTensorNetwork *network)
{
    if (!network) return;
    
    g_hash_table_destroy(network->nodes);
    
    // Clear message queue
    while (!g_queue_is_empty(network->message_queue)) {
        GncTensorMessage *msg = (GncTensorMessage*)g_queue_pop_head(network->message_queue);
        g_free(msg->source_node_id);
        g_free(msg->target_node_id);
        g_free(msg->message_type);
        if (msg->payload) {
            gnc_tensor_data_destroy(msg->payload);
        }
        g_free(msg);
    }
    g_queue_free(network->message_queue);
    
    g_free(network);
}

/********************************************************************\
 * Tensor Node Operations                                           *
\********************************************************************/

GncTensorNode* gnc_tensor_node_create(GncTensorNodeType type, const gchar *node_id)
{
    GncTensorNode *node = g_new0(GncTensorNode, 1);
    
    node->type = type;
    node->node_id = g_strdup(node_id);
    node->input_tensor = NULL;
    node->output_tensor = NULL;
    node->parameters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    node->active = TRUE;
    node->attention_weight = 1.0;
    node->last_update = g_get_real_time();
    
    g_message("Created tensor node: %s (type: %d)", node_id, type);
    return node;
}

void gnc_tensor_node_destroy(GncTensorNode *node)
{
    if (!node) return;
    
    g_free(node->node_id);
    if (node->input_tensor) {
        gnc_tensor_data_destroy(node->input_tensor);
    }
    if (node->output_tensor) {
        gnc_tensor_data_destroy(node->output_tensor);
    }
    g_hash_table_destroy(node->parameters);
    g_free(node);
}

gboolean gnc_tensor_network_add_node(GncTensorNetwork *network, GncTensorNode *node)
{
    if (!network || !node) return FALSE;
    
    g_hash_table_insert(network->nodes, g_strdup(node->node_id), node);
    g_message("Added node %s to tensor network", node->node_id);
    return TRUE;
}

gboolean gnc_tensor_network_remove_node(GncTensorNetwork *network, const gchar *node_id)
{
    if (!network || !node_id) return FALSE;
    
    gboolean removed = g_hash_table_remove(network->nodes, node_id);
    if (removed) {
        g_message("Removed node %s from tensor network", node_id);
    }
    return removed;
}

GncTensorNode* gnc_tensor_network_get_node(GncTensorNetwork *network, const gchar *node_id)
{
    if (!network || !node_id) return NULL;
    
    return (GncTensorNode*)g_hash_table_lookup(network->nodes, node_id);
}

/********************************************************************\
 * Tensor Data Operations                                           *
\********************************************************************/

GncTensorData* gnc_tensor_data_create(const gchar *name, gsize n_dims, gsize *shape)
{
    GncTensorData *tensor = g_new0(GncTensorData, 1);
    
    tensor->name = g_strdup(name);
    tensor->n_dims = n_dims;
    tensor->shape = g_new(gsize, n_dims);
    memcpy(tensor->shape, shape, n_dims * sizeof(gsize));
    
    // Calculate total size
    tensor->total_size = 1;
    for (gsize i = 0; i < n_dims; i++) {
        tensor->total_size *= shape[i];
    }
    
    tensor->data = g_new0(gfloat, tensor->total_size);
    
#ifdef HAVE_GGML
    // Initialize GGML tensor if available
    // This would require a proper GGML context initialization
    tensor->ggml_tensor = NULL;  // Placeholder
#endif
    
    g_message("Created tensor '%s' with %zu dimensions, total size: %zu", 
              name, n_dims, tensor->total_size);
    return tensor;
}

void gnc_tensor_data_destroy(GncTensorData *tensor)
{
    if (!tensor) return;
    
    g_free(tensor->name);
    g_free(tensor->shape);
    g_free(tensor->data);
    
#ifdef HAVE_GGML
    // Clean up GGML tensor if needed
    if (tensor->ggml_tensor) {
        // ggml_free(tensor->ggml_tensor);
    }
#endif
    
    g_free(tensor);
}

gboolean gnc_tensor_data_from_transactions(GncTensorData *tensor, GList *transactions)
{
    if (!tensor || !transactions) return FALSE;
    
    g_message("Encoding %d transactions into tensor '%s'", 
              g_list_length(transactions), tensor->name);
    
    /* Feature schema v1 (8 dims):
     * 0 date, 1 amount_magnitude, 2 split_count, 3 validity,
     * 4 log_magnitude, 5 imbalance_flag, 6 dow, 7 normalized_imbalance
     */
    gsize tx_count = g_list_length(transactions);
    gsize feature_dim = 8;
    
    if (tensor->total_size < tx_count * feature_dim) {
        g_warning("Tensor too small for transaction data");
        return FALSE;
    }
    
    gsize idx = 0;
    for (GList *node = transactions; node; node = node->next) {
        Transaction *tx = (Transaction*)node->data;
        GList *splits = xaccTransGetSplitList(tx);
        gint split_count = g_list_length(splits);
        gdouble mag = 0.0;
        for (GList *s = splits; s; s = s->next) {
            mag += fabs(gnc_numeric_to_double(xaccSplitGetAmount(GNC_SPLIT(s->data))));
        }
        gdouble imbalance = gnc_numeric_to_double(xaccTransGetImbalanceValue(tx));
        time64 tdate = xaccTransGetDate(tx);
        gfloat dow = 0.0f;
        if (tdate > 0) {
            GDate date;
            g_date_clear(&date, 1);
            g_date_set_time_t(&date, (time_t)tdate);
            dow = (gfloat)g_date_get_weekday(&date);
        }

        tensor->data[idx++] = (gfloat)tdate;
        /* Prefer magnitude; fall back to |imbalance| for single-sided test txs */
        tensor->data[idx++] = (gfloat)(mag > 0.0 ? mag : fabs(imbalance));
        tensor->data[idx++] = (gfloat)split_count;
        /* Feature 3 "validity": successfully encoded / committed row (always 1 here) */
        tensor->data[idx++] = 1.0f;
        tensor->data[idx++] = (gfloat)log1p(mag > 0.0 ? mag : fabs(imbalance));
        tensor->data[idx++] = (fabs(imbalance) < 1e-9) ? 0.0f : 1.0f;
        tensor->data[idx++] = dow;
        tensor->data[idx++] = (gfloat)((mag > 0.0) ? (fabs(imbalance) / mag) : fabs(imbalance));
    }
    
    return TRUE;
}

gboolean gnc_tensor_data_from_accounts(GncTensorData *tensor, GList *accounts)
{
    if (!tensor || !accounts) return FALSE;
    
    g_message("Encoding %d accounts into tensor '%s'", 
              g_list_length(accounts), tensor->name);
    
    // Encode account hierarchy into tensor format
    gsize acc_count = g_list_length(accounts);
    gsize feature_dim = 6;  // Example: type, balance, depth, etc.
    
    if (tensor->total_size < acc_count * feature_dim) {
        g_warning("Tensor too small for account data");
        return FALSE;
    }
    
    gsize idx = 0;
    for (GList *node = accounts; node; node = node->next) {
        Account *acc = (Account*)node->data;
        
        // Encode account features
        time64 last_reconcile = 0;
        xaccAccountGetReconcileLastDate(acc, &last_reconcile);
        tensor->data[idx++] = (gfloat)xaccAccountGetType(acc);
        tensor->data[idx++] = (gfloat)gnc_numeric_to_double(xaccAccountGetBalance(acc));
        tensor->data[idx++] = (gfloat)gnc_account_get_current_depth(acc);
        tensor->data[idx++] = (gfloat)gnc_account_n_children(acc);
        tensor->data[idx++] = (gfloat)last_reconcile;
        tensor->data[idx++] = 1.0f;  // Account validity
    }
    
    return TRUE;
}

/* Simple k-means over row-major feature vectors.
 * input shape preferred [n_rows, n_cols]; falls back to flat length.
 * output[i] = cluster id for row i (repeated across feature dim if needed).
 */
static gboolean
gnc_tensor_kmeans(GncTensorData *input, GncTensorData *output, gint k, gint max_iter)
{
    if (!input || !output || !input->data || !output->data) return FALSE;
    if (k < 1) k = 1;
    if (max_iter < 1) max_iter = 10;

    gsize n_cols = (input->n_dims >= 2) ? input->shape[1] : 1;
    if (n_cols == 0) n_cols = 1;
    gsize n_rows = input->total_size / n_cols;
    if (n_rows == 0) return FALSE;
    if ((gsize)k > n_rows) k = (gint)n_rows;

    gfloat *centroids = g_new0(gfloat, (gsize)k * n_cols);
    gint *assign = g_new0(gint, n_rows);

    /* Init centroids to first k rows */
    for (gint c = 0; c < k; c++) {
        for (gsize j = 0; j < n_cols; j++)
            centroids[c * n_cols + j] = input->data[c * n_cols + j];
    }

    for (gint iter = 0; iter < max_iter; iter++) {
        gboolean changed = FALSE;
        for (gsize i = 0; i < n_rows; i++) {
            gfloat best = G_MAXFLOAT;
            gint best_c = 0;
            for (gint c = 0; c < k; c++) {
                gfloat dist = 0.0f;
                for (gsize j = 0; j < n_cols; j++) {
                    gfloat d = input->data[i * n_cols + j] - centroids[c * n_cols + j];
                    dist += d * d;
                }
                if (dist < best) {
                    best = dist;
                    best_c = c;
                }
            }
            if (assign[i] != best_c) {
                assign[i] = best_c;
                changed = TRUE;
            }
        }

        /* Recompute centroids */
        gfloat *sums = g_new0(gfloat, (gsize)k * n_cols);
        gint *counts = g_new0(gint, k);
        for (gsize i = 0; i < n_rows; i++) {
            gint c = assign[i];
            counts[c]++;
            for (gsize j = 0; j < n_cols; j++)
                sums[c * n_cols + j] += input->data[i * n_cols + j];
        }
        for (gint c = 0; c < k; c++) {
            if (counts[c] == 0) continue;
            for (gsize j = 0; j < n_cols; j++)
                centroids[c * n_cols + j] = sums[c * n_cols + j] / (gfloat)counts[c];
        }
        g_free(sums);
        g_free(counts);
        if (!changed) break;
    }

    for (gsize i = 0; i < output->total_size; i++) {
        gsize row = (n_rows > 0) ? (i % n_rows) : 0;
        if (row >= n_rows) row = n_rows - 1;
        output->data[i] = (gfloat)assign[row];
    }

    g_free(centroids);
    g_free(assign);
    return TRUE;
}

gboolean gnc_tensor_data_apply_clustering(GncTensorData *input, GncTensorData *output, 
                                        const gchar *clustering_algorithm)
{
    if (!input || !output || !clustering_algorithm) return FALSE;
    
    g_message("Applying %s clustering to tensor '%s'", clustering_algorithm, input->name);
    
    if (g_strcmp0(clustering_algorithm, "kmeans") == 0) {
        return gnc_tensor_kmeans(input, output, 3, 15);
    } else if (g_strcmp0(clustering_algorithm, "cogfluence") == 0) {
        return gnc_cogfluence_cluster_transactions(input, output, "enhanced");
    }
    
    return TRUE;
}

/********************************************************************\
 * Distributed Node Implementation                                  *
\********************************************************************/

gboolean gnc_tensor_node_memory_process(GncTensorNode *node, GncTensorData *input)
{
    if (!node || node->type != GNC_TENSOR_NODE_MEMORY) return FALSE;
    
    g_message("Memory node %s processing tensor input", node->node_id);
    
    // Memory node: Store transactions, clusters, states
    // Update memory tensor with new data
    if (node->input_tensor) {
        gnc_tensor_data_destroy(node->input_tensor);
    }
    
    // Store input data in memory
    gsize shape[] = {input->n_dims, input->total_size};
    node->input_tensor = gnc_tensor_data_create("memory_state", 2, shape);
    memcpy(node->input_tensor->data, input->data, 
           input->total_size * sizeof(gfloat));
    
    node->last_update = g_get_real_time();
    return TRUE;
}

gboolean gnc_tensor_node_task_process(GncTensorNode *node, GncTensorData *input)
{
    if (!node || node->type != GNC_TENSOR_NODE_TASK) return FALSE;
    (void)input;
    
    g_message("Task node %s orchestrating workflow", node->node_id);
    
    // Task node: Orchestrate workflow, trigger clustering
    // Create task parameters tensor
    gsize shape[] = {10, 1};  // Task parameters vector
    if (!node->output_tensor) {
        node->output_tensor = gnc_tensor_data_create("task_params", 2, shape);
    }
    
    // Set task parameters
    node->output_tensor->data[0] = 1.0f;  // Clustering enabled
    node->output_tensor->data[1] = 0.7f;  // Clustering threshold
    node->output_tensor->data[2] = 1.0f;  // Attention allocation enabled
    
    node->last_update = g_get_real_time();
    return TRUE;
}

gboolean gnc_tensor_node_ai_process(GncTensorNode *node, GncTensorData *input)
{
    if (!node || node->type != GNC_TENSOR_NODE_AI || !input) return FALSE;
    
    g_message("AI node %s performing pattern recognition", node->node_id);
    
    // AI node: Financial pattern recognition, clustering
    gsize n_rows = (input->n_dims >= 2 && input->shape[1] > 0)
        ? input->total_size / input->shape[1] : input->total_size;
    if (n_rows == 0) n_rows = 1;
    gsize shape[] = {n_rows, 1};  // Cluster id per row
    if (!node->output_tensor) {
        node->output_tensor = gnc_tensor_data_create("ai_clusters", 2, shape);
    }
    
    // Apply Cogfluence clustering
    gnc_tensor_data_apply_clustering(input, node->output_tensor, "cogfluence");
    
    node->last_update = g_get_real_time();
    return TRUE;
}

gboolean gnc_tensor_node_autonomy_process(GncTensorNode *node, GncTensorData *input)
{
    if (!node || node->type != GNC_TENSOR_NODE_AUTONOMY) return FALSE;
    (void)input;
    
    g_message("Autonomy node %s performing self-modification", node->node_id);
    
    // Autonomy node: Self-modification, attention allocation
    gsize shape[] = {5, 1};  // Autonomy signals
    if (!node->output_tensor) {
        node->output_tensor = gnc_tensor_data_create("autonomy_signals", 2, shape);
    }
    
    // Self-modification signals
    node->output_tensor->data[0] = node->attention_weight;
    node->output_tensor->data[1] = 0.8f;  // Adaptation factor
    node->output_tensor->data[2] = 1.2f;  // Learning rate
    node->output_tensor->data[3] = 0.9f;  // Confidence
    node->output_tensor->data[4] = 1.0f;  // Active flag
    
    node->last_update = g_get_real_time();
    return TRUE;
}

/********************************************************************\
 * Message Passing Framework                                        *
\********************************************************************/

gboolean gnc_tensor_network_send_message(GncTensorNetwork *network, 
                                        const gchar *source_id, 
                                        const gchar *target_id,
                                        const gchar *message_type,
                                        GncTensorData *payload)
{
    if (!network || !source_id || !target_id || !message_type) return FALSE;
    
    GncTensorMessage *msg = g_new0(GncTensorMessage, 1);
    msg->source_node_id = g_strdup(source_id);
    msg->target_node_id = g_strdup(target_id);
    msg->message_type = g_strdup(message_type);
    msg->payload = payload;
    msg->priority = 0.5;  // Default priority
    msg->timestamp = g_get_real_time();
    
    g_queue_push_tail(network->message_queue, msg);
    
    g_message("Message queued: %s -> %s (%s)", source_id, target_id, message_type);
    return TRUE;
}

gboolean gnc_tensor_network_process_messages(GncTensorNetwork *network)
{
    if (!network) return FALSE;
    
    gint messages_processed = 0;
    
    while (!g_queue_is_empty(network->message_queue)) {
        GncTensorMessage *msg = (GncTensorMessage*)g_queue_pop_head(network->message_queue);
        
        GncTensorNode *target = gnc_tensor_network_get_node(network, msg->target_node_id);
        if (target) {
            /* Memory/AI need payloads; task/autonomy can run control-only messages. */
            gboolean handled = FALSE;
            switch (target->type) {
                case GNC_TENSOR_NODE_MEMORY:
                    if (msg->payload)
                        handled = gnc_tensor_node_memory_process(target, msg->payload);
                    break;
                case GNC_TENSOR_NODE_TASK:
                    handled = gnc_tensor_node_task_process(target, msg->payload);
                    break;
                case GNC_TENSOR_NODE_AI:
                    if (msg->payload)
                        handled = gnc_tensor_node_ai_process(target, msg->payload);
                    else if (target->input_tensor)
                        handled = gnc_tensor_node_ai_process(target, target->input_tensor);
                    break;
                case GNC_TENSOR_NODE_AUTONOMY:
                    handled = gnc_tensor_node_autonomy_process(target, msg->payload);
                    break;
            }
            if (handled)
                messages_processed++;
        }
        
        // Clean up message
        g_free(msg->source_node_id);
        g_free(msg->target_node_id);
        g_free(msg->message_type);
        g_free(msg);
    }
    
    if (messages_processed > 0) {
        g_message("Processed %d tensor network messages", messages_processed);
    }
    
    return TRUE;
}

gboolean gnc_tensor_network_broadcast_message(GncTensorNetwork *network,
                                             const gchar *source_id,
                                             const gchar *message_type,
                                             GncTensorData *payload)
{
    if (!network || !source_id || !message_type) return FALSE;
    
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, network->nodes);
    
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        gchar *node_id = (gchar*)key;
        if (g_strcmp0(node_id, source_id) != 0) {  // Don't send to self
            gnc_tensor_network_send_message(network, source_id, node_id, 
                                          message_type, payload);
        }
    }
    
    return TRUE;
}

/********************************************************************\
 * ECAN Attention Allocation                                        *
\********************************************************************/

gboolean gnc_tensor_network_allocate_attention(GncTensorNetwork *network)
{
    if (!network) return FALSE;
    
    g_message("Allocating attention across tensor network nodes");
    
    // Calculate attention distribution based on node activity
    GHashTableIter iter;
    gpointer key, value;
    gdouble total_activity = 0.0;
    
    g_hash_table_iter_init(&iter, network->nodes);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        GncTensorNode *node = (GncTensorNode*)value;
        total_activity += node->attention_weight;
    }
    
    // Redistribute attention
    g_hash_table_iter_init(&iter, network->nodes);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        GncTensorNode *node = (GncTensorNode*)value;
        node->attention_weight = (node->attention_weight / total_activity) * 
                                network->total_attention;
    }
    
    return TRUE;
}

gboolean gnc_tensor_node_update_attention(GncTensorNode *node, gdouble activity_level)
{
    if (!node) return FALSE;
    
    // ECAN-style attention update
    gdouble sti_update = activity_level * 10.0;
    gdouble decay = 0.01;
    
    node->attention_weight = (node->attention_weight + sti_update) * (1.0 - decay);
    
    // Ensure minimum attention
    if (node->attention_weight < 0.1) {
        node->attention_weight = 0.1;
    }
    
    return TRUE;
}

gboolean gnc_tensor_network_get_attention_stats(GncTensorNetwork *network,
                                               gdouble *total_attention,
                                               gdouble *avg_attention,
                                               gdouble *max_attention)
{
    if (!network) return FALSE;
    
    gdouble total = 0.0, max = 0.0;
    gint count = 0;
    
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, network->nodes);
    
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        GncTensorNode *node = (GncTensorNode*)value;
        total += node->attention_weight;
        if (node->attention_weight > max) {
            max = node->attention_weight;
        }
        count++;
    }
    
    if (total_attention) *total_attention = total;
    if (avg_attention) *avg_attention = count > 0 ? total / count : 0.0;
    if (max_attention) *max_attention = max;
    
    return TRUE;
}

/********************************************************************\
 * Cogfluence Financial Clustering                                  *
\********************************************************************/

gboolean gnc_cogfluence_cluster_transactions(GncTensorData *transaction_tensor,
                                           GncTensorData *cluster_output,
                                           const gchar *clustering_method)
{
    if (!transaction_tensor || !cluster_output || !clustering_method) return FALSE;
    
    g_message("Applying Cogfluence clustering method: %s", clustering_method);
    
    /* enhanced = k-means with k chosen from sqrt(n_rows) */
    gsize n_cols = (transaction_tensor->n_dims >= 2) ? transaction_tensor->shape[1] : 1;
    if (n_cols == 0) n_cols = 1;
    gsize n_rows = transaction_tensor->total_size / n_cols;
    gint k = (gint)floor(sqrt((double)std::max<gsize>(1, n_rows)));
    if (k < 2) k = 2;
    if (k > 8) k = 8;
    if (g_strcmp0(clustering_method, "enhanced") != 0 &&
        g_strcmp0(clustering_method, "kmeans") != 0) {
        k = 3;
    }
    return gnc_tensor_kmeans(transaction_tensor, cluster_output, k, 20);
}

gboolean gnc_cogfluence_discover_patterns(GncTensorData *input_tensor,
                                         GncTensorData *pattern_output,
                                         gdouble pattern_threshold)
{
    if (!input_tensor || !pattern_output) return FALSE;
    
    g_message("Discovering emergent patterns with threshold: %f", pattern_threshold);
    
    /* Z-score style anomaly flags relative to mean/stdev */
    gdouble sum = 0.0, sumsq = 0.0;
    for (gsize i = 0; i < input_tensor->total_size; i++) {
        sum += input_tensor->data[i];
        sumsq += input_tensor->data[i] * input_tensor->data[i];
    }
    gdouble mean = sum / std::max<gsize>(1, input_tensor->total_size);
    gdouble var = sumsq / std::max<gsize>(1, input_tensor->total_size) - mean * mean;
    if (var < 0.0) var = 0.0;
    gdouble stdev = sqrt(var);
    if (stdev < 1e-9) stdev = 1.0;

    for (gsize i = 0; i < pattern_output->total_size; i++) {
        gfloat input_val = input_tensor->data[i % input_tensor->total_size];
        gdouble z = fabs((input_val - mean) / stdev);
        pattern_output->data[i] = (z > pattern_threshold) ? (gfloat)z : 0.0f;
    }
    
    return TRUE;
}

gboolean gnc_cogfluence_generate_insights(GncTensorData *cluster_data,
                                         GHashTable *insights)
{
    if (!cluster_data || !insights) return FALSE;
    
    g_message("Generating financial insights from clustered data");
    
    gdouble avg_cluster = 0.0;
    gdouble max_cluster = 0.0;
    gint distinct = 0;
    gboolean seen[32] = { FALSE };
    
    for (gsize i = 0; i < cluster_data->total_size; i++) {
        avg_cluster += cluster_data->data[i];
        if (cluster_data->data[i] > max_cluster)
            max_cluster = cluster_data->data[i];
        gint id = (gint)lroundf(cluster_data->data[i]);
        if (id >= 0 && id < 32 && !seen[id]) {
            seen[id] = TRUE;
            distinct++;
        }
    }
    if (cluster_data->total_size > 0)
        avg_cluster /= cluster_data->total_size;
    
    g_hash_table_insert(insights, g_strdup("avg_cluster"), g_strdup_printf("%f", avg_cluster));
    g_hash_table_insert(insights, g_strdup("max_cluster"), g_strdup_printf("%f", max_cluster));
    g_hash_table_insert(insights, g_strdup("cluster_count"), g_strdup_printf("%zu", cluster_data->total_size));
    g_hash_table_insert(insights, g_strdup("distinct_clusters"), g_strdup_printf("%d", distinct));
    g_hash_table_insert(insights, g_strdup("schema_version"), g_strdup("1"));
    
    return TRUE;
}

/********************************************************************\
 * Network Synchronization                                          *
\********************************************************************/

gboolean gnc_tensor_network_synchronize(GncTensorNetwork *network)
{
    if (!network) return FALSE;
    
    g_message("Synchronizing tensor network nodes");
    
    // Update network timestamp
    network->network_timestamp = g_get_real_time();
    
    // Send sync message to all nodes
    gsize sync_shape[1] = {1};
    GncTensorData *sync_payload = gnc_tensor_data_create("sync", 1, sync_shape);
    sync_payload->data[0] = (gfloat)network->network_timestamp;
    
    gnc_tensor_network_broadcast_message(network, "network", "sync", sync_payload);
    gnc_tensor_network_process_messages(network);
    
    return TRUE;
}

gboolean gnc_tensor_network_update_timestamp(GncTensorNetwork *network)
{
    if (!network) return FALSE;
    
    network->network_timestamp = g_get_real_time();
    return TRUE;
}

gboolean gnc_tensor_network_health_check(GncTensorNetwork *network)
{
    if (!network) return FALSE;
    
    gint active_nodes = 0;
    gint total_nodes = g_hash_table_size(network->nodes);
    
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, network->nodes);
    
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        GncTensorNode *node = (GncTensorNode*)value;
        if (node->active) {
            active_nodes++;
        }
    }
    
    g_message("Network health: %d/%d nodes active", active_nodes, total_nodes);
    
    /* Healthy when the network is marked active. Idle (zero active nodes)
     * is allowed while the network remains structurally active. */
    return network->network_active;
}