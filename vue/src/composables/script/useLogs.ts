import { nextTick, ref, useTemplateRef, watch, type Ref } from 'vue';
import { timeUtcToString } from 'vuesp-components/helpers';
import type { IMessageMessage } from 'vuesp-components/types';

interface ILog {
  time: string;
  type: number;
  text: string;
}

export const useLogs = (message: Ref<IMessageMessage, IMessageMessage>) => {
  const logs = ref<ILog[]>([]);
  const isHover = ref(false);
  const container = useTemplateRef<HTMLDivElement>('container');
  const scrollLastLog = () => {
    if (!container.value?.lastElementChild) return;
    if (isHover.value) return;
    container.value.scrollTop = container.value.scrollHeight;
  };

  const onHover = (value: boolean) => {
    isHover.value = value;
  };

  watch(
    () => message.value,
    (v) => {
      if (v.type === 0) {
        const date = new Date();
        logs.value.push({ time: `${timeUtcToString(date, { minute: '2-digit', hour: '2-digit', second: '2-digit' })}`, type: 0, text: v?.text || '' });
        nextTick(() => {
          scrollLastLog();
        });
      }
    }
  );
  return {
    logs,
    onHover,
  };
};
