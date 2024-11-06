<script lang="ts" setup>
interface Props {
  password: string
}

const props = defineProps<Props>()

import { onMounted, reactive, ref } from "vue"
import { ComponentSize, ElMessage, FormInstance, FormRules } from "element-plus"
import { createEnvConfigApi, getConfigApi } from "../../../../api/config"
import { TOKEN } from "@/constants/token"

interface RuleForm {
  in1?: string
  in2?: string
}
const loading = ref<boolean>(false)
const formSize = ref<ComponentSize>("default")
const ruleFormRef = ref<FormInstance>()
const ruleForm = ref<RuleForm>({
  in1: "",
  in2: ""
})

const rules = reactive<FormRules<RuleForm>>({
  in1: [
    {
      required: true,
      message: "请输入 in1",
      trigger: "blur"
    }
  ],
  in2: [
    {
      required: true,
      message: "请输入 in1",
      trigger: "blur"
    }
  ]
})

const getData = () => {
  getConfigApi()
    .then((res) => {
      const { env } = res.data
      ruleForm.value = env
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
    createEnvConfigApi(ruleForm.value)
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
    <el-form-item label="in1" prop="in1">
      <el-input v-model="ruleForm.in1" />
    </el-form-item>
    <el-form-item label="in2" prop="in2">
      <el-input v-model="ruleForm.in2" />
    </el-form-item>

    <el-form-item>
      <el-button type="primary" @click="submitForm(ruleFormRef)" :loading="loading">提交</el-button>
      <el-button @click="resetForm(ruleFormRef)">重置</el-button>
    </el-form-item>
  </el-form>
</template>
