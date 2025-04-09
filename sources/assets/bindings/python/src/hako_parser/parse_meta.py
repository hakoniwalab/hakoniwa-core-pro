from hako_config import HAKO_DATA_MAX_ASSET_NUM, HAKO_FIXED_STRLEN_MAX
import struct

class HakoMetaParser:
    HAKO_MASTERDATA_PADDING = 8
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
            offset += HakoMetaParser.HAKO_MASTERDATA_PADDING

            assets = []
            for _ in range(HAKO_DATA_MAX_ASSET_NUM):
                entry = {}
                entry["id"], = struct.unpack_from("<I", data, offset)
                offset += 4

                name_bytes = struct.unpack_from(f"<{HAKO_FIXED_STRLEN_MAX}s", data, offset)[0]
                entry["name"] = name_bytes.split(b'\x00', 1)[0].decode('utf-8')
                offset += HAKO_FIXED_STRLEN_MAX

                entry["type"], = struct.unpack_from("<I", data, offset)
                offset += 4

                entry["callback"], = struct.unpack_from("<I", data, offset)
                offset += 4

                assets.append(entry)

            assets_ev = []
            for _ in range(HAKO_DATA_MAX_ASSET_NUM):
                ev = {}
                ev["pid"], = struct.unpack_from("<i", data, offset)
                offset += 4

                ev["ctime"], = struct.unpack_from("<Q", data, offset)
                offset += 8

                ev["update_time"], = struct.unpack_from("<Q", data, offset)
                offset += 8

                ev["event"], = struct.unpack_from("<i", data, offset)
                offset += 4

                ev["event_feedback"], = struct.unpack_from("<?", data, offset)
                offset += 1

                ev["semid"], = struct.unpack_from("<i", data, offset)
                offset += 4

                assets_ev.append(ev)

            # PDUメタの読み込みは後回し（オフセット記録）
            pdu_meta_offset = offset

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
                "pdu_meta_offset": pdu_meta_offset
            }
            return True
        except Exception as e:
            print(f"Exception in HakoMetaParser.parse: {e}")
            return False

    def get_meta_info(self):
        return {
            "shared_memory": self.shared_memory_info,
            "master_data": self.meta_info
        }