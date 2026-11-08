import type { IMessageBuffer, IDashboardItem } from '@/types';

export interface ICardChartProps extends Omit<IDashboardItem, 'value'> {
  value?: IMessageBuffer;
}

export interface ICardChartEmit {
  (e: 'edit' | 'close' | 'click' | 'clone', event: Event): void;
  (e: 'open', event: boolean): void;
  (e: 'setState', item: string | number | boolean): void;
  (e: 'update', event: Event): void;
}
