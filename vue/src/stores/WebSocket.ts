import type { IWebSocket, TypeMessage } from '../types/types.ts';
import { KEYS } from '@/utils/const';

import { defineStore } from 'pinia';
import { useWebSocketStore } from './WebSocketStore.ts';

import { struct, event } from '@/assets/js/';

const log = (...arg: any) => (process.env.NODE_ENV === 'development' ? console.log(arg) : null);

const initialState = (): IWebSocket => ({
  socket: null,
  pingClient: 5000,
  pingDevice: 0,
  isInit: false,
  infoLoaded: false,
});

export const useWebSocket = defineStore('webSocket', {
  state: initialState,
  actions: {
    init() {
      this.infoLoaded = false;
      this.onSend(KEYS.INFO);
      event.emit('init');
      this.isInit = true;
    },
    requestInfo() {
      if (!this.socket || this.socket.readyState !== WebSocket.OPEN) return;
      const buffer = struct.set(KEYS.INFO);
      if (buffer) this.socket.send(buffer);
    },
    onopen() {
      this.pingDevice = Date.now();
      this.pingClient = Date.now();
      this.init();
      event.emit('connected', true);
    },
    onmessage(message: MessageEvent) {
      this.pingDevice = Date.now();
      if (message.data instanceof ArrayBuffer) {
        const data = struct.get(message.data);
        event.emit('messages', data as TypeMessage);
        if (data) {
          const { object, key } = data;
          if (key !== KEYS.PING) log(object, key);
          if (key === KEYS.INFO) this.infoLoaded = true;
          const store = useWebSocketStore();
          const isStoreMethod = (key: string): key is keyof typeof store => key in store && typeof store[key as keyof typeof store] === 'function';
          if (isStoreMethod(`SET_${key}`)) {
            const methodName = `SET_${key}` as keyof typeof store;
            const method = store[methodName] as Function;
            method.call(store, object);
          } else store.SET_MAIN(data);
        }
      }
    },
    onclose(data: any) {
      event.emit('connected', false);
      log(data);
    },
    onerror(data: any) {
      event.emit('connected', false);
      log(data);
    },
    onSend<K extends TypeMessage['key'], O extends TypeMessage['object']>(key: K, object?: O) {
      log(key, object);
      if (this.socket && this.socket.readyState === WebSocket.OPEN && this.isConnect) {
        const buffer = struct.set(key, object as Record<string, unknown>);
        if (buffer) this.socket.send(buffer);
      }
    },
    onPing() {
      this.pingClient = Date.now();
    },
  },
  getters: {
    isConnect: (state) => state.pingClient - state.pingDevice < 30000,
  },
});
