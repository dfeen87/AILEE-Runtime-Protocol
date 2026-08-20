/**
 * Public API Contracts for AILEE Runtime Kernel Clock.
 * Note: AILEE Core is closed-source; these interfaces reflect public AILEE extension points.
 */

/**
 * Representation of deterministic internal clock aligned to Bitcoin mainnet.
 */
export interface KernelTime {
  /**
   * Bitcoin mainnet block height.
   */
  blockHeight: number;

  /**
   * Bitcoin mainnet median past time (BIP-113) in UNIX seconds or nanoseconds.
   */
  medianBlockTime: bigint;

  /**
   * Mempool event timestamp in UNIX nanoseconds.
   */
  mempoolTimestamp: bigint;

  /**
   * Internal high-resolution kernel clock time in UNIX nanoseconds.
   */
  unixNs: bigint;
}

/**
 * Event emitted by AILEE Kernel Clock.
 */
export interface KernelTimeEvent {
  /**
   * Kernel time snapshot when event occurred.
   */
  kernelTime: KernelTime;

  /**
   * Type of kernel clock event.
   */
  type: 'BLOCK_CONNECTED' | 'MEMPOOL_TRANSACTION' | 'KERNEL_TICK';

  /**
   * Optional transaction or block hash associated with event.
   */
  payloadHash?: string;
}

/**
 * Public extension interface exposed by AILEE Kernel Clock.
 */
export interface KernelClockAPI {
  /**
   * Query current kernel time.
   */
  getKernelTime(): KernelTime;

  /**
   * Subscribe to kernel clock events.
   * Returns an unsubscribe function.
   */
  subscribeKernelClock(callback: (event: KernelTimeEvent) => void): () => void;
}
