import { onMounted, onUnmounted } from 'vue';
import { event } from '@/assets/js/';
import Queue from '@/assets/js/queue';

import { useStore } from '@/composables/useStore';

import type { TypeMessage, TypeSend } from '@/types';

export const useConnection = (init?: (send: TypeSend) => void, messages?: (message: TypeMessage) => void) => {
  const store = useStore();

  const queue = new Queue<{ key: TypeMessage['key']; object?: TypeMessage['object'] }>(100, ({ key, object }) => {
    store.wsSend(key, object);
  });

  const onSend = (key: TypeMessage['key'], object?: TypeMessage['object']) => {
    queue.add({ key, object });
  };

  const onInit = () => {
    if (!store.isInit.value && init) init(onSend);
  };

  const onMessages = (message: TypeMessage) => {
    messages?.(message);
  };

  event.on('init', onInit);
  if (messages) event.on('messages', onMessages);

  onMounted(() => {
    if (store.isInit.value && init) init(onSend);
  });

  // Without this every visited page keeps listening to init/messages.
  // After a few route changes old handlers flood the device and can drop WS.
  onUnmounted(() => {
    event.off('init', onInit);
    if (messages) event.off('messages', onMessages);
  });

  return { ...store, onSend };
};
