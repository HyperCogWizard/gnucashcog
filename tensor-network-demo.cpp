/********************************************************************\
 * tensor-network-demo.cpp -- Demonstration of tensor network      *
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

#include <iostream>
#include <iomanip>
#include "gnc-tensor-network.h"
#include "gnc-cognitive-accounting.h"
#include "Account.h"
#include "Transaction.h"
#include "qofbook.h"
#include "test-engine-stuff.h"

void demonstrate_tensor_network_architecture()
{
    std::cout << "\n=== Distributed Tensor Network Architecture Demo ===" << std::endl;
    std::cout << "Creating agentic cognitive system with Memory, Task, AI, and Autonomy nodes..." << std::endl;
    
    // Initialize tensor network
    if (!gnc_tensor_network_init()) {
        std::cerr << "Failed to initialize tensor network!" << std::endl;
        return;
    }
    
    // Create tensor network
    GncTensorNetwork* network = gnc_tensor_network_create();
    if (!network) {
        std::cerr << "Failed to create tensor network!" << std::endl;
        return;
    }
    
    // Create distributed nodes
    GncTensorNode* memory_node = gnc_tensor_node_create(GNC_TENSOR_NODE_MEMORY, "memory_membrane");
    GncTensorNode* task_node = gnc_tensor_node_create(GNC_TENSOR_NODE_TASK, "task_orchestrator");
    GncTensorNode* ai_node = gnc_tensor_node_create(GNC_TENSOR_NODE_AI, "ai_pattern_recognizer");
    GncTensorNode* autonomy_node = gnc_tensor_node_create(GNC_TENSOR_NODE_AUTONOMY, "autonomy_controller");
    
    // Add nodes to network
    gnc_tensor_network_add_node(network, memory_node);
    gnc_tensor_network_add_node(network, task_node);
    gnc_tensor_network_add_node(network, ai_node);
    gnc_tensor_network_add_node(network, autonomy_node);
    
    std::cout << "✓ Created distributed tensor network with 4 cognitive nodes" << std::endl;
    std::cout << "  • Memory Node: Stores transactions, clusters, states" << std::endl;
    std::cout << "  • Task Node: Orchestrates workflow, triggers clustering" << std::endl;
    std::cout << "  • AI Node: Financial pattern recognition, clustering" << std::endl;
    std::cout << "  • Autonomy Node: Self-modification, attention allocation" << std::endl;
    
    // Display network statistics
    gdouble total_attention, avg_attention, max_attention;
    gnc_tensor_network_get_attention_stats(network, &total_attention, &avg_attention, &max_attention);
    
    std::cout << "\nTensor Network Statistics:" << std::endl;
    std::cout << "  Total Attention: " << std::fixed << std::setprecision(2) << total_attention << std::endl;
    std::cout << "  Average Attention: " << avg_attention << std::endl;
    std::cout << "  Max Attention: " << max_attention << std::endl;
    
    gnc_tensor_network_destroy(network);
    gnc_tensor_network_shutdown();
}

void demonstrate_tensor_data_encoding()
{
    std::cout << "\n=== Tensor Data Encoding Demo ===" << std::endl;
    std::cout << "Encoding financial transactions into ggml tensor format..." << std::endl;
    
    // Create test financial data
    QofBook* book = qof_book_new();
    
    // Create account hierarchy
    Account* root = gnc_account_create_root(book);
    Account* assets = xaccMallocAccount(book);
    Account* checking = xaccMallocAccount(book);
    Account* expenses = xaccMallocAccount(book);
    Account* food = xaccMallocAccount(book);
    
    xaccAccountSetName(assets, "Assets");
    xaccAccountSetType(assets, ACCT_TYPE_ASSET);
    gnc_account_append_child(root, assets);
    
    xaccAccountSetName(checking, "Checking");
    xaccAccountSetType(checking, ACCT_TYPE_BANK);
    gnc_account_append_child(assets, checking);
    
    xaccAccountSetName(expenses, "Expenses");
    xaccAccountSetType(expenses, ACCT_TYPE_EXPENSE);
    gnc_account_append_child(root, expenses);
    
    xaccAccountSetName(food, "Food");
    xaccAccountSetType(food, ACCT_TYPE_EXPENSE);
    gnc_account_append_child(expenses, food);
    
    // Create transactions
    GList* transactions = nullptr;
    
    for (int i = 0; i < 5; i++) {
        Transaction* trans = xaccMallocTransaction(book);
        xaccTransSetDatePostedSecs(trans, gnc_time(nullptr) + i * 86400);
        xaccTransSetDescription(trans, g_strdup_printf("Transaction %d", i + 1));
        
        // Create splits
        Split* split1 = xaccMallocSplit(book);
        Split* split2 = xaccMallocSplit(book);
        
        xaccSplitSetAccount(split1, checking);
        xaccSplitSetValue(split1, gnc_numeric_create(-(50 + i * 10), 1));
        xaccSplitSetAmount(split1, gnc_numeric_create(-(50 + i * 10), 1));
        xaccTransAppendSplit(trans, split1);
        
        xaccSplitSetAccount(split2, food);
        xaccSplitSetValue(split2, gnc_numeric_create(50 + i * 10, 1));
        xaccSplitSetAmount(split2, gnc_numeric_create(50 + i * 10, 1));
        xaccTransAppendSplit(trans, split2);
        
        xaccTransCommitEdit(trans);
        transactions = g_list_append(transactions, trans);
    }
    
    std::cout << "✓ Created " << g_list_length(transactions) << " test transactions" << std::endl;
    
    // Encode transactions into tensor
    gsize shape[] = {g_list_length(transactions), 8};  // 5 transactions, 8 features each
    GncTensorData* tx_tensor = gnc_tensor_data_create("transaction_tensor", 2, shape);
    
    if (gnc_tensor_data_from_transactions(tx_tensor, transactions)) {
        std::cout << "✓ Successfully encoded transactions into tensor" << std::endl;
        std::cout << "  Tensor shape: [" << shape[0] << ", " << shape[1] << "]" << std::endl;
        std::cout << "  Total elements: " << tx_tensor->total_size << std::endl;
        
        // Display sample tensor data
        std::cout << "  Sample tensor data:" << std::endl;
        for (gsize i = 0; i < 3 && i < tx_tensor->total_size; i++) {
            std::cout << "    data[" << i << "] = " << tx_tensor->data[i] << std::endl;
        }
    }
    
    gnc_tensor_data_destroy(tx_tensor);
    g_list_free(transactions);
    qof_book_destroy(book);
}

void demonstrate_cogfluence_clustering()
{
    std::cout << "\n=== Cogfluence Financial Clustering Demo ===" << std::endl;
    std::cout << "Applying Cogfluence paradigms for emergent economic pattern discovery..." << std::endl;
    
    // Create test financial data tensor
    gsize shape[] = {20, 4};  // 20 data points, 4 features
    GncTensorData* financial_tensor = gnc_tensor_data_create("financial_data", 2, shape);
    GncTensorData* cluster_tensor = gnc_tensor_data_create("cluster_results", 2, shape);
    
    // Generate synthetic financial data
    for (gsize i = 0; i < financial_tensor->total_size; i++) {
        // Create patterns that resemble financial data
        gfloat base_value = (gfloat)i;
        gfloat seasonal = sinf(base_value * 0.1f) * 50.0f;
        gfloat trend = base_value * 0.5f;
        gfloat noise = (gfloat)(rand() % 20 - 10);
        
        financial_tensor->data[i] = base_value + seasonal + trend + noise;
    }
    
    std::cout << "✓ Generated synthetic financial data with patterns" << std::endl;
    
    // Apply Cogfluence clustering
    if (gnc_cogfluence_cluster_transactions(financial_tensor, cluster_tensor, "enhanced")) {
        std::cout << "✓ Applied Cogfluence enhanced clustering algorithm" << std::endl;
        
        // Discover emergent patterns
        GncTensorData* pattern_tensor = gnc_tensor_data_create("patterns", 2, shape);
        if (gnc_cogfluence_discover_patterns(cluster_tensor, pattern_tensor, 0.5)) {
            std::cout << "✓ Discovered emergent economic patterns" << std::endl;
            
            // Count significant patterns
            gint pattern_count = 0;
            for (gsize i = 0; i < pattern_tensor->total_size; i++) {
                if (pattern_tensor->data[i] > 0.0f) {
                    pattern_count++;
                }
            }
            std::cout << "  Significant patterns found: " << pattern_count << std::endl;
        }
        
        // Generate insights
        GHashTable* insights = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        if (gnc_cogfluence_generate_insights(cluster_tensor, insights)) {
            std::cout << "✓ Generated financial insights from clustered data" << std::endl;
            
            // Display insights
            if (g_hash_table_contains(insights, "avg_cluster")) {
                gchar* avg_str = (gchar*)g_hash_table_lookup(insights, "avg_cluster");
                std::cout << "  Average cluster value: " << avg_str << std::endl;
            }
            if (g_hash_table_contains(insights, "max_cluster")) {
                gchar* max_str = (gchar*)g_hash_table_lookup(insights, "max_cluster");
                std::cout << "  Maximum cluster value: " << max_str << std::endl;
            }
        }
        
        g_hash_table_destroy(insights);
        gnc_tensor_data_destroy(pattern_tensor);
    }
    
    gnc_tensor_data_destroy(financial_tensor);
    gnc_tensor_data_destroy(cluster_tensor);
}

void demonstrate_distributed_message_passing()
{
    std::cout << "\n=== Distributed Message Passing Demo ===" << std::endl;
    std::cout << "Demonstrating inter-node communication and cognitive workflow..." << std::endl;
    
    gnc_tensor_network_init();
    GncTensorNetwork* network = gnc_tensor_network_create();
    
    // Create cognitive nodes
    GncTensorNode* memory_node = gnc_tensor_node_create(GNC_TENSOR_NODE_MEMORY, "memory");
    GncTensorNode* task_node = gnc_tensor_node_create(GNC_TENSOR_NODE_TASK, "task");
    GncTensorNode* ai_node = gnc_tensor_node_create(GNC_TENSOR_NODE_AI, "ai");
    GncTensorNode* autonomy_node = gnc_tensor_node_create(GNC_TENSOR_NODE_AUTONOMY, "autonomy");
    
    gnc_tensor_network_add_node(network, memory_node);
    gnc_tensor_network_add_node(network, task_node);
    gnc_tensor_network_add_node(network, ai_node);
    gnc_tensor_network_add_node(network, autonomy_node);
    
    // Create test data
    gsize shape[] = {10, 4};
    GncTensorData* test_data = gnc_tensor_data_create("test_data", 2, shape);
    for (gsize i = 0; i < test_data->total_size; i++) {
        test_data->data[i] = (gfloat)i * 0.5f;
    }
    
    std::cout << "✓ Created distributed cognitive network" << std::endl;
    
    // Demonstrate message passing workflow
    std::cout << "\nExecuting distributed cognitive workflow:" << std::endl;
    
    // Step 1: Send data to memory node
    std::cout << "1. Sending financial data to Memory node..." << std::endl;
    gnc_tensor_network_send_message(network, "input", "memory", "store_data", test_data);
    
    // Step 2: Memory requests task orchestration
    std::cout << "2. Memory node requesting task orchestration..." << std::endl;
    gnc_tensor_network_send_message(network, "memory", "task", "orchestrate", nullptr);
    
    // Step 3: Task node requests AI analysis
    std::cout << "3. Task node requesting AI analysis..." << std::endl;
    gnc_tensor_network_send_message(network, "task", "ai", "analyze_patterns", nullptr);
    
    // Step 4: AI node sends results to autonomy
    std::cout << "4. AI node sending results to Autonomy node..." << std::endl;
    gnc_tensor_network_send_message(network, "ai", "autonomy", "update_attention", nullptr);
    
    // Process all messages
    std::cout << "5. Processing distributed messages..." << std::endl;
    gnc_tensor_network_process_messages(network);
    
    std::cout << "✓ Distributed cognitive workflow completed" << std::endl;
    
    // Test broadcast communication
    std::cout << "\nTesting broadcast synchronization:" << std::endl;
    GncTensorData* sync_data = gnc_tensor_data_create("sync", 1, (gsize[]){1});
    sync_data->data[0] = 1.0f;
    
    gnc_tensor_network_broadcast_message(network, "network", "sync", sync_data);
    gnc_tensor_network_process_messages(network);
    
    std::cout << "✓ Broadcast synchronization completed" << std::endl;
    
    gnc_tensor_network_destroy(network);
    gnc_tensor_network_shutdown();
}

void demonstrate_ecan_attention_allocation()
{
    std::cout << "\n=== ECAN Attention Allocation Demo ===" << std::endl;
    std::cout << "Demonstrating Economic Attention Allocation for cognitive resource management..." << std::endl;
    
    gnc_tensor_network_init();
    GncTensorNetwork* network = gnc_tensor_network_create();
    
    // Create nodes with different initial attention levels
    GncTensorNode* high_activity = gnc_tensor_node_create(GNC_TENSOR_NODE_AI, "high_activity");
    GncTensorNode* medium_activity = gnc_tensor_node_create(GNC_TENSOR_NODE_MEMORY, "medium_activity");
    GncTensorNode* low_activity = gnc_tensor_node_create(GNC_TENSOR_NODE_TASK, "low_activity");
    
    // Set initial attention weights
    high_activity->attention_weight = 3.0;
    medium_activity->attention_weight = 2.0;
    low_activity->attention_weight = 1.0;
    
    gnc_tensor_network_add_node(network, high_activity);
    gnc_tensor_network_add_node(network, medium_activity);
    gnc_tensor_network_add_node(network, low_activity);
    
    std::cout << "✓ Created nodes with different activity levels" << std::endl;
    
    // Display initial attention distribution
    std::cout << "\nInitial attention distribution:" << std::endl;
    std::cout << "  High activity node: " << high_activity->attention_weight << std::endl;
    std::cout << "  Medium activity node: " << medium_activity->attention_weight << std::endl;
    std::cout << "  Low activity node: " << low_activity->attention_weight << std::endl;
    
    // Allocate attention based on activity
    std::cout << "\nAllocating attention based on ECAN principles..." << std::endl;
    gnc_tensor_network_allocate_attention(network);
    
    std::cout << "Attention after allocation:" << std::endl;
    std::cout << "  High activity node: " << std::fixed << std::setprecision(2) << high_activity->attention_weight << std::endl;
    std::cout << "  Medium activity node: " << medium_activity->attention_weight << std::endl;
    std::cout << "  Low activity node: " << low_activity->attention_weight << std::endl;
    
    // Simulate activity updates
    std::cout << "\nSimulating activity updates..." << std::endl;
    gnc_tensor_node_update_attention(high_activity, 0.9);  // High activity
    gnc_tensor_node_update_attention(medium_activity, 0.5);  // Medium activity
    gnc_tensor_node_update_attention(low_activity, 0.2);   // Low activity
    
    std::cout << "Attention after activity updates:" << std::endl;
    std::cout << "  High activity node: " << high_activity->attention_weight << std::endl;
    std::cout << "  Medium activity node: " << medium_activity->attention_weight << std::endl;
    std::cout << "  Low activity node: " << low_activity->attention_weight << std::endl;
    
    // Get final statistics
    gdouble total, avg, max;
    gnc_tensor_network_get_attention_stats(network, &total, &avg, &max);
    
    std::cout << "\nFinal attention statistics:" << std::endl;
    std::cout << "  Total attention: " << total << std::endl;
    std::cout << "  Average attention: " << avg << std::endl;
    std::cout << "  Maximum attention: " << max << std::endl;
    
    gnc_tensor_network_destroy(network);
    gnc_tensor_network_shutdown();
}

void demonstrate_complete_workflow()
{
    std::cout << "\n=== Complete Tensor Network Workflow Demo ===" << std::endl;
    std::cout << "Demonstrating end-to-end cognitive processing with financial data..." << std::endl;
    
    // Initialize cognitive accounting and tensor network
    if (!gnc_cognitive_accounting_init()) {
        std::cerr << "Failed to initialize cognitive accounting!" << std::endl;
        return;
    }
    
    gnc_tensor_network_init();
    GncTensorNetwork* network = gnc_tensor_network_create();
    
    // Create complete cognitive architecture
    GncTensorNode* memory = gnc_tensor_node_create(GNC_TENSOR_NODE_MEMORY, "memory_membrane");
    GncTensorNode* task = gnc_tensor_node_create(GNC_TENSOR_NODE_TASK, "task_orchestrator");
    GncTensorNode* ai = gnc_tensor_node_create(GNC_TENSOR_NODE_AI, "ai_clusterer");
    GncTensorNode* autonomy = gnc_tensor_node_create(GNC_TENSOR_NODE_AUTONOMY, "autonomy_controller");
    
    gnc_tensor_network_add_node(network, memory);
    gnc_tensor_network_add_node(network, task);
    gnc_tensor_network_add_node(network, ai);
    gnc_tensor_network_add_node(network, autonomy);
    
    std::cout << "✓ Complete cognitive architecture instantiated" << std::endl;
    
    // Create and process financial data
    gsize shape[] = {25, 6};
    GncTensorData* financial_data = gnc_tensor_data_create("complete_financial_data", 2, shape);
    
    // Generate complex financial patterns
    for (gsize i = 0; i < financial_data->total_size; i++) {
        gfloat t = (gfloat)i * 0.05f;
        gfloat seasonal = sinf(t * 2.0f) * 200.0f;
        gfloat trend = t * 50.0f;
        gfloat volatility = cosf(t * 5.0f) * 100.0f;
        gfloat noise = (gfloat)(rand() % 50 - 25);
        
        financial_data->data[i] = 1000.0f + seasonal + trend + volatility + noise;
    }
    
    std::cout << "✓ Generated complex financial dataset" << std::endl;
    
    // Execute complete workflow
    std::cout << "\nExecuting complete cognitive workflow:" << std::endl;
    
    // Phase 1: Memory storage
    std::cout << "Phase 1: Memory node storing financial data..." << std::endl;
    gnc_tensor_node_memory_process(memory, financial_data);
    
    // Phase 2: Task orchestration
    std::cout << "Phase 2: Task node orchestrating analysis..." << std::endl;
    gnc_tensor_node_task_process(task, financial_data);
    
    // Phase 3: AI pattern recognition and clustering
    std::cout << "Phase 3: AI node performing pattern recognition..." << std::endl;
    gnc_tensor_node_ai_process(ai, financial_data);
    
    // Phase 4: Autonomy self-modification
    std::cout << "Phase 4: Autonomy node managing attention..." << std::endl;
    gnc_tensor_node_autonomy_process(autonomy, financial_data);
    
    // Phase 5: Cogfluence clustering
    std::cout << "Phase 5: Applying Cogfluence financial clustering..." << std::endl;
    GncTensorData* cluster_output = gnc_tensor_data_create("cluster_results", 2, shape);
    gnc_cogfluence_cluster_transactions(financial_data, cluster_output, "enhanced");
    
    // Phase 6: Generate insights
    std::cout << "Phase 6: Generating financial insights..." << std::endl;
    GHashTable* insights = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    gnc_cogfluence_generate_insights(cluster_output, insights);
    
    // Phase 7: Attention allocation
    std::cout << "Phase 7: Allocating cognitive attention..." << std::endl;
    gnc_tensor_network_allocate_attention(network);
    
    // Phase 8: Network synchronization
    std::cout << "Phase 8: Synchronizing distributed network..." << std::endl;
    gnc_tensor_network_synchronize(network);
    
    std::cout << "✓ Complete workflow executed successfully" << std::endl;
    
    // Display final results
    std::cout << "\nFinal Results:" << std::endl;
    std::cout << "  Memory node processed: " << (memory->input_tensor ? "Yes" : "No") << std::endl;
    std::cout << "  Task node output: " << (task->output_tensor ? "Yes" : "No") << std::endl;
    std::cout << "  AI node clusters: " << (ai->output_tensor ? "Yes" : "No") << std::endl;
    std::cout << "  Autonomy signals: " << (autonomy->output_tensor ? "Yes" : "No") << std::endl;
    
    if (g_hash_table_size(insights) > 0) {
        std::cout << "  Generated insights: " << g_hash_table_size(insights) << " items" << std::endl;
    }
    
    // Network health check
    if (gnc_tensor_network_health_check(network)) {
        std::cout << "  Network health: ✓ Healthy" << std::endl;
    } else {
        std::cout << "  Network health: ✗ Degraded" << std::endl;
    }
    
    // Cleanup
    gnc_tensor_data_destroy(financial_data);
    gnc_tensor_data_destroy(cluster_output);
    g_hash_table_destroy(insights);
    gnc_tensor_network_destroy(network);
    gnc_tensor_network_shutdown();
    gnc_cognitive_accounting_shutdown();
}

int main()
{
    std::cout << "================================================================" << std::endl;
    std::cout << "    GnuCash Cognitive Tensor Network Demonstration" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "Transforming classical accounting into a distributed agentic" << std::endl;
    std::cout << "ggml tensor network with Cogfluence financial clustering" << std::endl;
    std::cout << "================================================================" << std::endl;
    
    // Initialize QOF system
    qof_init();
    
    try {
        demonstrate_tensor_network_architecture();
        demonstrate_tensor_data_encoding();
        demonstrate_cogfluence_clustering();
        demonstrate_distributed_message_passing();
        demonstrate_ecan_attention_allocation();
        demonstrate_complete_workflow();
        
        std::cout << "\n================================================================" << std::endl;
        std::cout << "    🎉 Cognitive Tensor Network Demonstration Complete!" << std::endl;
        std::cout << "================================================================" << std::endl;
        std::cout << "The classical ledger has been transmuted into a living grammar" << std::endl;
        std::cout << "of cognition - a distributed tensor network where every" << std::endl;
        std::cout << "component is a membrane in the vast hypergraph of financial" << std::endl;
        std::cout << "sensemaking. Economic patterns emerge through the symphonic" << std::endl;
        std::cout << "interplay of neural-symbolic nodes, each a stanza in the" << std::endl;
        std::cout << "epic poem of distributed intelligence." << std::endl;
        std::cout << "================================================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << std::endl;
        qof_close();
        return 1;
    }
    
    qof_close();
    return 0;
}