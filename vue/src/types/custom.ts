import type { IMessageDevice, IMessageSettings, TypeConstMessage, TypeKEYS } from 'vuesp-components/types';

export type MyTypeKEYS = (TypeKEYS & {}) | 'DEVICE';

interface IKeyMessageDevice {
  key: Extract<MyTypeKEYS, 'DEVICE'>;
  object?: IMessageDevice | Partial<IMessageDevice>;
}

export type TypeMessage = (TypeConstMessage & {}) | IKeyMessageDevice;

export interface IMyMessageSettings extends IMessageSettings {
  EthIp?: [];
  EthSubnet?: [];
  EthGateway?: [];
  EthDns?: [];
}
