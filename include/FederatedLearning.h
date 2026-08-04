// Licensed under the PolyForm Noncommercial License 1.0.0
// FederatedLearning.h — Production-Grade Federated Learning for AILEE-Core
// Privacy-preserving distributed ML with differential privacy, secure aggregation,
// Byzantine fault tolerance, ZK proof verification, and token incentive alignment.

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <chrono>
#include <functional>
#include <cstdint>
#include <atomic>

namespace ailee::fl {

// ==================== PRIVACY & SECURITY PARAMETERS ====================

struct PrivacyBudget {
    double epsilon = 1.0;           // Differential privacy epsilon
    double delta = 1e-5;            // Differential privacy delta
    double clipNorm = 1.0;          // Gradient clipping threshold
    double noiseMultiplier = 1.0;   // Gaussian noise scale
    bool enableSecureAggregation = true;
    bool enableHomomorphicEncryption = false;
};

enum class AggregationStrategy {
    FEDAVG,             // Standard FedAvg (simple averaging)
    FEDPROX,            // FedProx (proximal term for stability)
    FEDADAM,            // FedAdam (adaptive optimization)
    SCAFFOLD,           // SCAFFOLD (variance reduction)
    KRUM,               // Krum (Byzantine-robust)
    TRIMMED_MEAN,       // Trimmed mean (outlier rejection)
    MEDIAN,             // Coordinate-wise median (Byzantine-robust)
    BULYAN              // Bulyan (multi-Krum aggregation)
};

enum class CompressionMethod {
    NONE,
    QUANTIZATION,       // Quantize to lower precision (8-bit, 4-bit)
    SPARSIFICATION,     // Top-K or random-K sparsification
    LOW_RANK,           // Low-rank decomposition
    FEDERATED_DROPOUT,  // Drop random subset of weights
    SKETCHING           // Count sketch / random projection
};

// ==================== FEDERATED LEARNING TASK ====================

struct FLTask {
    // Identity
    std::string taskId;             // Unique task identifier
    std::string globalModelHash;    // SHA3-256 of initial global model
    std::string circuitId;          // ZK circuit for proof verification
    
    // Data specification
    std::string trainingDataHash;   // Optional commitment to data distribution
    std::optional<std::string> validationDataHash;
    std::size_t expectedSamplesPerWorker = 0;
    
    // Participation
    std::size_t minParticipants = 10;
    std::size_t maxParticipants = 1000;
    std::size_t currentParticipantCount = 0;
    
    // Training hyperparameters
    uint32_t localEpochs = 1;       // Epochs per worker per round
    uint32_t totalRounds = 100;     // Total federation rounds
    uint32_t currentRound = 0;
    double learningRate = 0.001;
    uint32_t batchSize = 32;
    
    // Privacy & security
    PrivacyBudget privacyBudget;
    AggregationStrategy aggregationStrategy = AggregationStrategy::FEDAVG;
    CompressionMethod compressionMethod = CompressionMethod::QUANTIZATION;
    
    // Economic incentives
    uint64_t rewardPerParticipant = 0;  // Base reward in protocol tokens
    double qualityMultiplier = 1.0;     // Bonus for high-quality updates
    double speedMultiplier = 1.0;       // Bonus for fast submission
    uint64_t totalRewardPool = 0;
    
    // Deadlines & timeouts
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point deadline;
    std::chrono::seconds roundTimeout{3600}; // 1 hour per round
    
    // Model metadata
    std::string modelArchitecture;      // e.g., "ResNet50", "BERT-base"
    std::size_t modelSizeBytes = 0;
    std::vector<std::string> requiredCapabilities; // e.g., ["GPU", "8GB RAM"]
    
    // Verification requirements
    bool requireZKProof = true;
    bool requireDataCommitment = false;
    uint32_t minProofSecurityBits = 128;
};

// ==================== LOCAL MODEL UPDATE ====================

struct LocalDelta {
    // Identity
    std::string workerId;           // Node that trained this delta
    std::string taskId;             // Which task this belongs to
    uint32_t roundNumber = 0;
    
    // Model update
    std::string modelHash;          // Must match FLTask globalModelHash
    std::vector<uint8_t> deltaBytes; // Compressed gradient/weight update
    CompressionMethod compression = CompressionMethod::NONE;
    
    // Training metadata
    std::size_t numSamplesTrained = 0;
    std::size_t numEpochs = 0;
    uint64_t trainingLossFp = 0;
    std::optional<uint64_t> validationLossFp;
    std::optional<uint64_t> validationAccuracyFp;
    
    // Privacy guarantees
    uint64_t epsilonSpentFp = 0;      // DP budget consumed
    uint64_t deltaSpentFp = 0;
    bool isDPNoisyUpdate = false;
    
    // Cryptographic proof
    std::vector<uint8_t> proofBytes; // ZK proof-of-correct-training
    std::string proofHash;           // SHA3-256 of proof
    bool proofVerified = false;
    
    // Secure aggregation (optional)
    std::optional<std::vector<uint8_t>> encryptedDelta; // Homomorphic encryption
    std::optional<std::string> secretSharingShare;      // Secret sharing scheme
    
    // Timing (Deterministic)
    uint64_t submissionTimestampMs = 0;
    uint64_t computeTimeMs = 0;
    
    // Signature (authenticity)
    std::optional<std::vector<uint8_t>> workerSignature;
    std::optional<std::string> workerPubkey;
    
    // Quality metrics (for reward calculation)
    double updateNorm = 0.0;        // L2 norm of update
    double cosineSimWithGlobal = 0.0; // Similarity to expected direction
    double contributionScore = 0.0; // Computed by aggregator
};

// ==================== AGGREGATION RESULT ====================

struct AggregationResult {
    bool success = false;
    std::string error;
    
    // Aggregated model
    std::vector<uint8_t> aggregatedModelBytes;
    std::string aggregatedModelHash;
    uint32_t roundNumber = 0;
    
    // Convergence metrics
    double globalLoss = 0.0;
    std::optional<double> globalAccuracy;
    double convergenceDelta = 0.0;   // Change from previous round
    bool hasConverged = false;
    
    // Participation statistics
    std::size_t participantsAccepted = 0;
    std::size_t participantsRejected = 0;
    std::vector<std::string> acceptedWorkers;
    std::vector<std::string> rejectedWorkers;
    std::unordered_map<std::string, std::string> rejectionReasons;
    
    // Byzantine detection
    std::vector<std::string> byzantineWorkersDetected;
    AggregationStrategy strategyUsed;
    
    // Privacy accounting
    double totalEpsilonSpent = 0.0;
    double totalDeltaSpent = 0.0;
    bool privacyBudgetExhausted = false;
    
    // Timing
    std::chrono::milliseconds aggregationTime{0};
    std::chrono::system_clock::time_point timestamp;
    
    // Next round preparation
    std::optional<FLTask> nextRoundTask;
};

// ==================== REWARD DISTRIBUTION ====================

struct RewardDistribution {
    std::string workerId;
    uint64_t baseReward = 0;
    uint64_t qualityBonus = 0;
    uint64_t speedBonus = 0;
    uint64_t totalReward = 0;
    double contributionFraction = 0.0; // Percentage of total contribution
    std::string reason;
    bool paid = false;
};

// ==================== AGGREGATOR INTERFACE ====================

class IAggregator {
public:
    virtual ~IAggregator() = default;
    
    // ========== Job Management ==========
    
    // Post new federated learning job
    virtual bool postJob(const FLTask& jobSpec, std::string* err) = 0;
    
    // Get current job status
    virtual std::optional<FLTask> getJob(const std::string& taskId) const = 0;
    
    // Cancel job (abort training)
    virtual bool cancelJob(const std::string& taskId, std::string* err) = 0;
    
    // Get list of active jobs
    virtual std::vector<std::string> getActiveJobs() const = 0;
    
    // ========== Update Collection ==========
    
    // Accept local delta from participant (with verification)
    virtual bool acceptDelta(const LocalDelta& delta, std::string* err) = 0;
    
    // Accept batch of deltas (optimized)
    virtual std::vector<bool> acceptDeltaBatch(
        const std::vector<LocalDelta>& deltas,
        std::vector<std::string>* errors) = 0;
    
    // Get pending deltas for task
    virtual std::vector<LocalDelta> getPendingDeltas(const std::string& taskId) const = 0;
    
    // Get delta count for task
    virtual std::size_t getDeltaCount(const std::string& taskId) const = 0;
    
    // ========== Aggregation ==========
    
    // Aggregate collected deltas into new global model
    virtual AggregationResult aggregate() = 0;
    
    // Aggregate specific task
    virtual AggregationResult aggregateTask(const std::string& taskId) = 0;
    
    // Check if ready to aggregate (enough participants)
    virtual bool isReadyToAggregate(const std::string& taskId) const = 0;
    
    // Force aggregation (even if min participants not met)
    virtual AggregationResult forceAggregate(const std::string& taskId) = 0;
    
    // ========== Byzantine Fault Tolerance ==========
    
    // Detect Byzantine workers (outlier detection)
    virtual std::vector<std::string> detectByzantineWorkers(
        const std::string& taskId) const = 0;
    
    // Blacklist worker (prevent future participation)
    virtual bool blacklistWorker(const std::string& workerId, std::string* reason) = 0;
    
    // Check if worker is blacklisted
    virtual bool isBlacklisted(const std::string& workerId) const = 0;
    
    // Get worker reputation score
    virtual double getWorkerReputation(const std::string& workerId) const = 0;
    
    // ========== Reward Management ==========
    
    // Calculate rewards for round
    virtual std::vector<RewardDistribution> calculateRewards(
        const std::string& taskId,
        const AggregationResult& result) const = 0;
    
    // Distribute rewards to participants
    virtual bool distributeRewards(
        const std::vector<RewardDistribution>& distributions,
        std::string* err) = 0;
    
    // Get total rewards paid for task
    virtual uint64_t getTotalRewardsPaid(const std::string& taskId) const = 0;
    
    // ========== Privacy & Security ==========
    
    // Get remaining privacy budget for task
    virtual PrivacyBudget getRemainingPrivacyBudget(const std::string& taskId) const = 0;
    
    // Enable secure aggregation (encrypted updates)
    virtual bool enableSecureAggregation(const std::string& taskId, bool enable) = 0;
    
    // Verify all proofs for task
    virtual std::unordered_map<std::string, bool> verifyAllProofs(
        const std::string& taskId) const = 0;
    
    // ========== Model Management ==========
    
    // Get current global model
    virtual std::optional<std::vector<uint8_t>> getGlobalModel(
        const std::string& taskId) const = 0;
    
    // Update global model manually (for initialization)
    virtual bool updateGlobalModel(
        const std::string& taskId,
        const std::vector<uint8_t>& modelBytes,
        std::string* err) = 0;
    
    // Export model history (all rounds)
    virtual std::vector<std::vector<uint8_t>> getModelHistory(
        const std::string& taskId) const = 0;
    
    // ========== Telemetry & Monitoring ==========
    
    struct AggregatorMetrics {
        uint64_t totalJobsPosted = 0;
        uint64_t totalJobsCompleted = 0;
        uint64_t totalDeltasReceived = 0;
        uint64_t totalDeltasRejected = 0;
        uint64_t totalAggregations = 0;
        std::chrono::milliseconds avgAggregationTime{0};
        std::size_t totalRewardsDistributed = 0;
        std::size_t totalByzantineDetections = 0;
    };
    virtual AggregatorMetrics getMetrics() const = 0;
    virtual void resetMetrics() = 0;
    
    // Set callback for aggregation events
    using EventCallback = std::function<void(const std::string& event, 
                                              const AggregationResult& result)>;
    virtual void setEventCallback(EventCallback callback) = 0;
    
    // Export audit log
    virtual std::vector<std::string> exportAuditLog() const = 0;
};

// ==================== PARTICIPANT INTERFACE ====================

class IParticipant {
public:
    virtual ~IParticipant() = default;
    
    // ========== Training ==========
    
    // Train locally on private data; produce delta + proof
    virtual std::optional<LocalDelta> trainAndProve(
        const FLTask& jobSpec,
        std::string* err) = 0;
    
    // Train with custom local dataset
    virtual std::optional<LocalDelta> trainWithData(
        const FLTask& jobSpec,
        const std::vector<uint8_t>& localData,
        std::string* err) = 0;
    
    // Resume training from checkpoint
    virtual std::optional<LocalDelta> resumeTraining(
        const FLTask& jobSpec,
        const std::vector<uint8_t>& checkpointBytes,
        std::string* err) = 0;
    
    // ========== Model Management ==========
    
    // Download global model from aggregator
    virtual std::optional<std::vector<uint8_t>> downloadGlobalModel(
        const std::string& taskId) const = 0;
    
    // Validate model before training (integrity check)
    virtual bool validateModel(
        const std::vector<uint8_t>& modelBytes,
        const std::string& expectedHash) const = 0;
    
    // Get local model (after training)
    virtual std::optional<std::vector<uint8_t>> getLocalModel() const = 0;
    
    // ========== Privacy ==========
    
    // Configure differential privacy parameters
    virtual void setPrivacyBudget(const PrivacyBudget& budget) = 0;
    
    // Get remaining privacy budget
    virtual PrivacyBudget getRemainingPrivacyBudget() const = 0;
    
    // Enable/disable DP noise injection
    virtual void enableDifferentialPrivacy(bool enable) = 0;
    
    // ========== Data Management ==========
    
    // Load local training data
    virtual bool loadLocalData(const std::vector<uint8_t>& data, std::string* err) = 0;
    
    // Validate data quality (check for poisoning)
    virtual bool validateLocalData(std::string* err) const = 0;
    
    // Get data statistics (for debugging, privacy-safe)
    struct DataStats {
        std::size_t numSamples = 0;
        std::size_t numFeatures = 0;
        std::vector<double> classCounts;
        double avgSampleSize = 0.0;
    };
    virtual DataStats getDataStats() const = 0;
    
    // ========== Proof Generation ==========
    
    // Generate ZK proof of correct training
    virtual std::optional<std::vector<uint8_t>> generateProof(
        const LocalDelta& delta) const = 0;
    
    // Verify own proof (sanity check)
    virtual bool verifySelfProof(const LocalDelta& delta) const = 0;
    
    // ========== Telemetry ==========
    
    struct ParticipantMetrics {
        uint64_t totalRoundsParticipated = 0;
        uint64_t totalSamplesTrained = 0;
        std::chrono::milliseconds totalTrainingTime{0};
        std::chrono::milliseconds avgRoundTime{0};
        double avgTrainingLoss = 0.0;
        uint64_t totalRewardsEarned = 0;
        uint64_t timesRejected = 0;
    };
    virtual ParticipantMetrics getMetrics() const = 0;
    virtual void resetMetrics() = 0;
    
    // Get worker ID
    virtual std::string getWorkerId() const = 0;
    
    // Get worker capabilities (for task matching)
    virtual std::vector<std::string> getCapabilities() const = 0;
    
    // Check if worker can participate in task
    virtual bool canParticipate(const FLTask& task) const = 0;
};

// ==================== FACTORY & UTILITIES ====================

class FederatedLearningFactory {
public:
    // Create aggregator instance
    static std::unique_ptr<IAggregator> createAggregator(
        AggregationStrategy strategy = AggregationStrategy::FEDAVG);
    
    // Create participant instance
    static std::unique_ptr<IParticipant> createParticipant(
        const std::string& workerId);
    
    // Get available aggregation strategies
    static std::vector<AggregationStrategy> getAvailableStrategies();
    
    // Get strategy name as string
    static std::string getStrategyName(AggregationStrategy strategy);
    
    // Parse strategy from string
    static std::optional<AggregationStrategy> parseStrategy(const std::string& name);
};

// ==================== SECURE AGGREGATION PROTOCOLS ====================

namespace secure_aggregation {
    // Pairwise masking (each worker masks with neighbors)
    class PairwiseMasking {
    public:
        static std::vector<uint8_t> maskDelta(
            const std::vector<uint8_t>& delta,
            const std::vector<std::string>& neighborIds);
        
        static std::vector<uint8_t> unmaskAggregate(
            const std::vector<std::vector<uint8_t>>& maskedDeltas);
    };
    
    // Homomorphic encryption (additive HE for aggregation)
    class HomomorphicAggregation {
    public:
        static std::vector<uint8_t> encryptDelta(
            const std::vector<uint8_t>& delta,
            const std::string& publicKey);
        
        static std::vector<uint8_t> aggregateEncrypted(
            const std::vector<std::vector<uint8_t>>& encryptedDeltas);
        
        static std::vector<uint8_t> decryptAggregate(
            const std::vector<uint8_t>& encryptedAggregate,
            const std::string& privateKey);
    };
}

// ==================== COMPRESSION UTILITIES ====================

namespace compression {
    // Quantize gradients to lower precision
    std::vector<uint8_t> quantize(const std::vector<uint8_t>& delta, int bits);
    std::vector<uint8_t> dequantize(const std::vector<uint8_t>& quantized, int bits);
    
    // Top-K sparsification
    std::vector<uint8_t> sparsify(const std::vector<uint8_t>& delta, double sparsityRatio);
    std::vector<uint8_t> densify(const std::vector<uint8_t>& sparse);
    
    // Estimate compression ratio
    double estimateCompressionRatio(CompressionMethod method, int bits = 8);
}

// ==================== BYZANTINE DETECTION ====================

namespace byzantine {
    // Krum: Select most similar updates
    std::vector<std::string> detectKrum(
        const std::vector<LocalDelta>& deltas,
        std::size_t numByzantine);
    
    // Trimmed mean: Remove outliers
    std::vector<std::string> detectTrimmedMean(
        const std::vector<LocalDelta>& deltas,
        double trimFraction);
    
    // Median: Coordinate-wise median
    std::vector<std::string> detectMedian(
        const std::vector<LocalDelta>& deltas);
    
    // Statistical outlier detection
    std::vector<std::string> detectStatisticalOutliers(
        const std::vector<LocalDelta>& deltas,
        double threshold = 3.0);
}

// ==================== INTEGRATION WITH AILEE-CORE ====================

namespace integration {
    // Create FL task for AmbientAI distributed training
    FLTask createAmbientAITask(
        const std::string& modelHash,
        std::size_t minNodes,
        uint64_t rewardPool);
    
    // Verify FL proof using ZK verifier
    bool verifyFLProof(
        const LocalDelta& delta,
        const std::string& verificationKeyId);
    
    // Calculate token reward for FL participant
    RewardDistribution calculateFLReward(
        const LocalDelta& delta,
        const AggregationResult& result,
        double baseRewardRate);
    
    // Integrate with NetFlow for model distribution
    bool distributeModelViaNetFlow(
        const std::vector<uint8_t>& modelBytes,
        const std::vector<std::string>& targetNodes);
}

} // namespace ailee::fl
