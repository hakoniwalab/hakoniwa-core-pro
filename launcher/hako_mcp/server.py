import asyncio
import mcp.server.stdio
#from .hako_mcp_base_server import HakoMcpBaseServer
from .hako_mcp_drone_server import HakoMcpDroneServer
import mcp.types as types
from pydantic import AnyUrl
import logging
import argparse
import json

#logging.basicConfig(
#    level=logging.DEBUG,
#    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
#    handlers=[
#        logging.FileHandler("/Users/tmori/project/private/hakoniwa-core-pro/mcp_server.log", mode="w"),
#        logging.StreamHandler()  # ← 残すならコンソールも
#    ]
#)

server_instance = HakoMcpDroneServer(server_name="hakoniwa")
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
    print("Manual mode: type 'list' or 'call <tool_name> key1=value1 key2=value2 ...'")
    loop = asyncio.get_event_loop()
    while True:
        try:
            cmd_input = await loop.run_in_executor(None, input, "> ")
            parts = cmd_input.split()
            command = parts[0]

            if command == "list":
                tools = await server_instance.list_tools()
                for tool in tools:
                    print(f"- {tool.name}: {tool.description}")
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

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--manual", action="store_true", help="Run in manual interactive mode.")
    args = parser.parse_args()

    if args.manual:
        asyncio.run(manual_main())
    else:
        asyncio.run(main())
