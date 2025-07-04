/********************************************************************\
 * gnc-tensor-network.h -- Distributed ggml tensor network        *
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

/** @addtogroup Engine
    @{ */
/** @addtogroup TensorNetwork
    Distributed ggml tensor network for agentic cognitive processing.
    Each component (Memory, Task, AI, Autonomy) is implemented as a node
    in a tensor network with message passing capabilities.
    @{ */

/** @file gnc-tensor-network.h
    @brief Distributed ggml tensor network architecture
    @author Copyright (C) 2024 GnuCash Cognitive Engine
*/

#ifndef GNC_TENSOR_NETWORK_H
#define GNC_TENSOR_NETWORK_H

#include "gnc-cognitive-accounting.h"
#include "qof.h"

#ifdef HAVE_GGML
#include <ggml.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @name Tensor Network Node Types */
/** @{ */

/** Node types in the distributed tensor network */
typedef enum {
    GNC_TENSOR_NODE_MEMORY,     /**< Memory node - stores transactions, clusters, states */
    GNC_TENSOR_NODE_TASK,       /**< Task node - orchestrates workflow, triggers clustering */
    GNC_TENSOR_NODE_AI,         /**< AI node - financial pattern recognition, clustering */
    GNC_TENSOR_NODE_AUTONOMY    /**< Autonomy node - self-modification, attention allocation */
} GncTensorNodeType;

/** Tensor data structure for financial data */
typedef struct {
    gfloat *data;               /**< Raw tensor data */
    gsize *shape;               /**< Tensor dimensions */
    gsize n_dims;               /**< Number of dimensions */
    gsize total_size;           /**< Total number of elements */
    gchar *name;                /**< Tensor name/identifier */
#ifdef HAVE_GGML
    struct ggml_tensor *ggml_tensor;  /**< GGML tensor handle */
#endif
} GncTensorData;

/** Tensor network node structure */
typedef struct {
    GncTensorNodeType type;     /**< Node type */
    gchar *node_id;             /**< Unique node identifier */
    GncTensorData *input_tensor; /**< Input tensor data */
    GncTensorData *output_tensor; /**< Output tensor data */
    GHashTable *parameters;     /**< Node parameters */
    gboolean active;            /**< Node active state */
    gdouble attention_weight;   /**< ECAN attention weight */
    gint64 last_update;         /**< Last update timestamp */
} GncTensorNode;

/** Message structure for inter-node communication */
typedef struct {
    gchar *source_node_id;      /**< Source node identifier */
    gchar *target_node_id;      /**< Target node identifier */
    gchar *message_type;        /**< Message type */
    GncTensorData *payload;     /**< Tensor payload */
    gdouble priority;           /**< Message priority */
    gint64 timestamp;           /**< Message timestamp */
} GncTensorMessage;

/** Tensor network context */
typedef struct {
    GHashTable *nodes;          /**< Map of node_id -> GncTensorNode */
    GQueue *message_queue;      /**< Message queue for async processing */
    gboolean network_active;    /**< Network active state */
    gdouble total_attention;    /**< Total attention resources */
    gint64 network_timestamp;   /**< Network synchronization timestamp */
} GncTensorNetwork;

/** @} */

/** @name Tensor Network Management */
/** @{ */

/** Initialize the tensor network system */
gboolean gnc_tensor_network_init(void);

/** Shutdown the tensor network system */
void gnc_tensor_network_shutdown(void);

/** Create a new tensor network context */
GncTensorNetwork* gnc_tensor_network_create(void);

/** Destroy a tensor network context */
void gnc_tensor_network_destroy(GncTensorNetwork *network);

/** @} */

/** @name Tensor Node Operations */
/** @{ */

/** Create a new tensor node */
GncTensorNode* gnc_tensor_node_create(GncTensorNodeType type, const gchar *node_id);

/** Destroy a tensor node */
void gnc_tensor_node_destroy(GncTensorNode *node);

/** Add a node to the tensor network */
gboolean gnc_tensor_network_add_node(GncTensorNetwork *network, GncTensorNode *node);

/** Remove a node from the tensor network */
gboolean gnc_tensor_network_remove_node(GncTensorNetwork *network, const gchar *node_id);

/** Get a node from the tensor network */
GncTensorNode* gnc_tensor_network_get_node(GncTensorNetwork *network, const gchar *node_id);

/** @} */

/** @name Tensor Data Operations */
/** @{ */

/** Create a new tensor data structure */
GncTensorData* gnc_tensor_data_create(const gchar *name, gsize n_dims, gsize *shape);

/** Destroy a tensor data structure */
void gnc_tensor_data_destroy(GncTensorData *tensor);

/** Set tensor data from financial transactions */
gboolean gnc_tensor_data_from_transactions(GncTensorData *tensor, GList *transactions);

/** Set tensor data from account hierarchy */
gboolean gnc_tensor_data_from_accounts(GncTensorData *tensor, GList *accounts);

/** Apply clustering to tensor data */
gboolean gnc_tensor_data_apply_clustering(GncTensorData *input, GncTensorData *output, 
                                         const gchar *clustering_algorithm);

/** @} */

/** @name Distributed Node Implementation */
/** @{ */

/** Memory Node - Stores transactions, clusters, states */
gboolean gnc_tensor_node_memory_process(GncTensorNode *node, GncTensorData *input);

/** Task Node - Orchestrates workflow, triggers clustering */
gboolean gnc_tensor_node_task_process(GncTensorNode *node, GncTensorData *input);

/** AI Node - Financial pattern recognition, clustering */
gboolean gnc_tensor_node_ai_process(GncTensorNode *node, GncTensorData *input);

/** Autonomy Node - Self-modification, attention allocation */
gboolean gnc_tensor_node_autonomy_process(GncTensorNode *node, GncTensorData *input);

/** @} */

/** @name Message Passing Framework */
/** @{ */

/** Send a message between nodes */
gboolean gnc_tensor_network_send_message(GncTensorNetwork *network, 
                                        const gchar *source_id, 
                                        const gchar *target_id,
                                        const gchar *message_type,
                                        GncTensorData *payload);

/** Process pending messages in the network */
gboolean gnc_tensor_network_process_messages(GncTensorNetwork *network);

/** Broadcast a message to all nodes */
gboolean gnc_tensor_network_broadcast_message(GncTensorNetwork *network,
                                             const gchar *source_id,
                                             const gchar *message_type,
                                             GncTensorData *payload);

/** @} */

/** @name ECAN Attention Allocation */
/** @{ */

/** Allocate attention resources across nodes */
gboolean gnc_tensor_network_allocate_attention(GncTensorNetwork *network);

/** Update node attention weights based on activity */
gboolean gnc_tensor_node_update_attention(GncTensorNode *node, gdouble activity_level);

/** Get network attention statistics */
gboolean gnc_tensor_network_get_attention_stats(GncTensorNetwork *network,
                                               gdouble *total_attention,
                                               gdouble *avg_attention,
                                               gdouble *max_attention);

/** @} */

/** @name Cogfluence Financial Clustering */
/** @{ */

/** Apply Cogfluence clustering paradigms to financial data */
gboolean gnc_cogfluence_cluster_transactions(GncTensorData *transaction_tensor,
                                           GncTensorData *cluster_output,
                                           const gchar *clustering_method);

/** Discover emergent economic patterns */
gboolean gnc_cogfluence_discover_patterns(GncTensorData *input_tensor,
                                         GncTensorData *pattern_output,
                                         gdouble pattern_threshold);

/** Generate financial insights from clustered data */
gboolean gnc_cogfluence_generate_insights(GncTensorData *cluster_data,
                                         GHashTable *insights);

/** @} */

/** @name Network Synchronization */
/** @{ */

/** Synchronize all nodes in the network */
gboolean gnc_tensor_network_synchronize(GncTensorNetwork *network);

/** Update network timestamp */
gboolean gnc_tensor_network_update_timestamp(GncTensorNetwork *network);

/** Check network health and connectivity */
gboolean gnc_tensor_network_health_check(GncTensorNetwork *network);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* GNC_TENSOR_NETWORK_H */

/** @} */
/** @} */