# Network System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 3
> **Dependencies:** Types, Entity, Events
> **Lua Paths:** `bestow.network` (high-level), `bestow.network.core` (low-level)

## Purpose

The Network System provides client-server multiplayer networking for Bestow games. It handles connection management, entity replication, remote procedure calls (RPCs), and latency compensation. The high-level API covers hosting, connecting, replicating entities, and subscribing to connection events with a single method each. The low-level API exposes the full networking surface including initialization, replicated component registration, ownership management, RPC registration and invocation, lobby management, per-client latency monitoring, and entity serialization helpers. This is a new system in Bestow V2.

## High-Level API: `INetworkSystem`

The simplified networking interface for common multiplayer tasks. One-call host/connect, entity-centric replication, and connection event callbacks. No lifecycle methods -- the engine calls `initialize()`, `update()`, and `shutdown()` internally.

### Connection

| Method | Returns | Description |
|--------|---------|-------------|
| `host(int port)` | `Result<void>` | Start hosting a game server on the given port with default settings |
| `connect(std::string_view address, int port)` | `Result<void>` | Connect to a remote server at the given address and port |
| `disconnect()` | `void` | Disconnect from the current server or stop hosting |
| `isConnected()` | `bool` | Check whether the local client is currently connected to a server (or hosting) |
| `isHost()` | `bool` | Check whether the local instance is the server/host |
| `getLocalId()` | `NetworkId` | Return the network identifier of the local client |

### Entity Replication

| Method | Returns | Description |
|--------|---------|-------------|
| `replicate(Entity entity)` | `Result<void>` | Register an entity for automatic network replication to all connected clients |
| `isLocallyOwned(Entity entity)` | `bool` | Check whether the local client owns (has authority over) the given replicated entity |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `onClientConnected(std::function<void(NetworkId)> cb)` | `SubscriptionId` | Subscribe to notifications when a new client connects to the server |
| `onClientDisconnected(std::function<void(NetworkId)> cb)` | `SubscriptionId` | Subscribe to notifications when a client disconnects from the server |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered callback subscription |

### Latency

| Method | Returns | Description |
|--------|---------|-------------|
| `getPing()` | `float` | Return the round-trip ping time in seconds to the server (or average across clients if host) |

## Low-Level API: `INetworkCore`

Full control API. Exposes initialization, connection management, entity replication with ownership, component-level replication, RPC registration and invocation, lobby management, per-client latency monitoring, connection events, and entity serialization.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize(const NetworkConfig& config)` | `Result<void>` | Initialize the networking subsystem with the given configuration |
| `shutdown()` | `void` | Shut down all networking, disconnecting from any server and releasing resources |
| `update(DeltaTime dt)` | `void` | Process incoming packets, apply replication updates, and send outgoing data |

### Connection

| Method | Returns | Description |
|--------|---------|-------------|
| `host(int port, int maxClients)` | `Result<void>` | Start hosting a game server on the given port with the specified maximum client count |
| `connect(std::string_view address, int port)` | `Result<void>` | Connect to a remote server at the given address and port |
| `disconnect()` | `void` | Disconnect from the current server or stop hosting |
| `isConnected()` | `bool` | Check whether the local instance is currently connected or hosting |
| `isHost()` | `bool` | Check whether the local instance is the server/host |
| `getLocalId()` | `NetworkId` | Return the network identifier of the local client |
| `getConnectedClients()` | `std::vector<NetworkId>` | Return the network IDs of all currently connected clients (host only) |

### Entity Replication

| Method | Returns | Description |
|--------|---------|-------------|
| `registerReplicatedEntity(Entity entity)` | `Result<void>` | Register an entity for network replication; its replicated components will be synchronized |
| `unregisterReplicatedEntity(Entity entity)` | `Result<void>` | Stop replicating an entity and remove it from the network synchronization list |
| `setOwner(Entity entity, NetworkId owner)` | `Result<void>` | Set the owning client for a replicated entity; only the owner can modify authoritative state |
| `getOwner(Entity entity)` | `NetworkId` | Return the network ID of the client that owns a replicated entity |
| `isLocallyOwned(Entity entity)` | `bool` | Check whether the local client owns the given replicated entity |

### Replicated Components

| Method | Returns | Description |
|--------|---------|-------------|
| `registerReplicatedComponent(std::string_view componentName, ReplicationMode mode)` | `Result<void>` | Register a component type for network replication with the given reliability mode |
| `markDirty(Entity entity, std::string_view componentName)` | `Result<void>` | Mark a component as modified so it will be included in the next replication update |

### RPCs (Remote Procedure Calls)

| Method | Returns | Description |
|--------|---------|-------------|
| `registerRPC(std::string_view name, std::function<void(NetworkId sender, std::span<const std::byte> data)> handler)` | `Result<RPCHandle>` | Register a named RPC handler that can be invoked remotely; returns a handle for calling |
| `callRPC(RPCHandle rpc, NetworkId target, std::span<const std::byte> data)` | `Result<void>` | Call an RPC on a specific client, sending the given payload data |
| `callRPCAll(RPCHandle rpc, std::span<const std::byte> data)` | `Result<void>` | Call an RPC on all connected clients |
| `callRPCServer(RPCHandle rpc, std::span<const std::byte> data)` | `Result<void>` | Call an RPC on the server (client-to-server invocation) |
| `unregisterRPC(RPCHandle rpc)` | `void` | Unregister a previously registered RPC handler |

### Lobby

| Method | Returns | Description |
|--------|---------|-------------|
| `setMaxClients(int max)` | `Result<void>` | Set the maximum number of clients allowed to connect (host only) |
| `kickClient(NetworkId client)` | `Result<void>` | Forcefully disconnect a specific client (host only) |

### Latency

| Method | Returns | Description |
|--------|---------|-------------|
| `getPing(NetworkId client)` | `float` | Return the round-trip ping time in seconds for a specific client |
| `getAveragePing()` | `float` | Return the average ping time across all connected clients |
| `getPacketLoss()` | `float` | Return the packet loss ratio (0.0 to 1.0) over a recent window |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `onClientConnected(std::function<void(NetworkId)> cb)` | `SubscriptionId` | Subscribe to notifications when a new client connects |
| `onClientDisconnected(std::function<void(NetworkId)> cb)` | `SubscriptionId` | Subscribe to notifications when a client disconnects |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered callback subscription |

### Serialization Helpers

| Method | Returns | Description |
|--------|---------|-------------|
| `serializeEntity(Entity entity)` | `std::vector<std::byte>` | Serialize all replicated components of an entity into a byte buffer for network transmission |
| `deserializeEntity(Entity entity, std::span<const std::byte> data)` | `Result<void>` | Apply deserialized component data from a byte buffer to an existing entity |

## Types

### NetworkConfig

Configuration for the networking subsystem, controlling protocol, tick rate, and latency compensation.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `protocol` | `NetworkConfig::Protocol` | `UDP` | The transport protocol to use for network communication |
| `tickRate` | `int` | `30` | Number of server simulation updates per second |
| `interpolationDelay` | `float` | `0.1f` | Interpolation delay in seconds for smoothing remote entity positions |
| `enablePrediction` | `bool` | `true` | Whether to enable client-side prediction for locally controlled entities |
| `enableReconciliation` | `bool` | `true` | Whether to enable server reconciliation to correct mispredicted client state |

### NetworkConfig::Protocol

```cpp
enum class Protocol : std::uint8_t {
    UDP,
    WebSocket
};
```

| Value | Description |
|-------|-------------|
| `UDP` | Standard UDP transport; lowest latency, suitable for most games |
| `WebSocket` | WebSocket transport; required for browser-based clients |

### ReplicationMode

Controls the reliability and ordering guarantees for replicated component data.

```cpp
enum class ReplicationMode : std::uint8_t {
    Reliable,
    Unreliable,
    UnreliableOrdered
};
```

| Value | Description |
|-------|-------------|
| `Reliable` | TCP-like delivery: guaranteed, ordered delivery; use for critical state (health, inventory) |
| `Unreliable` | UDP-like delivery: fastest, no delivery or ordering guarantees; use for frequent updates (position) |
| `UnreliableOrdered` | Unreliable but ordered: out-of-order packets are dropped; use for state that must be current (input) |

### NetworkId

A strong typed handle identifying a client on the network.

```cpp
using NetworkId = Handle<struct NetworkTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Unique network identifier; 0 means invalid/unconnected |

### RPCHandle

A strong typed handle referencing a registered remote procedure call.

```cpp
using RPCHandle = Handle<struct RPCTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Internal identifier; 0 means invalid/unregistered |

## Lua Examples

```lua
-- High-level: Host a game
local _, err = bestow.network.host(7777)
if err then
    print("Failed to host: " .. err.message)
    return
end

-- Or connect to an existing game
bestow.network.connect("192.168.1.100", 7777)

-- Check connection status
if bestow.network.isConnected() then
    print("Connected! Local ID: " .. tostring(bestow.network.getLocalId()))
    print("Is host: " .. tostring(bestow.network.isHost()))
end

-- Replicate an entity
local player = bestow.entity.create()
bestow.entity.addComponent(player, "Transform3D", { x = 0, y = 0, z = 0 })
bestow.network.replicate(player)

if bestow.network.isLocallyOwned(player) then
    -- We control this entity
end

-- Connection events
local connectSub = bestow.network.onClientConnected(function(clientId)
    print("Client connected: " .. tostring(clientId))
end)

local disconnectSub = bestow.network.onClientDisconnected(function(clientId)
    print("Client disconnected: " .. tostring(clientId))
end)

-- Latency
print("Ping: " .. bestow.network.getPing() .. "s")

-- Clean up
bestow.network.unsubscribe(connectSub)
bestow.network.unsubscribe(disconnectSub)
bestow.network.disconnect()

-- Low-level: Full networking configuration
bestow.network.core.registerReplicatedComponent("Transform3D", "Unreliable")
bestow.network.core.registerReplicatedComponent("Health", "Reliable")

-- Mark a component as dirty for replication
bestow.network.core.markDirty(player, "Transform3D")

-- RPCs
local damageRPC = bestow.network.core.registerRPC("ApplyDamage",
    function(sender, data)
        -- Handle incoming damage RPC
        print("Damage from " .. tostring(sender))
    end
)

-- Call RPC on server
bestow.network.core.callRPCServer(damageRPC, someData)

-- Call RPC on all clients
bestow.network.core.callRPCAll(damageRPC, someData)

-- Call RPC on specific client
bestow.network.core.callRPC(damageRPC, targetClientId, someData)

-- Lobby management (host only)
bestow.network.core.setMaxClients(8)
bestow.network.core.kickClient(troublemaker)

-- Per-client latency
local clients = bestow.network.core.getConnectedClients()
for _, client in ipairs(clients) do
    local ping = bestow.network.core.getPing(client)
    print("Client " .. tostring(client) .. " ping: " .. ping .. "s")
end

print("Average ping: " .. bestow.network.core.getAveragePing() .. "s")
print("Packet loss: " .. (bestow.network.core.getPacketLoss() * 100) .. "%")

-- Entity ownership
bestow.network.core.setOwner(npc, serverNetworkId)
local owner = bestow.network.core.getOwner(npc)
```

## C++ Examples

```cpp
// Initialize with config
NetworkConfig config{
    .protocol = NetworkConfig::Protocol::UDP,
    .tickRate = 60,
    .interpolationDelay = 0.1f,
    .enablePrediction = true,
    .enableReconciliation = true
};
networkCore->initialize(config);

// Host a game
auto hostResult = networkCore->host(7777, 16);
if (!hostResult) {
    spdlog::error("Failed to host: {}", hostResult.error().message);
}

// Or connect as client
networkCore->connect("192.168.1.100", 7777);

// Check status
if (networkCore->isConnected()) {
    NetworkId localId = networkCore->getLocalId();
    bool hosting = networkCore->isHost();
}

// Register replicated components
networkCore->registerReplicatedComponent("Transform3D", ReplicationMode::Unreliable);
networkCore->registerReplicatedComponent("Health", ReplicationMode::Reliable);

// Replicate an entity
networkCore->registerReplicatedEntity(player);
networkCore->setOwner(player, networkCore->getLocalId());

// Mark dirty when state changes
networkCore->markDirty(player, "Transform3D");

// RPCs
auto rpcResult = networkCore->registerRPC("ApplyDamage",
    [&](NetworkId sender, std::span<const std::byte> data) {
        // Deserialize damage data and apply
        spdlog::info("Damage RPC from client {}", sender.id);
    });

if (rpcResult) {
    RPCHandle damageRPC = rpcResult.value();

    // Call on specific client
    std::vector<std::byte> payload = serializeDamage(50.0f);
    networkCore->callRPC(damageRPC, targetClient, payload);

    // Call on all clients
    networkCore->callRPCAll(damageRPC, payload);

    // Call on server
    networkCore->callRPCServer(damageRPC, payload);
}

// Lobby
networkCore->setMaxClients(8);
networkCore->kickClient(troublemaker);

// Latency monitoring
auto clients = networkCore->getConnectedClients();
for (auto client : clients) {
    float ping = networkCore->getPing(client);
    spdlog::debug("Client {} ping: {:.1f}ms", client.id, ping * 1000.0f);
}
float avgPing = networkCore->getAveragePing();
float packetLoss = networkCore->getPacketLoss();

// Connection events
auto connectSub = networkCore->onClientConnected([](NetworkId id) {
    spdlog::info("Client connected: {}", id.id);
});

auto disconnectSub = networkCore->onClientDisconnected([](NetworkId id) {
    spdlog::info("Client disconnected: {}", id.id);
});

// Serialization helpers
auto bytes = networkCore->serializeEntity(player);
networkCore->deserializeEntity(remotePlayer, bytes);

// High-level usage
network->host(7777);
network->replicate(player);
bool mine = network->isLocallyOwned(player);
float ping = network->getPing();

network->disconnect();

// Cleanup
networkCore->unsubscribe(connectSub);
networkCore->unsubscribe(disconnectSub);
networkCore->shutdown();
```
