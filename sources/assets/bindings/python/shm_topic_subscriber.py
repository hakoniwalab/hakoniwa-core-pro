from shm_common import ShmCommon

class ShmSubscriber:
    def __init__(self, shm: ShmCommon, node_name: str, topic_name: str):
        self.shm = shm
        self.node_name = node_name
        self.topic_name = topic_name
        
    def initialize(self, service_config_path: str, callback=None):
        self.callback = callback
        self.shm.pdu_manager.append_pdu_def(service_config_path)
        self.shm.subscribe_topic(self.node_name, self.topic_name)
        return True

    def read(self):
        return self.shm.read_topic(self.node_name, self.topic_name)


    async def spin(self):
        while True:
            msg = self.read()
            if msg and self.callback:
                self.callback(msg)
            await self.shm.sleep()