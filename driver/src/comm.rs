use channels::ChannelComm;
use logger::{log_debug, log_error, log_warn};
use serialport::*;
use std::{io::{Error, ErrorKind, Read, Write}, sync::{Mutex}};
use std::sync::{Arc, atomic::AtomicBool, atomic::Ordering};
use std::sync::mpsc::{channel, Sender, Receiver};
use std::thread::spawn;
use std::sync::RwLock;
use lazy_static::lazy_static;
use crate::{channels, logger::{self, log_debug_str, log_error_str, log_m2_msg}};
use j2534_rust::{PassthruError, Parsable};
use crate::passthru_drv::set_error_string;
use byteorder::{ByteOrder, WriteBytesExt, LittleEndian};

#[cfg(windows)]
use winreg::{RegKey, RegValue, enums::HKEY_LOCAL_MACHINE};

const M2_CMD_TIMEOUT: std::time::Duration = std::time::Duration::from_secs(2); // Seconds

lazy_static! {
    pub static ref M2: RwLock<Option<MacchinaM2>> = RwLock::new(None);
    static ref MSG_ID: Arc<Mutex<u8>> = Arc::new(Mutex::new(1));
}

fn get_id() -> u8 {
    let mut x = MSG_ID.lock().unwrap();
    *x += 1;
    if *x >= 100 {
        *x = 1
    }
    *x
}

#[derive(Debug, Clone)]
pub enum M2Resp {
    Ok(Vec<u8>),
    Err { status: PassthruError, string: String }
}

pub struct MacchinaM2 {
    is_running: Arc<AtomicBool>,
    tx_send_queue: Sender<CommMsg>,
    rx_recv_queue: Vec<Receiver<CommMsg>>
}

unsafe impl Send for MacchinaM2{}
unsafe impl Sync for MacchinaM2{}

type Result<T> = std::io::Result<T>;

#[cfg(feature = "M2")]
const JSON_PATH : &str = "~/.passthru/macchina_m2.json";
#[cfg(feature = "A0")]
const JSON_PATH: &str = "~/.passthru/macchina_a0.json";

#[cfg(feature = "M2")]
const REG_PATH : &str = "SOFTWARE\\WOW6432Node\\PassThruSupport.04.04\\Macchina-Passthru-M2";
#[cfg(feature = "A0")]
const REG_PATH: &str = "SOFTWARE\\WOW6432Node\\PassThruSupport.04.04\\Macchina-Passthru-A0";

#[cfg(unix)]
fn get_comm_port() -> Option<String> {
    if let Ok(content) = std::fs::read_to_string(shellexpand::tilde(JSON_PATH).to_string()) {
        return match serde_json::from_str::<serde_json::Value>(content.as_str()) {
            Ok(v) => v["COM-PORT"].as_str().map(String::from),
            Err(_) => None
        }
    }
    None
}

#[cfg(windows)]
fn get_comm_port() -> Option<String> {
    if let Ok(reg) = RegKey::predef(HKEY_LOCAL_MACHINE).open_subkey(REG_PATH) {
        logger::log_info_str("Found regkey");
        return match reg.get_value("COM-PORT") {
            Ok(s) => {
                logger::log_info(format!("Com port is {}", s));
                Some(s)
            },
            Err(_) => None
        }
    }
    None
}

pub type PTResult<T> = std::result::Result<T, PassthruError>;
pub fn run_on_m2<T, F: FnOnce(&MacchinaM2) -> PTResult<T>>(op: F) -> PTResult<T> {
    match M2.read() {
        Ok(d) => {
            match d.as_ref() {
                Some(dev) => op(dev),
                None => Err(PassthruError::ERR_DEVICE_NOT_CONNECTED)
            }
        },
        Err(x) => {
            set_error_string(format!("RWLockGuard on M2 failed to be acquired {}", x));
            Err(PassthruError::ERR_FAILED)
        }
    }
}

const MAX_BUFFER_SIZE: usize = 16;

impl MacchinaM2 {
    pub fn open_connection() -> Result<Self> {
        match get_comm_port() {
            Some(s) => MacchinaM2::open_conn(s.as_str()),
            None => Err(Error::new(ErrorKind::NotFound, "Cannot find COM-PORT attribute"))
        }
    }


    fn open_conn(port: &str) -> Result<Self> {
        let mut port = match serialport::new(port, 500000).open() {
            Ok(mut p) => {
                p.set_flow_control(FlowControl::Hardware).expect("Fatal. Could not setup hardware flow control");

                #[cfg(feature = "A0")]
                {
                    p.set_flow_control(FlowControl::None).expect("Fatal. Could not setup hardware flow control");
                    // A0 uses real Serial, but it can handle 2M/s easily.
                    p.set_baud_rate(2000000).expect("Fatal. Could not setup A0 baud rate");                
                }
                p.set_timeout(std::time::Duration::from_millis(10)).expect("Fatal. Could not set Serial timeout");
                p.clear(ClearBuffer::All).expect("Fatal. Could not clear Serial buffers");
                p
            },
            Err(e) => {return Err(std::io::Error::new(std::io::ErrorKind::Other, format!("Error opening port {}", e.to_string())));}
        };

        #[cfg(all(windows, feature = "A0"))]
        {
            // Ok so windows is strange with A0....even with DTR off it seems to reset ESP32
            // So wait for 1 second for reset, then clear any extra data in the buffer
            std::thread::sleep(std::time::Duration::from_millis(2000));
            port.clear(ClearBuffer::All).expect("Could not clear port!");
        }
        // For data going from Caller -> M2
        let (send_tx, send_rx) : (Sender<CommMsg>, Receiver<CommMsg>) = channel();

        // For data going from Caller <- M2
        let mut senders: Vec<Sender<CommMsg>> = Vec::new();
        let mut receivers: Vec<Receiver<CommMsg>> = Vec::new();

        for _ in 0..100 {
            let (recv_tx, recv_rx) : (Sender<CommMsg>, Receiver<CommMsg>) = channel();
            senders.push(recv_tx);
            receivers.push(recv_rx);
        }

        let (chan_tx, chan_rx) : (Sender<CommMsg>, Receiver<CommMsg>) = channel();

        // Set tell the thread to run by default
        let is_running = Arc::new(AtomicBool::new(true));
        let is_running_t = is_running.clone();
        let is_running_tw = is_running.clone();
        let is_running_ts = is_running.clone();
        // Since UNIX has a 4KB Page size, I want to store more data,
        // Use a 16KB Buffer
        let mut port_write = port.try_clone().unwrap();

        // This thread is responsible for writing data to the M2's
        // serial port.
        spawn(move||{
            logger::log_debug_str("M2 serial writer thread starting!");
            while is_running_tw.load(Ordering::Relaxed) {
                // Any messages to write?
                if let Ok(m) = send_rx.recv() {
                    if let Err(e) = port_write.write_all(&m.to_slice()) {
                        log_warn(format!("Could not write TxPayload to M2 {}", e));
                    }
                }
            }
            logger::log_debug_str("M2 serial writer thread exiting");
        });
        
        // This thread is responsible for pushing data to channel queues,
        // This prevents the serial reader thread from being blocked,
        // which could result in data being lost!
        spawn(move || {
            logger::log_debug_str("M2 channel sender thread starting!");
            let mut activity;
            while is_running_ts.load(Ordering::Relaxed) {
                activity = false;
                if let Ok(msg) = chan_rx.try_recv() {
                    activity = true;
                    ChannelComm::receive_channel_data(&msg)
                }
                if !activity {
                    std::thread::sleep(std::time::Duration::from_micros(10));
                }
            }
            logger::log_debug_str("M2 channel sender thread exiting!");
        });

        // This thread is responsible for reading serial data from the M2
        // Since each packet is 4096 bytes, its imperative that this
        // thread does NOT block, or else data will be lost by the OS's
        // serial buffer.
        spawn(move || {

            let mut is_reading = false;
            logger::log_debug_str("M2 serial reader thread starting!");
            let msg = CommMsg::new_with_args(MsgType::StatusMsg, &[0x01]);
            if port.write_all(&msg.to_slice()).is_err() {
                logger::log_error_str("Timeout writing init struct!");
                is_running_t.store(false, Ordering::Relaxed);
                return;
            }

            let mut read_count = 0;
            let mut read_target = 0;
            let mut read_buffer: [u8; COMM_MSG_SIZE * MAX_BUFFER_SIZE] = [0x00; COMM_MSG_SIZE * MAX_BUFFER_SIZE];
            let mut activity: bool;
            let mut _loop_count: u128 = 0;
            while is_running_t.load(Ordering::Relaxed) {
                #[cfg(feature = "M2")]
                {
                    let incoming = port.read(&mut read_buffer[read_count..]).unwrap_or(0);
                    read_count += incoming;
                    activity = incoming > 0;
                    //if incoming > 0 {
                    //    log_debug(format!("READ {} bytes. {} in buffer", incoming, read_count));
                    //}
                    while read_count >= COMM_MSG_SIZE {
                        let msg = CommMsg::from_vec(&read_buffer[0..COMM_MSG_SIZE]);
                        unsafe {
                            std::ptr::copy(&read_buffer[COMM_MSG_SIZE], &mut read_buffer[0], COMM_MSG_SIZE*(MAX_BUFFER_SIZE-1));
                        }
                        read_count -= COMM_MSG_SIZE;
                        match msg.msg_type {
                            MsgType::LogMsg => log_m2_msg(String::from_utf8(msg.args).unwrap()),
                            MsgType::ReceiveChannelData => {
                                if chan_tx.send(msg).is_err() {
                                    log_error_str("Could not write data to channel thread receiver!");
                                }
                            },
                            _ => {
                                if msg.msg_id != 0 && msg.msg_id < 100 {
                                    if let Err(e) = senders[(msg.msg_id-1) as usize].send(msg) {
                                        // Shouldn't happen, log it if it does
                                        log_error(format!("Could not push COMM_MSG to receive queue: {}", e))
                                    }
                                } else {
                                    log_error(format!("Invalid message ID {} - Type: {:?}", msg.msg_id, msg.msg_type))
                                }
                            }
                        }
                    }
                    if !activity {
                        std::thread::sleep(std::time::Duration::from_micros(10));
                    }
                }
                #[cfg(feature = "A0")]
                {
                    let read = port.bytes_to_read().unwrap_or(0);
                    if read >= 2 && !is_reading {
                        let mut tmp: [u8; 2] = [0; 2];
                        port.read_exact(&mut tmp);
                        read_target = u16::from_le_bytes(tmp) as usize;
                        read_count = 0;
                        is_reading = true;
                    } else if is_reading && read > 0 {
                        let max_read = std::cmp::min(read_target - read_count, read as usize);
                        port.read_exact(&mut read_buffer[read_count..read_count+max_read]);
                        read_count += max_read;
                        if read_count == read_target {
                            is_reading = false;
                            // Complete payload!
                            let mut msg = CommMsg::new_with_args(MsgType::from_u8(&read_buffer[1]), &read_buffer[2..read_target as usize]);
                            msg.msg_id = read_buffer[0];
                            match msg.msg_type {
                                MsgType::LogMsg => log_m2_msg(String::from_utf8(msg.args).unwrap()),
                                MsgType::ReceiveChannelData => {
                                    if chan_tx.send(msg).is_err() {
                                        log_error_str("Could not write data to channel thread receiver!");
                                    }
                                },
                                _ => {
                                    if msg.msg_id != 0 && msg.msg_id < 100 {
                                        if let Err(e) = senders[(msg.msg_id-1) as usize].send(msg) {
                                            // Shouldn't happen, log it if it does
                                            log_error(format!("Could not push COMM_MSG to receive queue: {}", e))
                                        }
                                    } else {
                                        log_error(format!("Invalid message ID {} - Type: {:?}", msg.msg_id, msg.msg_type))
                                    }
                                }
                            }


                        }
                    } else {
                        std::thread::sleep(std::time::Duration::from_micros(10));
                    }
                }
            }
            let msg = CommMsg::new_with_args(MsgType::StatusMsg, &[0x00]);
            if let Err(e) = port.write_all(&msg.to_slice()) {
                log_warn(format!("Could not write exit message to M2 {}", e));
            }
            logger::log_debug_str("M2 serial reader thread exiting");
        });
        std::thread::sleep(std::time::Duration::from_millis(50));
        if !is_running.load(Ordering::Relaxed) {
            return Err(std::io::Error::new(std::io::ErrorKind::ConnectionAborted, "Error initializing M2"));
        }

        let m = MacchinaM2 {
            is_running,
            tx_send_queue: send_tx,
            rx_recv_queue: receivers
        };
        Ok(m)
    }

    /// Writes a CommMsg to the M2, and does not retrieve a response
    /// from the M2
    pub fn write_comm_struct(&self, mut s: CommMsg) -> PTResult<()> {
        s.msg_id = 0x00; // Tell M2 it doesn't have to respond to request
        match self.tx_send_queue.send(s) {
            Ok(_) => Ok(()),
            Err(e) => {
                set_error_string(format!("Error sending data to M2 TxChannel {}", e));
                Err(PassthruError::ERR_FAILED)
            }
        }
    }

    /// Writes a commMsg to the M2, and then waits for its response
    /// # Params
    /// * s - CommMsg to write to the M2
    /// * timeout_ms - Max timeout for waiting for the M2's response, in milliseconds
    pub fn write_and_read_ptcmd(&self, s: &mut CommMsg, timeout_ms: u128) -> M2Resp {
        match self.write_and_read(s, timeout_ms) {
            // Error writing or reading data from the M2
            Err(e) => M2Resp::Err { status: e, string: format!("M2 communication failure: {:?}", e) },
            // M2 responded with a message, process it
            Ok(mut resp) => {
                // Process the status of the message, this should be a PassthruError
                let status = match PassthruError::from_raw(resp.args[0] as u32) {
                    Some(x) => x, // Error processed successfully!
                    None => {
                        // M2 responded with an error code not found in J2534 Spec??
                        return M2Resp::Err{ status: PassthruError::ERR_FAILED, string: format!("Unrecognized status {}", resp.args[0]) }
                    }
                };
                resp.args.drain(0..1); // Drain the first byte from args as that was the status ID
                // Match the status returned
                match status {
                    PassthruError::STATUS_NOERROR => { // Operation completed successfully
                        match resp.args.len() {
                            1 => M2Resp::Ok(Vec::new()), // No args in M2's response
                            _ => M2Resp::Ok(resp.args) // Store M2's args
                        }
                    },
                    _ => { // M2 returned an error!
                        // Check if M2 responded with an error string
                        let text = if resp.args.len() > 1 {
                            // Yes, set the error string
                            String::from_utf8(resp.args).unwrap()
                        } else {
                            // No error string
                            format!("")
                        };
                        // Return The formatted error
                        M2Resp::Err { status, string: text }
                    }
                }
            }
        }
    }

    /// Writes a message to the M2 unit, and expects a designated response back from the unit
    pub fn write_and_read(&self, msg: &mut CommMsg, _timeout_ms: u128) -> PTResult<CommMsg> {
        let query_id = get_id(); // Set a unique ID, M2 is now forced to respond
        msg.msg_id = query_id;

        logger::log_debug(format!("Write data: {}", &msg));
        if let Err(e) = self.tx_send_queue.send(msg.clone()) {
            log_error(format!("Error writing comm msg to queue {}", e));
            return Err(PassthruError::ERR_FAILED);
        }

        let start_time = std::time::Instant::now(); // This is just for logging and serves no other purpose
        
        // Wait for our response message to appear within the Rx queue
        if let Ok(msg) = self.rx_recv_queue[query_id as usize -1].recv_timeout(M2_CMD_TIMEOUT) {
            // For debugging, just log how long the CMD took to do a round trip (Req -> M2 -> Resp)
            log_debug(format!("Command took {}us to execute", start_time.elapsed().as_micros()));
            return Ok(msg); // Return our message
        }
        Err(PassthruError::ERR_TIMEOUT) // M2 timeout!
    }

    pub fn stop(&mut self) {
        self.is_running.store(false, Ordering::Relaxed);
    }
}


const COMM_MSG_SIZE: usize = 8192;
const COMM_MSG_ARG_SIZE: usize = COMM_MSG_SIZE - 4;

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
/// MsgTypes definitions
/// These are found in comm.h of the M2's firmware
pub enum MsgType {
    Unknown = 0x00,
    LogMsg = 0x01,
    OpenChannel = 0x02,
    CloseChannel = 0x03,
    SetChannelFilter = 0x04,
    RemoveChannelFilter = 0x05,
    TransmitChannelData = 0x06,
    ReceiveChannelData = 0x07,
    ReadBatt = 0x08,
    IoctlSet = 0x09,
    IoctlGet = 0x10,
    StatusMsg = 0xAA,
    GetFwVersion = 0xAB,
    #[cfg(test)]
    TestMessage = 0xFF
}

impl MsgType {
    fn from_u8(s: &u8) -> MsgType {
        match s {
            0x01 => MsgType::LogMsg,
            0x02 => MsgType::OpenChannel,
            0x03 => MsgType::CloseChannel,
            0x04 => MsgType::SetChannelFilter,
            0x05 => MsgType::RemoveChannelFilter,
            0x06 => MsgType::TransmitChannelData,
            0x07 => MsgType::ReceiveChannelData,
            0x08 => MsgType::ReadBatt,
            0x09 => MsgType::IoctlSet,
            0x10 => MsgType::IoctlGet,
            0xAA => MsgType::StatusMsg,
            0xAB => MsgType::GetFwVersion,
            #[cfg(test)]
            0xFF => MsgType::TestMessage,
            _ => {
                logger::log_warn(format!("Unknown message type {:02X}", s));
                MsgType::Unknown
            }
        }
    }
}
#[derive(Debug, Clone, PartialEq, Eq)]
/// Comm message that is sent and received fro the M2 module
pub struct CommMsg {
    /// Unique ID of the message
    pub msg_id: u8,
    /// Message type
    pub msg_type: MsgType,
    /// Args of the message
    pub args: Vec<u8>,
}

impl std::fmt::Display for CommMsg {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "COMM_MSG: ID: {:02X} Type: {:?}, Args={:02X?}", self.msg_id, self.msg_type, self.args)
    }
}

impl CommMsg {
    pub fn from_vec(buf: &[u8]) -> Self {
        let size = LittleEndian::read_u16(&buf[2..4]) as usize;
        CommMsg {
            msg_id: buf[0],
            msg_type: MsgType::from_u8(&buf[1]),
            args: Vec::from(&buf[4..size+4]),
        }
    }

    pub fn new(msg_type: MsgType) -> Self {
        CommMsg {
            msg_type,
            args: Vec::new(),
            msg_id: 0,
        }
    }

    pub fn new_with_args(msg_type: MsgType, args_array: &[u8]) -> Self {
        let max_copy = std::cmp::min(args_array.len(), COMM_MSG_ARG_SIZE);
        CommMsg {
            msg_type,
            args: Vec::from(&args_array[0..max_copy]),
            msg_id: 0,
        }
    }

    #[allow(dead_code)] // Might need this in future
    pub fn put_args(&mut self, args: &[u8]) {
        if args.len() > COMM_MSG_ARG_SIZE {
            logger::log_warn(format!("Input args is {} larger than payload size, truncating", args.len() - COMM_MSG_ARG_SIZE));
        }
        let max_copy = std::cmp::min(args.len(), COMM_MSG_ARG_SIZE);
        self.args[0..max_copy].copy_from_slice(&args[0..max_copy]);
    }

    #[allow(dead_code)] // Might need this in future
    pub fn set_type(&mut self, msg_type: MsgType) {
        self.msg_type = msg_type;
    }

    pub fn to_slice(&self) -> Vec<u8> {
        let mut params: Vec<u8> = Vec::with_capacity(self.args.len() + 4);
        params.write_u16::<LittleEndian>(self.args.len() as u16+2).unwrap(); // 0,1
        params.push(self.msg_id); // 2
        params.push(self.msg_type as u8); // 3
        params.extend_from_slice(&self.args);
        params
    }
}