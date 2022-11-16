/**
  ******************************************************************************
  * @file    rcdevice.h
  * @author  Zeyang Zhang
  * @brief   RunCam Device Protocol File
  ******************************************************************************
  */

#pragma once

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "utils.h"


#define RCDEVICE_PROTOCOL_HEADER                                    0xCC

#define RCDEVICE_PROTOCOL_MAX_PACKET_SIZE                           64
#define RCDEVICE_PROTOCOL_MAX_DATA_SIZE                             62

// Commands
#define RCDEVICE_PROTOCOL_COMMAND_GET_DEVICE_INFO                   0x00
// camera control
#define RCDEVICE_PROTOCOL_COMMAND_CAMERA_CONTROL                    0x01
// 5 key osd cable simulation
#define RCDEVICE_PROTOCOL_COMMAND_5KEY_SIMULATION_PRESS             0x02
#define RCDEVICE_PROTOCOL_COMMAND_5KEY_SIMULATION_RELEASE           0x03
#define RCDEVICE_PROTOCOL_COMMAND_5KEY_CONNECTION                   0x04
#define RCDEVICE_PROTOCOL_COMMAND_REQUEST_FC_ATTITUDE               0x50
//settings reading and writing
#define RCDEVICE_PROTOCOL_COMMAND_GET_SETTINGS						0x10
#define RCDEVICE_PROTOCOL_COMMAND_READ_SETTING_DETAIL				0x11
#define RCDEVICE_PROTOCOL_COMMAND_WRITE_SETTING						0x13

typedef enum {
    RCDEVICE_PROTOCOL_FEATURE_SIMULATE_POWER_BUTTON    = (1 << 0),
    RCDEVICE_PROTOCOL_FEATURE_SIMULATE_WIFI_BUTTON     = (1 << 1),
    RCDEVICE_PROTOCOL_FEATURE_CHANGE_MODE              = (1 << 2),
    RCDEVICE_PROTOCOL_FEATURE_SIMULATE_5_KEY_OSD_CABLE = (1 << 3),
    RCDEVICE_PROTOCOL_FEATURE_START_RECORDING          = (1 << 6),
    RCDEVICE_PROTOCOL_FEATURE_STOP_RECORDING           = (1 << 7),
    RCDEVICE_PROTOCOL_FEATURE_CMS_MENU                 = (1 << 8),
    RCDEVICE_PROTOCOL_FEATURE_FC_ATTITUDE              = (1 << 9)
} rcdevice_features_e;

// Operation of Camera Button Simulation
typedef enum {
    RCDEVICE_PROTOCOL_CAM_CTRL_SIMULATE_WIFI_BTN        = 0x00,
    RCDEVICE_PROTOCOL_CAM_CTRL_SIMULATE_POWER_BTN       = 0x01,
    RCDEVICE_PROTOCOL_CAM_CTRL_CHANGE_MODE              = 0x02,
    RCDEVICE_PROTOCOL_CAM_CTRL_START_RECORDING          = 0x03,
    RCDEVICE_PROTOCOL_CAM_CTRL_STOP_RECORDING           = 0x04,
    RCDEVICE_PROTOCOL_CAM_CTRL_UNKNOWN_CAMERA_OPERATION = 0xFF
} rcdevice_camera_control_opeation_e;

//	Device Setting IDs
typedef enum {
	RCDEVICE_SID_DISP_CHARSET				= 0x00,
	RCDEVICE_SID_DISP_COLUMNS				= 0x01,
	RCDEVICE_SID_DISP_TV_MODE				= 0x02,
	RCDEVICE_SID_DISP_SDCARD_CAPACITY		= 0x03,
	RCDEVICE_SID_DISP_REMAIN_RECORDING_TIME = 0x04,
	RCDEVICE_SID_DISP_RESOLUTION			= 0x05,
	RCDEVICE_SID_DISP_CAMERA_TIME			= 0x06
} rcdeviceSettingID_e;

typedef enum {
	UINT8			= 0x00,
	INT8			= 0x01,
	UINT16			= 0x02,
	INT16			= 0x03,
	FLOAT			= 0x08,
	TEXT_SELECTION	= 0x09,
	STRING			= 10,
	FOLDER			= 11,
	INFO			= 12
} rcdeviceSettingTypes_e;

typedef enum {
    RCDEVICE_PROTOCOL_RCSPLIT_VERSION = 0x00, // this is used to indicate the
                                              // device that using rcsplit
                                              // firmware version that <= 1.1.0
    RCDEVICE_PROTOCOL_VERSION_1_0 = 0x01,
    RCDEVICE_PROTOCOL_UNKNOWN
} rcdevice_protocol_version_e;

typedef struct runcamDeviceInfo_s {
    rcdevice_protocol_version_e protocolVersion;
    uint16_t features;
} runcamDeviceInfo_t;

typedef struct {
	long int time_int;
	char* time_str;
	uint8_t strLen;
} runcamDeviceRecordingTime_t;

typedef struct{
	runcamDeviceInfo_t info;
	UART_HandleTypeDef* uart;
	uint8_t recieveRetries;
	runcamDeviceRecordingTime_t currentRecordingTime;
	bool isRecording;
} runcamDevice_t;

typedef struct{
	uint8_t header;
	uint8_t* data;
	uint8_t dataLen;
	uint8_t crc;
	uint8_t remaining_chunks;
	bool correct_crc;
} runcamDeviceRecieveData_t;

void get_device_info(runcamDevice_t* device);
void init_device(runcamDevice_t* device, UART_HandleTypeDef* uart, uint8_t recieveRetries);
runcamDeviceRecordingTime_t get_remaining_recording_time(runcamDevice_t* device);
void toggle_recording(runcamDevice_t* device);
bool is_currently_recording(runcamDevice_t* device);
