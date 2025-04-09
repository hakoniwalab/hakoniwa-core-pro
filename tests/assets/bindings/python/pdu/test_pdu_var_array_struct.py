from data.pdu_info import ROBOT_CHANNELS

def test_pointcloud2(pdu_manager):
    robot_name = "Robot"
    pdu_name = "lidar_points"

    # PDUの取得（読み書き共用）
    pdu = pdu_manager.get_pdu(robot_name, ROBOT_CHANNELS[robot_name][pdu_name])
    pointcloud = pdu.get()

    # 初期確認
    assert 'fields' in pointcloud, "Missing 'fields' in PointCloud2"
    assert isinstance(pointcloud['fields'], list), "'fields' should be a list"
    assert len(pointcloud['fields']) == 0, "Expected empty 'fields' list initially"

    # 書き込みテスト
    point_fields = []
    for i in range(2):
        field = {
            "name": f"field_{i}",
            "offset": i + 100,
            "datatype": 1,
            "count": 1
        }
        point_fields.append(field)

    pointcloud['fields'] = point_fields

    ret = pdu.write()
    assert ret is True, "Failed to write PointCloud2 PDU"

    # 読み出し
    rpdu = pdu_manager.get_pdu(robot_name, ROBOT_CHANNELS[robot_name][pdu_name])
    assert rpdu is not None, "Failed to read PDU"

    read_pointcloud = rpdu.read()
    assert len(read_pointcloud['fields']) == 2, "Unexpected number of fields in read data"
    assert read_pointcloud['fields'][0]['offset'] == 100, "Offset mismatch for field[0]"
    assert read_pointcloud['fields'][1]['offset'] == 101, "Offset mismatch for field[1]"
