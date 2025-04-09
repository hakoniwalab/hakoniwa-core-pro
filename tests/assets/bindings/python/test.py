#!/usr/bin/python
# -*- coding: utf-8 -*-
from test_runner import TestRunner
from pdu.test_pdu_struct import test_pdu_struct
import hakopy
import hako_pdu
import sys

def test_example(pdu_manager):
    assert 1 + 1 == 2
    assert "hako".upper() == "HAKO"

pdu_manager = None
def my_on_manual_timing_control(context):
    global pdu_manager

    runner = TestRunner(pdu_manager)
    runner.add(test_pdu_struct)
    runner.run()

    return 0

def my_on_initialize(context):
    return 0

def my_on_reset(context):
    return 0

my_callback = {
    'on_initialize': my_on_initialize,
    'on_simulation_step': None,
    'on_manual_timing_control': my_on_manual_timing_control,
    'on_reset': my_on_reset
}
def main():
    global pdu_manager

    asset_name = 'Writer'
    config_path = 'tests/assets/bindings/python/data/test_data.json'
    delta_time_usec = 1000

    pdu_manager = hako_pdu.HakoPduManager('/usr/local/lib/hakoniwa/hako_binary/offset', config_path)

    hakopy.conductor_start(delta_time_usec, delta_time_usec)
    ret = hakopy.asset_register(asset_name, config_path, my_callback, delta_time_usec, hakopy.HAKO_ASSET_MODEL_PLANT)
    if ret == False:
        print(f"ERROR: hako_asset_register() returns {ret}.")
        return 1

    ret = hakopy.start()
    print(f"INFO: hako_asset_start() returns {ret}")

    hakopy.conductor_stop()
    return 0

if __name__ == "__main__":
    sys.exit(main())


