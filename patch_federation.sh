cat << 'DIFF' > patch.diff
--- src/l6/FederationExport.cpp
+++ src/l6/FederationExport.cpp
@@ -19,6 +19,15 @@
     return oss.str();
 }

+std::string to_hex(const uint8_t* data, size_t length) {
+    std::ostringstream oss;
+    oss << std::hex << std::setfill('0');
+    for (size_t i = 0; i < length; ++i) {
+        oss << std::setw(2) << static_cast<int>(data[i]);
+    }
+    return oss.str();
+}
+
 } // anonymous namespace

 ExternalClusterView FederationExport::export_view(const l4::ClusterView& view) {
@@ -31,7 +40,19 @@
     // We convert it to a double. Since it's integer based, it's deterministic.
     ext_view.coherence = static_cast<double>(view.coherence_summary.global_coherence_score);

+    ext_view.clock.height = view.clock.height;
+    ext_view.clock.consensus_time = view.clock.consensus_time;
+    ext_view.clock.interval_seconds = view.clock.interval_seconds;
+
+    ext_view.replay_events.reserve(view.replay_events.size());
+    for (const auto& internal_ev : view.replay_events) {
+        ExternalReplayEvent ext_ev;
+        ext_ev.type = static_cast<uint8_t>(internal_ev.type);
+        ext_ev.height = internal_ev.height;
+        ext_ev.block_hash = to_hex(internal_ev.block_hash.data(), internal_ev.block_hash.size());
+        ext_ev.txid = to_hex(internal_ev.txid.data(), internal_ev.txid.size());
+        ext_view.replay_events.push_back(ext_ev);
+    }
+
     ext_view.envelopes.reserve(view.mesh_envelopes.size());
     for (const auto& internal_env : view.mesh_envelopes) {
         ExternalEnvelope ext_env;
DIFF
patch -p0 < patch.diff
