use j2534_rust::{IoctlParam, PASSTHRU_MSG, Parsable, PassthruError, SBYTE_ARRAY, SConfigList};
use crate::{channels, comm::*, logger::{log_warn, log_warn_str}, passthru_drv::set_error_string};
use crate::logger::{log_error};
use byteorder::{ByteOrder, LittleEndian};


/// Reads the battery voltage into an output pointer, storing the value as mV
/// # Params
/// * output_ptr - Output pointer to store batter voltage into

static mut last_vbatt : u32 = 0;
pub fn read_vbatt(output_ptr: *mut u32) -> PassthruError {
    match run_on_m2(|dev| {
        match dev.write_and_read_ptcmd(&mut CommMsg::new(MsgType::ReadBatt), 250) {
            M2Resp::Ok(args) => {
                if args.len() < 4 { // This should stop a panic from randomly occurring when M2 is under load
                    log_error("Error reading battery voltage - Args size was not correct, returning last known".into());
                    Ok(unsafe { last_vbatt })
                } else {
                    Ok(LittleEndian::read_u32(&args))
                }
            },
            M2Resp::Err{status, string} => {
                log_error(format!("Error reading battery voltage (Status {:?}): {}", status, string));
                Err(status)
            }
        }
    }) {
        Ok(v) => {
            unsafe { last_vbatt = v };
            unsafe { *output_ptr = v };
            PassthruError::STATUS_NOERROR
        },
        Err(x) => x
    }
}

#[allow(unused_variables)]
pub fn read_prog_voltage(output_ptr: *mut u32) -> PassthruError {
    log_warn_str("Read programming voltage unimplemented");
    PassthruError::STATUS_NOERROR
}

pub fn set_config(channel_id: u32, cfg_ptr: &SConfigList) -> PassthruError {
    for i in 0..cfg_ptr.num_of_params as isize {
        match unsafe { cfg_ptr.config_ptr.offset(i).as_ref() } {
            None => return PassthruError::ERR_NULL_PARAMETER,
            Some(param) => {
                if param.parameter >= 0x20 {
                    log_warn(format!("setconfig param name is reserved / tool specific?. Param: {:08X}, value: {:08X}", param.parameter, param.value));
                } else if let Some(pname) = IoctlParam::from_raw(param.parameter) {
                    if let Err(e) = channels::ChannelComm::ioctl_set_cfg(channel_id, pname, param.value) {
                        return e
                    }
                } else {
                    return PassthruError::ERR_NOT_SUPPORTED
                }
            }
        }
    }
    PassthruError::STATUS_NOERROR
}

pub fn get_config(channel_id: u32, cfg_ptr: &SConfigList) -> PassthruError {
    for i in 0..cfg_ptr.num_of_params as isize {
        match unsafe { cfg_ptr.config_ptr.offset(i).as_mut() } {
            None => return PassthruError::ERR_NULL_PARAMETER,
            Some(mut param) => {
                if param.parameter >= 0x20 {
                    log_warn(format!("get config param name is reserved / tool specific?. Param: {:08X}, value: {:08X}", param.parameter, param.value));
                } else if let Some(pname) = IoctlParam::from_raw(param.parameter) {
                    if let Ok(pvalue) = channels::ChannelComm::ioctl_get_cfg(channel_id, pname) {
                        param.value = pvalue;
                    } else {
                        return PassthruError::ERR_FAILED
                    }
                } else {
                    return PassthruError::ERR_NOT_SUPPORTED
                }
            }
        }
    }
    PassthruError::STATUS_NOERROR
}

#[allow(unused_variables)] // TODO
pub fn five_baud_init(channel_id: u32, input: &mut SBYTE_ARRAY, output: &mut SBYTE_ARRAY) -> PassthruError {
    log_warn_str("Five baud init unimplemented");
    PassthruError::STATUS_NOERROR
}

#[allow(unused_variables)] // TODO
pub fn fast_init(channel_id: u32, input: &mut PASSTHRU_MSG, output: &mut PASSTHRU_MSG) -> PassthruError {
    log_warn_str("Five baud init unimplemented");
    PassthruError::STATUS_NOERROR
}

pub fn clear_tx_buffer(channel_id: u32) -> PassthruError {
    channels::ChannelComm::clear_tx_buffer(channel_id)
}

pub fn clear_rx_buffer(channel_id: u32) -> PassthruError {
    channels::ChannelComm::clear_rx_buffer(channel_id)
}

#[allow(unused_variables)] // TODO
pub fn clear_periodic_msgs(channel_id: u32) -> PassthruError {
    log_warn_str("Clear periodic messages unimplemented");
    PassthruError::STATUS_NOERROR
}

#[allow(unused_variables)] // TODO
pub fn clear_msg_filters(channel_id: u32) -> PassthruError {
    log_warn_str("Clear message filters unimplemented");
    PassthruError::STATUS_NOERROR
}

#[allow(unused_variables)] // TODO
pub fn clear_funct_msg_lookup_table(channel_id: u32) -> PassthruError {
    log_warn_str("Clear message lookup table unimplemented");
    PassthruError::STATUS_NOERROR
}

#[allow(unused_variables)] // TODO
pub fn add_to_funct_msg_lookup_table(channel_id: u32, input: &mut SBYTE_ARRAY) -> PassthruError {
    log_warn_str("Add to function message lookup table unimplemented");
    PassthruError::STATUS_NOERROR
}

#[allow(unused_variables)] // TODO
pub fn delete_from_funct_msg_lookup_table(channel_id: u32, input: &mut SBYTE_ARRAY) -> PassthruError {
    log_warn_str("Delete from function message lookup table unimplemented");
    PassthruError::STATUS_NOERROR
}