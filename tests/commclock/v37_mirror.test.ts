import { GovernanceMirror, AlcoaLedgerMirror, CompartmentStateMachineMirror, CompartmentState } from '../../src/index.js';

describe('v37 TypeScript Governance Mirror', () => {
  test('GovernanceMirror evaluates valid factors correctly', () => {
    const mirror = new GovernanceMirror();
    const decision = mirror.evaluateFactors({
      quorumCount: 4,
      totalValidators: 5,
      operatorSignatureValid: true,
      systemSignatureValid: true,
      zkStateConsistent: true,
      postureScore: 1.2,
      temporalCoherenceIndex: 0.95,
      zkRecursionRoot: '0x123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0',
    });

    expect(decision.approved).toBe(true);
    expect(decision.quorumPassed).toBe(true);
    expect(decision.evaluatedFactors.length).toBe(5);
  });

  test('GovernanceMirror rejects high posture score and zero/stale ZK root', () => {
    const mirror = new GovernanceMirror();

    // High posture score (> 2.5) fails
    const decHighPosture = mirror.evaluateFactors({
      quorumCount: 4,
      totalValidators: 5,
      operatorSignatureValid: true,
      systemSignatureValid: true,
      zkStateConsistent: true,
      postureScore: 3.8,
      temporalCoherenceIndex: 0.95,
      zkRecursionRoot: '0x123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0',
    });
    expect(decHighPosture.approved).toBe(false);
    expect(decHighPosture.posturePassed).toBe(false);

    // Empty/zero ZK recursion root fails
    const decZeroRoot = mirror.evaluateFactors({
      quorumCount: 4,
      totalValidators: 5,
      operatorSignatureValid: true,
      systemSignatureValid: true,
      zkStateConsistent: true,
      postureScore: 1.0,
      temporalCoherenceIndex: 0.95,
      zkRecursionRoot: '0x0000000000000000000000000000000000000000000000000000000000000000',
    });
    expect(decZeroRoot.approved).toBe(false);
    expect(decZeroRoot.zkValid).toBe(false);
  });

  test('AlcoaLedgerMirror records, verifies entries, and verifies parent hash chain', () => {
    const ledger = new AlcoaLedgerMirror();
    const entry1 = ledger.recordEntry({
      operatorId: 'operator-ts',
      systemId: 'ailee-ts-mirror',
      operatorSignature: '0xsig123',
      humanReadableSummary: 'TS Mirror Entry 1',
      regimeLabel: 'neutral',
      compartmentLabel: 'core-execution',
      epochId: 3700,
      epochHash: '0xepochhash37',
      sourceSystem: 'AILEE-TS-Mirror',
      postureRegimeId: 'neutral',
      postureScore: 1.0,
      zkRecursionRoot: '0xzkroot',
      temporalCoherenceIndex: 0.99,
      signalEnergy: 10.0,
      coherenceScore: 0.99,
    });

    const entry2 = ledger.recordEntry({
      operatorId: 'operator-ts',
      systemId: 'ailee-ts-mirror',
      operatorSignature: '0xsig456',
      humanReadableSummary: 'TS Mirror Entry 2',
      regimeLabel: 'chop',
      compartmentLabel: 'core-execution',
      epochId: 3701,
      epochHash: '0xepochhash38',
      sourceSystem: 'AILEE-TS-Mirror',
      postureRegimeId: 'chop',
      postureScore: 1.5,
      zkRecursionRoot: '0xzkroot2',
      temporalCoherenceIndex: 0.98,
      signalEnergy: 12.0,
      coherenceScore: 0.98,
    });

    expect(entry1.entryId).toBeDefined();
    expect(ledger.verifyEntry(entry1.entryId)).toBe(true);
    expect(ledger.verifyEntry(entry2.entryId)).toBe(true);
    expect(ledger.verifyChain()).toBe(true);
  });

  test('CompartmentStateMachineMirror manages valid/invalid transitions', () => {
    const sm = new CompartmentStateMachineMirror();
    sm.registerCompartment('test-comp', 'Test Compartment'); // Starts in ISOLATED

    // Direct ISOLATED -> ACTIVE should fail
    expect(sm.transitionState('test-comp', CompartmentState.ACTIVE)).toBe(false);

    // ISOLATED -> MONITORED -> ACTIVE should pass
    expect(sm.transitionState('test-comp', CompartmentState.MONITORED)).toBe(true);
    expect(sm.transitionState('test-comp', CompartmentState.ACTIVE)).toBe(true);
  });
});
