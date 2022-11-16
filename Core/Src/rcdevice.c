#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "rcdevice.h"
#include "crc.h"

typedef struct runcamDeviceExpectedResponseLength_s {
    uint8_t command;
    uint8_t reponseLength;
} runcamDeviceExpectedResponseLength_t;

static runcamDeviceExpectedResponseLength_t expectedResponsesLength[] = {
    { RCDEVICE_PROTOCOL_COMMAND_GET_DEVICE_INFO,            5},
    { RCDEVICE_PROTOCOL_COMMAND_5KEY_SIMULATION_PRESS,      2},
    { RCDEVICE_PROTOCOL_COMMAND_5KEY_SIMULATION_RELEASE,    2},
    { RCDEVICE_PROTOCOL_COMMAND_5KEY_CONNECTION,            3},
};

/**
 * @brief Get the expected response length for a fixed response length command
 *
 * @param command (uint8_t): the command being outputted to the runcam
 * @return (uint8_t): the expected response length or 0 if the command is not found
 */
static uint8_t runcamDeviceGetRespLen(uint8_t command)
{
    for (unsigned int i = 0; i < ARRAYLEN(expectedResponsesLength); i++) {
        if (expectedResponsesLength[i].command == command) {
            return expectedResponsesLength[i].reponseLength;
        }
    }
    return 0;
}

/**
 * @brief initializes the members the device structure
 *
 * @param device (runcamDevice_t*): A pointer to the device structure
 * @param uart (UART_HandleTypeDef*): A pointer to the uart the runcam is connected to
 * @param recieveRetries (uint8_t): How many times a command should be done if the crc fails
 */
void init_device(runcamDevice_t* device, UART_HandleTypeDef* uart, uint8_t recieveRetries) {
	device->uart = uart;
	device->recieveRetries = recieveRetries;
	device->currentRecordingTime = get_remaining_recording_time(device);
	get_device_info(device);
}

/**
 * @brief checks if the buffer has the correct crc value
 *
 * @param buf (uint8_t*): the buffer from recieving uart data
 * @param bufLength (uint8_t): the length of the buffer including the crc element
 * @return (bool): true if the crc is the same
 */
bool is_correct_crc(uint8_t* buf, uint8_t bufLength)
{
	uint8_t recieved_crc = calcCrc_dvb_s2(buf, bufLength - 1);

	return recieved_crc == buf[bufLength-1];
}

/**
 * @brief Sends a command over uart to the runcam
 *
 * @param device (runcamDevice_t*): the runcam device to be communicated with
 * @param commandID (uint8_t): the id of the command
 * @param data (uint8_t*): extra fields that needs to be sent with the command
 * @param dataLen (uint8_t): how many extra fields are being sent
 */
void send_command(runcamDevice_t* device, uint8_t commandID, uint8_t *data, uint8_t dataLen)
{
	uint8_t commandLen = 2+dataLen+1;
	uint8_t txBuf[commandLen];

	txBuf[0] = RCDEVICE_PROTOCOL_HEADER;
	txBuf[1] = commandID;
	for (int i = 0; i < dataLen; i ++) {
		txBuf[2+i] = data[i];
	}
	txBuf[commandLen-1] = calcCrc_dvb_s2(txBuf, commandLen - 1);

	HAL_UART_Transmit(device->uart, txBuf, commandLen, 10);
}

/**
 * @brief Receives data of known length from the runcam (for fixed length commands)
 *
 * @param device (runcamDevice_t*): the device that it is receiving data from
 * @param data (runcamDeviceRecieveData_t*): the structure to store the data
 * @param packetLen (uint8_t): the assumed packet length
 *
 * TODO: Potentially use DMA or interrupts to receive data
 */
void recieve_data_fixed_len(runcamDevice_t* device, runcamDeviceRecieveData_t* data, uint8_t packetLen)
{
	uint8_t rxBuf[packetLen];

	HAL_UART_Receive(device->uart, rxBuf, packetLen, 10); // should really be using dma here

	data->correct_crc = is_correct_crc(rxBuf, packetLen);

	if (!data->correct_crc) return;

	data->header = rxBuf[0];
	data->dataLen = packetLen - 2;

	data->data = (uint8_t *)malloc(data->dataLen);
	for(int i = 0; i < data->dataLen; i ++){
		data->data[i] = rxBuf[i + 1];
	}

	data->crc = rxBuf[packetLen - 1];
}

/**
 * @brief Receives data of unknown length from the runcam (for variable length commands)
 *
 * @param device (runcamDevice_t*): the device that it is receiving data from
 * @param data (runcamDeviceRecieveData_t*): the structure to store the data
 *
 * TODO: Potentially use DMA or interrupts to receive data
 */
void recieve_data_unkown_len(runcamDevice_t* device, runcamDeviceRecieveData_t* data)
{
	uint8_t rxBuf;
	HAL_UART_Receive(device->uart, &rxBuf, 1, 10);
	data->header = rxBuf;
	HAL_UART_Receive(device->uart, &rxBuf, 1, 1);
	data->remaining_chunks = rxBuf;
	HAL_UART_Receive(device->uart, &rxBuf, 1, 1);
	data->dataLen = rxBuf;

	data->data = (uint8_t *) malloc(data->dataLen);
	HAL_UART_Receive(device->uart, data->data, data->dataLen, 10);

	HAL_UART_Receive(device->uart, &rxBuf, 1, 1);
	data->crc = rxBuf;

	uint8_t rxBufArr[3 + data->dataLen + 1];
	rxBufArr[0] = data->header;
	rxBufArr[1] = data->remaining_chunks;
	rxBufArr[2] = data->dataLen;

	for(int i = 0; i<data->dataLen; i++){
		rxBufArr[3+i] = data->data[i];
	}

	rxBufArr[3 + data->dataLen] = data->crc;

	data->correct_crc = is_correct_crc(rxBufArr, 3 + data->dataLen + 1);
}

/**
 * @brief gets and sets the device information in the structure provided
 *
 * @param device (runcamDevice_t*): the pointer to the structure of the runcam device
 */
void get_device_info(runcamDevice_t* device)
{
	runcamDeviceRecieveData_t infoData;
	uint8_t responseLength = runcamDeviceGetRespLen(RCDEVICE_PROTOCOL_COMMAND_GET_DEVICE_INFO);

	for (int i = 0; i < device->recieveRetries; i ++){
		send_command(device, RCDEVICE_PROTOCOL_COMMAND_GET_DEVICE_INFO, NULL, 0);

		recieve_data_fixed_len(device, &infoData, responseLength);

		if (infoData.correct_crc) break;
	}

	if (infoData.correct_crc) {
		device->info.protocolVersion = infoData.data[0];
		device->info.features = infoData.data[1] | (infoData.data[2] << 8);
	}
}

/**
 * @brief gets the remaining recording time on the runcam
 *
 * @param device (runcamDevice_t*): The runcam to get the recording time off
 * @return (runcamDeviceRecordingTime_t) structure containing both the string and integer representation
 */
runcamDeviceRecordingTime_t get_remaining_recording_time(runcamDevice_t* device)
{
	runcamDeviceRecieveData_t recTimeData;

	uint8_t data[] = {0x04, 0x00};
	for (int i = 0; i < device->recieveRetries; i ++){
		send_command(device, RCDEVICE_PROTOCOL_COMMAND_READ_SETTING_DETAIL, data, ARRAYLEN(data));

		recieve_data_unkown_len(device, &recTimeData);

		if (recTimeData.correct_crc) break;
	}

	runcamDeviceRecordingTime_t remaining_time;

	if (recTimeData.correct_crc){
		remaining_time.time_str = (char*)(++recTimeData.data);
		remaining_time.strLen = recTimeData.dataLen-1;
		remaining_time.time_int = atoi(remaining_time.time_str);
		return remaining_time;
	}
	return remaining_time;
}
