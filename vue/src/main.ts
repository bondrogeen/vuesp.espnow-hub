import { createApp } from 'vue';
import { createPinia } from 'pinia';
import { useFetch } from '@vueuse/core';

import { struct } from '@/assets/js/';

import App from '@/App.vue';
import router from '@/router/index.ts';

import vuesp from 'vuesp-components';
import { i18nPlugin, i18nPluralizer } from 'vuesp-components/plugins';
import type { I18nPluralRule } from 'vuesp-components/types';

const customRules: Record<string, I18nPluralRule> = {
  ru: (count, forms) => {
    if (count === 1) return forms[0];
    if (count >= 2 && count <= 4) return forms[1];
    return forms[2];
  },
  en: (count, forms) => {
    return count === 1 ? forms[0] : forms[1];
  },
};

const pluralize = i18nPluralizer(customRules);

import '@/assets/tailwind.css';

const pinia = createPinia();
const app = createApp(App);

(async () => {
  if (import.meta.env.MODE === 'demo') {
    const { installDemoRuntime } = await import('@/demo/runtime');
    installDemoRuntime();
  }

  try {
    const res = await useFetch(`/default.json`).get().json();

    if (!res.data.value) {
      throw new Error('Failed to load configuration from device');
    }

    const { struct: structData, locales, ...resDefault } = res.data.value;
    struct.init(structData);
    pinia.use(({ store }) => {
      if (resDefault && store.$id === 'app') {
        store.$patch(resDefault);
        // @ts-ignore: Unreachable code error
        store.$patch({ pkg: __APP__ });
      }
    });
    app.use(i18nPlugin, { locales, pluralize });
    app.use(pinia);
    app.use(vuesp);
    app.use(router);
    app.mount('#appVuesp');
  } catch (error) {
    console.error('Failed to initialize application:', error);
    const errorEl = document.getElementById('appVuesp');
    if (errorEl) {
      errorEl.innerHTML = `
        <div style="padding: 2rem; text-align: center;">
          <h1>Failed to Load Application</h1>
          <p>${error instanceof Error ? error.message : 'Unknown error occurred'}</p>
          <p style="color: #666; margin-top: 1rem;">Please check that the device is accessible and try refreshing the page.</p>
          <button onclick="location.reload()" style="padding: 0.5rem 1rem; margin-top: 1rem; cursor: pointer;">
            Reload Page
          </button>
          <a href="/recovery?force=1" style="padding: 0.5rem 1rem; margin-top: 1rem; cursor: pointer;">
            Recovery
          </a>
        </div>
      `;
    }
  }
})();
