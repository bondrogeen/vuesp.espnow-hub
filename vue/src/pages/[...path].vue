<script setup lang="ts">
import { useRoute } from 'vue-router';

const route = useRoute();
</script>

<template>
  <div class="flex flex-col items-center justify-center h-full">
    <h1 class="text-8xl">
      4
      <span class="text-blue-700 dark:text-blue-800">0</span>
      4
    </h1>

    <h2 class="text-3xl">{{ $t('message.pageNotFound') }}</h2>

    <p class="text-gray-500 my-4">{{ $t('message.pathExist', { path: route.fullPath }) }}</p>

    <v-button to="/">
      {{ $t('home') }}
    </v-button>
  </div>
</template>
