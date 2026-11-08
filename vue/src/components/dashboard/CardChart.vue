<script setup lang="ts">
import { computed } from 'vue';

import type { ICardChartProps, ICardChartEmit } from './types';

import { loadScript, timeUtcToString } from 'vuesp-components/helpers';
import { CardBase } from 'vuesp-components/components';

const props = defineProps<ICardChartProps>();

const emit = defineEmits<ICardChartEmit>();

declare global {
  interface Window {
    Chart: any;
  }
}

const getID = ({ id }: ICardChartProps): string => `chart-` + id.replace('.', '_');

const data = computed(() => {
  const { data = [], head = 0, count = 0, tail = 0 } = props.value || {};
  if (count === 0) return [];

  const result = [];
  if (head <= tail) {
    for (let i = tail; i <= count - 1; i++) result.push(data[i]);
    for (let i = 0; i <= head - 1; i++) result.push(data[i]);
  } else {
    for (let i = tail; i <= head - 1; i++) result.push(data[i]);
  }
  return result;
});

const dataLength = computed(() => data.value.length);
const getValue = computed(() => (data.value.reduce((acc: number, i: number) => acc + i, 0) / dataLength.value).toFixed(2));

const getTime = (i: number) => new Date(Date.now() - (dataLength.value - i) * 10000);

let chart: any;

const createChart = () => {
  const el = document.querySelector(`#${getID(props)}`);
  if (!el) return;
  const Chart = window.Chart;
  chart = new Chart(el, {
    type: 'line',
    data: {
      labels: new Array(dataLength.value).fill(0).map((_, i) => timeUtcToString(getTime(i), { minute: '2-digit', hour: '2-digit', second: '2-digit' })),
      datasets: [{ label: 'Temp', data: data.value, borderWidth: 1 }],
    },
    options: {
      scales: { x: { display: true }, y: { display: true } },
      animations: {
        radius: { duration: 400, easing: 'linear', loop: (context: any) => context.active },
      },
      hoverRadius: 6,
      hoverBackgroundColor: 'blue',
      interaction: {
        mode: 'nearest',
        intersect: false,
        axis: 'x',
      },
    },
  });
};

const onUpdate = (e: Event) => {
  if (chart) {
    chart.destroy();
    emit('update', e);
    createChart();
  }
};

const openDialog = async () => {
  // https://www.chartjs.org/
  await loadScript('https://cdn.jsdelivr.net/npm/chart.js');
  createChart();
};
</script>

<template>
  <CardBase v-bind="{ ...props, value: getValue }" size="lg" @click="emit('click', $event)" @edit="emit('edit', $event)" @clone="emit('clone', $event)" @open="openDialog">
    <template #icon="item">
      <slot name="icon" v-bind="item"></slot>
    </template>

    <template #value>{{ getValue }} °C</template>
    <template #dialog-value>Average {{ getValue }} °C</template>

    <template #dialog>
      <div class="w-full overflow-x-auto scrollbar">
        <div class="h-[300px] min-w-10" :style="{ width: dataLength * 30 + 'px' }">
          <canvas :id="getID(props)" height="300" width="0"></canvas>
        </div>
      </div>
      <v-button class="mt-4" @click="onUpdate">{{ $t('btnUpdate') }}</v-button>
    </template>
  </CardBase>
</template>
