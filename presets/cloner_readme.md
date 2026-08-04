# AILEE-Core Presets Guide for Cloners

Welcome to the AILEE-Core customization layer. This `presets/` directory is designed to give you out-of-the-box configurations for spinning up a deterministic sandbox, connecting to Bitcoin L1, and enforcing Governor rules on cross-chain Web3 transfers.

These presets guarantee safe defaults, meaning you won't accidentally mutate state based on malformed RPC data or insecure DeFi requests.

## Included Presets

1. **`rpc_defaults.json`**
   - Configures your real Bitcoin RPC endpoints, retries, and fallback behavior.
   - Defines strict rules for how 404s and null JSON values should be handled.

2. **`sandbox_defaults.json`**
   - Configures the internal `RpcSandboxSimulator`.
   - Use this to mock block heights, transaction IDs, latency, and specific failure modes (like 404s) without touching mainnet.
   - Ideal for CI/CD and offline deterministic testing.

3. **`governor_rules.json`**
   - Governs asset transfers across DeFi/Web3 middle-layers.
   - Set limits on transfer amounts based on reputation scores and asset types.
   - Configures the conditions under which AILEE-Core enters **Safe Mode** (e.g., losing the RPC endpoint).

4. **`handshake_domain.json`**
   - Domain invariants for establishing secure communication with node peers.
   - Enforces specific protocol versions and invariant validation rules.

## How to Use

Load these JSON files into your instance of AILEE-Core during initialization. For example, pass the Governor rules directly to the `PolicyRulesManager` to dynamically load risk boundaries.

> **Note:** These presets are designed to ensure Bitcoin's conservative protocol guarantees are preserved when integrating with middle-layer protocols. Modify them with caution.

*All logic is deterministic, safe, and PolyForm Noncommercial licensed.*
