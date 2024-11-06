<script lang="ts" setup>
interface Props {
  password: string
}

const props = defineProps<Props>()

import { onMounted, reactive, ref } from "vue"
import { ComponentSize, ElMessage, FormInstance, FormRules } from "element-plus"
import { createIPConfigApi, getConfigApi } from "../../../../api/config"
import { TOKEN } from "@/constants/token"

interface RuleForm {
  ipaddress?: string
  gateway?: string
  netmask?: string
  broadcast?: string
}
const loading = ref<boolean>(false)
const formSize = ref<ComponentSize>("default")
const ruleFormRef = ref<FormInstance>()
const ruleForm = ref<RuleForm>({
  ipaddress: "",
  gateway: "",
  netmask: "",
  broadcast: ""
})

const validateIP = (rule: any, value: any, callback: any) => {
  if (value === "" || value === undefined || value === null) {
    callback(new Error("请输入 IP 地址"))
  } else {
    const reg =
      /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$/
    if (!reg.test(value) && value != "") {
      callback(new Error("请输入正确的 IP 地址"))
    } else {
      callback()
    }
  }
}

const validateGateway = (rule: any, value: any, callback: any) => {
  if (value === "" || value === undefined || value === null) {
    callback(new Error("请输入网关"))
  } else {
    const reg =
      /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$/
    if (!reg.test(value) && value != "") {
      callback(new Error("请输入正确的网关"))
    } else {
      callback()
    }
  }
}

const validateNetmask = (rule: any, value: any, callback: any) => {
  if (value === "" || value === undefined || value === null) {
    callback(new Error("请输入子网掩码"))
  } else {
    const reg =
      /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$/
    if (!reg.test(value) && value != "") {
      callback(new Error("请输入正确的子网掩码"))
    } else {
      callback()
    }
  }
}

const validateBroadcast = (rule: any, value: any, callback: any) => {
  if (value === "" || value === undefined || value === null) {
    callback(new Error("请输入广播地址"))
  } else {
    const reg =
      /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$/
    if (!reg.test(value) && value != "") {
      callback(new Error("请输入正确的广播地址"))
    } else {
      callback()
    }
  }
}

const rules = reactive<FormRules<RuleForm>>({
  ipaddress: [
    // 表单校验
    {
      required: true,
      validator: validateIP,
      trigger: "blur"
    }
  ],
  gateway: [
    // 表单校验
    {
      required: true,
      validator: validateGateway,
      trigger: "blur"
    }
  ],
  netmask: [
    // 表单校验
    {
      required: true,
      validator: validateNetmask,
      trigger: "blur"
    }
  ],
  broadcast: [
    // 表单校验
    {
      required: true,
      validator: validateBroadcast,
      trigger: "blur"
    }
  ]
})

const getData = () => {
  getConfigApi()
    .then((res) => {
      const { ip } = res.data
      ruleForm.value = ip
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
    createIPConfigApi(ruleForm.value)
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
    <el-form-item label="IP地址" prop="ipaddress">
      <el-input v-model="ruleForm.ipaddress" />
    </el-form-item>
    <el-form-item label="网关" prop="gateway">
      <el-input v-model="ruleForm.gateway" />
    </el-form-item>
    <el-form-item label="子网掩码" prop="netmask">
      <el-input v-model="ruleForm.netmask" />
    </el-form-item>
    <el-form-item label="广播地址" prop="broadcast">
      <el-input v-model="ruleForm.broadcast" />
    </el-form-item>
    <el-form-item>
      <el-button type="primary" @click="submitForm(ruleFormRef)" :loading="loading">提交</el-button>
      <el-button @click="resetForm(ruleFormRef)">重置</el-button>
    </el-form-item>
  </el-form>
</template>
