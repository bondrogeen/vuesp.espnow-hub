import type { IStateMain } from '@/types';
import { computed, ref, type Ref } from 'vue';
import type { IMessageMessage } from 'vuesp-components/types';

export const useSlots = (main: Ref<IStateMain, IStateMain>) => {
  const currentSlot = ref<IMessageMessage | null>(null);
  const slotInfo = computed(() => {
    return Object.values(main.value.slots).reduce(
      (acc, i: IMessageMessage) => {
        if (i.active || i.handler) {
          acc.used = acc.used + 1;
        }
        acc.total = acc.total + 1;
        return acc;
      },
      { total: 0, used: 0 }
    );
  });

  const onSlot = (slot: IMessageMessage) => {
    currentSlot.value = slot;
  };
  const getColorSlot = (slot: IMessageMessage) => {
    if (slot.active) return 'bg-violet-500';
    if (slot.handler) return 'bg-blue-500';
    return 'bg-gray-200 dark:bg-gray-500 ';
  };
  return {
    slotInfo,
    currentSlot,
    onSlot,
    getColorSlot,
  };
};
