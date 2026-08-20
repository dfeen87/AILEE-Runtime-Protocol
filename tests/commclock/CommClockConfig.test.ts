import { CommClockConfig } from '../../src/config/CommClockConfig.js';
import * as path from 'path';

describe('CommClockConfig', () => {
  test('loads valid profile config from JSON file', () => {
    const configPath = path.join(__dirname, '../../configs/maxlinear_rf_dsp_network.json');
    const config = CommClockConfig.loadFromFile(configPath);

    expect(config.profileName).toBe('maxlinear_rf_dsp_network');
    expect(config.vendorName).toBe('MaxLinear');

    const enabled = config.getEnabledDomains();
    expect(enabled.length).toBe(3);

    const rfConfig = enabled.find((d) => d.domainId === 'RF');
    expect(rfConfig).toBeDefined();
    expect(rfConfig?.adapterClassName).toBe('MaxLinearRFClockAdapter');
    expect(rfConfig?.parameters['carrierFrequencyHz']).toBe(3500000000);
  });

  test('throws error if file does not exist', () => {
    expect(() => CommClockConfig.loadFromFile('non_existent_config.json')).toThrow(
      /Configuration file not found/
    );
  });
});
