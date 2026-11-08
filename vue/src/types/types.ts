import type {
  IMessageInfo,
  IMessageProgress,
  IMessagePort,
  IMessageMessage,
  IMessageDallas,
  IMessageDiscovery,
  INotification,
  IDashboardItem,
  IMenuItem,
  IDialog,
  IPackage,
  ISuggestion,
  IMessageDevice,
} from 'vuesp-components/types';

import type { TypeMessage, IMyMessageSettings } from './custom';

export type * from './custom';

type TypeMessageMap = { [K in TypeMessage['key']]: Extract<TypeMessage, { key: K }>['object'] };

export type TypeSend = <K extends keyof TypeMessageMap>(key: K, object?: TypeMessageMap[K]) => void;

// ======== IWebSocketStore ===========

export interface IAppStore {
  menu: IMenuItem[];
  dashboard: IDashboardItem[];
  suggestions: ISuggestion[];
  theme: string;
  dialog: IDialog;
  dialogInfo: boolean;
  pkg: IPackage;
  notifications: INotification[];
}

// ======== IWebSocket ===========

export interface IWebSocket {
  socket: WebSocket | null;
  pingClient: number;
  pingDevice: number;
  isInit: boolean;
  infoLoaded: boolean;
}

// ======== IWebSocketStore ===========

export interface IWebSocketStore {
  settings: IMyMessageSettings;
  main: IStateMain;
  progress: IMessageProgress;
  message: IMessageMessage;
}

export interface IStateMain {
  dallas: Record<string, IMessageDallas>;
  discovery: Record<string, IMessageDiscovery>;
  info: IMessageInfo;
  ports: Record<string, IMessagePort>;
  device: IMessageDevice;
  slots: Record<number, IMessageMessage>;
}
