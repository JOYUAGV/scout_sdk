/* 
 * hunter_protocol.h
 * 
 * Created on: Aug 07, 2019 21:30
 * Description: 
 * 
 * Copyright (c) 2019 Ruixiang Du (rdu)
 */ 

#ifndef HUNTER_PROTOCOL_H
#define HUNTER_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

#define HUNTER_CMD_BUF_LEN               32

#define HUNTER_MOTOR_STEER_ID           ((uint8_t)0x01)
#define HUNTER_MOTOR_DRIVE_ID           ((uint8_t)0x02)

/*--------------------- Control/State Constants ------------------------*/

// Motion Control
#define CTRL_MODE_REMOTE                ((uint8_t)0x00)
#define CTRL_MODE_CMD_CAN               ((uint8_t)0x01)
#define CTRL_MODE_CMD_UART              ((uint8_t)0x02)

#define FAULT_CLR_NONE                  ((uint8_t)0x00)
#define FAULT_CLR_BAT_UNDER_VOL         ((uint8_t)0x01)
#define FAULT_CLR_BAT_OVER_VOL          ((uint8_t)0x02)
#define FAULT_CLR_MOTOR1_COMM           ((uint8_t)0x03)
#define FAULT_CLR_MOTOR2_COMM           ((uint8_t)0x04)
#define FAULT_CLR_MOTOR3_COMM           ((uint8_t)0x05)
#define FAULT_CLR_MOTOR4_COMM           ((uint8_t)0x06)
#define FAULT_CLR_MOTOR_DRV_OVERHEAT    ((uint8_t)0x07)
#define FAULT_CLR_MOTOR_OVERCURRENT     ((uint8_t)0x08)

// System Status Feedback
#define BASE_STATE_NORMAL               ((uint8_t)0x00)
#define BASE_STATE_ESTOP                ((uint8_t)0x01)
#define BASE_STATE_EXCEPTION            ((uint8_t)0x02)

#define FAULT_CAN_CHECKSUM_ERROR        ((uint16_t)0x0100)
#define FAULT_MOTOR_DRV_OVERHEAT_W      ((uint16_t)0x0200)
#define FAULT_MOTOR_OVERCURRENT_W       ((uint16_t)0x0400)
#define FAULT_BAT_UNDER_VOL_W           ((uint16_t)0x0800)
#define FAULT_HIGH_BYTE_RESERVED1       ((uint16_t)0x1000)
#define FAULT_HIGH_BYTE_RESERVED2       ((uint16_t)0x2000)
#define FAULT_HIGH_BYTE_RESERVED3       ((uint16_t)0x4000)
#define FAULT_HIGH_BYTE_RESERVED4       ((uint16_t)0x8000)

#define FAULT_BAT_UNDER_VOL_F           ((uint16_t)0x0001)
#define FAULT_BAT_OVER_VOL_F            ((uint16_t)0x0002)
#define FAULT_MOTOR1_COMM_F             ((uint16_t)0x0004)
#define FAULT_MOTOR2_COMM_F             ((uint16_t)0x0008)
#define FAULT_MOTOR3_COMM_F             ((uint16_t)0x0010)
#define FAULT_MOTOR4_COMM_F             ((uint16_t)0x0020)
#define FAULT_MOTOR_DRV_OVERHEAT_F      ((uint16_t)0x0040)
#define FAULT_MOTOR_OVERCURRENT_F       ((uint16_t)0x0080)

/*-------------------- Control/Feedback Messages -----------------------*/

/* No padding in the struct */
// reference: https://stackoverflow.com/questions/3318410/pragma-pack-effect
#pragma pack(push, 1)

// Note: id could be different for UART and CAN protocol

// Motion Control
typedef struct {
    int32_t id;
    const uint8_t len = 8;
    union
    {
        struct
        {
            uint8_t control_mode;
            uint8_t fault_clear_flag;
            int8_t linear_velocity_cmd;
            int8_t angular_velocity_cmd;
            uint8_t reserved0;
            uint8_t reserved1;
            uint8_t count;
            uint8_t checksum;
        } cmd;
        uint8_t raw[8];
    } msg;
} MotionControlMessage;

typedef union {
    int32_t id;
    const uint8_t len = 8;
    union
    {
        struct
        {
            struct
            {
                uint8_t high_byte;
                uint8_t low_byte;
            } linear_velocity;
            struct
            {
                uint8_t high_byte;
                uint8_t low_byte;
            } angular_velocity;
            uint8_t reserved0;
            uint8_t reserved1;
            uint8_t count;
            uint8_t checksum;
        } status;
        uint8_t raw[8];
    } msg;
} MotionStatusMessage;

// System Status Feedback
typedef struct {
    int32_t id;
    const uint8_t len = 8;
    union
    {
        struct
        {
            uint8_t base_state;
            uint8_t control_mode;
            struct
            {
                uint8_t high_byte;
                uint8_t low_byte;
            } battery_voltage;
            struct
            {
                uint8_t high_byte;
                uint8_t low_byte;
            } fault_code;
            uint8_t count;
            uint8_t checksum;
        } status;
        uint8_t raw[8];
    } msg;
} SystemStatusMessage;

typedef enum
{
    HunterStatusNone = 0x00,
    HunterMotionStatusMsg = 0x01,
    HunterSystemStatusMsg = 0x02
} HunterStatusMsgType;

typedef struct 
{
    HunterStatusMsgType updated_msg_type;

    // only one of the following fields is updated, as specified by updated_msg_type
    MotionStatusMessage motion_status_msg;
    SystemStatusMessage system_status_msg;
} HunterStatusMessage;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* HUNTER_PROTOCOL_H */
