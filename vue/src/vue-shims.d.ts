declare module '*.vue' {
  import { defineComponent } from 'vue';
  const component: ReturnType<typeof defineComponent>;
  export default component;
}

import type { I18nInstance, I18nGet } from './i18n';

declare module '@vue/runtime-core' {
  interface ComponentCustomProperties {
    $i18n: I18nInstance;
    $t: I18nGet;
  }
}

export {};
