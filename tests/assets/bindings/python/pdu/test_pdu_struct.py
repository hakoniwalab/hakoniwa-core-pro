from data.pdu_info import ROBOT_CHANNELS

def test_pdu_struct(pdu_manager):
    robotName = 'Robot'
    pdu_pos = pdu_manager.get_pdu(robotName, ROBOT_CHANNELS[robotName]['pos'])
    pos = pdu_pos.get()
    pos['linear']['x'] = 1
    pos['linear']['y'] = 2
    pos['linear']['z'] = 3
    pos['angular']['x'] = 4
    pos['angular']['y'] = 5
    pos['angular']['z'] = 6

    ret = pdu_pos.write()
    assert ret == True, "ERROR: hako_asset_pdu_write"

    read_pdu_pos = pdu_manager.get_pdu(robotName, ROBOT_CHANNELS[robotName]['pos'])
    read_pos = read_pdu_pos.read()
    assert read_pos is not None, "ERROR: hako_asset_pdu_read"
    assert read_pos['linear']['x'] == pos['linear']['x'], "ERROR: hako_asset_pdu_read"
    assert read_pos['linear']['y'] == pos['linear']['y'], "ERROR: hako_asset_pdu_read"
    assert read_pos['linear']['z'] == pos['linear']['z'], "ERROR: hako_asset_pdu_read"
    assert read_pos['angular']['x'] == pos['angular']['x'], "ERROR: hako_asset_pdu_read"
    assert read_pos['angular']['y'] == pos['angular']['y'], "ERROR: hako_asset_pdu_read"
    assert read_pos['angular']['z'] == pos['angular']['z'], "ERROR: hako_asset_pdu_read"
