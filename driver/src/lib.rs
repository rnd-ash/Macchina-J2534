use std::convert::TryFrom;

use libc::c_char;
use j2534_rust::*;
mod logger;
mod comm;
mod channels;
mod ioctl;
mod passthru_drv;
use logger::log_error_str;
use passthru_drv::*;

#[cfg(test)]
mod lib_tests;

// Dll Load function (Windows only) - Just return true
#[no_mangle]
#[cfg(windows)]
#[allow(non_snake_case)]
pub extern "stdcall" fn DllMain(_module: u32, _reason: u32, _reserved: *mut std::ffi::c_void) -> bool {
    return true
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruOpen(_name: *mut libc::c_void, device_id: *mut u32) -> i32 {
    passthru_open(device_id) as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruClose(pDeviceID: u32) -> i32 {
    passthru_close(pDeviceID) as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruConnect(
    DeviceID: u32,
    ProtocolID: u32,
    Flags: u32,
    BaudRate: u32,
    pChannelID: *mut u32,
) -> i32 {
    passthru_connect(DeviceID, ProtocolID, Flags, BaudRate, pChannelID) as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruDisconnect(ChannelID: u32) -> i32 {
    passthru_disconnect(ChannelID) as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruReadVersion(
    _device_id: u32,
    fw_version_ptr: *mut c_char,
    dll_version_ptr: *mut c_char,
    api_version_ptr: *mut c_char,
) -> i32 {
    passthru_read_version(fw_version_ptr, dll_version_ptr, api_version_ptr) as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruGetLastError(pErrorDescription: *mut c_char) -> i32 {
    passthru_get_last_error(pErrorDescription) as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruReadMsgs(
    ChannelID: u32,
    pMsg: *mut PASSTHRU_MSG,
    pNumMsgs: *mut u32,
    Timeout: u32,
) -> i32 {
    passthru_drv::read_msgs(ChannelID, pMsg, pNumMsgs, Timeout) as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruStartMsgFilter(
    ChannelID: u32,
    FilterType: u32,
    pMaskMsg: *const PASSTHRU_MSG,
    pPatternMsg: *const PASSTHRU_MSG,
    pFlowControlMsg: *const PASSTHRU_MSG,
    pMsgID: *mut u32,
) -> i32 {
    let filter: FilterType = match FilterType::try_from(FilterType) {
        Ok(f) => f,
        Err(_)=> {
            set_error_string(format!("0x{:02X} is not a valid filter type", FilterType));
            return PassthruError::ERR_FAILED as i32;
        } 
    };
    passthru_drv::set_channel_filter(ChannelID, filter, pMaskMsg, pPatternMsg, pFlowControlMsg, pMsgID) as i32
}

#[no_mangle]
#[allow(non_snake_case, unused_variables)]
pub extern "stdcall" fn PassThruStopMsgFilter(ChannelID: u32, MsgID: u32) -> i32 {
    passthru_drv::del_channel_filter(ChannelID, MsgID) as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruWriteMsgs(
    ChannelID: u32,
    pMsg: *const PASSTHRU_MSG,
    pNumMsgs: *mut u32,
    Timeout: u32,
) -> i32 {
    passthru_drv::write_msgs(ChannelID, pMsg, pNumMsgs, Timeout) as i32
}

#[no_mangle]
#[allow(non_snake_case, unused_variables)]
pub extern "stdcall" fn PassThruStartPeriodicMsg(
    ChannelID: u32,
    pMsg: *const PASSTHRU_MSG,
    pMsgID: *const u32,
    TimeInterval: u32,
) -> i32 {
    PassthruError::STATUS_NOERROR as i32
}

#[no_mangle]
#[allow(non_snake_case, unused_variables)]
pub extern "stdcall" fn PassThruStopPeriodicMsg(ChannelID: u32, MsgID: u32) -> i32 {
    PassthruError::STATUS_NOERROR as i32
}

#[no_mangle]
#[allow(non_snake_case)]
pub extern "stdcall" fn PassThruIoctl(
    HandleID: u32,
    IoctlID: u32,
    pInput: *mut libc::c_void,
    pOutput: *mut libc::c_void,
) -> i32 {
    passthru_ioctl(HandleID, IoctlID, pInput, pOutput) as i32
}

#[no_mangle]
#[allow(non_snake_case, unused_variables)]
pub extern "stdcall" fn PassThruSetProgrammingVoltage(
    DeviceID: u32,
    PinNumber: u32,
    Voltage: u32,
) -> i32 {
    // This isn't used as Macchina hardware does not support this
    log_error_str("Programming voltage setting not supported");
    set_error_string("Programming voltage is not supported".to_string());
    PassthruError::ERR_FAILED as i32
}