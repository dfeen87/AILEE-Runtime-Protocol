import * as fs from 'node:fs';

export interface DomainConfig {
  domainId: string;
  enabled: boolean;
  adapterClassName: string;
  parameters: Record<string, string | number | boolean>;
}

export class CommClockConfig {
  public profileName: string;
  public vendorName: string;
  public domains: DomainConfig[];

  constructor(profileName = 'default', vendorName = 'generic', domains: DomainConfig[] = []) {
    this.profileName = profileName;
    this.vendorName = vendorName;
    this.domains = domains;
  }

  /**
   * Load configuration from JSON file path.
   */
  public static loadFromFile(path: string): CommClockConfig {
    if (!fs.existsSync(path)) {
      throw new Error(`Configuration file not found at path: ${path}`);
    }

    const fileContent = fs.readFileSync(path, 'utf-8');
    const rawConfig = JSON.parse(fileContent);

    if (!Array.isArray(rawConfig.domains)) {
      throw new Error(`Invalid configuration format: 'domains' must be an array.`);
    }

    const domains: DomainConfig[] = rawConfig.domains.map((d: any, index: number) => {
      if (!d.domainId || typeof d.enabled !== 'boolean' || !d.adapterClassName) {
        throw new Error(`Invalid domain configuration at index ${index} in file ${path}`);
      }
      return {
        domainId: String(d.domainId),
        enabled: Boolean(d.enabled),
        adapterClassName: String(d.adapterClassName),
        parameters: d.parameters && typeof d.parameters === 'object' ? d.parameters : {},
      };
    });

    return new CommClockConfig(
      rawConfig.profileName ?? 'unknown_profile',
      rawConfig.vendorName ?? 'generic',
      domains
    );
  }

  /**
   * Helper to retrieve enabled domain configs.
   */
  public getEnabledDomains(): DomainConfig[] {
    return this.domains.filter((d) => d.enabled);
  }
}
