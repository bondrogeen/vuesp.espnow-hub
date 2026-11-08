import type { MyTypeKEYS } from '@/types';

export const pathListDef = '/default.json';

export const PATH_FS = '/fs';

export const pathTmp = '/tmp';
export const pathList = `${pathTmp}/dashboard.json`;
export const pathGPIO = `${pathTmp}/gpio.io`;

export const KEYS: Record<string, MyTypeKEYS> = {
  INIT: 'INIT',
  SETTINGS: 'SETTINGS',
  INFO: 'INFO',
  PING: 'PING',
  SCAN: 'SCAN',
  PROGRESS: 'PROGRESS',
  FILES: 'FILES',
  REBOOT: 'REBOOT',
  EVENT: 'EVENT',
  PORT: 'PORT',
  MESSAGE: 'MESSAGE',
  DALLAS: 'DALLAS',
  BUFFER: 'BUFFER',
  DEVICE: 'DEVICE',
  DISCOVERY: 'DISCOVERY',
};
