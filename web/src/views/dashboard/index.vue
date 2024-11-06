<template>
  <div class="app-container center">
    <el-card class="w-full">
      <template #header>
        <div class="card-header">
          <span>貔貅物联网网关 配置面板</span>
        </div>
      </template>
      <div class="flex mb-4 items-center">
        <span>输入密码：</span>
        <el-input v-model="password" style="width: 240px" type="password" placeholder="请输入密码" show-password />
      </div>
      <el-tabs v-model="activeName" class="demo-tabs" @tab-click="handleClick">
        <el-tab-pane label="物联网配置" name="iotConfig"><IoTConfig :password="password" /></el-tab-pane>
        <el-tab-pane label="网络配置" name="ipConfig"><IPConfig :password="password" /></el-tab-pane>
        <el-tab-pane label="环境配置" name="envConfig"><EnvConfig :password="password" /></el-tab-pane>
      </el-tabs>
      <el-divider />
      <Reboot :password="password" />
    </el-card>
  </div>
</template>

<script lang="ts" setup>
import { ref } from "vue"
import type { TabsPaneContext } from "element-plus"
import Reboot from "./components/reboot/index.vue"
import IoTConfig from "./components/iot-config/index.vue"
import IPConfig from "./components/ip-config/index.vue"
import EnvConfig from "./components/env-config/index.vue"

const password = ref("")
const activeName = ref("iotConfig")

const handleClick = (tab: TabsPaneContext, event: Event) => {
  console.log(tab, event)
}
</script>

<style lang="scss" scoped>
.center {
  width: 100%;
  height: 100%;
  display: flex;
}

.demo-tabs > .el-tabs__content {
  padding: 32px;
  color: #6b778c;
  font-size: 32px;
  font-weight: 600;
}
</style>
