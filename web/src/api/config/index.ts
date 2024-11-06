import { request } from "@/utils/service"
import type * as Config from "./types/config"

/** 重启设备 */
export function rebootApi() {
  return request({
    url: "/reboot",
    method: "post"
  })
}

/** 物联网配置 */
export function createIoTConfigApi(data: Config.IoTConfigRequestData) {
  return request({
    url: "/iot_config",
    method: "post",
    data
  })
}

/** 网络配置 */
export function createIPConfigApi(data: Config.IPConfigRequestData) {
  return request({
    url: "/ip_config",
    method: "post",
    data
  })
}

/** 物联网配置 */
export function createEnvConfigApi(data: Config.EnvConfigRequestData) {
  return request({
    url: "/env_config",
    method: "post",
    data
  })
}

export function getConfigApi() {
  return request<Config.ConfigResponseData>({
    url: "/config",
    method: "get"
  })
}
