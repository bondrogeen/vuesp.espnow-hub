import { storeToRefs } from 'pinia';
import { useAppStore } from '@/stores/AppStore';
import { useWebSocket } from '@/stores/WebSocket';
import { useWebSocketStore } from '@/stores/WebSocketStore';

export const useStore = () => {
  const webSocketStore = useWebSocketStore();
  const { ...socket } = storeToRefs(webSocketStore);

  const appStore = useAppStore();
  const { ...app } = storeToRefs(appStore);

  const webSocket = useWebSocket();
  const { isInit } = storeToRefs(webSocket);

  const onDialog = appStore.setDialog;
  const wsSend = webSocketStore.onSend;
  const changeTheme = appStore.changeTheme;

  return { ...socket, ...app, isInit, onDialog, changeTheme, wsSend };
};
