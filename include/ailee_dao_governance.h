/**
 * AILEE DAO Governance System
 * 
 * Decentralized governance for protocol upgrades, AI parameter tuning,
 * validator management, and treasury allocation. Ensures no single entity
 * controls the AILEE network evolution.
 * 
 * Implements:
 * - Proposal submission and voting
 * - Quadratic voting with stake weighting
 * - Time-locked execution
 * - Emergency override mechanisms
 * - Validator reputation scoring
 * - Treasury management for development funding
 * 
 * Licensed under the PolyForm Noncommercial License 1.0.0
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_DAO_GOVERNANCE_H
#define AILEE_DAO_GOVERNANCE_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <map>
#include <set>
#include <optional>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <openssl/sha.h>

namespace ailee {

// Governance constants
constexpr uint64_t MIN_PROPOSAL_STAKE = 1000;        // Min ADU to submit proposal
constexpr uint64_t VOTING_PERIOD_DAYS = 14;          // 2 weeks voting window
constexpr uint64_t TIMELOCK_PERIOD_DAYS = 7;         // 1 week execution delay
constexpr double QUORUM_PERCENT = 10.0;              // 10% of total stake must vote
constexpr double APPROVAL_THRESHOLD_PERCENT = 66.67; // 2/3 supermajority
constexpr double EMERGENCY_THRESHOLD_PERCENT = 80.0; // 80% for emergency actions
constexpr size_t MAX_ACTIVE_PROPOSALS = 10;          // Prevent spam

/**
 * Proposal Types
 */
enum class ProposalType {
    PARAMETER_CHANGE,      // Modify AI optimization parameters
    PROTOCOL_UPGRADE,      // Soft/hard fork proposals
    VALIDATOR_ADDITION,    // Add new validator to network
    VALIDATOR_REMOVAL,     // Remove malicious validator
    TREASURY_ALLOCATION,   // Fund development/research
    EMERGENCY_HALT,        // Emergency circuit breaker override
    CONSTITUTION_AMENDMENT // Change governance rules themselves
};

/**
 * Vote Choice
 */
enum class VoteChoice {
    ABSTAIN = 0,
    FOR = 1,
    AGAINST = 2
};

/**
 * Proposal Status
 */
enum class ProposalStatus {
    DRAFT,           // Being prepared
    ACTIVE,          // Open for voting
    SUCCEEDED,       // Passed, awaiting timelock
    DEFEATED,        // Failed to meet threshold
    QUEUED,          // In timelock period
    EXECUTED,        // Successfully implemented
    CANCELLED,       // Withdrawn by proposer
    EXPIRED          // Voting period ended without quorum
};

/**
 * Stake Holder - Represents ADU token holder with voting power
 */
class StakeHolder {
public:
    struct StakeData {
        std::string address;
        uint64_t stakedAmount;
        uint64_t lockedUntil;
        double reputationScore;      // 0.0 to 1.0
        uint64_t proposalsSubmitted;
        uint64_t votesParticipated;
        bool isValidator;
    };

    StakeHolder(const std::string& addr, uint64_t stake)
        : address_(addr), stakedAmount_(stake) {
        data_.address = addr;
        data_.stakedAmount = stake;
        data_.lockedUntil = 0;
        data_.reputationScore = 0.5; // Start neutral
        data_.proposalsSubmitted = 0;
        data_.votesParticipated = 0;
        data_.isValidator = false;
    }

    // Quadratic voting power: sqrt(stake) * reputation
    double getVotingPower() const {
        return std::sqrt(static_cast<double>(data_.stakedAmount)) * 
               data_.reputationScore;
    }

    void increaseReputation(double amount) {
        data_.reputationScore = std::min(1.0, data_.reputationScore + amount);
    }

    void decreaseReputation(double amount) {
        data_.reputationScore = std::max(0.0, data_.reputationScore - amount);
    }

    void recordProposal() { data_.proposalsSubmitted++; }
    void recordVote() { data_.votesParticipated++; }

    const StakeData& getData() const { return data_; }
    uint64_t getStake() const { return data_.stakedAmount; }
    std::string getAddress() const { return address_; }

private:
    std::string address_;
    uint64_t stakedAmount_;
    StakeData data_;
};

/**
 * Governance Proposal
 */
class Proposal {
public:
    struct ProposalData {
        std::string proposalId;
        std::string title;
        std::string description;
        ProposalType type;
        std::string proposer;
        uint64_t submissionTime;
        uint64_t votingStartTime;
        uint64_t votingEndTime;
        uint64_t executionTime;
        ProposalStatus status;
        
        // Voting tallies
        double votesFor;
        double votesAgainst;
        double votesAbstain;
        double totalVotingPower;
        
        // Execution data
        std::vector<uint8_t> executionPayload;
        std::string targetContract;
        
        // Metadata
        std::map<std::string, std::string> parameters;
        std::vector<std::string> supportingDocuments;
    };

    Proposal(
        const std::string& title,
        const std::string& description,
        ProposalType type,
        const std::string& proposer
    ) {
        data_.proposalId = generateProposalId(title, proposer);
        data_.title = title;
        data_.description = description;
        data_.type = type;
        data_.proposer = proposer;
        data_.submissionTime = getCurrentTimestamp();
        data_.status = ProposalStatus::DRAFT;
        data_.votesFor = 0.0;
        data_.votesAgainst = 0.0;
        data_.votesAbstain = 0.0;
        data_.totalVotingPower = 0.0;
    }

    bool activate() {
        if (data_.status != ProposalStatus::DRAFT) return false;
        
        uint64_t now = getCurrentTimestamp();
        data_.votingStartTime = now;
        data_.votingEndTime = now + (VOTING_PERIOD_DAYS * 24 * 3600);
        data_.status = ProposalStatus::ACTIVE;
        
        return true;
    }

    bool recordVote(const std::string& voter, VoteChoice choice, double votingPower) {
        if (data_.status != ProposalStatus::ACTIVE) return false;
        
        uint64_t now = getCurrentTimestamp();
        if (now < data_.votingStartTime || now > data_.votingEndTime) {
            return false;
        }

        // Prevent double voting (simplified - in production use merkle tree)
        if (voters_.count(voter) > 0) return false;
        
        voters_.insert(voter);
        
        switch (choice) {
            case VoteChoice::FOR:
                data_.votesFor += votingPower;
                break;
            case VoteChoice::AGAINST:
                data_.votesAgainst += votingPower;
                break;
            case VoteChoice::ABSTAIN:
                data_.votesAbstain += votingPower;
                break;
        }
        
        data_.totalVotingPower += votingPower;
        return true;
    }

    bool finalizeVoting(double totalNetworkStake) {
        if (data_.status != ProposalStatus::ACTIVE) return false;
        
        uint64_t now = getCurrentTimestamp();
        if (now < data_.votingEndTime) return false;
        
        // Check quorum
        double quorumRequired = totalNetworkStake * (QUORUM_PERCENT / 100.0);
        if (data_.totalVotingPower < quorumRequired) {
            data_.status = ProposalStatus::EXPIRED;
            return false;
        }
        
        // Calculate approval percentage
        double totalVotes = data_.votesFor + data_.votesAgainst;
        if (totalVotes == 0) {
            data_.status = ProposalStatus::DEFEATED;
            return false;
        }
        
        double approvalPercent = (data_.votesFor / totalVotes) * 100.0;
        
        // Determine threshold based on proposal type
        double requiredThreshold = APPROVAL_THRESHOLD_PERCENT;
        if (data_.type == ProposalType::EMERGENCY_HALT ||
            data_.type == ProposalType::CONSTITUTION_AMENDMENT) {
            requiredThreshold = EMERGENCY_THRESHOLD_PERCENT;
        }
        
        if (approvalPercent >= requiredThreshold) {
            data_.status = ProposalStatus::SUCCEEDED;
            data_.executionTime = now + (TIMELOCK_PERIOD_DAYS * 24 * 3600);
            return true;
        } else {
            data_.status = ProposalStatus::DEFEATED;
            return false;
        }
    }

    bool queueForExecution() {
        if (data_.status != ProposalStatus::SUCCEEDED) return false;
        data_.status = ProposalStatus::QUEUED;
        return true;
    }

    bool canExecute() const {
        return data_.status == ProposalStatus::QUEUED &&
               getCurrentTimestamp() >= data_.executionTime;
    }

    bool execute() {
        if (!canExecute()) return false;
        
        data_.status = ProposalStatus::EXECUTED;
        return true;
    }

    bool cancel() {
        if (data_.status == ProposalStatus::EXECUTED) return false;
        data_.status = ProposalStatus::CANCELLED;
        return true;
    }

    void addParameter(const std::string& key, const std::string& value) {
        data_.parameters[key] = value;
    }

    void addDocument(const std::string& documentHash) {
        data_.supportingDocuments.+= documentHash);
    }

    const ProposalData& getData() const { return data_; }
    ProposalStatus getStatus() const { return data_.status; }
    std::string getId() const { return data_.proposalId; }

private:
    ProposalData data_;
    std::set<std::string> voters_;

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }

    static std::string generateProposalId(const std::string& title, 
                                         const std::string& proposer) {
        uint64_t timestamp = getCurrentTimestamp();
        std::string combined = title + proposer + std::to_string(timestamp);
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(combined.data()),
               combined.size(), hash);
        
        char hexStr[65];
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            sprintf(hexStr + (i * 2), "%02x", hash[i]);
        }
        return std::string(hexStr, 64);
    }
};

/**
 * Treasury Manager
 * Manages funds for development, research, and grants
 */
class Treasury {
public:
    struct TreasuryAllocation {
        std::string allocationId;
        std::string proposalId;
        std::string recipient;
        uint64_t amount;
        std::string purpose;
        uint64_t releaseTime;
        bool released;
        std::vector<std::string> milestones;
        size_t milestonesCompleted;
    };

    Treasury(uint64_t initialBalance) 
        : balance_(initialBalance), totalAllocated_(0) {}

    std::string createAllocation(
        const std::string& proposalId,
        const std::string& recipient,
        uint64_t amount,
        const std::string& purpose,
        const std::vector<std::string>& milestones
    ) {
        if (amount > getAvailableBalance()) return "";
        
        TreasuryAllocation allocation;
        allocation.allocationId = generateAllocationId(recipient, amount);
        allocation.proposalId = proposalId;
        allocation.recipient = recipient;
        allocation.amount = amount;
        allocation.purpose = purpose;
        allocation.releaseTime = getCurrentTimestamp() + (30 * 24 * 3600); // 30 days
        allocation.released = false;
        allocation.milestones = milestones;
        allocation.milestonesCompleted = 0;
        
        allocations_[allocation.allocationId] = allocation;
        totalAllocated_ += amount;
        
        return allocation.allocationId;
    }

    bool releaseAllocation(const std::string& allocationId) {
        auto it = allocations_.find(allocationId);
        if (it == allocations_.end()) return false;
        
        TreasuryAllocation& allocation = it->second;
        
        if (allocation.released) return false;
        if (getCurrentTimestamp() < allocation.releaseTime) return false;
        
        // Check milestone completion (simplified)
        if (allocation.milestonesCompleted < allocation.milestones.size() / 2) {
            return false; // Need at least 50% milestone completion
        }
        
        allocation.released = true;
        balance_ -= allocation.amount;
        totalAllocated_ -= allocation.amount;
        
        return true;
    }

    bool completeMilestone(const std::string& allocationId, size_t milestoneIndex) {
        auto it = allocations_.find(allocationId);
        if (it == allocations_.end()) return false;
        
        TreasuryAllocation& allocation = it->second;
        
        if (milestoneIndex >= allocation.milestones.size()) return false;
        
        allocation.milestonesCompleted++;
        return true;
    }

    void addFunds(uint64_t amount) {
        balance_ += amount;
    }

    uint64_t getBalance() const { return balance_; }
    uint64_t getAvailableBalance() const { return balance_ - totalAllocated_; }
    uint64_t getTotalAllocated() const { return totalAllocated_; }

    const std::map<std::string, TreasuryAllocation>& getAllocations() const {
        return allocations_;
    }

private:
    uint64_t balance_;
    uint64_t totalAllocated_;
    std::map<std::string, TreasuryAllocation> allocations_;

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }

    static std::string generateAllocationId(const std::string& recipient, 
                                           uint64_t amount) {
        uint64_t timestamp = getCurrentTimestamp();
        std::string combined = recipient + std::to_string(amount) + 
                              std::to_string(timestamp);
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(combined.data()),
               combined.size(), hash);
        
        char hexStr[65];
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            sprintf(hexStr + (i * 2), "%02x", hash[i]);
        }
        return std::string(hexStr, 64);
    }
};

/**
 * Validator Registry
 * Manages validator set through governance
 */
class ValidatorRegistry {
public:
    struct ValidatorInfo {
        std::string address;
        std::string identity;
        uint64_t stake;
        uint64_t joinedTime;
        double performanceScore;
        uint64_t blocksValidated;
        uint64_t slashingEvents;
        bool active;
    };

    bool addValidator(
        const std::string& address,
        const std::string& identity,
        uint64_t stake
    ) {
        if (validators_.count(address) > 0) return false;
        
        ValidatorInfo info;
        info.address = address;
        info.identity = identity;
        info.stake = stake;
        info.joinedTime = getCurrentTimestamp();
        info.performanceScore = 1.0;
        info.blocksValidated = 0;
        info.slashingEvents = 0;
        info.active = true;
        
        validators_[address] = info;
        return true;
    }

    bool removeValidator(const std::string& address) {
        auto it = validators_.find(address);
        if (it == validators_.end()) return false;
        
        it->second.active = false;
        return true;
    }

    bool slashValidator(const std::string& address, double penalty) {
        auto it = validators_.find(address);
        if (it == validators_.end()) return false;
        
        ValidatorInfo& validator = it->second;
        validator.slashingEvents++;
        validator.performanceScore = std::max(0.0, 
            validator.performanceScore - penalty);
        
        // Automatic removal after 3 slashing events
        if (validator.slashingEvents >= 3) {
            validator.active = false;
        }
        
        return true;
    }

    void recordValidation(const std::string& address) {
        auto it = validators_.find(address);
        if (it != validators_.end() && it->second.active) {
            it->second.blocksValidated++;
            
            // Slowly improve performance score
            it->second.performanceScore = std::min(1.0,
                it->second.performanceScore + 0.001);
        }
    }

    std::vector<std::string> getActiveValidators() const {
        std::vector<std::string> active;
        for (const auto& pair : validators_) {
            if (pair.second.active) {
                active.+= pair.first);
            }
        }
        return active;
    }

    const std::map<std::string, ValidatorInfo>& getAllValidators() const {
        return validators_;
    }

private:
    std::map<std::string, ValidatorInfo> validators_;

    static uint64_t getCurrentTimestamp() {
        return static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000000
        );
    }
};

/**
 * DAO Governance Manager
 * Main orchestrator for decentralized governance
 */
class DAOGovernance {
public:
    DAOGovernance(uint64_t initialTreasuryBalance)
        : treasury_(std::make_unique<Treasury>(initialTreasuryBalance)),
          validatorRegistry_(std::make_unique<ValidatorRegistry>()),
          totalNetworkStake_(0) {}

    // Stake management
    bool registerStakeHolder(const std::string& address, uint64_t stake) {
        if (stakeHolders_.count(address) > 0) return false;
        
        auto holder = std::make_shared<StakeHolder>(address, stake);
        stakeHolders_[address] = holder;
        totalNetworkStake_ += stake;
        
        return true;
    }

    bool increaseStake(const std::string& address, uint64_t additionalStake) {
        auto it = stakeHolders_.find(address);
        if (it == stakeHolders_.end()) return false;
        
        // In production, this would require actual token transfer
        totalNetworkStake_ += additionalStake;
        return true;
    }

    // Proposal management
    std::string submitProposal(
        const std::string& proposer,
        const std::string& title,
        const std::string& description,
        ProposalType type
    ) {
        auto holderIt = stakeHolders_.find(proposer);
        if (holderIt == stakeHolders_.end()) return "";
        
        if (holderIt->second->getStake() < MIN_PROPOSAL_STAKE) {
            return ""; // Insufficient stake
        }
        
        if (activeProposals_.size() >= MAX_ACTIVE_PROPOSALS) {
            return ""; // Too many active proposals
        }
        
        auto proposal = std::make_shared<Proposal>(title, description, type, proposer);
        
        std::string proposalId = proposal->getId();
        proposals_[proposalId] = proposal;
        
        holderIt->second->recordProposal();
        holderIt->second->increaseReputation(0.01); // Reward participation
        
        return proposalId;
    }

    bool activateProposal(const std::string& proposalId) {
        auto it = proposals_.find(proposalId);
        if (it == proposals_.end()) return false;
        
        if (it->second->activate()) {
            activeProposals_.insert(proposalId);
            return true;
        }
        return false;
    }

    bool vote(
        const std::string& proposalId,
        const std::string& voter,
        VoteChoice choice
    ) {
        auto proposalIt = proposals_.find(proposalId);
        if (proposalIt == proposals_.end()) return false;
        
        auto holderIt = stakeHolders_.find(voter);
        if (holderIt == stakeHolders_.end()) return false;
        
        double votingPower = holderIt->second->getVotingPower();
        
        if (proposalIt->second->recordVote(voter, choice, votingPower)) {
            holderIt->second->recordVote();
            holderIt->second->increaseReputation(0.005); // Reward voting
            return true;
        }
        
        return false;
    }

    bool finalizeProposal(const std::string& proposalId) {
        auto it = proposals_.find(proposalId);
        if (it == proposals_.end()) return false;
        
        bool result = it->second->finalizeVoting(
            std::sqrt(static_cast<double>(totalNetworkStake_))
        );
        
        if (result && it->second->getStatus() == ProposalStatus::SUCCEEDED) {
            it->second->queueForExecution();
        }
        
        activeProposals_.erase(proposalId);
        return result;
    }

    bool executeProposal(const std::string& proposalId) {
        auto it = proposals_.find(proposalId);
        if (it == proposals_.end()) return false;
        
        if (!it->second->canExecute()) return false;
        
        // Execute based on proposal type
        bool executed = executeProposalLogic(it->second);
        
        if (executed) {
            it->second->execute();
            
            // Reward proposer for successful proposal
            auto holderIt = stakeHolders_.find(it->second->getData().proposer);
            if (holderIt != stakeHolders_.end()) {
                holderIt->second->increaseReputation(0.05);
            }
        }
        
        return executed;
    }

    // Accessors
    Treasury* getTreasury() { return treasury_.get(); }
    ValidatorRegistry* getValidatorRegistry() { return validatorRegistry_.get(); }
    
    std::shared_ptr<Proposal> getProposal(const std::string& proposalId) {
        auto it = proposals_.find(proposalId);
        return (it != proposals_.end()) ? it->second : nullptr;
    }

    std::vector<std::string> getActiveProposals() const {
        return std::vector<std::string>(activeProposals_.begin(), 
                                       activeProposals_.end());
    }

    uint64_t getTotalNetworkStake() const { return totalNetworkStake_; }

private:
    std::unique_ptr<Treasury> treasury_;
    std::unique_ptr<ValidatorRegistry> validatorRegistry_;
    std::map<std::string, std::shared_ptr<StakeHolder>> stakeHolders_;
    std::map<std::string, std::shared_ptr<Proposal>> proposals_;
    std::set<std::string> activeProposals_;
    uint64_t totalNetworkStake_;

    bool executeProposalLogic(std::shared_ptr<Proposal> proposal) {
        // In production, this would trigger actual protocol changes
        switch (proposal->getData().type) {
            case ProposalType::TREASURY_ALLOCATION: {
                // Extract parameters and create treasury allocation
                auto& params = proposal->getData().parameters;
                if (params.count("recipient") && params.count("amount")) {
                    uint64_t amount = std::stoull(params["amount"]);
                    treasury_->createAllocation(
                        proposal->getId(),
                        params["recipient"],
                        amount,
                        params["purpose"],
                        {}
                    );
                }
                return true;
            }
            case ProposalType::VALIDATOR_ADDITION: {
                auto& params = proposal->getData().parameters;
                if (params.count("address") && params.count("stake")) {
                    validatorRegistry_->addValidator(
                        params["address"],
                        params["identity"],
                        std::stoull(params["stake"])
                    );
                }
                return true;
            }
            case ProposalType::VALIDATOR_REMOVAL: {
                auto& params = proposal->getData().parameters;
                if (params.count("address")) {
                    validatorRegistry_->removeValidator(params["address"]);
                }
                return true;
            }
            default:
                // Other proposal types would be handled here
                return true;
        }
    }
};

} // namespace ailee

#endif // AILEE_DAO_GOVERNANCE_H
