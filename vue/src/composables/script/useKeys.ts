import { useMagicKeys, whenever } from '@vueuse/core';

export const useKeys = ({ format, save }: Record<string, () => void>) => {
  const keys = useMagicKeys({
    passive: false,
    onEventFired(e) {
      if (e.ctrlKey && e.code === 'KeyS' && e.type === 'keydown') e.preventDefault();
      if (e.ctrlKey && e.code === 'KeyF' && e.type === 'keydown') e.preventDefault();
    },
  });
  const ctrl_f = keys['control+keyf'];
  const ctrl_s = keys['control+keys'];

  whenever(ctrl_s, () => {
    if (save) save();
  });
  whenever(ctrl_f, () => {
    if (format) format();
  });
  return {};
};
