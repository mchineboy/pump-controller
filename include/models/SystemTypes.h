#pragma once

#include <Arduino.h>

enum class SystemState : uint8_t {
    Booting,
    Idle,
    Priming,
    Calibrating,
    AwaitingCalibrationMeasurement,
    Dispensing,
    Stopping,
    Completed,
    Fault
};

enum class FaultCode : uint8_t {
    None,
    NotCalibrated,
    InvalidVolume,
    VolumeBelowLimit,
    VolumeAboveLimit,
    OperationAlreadyRunning,
    EmergencyStop,
    MotorDriverFault,
    MotorTimeout,
    ValveFault,
    StorageFailure,
    InvalidCalibration,
    NetworkFailure,
    InternalError,
    ReservoirEmpty
};

inline const char* systemStateToString(SystemState state) {
    switch (state) {
        case SystemState::Booting: return "booting";
        case SystemState::Idle: return "idle";
        case SystemState::Priming: return "priming";
        case SystemState::Calibrating: return "calibrating";
        case SystemState::AwaitingCalibrationMeasurement:
            return "awaiting_calibration_measurement";
        case SystemState::Dispensing: return "dispensing";
        case SystemState::Stopping: return "stopping";
        case SystemState::Completed: return "completed";
        case SystemState::Fault: return "fault";
        default: return "unknown";
    }
}

inline const char* faultCodeToString(FaultCode code) {
    switch (code) {
        case FaultCode::None: return nullptr;
        case FaultCode::NotCalibrated: return "not_calibrated";
        case FaultCode::InvalidVolume: return "invalid_volume";
        case FaultCode::VolumeBelowLimit: return "volume_below_limit";
        case FaultCode::VolumeAboveLimit: return "volume_above_limit";
        case FaultCode::OperationAlreadyRunning:
            return "operation_already_running";
        case FaultCode::EmergencyStop: return "emergency_stop";
        case FaultCode::MotorDriverFault: return "motor_driver_fault";
        case FaultCode::MotorTimeout: return "motor_timeout";
        case FaultCode::ValveFault: return "valve_fault";
        case FaultCode::StorageFailure: return "storage_failure";
        case FaultCode::InvalidCalibration: return "invalid_calibration";
        case FaultCode::NetworkFailure: return "network_failure";
        case FaultCode::InternalError: return "internal_error";
        case FaultCode::ReservoirEmpty: return "reservoir_empty";
        default: return "unknown";
    }
}
