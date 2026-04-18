#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "curl_utils.h"

/* Minimal PiXiu edge task receipt callback example using the existing curl client. */

int pixiu_report_edge_task_receipt(
    const char* base_url,
    const char* device_id,
    const char* runtime_type,
    const char* instance_id,
    const char* task_id)
{
    char payload[2048];
    char request_addr[512];
    struct MemoryStruct response = { 0 };
    long response_code = 0;
    int ok = 0;

    if (base_url == NULL || device_id == NULL || runtime_type == NULL || instance_id == NULL || task_id == NULL)
    {
        return 0;
    }

    snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"contractVersion\":\"edge-task-v1\","
        "\"taskId\":\"%s\","
        "\"targetType\":\"PixiuRuntime\","
        "\"targetKey\":\"%s:%s:%s\","
        "\"runtimeType\":\"%s\","
        "\"instanceId\":\"%s\","
        "\"status\":\"Succeeded\","
        "\"message\":\"PiXiu example finished successfully.\","
        "\"reportedAt\":\"2026-04-19T00:00:00Z\","
        "\"progress\":100"
        "}",
        task_id,
        device_id,
        runtime_type,
        instance_id,
        runtime_type,
        instance_id);

    memset(request_addr, 0, sizeof(request_addr));
    snprintf(request_addr, sizeof(request_addr), "%s", base_url);

    ok = RequestURL_Raw(request_addr, "api/EdgeTask/Receipt", "POST", payload, &response_code, &response, 10);
    if (!ok)
    {
        if (response.memory != NULL)
        {
            echo_app("edge task receipt request failed, response=%s", response.memory);
        }
        FreeMemoryStruct(&response);
        return 0;
    }

    if (response.memory != NULL)
    {
        echo_app("edge task receipt response: %s", response.memory);
    }

    FreeMemoryStruct(&response);
    return response_code == 200 ? 1 : 0;
}