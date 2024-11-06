<script lang="ts" setup>
interface Props {
  password: string
}

const props = defineProps<Props>()

import { onMounted, reactive, ref } from "vue"
import { ComponentSize, ElMessage, FormInstance, FormRules } from "element-plus"
import { createIoTConfigApi, getConfigApi } from "../../../../api/config"
import { TOKEN } from "@/constants/token"

interface RuleForm {
  server?: string
  accessToken?: string
}
const loading = ref<boolean>(false)
const formSize = ref<ComponentSize>("default")
const ruleFormRef = ref<FormInstance>()
const ruleForm = ref<RuleForm>({
  server: "",
  accessToken: ""
})


const rules = reactive<FormRules<RuleForm>>({
  server: [
    // 表单校验
    {
      required: true,
      message: "请输入IoTSharp的服务器地址",
      trigger: "blur"
    }
  ],
  accessToken: [
    {
      required: true,
      message: "请输入设备的Token",
      trigger: "blur"
    }
  ]
})

const getData = () => {
  getConfigApi()
    .then((res) => {
      const { iot } = res.data
      ruleForm.value = iot
    })

    .finally(() => {
      loading.value = false
    })
}
const submitForm = async (formEl: FormInstance | undefined) => {
  if (!formEl) return

  await formEl.validate((valid, fields) => {
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
    if (!valid) return console.error("表单校验不通过", fields)
    loading.value = true
    createIoTConfigApi(ruleForm.value)
      .then(() => {
        ElMessage.success("操作成功")
        getData()
      })
      .catch(() => {
        ElMessage.success("操作失败")
      })
      .finally(() => {
        loading.value = false
      })
  })
}

const resetForm = (formEl: FormInstance | undefined) => {
  if (!formEl) return
  formEl.resetFields()
}

onMounted(() => {
  getData()
})
</script>

<template>
  <el-form
    ref="ruleFormRef"
    style="max-width: 600px"
    :model="ruleForm"
    :rules="rules"
    label-width="auto"
    class="demo-ruleForm"
    :size="formSize"
    status-icon
  >
    <el-form-item label="IoTSharp服务器地址" prop="server">
      <el-input v-model="ruleForm.server" />
    </el-form-item>
    <el-form-item label="设备Token" prop="accessToken">
      <el-input v-model="ruleForm.accessToken" />
    </el-form-item>
 
    <el-form-item>
      <el-button type="primary" @click="submitForm(ruleFormRef)" :loading="loading">提交</el-button>
      <el-button @click="resetForm(ruleFormRef)">重置</el-button>
    </el-form-item>
  </el-form>
</template>
