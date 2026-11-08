type Callback<T> = (data: T) => void;

export default class CompactQueue<T> {
  private queue: T[] = [];
  private interval: number;
  private isProcessing: boolean = false;
  private callback: Callback<T> | null = null;

  constructor(interval: number = 1000, callback: Callback<T> | null = null) {
    this.interval = interval;
    this.callback = callback;
  }

  add(data: T): void {
    this.queue.push(data);
    this.#checkAndStart();
  }

  onData(callback: Callback<T>): void {
    this.callback = callback;
    this.#checkAndStart();
  }

  #checkAndStart(): void {
    if (!this.isProcessing && this.queue.length > 0 && this.callback) {
      this.isProcessing = true;
      this.#process();
    }
  }

  #process(): void {
    if (this.queue.length === 0 || !this.callback) {
      this.isProcessing = false;
      return;
    }

    const data = this.queue.shift()!;
    this.callback(data);

    setTimeout(() => this.#process(), this.interval);
  }
}
