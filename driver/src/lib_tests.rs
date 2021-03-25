// Contains all the tests for the J2534 library interior  functions from 'passthru_drv.rs'

#[cfg(test)]
mod tests {
    use crate::{PassThruConnect, PassThruOpen, passthru_drv};
    use crate::comm::*;
    use j2534_rust::*;
    use passthru_drv::{passthru_close, passthru_connect, passthru_open, set_channel_filter};

    #[test]
    fn test_channel() {
        let mut dev_idx: u32 = 0;
        assert!(passthru_open(&mut dev_idx) == PassthruError::STATUS_NOERROR);
        
        let mut channel_idx: u32 = 0;
        assert!(passthru_connect(dev_idx, Protocol::CAN as u32, 0, 500_000, &mut channel_idx) == PassthruError::STATUS_NOERROR);

        let mut filter_idx: u32 = 0;
        let mut mask = PASSTHRU_MSG::default();
        let mut ptn = PASSTHRU_MSG::default();
        mask.protocol_id = Protocol::CAN as u32;
        mask.data_size = 4;
        mask.data[0] = 0xFF;
        mask.data[1] = 0xFF;
        mask.data[2] = 0xFF;
        mask.data[3] = 0xFF;

        ptn.protocol_id = Protocol::CAN as u32;
        ptn.data_size = 4;
        ptn.data[0] = 0x00;
        ptn.data[1] = 0x00;
        ptn.data[2] = 0x03;
        ptn.data[3] = 0x08;

        assert!(set_channel_filter(channel_idx, FilterType::PASS_FILTER, &mask, &ptn, std::ptr::null(), &mut filter_idx) == PassthruError::STATUS_NOERROR);
        std::thread::sleep(std::time::Duration::from_millis(1000));
        assert!(passthru_close(dev_idx) == PassthruError::STATUS_NOERROR);
    }
}