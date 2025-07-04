# Distributed Tensor Network Architecture Documentation

## Overview

This document describes the distributed ggml tensor network architecture that transforms GnuCashCog into an agentic cognitive system. The architecture implements a living grammar of cognition where each component serves as a node in the tensor network.

## System Architecture

```mermaid
graph TD
    subgraph "Distributed Tensor Network"
        A[Raw Financial Data] -->|encode| B[Memory Node]
        B -->|tensor operations| C[Task Node]
        C -->|orchestrate| D[AI Node]
        D -->|pattern recognition| E[Autonomy Node]
        E -->|attention allocation| F[AtomSpace Knowledge Store]
        F -->|feedback| B
    end
    
    subgraph "Node Types"
        G[Memory Node<br/>- Stores transactions<br/>- Manages clusters<br/>- Maintains states]
        H[Task Node<br/>- Orchestrates workflow<br/>- Triggers clustering<br/>- Manages pipelines]
        I[AI Node<br/>- Pattern recognition<br/>- Cogfluence clustering<br/>- Financial analysis]
        J[Autonomy Node<br/>- Self-modification<br/>- Attention allocation<br/>- ECAN dynamics]
    end
    
    subgraph "Tensor Operations"
        K[ggml Tensor<br/>N_tx × D_feat]
        L[ggml Tensor<br/>N_task × D_param]
        M[ggml Tensor<br/>N_cluster × D_metric]
        N[ggml Tensor<br/>N_mod × D_signal]
    end
    
    B -.->|implements| G
    C -.->|implements| H
    D -.->|implements| I
    E -.->|implements| J
    
    G -->|tensor data| K
    H -->|tensor data| L
    I -->|tensor data| M
    J -->|tensor data| N
```

## Message Passing Architecture

```mermaid
sequenceDiagram
    participant Input as Financial Input
    participant Memory as Memory Node
    participant Task as Task Node
    participant AI as AI Node
    participant Autonomy as Autonomy Node
    participant AtomSpace as Knowledge Store
    
    Input->>Memory: store_data(transactions)
    Memory->>Task: process_request(tensor_data)
    Task->>AI: cluster_data(financial_tensor)
    AI->>Autonomy: update_attention(cluster_results)
    Autonomy->>AtomSpace: store_insights(attention_weights)
    AtomSpace->>Memory: feedback(knowledge_updates)
    
    Note over Memory,Autonomy: Distributed Processing
    Note over AtomSpace: Hypergraph Knowledge Store
```

## Tensor Data Flow

```mermaid
flowchart LR
    subgraph "Input Layer"
        A[Transactions] --> B[Account Data]
        B --> C[Financial Metrics]
    end
    
    subgraph "Tensor Encoding"
        D[Transaction Tensor<br/>shape: [N_tx, D_feat]]
        E[Account Tensor<br/>shape: [N_acc, D_acc]]
        F[Metric Tensor<br/>shape: [N_metric, D_val]]
    end
    
    subgraph "Processing Nodes"
        G[Memory Node Processing]
        H[Task Node Processing]
        I[AI Node Processing]
        J[Autonomy Node Processing]
    end
    
    subgraph "Output Layer"
        K[Clustered Data]
        L[Attention Weights]
        M[Insights]
    end
    
    A --> D
    B --> E
    C --> F
    
    D --> G
    E --> H
    F --> I
    
    G --> J
    H --> J
    I --> J
    
    J --> K
    J --> L
    J --> M
```

## Cogfluence Clustering Integration

```mermaid
graph TB
    subgraph "Cogfluence Financial Clustering"
        A[Financial Data Tensor] --> B[Enhanced Clustering Algorithm]
        B --> C[Pattern Discovery Engine]
        C --> D[Emergent Insights Generator]
        D --> E[Economic Knowledge Base]
    end
    
    subgraph "Clustering Methods"
        F[K-means Clustering]
        G[Cogfluence Enhanced]
        H[Hierarchical Clustering]
        I[Density-based Clustering]
    end
    
    subgraph "Pattern Types"
        J[Seasonal Patterns]
        K[Trend Analysis]
        L[Volatility Clusters]
        M[Anomaly Detection]
    end
    
    B --> F
    B --> G
    B --> H
    B --> I
    
    C --> J
    C --> K
    C --> L
    C --> M
```

## ECAN Attention Allocation

```mermaid
graph TD
    subgraph "Economic Attention Allocation"
        A[Attention Pool<br/>Total: 100.0] --> B[STI Calculation]
        B --> C[LTI Calculation]
        C --> D[VLTI Calculation]
        D --> E[Attention Distribution]
    end
    
    subgraph "Node Attention Weights"
        F[Memory Node<br/>Weight: Dynamic]
        G[Task Node<br/>Weight: Dynamic]
        H[AI Node<br/>Weight: Dynamic]
        I[Autonomy Node<br/>Weight: Dynamic]
    end
    
    subgraph "Attention Dynamics"
        J[Activity Level] --> K[Attention Update]
        K --> L[Decay Function]
        L --> M[Redistribution]
    end
    
    E --> F
    E --> G
    E --> H
    E --> I
    
    F --> J
    G --> J
    H --> J
    I --> J
    
    M --> A
```

## Network Synchronization

```mermaid
stateDiagram-v2
    [*] --> Initializing
    Initializing --> Active: network_init()
    Active --> Processing: process_messages()
    Processing --> Synchronizing: sync_required()
    Synchronizing --> Processing: sync_complete()
    Processing --> AttentionAllocation: attention_update()
    AttentionAllocation --> Processing: allocation_complete()
    Processing --> HealthCheck: health_check()
    HealthCheck --> Processing: healthy()
    HealthCheck --> Degraded: unhealthy()
    Degraded --> Processing: recovery()
    Processing --> Shutdown: shutdown_request()
    Shutdown --> [*]
```

## Component Integration

```mermaid
graph LR
    subgraph "GnuCash Core"
        A[Accounts] --> B[Transactions]
        B --> C[Splits]
        C --> D[Books]
    end
    
    subgraph "Cognitive Layer"
        E[AtomSpace] --> F[PLN]
        F --> G[ECAN]
        G --> H[MOSES]
        H --> I[URE]
    end
    
    subgraph "Tensor Network"
        J[Memory Node] --> K[Task Node]
        K --> L[AI Node]
        L --> M[Autonomy Node]
    end
    
    subgraph "Cogfluence"
        N[Financial Clustering] --> O[Pattern Discovery]
        O --> P[Insight Generation]
    end
    
    D --> E
    I --> J
    M --> N
    P --> A
```

## API Architecture

```mermaid
classDiagram
    class GncTensorNetwork {
        +nodes: GHashTable
        +message_queue: GQueue
        +network_active: gboolean
        +total_attention: gdouble
        +network_timestamp: gint64
        +create() GncTensorNetwork
        +destroy()
        +add_node(node)
        +remove_node(node_id)
        +get_node(node_id)
        +send_message(source, target, type, payload)
        +process_messages()
        +broadcast_message(source, type, payload)
        +synchronize()
        +health_check()
    }
    
    class GncTensorNode {
        +type: GncTensorNodeType
        +node_id: gchar*
        +input_tensor: GncTensorData
        +output_tensor: GncTensorData
        +parameters: GHashTable
        +active: gboolean
        +attention_weight: gdouble
        +last_update: gint64
        +create(type, node_id)
        +destroy()
        +process(input)
        +update_attention(activity)
    }
    
    class GncTensorData {
        +data: gfloat*
        +shape: gsize*
        +n_dims: gsize
        +total_size: gsize
        +name: gchar*
        +ggml_tensor: ggml_tensor*
        +create(name, n_dims, shape)
        +destroy()
        +from_transactions(transactions)
        +from_accounts(accounts)
        +apply_clustering(algorithm)
    }
    
    class GncTensorMessage {
        +source_node_id: gchar*
        +target_node_id: gchar*
        +message_type: gchar*
        +payload: GncTensorData
        +priority: gdouble
        +timestamp: gint64
    }
    
    GncTensorNetwork --> GncTensorNode
    GncTensorNode --> GncTensorData
    GncTensorNetwork --> GncTensorMessage
    GncTensorMessage --> GncTensorData
```

## Installation and Dependencies

### Required Dependencies

- **ggml**: Tensor operations library
- **OpenCog**: Cognitive architecture framework
- **glib**: Foundation library
- **cmake**: Build system

### Optional Dependencies

- **OpenCog modules**: atomspace, pln, ecan, moses, ure
- **Google Test**: For testing

### Build Configuration

```cmake
# Enable tensor network support
set(HAVE_GGML 1)
set(HAVE_COGFLUENCE_CLUSTERING 1)
add_definitions(-DHAVE_GGML -DHAVE_COGFLUENCE_CLUSTERING)
```

## Usage Examples

### Basic Tensor Network Initialization

```c
// Initialize tensor network
gnc_tensor_network_init();

// Create network context
GncTensorNetwork* network = gnc_tensor_network_create();

// Create nodes
GncTensorNode* memory = gnc_tensor_node_create(GNC_TENSOR_NODE_MEMORY, "memory");
GncTensorNode* ai = gnc_tensor_node_create(GNC_TENSOR_NODE_AI, "ai");

// Add nodes to network
gnc_tensor_network_add_node(network, memory);
gnc_tensor_network_add_node(network, ai);

// Send messages
gnc_tensor_network_send_message(network, "memory", "ai", "process_data", tensor_data);
gnc_tensor_network_process_messages(network);

// Cleanup
gnc_tensor_network_destroy(network);
gnc_tensor_network_shutdown();
```

### Cogfluence Clustering

```c
// Create financial data tensor
GncTensorData* financial_data = gnc_tensor_data_create("financial_data", 2, shape);
GncTensorData* cluster_output = gnc_tensor_data_create("clusters", 2, shape);

// Apply Cogfluence clustering
gnc_cogfluence_cluster_transactions(financial_data, cluster_output, "enhanced");

// Discover patterns
GncTensorData* patterns = gnc_tensor_data_create("patterns", 2, shape);
gnc_cogfluence_discover_patterns(cluster_output, patterns, 0.5);

// Generate insights
GHashTable* insights = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
gnc_cogfluence_generate_insights(cluster_output, insights);
```

## Testing

The tensor network includes comprehensive tests covering:

- Network initialization and shutdown
- Node creation and management
- Tensor data encoding and operations
- Message passing and communication
- Attention allocation and ECAN dynamics
- Cogfluence clustering algorithms
- Complete workflow integration

Run tests with:
```bash
make test-tensor-network
```

## Performance Considerations

- **Memory Management**: Efficient tensor allocation and deallocation
- **Message Queue**: Asynchronous processing for scalability
- **Attention Allocation**: Dynamic resource management
- **Clustering**: Optimized algorithms for financial data
- **Network Synchronization**: Minimal overhead coordination

## Future Enhancements

1. **GPU Acceleration**: CUDA/OpenCL support for tensor operations
2. **Distributed Computing**: Multi-node network deployment
3. **Advanced Clustering**: Deep learning integration
4. **Real-time Processing**: Streaming financial data support
5. **Visualization**: Interactive network monitoring tools

## Contributing

Contributions to the tensor network are welcome. Please follow the existing code style and include comprehensive tests for new features.

## License

This tensor network implementation is licensed under the GNU General Public License v2.0, consistent with the GnuCash project.