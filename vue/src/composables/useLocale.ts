import type { I18nInstance } from 'vuesp-components/types';
import { inject } from 'vue';

import { localSet } from 'vuesp-components/helpers';

export const useLocale = () => {
  const i18n = inject<I18nInstance>('i18n');

  if (!i18n) {
    throw new Error('i18n not provided');
  }

  const setLocale = (locale: string) => {
    i18n.setLocale(locale);
    localSet('locale', i18n.getLocale());
  };

  const getLocale = () => i18n.getLocale();

  const $t = i18n.$t;

  const list = i18n.getListLocales();

  return { i18n, list, $t, setLocale, getLocale };
};
