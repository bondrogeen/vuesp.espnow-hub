import { defineStore } from 'pinia';
import { changeTheme, localGet, localSet, isNewVersion } from 'vuesp-components/helpers';
import { useFetch } from '@vueuse/core';
import { pathList } from '@/utils/const';

import type { IDialog, IAppStore, INotification } from '@/types';

const initialState = (): IAppStore => ({
  theme: localGet('theme') || 'dark',
  dialog: { value: false },
  suggestions: [],
  menu: [],
  dashboard: [],
  dialogInfo: isNewVersion(),
  pkg: { name: '', version: '' },
  notifications: localGet('notifications', true) || [],
});

export const useAppStore = defineStore('app', {
  state: initialState,
  actions: {
    async init({ theme }: { theme?: string }) {
      if (theme && ['light', 'dark'].includes(theme)) this.theme = theme;
      changeTheme(this.theme);
      const { data } = await useFetch(`/fs?file=${pathList}`).json();
      if (data.value) this.dashboard = data.value;
    },
    changeTheme(value?: string) {
      this.theme = value || (this.theme === 'light' ? 'dark' : 'light');
      changeTheme(this.theme);
    },
    setDialog(data: IDialog) {
      this.dialog = data;
    },
    setNotification({ color = 1, isNew = 1, timeout = 0, text = '', date = 0 }: Partial<INotification>) {
      this.notifications = [...this.notifications, { color, isNew, timeout, text, date: date || Date.now() }];
      localSet('notifications', this.notifications);
    },
    readNotification(notification: INotification) {
      this.notifications = this.notifications.map((i) => (notification.date === i.date ? { ...i, isNew: 0 } : i));
      localSet('notifications', this.notifications);
    },
    readAllNotification() {
      this.notifications = this.notifications.map((i) => ({ ...i, isNew: 0 }));
      localSet('notifications', this.notifications);
    },
    removeNotification(notification: INotification) {
      this.notifications = this.notifications.filter((i) => i.date !== notification.date);
      localSet('notifications', this.notifications);
    },
  },
});
