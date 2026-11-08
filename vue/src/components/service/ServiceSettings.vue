<script setup lang="ts">
import type { Ref } from 'vue';
import type { TypeMessage, IMessageScan, IListItem, TypeSend } from '@/types';

import { computed, ref, nextTick, watch } from 'vue';
import { required, maxLen, minLen, sameAs, ip, minVal, maxVal } from 'vuesp-components/validate';

import { KEYS } from '@/utils/const';

import { useForm } from 'vuesp-components/composables';
import type { ValidationSchema } from 'vuesp-components/types';

import { useConnection } from '@/composables/useConnection';
import { arrToStr, strToArr } from 'vuesp-components/helpers';

import { VCheckbox, BlockScan, BlockService, BlockUpdate } from 'vuesp-components/components';
import { useLocale } from '@/composables/useLocale';
import { useFetch } from '@vueuse/core';

const { $t, getLocale, setLocale, list } = useLocale();

const listLocale = computed(() => list.map((i: string) => ({ name: $t(`locale.${i}`), value: i })));
const onLocale = ({ value }: IListItem<string>) => setLocale(value);

const listMenu: IListItem<number>[] = [{ name: $t('save'), value: 1 }];

const isPass = ref(false);
const isAuthPass = ref(false);
const isDialog = ref(false);

const scanList: Ref<Partial<IMessageScan>[]> = ref([]);

const onMessage = ({ key, object }: TypeMessage) => {
  if (key === KEYS.SCAN && object) scanList.value.push(object);
};

const onInit = (send: TypeSend) => {
  send(KEYS.SETTINGS);
};

const { settings, dialogInfo, onSend, onDialog } = useConnection(onInit, onMessage);

const { defineField, handleSubmit } = useForm({
  validationSchema: (values) =>
    ({
      wifiSsid: [required, maxLen(32)],
      wifiPass: values.wifiPass !== '' ? [required, minLen(8), maxLen(32)] : [],
      rePassword: values.wifiPass !== '' ? [required, minLen(8), maxLen(32), sameAs('wifiPass')] : [],
      wifiIp: [ip],
      wifiSubnet: [ip],
      wifiGateway: [ip],
      wifiDns: [ip],
      authLogin: settings.value.authMode ? [required, maxLen(12)] : [],
      authPass: settings.value.authMode ? [required, maxLen(12)] : [],
      reAuthPassword: settings.value.authMode ? [required, maxLen(12), sameAs('authPass')] : [],
      discoveryInterval: settings.value.discovery ? [required, minVal(5), maxVal(255)] : [],
      discoveryPort: settings.value.discovery ? [required, minVal(1000), maxVal(65535)] : [],
    }) as ValidationSchema,
});

const [wifiSsid, wifiSsidProps] = defineField<string>('wifiSsid');
const [wifiPass, wifiPassProps] = defineField<string>('wifiPass');
const [rePassword, rePasswordProps] = defineField<string>('rePassword');
const [wifiIp, wifiIpProps] = defineField<string>('wifiIp');
const [wifiSubnet, wifiSubnetProps] = defineField<string>('wifiSubnet');
const [wifiGateway, wifiGatewayProps] = defineField<string>('wifiGateway');
const [wifiDns, wifiDnsProps] = defineField<string>('wifiDns');
const [authLogin, authLoginProps] = defineField<string>('authLogin');
const [authPass, authPassProps] = defineField<string>('authPass');
const [reAuthPassword, reAuthPasswordProps] = defineField<string>('reAuthPassword');
const [discoveryInterval, discoveryIntervalProps] = defineField<string>('discoveryInterval');
const [discoveryPort, discoveryPortProps] = defineField<string>('discoveryPort');

watch(
  () => settings.value,
  () => {
    wifiSsid.value = settings.value.wifiSsid;
    if (settings.value.wifiPass) wifiPass.value = settings.value.wifiPass;
    if (settings.value.wifiPass) rePassword.value = settings.value.wifiPass;
    wifiIp.value = arrToStr(settings.value.wifiIp);
    wifiSubnet.value = arrToStr(settings.value.wifiSubnet);
    wifiGateway.value = arrToStr(settings.value.wifiGateway);
    wifiDns.value = arrToStr(settings.value.wifiDns);
    authLogin.value = settings.value.authLogin;
    authPass.value = settings.value.authPass;
    reAuthPassword.value = settings.value.authPass;
    discoveryPort.value = `${settings.value.discoveryPort}`;
    discoveryInterval.value = `${settings.value.discoveryInterval}`;
  }
);

const onSubmit = handleSubmit((values) => {
  settings.value.wifiSsid = values.wifiSsid as string;
  settings.value.wifiPass = values.wifiPass as string;
  settings.value.wifiIp = strToArr(values.wifiIp as string);
  settings.value.wifiSubnet = strToArr(values.wifiSubnet as string);
  settings.value.wifiGateway = strToArr(values.wifiGateway as string);
  settings.value.wifiDns = strToArr(values.wifiDns as string);
  settings.value.authLogin = values.authLogin as string;
  settings.value.authPass = values.authPass as string;
  settings.value.discoveryPort = values.discoveryPort as number;
  settings.value.discoveryInterval = values.discoveryInterval as number;
  onSave();
});

const listWiFi: IListItem<number>[] = [
  { name: $t('off'), value: 0 },
  { name: $t('sta'), value: 1 },
  { name: $t('ap'), value: 2 },
  // { name: 'STA + AP', value: 3 },
];

const getValueListWiFi = (mode: number) => listWiFi.find((i) => i.value === mode)?.name;

const isWifiDHCP = computed(() => Boolean(settings.value.wifiDhcp || !settings.value.wifiMode));
const isWifi = computed(() => Boolean(!settings.value.wifiMode));
const isAuth = computed(() => Boolean(!settings.value.authMode));

const onReboot = () => {
  onSend(KEYS.REBOOT);
};

const onSave = () => {
  onSend(KEYS.SETTINGS, settings.value);
  onSureReboot();
};

const onReset = () => {
  settings.value.version = Math.floor(Math.random() * 65000);
  nextTick(() => {
    onSave();
    onReboot();
  });
};

const onSureReboot = () => onDialog({ value: true, message: $t('dialog.doRestart'), callback: onReboot });
const onSureReset = () => onDialog({ value: true, message: $t('dialog.doReset'), callback: onReset });

const onMenu = () => {
  onSubmit();
};

const onSelectSsid = ({ ssid }: IMessageScan) => {
  settings.value.wifiMode = 1;
  settings.value.wifiSsid = ssid;
  wifiSsid.value = ssid;
  wifiPass.value = '';
  rePassword.value = '';
  const input: HTMLInputElement | null = document.querySelector('#wifiPass input');
  if (input) input.focus();
  onClose();
};

const onClose = () => (isDialog.value = false);

const onScan = () => {
  isDialog.value = true;
  scanList.value = [];
  onSend(KEYS.SCAN);
};

const onChange = (value: number) => (settings.value.wifiMode = value);
const onSureOffWifi = ({ value }: IListItem<number>) => {
  const v = value as number;
  return !v ? onDialog({ value: true, message: $t('dialog.doReset'), callback: onChange.bind(this, v) }) : onChange(v);
};

const onFlash = async (body: FormData) => {
  const { data } = await useFetch('/update', { body }).post().json();
  if (data.value?.state) onDialog({ value: true, title: 'Done', message: 'Reboot...' });
};

const onUpdate = (name: string, files: FormData) => {
  if (!files) return;
  onDialog({
    value: true,
    message: $t('dialog.doUpdate', { name }),
    callback: name === 'firmware' ? onFlash.bind(null, files) : onFlash.bind(null, files),
  });
};
</script>

<template>
  <div class="grid grid-cols-1 xl:grid-cols-2 2xl:grid-cols-3 gap-4">
    <card-main class="xl:col-span-2 2xl:col-span-3">
      <div class="grid grid-cols-1 xl:grid-cols-2 gap-x-4">
        <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
          <div class="flex justify-between mb-4 md:col-span-2">
            <h3 class="text-lg font-semibold text-gray-800 dark:text-white/90">{{ $t('wifi') }}</h3>
          </div>

          <div class="mb-6">
            <v-select :model-value="getValueListWiFi(settings.wifiMode)" :label="$t('mode')" :items="listWiFi" @change="onSureOffWifi" />
          </div>

          <v-text-field v-model="wifiSsid" v-bind="wifiSsidProps" :label="$t('ssid')" :disabled="isWifi" :append-button="!isWifi" @on-icon="onScan">
            <template #icon>
              <icon-ri-search-line></icon-ri-search-line>
            </template>
          </v-text-field>

          <v-text-field id="wifiPass" v-model="wifiPass" v-bind="wifiPassProps" :label="$t('pass')" :disabled="isWifi" :type="isPass ? 'text' : 'password'" @on-icon="isPass = !isPass">
            <template #icon>
              <icon-ri-eye-line v-if="isPass" class="size-5"></icon-ri-eye-line>
              <icon-ri-eye-off-line v-else class="size-5"></icon-ri-eye-off-line>
            </template>
          </v-text-field>

          <v-text-field v-model="rePassword" v-bind="rePasswordProps" :label="$t('passRe')" :disabled="isWifi" :type="isPass ? 'text' : 'password'" @on-icon="isPass = !isPass">
            <template #icon>
              <icon-ri-eye-line v-if="isPass" class="size-5"></icon-ri-eye-line>
              <icon-ri-eye-off-line v-else class="size-5"></icon-ri-eye-off-line>
            </template>
          </v-text-field>
        </div>

        <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
          <div class="flex justify-between mb-4 md:col-span-2">
            <h3 class="text-lg font-semibold text-gray-800 dark:text-white/90 mb-0">{{ $t('ipSett') }}</h3>

            <div>
              <VCheckbox v-model="settings.wifiDhcp">{{ $t('dhcp') }}</VCheckbox>
            </div>
          </div>

          <v-text-field v-model="wifiIp" v-bind="wifiIpProps" :label="$t('ip')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiSubnet" v-bind="wifiSubnetProps" :label="$t('subnet')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiGateway" v-bind="wifiGatewayProps" :label="$t('gateway')" :disabled="isWifiDHCP" />

          <v-text-field v-model="wifiDns" v-bind="wifiDnsProps" :label="$t('dns')" :disabled="isWifiDHCP" />
        </div>
      </div>
    </card-main>

    <card-main :title="$t('security')">
      <template #header>
        <div class="col-span-full">
          <VCheckbox v-model="settings.authMode">{{ $t('auth') }}</VCheckbox>
        </div>
      </template>

      <div class="grid grid-cols-1 md:grid-cols-2 gap-x-4">
        <v-text-field v-model="authLogin" v-bind="authLoginProps" :label="$t('login')" :disabled="isAuth" />

        <v-text-field v-model="authPass" v-bind="authPassProps" :label="$t('pass')" class="col-end-2" :type="isAuthPass ? 'text' : 'password'" :disabled="isAuth" @on-icon="isAuthPass = !isAuthPass">
          <template #icon>
            <icon-ri-eye-line v-if="isAuthPass" class="size-5"></icon-ri-eye-line>
            <icon-ri-eye-off-line v-else class="size-5"></icon-ri-eye-off-line>
          </template>
        </v-text-field>

        <v-text-field v-model="reAuthPassword" v-bind="reAuthPasswordProps" :label="$t('pass')" :type="isAuthPass ? 'text' : 'password'" :disabled="isAuth" @on-icon="isAuthPass = !isAuthPass">
          <template #icon>
            <icon-ri-eye-line v-if="isAuthPass" class="size-5"></icon-ri-eye-line>
            <icon-ri-eye-off-line v-else class="size-5"></icon-ri-eye-off-line>
          </template>
        </v-text-field>
      </div>
    </card-main>

    <card-main :title="$t('update')">
      <BlockUpdate v-slot="{ file }" @update="onUpdate">
        <span>{{ file || $t('selectFile') }}</span>
      </BlockUpdate>
    </card-main>

    <card-main :title="$t('system')">
      <template #header>
        <button class="text-gray-400 cursor-pointer" @click="dialogInfo = true">
          <icon-ri-information-line class="size-5" />
        </button>
      </template>

      <BlockService :locale="getLocale()" :locales="listLocale" @locale="onLocale" @reset="onSureReset" @reboot="onSureReboot" />
    </card-main>

    <card-main :title="`${$t('discovery')} (UDP)`">
      <template #header>
        <div class="col-span-full">
          <VCheckbox v-model="settings.discovery">{{ $t('discovery') }}</VCheckbox>
        </div>
      </template>

      <v-text-field v-model="discoveryPort" v-bind="discoveryPortProps" :label="$t('port')" :disabled="!settings.discovery" />

      <v-text-field v-model="discoveryInterval" v-bind="discoveryIntervalProps" :label="$t('interval')" :disabled="!settings.discovery" />
    </card-main>

    <teleport to="[data-slot='device']">
      <v-select :items="listMenu" @change="onMenu">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>
      </v-select>
    </teleport>

    <v-dialog v-if="isDialog" :title="$t('scan')" @close="onClose">
      <BlockScan :items="scanList" @select="onSelectSsid" @scan="onScan" />
    </v-dialog>
  </div>
</template>
