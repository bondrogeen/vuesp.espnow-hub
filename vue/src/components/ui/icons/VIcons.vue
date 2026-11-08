<script setup lang="ts">
const { name } = defineProps<{ name: string }>();
</script>

<template>
  <icon-ri-shut-down-line v-if="name === 'shut'" class="size-full" />
  <icon-ri-plant-line v-if="name === 'plant'" class="size-full" />
  <icon-ri-lightbulb-line v-if="name === 'bulb'" class="size-full" />
  <icon-ri-video-on-line v-if="name === 'video'" class="size-full" />
  <icon-ri-windy-line v-if="name === 'windy'" class="size-full" />
  <icon-ri-camera-line v-if="name === 'camera'" class="size-full" />
  <icon-ri-flashlight-line v-if="name === 'light'" class="size-full" />
  <icon-ri-tv-line v-if="name === 'tv'" class="size-full" />
  <icon-ri-temp-cold-line v-if="name === 'therm'" class="size-full" />
  <icon-ri-database-line v-if="name === 'store'" class="size-full" />
  <icon-ri-outlet-line v-if="name === 'socket'" class="size-full" />
  <icon-ri-plug-line v-if="name === 'plug'" class="size-full" />
  <icon-ri-notification-line v-if="name === 'noti'" class="size-full" />
  <icon-ri-lock-line v-if="name === 'lock'" class="size-full" />
  <icon-ri-line-chart-line v-if="name === 'chart'" class="size-full" />
  <icon-ri-battery-line v-if="name === 'battery'" class="size-full" />
  <icon-ri-ram-line v-if="name === 'ram'" class="size-full" />
  <icon-ri-map-pin-line v-if="name === 'map'" class="size-full" />
  <icon-ri-speaker-line v-if="name === 'speaker'" class="size-full" />
  <icon-ri-timer-line v-if="name === 'timer'" class="size-full" />
  <icon-ri-snowflake-line v-if="name === 'snow'" class="size-full" />
  <icon-ri-blaze-fill v-if="name === 'blaze'" class="size-full" />
  <icon-ri-door-line v-if="name === 'door'" class="size-full" />
</template>
