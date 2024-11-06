<script lang="ts" setup>
import AppMain from "./components/AppMain.vue"
</script>

<template>
  <div class="app-wrapper">
    <!-- 主容器 -->
    <div class="main-container">
      <!-- 页面主体内容 -->
      <AppMain class="app-main" />
    </div>
  </div>
</template>

<style lang="scss" scoped>
@import "@/styles/mixins.scss";
$transition-time: 0.35s;

.app-wrapper {
  @extend %clearfix;
  position: relative;
  width: 100%;
}

.main-container {
  min-height: 100%;
  transition: margin-left $transition-time;
  position: relative;
}

.app-main {
  min-height: calc(100vh - var(--v3-navigationbar-height));
  position: relative;
  overflow: hidden;
}
</style>
