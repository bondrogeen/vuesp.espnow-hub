<script setup lang="ts">
import { useDebounceFn } from '@vueuse/core';
import { KEYS } from '@/utils/const';
import { computed, ref } from 'vue';

import { ScriptType, examples } from '@/assets/js/script';

import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';
import { useLogs } from '@/composables/script/useLogs';
import { useScripts } from '@/composables/script/useScripts';
import { useSlots } from '@/composables/script/useSlots';
import { normalizeScript } from 'vuesp-components/helpers';
import { useFiles } from '@/composables/useFiles';

import { VFile, ScriptEditor, ScriptViewDocs, VTooltip, ScriptDialog } from 'vuesp-components/components';

const { $t } = useLocale();

const { suggestions, message, main, onSend } = useConnection((send) => {
  send(KEYS.MESSAGE, { ...message.value, id: 0, type: ScriptType.SCRIPT_GET_ALL_SLOT });
});

const { logs, onHover } = useLogs(message);
const { slotInfo, currentSlot, onSlot, getColorSlot } = useSlots(main);
const { PATH, ids, content, scripts, idScript, selectedScript, addScript, onRemove, onSelect, onSaveScript, isScriptSave, onExample, onLoad } = useScripts();
const fullPath = computed(() => '/');
const onUpdate = () => {
  onLoad();
};
const { createLink, onUpload, onClickUpload } = useFiles(fullPath, onUpdate);

const dialogAdd = ref(false);
const onAddScriptDialog = () => {
  const getId = Array.from({ length: 256 }, (_, i) => i).find((i) => !ids.value.includes(i));
  idScript.value = getId || 0;
  dialogAdd.value = true;
};

const dialogViewDocs = ref(false);

const onUpdateScript = useDebounceFn(() => onSend(KEYS.MESSAGE, { ...message.value, id: 0, type: ScriptType.SCRIPT_GET_ALL_SLOT }), 1000);

const onRunScript = () => {
  const id = selectedScript.value?.id || 0;
  onSend(KEYS.MESSAGE, { ...message.value, id, type: ScriptType.SCRIPT_START, text: content.value });
  onUpdateScript();
};

const onStopScript = () => {
  onSend(KEYS.MESSAGE, { ...currentSlot.value, type: ScriptType.SCRIPT_STOP, text: '' });
  onUpdateScript();
};

const onStopScriptAll = () => {
  onSend(KEYS.MESSAGE, { ...message.value, type: ScriptType.SCRIPT_STOP_ALL, text: '' });
  onUpdateScript();
};

const onRemoveScript = () => {
  onSend(KEYS.MESSAGE, { ...currentSlot.value, type: ScriptType.SCRIPT_REMOVE, text: '' });
  onUpdateScript();
};
</script>

<template>
  <div class="container mx-auto">
    <div class="flex flex-col gap-6 mx-auto">
      <div class="flex justify-between">
        <h1>{{ $t('menu.script') }}</h1>

        <div>
          <v-button icon @click="dialogViewDocs = true">
            <icon-ri-file-info-line />
          </v-button>
        </div>
      </div>

      <div
        class="grid [grid-template-areas:'editor''logs''list'] xl:[grid-template-areas:'list_editor_editor_editor_editor''logs_editor_editor_editor_editor'] xl:grid-rows-[370px_1fr] grid-cols-1 xl:grid-cols-[400px_1fr_1fr_1fr] gap-3 sm:gap-4"
      >
        <card-main :title="$t('list')" class="order-2 md:order-1 [grid-area:list]">
          <template #header>
            <div class="flex gap-3">
              <v-button color="transparent" class="size-6 text-gray-500" :title="$t('export')" :disabled="false" @click="createLink(PATH)">
                <icon-ri-upload-2-line></icon-ri-upload-2-line>
              </v-button>

              <v-button color="transparent" class="size-6 text-gray-500" :title="$t('import')" :disabled="false" @click="onClickUpload()">
                <icon-ri-download-2-line></icon-ri-download-2-line>
              </v-button>

              <v-button color="transparent" class="size-6 text-gray-500" :title="$t('add')" :disabled="false" @click="onAddScriptDialog()">
                <icon-ri-sticky-note-add-line class=""></icon-ri-sticky-note-add-line>
              </v-button>
              <VFile accept=".txt" @change="onUpload($event, PATH)"></VFile>
            </div>
          </template>

          <ul class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-2 h-40 scrollbar">
            <li v-for="script of scripts" :key="script.id" class="group text-sm cursor-pointer transition-all flex items-center justify-between hover:opacity-80 px-1" @click="onSelect(script)">
              <div class="flex items-center gap-2 min-w-0">
                <span>#{{ script.id }}</span>
                <span class="truncate">{{ script.name }}</span>
              </div>

              <div class="group flex items-center gap-1">
                <v-button
                  color="transparent"
                  class="size-6"
                  :class="isScriptSave(script) ? 'text-gray-500' : 'text-blue-500 animate-shake'"
                  :title="$t('save')"
                  :disabled="isScriptSave(script)"
                  @click="onSaveScript"
                >
                  <icon-ri-save-line class=""></icon-ri-save-line>
                </v-button>
                <v-button color="transparent" class="size-6 text-gray-500 hover:text-red-400" :title="$t('remove')" @click.stop="onRemove(script)">
                  <icon-ri-delete-bin-line class="transition-all"></icon-ri-delete-bin-line>
                </v-button>
              </div>
            </li>

            <li v-if="scripts.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noScripts') }}</li>
          </ul>

          <div class="px-2 border-t border-gray-200 dark:border-gray-700 text-slate-400 flex justify-between mt-4 text-xs">
            <span>{{ scripts.length }} {{ $t('script', { count: `${scripts.length}` }) }}</span>
          </div>

          <div class="mt-4">
            <div class="flex items-center justify-between">
              <h3 class="text-lg font-semibold text-gray-800 dark:text-white/90 mb-2">{{ $t('slots') }}</h3>

              <div class="flex gap-2">
                <v-button color="transparent" class="size-5 px-0! text-gray-500 not-disabled:hover:text-blue-600" :disabled="!currentSlot?.active" @click="onStopScript">
                  <icon-ri-stop-circle-line class="transition-all size-5"></icon-ri-stop-circle-line>
                </v-button>

                <v-button color="transparent" class="size-5 px-0! text-gray-500 not-disabled:hover:text-red-600" :disabled="!currentSlot?.text" @click="onRemoveScript">
                  <icon-ri-close-circle-line class="transition-all size-5"></icon-ri-close-circle-line>
                </v-button>

                <v-button color="transparent" class="size-5" :title="$t('update')" @click="onUpdateScript">
                  <icon-ri-refresh-line class="text-gray-500"></icon-ri-refresh-line>
                </v-button>

                <v-button color="transparent" class="size-5" :title="$t('stop')" :disabled="!slotInfo.used" @click="onStopScriptAll">
                  <icon-ri-stop-circle-fill class="text-gray-500"></icon-ri-stop-circle-fill>
                </v-button>
              </div>
            </div>

            <ul class="flex flex-wrap gap-1 mb-2">
              <li v-for="slot in main.slots" :key="slot.index" class="group text-sm transition-all flex items-center rounded">
                <VTooltip>
                  <template #activator>
                    <v-button class="px-0! rounded-none" :class="getColorSlot(slot)" color="" size="none" @click="onSlot(slot)">
                      <div class="flex-[0_0_8px] h-4 w-2"></div>
                    </v-button>
                  </template>
                  <template #default="{ show }">
                    <div v-if="show" class="text-sm absolute bottom-full left-0 bg-white-main px-3 py-2 rounded shadow-nav z-5 w-50 bg-white dark:bg-gray-800">
                      <div class="text-slate-400">{{ slot?.id === 255 ? '' : `id:${slot?.id}` }} ({{ slot.active ? 'active' : slot.handler ? 'event' : slot?.id === 255 ? 'empty' : 'stopped' }})</div>

                      <p>{{ slot?.text }}</p>
                    </div>
                  </template>
                </VTooltip>
              </li>
            </ul>
          </div>

          <div class="px-2 border-t border-gray-200 dark:border-gray-700 text-slate-400 flex text-xs">
            <span>{{ `${$t('total')}: ${slotInfo.total} ${$t('used')}: ${slotInfo.used}` }}</span>
          </div>
        </card-main>

        <card-main :title="`${$t('editor')}  ${selectedScript?.name ? selectedScript?.name : ''}`" class="order-1 [grid-area:editor]">
          <template #header>
            <div class="flex gap-3">
              <v-button
                color="transparent"
                class="size-6"
                :class="!Boolean(content.trim()) ? 'text-gray-500' : 'text-green-500'"
                :title="$t('run')"
                :disabled="!Boolean(content.trim()) || !selectedScript?.name"
                @click="onRunScript()"
              >
                <icon-ri-play-circle-line />
              </v-button>

              <v-select :items="examples" @change="onExample">
                <template #activator="{ on }">
                  <v-button color="transparent" class="size-6" :disabled="!selectedScript" @click="on.click">
                    <icon-ri-file-list-3-line class="text-gray-500"></icon-ri-file-list-3-line>
                  </v-button>
                </template>
              </v-select>
            </div>
          </template>

          <div class="flex-auto bg-gray-50 dark:bg-gray-900 dark:border-gray-700 w-full overflow-auto scrollbar h-50 xl:h-[calc(100dvh-300px)]">
            <ScriptEditor v-if="selectedScript?.name" v-model="content" class="h-full w-full overflow-auto scrollbar" :suggestions="suggestions"></ScriptEditor>
            <p v-else class="text-gray-500 ms-3 text-center mt-4">{{ $t('selectScript') }}</p>
          </div>

          <div class="text-sm border-t border-gray-200 dark:border-gray-700 text-slate-400 flex flex-wrap justify-between mt-4">
            <span>
              <i class="far fa-file-alt mr-1"></i>
              {{ `${$t('length')}: ${normalizeScript(content)?.length || 0}` }}
            </span>
          </div>
        </card-main>

        <card-main :title="$t('logs')" class="order-1 [grid-area:logs]">
          <template #header>
            <v-button color="transparent" class="size-6 opacity-60 hover:opacity-100" size="icon" @click="logs = []">
              <icon-ri-eraser-line class="text-gray-500"></icon-ri-eraser-line>
            </v-button>
          </template>

          <ul
            ref="container"
            class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-4 max-h-50 min-h-50 scrollbar"
            @mouseenter="onHover(true)"
            @mouseleave="onHover(false)"
          >
            <li v-for="(log, idx) in logs" :key="idx" class="py-0.5 flex items-start gap-2 border-b border-slate-600/50 last:border-0 text-sm">
              <span class="text-slate-400 whitespace-nowrap">{{ log.time }}</span>
              <span>
                {{ log.text }}
              </span>
            </li>
            <li v-if="logs.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noLogs') }}</li>
          </ul>
        </card-main>
      </div>
    </div>

    <v-dialog v-if="dialogAdd" size="sm" :title="`${$t('add')} #${idScript}`" @close="dialogAdd = false">
      <ScriptDialog :idScript="idScript" @add="addScript" @close="dialogAdd = false" />
    </v-dialog>

    <v-dialog v-if="dialogViewDocs" size="lg" @close="dialogViewDocs = false">
      <ScriptViewDocs class="min-h-100" />
    </v-dialog>
  </div>
</template>
