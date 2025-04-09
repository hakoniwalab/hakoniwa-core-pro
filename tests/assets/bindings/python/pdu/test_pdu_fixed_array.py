from data.pdu_info import ROBOT_CHANNELS

def test_ev3_pdu_sensor(pdu_manager):
    robotName = "Robot"
    pduName = "ev3_sensor"

    pdu = pdu_manager.get_pdu(robotName, ROBOT_CHANNELS[robotName][pduName])
    sensor = pdu.get()

    # 配列の長さチェック
    assert len(sensor['color_sensors']) == 2, "ERROR: color_sensors length"
    assert len(sensor['touch_sensors']) == 2, "ERROR: touch_sensors length"
    assert len(sensor['motor_angle']) == 3, "ERROR: motor_angle length"

    # 値の書き込み
    sensor['color_sensors'][0]['rgb_r'] = 99
    sensor['color_sensors'][1]['rgb_r'] = 101
    sensor['touch_sensors'][0]['value'] = 9
    sensor['touch_sensors'][1]['value'] = 8
    sensor['motor_angle'] = [1, 2, 3]

    # 書いた内容の確認
    assert sensor['color_sensors'][0]['rgb_r'] == 99, "ERROR: color_sensors[0].rgb_r"
    assert sensor['color_sensors'][1]['rgb_r'] == 101, "ERROR: color_sensors[1].rgb_r"
    assert sensor['touch_sensors'][0]['value'] == 9, "ERROR: touch_sensors[0].value"
    assert sensor['touch_sensors'][1]['value'] == 8, "ERROR: touch_sensors[1].value"
    assert sensor['motor_angle'][0] == 1, "ERROR: motor_angle[0]"
    assert sensor['motor_angle'][1] == 2, "ERROR: motor_angle[1]"
    assert sensor['motor_angle'][2] == 3, "ERROR: motor_angle[2]"

    # 書き込み実行
    ret = pdu.write()
    assert ret is True, "ERROR: hako_asset_pdu_write"

    # 読み戻し
    rpdu = pdu_manager.get_pdu(robotName, ROBOT_CHANNELS[robotName][pduName])
    rsensor = rpdu.read()

    assert rsensor['color_sensors'][0]['rgb_r'] == 99
    assert rsensor['color_sensors'][1]['rgb_r'] == 101
    assert rsensor['touch_sensors'][0]['value'] == 9
    assert rsensor['touch_sensors'][1]['value'] == 8
    assert rsensor['motor_angle'][0] == 1
    assert rsensor['motor_angle'][1] == 2
    assert rsensor['motor_angle'][2] == 3
