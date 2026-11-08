import { PATH_FS } from '@/utils/const';

import { createDownloadLink } from 'vuesp-components/helpers';
import { useFetch } from '@vueuse/core';
import { useConnection } from '@/composables/useConnection';
import type { ComputedRef } from 'vue';

export const useFiles = (fullPath: ComputedRef<string>, onUpdate?: () => void) => {
  const { main, onDialog } = useConnection();

  const fileName = (name: string) => `${fullPath.value}${name}`;

  const onUpload = async (files: FileList | null, fileName?: string) => {
    if (!files) return;
    let totalSize = 0;
    const body = new FormData();
    for (let i = 0; i < files.length; i++) {
      const file = files.item(i);
      if (!file) return;
      totalSize += file.size;
      body.append(`file[${i}]`, file, fileName || `${fullPath.value}${file.name}`);
    }
    const { totalBytes = 0, usedBytes = 0 } = main.value.info;
    if (totalSize < totalBytes - usedBytes) {
      const { data } = await useFetch(PATH_FS, { body }).post().json();
      if (data.value?.state) onUpdate?.();
    } else {
      onDialog({ value: true, message: 'No free space' });
    }
  };

  const onDelete = async (name: string) => {
    const { data } = await useFetch(`${PATH_FS}?file=${fileName(name)}`)
      .delete()
      .json();
    if (data.value?.state) onUpdate?.();
    else onDialog({ value: true, message: 'Directory is not empty' });
  };

  const onFormat = async () => {
    const { data } = await useFetch(`${PATH_FS}?format=true`).post().json();
    if (data.value?.state) onUpdate?.();
  };

  const createLink = (name: string) => createDownloadLink(`${PATH_FS}?file=${fileName(name)}`, name);

  const onClickUpload = () => {
    const el: any = document.querySelector('input[type="file"]');
    if (el) el.click();
  };

  return { fileName, onUpload, onDelete, onFormat, createLink, onClickUpload };
};
