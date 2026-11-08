import { useFetch } from '@vueuse/core';
import { computed, onMounted, ref } from 'vue';
import { normalizeScript, formatScript } from 'vuesp-components/helpers';
import { useKeys } from '@/composables/script/useKeys';
import type { IListItem } from 'vuesp-components/types';
interface IScript {
  id: number;
  name: string;
  content: string;
}

export const useScripts = () => {
  const PATH = '/www/scripts.txt';
  const idScript = ref(0);
  const scripts = ref<IScript[]>([]);
  const selectedScript = ref<IScript | null>(null);
  const content = ref('');

  const ids = computed(() => scripts.value.map(({ id }) => id));

  const isScriptSave = (script: IScript) => {
    if (selectedScript.value?.id === script.id && selectedScript.value?.content !== normalizeScript(content.value)) return false;
    return true;
  };

  const onRemove = ({ id }: IScript) => {
    scripts.value = scripts.value.filter((i) => i.id !== id);
    selectedScript.value = null;
    content.value = '';
    onSave();
  };
  const onSaveScript = () => {
    if (!selectedScript.value) return;
    const { id } = selectedScript.value;
    scripts.value = scripts.value.map((i) => (i.id === id ? { ...i, content: normalizeScript(content.value) } : i));
    selectedScript.value.content = normalizeScript(content.value);
    onSave();
  };
  const onSelect = (script: IScript) => {
    selectedScript.value = script;
    content.value = formatScript(script.content);
  };

  const addScript = (name: string) => {
    scripts.value.push({ id: idScript.value, name: name, content: '' });
  };

  const onExample = (item: IListItem<string>) => {
    content.value = formatScript(item.value);
  };

  const onLoad = async () => {
    const { data } = await useFetch(`/fs?file=${PATH}`).text();
    if (!data.value) return;
    const lines = data.value.split('\n');
    scripts.value = lines.map((script): IScript => {
      const parts = script.split(':');
      const [id, name] = parts;
      return { id: +id, name, content: parts.slice(2).join(':') };
    });
    if (scripts.value?.[0]) {
      selectedScript.value = scripts.value?.[0] || null;
      content.value = formatScript(scripts.value?.[0]?.content);
    }
  };

  const onSave = async () => {
    const text = scripts.value.map(({ id, name, content }) => `${id}:${name}:${content}`).join('\n');
    const body = new FormData();
    body.append('file[0]', new Blob([text], { type: 'text/plain' }), PATH);
    return await useFetch('/fs', { body }).post();
  };

  onMounted(() => {
    onLoad();
  });

  const onFormat = () => {
    content.value = formatScript(content.value);
  };

  useKeys({ format: onFormat, save: onSaveScript });
  return {
    PATH,
    ids,
    content,
    scripts,
    idScript,
    selectedScript,
    onLoad,
    onSave,
    onRemove,
    onSelect,
    addScript,
    onExample,
    onSaveScript,
    isScriptSave,
  };
};
