from hako_config import HAKO_DATA_MAX_ASSET_NUM, HAKO_FIXED_STRLEN_MAX
import struct

class HakoMetaParser:
    def __init__(self):
        self.shared_memory_info = None
        self.meta_info = None

    def parse(self, data: bytearray):
        """
        Parse raw SharedMemoryMetaData + MasterData byte content to extract meta information.
        """
        try:
            if len(data) < 64:
                print("Insufficient data length for HakoMasterDataType header")
                return False

            offset = 0
            shm_id, sem_id, data_size = struct.unpack_from("<iiI", data, offset)
            offset += 12
            self.shared_memory_info = {
                "shm_id": shm_id,
                "sem_id": sem_id,
                "data_size": data_size
            }

            master_pid, = struct.unpack_from("<i", data, offset)
            offset += 4

            state, = struct.unpack_from("<i", data, offset)
            offset += 4

            max_delay, delta, current = struct.unpack_from("<QQQ", data, offset)
            offset += 24

            asset_num, = struct.unpack_from("<I", data, offset)
            offset += 4

            #padding
            offset += 4

            assets = []
            for _ in range(HAKO_DATA_MAX_ASSET_NUM):
                entry = {}
                entry["id"], = struct.unpack_from("<I", data, offset)
                offset += 4

                entry["name"] = {}
                name_len = struct.unpack_from("<I", data, offset)[0]
                entry["name"]["len"] = name_len
                offset += 4
                name_bytes = struct.unpack_from(f"<{HAKO_FIXED_STRLEN_MAX}s", data, offset)[0]
                entry["name"]["data"] = name_bytes.split(b'\x00', 1)[0].decode('utf-8')
                offset += HAKO_FIXED_STRLEN_MAX + 4

                entry["type"], = struct.unpack_from("<I", data, offset)
                offset += 4

                start_ptr, stop_ptr, reset_ptr = struct.unpack_from("<QQQ", data, offset)
                entry["callback"] = {
                    "start": start_ptr,
                    "stop": stop_ptr,
                    "reset": reset_ptr
                }
                offset += 24
                assets.append(entry)

            assets_ev = []
            for _ in range(HAKO_DATA_MAX_ASSET_NUM):
                ev = {}
                ev["pid"], = struct.unpack_from("<i", data, offset)
                offset += 4

                # padding
                offset += 4

                ev["ctime"], = struct.unpack_from("<Q", data, offset)
                offset += 8

                ev["update_time"], = struct.unpack_from("<Q", data, offset)
                offset += 8

                # padding
                offset += 4

                ev["event"], = struct.unpack_from("<i", data, offset)
                offset += 4

                ev["event_feedback"], = struct.unpack_from("<?", data, offset)
                offset += 4

                ev["semid"], = struct.unpack_from("<i", data, offset)
                offset += 4

                assets_ev.append(ev)

            pdu_meta_offset = offset
            meta_data = self.parse_meta(offset, data)
            if not meta_data:
                print("Failed to parse meta data")
                return False

            self.meta_info = {
                "master_pid": master_pid,
                "state": state,
                "time_usec": {
                    "max_delay": max_delay,
                    "delta": delta,
                    "current": current
                },
                "asset_num": asset_num,
                "assets": assets,
                "assets_ev": assets_ev,
                "pdu_meta_offset": pdu_meta_offset,
                "meta_data": meta_data
            }
            return True
        except Exception as e:
            print(f"Exception in HakoMetaParser.parse: {e}")
            return False
    
    def parse_meta(self, offset, data):
        print(f'Parsing meta data at offset: {offset}')
        meta_data = {}
        meta_data['mode'] = struct.unpack_from('<i', data, offset)[0]

        offset += 4
        meta_data['asset_num'] = struct.unpack_from('<i', data, offset)[0]
        offset += 4

        meta_data['pdu_sync_asset_id'] = struct.unpack_from('<i', data, offset)[0]
        offset += 4

        asset_pdu_check_status = []
        for i in range(HAKO_DATA_MAX_ASSET_NUM):
            check_status = struct.unpack_from('<i', data, offset)[0]
            if check_status == 0:
                asset_pdu_check_status.append(False)
            else:
                asset_pdu_check_status.append(True)
            offset += 1
        meta_data['asset_pdu_check_status'] = asset_pdu_check_status
        meta_data['channel_num'] = struct.unpack_from('<i', data, offset)[0]
        offset += 4
        return meta_data


    def get_meta_info(self):
        return {
            "shared_memory": self.shared_memory_info,
            "master_data": self.meta_info
        }