/********************************************************************\
 * minimal-tensor-demo.cpp -- Minimal tensor network demo         *
 * Copyright (C) 2024 GnuCash Cognitive Engine                     *
 *                                                                  *
 * Simple demonstration of tensor network concepts without         *
 * full GnuCash build dependencies                                 *
 ********************************************************************/

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <memory>
#include <queue>

// Minimal tensor data structure
struct MinimalTensor {
    std::vector<float> data;
    std::vector<size_t> shape;
    std::string name;
    
    MinimalTensor(const std::string& n, const std::vector<size_t>& s) 
        : name(n), shape(s) {
        size_t total_size = 1;
        for (auto dim : shape) {
            total_size *= dim;
        }
        data.resize(total_size, 0.0f);
    }
    
    void fill_with_pattern(float base) {
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = base + std::sin(i * 0.1f) * 10.0f;
        }
    }
    
    void print_info() const {
        std::cout << "Tensor '" << name << "' shape: [";
        for (size_t i = 0; i < shape.size(); i++) {
            std::cout << shape[i];
            if (i < shape.size() - 1) std::cout << ", ";
        }
        std::cout << "] size: " << data.size() << std::endl;
    }
};

// Node types
enum NodeType {
    MEMORY_NODE,
    TASK_NODE,
    AI_NODE,
    AUTONOMY_NODE
};

// Minimal tensor node
struct TensorNode {
    NodeType type;
    std::string id;
    std::unique_ptr<MinimalTensor> input_tensor;
    std::unique_ptr<MinimalTensor> output_tensor;
    double attention_weight;
    bool active;
    
    TensorNode(NodeType t, const std::string& node_id) 
        : type(t), id(node_id), attention_weight(1.0), active(true) {}
    
    void process_data(std::unique_ptr<MinimalTensor> input) {
        input_tensor = std::move(input);
        
        switch (type) {
            case MEMORY_NODE:
                process_memory();
                break;
            case TASK_NODE:
                process_task();
                break;
            case AI_NODE:
                process_ai();
                break;
            case AUTONOMY_NODE:
                process_autonomy();
                break;
        }
    }
    
private:
    void process_memory() {
        std::cout << "Memory node '" << id << "' storing data..." << std::endl;
        // Memory node: Store the input data
        if (input_tensor) {
            output_tensor = std::make_unique<MinimalTensor>("memory_state", input_tensor->shape);
            output_tensor->data = input_tensor->data;
        }
    }
    
    void process_task() {
        std::cout << "Task node '" << id << "' orchestrating workflow..." << std::endl;
        // Task node: Create task parameters
        output_tensor = std::make_unique<MinimalTensor>("task_params", std::vector<size_t>{5});
        output_tensor->data[0] = 1.0f;  // Clustering enabled
        output_tensor->data[1] = 0.7f;  // Threshold
        output_tensor->data[2] = 1.0f;  // Attention enabled
        output_tensor->data[3] = 0.8f;  // Activity level
        output_tensor->data[4] = 1.2f;  // Learning rate
    }
    
    void process_ai() {
        std::cout << "AI node '" << id << "' applying Cogfluence clustering..." << std::endl;
        // AI node: Apply clustering
        if (input_tensor) {
            output_tensor = std::make_unique<MinimalTensor>("ai_clusters", input_tensor->shape);
            
            // Cogfluence clustering simulation
            for (size_t i = 0; i < input_tensor->data.size(); i++) {
                float input_val = input_tensor->data[i];
                float cluster_val = std::sin(input_val * 0.1f) * std::cos(input_val * 0.05f);
                cluster_val = std::abs(cluster_val) * 5.0f;
                output_tensor->data[i] = cluster_val;
            }
        }
    }
    
    void process_autonomy() {
        std::cout << "Autonomy node '" << id << "' managing attention allocation..." << std::endl;
        // Autonomy node: Generate attention signals
        output_tensor = std::make_unique<MinimalTensor>("autonomy_signals", std::vector<size_t>{4});
        output_tensor->data[0] = attention_weight;
        output_tensor->data[1] = 0.9f;  // Adaptation factor
        output_tensor->data[2] = 1.1f;  // Learning rate
        output_tensor->data[3] = 1.0f;  // Active flag
    }
};

// Message structure
struct TensorMessage {
    std::string source;
    std::string target;
    std::string type;
    std::unique_ptr<MinimalTensor> payload;
    double priority;
    
    TensorMessage(const std::string& src, const std::string& tgt, 
                  const std::string& msg_type, std::unique_ptr<MinimalTensor> data)
        : source(src), target(tgt), type(msg_type), payload(std::move(data)), priority(0.5) {}
};

// Minimal tensor network
class MinimalTensorNetwork {
private:
    std::map<std::string, std::unique_ptr<TensorNode>> nodes;
    std::queue<std::unique_ptr<TensorMessage>> message_queue;
    double total_attention;
    
public:
    MinimalTensorNetwork() : total_attention(100.0) {}
    
    void add_node(std::unique_ptr<TensorNode> node) {
        std::string id = node->id;
        nodes[id] = std::move(node);
        std::cout << "Added node: " << id << std::endl;
    }
    
    void send_message(const std::string& source, const std::string& target,
                      const std::string& type, std::unique_ptr<MinimalTensor> payload) {
        auto msg = std::make_unique<TensorMessage>(source, target, type, std::move(payload));
        message_queue.push(std::move(msg));
        std::cout << "Message queued: " << source << " -> " << target << " (" << type << ")" << std::endl;
    }
    
    void process_messages() {
        std::cout << "\nProcessing distributed messages..." << std::endl;
        
        while (!message_queue.empty()) {
            auto msg = std::move(message_queue.front());
            message_queue.pop();
            
            auto it = nodes.find(msg->target);
            if (it != nodes.end()) {
                it->second->process_data(std::move(msg->payload));
            }
        }
    }
    
    void allocate_attention() {
        std::cout << "\nAllocating ECAN attention across nodes..." << std::endl;
        
        double total_activity = 0.0;
        for (const auto& [id, node] : nodes) {
            total_activity += node->attention_weight;
        }
        
        for (const auto& [id, node] : nodes) {
            double new_weight = (node->attention_weight / total_activity) * total_attention;
            std::cout << "Node '" << id << "' attention: " << new_weight << std::endl;
            node->attention_weight = new_weight;
        }
    }
    
    void synchronize() {
        std::cout << "\nSynchronizing tensor network..." << std::endl;
        auto sync_data = std::make_unique<MinimalTensor>("sync", std::vector<size_t>{1});
        sync_data->data[0] = 1.0f;
        
        // Broadcast sync to all nodes
        for (const auto& [id, node] : nodes) {
            auto sync_copy = std::make_unique<MinimalTensor>("sync", std::vector<size_t>{1});
            sync_copy->data[0] = 1.0f;
            send_message("network", id, "sync", std::move(sync_copy));
        }
        process_messages();
    }
    
    void print_status() {
        std::cout << "\n=== Tensor Network Status ===" << std::endl;
        std::cout << "Total nodes: " << nodes.size() << std::endl;
        
        for (const auto& [id, node] : nodes) {
            std::cout << "Node '" << id << "': ";
            if (node->output_tensor) {
                std::cout << "Output tensor ready (size: " << node->output_tensor->data.size() << ")";
            } else {
                std::cout << "No output";
            }
            std::cout << std::endl;
        }
    }
};

// Demonstration functions
void demonstrate_tensor_creation() {
    std::cout << "\n=== Tensor Data Creation Demo ===" << std::endl;
    
    // Create financial data tensor
    auto financial_tensor = std::make_unique<MinimalTensor>("financial_data", std::vector<size_t>{20, 5});
    financial_tensor->fill_with_pattern(100.0f);
    financial_tensor->print_info();
    
    std::cout << "Sample data: ";
    for (size_t i = 0; i < 5; i++) {
        std::cout << financial_tensor->data[i] << " ";
    }
    std::cout << "..." << std::endl;
}

void demonstrate_distributed_processing() {
    std::cout << "\n=== Distributed Processing Demo ===" << std::endl;
    
    MinimalTensorNetwork network;
    
    // Create cognitive nodes
    auto memory = std::make_unique<TensorNode>(MEMORY_NODE, "memory_membrane");
    auto task = std::make_unique<TensorNode>(TASK_NODE, "task_orchestrator");
    auto ai = std::make_unique<TensorNode>(AI_NODE, "ai_clusterer");
    auto autonomy = std::make_unique<TensorNode>(AUTONOMY_NODE, "autonomy_controller");
    
    network.add_node(std::move(memory));
    network.add_node(std::move(task));
    network.add_node(std::move(ai));
    network.add_node(std::move(autonomy));
    
    // Create test financial data
    auto financial_data = std::make_unique<MinimalTensor>("financial_input", std::vector<size_t>{15, 4});
    financial_data->fill_with_pattern(500.0f);
    
    // Simulate distributed workflow
    std::cout << "\nExecuting distributed cognitive workflow:" << std::endl;
    
    // Step 1: Send data to memory
    auto data_copy1 = std::make_unique<MinimalTensor>("financial_input", std::vector<size_t>{15, 4});
    data_copy1->data = financial_data->data;
    network.send_message("input", "memory_membrane", "store_data", std::move(data_copy1));
    
    // Step 2: Memory to task
    network.send_message("memory_membrane", "task_orchestrator", "process_request", nullptr);
    
    // Step 3: Task to AI
    auto data_copy2 = std::make_unique<MinimalTensor>("financial_input", std::vector<size_t>{15, 4});
    data_copy2->data = financial_data->data;
    network.send_message("task_orchestrator", "ai_clusterer", "cluster_data", std::move(data_copy2));
    
    // Step 4: AI to autonomy
    network.send_message("ai_clusterer", "autonomy_controller", "update_attention", nullptr);
    
    // Process all messages
    network.process_messages();
    
    // Allocate attention
    network.allocate_attention();
    
    // Synchronize network
    network.synchronize();
    
    // Print final status
    network.print_status();
}

void demonstrate_cogfluence_clustering() {
    std::cout << "\n=== Cogfluence Clustering Demo ===" << std::endl;
    
    // Create financial data with patterns
    auto financial_data = std::make_unique<MinimalTensor>("market_data", std::vector<size_t>{25, 3});
    
    // Generate synthetic financial patterns
    for (size_t i = 0; i < financial_data->data.size(); i++) {
        float t = i * 0.05f;
        float seasonal = std::sin(t * 2.0f) * 200.0f;
        float trend = t * 50.0f;
        float volatility = std::cos(t * 5.0f) * 100.0f;
        
        financial_data->data[i] = 1000.0f + seasonal + trend + volatility;
    }
    
    financial_data->print_info();
    
    // Apply clustering using AI node
    auto ai_node = std::make_unique<TensorNode>(AI_NODE, "clusterer");
    ai_node->process_data(std::move(financial_data));
    
    if (ai_node->output_tensor) {
        std::cout << "Clustering results:" << std::endl;
        ai_node->output_tensor->print_info();
        
        // Show sample clustered values
        std::cout << "Sample clusters: ";
        for (size_t i = 0; i < 5 && i < ai_node->output_tensor->data.size(); i++) {
            std::cout << ai_node->output_tensor->data[i] << " ";
        }
        std::cout << "..." << std::endl;
    }
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "    GnuCash Cognitive Tensor Network - Minimal Demo" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "Demonstrating distributed ggml tensor network concepts" << std::endl;
    std::cout << "with agentic cognitive nodes and Cogfluence clustering" << std::endl;
    std::cout << "================================================================" << std::endl;
    
    try {
        demonstrate_tensor_creation();
        demonstrate_cogfluence_clustering();
        demonstrate_distributed_processing();
        
        std::cout << "\n================================================================" << std::endl;
        std::cout << "    🎉 Tensor Network Demonstration Complete!" << std::endl;
        std::cout << "================================================================" << std::endl;
        std::cout << "Successfully demonstrated:" << std::endl;
        std::cout << "✓ Tensor data structures for financial information" << std::endl;
        std::cout << "✓ Distributed cognitive node architecture" << std::endl;
        std::cout << "✓ Message passing between Memory, Task, AI, and Autonomy nodes" << std::endl;
        std::cout << "✓ Cogfluence financial clustering algorithms" << std::endl;
        std::cout << "✓ ECAN-inspired attention allocation" << std::endl;
        std::cout << "✓ Network synchronization protocols" << std::endl;
        std::cout << "\nThe classical ledger has been conceptually transformed into" << std::endl;
        std::cout << "a distributed tensor network - a living grammar of cognition!" << std::endl;
        std::cout << "================================================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}