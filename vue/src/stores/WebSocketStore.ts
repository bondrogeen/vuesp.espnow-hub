import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';
import { useAppStore } from './AppStore.ts';

import type { TypeMessage, IStateMain, IWebSocketStore, IMessagePort, IMessageDallas, IMessageProgress, IMessageMessage, IMessageDiscovery, IMyMessageSettings } from '@/types';

const initialState = (): IWebSocketStore => ({
  main: {
    ports: {},
    info: { id: 0, firmware: [], totalBytes: 0, usedBytes: 0, uptime: 0, name: '', board: 0 },
    device: { command: 0, analog: 0, pwm: 0, now: 0, list: 0, message: '' },
    dallas: {},
    discovery: {},
    slots: {},
  },
  settings: {
    wifiDhcp: 1,
    wifiMode: 1,
    authMode: 1,
    version: 1,
    device: 0,
    wifiIp: [],
    wifiSubnet: [],
    wifiGateway: [],
    wifiDns: [],
    wifiSsid: '',
    wifiPass: '',
    authLogin: '',
    authPass: '',
    discovery: 0,
    discoveryInterval: 5,
    discoveryPort: 0,
  },
  progress: { status: 0, empty: 0, size: 0, length: 0 },
  message: { type: 0, active: 0, handler: 0, date: 0, index: 0, id: 0, len: 0, text: '' },
});

export const useWebSocketStore = defineStore('webSocketStore', {
  state: initialState,
  actions: {
    SET_SETTINGS(value: IMyMessageSettings) {
      this.settings = value;
    },
    SET_PROGRESS(value: IMessageProgress) {
      this.progress = value;
    },
    SET_PORT(port: IMessagePort) {
      const gpio = port.gpio.toString();
      this.main.ports[gpio] = port;
    },
    SET_DALLAS(data: IMessageDallas) {
      const name = (data.address || []).map((i) => (i < 16 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
    },
    SET_DISCOVERY(data: IMessageDiscovery) {
      const id = data.id.toString(16);
      this.main.discovery[id] = data;
      // if(data.status) appStore.setNotification({text: `new${}`})
    },
    SET_MAIN({ object, key }: { object: any; key: string }) {
      const name: keyof IStateMain = key.toLowerCase() as keyof IStateMain;
      if (['ping', 'files', 'progress', 'scan'].includes(name)) return;
      this.main[name] = object;
    },
    SET_MESSAGE(message: IMessageMessage) {
      this.message = message;
      const { type } = message;
      if (type === 1) {
        const appStore = useAppStore();
        appStore.setNotification(message)
      }
      if (type === 2) {
        const index = message.index;
        this.main.slots[index || 0] = message;
      }
    },
    onSend(key: TypeMessage['key'], object?: TypeMessage['object']) {
      const store = useWebSocket();
      store.onSend(key, object);
    },
  },
});
