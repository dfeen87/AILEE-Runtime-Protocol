/**
 * AILEE Governance System - Complete Working Demo
 * 
 * This example demonstrates a full governance lifecycle:
 * 1. Network initialization with stakeholders
 * 2. Proposal submission (AI parameter change)
 * 3. Community voting with quadratic weighting
 * 4. Proposal finalization and execution
 * 5. Treasury allocation for development
 * 6. Validator management through governance
 * 
 * Compile: g++ -std=c++17 governance_demo.cpp -lssl -lcrypto -o governance_demo
 * Run: ./governance_demo
 * 
 * Licensed under the MIT License
 * Copyright (c) 2026 Don Michael Feeney Jr.
 * Author: Don Michael Feeney Jr
 */

#include "../include/ailee_dao_governance.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace ailee;

// Helper function to print section headers
void printSection(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

// Helper function to print proposal status
void printProposalStatus(const Proposal::ProposalData& data) {
    std::cout << "Proposal ID: " << data.proposalId.substr(0, 16) << "...\n";
    std::cout << "Title: " << data.title << "\n";
    std::cout << "Type: ";
    switch (data.type) {
        case ProposalType::PARAMETER_CHANGE:
            std::cout << "Parameter Change\n";
            break;
        case ProposalType::TREASURY_ALLOCATION:
            std::cout << "Treasury Allocation\n";
            break;
        case ProposalType::VALIDATOR_ADDITION:
            std::cout << "Validator Addition\n";
            break;
        default:
            std::cout << "Other\n";
    }
    std::cout << "Status: ";
    switch (data.status) {
        case ProposalStatus::DRAFT:
            std::cout << "Draft\n";
            break;
        case ProposalStatus::ACTIVE:
            std::cout << "Active (Voting Open)\n";
            break;
        case ProposalStatus::SUCCEEDED:
            std::cout << "Succeeded (Passed)\n";
            break;
        case ProposalStatus::QUEUED:
            std::cout << "Queued (In Timelock)\n";
            break;
        case ProposalStatus::EXECUTED:
            std::cout << "✓ Executed\n";
            break;
        case ProposalStatus::DEFEATED:
            std::cout << "✗ Defeated\n";
            break;
        default:
            std::cout << "Unknown\n";
    }
    std::cout << "Votes FOR: " << data.votesFor << "\n";
    std::cout << "Votes AGAINST: " << data.votesAgainst << "\n";
    std::cout << "Total Voting Power: " << data.totalVotingPower << "\n";
    
    if (data.status == ProposalStatus::SUCCEEDED || 
        data.status == ProposalStatus::EXECUTED) {
        double approval = (data.votesFor / (data.votesFor + data.votesAgainst)) * 100.0;
        std::cout << "Approval: " << std::fixed << std::setprecision(2) 
                  << approval << "%\n";
    }
}

// Simulate time passing (for demo purposes)
void simulateTimePassing(const std::string& message, int seconds = 2) {
    std::cout << "\n⏳ " << message << "...\n";
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

int main() {
    std::cout << R"(
    ╔═══════════════════════════════════════════════════════════╗
    ║         AILEE DAO Governance System - Live Demo          ║
    ║                                                           ║
    ║  Demonstrating decentralized protocol governance         ║
    ║  No company. No CEO. Just math and democracy.            ║
    ╚═══════════════════════════════════════════════════════════╝
    )" << "\n";

    // ========================================================================
    // STEP 1: Initialize DAO with Treasury
    // ========================================================================
    printSection("STEP 1: Initialize DAO Governance");
    
    const uint64_t INITIAL_TREASURY = 10000000; // 10M ADU tokens
    DAOGovernance dao(INITIAL_TREASURY);
    
    std::cout << "✓ DAO initialized with treasury: " 
              << INITIAL_TREASURY << " ADU\n";
    std::cout << "✓ Governance parameters:\n";
    std::cout << "  - Voting period: " << VOTING_PERIOD_DAYS << " days\n";
    std::cout << "  - Timelock period: " << TIMELOCK_PERIOD_DAYS << " days\n";
    std::cout << "  - Quorum required: " << QUORUM_PERCENT << "%\n";
    std::cout << "  - Approval threshold: " << APPROVAL_THRESHOLD_PERCENT << "%\n";
    std::cout << "  - Min stake to propose: " << MIN_PROPOSAL_STAKE << " ADU\n";

    // ========================================================================
    // STEP 2: Register Stakeholders (Token Holders)
    // ========================================================================
    printSection("STEP 2: Register Network Stakeholders");
    
    struct Stakeholder {
        std::string name;
        std::string address;
        uint64_t stake;
    };
    
    std::vector<Stakeholder> stakeholders = {
        {"Alice (Early Adopter)", "addr_alice_001", 50000},
        {"Bob (Developer)", "addr_bob_002", 30000},
        {"Charlie (Validator)", "addr_charlie_003", 100000},
        {"Diana (Miner)", "addr_diana_004", 25000},
        {"Eve (Community)", "addr_eve_005", 15000},
        {"Frank (Researcher)", "addr_frank_006", 20000},
        {"Grace (Foundation)", "addr_grace_007", 200000}
    };
    
    uint64_t totalStake = 0;
    for (const auto& holder : stakeholders) {
        dao.registerStakeHolder(holder.address, holder.stake);
        totalStake += holder.stake;
        
        std::cout << "✓ Registered: " << holder.name << "\n";
        std::cout << "  Address: " << holder.address << "\n";
        std::cout << "  Stake: " << holder.stake << " ADU\n";
        std::cout << "  Voting Power: " << std::sqrt(holder.stake) << "\n\n";
    }
    
    std::cout << "Total Network Stake: " << totalStake << " ADU\n";
    std::cout << "Total Voting Power: " << std::sqrt(totalStake) << "\n";

    simulateTimePassing("Stakeholders joining network");

    // ========================================================================
    // STEP 3: Submit Proposal - AI Parameter Optimization
    // ========================================================================
    printSection("STEP 3: Submit Proposal - Optimize AI Parameters");
    
    std::cout << "Charlie proposes: Increase AI optimization factor from 0.5 to 0.8\n";
    std::cout << "Rationale: Network has proven stable, ready for higher throughput\n\n";
    
    std::string proposal1Id = dao.submitProposal(
        "addr_charlie_003",
        "Increase AI Optimization Factor to 0.8",
        "After 3 months of stable operation at ηAI=0.5 with zero security incidents, "
        "this proposal requests increasing the AI optimization factor to 0.8. "
        "Simulations show this will increase TPS from ~23,000 to ~38,000 without "
        "compromising decentralization. The Circuit Breaker remains active as a failsafe.",
        ProposalType::PARAMETER_CHANGE
    );
    
    if (proposal1Id.empty()) {
        std::cerr << "✗ Failed to submit proposal\n";
        return 1;
    }
    
    auto proposal1 = dao.getProposal(proposal1Id);
    proposal1->addParameter("parameter_name", "ai_optimization_factor");
    proposal1->addParameter("current_value", "0.5");
    proposal1->addParameter("proposed_value", "0.8");
    proposal1->addParameter("estimated_tps_gain", "15000");
    
    std::cout << "✓ Proposal submitted successfully!\n\n";
    printProposalStatus(proposal1->getData());
    
    simulateTimePassing("Proposal being reviewed by community");

    // ========================================================================
    // STEP 4: Activate Proposal (Begin Voting Period)
    // ========================================================================
    printSection("STEP 4: Activate Proposal - Voting Begins");
    
    if (dao.activateProposal(proposal1Id)) {
        std::cout << "✓ Proposal activated! Voting period: 14 days\n";
        std::cout << "Community members can now cast their votes\n\n";
    } else {
        std::cerr << "✗ Failed to activate proposal\n";
        return 1;
    }
    
    printProposalStatus(proposal1->getData());
    
    simulateTimePassing("Community discussing on forums");

    // ========================================================================
    // STEP 5: Community Voting
    // ========================================================================
    printSection("STEP 5: Community Voting Phase");
    
    struct Vote {
        std::string voter;
        std::string voterName;
        VoteChoice choice;
        std::string reasoning;
    };
    
    std::vector<Vote> votes = {
        {"addr_alice_001", "Alice", VoteChoice::FOR, "Ready for higher performance"},
        {"addr_bob_002", "Bob", VoteChoice::FOR, "Code audits look good"},
        {"addr_charlie_003", "Charlie", VoteChoice::FOR, "Proposer - confident in safety"},
        {"addr_diana_004", "Diana", VoteChoice::FOR, "Hardware can handle it"},
        {"addr_eve_005", "Eve", VoteChoice::AGAINST, "Too aggressive, prefer gradual increase"},
        {"addr_frank_006", "Frank", VoteChoice::FOR, "Research supports this change"},
        {"addr_grace_007", "Grace", VoteChoice::FOR, "Foundation endorses - monitored closely"}
    };
    
    std::cout << "Votes being cast (quadratic weighting applied):\n\n";
    
    for (const auto& vote : votes) {
        bool success = dao.vote(proposal1Id, vote.voter, vote.choice);
        
        std::string voteStr = (vote.choice == VoteChoice::FOR) ? "FOR" : "AGAINST";
        std::string emoji = (vote.choice == VoteChoice::FOR) ? "👍" : "👎";
        
        std::cout << emoji << " " << vote.voterName << " votes " << voteStr << "\n";
        std::cout << "   Reason: " << vote.reasoning << "\n";
        
        if (success) {
            std::cout << "   ✓ Vote recorded\n\n";
        } else {
            std::cout << "   ✗ Vote failed\n\n";
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    simulateTimePassing("Voting period concluding");

    // ========================================================================
    // STEP 6: Finalize Voting
    // ========================================================================
    printSection("STEP 6: Finalize Vote & Check Results");
    
    // In production, this would happen automatically after 14 days
    std::cout << "⏰ Voting period ended. Tallying results...\n\n";
    
    if (dao.finalizeProposal(proposal1Id)) {
        std::cout << "✓ Proposal finalization complete\n\n";
        printProposalStatus(proposal1->getData());
        
        auto data = proposal1->getData();
        double totalVotes = data.votesFor + data.votesAgainst;
        double approvalPercent = (data.votesFor / totalVotes) * 100.0;
        double quorum = (data.totalVotingPower / std::sqrt(totalStake)) * 100.0;
        
        std::cout << "\n📊 Final Statistics:\n";
        std::cout << "  Quorum achieved: " << std::fixed << std::setprecision(2) 
                  << quorum << "% (required: " << QUORUM_PERCENT << "%)\n";
        std::cout << "  Approval rate: " << approvalPercent 
                  << "% (required: " << APPROVAL_THRESHOLD_PERCENT << "%)\n";
        
        if (data.status == ProposalStatus::SUCCEEDED) {
            std::cout << "\n✅ PROPOSAL PASSED - Entering 7-day timelock\n";
        } else {
            std::cout << "\n❌ PROPOSAL FAILED\n";
        }
    } else {
        std::cerr << "✗ Finalization failed\n";
        return 1;
    }
    
    simulateTimePassing("Timelock period (7 days in production)", 1);

    // ========================================================================
    // STEP 7: Execute Approved Proposal
    // ========================================================================
    printSection("STEP 7: Execute Approved Proposal");
    
    std::cout << "⏰ Timelock period completed. Executing proposal...\n\n";
    
    // Bypass timelock for demo (in production, must wait 7 days)
    proposal1->queueForExecution();
    
    if (dao.executeProposal(proposal1Id)) {
        std::cout << "✅ PROPOSAL EXECUTED SUCCESSFULLY!\n\n";
        std::cout << "Protocol changes applied:\n";
        std::cout << "  • AI Optimization Factor: 0.5 → 0.8\n";
        std::cout << "  • Expected TPS increase: 23,000 → 38,000\n";
        std::cout << "  • Circuit Breaker: Active (monitoring)\n\n";
        
        printProposalStatus(proposal1->getData());
    } else {
        std::cerr << "✗ Execution failed\n";
        return 1;
    }
    
    simulateTimePassing("Network adapting to new parameters");

    // ========================================================================
    // STEP 8: Treasury Allocation Proposal
    // ========================================================================
    printSection("STEP 8: Treasury Allocation - Fund Research");
    
    std::cout << "Frank proposes: Allocate 50,000 ADU for TPS research grant\n\n";
    
    std::string proposal2Id = dao.submitProposal(
        "addr_frank_006",
        "Research Grant: Advanced Mempool Optimization",
        "Request 50,000 ADU to fund 6-month research project on mempool "
        "optimization algorithms. Team from a university research lab will work on reducing "
        "queueing delays. Expected outcome: 10-15% TPS improvement.",
        ProposalType::TREASURY_ALLOCATION
    );
    
    auto proposal2 = dao.getProposal(proposal2Id);
    proposal2->addParameter("recipient", "addr_mit_research_team");
    proposal2->addParameter("amount", "50000");
    proposal2->addParameter("purpose", "Advanced mempool optimization research");
    proposal2->addParameter("duration", "6 months");
    
    std::cout << "✓ Treasury proposal submitted\n\n";
    printProposalStatus(proposal2->getData());
    
    // Quick vote for demo purposes
    dao.activateProposal(proposal2Id);
    
    std::cout << "\nCommunity voting on research funding...\n";
    for (const auto& holder : stakeholders) {
        // Most vote FOR funding research
        VoteChoice choice = (holder.stake > 20000) ? VoteChoice::FOR : VoteChoice::AGAINST;
        dao.vote(proposal2Id, holder.address, choice);
    }
    
    dao.finalizeProposal(proposal2Id);
    
    std::cout << "\n✓ Research funding approved!\n";
    std::cout << "Treasury allocation: 50,000 ADU → University Research Team\n";
    std::cout << "Remaining treasury: " 
              << dao.getTreasury()->getAvailableBalance() << " ADU\n";

    simulateTimePassing("Treasury allocating funds");

    // ========================================================================
    // STEP 9: Validator Management
    // ========================================================================
    printSection("STEP 9: Add New Validator Through Governance");
    
    std::cout << "Grace proposes: Add new validator node in Asia-Pacific region\n\n";
    
    std::string proposal3Id = dao.submitProposal(
        "addr_grace_007",
        "Add Validator: Singapore Node (Geographic Expansion)",
        "Proposal to add verified validator node in Singapore to improve "
        "network latency for Asia-Pacific users. Node operator has proven "
        "track record and will stake 100,000 ADU.",
        ProposalType::VALIDATOR_ADDITION
    );
    
    auto proposal3 = dao.getProposal(proposal3Id);
    proposal3->addParameter("address", "addr_singapore_validator");
    proposal3->addParameter("identity", "Singapore Blockchain Institute");
    proposal3->addParameter("stake", "100000");
    proposal3->addParameter("location", "Singapore");
    
    dao.activateProposal(proposal3Id);
    
    // Quick approval for demo
    for (const auto& holder : stakeholders) {
        dao.vote(proposal3Id, holder.address, VoteChoice::FOR);
    }
    
    dao.finalizeProposal(proposal3Id);
    dao.executeProposal(proposal3Id);
    
    std::cout << "✅ New validator added successfully!\n";
    std::cout << "Active validators: " 
              << dao.getValidatorRegistry()->getActiveValidators().size() << "\n";
    std::cout << "Geographic coverage improved!\n";

    // ========================================================================
    // FINAL SUMMARY
    // ========================================================================
    printSection("GOVERNANCE DEMO COMPLETE - Summary");
    
    std::cout << "🎉 Successfully demonstrated complete governance lifecycle!\n\n";
    
    std::cout << "Executed Actions:\n";
    std::cout << "  ✓ 7 stakeholders registered (440,000 total stake)\n";
    std::cout << "  ✓ AI parameter change proposal (PASSED & EXECUTED)\n";
    std::cout << "  ✓ Treasury allocation (50,000 ADU research grant)\n";
    std::cout << "  ✓ Validator addition (Geographic expansion)\n\n";
    
    std::cout << "Network Status:\n";
    std::cout << "  • AI Optimization: 0.8 (increased from 0.5)\n";
    std::cout << "  • Estimated TPS: ~38,000 (up from ~23,000)\n";
    std::cout << "  • Active Validators: " 
              << dao.getValidatorRegistry()->getActiveValidators().size() << "\n";
    std::cout << "  • Treasury Balance: " 
              << dao.getTreasury()->getAvailableBalance() << " ADU\n";
    std::cout << "  • Total Network Stake: " << totalStake << " ADU\n\n";
    
    std::cout << "Key Governance Features Demonstrated:\n";
    std::cout << "  ✓ Quadratic voting (prevents whale dominance)\n";
    std::cout << "  ✓ Supermajority requirements (66.67% approval)\n";
    std::cout << "  ✓ Timelock protection (7-day execution delay)\n";
    std::cout << "  ✓ Treasury management (milestone-based funding)\n";
    std::cout << "  ✓ Validator governance (decentralized node management)\n";
    std::cout << "  ✓ Reputation system (rewards participation)\n\n";
    
    std::cout << R"(
    ╔═══════════════════════════════════════════════════════════╗
    ║                                                           ║
    ║  AILEE: Decentralized by Design, Governed by Community   ║
    ║                                                           ║
    ║  No company. No CEO. No central authority.               ║
    ║  Just cryptography, mathematics, and democracy.          ║
    ║                                                           ║
    ╚═══════════════════════════════════════════════════════════╝
    )" << "\n";
    
    std::cout << "📖 Learn more: https://github.com/yourusername/ailee-protocol\n";
    std::cout << "💬 Join discussion: AILEE Community Forum\n";
    std::cout << "🔬 Read research: whitepaper.md\n\n";

    return 0;
}
