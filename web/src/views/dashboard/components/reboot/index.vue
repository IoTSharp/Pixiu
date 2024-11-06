<script lang="ts" setup>
interface Props {
  password: string
}

const props = defineProps<Props>()

import { ElMessage, ElMessageBox } from "element-plus"
import { ref } from "vue"
import { rebootApi } from "../../../../api/config"
import { TOKEN } from "@/constants/token"

const loading = ref<boolean>(false)

const handleReboot = () => {
  if (!props.password) {
    ElMessage({
      message: "请输入密码",
      type: "warning"
    })
    return
  }
  if (props.password !== TOKEN) {
    ElMessage({
      message: "密码错误",
      type: "warning"
    })
    return
  }
  loading.value = true

  ElMessageBox.confirm("是否重启设备？", "提示", {
    confirmButtonText: "确定",
    cancelButtonText: "取消",
    type: "warning"
  }).then(() => {
    rebootApi()
      .then(() => {
        ElMessage.success("操作成功")
      })
      .catch(() => {
        ElMessage.success("操作失败")
      })
      .finally(() => {
        loading.value = false
      })
  })
}
</script>

<template>
  <el-button type="danger" @click="handleReboot" :loading="loading">重启设备</el-button>
</template>
