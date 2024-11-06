export interface IoTConfigRequestData {
  server?: string
  accessToken?: string
}

export interface IPConfigRequestData {
  ipaddress?: string
  gateway?: string
  netmask?: string
  broadcast?: string
}

export interface EnvConfigRequestData {
  in1?: string
  in2?: string
}

export type ConfigResponseData = ApiResponseData<{
  iot: IoTConfigRequestData
  ip: IPConfigRequestData
  env: EnvConfigRequestData
}>
