from data.pdu_info import ROBOT_CHANNELS

def test_hako_camera_data(pdu_manager):
    robotName = "Robot"
    pduName = "hako_camera_data"

    pdu = pdu_manager.get_pdu(robotName, ROBOT_CHANNELS[robotName][pduName])
    camera = pdu.get()

    # 初期値テスト（バッファの先頭）
    images = camera['image']['data']

    # 書き込みテスト
    new_images = bytearray(128)
    new_images[0] = ord('a')
    new_images[127] = ord('b')
    camera['image']['data'] = new_images

    # 書き込んだデータの確認
    test_images = camera['image']['data']
    assert test_images[0] == ord('a'), "ERROR: image[0] write"
    assert test_images[127] == ord('b'), "ERROR: image[127] write"

    ret = pdu.write()
    assert ret == True, "ERROR: hako_asset_pdu_write"

    # 読み出し確認
    rpdu = pdu_manager.get_pdu(robotName, ROBOT_CHANNELS[robotName][pduName])
    assert rpdu is not None, "ERROR: failed to read PDU"

    rcamera = rpdu.read()
    rimages = rcamera['image']['data']
    assert len(rimages) == 128, "ERROR: image length"
    assert rimages[0] == ord('a'), "ERROR: image[0] read"
    assert rimages[127] == ord('b'), "ERROR: image[127] read"
