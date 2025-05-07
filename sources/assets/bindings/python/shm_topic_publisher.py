from shm_common import ShmCommon

class ShmPublisher:
    def __init__(self, shm: ShmCommon, node_name: str, topic_name: str):
        self.shm = shm
        self.node_name = node_name
        self.topic_name = topic_name
        self.shm.advertise_topic(node_name, topic_name)

    def initialize(self, service_config_path: str):
        self.shm.pdu_manager.append_pdu_def(service_config_path)
        return True

    def get_empty_message(self):
        return self.shm.get_empty_message(self.node_name, self.topic_name)

    def publish(self, msg):
        return self.shm.write_topic(self.node_name, self.topic_name, msg)
