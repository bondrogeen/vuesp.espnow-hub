import { onMounted, ref, onUnmounted } from 'vue';

import { useAppStore } from '@/stores/AppStore';
import { useRouter, useRoute } from 'vue-router';

export const useFrame = () => {
  const appStore = useAppStore();

  const route = useRoute();
  const router = useRouter();

  let messageListener: ((event: MessageEvent) => void) | null = null;

  const isIframe = ref(false);

  onMounted(async () => {
    appStore.init(route.query);

    if (window.self !== window.top && import.meta.env.MODE !== 'demo') {
      isIframe.value = true;
      const html = document.querySelector('html');
      if (html) {
        html.classList.add('no-scrollbar');
      }

      messageListener = (event: MessageEvent) => {
        console.log('Data:', event);
        if (event?.data?.type === 'theme') {
          appStore.changeTheme(event.data.value);
        }
        if (event?.data?.type === 'route') {
          const data = event.data.data;
          router.push(data);
        }
      };
      window.addEventListener('message', messageListener);
    }
  });

  onUnmounted(() => {
    if (messageListener) window.removeEventListener('message', messageListener);
  });

  return { isIframe };
};
