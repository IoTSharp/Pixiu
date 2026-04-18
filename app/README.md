## Edge Task Receipt Example

本目录新增了一个最小执行端回执示例：

- `edge_task_receipt_example.c`

用途：

- 演示 PiXiu 完成平台任务后如何回调 IoTSharp 的 `POST /api/EdgeTask/Receipt`
- 当前示例已经接入现有 `curl_utils.c`，会真正发起 HTTP POST 请求

最小字段：

- `contractVersion=edge-task-v1`
- `taskId`
- `targetType=PixiuRuntime`
- `targetKey=deviceId:runtimeType:instanceId`
- `runtimeType=pixiu`
- `instanceId`
- `status`
- `reportedAt`

接入建议：

- 成功执行后回调 `Succeeded`
- 执行中可多次回调 `Running`
- 失败时回调 `Failed` 并填可读 `message`
- 如需复用到主流程，可在执行平台任务完成后直接调用 `pixiu_report_edge_task_receipt`
# YA_AN_IOT

## 

` docker run -p 3306:3306 --name mysql -e MYSQL_ROOT_PASSWORD=xjsjy@2023  -d mysql `


tfp 
tftp -gr YA_AN_IOT 192.168.1.60;./YA_AN_IOT
