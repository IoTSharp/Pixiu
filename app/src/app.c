#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include  <curl_utils.h>
#include <parson.h>
#include <parson_ex.h>
#include  <bits_iot_rs485_.h>
#include  "app.h"
#include  <uuid4.h>


char _iot_server[256] = { 0 };
char _accessToken[256] = { 0 };
char _runtimeName[128] = { 0 };
char _instanceId[UUID4_LEN] = { 0 };
char _siteName[128] = { 0 };
char _deploymentRole[128] = { 0 };

char _rs485[256] = { 0 };

#define PIXIU_RUNTIME_TYPE "pixiu-runtime"
#define PIXIU_RUNTIME_NAME "PiXiu"
#define PIXIU_EDGE_CONTRACT_VERSION "edge-v1"
#define EDGE_DEFAULT_HEARTBEAT_INTERVAL_SECONDS 30
#define EDGE_MIN_HEARTBEAT_INTERVAL_SECONDS 5
#define EDGE_MAX_HEARTBEAT_INTERVAL_SECONDS 300
#define EDGE_MIN_RETRY_SECONDS 5
#define EDGE_MAX_RETRY_SECONDS 300
#define EDGE_CAPABILITIES_REFRESH_SECONDS 60
#define SENSOR_FAILURE_THRESHOLD 3
#define PLATFORM_COMPONENT_MAX_LEN 60

time_t _process_start_time = 0;
int _edge_enabled = false;
int _edge_registered = false;
int _edge_capabilities_dirty = true;
int _edge_heartbeat_interval_seconds = EDGE_DEFAULT_HEARTBEAT_INTERVAL_SECONDS;
int _edge_register_backoff_seconds = EDGE_MIN_RETRY_SECONDS;
time_t _edge_next_register_at = 0;
time_t _edge_next_heartbeat_at = 0;
time_t _edge_next_capabilities_at = 0;
time_t _edge_next_capability_refresh_at = 0;
int _telemetry_success_count = 0;
int _telemetry_failure_count = 0;
int _last_sensor_read_code = 0;
int _sensor_read_failure_streak = 0;
char _last_capabilities_snapshot[1024] = { 0 };
char _runtime_host_name[128] = { 0 };
char _runtime_platform[128] = { 0 };
char _runtime_ip_address[64] = { 0 };
char _edge_contract_version[64] = { 0 };
int _door_status = -1;

static void SafeCopy(char* dst, size_t dstlen, const char* src)
{
	if (dst == NULL || dstlen == 0)
	{
		return;
	}
	if (src == NULL)
	{
		dst[0] = '\0';
		return;
	}
	strncpy(dst, src, dstlen - 1);
	dst[dstlen - 1] = '\0';
}

static int StringIsEmpty(const char* value)
{
	return value == NULL || value[0] == '\0';
}

static int ClampInt(int value, int min_value, int max_value)
{
	if (value < min_value)
	{
		return min_value;
	}
	if (value > max_value)
	{
		return max_value;
	}
	return value;
}

static int IsSafePathToken(const char* token)
{
	size_t i;
	if (StringIsEmpty(token))
	{
		return false;
	}
	for (i = 0; token[i] != '\0'; ++i)
	{
		unsigned char ch = (unsigned char)token[i];
		if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~')
		{
			continue;
		}
		return false;
	}
	return true;
}

static void ScheduleRegisterRetry(void)
{
	time_t now = time(NULL);
	if (_edge_register_backoff_seconds < EDGE_MIN_RETRY_SECONDS)
	{
		_edge_register_backoff_seconds = EDGE_MIN_RETRY_SECONDS;
	}
	_edge_next_register_at = now + _edge_register_backoff_seconds;
	if (_edge_register_backoff_seconds < EDGE_MAX_RETRY_SECONDS)
	{
		_edge_register_backoff_seconds *= 2;
		if (_edge_register_backoff_seconds > EDGE_MAX_RETRY_SECONDS)
		{
			_edge_register_backoff_seconds = EDGE_MAX_RETRY_SECONDS;
		}
	}
}

static void ResetRegisterRetry(void)
{
	_edge_register_backoff_seconds = EDGE_MIN_RETRY_SECONDS;
	_edge_next_register_at = time(NULL);
}

static int GetPrimaryIpv4Address(char* ipAddress, size_t ipAddressLen)
{
	struct ifaddrs* ifaddr = NULL;
	struct ifaddrs* ifa = NULL;
	if (ipAddress == NULL || ipAddressLen == 0)
	{
		return -1;
	}
	ipAddress[0] = '\0';
	if (getifaddrs(&ifaddr) != 0)
	{
		return -1;
	}
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
		{
			continue;
		}
		if (inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, ipAddress, ipAddressLen) != NULL)
		{
			if (strncmp(ipAddress, "127.", 4) == 0)
			{
				continue;
			}
			freeifaddrs(ifaddr);
			return 0;
		}
	}
	freeifaddrs(ifaddr);
	return -1;
}

static void RefreshRuntimeIdentity(void)
{
	struct utsname uts;
	char hostName[sizeof(_runtime_host_name)] = { 0 };
	if (gethostname(hostName, sizeof(hostName) - 1) == 0)
	{
		SafeCopy(_runtime_host_name, sizeof(_runtime_host_name), hostName);
	}
	else if (StringIsEmpty(_runtime_host_name))
	{
		SafeCopy(_runtime_host_name, sizeof(_runtime_host_name), PIXIU_RUNTIME_NAME);
	}

	if (uname(&uts) == 0)
	{
		char platform[sizeof(_runtime_platform)] = { 0 };
		snprintf(platform, sizeof(platform), "%.*s/%.*s", PLATFORM_COMPONENT_MAX_LEN, uts.sysname, PLATFORM_COMPONENT_MAX_LEN, uts.machine);
		SafeCopy(_runtime_platform, sizeof(_runtime_platform), platform);
	}
	else if (StringIsEmpty(_runtime_platform))
	{
		SafeCopy(_runtime_platform, sizeof(_runtime_platform), "Linux/unknown");
	}

	if (GetPrimaryIpv4Address(_runtime_ip_address, sizeof(_runtime_ip_address)) != 0)
	{
		_runtime_ip_address[0] = '\0';
	}

	if (StringIsEmpty(_runtimeName))
	{
		if (!StringIsEmpty(_runtime_host_name))
		{
			SafeCopy(_runtimeName, sizeof(_runtimeName), _runtime_host_name);
		}
		else
		{
			SafeCopy(_runtimeName, sizeof(_runtimeName), PIXIU_RUNTIME_NAME);
		}
	}
}

static void AppendStringArray(JSON_Object* root_object, const char* name, const char* const* values, size_t count)
{
	JSON_Value* array_value = json_value_init_array();
	JSON_Array* array = json_value_get_array(array_value);
	size_t i;
	for (i = 0; i < count; ++i)
	{
		json_array_append_string(array, values[i]);
	}
	json_object_set_value(root_object, name, array_value);
}

static JSON_Value* BuildRegistrationPayload(void)
{
	JSON_Value* root_value = json_value_init_object();
	JSON_Object* root_object = json_value_get_object(root_value);
	JSON_Value* metadata_value = json_value_init_object();
	JSON_Object* metadata_object = json_value_get_object(metadata_value);
	RefreshRuntimeIdentity();
	json_object_set_string(root_object, "runtimeType", PIXIU_RUNTIME_TYPE);
	json_object_set_string(root_object, "runtimeName", _runtimeName);
	json_object_set_string(root_object, "version", "");
	json_object_set_string(root_object, "instanceId", _instanceId);
	json_object_set_string(root_object, "platform", _runtime_platform);
	json_object_set_string(root_object, "hostName", _runtime_host_name);
	json_object_set_string(root_object, "ipAddress", _runtime_ip_address);
	if (!StringIsEmpty(_siteName))
	{
		json_object_set_string(metadata_object, "site", _siteName);
	}
	if (!StringIsEmpty(_deploymentRole))
	{
		json_object_set_string(metadata_object, "deploymentRole", _deploymentRole);
	}
	json_object_set_string(metadata_object, "controlPlaneContract", PIXIU_EDGE_CONTRACT_VERSION);
	json_object_set_value(root_object, "metadata", metadata_value);
	return root_value;
}

static JSON_Value* BuildCapabilitiesPayload(void)
{
	static const char* protocols[] = { "http", "rs485" };
	static const char* features[] = { "telemetry-upload", "time-sync", "gpio-monitoring", "edge-summary" };
	static const char* tasks[] = { "telemetry-upload", "gpio-report", "time-sync" };
	JSON_Value* root_value = json_value_init_object();
	JSON_Object* root_object = json_value_get_object(root_value);
	JSON_Value* metadata_value = json_value_init_object();
	JSON_Object* metadata_object = json_value_get_object(metadata_value);
	JSON_Value* diagnostics_value = json_value_init_object();
	JSON_Object* diagnostics_object = json_value_get_object(diagnostics_value);
	AppendStringArray(root_object, "protocols", protocols, sizeof(protocols) / sizeof(protocols[0]));
	AppendStringArray(root_object, "features", features, sizeof(features) / sizeof(features[0]));
	AppendStringArray(root_object, "tasks", tasks, sizeof(tasks) / sizeof(tasks[0]));
	if (!StringIsEmpty(_siteName))
	{
		json_object_set_string(metadata_object, "site", _siteName);
	}
	if (!StringIsEmpty(_deploymentRole))
	{
		json_object_set_string(metadata_object, "deploymentRole", _deploymentRole);
	}
	json_object_set_string(metadata_object, "runtimeType", PIXIU_RUNTIME_TYPE);
	json_object_set_string(metadata_object, "cacheMode", "none");
	json_object_set_string(diagnostics_object, "rs485", _rs485);
	json_object_set_string(diagnostics_object, "httpServer", "enabled");
	json_object_set_string(diagnostics_object, "controlPlaneContract", PIXIU_EDGE_CONTRACT_VERSION);
	json_object_set_value(metadata_object, "diagnostics", diagnostics_value);
	json_object_set_value(root_object, "metadata", metadata_value);
	return root_value;
}

static JSON_Value* BuildHeartbeatPayload(void)
{
	struct timeval tv;
	time_t uptime = time(NULL) - _process_start_time;
	JSON_Value* root_value = json_value_init_object();
	JSON_Object* root_object = json_value_get_object(root_value);
	JSON_Value* metrics_value = json_value_init_object();
	JSON_Object* metrics_object = json_value_get_object(metrics_value);
	RefreshRuntimeIdentity();
	gettimeofday(&tv, NULL);
	if (uptime < 0)
	{
		uptime = 0;
	}
	json_object_set_string_iso8601(root_object, "timestamp", &tv);
	json_object_set_string(root_object, "status", "Running");
	json_object_set_boolean(root_object, "healthy", _sensor_read_failure_streak < SENSOR_FAILURE_THRESHOLD ? true : false);
	json_object_set_number(root_object, "uptimeSeconds", (int)(uptime > INT_MAX ? INT_MAX : uptime));
	json_object_set_string(root_object, "ipAddress", _runtime_ip_address);
	json_object_set_number(metrics_object, "telemetrySuccessCount", _telemetry_success_count);
	json_object_set_number(metrics_object, "telemetryFailureCount", _telemetry_failure_count);
	json_object_set_number(metrics_object, "lastSensorReadCode", _last_sensor_read_code);
	json_object_set_number(metrics_object, "doorStatus", _door_status);
	json_object_set_number(metrics_object, "heartbeatIntervalSeconds", _edge_heartbeat_interval_seconds);
	json_object_set_value(root_object, "metrics", metrics_value);
	return root_value;
}

static int ExecuteEdgeRequest(const char* path, const char* operation, JSON_Value* payload, JSON_Value** response_root)
{
	char* serialized_string = NULL;
	struct MemoryStruct response = { NULL, 0 };
	long responseCode = -1;
	int requestResult = 0;
	int success = 0;
	serialized_string = json_serialize_to_string(payload);
	if (serialized_string == NULL)
	{
		echo_app("%s序列化请求失败", operation);
		return 0;
	}
	requestResult = RequestURL_Raw(_iot_server, (char*)path, POST, serialized_string, &responseCode, &response, 5);
	if (requestResult == 1 && responseCode == 200 && response.size > 0)
	{
		JSON_Value* root_value = json_parse_string(response.memory);
		if (root_value != NULL && json_value_get_type(root_value) == JSONObject)
		{
			JSON_Object* root_object = json_value_get_object(root_value);
			if (json_object_has_value_of_type(root_object, "code", JSONNumber))
			{
				int code = json_object_get_number(root_object, "code");
				if (code == 200)
				{
					success = 1;
					if (response_root != NULL)
					{
						*response_root = root_value;
					}
					else
					{
						json_value_free(root_value);
					}
				}
				else
				{
					echo_app("%s失败, 业务代码:%d, http:%ld", operation, code, responseCode);
					json_value_free(root_value);
				}
			}
			else
			{
				echo_app("%s响应缺少code字段, http:%ld", operation, responseCode);
				json_value_free(root_value);
			}
		}
		else
		{
			echo_app("%s响应不是有效Json, http:%ld", operation, responseCode);
			if (root_value != NULL)
			{
				json_value_free(root_value);
			}
		}
	}
	else
	{
		echo_app("%s请求失败:%d 响应代码:%ld", operation, requestResult, responseCode);
	}
	json_free_serialized_string(serialized_string);
	FreeMemoryStruct(&response);
	return success;
}

static int RegisterEdgeRuntime(void)
{
	JSON_Value* payload = BuildRegistrationPayload();
	char path[512] = { 0 };
	JSON_Value* response_root = NULL;
	int success = 0;
	if (snprintf(path, sizeof(path), "api/Edge/%s/Register", _accessToken) >= sizeof(path))
	{
		echo_app("边缘运行时注册路径过长");
		json_value_free(payload);
		ScheduleRegisterRetry();
		return 0;
	}
	success = ExecuteEdgeRequest(path, "边缘运行时注册", payload, &response_root);
	json_value_free(payload);
	if (success == 1)
	{
		JSON_Object* root_object = json_value_get_object(response_root);
		if (json_object_dothas_value_of_type(root_object, "data.timeout", JSONNumber))
		{
			int timeout = json_object_dotget_number(root_object, "data.timeout");
			_edge_heartbeat_interval_seconds = ClampInt(timeout > 0 ? timeout / 2 : EDGE_DEFAULT_HEARTBEAT_INTERVAL_SECONDS, EDGE_MIN_HEARTBEAT_INTERVAL_SECONDS, EDGE_MAX_HEARTBEAT_INTERVAL_SECONDS);
		}
		else
		{
			_edge_heartbeat_interval_seconds = EDGE_DEFAULT_HEARTBEAT_INTERVAL_SECONDS;
		}
		if (json_object_dothas_value_of_type(root_object, "data.contractVersion", JSONString))
		{
			SafeCopy(_edge_contract_version, sizeof(_edge_contract_version), json_object_dotget_string(root_object, "data.contractVersion"));
		}
		else
		{
			SafeCopy(_edge_contract_version, sizeof(_edge_contract_version), PIXIU_EDGE_CONTRACT_VERSION);
		}
		_edge_registered = true;
		_edge_capabilities_dirty = true;
		ResetRegisterRetry();
		_edge_next_heartbeat_at = time(NULL) + _edge_heartbeat_interval_seconds;
		_edge_next_capabilities_at = time(NULL);
	}
	else
	{
		_edge_registered = false;
		_edge_capabilities_dirty = true;
		ScheduleRegisterRetry();
	}
	if (response_root != NULL)
	{
		json_value_free(response_root);
	}
	return success;
}

static int ReportEdgeCapabilities(void)
{
	JSON_Value* payload = BuildCapabilitiesPayload();
	char path[512] = { 0 };
	char* snapshot = NULL;
	int success = 0;
	if (snprintf(path, sizeof(path), "api/Edge/%s/Capabilities", _accessToken) >= sizeof(path))
	{
		echo_app("边缘能力上报路径过长");
		_edge_registered = false;
		_edge_capabilities_dirty = true;
		ScheduleRegisterRetry();
		json_value_free(payload);
		return 0;
	}
	snapshot = json_serialize_to_string(payload);
	success = ExecuteEdgeRequest(path, "边缘能力上报", payload, NULL);
	if (success == 1)
	{
		_edge_capabilities_dirty = false;
		if (snapshot != NULL)
		{
			SafeCopy(_last_capabilities_snapshot, sizeof(_last_capabilities_snapshot), snapshot);
		}
		_edge_next_capabilities_at = time(NULL) + EDGE_CAPABILITIES_REFRESH_SECONDS;
	}
	else
	{
		_edge_registered = false;
		_edge_capabilities_dirty = true;
		ScheduleRegisterRetry();
	}
	if (snapshot != NULL)
	{
		json_free_serialized_string(snapshot);
	}
	json_value_free(payload);
	return success;
}

static int SendEdgeHeartbeat(void)
{
	JSON_Value* payload = BuildHeartbeatPayload();
	char path[512] = { 0 };
	int success = 0;
	if (snprintf(path, sizeof(path), "api/Edge/%s/Heartbeat", _accessToken) >= sizeof(path))
	{
		echo_app("边缘心跳上报路径过长");
		json_value_free(payload);
		ScheduleRegisterRetry();
		return 0;
	}
	success = ExecuteEdgeRequest(path, "边缘心跳上报", payload, NULL);
	json_value_free(payload);
	if (success == 1)
	{
		_edge_next_heartbeat_at = time(NULL) + _edge_heartbeat_interval_seconds;
	}
	else
	{
		_edge_registered = false;
		_edge_capabilities_dirty = true;
		ScheduleRegisterRetry();
	}
	return success;
}

static void RefreshCapabilitiesSnapshot(void)
{
	time_t now = time(NULL);
	if (now < _edge_next_capability_refresh_at)
	{
		return;
	}
	JSON_Value* payload = BuildCapabilitiesPayload();
	char* snapshot = json_serialize_to_string(payload);
	if (snapshot != NULL && strcmp(_last_capabilities_snapshot, snapshot) != 0)
	{
		_edge_capabilities_dirty = true;
		_edge_next_capabilities_at = now;
	}
	if (snapshot != NULL)
	{
		json_free_serialized_string(snapshot);
	}
	json_value_free(payload);
	_edge_next_capability_refresh_at = now + EDGE_CAPABILITIES_REFRESH_SECONDS;
}

char * GetRS485(void)
{
	return _rs485;
}
int  App_Init(void)
{
	int result = 0;
	uuid4_init();
	_process_start_time = time(NULL);
	
	if ((access(CFG_JSON, 0) == 0))
	{
		JSON_Value* _value = json_parse_file(CFG_JSON);
		if (_value != NULL)
		{
			JSON_Object* _object = json_value_get_object(_value);
			if (json_object_dothas_value_of_type(_object, "server", JSONString))
			{
				const char* server = json_object_dotget_string(_object, "server");
				SafeCopy(_iot_server, sizeof(_iot_server), server);
			}
			else 
			{
				SafeCopy(_iot_server, sizeof(_iot_server), "host.iotsharp.net");
			}
			if (json_object_dothas_value_of_type(_object, "rs485", JSONString))
			{
				const char* rs485 = json_object_dotget_string(_object, "rs485");
				SafeCopy(_rs485, sizeof(_rs485), rs485);
			}
			else 
			{
				SafeCopy(_rs485, sizeof(_rs485), "/dev/ttyS1");
			}
			if (json_object_dothas_value_of_type(_object, "accessToken", JSONString))
			{
				const char* accessToken = json_object_dotget_string(_object, "accessToken");
				SafeCopy(_accessToken, sizeof(_accessToken), accessToken);
			}
			else 
			{
				strcpy(_accessToken, "");
			}
			if (json_object_dothas_value_of_type(_object, "runtimeName", JSONString))
			{
				SafeCopy(_runtimeName, sizeof(_runtimeName), json_object_dotget_string(_object, "runtimeName"));
			}
			if (json_object_dothas_value_of_type(_object, "site", JSONString))
			{
				SafeCopy(_siteName, sizeof(_siteName), json_object_dotget_string(_object, "site"));
			}
			if (json_object_dothas_value_of_type(_object, "deploymentRole", JSONString))
			{
				SafeCopy(_deploymentRole, sizeof(_deploymentRole), json_object_dotget_string(_object, "deploymentRole"));
			}
			if (json_object_dothas_value_of_type(_object, "instanceId", JSONString))
			{
				SafeCopy(_instanceId, sizeof(_instanceId), json_object_dotget_string(_object, "instanceId"));
			}
			else
			{
				uuid4_generate(_instanceId);
				json_object_set_string(_object, "instanceId", _instanceId);
				if (json_serialize_to_file_pretty(_value, CFG_JSON) != JSONSuccess)
				{
					echo_app("instanceId写入配置文件失败");
				}
			}
			json_value_free(_value);
		}
		else
		{
			perror("配置文件未能正常加载 ");
			result = -1;
		}
	}
	else
	{
		perror("未找到配置文件 ");
		result = -2;
	}
	RefreshRuntimeIdentity();
	_edge_enabled = !StringIsEmpty(_iot_server) && !StringIsEmpty(_accessToken) && !StringIsEmpty(_instanceId) && IsSafePathToken(_accessToken);
	if (_edge_enabled == false && result == 0)
	{
		echo_app("未启用Edge控制面上报，请检查server/accessToken/instanceId配置");
	}
	return result;
}

void PrintHEX(char * titel, const unsigned char * data, int datlen)
{
	int i;
	fprintf(stderr, "%s LEN:%d DATA:[", titel, datlen);
	for (i = 0; i < datlen; i++)
	{
		fprintf(stderr, "%02X", data[i]);
	}
	fprintf(stderr, "]\n");
}

void UploadEvnData(BITS_IOT* _biotdata)
{
	JSON_Value* root_value = NULL;
	JSON_Object* root_object = NULL;
	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);
	json_object_set_number(root_object, "temp", _biotdata->_temperature);
	json_object_set_number(root_object, "humidity", _biotdata->_humidity);
	json_object_set_number(root_object, "pm25", _biotdata->_PM2_5);
	json_object_set_number(root_object, "pm10", _biotdata->_PM10);
	json_object_set_number(root_object, "pm1", _biotdata->_PM1_0);
	json_object_set_number(root_object, "smog", _biotdata->_smog);
	char* serialized_string = NULL;
	serialized_string = json_serialize_to_string_pretty(root_value);
	struct MemoryStruct  response = { NULL, 0 };
	long responseCode = -1;
	char url_telemetry[512] = { 0 };
	if (IsSafePathToken(_accessToken) == false)
	{
		echo_app("遥测上报accessToken包含不安全字符");
		_telemetry_failure_count++;
		json_free_serialized_string(serialized_string);
		FreeMemoryStruct(&response);
		json_value_free(root_value);
		return;
	}
	if (snprintf(url_telemetry, sizeof(url_telemetry), "api/devices/%s/telemetry", _accessToken) >= sizeof(url_telemetry))
	{
		echo_app("遥测上报路径过长");
		_telemetry_failure_count++;
		json_free_serialized_string(serialized_string);
		FreeMemoryStruct(&response);
		json_value_free(root_value);
		return;
	}
	int  ret_1 = RequestURL_Raw(_iot_server, url_telemetry, POST, serialized_string, &responseCode, &response, 5);
	if (ret_1 == 1  && response.size > 0)
	{
		JSON_Value* root_value_2;
		root_value_2 = json_parse_string(response.memory);
		if (root_value_2 != NULL && json_value_get_type(root_value_2) == JSONObject)
		{
			JSON_Object* jsonobj = json_value_get_object(root_value_2);
			if (json_object_has_value_of_type(jsonobj, "code", JSONNumber) && json_object_has_value_of_type(jsonobj, "msg", JSONString))
			{
				int  _code = json_object_get_number(jsonobj, "code");
				char msg[256] = { 0 };
				strncpy(msg, json_object_get_string(jsonobj, "msg"), 255);
				echo_app("返回代码:%d  消息:%s", _code, msg);
				if (_code == 200)
				{
					_telemetry_success_count++;
				}
				else
				{
					_telemetry_failure_count++;
				}
			}
			else
			{
				echo_app("缺少指定字段:%d 响应代码:%ld", ret_1, responseCode);
				_telemetry_failure_count++;
			}
		}
		else
		{
			echo_app("返回格式不是Json:%d 响应代码:%ld", ret_1, responseCode);
			_telemetry_failure_count++;
		}
		if (root_value_2 != NULL)
		{
			json_value_free(root_value_2);
		}
	}
	else
	{
		echo_app("请求失败:%d 响应代码:%ld", ret_1, responseCode);
		_telemetry_failure_count++;
	}
	json_free_serialized_string(serialized_string);
	FreeMemoryStruct(&response);
	json_value_free(root_value);
}

void SyncDateTime(void)
{
	struct MemoryStruct  response = { NULL, 0 };
	long responseCode = -1;
	int  ret_1 = RequestURL_Raw(_iot_server, "api/home/now", GET, "{}", &responseCode, &response, 5);
	if (ret_1 == 1  && response.size > 0)
	{
		JSON_Value* root_value_2 = json_parse_string(response.memory);
		if (json_value_get_type(root_value_2) == JSONObject)
		{
			JSON_Object* jsonobj = json_value_get_object(root_value_2);
			if (json_object_has_value_of_type(jsonobj, "code", JSONNumber) && json_object_has_value_of_type(jsonobj, "msg", JSONString))
			{
				int  _code = json_object_get_number(jsonobj, "code");
				char msg[256] = { 0 };
				strncpy(msg, json_object_get_string(jsonobj, "msg"), 255);
				echo_app("返回代码:%d  消息:%s", _code, msg);
				if (json_object_dothas_value_of_type(jsonobj, "data.nowString", JSONString))
				{
					struct tm tm;
					const char* _data= json_object_dotget_string(jsonobj, "data.nowString");
					// "2024-07-23 10:25:00"
					// 将日期字符串解析成 struct tm
					if (strptime(_data, "%Y-%m-%d %H:%M:%S", (struct tm*)&tm) == NULL) 
					{
						echo_app("解析日期%s失败。", _data);
					}
					else 
					{
					
						char buf[255] = { 0 };
						// 使用 strftime 打印易读格式的日期
						strftime(buf, sizeof(buf), "%A, %B %d, %Y", (struct tm*) &tm);
						echo_app("已解析的日期: %s\n", buf);
						time_t timep = mktime(&tm);
						struct timeval tv;  
						tv.tv_sec = timep;  
						tv.tv_usec = 0;  
						if (settimeofday(&tv, NULL) < 0)  
						{  
							echo_app("设置系统时间错误！");  
						}
						else 
						{
							echo_app("设置系统时间成功！");  
							
						}
					}
				}
				else 
				{
					echo_app("缺少时间字段:%d 响应代码:%ld", ret_1, responseCode);
					
				}
			}
			else
			{
				echo_app("缺少指定字段:%d 响应代码:%ld", ret_1, responseCode);
			}
		}
		else
		{
			echo_app("返回格式不是Json:%d 响应代码:%ld", ret_1, responseCode);
		}
		if (root_value_2 != NULL)
		{
			json_value_free(root_value_2);
		}
	}
	else
	{
		echo_app("请求失败:%d 响应代码:%ld", ret_1, responseCode);
	}
	FreeMemoryStruct(&response);
}
	



void UploadGpioValue(char* gpioname,int gpiovalue)
{
	JSON_Value* root_value = NULL;
	JSON_Object* root_object = NULL;
	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);
	json_object_dotset_number(root_object, gpioname, gpiovalue);
	char* serialized_string = NULL;
	serialized_string = json_serialize_to_string_pretty(root_value);
	struct MemoryStruct  response = { NULL, 0 };
	long responseCode = -1;
	char url_telemetry[512] = { 0 };
	if (IsSafePathToken(_accessToken) == false)
	{
		echo_app("GPIO上报accessToken包含不安全字符");
		json_free_serialized_string(serialized_string);
		FreeMemoryStruct(&response);
		json_value_free(root_value);
		return;
	}
	if (snprintf(url_telemetry, sizeof(url_telemetry), "api/devices/%s/telemetry", _accessToken) >= sizeof(url_telemetry))
	{
		echo_app("GPIO上报路径过长");
		json_free_serialized_string(serialized_string);
		FreeMemoryStruct(&response);
		json_value_free(root_value);
		return;
	}
	int  ret_1 = RequestURL_Raw(_iot_server, url_telemetry, POST, serialized_string, &responseCode, &response, 5);
	if (ret_1 == 1  && response.size > 0)
	{
		JSON_Value* root_value_2;
		root_value_2 = json_parse_string(response.memory);
		if (root_value_2 != NULL && json_value_get_type(root_value_2) == JSONObject)
		{
			JSON_Object* jsonobj = json_value_get_object(root_value_2);
			if (json_object_has_value_of_type(jsonobj, "code", JSONNumber) && json_object_has_value_of_type(jsonobj, "msg", JSONString))
			{
				int  _code = json_object_get_number(jsonobj, "code");
				char msg[256] = { 0 };
				strncpy(msg, json_object_get_string(jsonobj, "msg"), 255);
				echo_app("返回代码:%d  消息:%s", _code, msg);
				if (json_object_has_value_of_type(jsonobj, "response_code_string", JSONString))
				{
					char response_code_string[256] = { 0 };
					strncpy(response_code_string, json_object_get_string(jsonobj, "msg"), 255);
					echo_app("回复内容:%s", response_code_string);
				}
			}
			else
			{
				echo_app("缺少指定字段:%d 响应代码:%ld", ret_1, responseCode);
			}
		}
		else
		{
			echo_app("返回格式不是Json:%d 响应代码:%ld", ret_1, responseCode);
		}
		if (root_value_2 != NULL)
		{
			json_value_free(root_value_2);
		}
	}
	else
	{
		echo_app("请求失败:%d 响应代码:%ld", ret_1, responseCode);
	}
	json_free_serialized_string(serialized_string);
	FreeMemoryStruct(&response);
	json_value_free(root_value);
}

void App_StartControlPlane(void)
{
	if (_edge_enabled == false)
	{
		return;
	}
	_edge_registered = false;
	_edge_capabilities_dirty = true;
	_edge_next_register_at = time(NULL);
	_edge_next_heartbeat_at = time(NULL);
	_edge_next_capabilities_at = time(NULL);
	_edge_next_capability_refresh_at = 0;
	App_Process();
}

void App_Process(void)
{
	time_t now = time(NULL);
	if (_edge_enabled == false)
	{
		return;
	}
	RefreshCapabilitiesSnapshot();
	if (_edge_registered == false)
	{
		if (now >= _edge_next_register_at)
		{
			if (RegisterEdgeRuntime() == 1)
			{
				ReportEdgeCapabilities();
			}
		}
		return;
	}
	if (_edge_capabilities_dirty == true && now >= _edge_next_capabilities_at)
	{
		if (_edge_registered == true)
		{
			ReportEdgeCapabilities();
		}
	}
	if (_edge_registered == true && now >= _edge_next_heartbeat_at)
	{
		SendEdgeHeartbeat();
	}
}

void App_RecordSensorReadStatus(int result)
{
	_last_sensor_read_code = result;
	if (result == 0)
	{
		_sensor_read_failure_streak = 0;
	}
	else
	{
		_sensor_read_failure_streak++;
	}
}

void CheckDoorStatus(void)
{
	 char buf[2] = { 0 };
	char _gpio_path[100] = { 0 };
	int _gpio_door_id = 1;
	sprintf(_gpio_path, "/sys/class/gpio/GPIO%d/value" , _gpio_door_id);
	if ((access(_gpio_path, 0) == 0))
	{
		FILE *pf = fopen(_gpio_path, "r");
		if (pf == NULL)
		{
			echo_app("无法读取GPIO文件%s", _gpio_path);
		}
		else 
		{
			int readlen = fread(buf, 1, 1, pf);  
			fclose(pf);
			if (readlen > 0)
			{
				int _value = buf[0] - (char)'0';
				if (_door_status != _value)
				{
					_door_status = _value;
					echo_app("门状态切换为:%d", _door_status);
					UploadGpioValue("door",_door_status);
				}
			}
			else 
			{
				echo_app("GPIO%d读取失败:%d", _gpio_door_id, readlen);
			}
		}
	}
	else 
	{
		echo_app("GPIO%d不存在", _gpio_door_id);
	}
}
