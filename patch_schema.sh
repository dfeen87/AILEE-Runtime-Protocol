cat << 'DIFF' > patch.diff
--- include/l6/ExternalSchema.h
+++ include/l6/ExternalSchema.h
@@ -10,12 +10,27 @@
     std::string payload_hash;
 };

+struct ExternalBitcoinClockState {
+    uint64_t height;
+    double consensus_time;
+    double interval_seconds;
+};
+
+struct ExternalReplayEvent {
+    uint8_t type;
+    uint64_t height;
+    std::string block_hash;
+    std::string txid;
+};
+
 struct ExternalClusterView {
     size_t node_count;
     double coherence;
     std::vector<ExternalEnvelope> envelopes;
+    ExternalBitcoinClockState clock;
+    std::vector<ExternalReplayEvent> replay_events;
 };

 struct ExternalReplayTick {
DIFF
patch -p0 < patch.diff
