<script setup lang="ts">
import type { Ref } from 'vue';
import type { IListItem, IMessageFile, TypeMessage } from '@/types';
import { KEYS } from '@/utils/const';

import { watchEffect, ref, computed, nextTick } from 'vue';
import { useDebounceFn } from '@vueuse/core';
import { toByte } from 'vuesp-components/helpers';
import { useFiles } from '@/composables/useFiles';
import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import { VFile } from 'vuesp-components/components';

const { $t } = useLocale();

const mainMenu: IListItem<number>[] = [
  { name: $t('upload'), value: 2 },
  { name: $t('reload'), value: 3 },
  { name: $t('format'), value: 4 },
];
const listMenu: IListItem<number>[] = [
  { name: $t('download'), value: 1 },
  { name: $t('remove'), value: 2 },
];

const isLoading = ref(false);
const path = ref(['root']);
const files: Ref<IMessageFile[]> = ref([]);
const fullPath = computed(() => `${path.value.join('/').replace('root', '')}/`);
const sortFiles = computed(() => JSON.parse(JSON.stringify(files.value)).sort((a: IMessageFile, b: IMessageFile) => ((a.isFile || 0) > (b.isFile || 0) ? 1 : -1)));

const getListMenu = (isDir: boolean) => listMenu.filter((i) => (isDir ? i.value !== 1 : true));
const isLast = (path: string[], i: number) => path.length > i + 1;

const onMessage = ({ key, object }: TypeMessage) => {
  if (key === 'FILES' && object) files.value.push(object);
};

const { onSend, onDialog } = useConnection((send) => {
  send(KEYS.FILES, { command: 0, name: fullPath.value });
}, onMessage);

const onUpdate = () => {
  isLoading.value = true;
  files.value = [];
  onSend(KEYS.FILES, { command: 0, name: fullPath.value });
  onSend(KEYS.INFO);
};

const { fileName, createLink, onDelete, onUpload, onFormat, onClickUpload } = useFiles(fullPath, onUpdate);

const onPrev = (index: number) => {
  if (path.value.length > index + 1) {
    path.value = path.value.filter((_, i) => i < index + 1);
    nextTick(() => onUpdate());
  }
};

const onNext = (isDir: boolean, value: string) => {
  if (isDir && path) {
    path.value.push(value);
    onUpdate();
  }
};

const onEventService = ({ value }: IListItem<number>) => {
  if (value === 2) onClickUpload();
  if (value === 3) onUpdate();
  if (value === 4) onSureFormat();
};

const onSureFormat = () => onDialog({ value: true, message: $t('dialog.allDel'), callback: onFormat });

const onEventList = (name: string, { value }: IListItem<number>) => {
  if (value === 1) createLink(name);
  if (value === 2) onSureDelete(name);
};

const onSureDelete = (name: string) => {
  if (fileName(name).includes('www')) {
    onDialog({ value: true, message: $t('dialog.wwwDir'), callback: onDelete.bind(this, name) });
  } else {
    onDelete(name);
  }
};

const onLoad = useDebounceFn((_?: IMessageFile[]) => {
  isLoading.value = false;
}, 300);

watchEffect(() => {
  onLoad(files.value);
});
</script>

<template>
  <div class="grid grid-cols-1 xl:grid-cols-2">
    <card-main :title="$t('fb')">
      <div class="px-4 pb-2 flex items-center">
        <div class="flex gap-2 items-center font-bold flex-auto">
          <div
            v-for="(value, i) of path"
            :key="value"
            class="flex items-center gap-2 cursor-pointer text-gray-600 last:cursor-default last:text-gray-900 dark:text-gray-200 dark:last:text-white"
            @click="onPrev(i)"
          >
            <div>{{ value }}</div>

            <icon-ri-arrow-right-s-line v-if="isLast(path, i)" class="h-5 w-4"></icon-ri-arrow-right-s-line>
          </div>
        </div>

        <VFile multiple="multiple" @change="onUpload"></VFile>
      </div>

      <div class="relative min-h-[260px]">
        <v-loader v-if="isLoading" class="absolute top-1/2 left-1/2 text-primary" />

        <v-list v-slot="{ item: { name, size, isDir, isFile } }" :items="sortFiles" class="">
          <div class="flex items-center flex-auto" @click="onNext(isDir, name)">
            <div class="mr-4 text-gray-400">
              <icon-ri-folder-6-line v-if="isDir"></icon-ri-folder-6-line>
              <icon-ri-file-line v-else></icon-ri-file-line>
            </div>

            <div>
              <div class="text-sm">{{ isDir ? `${name}` : name }}</div>

              <div v-if="isFile" class="text-sm text-[10px] text-gray-400">{{ toByte(size) }} ({{ size }})</div>
            </div>
          </div>

          <v-select :items="getListMenu(isDir)" @change="onEventList(name, $event)">
            <template #activator="{ on }">
              <v-button @click="on.click">
                <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
              </v-button>
            </template>
          </v-select>
        </v-list>
      </div>
    </card-main>

    <Teleport to="[data-slot='device']">
      <v-select :items="mainMenu" @change="onEventService">
        <template #activator="{ on }">
          <v-button type="icon" @click="on.click">
            <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
          </v-button>
        </template>
      </v-select>
    </Teleport>
  </div>
</template>
