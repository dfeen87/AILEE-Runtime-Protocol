fetch('dashboard.json')
  .then(response => response.json())
  .then(data => {
      if (data && data.telemetry) {
          window.liveTelemetry = data.telemetry;
          if (!window.isReplayMode) {
              renderAllCharts(data.telemetry);
          }
      }
  })
  .catch(err => {
      console.error('Error loading deterministic telemetry:', err);
      document.getElementById('tickChart').innerHTML = '<div class="error">Failed to load deterministic telemetry</div>';
  });

function renderAllCharts(samples) {
    if (window.federationMode) {
        renderFederationView(samples);
    }
    renderTickChart(samples);
    renderCoherenceChart(samples);
    renderStateRootChart(samples);
}

// Federation Mode Logic
window.federationMode = false;
const federationToggle = document.getElementById('federationToggle');

federationToggle.addEventListener('click', () => {
    window.federationMode = !window.federationMode;
    if (window.federationMode) {
        federationToggle.style.backgroundColor = '#e53e3e';
        federationToggle.innerText = 'Federation Mode: ON';
        document.getElementById('federationViewContainer').style.display = 'block';
    } else {
        federationToggle.style.backgroundColor = '#2b6cb0';
        federationToggle.innerText = 'Federation Mode: OFF';
        document.getElementById('federationViewContainer').style.display = 'none';
    }

    // Re-render based on current state
    if (window.isReplayMode && replayData) {
        renderReplayTick(replayData, parseInt(replaySlider.value, 10));
    } else if (window.liveTelemetry) {
        renderAllCharts(window.liveTelemetry);
    }
});

function renderFederationView(samples) {
    if (!samples || samples.length === 0) return;

    // In a real implementation we would render data about multiple clusters and cross-cluster envelopes.
    // For now we simulate federation metrics based on the current cluster data or actual federation structures.
    const latest = samples[samples.length - 1];

    const summaryHtml = `
        <div class="deterministic-chart-data">
            <div class="metric"><span class="metric-label">Avg Coherence</span><span class="metric-value">${latest.global_coherence_score.toFixed(1)}%</span></div>
            <div class="metric"><span class="metric-label">In-Flight Envelopes</span><span class="metric-value">0</span></div>
            <div class="metric"><span class="metric-label">Total Clusters</span><span class="metric-value">1 (Simulated)</span></div>
        </div>
    `;
    document.getElementById('federationSummary').innerHTML = summaryHtml;
}

// Replay Mode Logic
window.isReplayMode = false;
let replayData = null;

const replayToggle = document.getElementById('replayToggle');
const replaySlider = document.getElementById('replaySlider');
const replayTickDisplay = document.getElementById('replayTickDisplay');

function parseReplayBinary(buffer) {
    const view = new DataView(buffer);
    
    // Check magic bytes "AILEE-RPL\0"
    const magic = new Uint8Array(buffer, 0, 10);
    const magicStr = String.fromCharCode.apply(null, magic);
    // Ignore exact string comparison in JS, just assume valid for now or check first chars
    
    // Version is at offset 10 (uint32)
    const version = view.getUint32(10, true);
    
    // Tick count is at offset 14 (uint64)
    // DataView doesn't have getUint64 across all browsers easily, but we can safely use BigInt or two Uint32
    // assuming little endian
    const tickCountLow = view.getUint32(14, true);
    const tickCountHigh = view.getUint32(18, true);
    const tickCount = tickCountLow; // Safe assuming max ticks < 4B
    
    // Size is dynamic, but telemetry is always at the end!
    // Since we know the struct sizes from C++:
    // We can parse the struct sequentially.
    let offset = 22; // Start after tick_count
    for (let i = 0; i < tickCount; i++) offset += 64; // skip ReplaySchedulerSnapshot
    
    for (let i = 0; i < tickCount; i++) {
        // Read dynamic length fields for ReplayClusterViewSnapshot
        const nodesSize = view.getUint32(offset, true);
        offset += 8; // skip 64 bit size
        for (let j = 0; j < nodesSize; j++) {
             offset += 1088; // ClusterNodeState fixed
             const syncSize = view.getUint32(offset, true);
             offset += 8;
             offset += (syncSize * 24); // l3::PeerSyncState
        }
        
        const envSize = view.getUint32(offset, true);
        offset += 8;
        offset += (envSize * 352); // MeshPropagationEnvelope
        
        const msgSize = view.getUint32(offset, true);
        offset += 8;
        offset += (msgSize * 168); // TransportMessage
        
        offset += 40; // padding of TransportQueue
        
        offset += 8; // total_nodes
        offset += 8; // total_steps
        offset += 128; // ClusterCoherenceSummary
    }
    
    const telemetrySamples = [];
    for (let i = 0; i < tickCount; i++) {
        const tick = view.getUint32(offset, true);
        const epoch = view.getUint32(offset + 8, true);
        const total_nodes = view.getUint32(offset + 16, true);
        const in_sync_nodes = view.getUint32(offset + 24, true);
        const consistent = view.getUint32(offset + 32, true);
        const inconsistent = view.getUint32(offset + 40, true);
        const score = view.getUint32(offset + 48, true);
        
        offset += 64; // skip entire 64 byte padding chunk
        
        telemetrySamples.push({
            tick: tick,
            epoch: epoch,
            total_nodes: total_nodes,
            in_sync_nodes: in_sync_nodes,
            consistent_state_root_nodes: consistent,
            inconsistent_state_root_nodes: inconsistent,
            global_coherence_score: score
        });
    }
    
    return {
        tickCount: tickCount,
        telemetry: telemetrySamples
    };
}

function loadReplay() {
    fetch('replay.bin')
        .then(response => response.arrayBuffer())
        .then(buffer => {
            replayData = parseReplayBinary(buffer);
            replaySlider.max = replayData.tickCount - 1;
            replaySlider.disabled = false;
            
            // Render first tick
            replaySlider.value = 0;
            renderReplayTick(replayData, 0);
        })
        .catch(err => {
            console.error('Failed to load replay binary:', err);
            alert('Failed to load replay.bin. Run the simulation first.');
            toggleReplayMode(); // Revert back
        });
}

function renderReplayTick(data, tickIndex) {
    replayTickDisplay.innerText = tickIndex;
    // We render up to the current tick to show progression, just like live telemetry array
    const samplesToRender = data.telemetry.slice(0, tickIndex + 1);
    renderAllCharts(samplesToRender);
}

function toggleReplayMode() {
    window.isReplayMode = !window.isReplayMode;
    if (window.isReplayMode) {
        replayToggle.style.backgroundColor = '#e53e3e';
        replayToggle.innerText = 'Exit Replay';
        loadReplay();
    } else {
        replayToggle.style.backgroundColor = '#4a5568';
        replayToggle.innerText = 'Replay Mode';
        replaySlider.disabled = true;
        
        if (window.liveTelemetry) {
            renderAllCharts(window.liveTelemetry);
        }
    }
}

replayToggle.addEventListener('click', toggleReplayMode);

replaySlider.addEventListener('input', (e) => {
    const tick = parseInt(e.target.value, 10);
    if (replayData) {
        renderReplayTick(replayData, tick);
    }
});

function renderTickChart(samples) {
    const ticks = samples.map(s => s.tick);
    const epochs = samples.map(s => s.epoch);

    const ctx = document.getElementById('tickChart');
    ctx.innerHTML = `
        <h3>Tick & Epoch Progression</h3>
        <div class="deterministic-chart-data">
            <div class="metric"><span class="metric-label">Ticks</span><span class="metric-value">[ ${ticks.join(', ')} ]</span></div>
            <div class="metric"><span class="metric-label">Epochs</span><span class="metric-value">[ ${epochs.join(', ')} ]</span></div>
        </div>
    `;
}

function renderCoherenceChart(samples) {
    const total = samples.map(s => s.total_nodes);
    const inSync = samples.map(s => s.in_sync_nodes);
    const scores = samples.map(s => s.global_coherence_score);

    const ctx = document.getElementById('coherenceChart');
    ctx.innerHTML = `
        <h3>Global Coherence</h3>
        <div class="deterministic-chart-data">
            <div class="metric"><span class="metric-label">Total Nodes</span><span class="metric-value">[ ${total.join(', ')} ]</span></div>
            <div class="metric"><span class="metric-label">In-Sync Nodes</span><span class="metric-value">[ ${inSync.join(', ')} ]</span></div>
            <div class="metric"><span class="metric-label">Coherence Score</span><span class="metric-value">[ ${scores.join(', ')} ]</span></div>
        </div>
    `;
}

function renderStateRootChart(samples) {
    const consistent = samples.map(s => s.consistent_state_root_nodes);
    const inconsistent = samples.map(s => s.inconsistent_state_root_nodes);

    const ctx = document.getElementById('stateRootChart');
    ctx.innerHTML = `
        <h3>State-Root Consistency</h3>
        <div class="deterministic-chart-data">
            <div class="metric"><span class="metric-label">Consistent Nodes</span><span class="metric-value">[ ${consistent.join(', ')} ]</span></div>
            <div class="metric"><span class="metric-label">Inconsistent Nodes</span><span class="metric-value">[ ${inconsistent.join(', ')} ]</span></div>
        </div>
    `;
}
