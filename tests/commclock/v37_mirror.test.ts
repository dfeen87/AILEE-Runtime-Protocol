import { GovernanceMirror, AlcoaLedgerMirror, CompartmentStateMachineMirror, CompartmentState } from '../../src/index.js';

describe('v37 TypeScript Governance Mirror', () => {
  test('GovernanceMirror evaluates factors correctly', () => {
    const mirror = new GovernanceMirror();
    const decision = mirror.evaluateFactors({
      quorumCount: 4,
      totalValidators: 5,
      operatorSignatureValid: true,
      systemSignatureValid: true,
      zkStateConsistent: true,
      postureScore: 1.2,
      temporalCoherenceIndex: 0.95,
      zkRecursionRoot: '0x123456789',
    });

    expect(decision.approved).toBe(true);
    expect(decision.quorumPassed).toBe(true);
    expect(decision.evaluatedFactors.length).toBe(5);
  });

  test('AlcoaLedgerMirror records and verifies entries', () => {
    const ledger = new AlcoaLedgerMirror();
    const entry = ledger.recordEntry({
      operatorId: 'operator-ts',
      systemId: 'ailee-ts-mirror',
      operatorSignature: '0xsig123',
      humanReadableSummary: 'TS Mirror Entry',
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

    expect(entry.entryId).toBeDefined();
    expect(ledger.verifyEntry(entry.entryId)).toBe(true);
    expect(ledger.getEntries().length).toBe(1);
  });

  test('CompartmentStateMachineMirror manages states', () => {
    const sm = new CompartmentStateMachineMirror();
    expect(sm.isIsolated('core-execution')).toBe(false);
    expect(sm.isIsolated('unknown-comp')).toBe(true);

    sm.transitionState('core-execution', CompartmentState.QUARANTINED, 'Safety override');
    expect(sm.isIsolated('core-execution')).toBe(true);
  });
});
