import { onMounted, onUnmounted } from 'vue';

import { storeToRefs } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';

export const useSocket = () => {
  const webSocket = useWebSocket();
  const { socket, isConnect } = storeToRefs(webSocket);

  let ping: ReturnType<typeof setInterval> | null = null;
  let lastInfoRequest = 0;
  let reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  let reconnectAttempt = 0;
  let stopped = false;
  let generation = 0;

  const clearReconnect = () => {
    if (reconnectTimer) {
      clearTimeout(reconnectTimer);
      reconnectTimer = null;
    }
  };

  const connect = () => {
    if (stopped) return;

    const current = ++generation;
    const instance: WebSocket = new WebSocket(`/esp`);
    instance.binaryType = 'arraybuffer';

    instance.onopen = () => {
      if (current !== generation || stopped) return;
      reconnectAttempt = 0;
      webSocket.onopen();
    };

    instance.onmessage = (e: MessageEvent) => {
      if (current === generation) webSocket.onmessage(e);
    };

    instance.onclose = (e: CloseEvent) => {
      if (current !== generation) return;
      webSocket.onclose(e);

      if (stopped || e.code === 1000) return;

      const delay = Math.min(30000, 1000 * 2 ** reconnectAttempt);
      reconnectAttempt += 1;
      clearReconnect();
      reconnectTimer = setTimeout(connect, delay);
    };

    instance.onerror = (e: Event) => {
      if (current === generation) webSocket.onerror(e);
    };

    socket.value = instance;
  };

  onMounted(() => {
    stopped = false;
    ping = setInterval(() => {
      webSocket.onPing();

      // A reload can race with the device's snapshot: if INFO never arrives,
      // retry it until the first response, otherwise pages stay without data.
      if (socket.value?.readyState === WebSocket.OPEN && !webSocket.infoLoaded && Date.now() - lastInfoRequest > 2000) {
        lastInfoRequest = Date.now();
        webSocket.requestInfo();
      }
    }, 1000);
    reconnectTimer = setTimeout(connect, 100);
  });

  onUnmounted(() => {
    stopped = true;
    generation += 1;
    clearReconnect();
    if (ping) clearInterval(ping);
    if (socket.value) socket.value.close(1000);
    socket.value = null;
  });
  return { isConnect };
};
