from .hako_mcp_base_server import HakoMcpBaseServer
import mcp.types as types
import sys
import subprocess
import os

class HakoMcpDroneServer(HakoMcpBaseServer):
    def __init__(self, server_name="hakoniwa_drone"):
        super().__init__(server_name, simulator_name="Drone")

    def drone_command(self, command: str, args: list[str] = []) -> str:
        subprocess.Popen(
            [sys.executable, "libs/drone_operator.py", command] + args,
            cwd="/Users/tmori/project/private/work/hakoniwa-drone-pro/drone_api",
            env=os.environ.copy(),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        return "Command executed"

    async def list_tools(self) -> list[types.Tool]:
        base_tools = await super().list_tools()
        drone_tools = [
            types.Tool(
                name="describe_hakoniwa_drone",
                description="Provides an overview of the Hakoniwa Drone Simulator",
                inputSchema={"type": "object", "properties": {}, "required": []},
            ),
            types.Tool(
                name="provide_hakoniwa_drone_manual",
                description="Provides the Hakoniwa Drone Simulator operation manual (PDF)",
                inputSchema={"type": "object", "properties": {}, "required": []}
            ),
            types.Tool(
                name="drone_command_takeoff",
                description="Executes a takeoff command on the drone simulator",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "height": {
                            "type": "number",
                            "description": "The target height for takeoff (default 1.0m)"
                        }
                    },
                    "required": []
                }
            ),
            types.Tool(
                name="drone_command_move_to_position",
                description="Move drone to specified (x, y, z) position (ROS coordinate system, meters)",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "x": {"type": "number", "description": "Forward direction, meters"},
                        "y": {"type": "number", "description": "Left direction, meters"},
                        "z": {"type": "number", "description": "Upward direction, meters"},
                        "speed": {"type": "number", "description": "Movement speed, meters/second"}
                    },
                    "required": ["x", "y", "z", "speed"]
                }
            ),
            types.Tool(
                name="drone_command_land",
                description="Land the drone",
                inputSchema={"type": "object", "properties": {}, "required": []}
            )
        ]
        return drone_tools + base_tools

    async def call_tool(self, name: str, arguments: dict | None) -> list[types.TextContent]:
        try:
            return await super().call_tool(name, arguments)
        except ValueError:
            if name == "describe_hakoniwa_drone":
                return [
                    types.TextContent(
                        type="text",
                        text=(
                            "Hakoniwa Drone Simulatorは、仮想環境上でドローンの自律飛行や荷物輸送タスクを模擬できる高機能シミュレータです。"
                            "ROS座標系およびUnity座標系の両方に対応し、リアルタイムなカメラ画像取得、Lidarデータ取得、API制御による移動・離着陸・物体把持が可能です。"
                            "訓練用途、研究用途、またはAI開発向けのプロトタイピングに適しています。"
                        )
                    ),
                ]
            elif name == "drone_command_takeoff":
                height = 1.0
                if arguments and "height" in arguments:
                    height = float(arguments["height"])
                return [types.TextContent(type="text", text=self.drone_command("takeoff", [str(height)]))]
            elif name == "drone_command_move_to_position":
                if arguments is None:
                    raise ValueError("Arguments required for moveToPosition")
                x = str(arguments["x"])
                y = str(arguments["y"])
                z = str(arguments["z"])
                speed = str(arguments["speed"])
                return [types.TextContent(type="text", text=self.drone_command("moveToPosition", [x, y, z, speed]))]
            elif name == "drone_command_land":
                return [types.TextContent(type="text", text=self.drone_command("land"))]
            elif name == "provide_hakoniwa_drone_manual":
                return [
                    types.TextContent(
                        type="text",
                        text=(
                            "箱庭ドローンシミュレータの公式マニュアルはこちらです！\n"
                            "👉 [Hakoniwa Drone Manual PDF](https://www.jasa.or.jp/dl/tech/simulator_operation_edition.pdf)"
                        )
                    )
                ]
            else:
                raise ValueError(f"Unknown tool: {name}")
