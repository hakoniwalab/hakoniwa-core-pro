import asyncio
import mcp.server.stdio
#from .hako_mcp_base_server import HakoMcpBaseServer
from .hako_mcp_drone_server import HakoMcpDroneServer
import mcp.types as types
from pydantic import AnyUrl
import logging
import argparse
import json
import readline
import os

#logging.basicConfig(
#    level=logging.DEBUG,
#    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
#    handlers=[
#        logging.FileHandler("/Users/tmori/project/private/hakoniwa-core-pro/mcp_server.log", mode="w"),
#        logging.StreamHandler()  # ← 残すならコンソールも
#    ]
#)

pdu_config_path = os.getenv("PDU_CONFIG_PATH", "launcher/config/drone_pdu_config.json")
service_config_path = os.getenv("SERVICE_CONFIG_PATH", "launcher/config/drone_service.json")

server_instance = HakoMcpDroneServer(
    pdu_config_path=pdu_config_path,
    service_config_path=service_config_path,
    server_name="hakoniwa_drone"
)
# 2. Get the mcp.Server object from the instance
server = server_instance.server

# 3. Define handlers with decorators, calling instance methods
@server.list_tools()
async def list_tools() -> list[types.Tool]:
    return await server_instance.list_tools()

@server.call_tool()
async def call_tool(name: str, arguments: dict | None) -> list[types.TextContent]:
    return await server_instance.call_tool(name, arguments)

@server.list_resources()
async def list_resources() -> list[types.Resource]:
    return await server_instance.list_resources()

@server.read_resource()
async def read_resource(uri: AnyUrl) -> str:
    return await server_instance.read_resource(uri)

# 4. Define the main run function
async def main():
    await server_instance.initialize_rpc_clients()
    async with mcp.server.stdio.stdio_server() as (read_stream, write_stream):
        await server.run(
            read_stream,
            write_stream,
            server.create_initialization_options(),
        )

async def manual_main():
    await server_instance.initialize_rpc_clients()

    # Setup for tab completion
    tools = await server_instance.list_tools()
    tool_names = [tool.name for tool in tools]
    commands = ['list', 'call', 'exit']
    
    # Create a map of tool names to their arguments for completion
    tools_map = {}
    for tool in tools:
        try:
            if hasattr(tool, 'inputSchema') and 'properties' in tool.inputSchema:
                tools_map[tool.name] = list(tool.inputSchema['properties'].keys())
            else:
                tools_map[tool.name] = []
        except Exception:
            tools_map[tool.name] = []

    def completer(text, state):
        line = readline.get_line_buffer()
        parts = line.lstrip().split()
        
        options = []

        # Case 1: Completing the command itself
        if len(parts) == 0 or (len(parts) == 1 and not line.endswith(' ')):
            options = [cmd for cmd in commands if cmd.startswith(text)]
        
        # Case 2: Completing a tool name for 'call'
        elif parts and parts[0] == 'call' and (len(parts) == 1 or (len(parts) == 2 and not line.endswith(' '))):
            if len(parts) == 1 and line.endswith(' '): # after "call "
                 options = [name for name in tool_names if name.startswith(text)]
            else: # completing second word
                 options = [name for name in tool_names if name.startswith(text)]

        # Case 3: Completing arguments for a tool
        elif parts and parts[0] == 'call' and len(parts) >= 2:
            tool_name = parts[1]
            if tool_name in tools_map:
                # Do not complete if we are typing a value (after '=')
                if not line.endswith(' ') and '=' in parts[-1]:
                    options = []
                else:
                    used_args = {p.split('=')[0] for p in parts[2:]}
                    available_args = [arg for arg in tools_map[tool_name] if arg not in used_args]
                    options = [arg + '=' for arg in available_args if arg.startswith(text)]

        if state < len(options):
            return options[state]
        else:
            return None

    readline.set_completer(completer)
    readline.parse_and_bind("tab: complete")

    print("Manual mode: type 'list' or 'call <tool_name> key1=value1 key2=value2 ...'")
    loop = asyncio.get_event_loop()
    while True:
        try:
            cmd_input = await loop.run_in_executor(None, input, "> ")
            if not cmd_input:
                continue
            parts = cmd_input.split()
            command = parts[0]

            if command == "list":
                # Re-fetch to ensure latest info, though it's likely static
                current_tools = await server_instance.list_tools()
                for tool in current_tools:
                    print(f"- {tool.name}: {tool.description}")
                    # Also print parameters for user convenience
                    if tool.name in tools_map and tools_map[tool.name]:
                        print(f"  Args: {', '.join(tools_map[tool.name])}")

            elif command == "call":
                if len(parts) < 2:
                    print("Usage: call <tool_name> key1=value1 key2=value2 ...")
                    continue
                tool_name = parts[1]
                args = {}
                if len(parts) > 2:
                    for part in parts[2:]:
                        if "=" in part:
                            key, value = part.split("=", 1)
                            # Attempt to convert to number, otherwise keep as string
                            try:
                                if '.' in value:
                                    args[key] = float(value)
                                else:
                                    args[key] = int(value)
                            except ValueError:
                                args[key] = value
                        else:
                            print(f"Warning: Ignoring malformed argument: {part}")
                
                results = await server_instance.call_tool(tool_name, args)
                for result in results:
                    print(result.text)
            elif command == "exit":
                break
            else:
                print(f"Unknown command: {command}")
        except (EOFError, KeyboardInterrupt):
            break
        except Exception as e:
            print(f"An error occurred: {e}")

import sys
import atexit
import os
import socket
_lock_socket = None

def ensure_single_instance():
    global _lock_socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # ポート番号は適当。通信には使わない
        sock.bind(("127.0.0.1", 49251))
    except OSError:
        # stdout は触らないこと！Claude 側が混乱する
        #print("Another instance already running, exiting.", file=sys.stderr)
        sys.exit(0)
    # グローバルに保持して GC されないようにする
    _lock_socket = sock


# main()の最初に追加
def main_entry():
    ensure_single_instance()
    asyncio.run(main())

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--manual", action="store_true", help="Run in manual interactive mode.")
    args = parser.parse_args()

    if args.manual:
        asyncio.run(manual_main())
    else:
        #asyncio.run(main())
        main_entry()
