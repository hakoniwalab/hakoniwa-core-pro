import asyncio
import mcp.server.stdio
from .hako_mcp_base_server import HakoMcpBaseServer
import mcp.types as types
from pydantic import AnyUrl

base_server_instance = HakoMcpBaseServer(server_name="hakoniwa")
# 2. Get the mcp.Server object from the instance
server = base_server_instance.server

# 3. Define handlers with decorators, calling instance methods
@server.list_tools()
async def list_tools() -> list[types.Tool]:
    return await base_server_instance.list_tools()

@server.call_tool()
async def call_tool(name: str, arguments: dict | None) -> list[types.TextContent]:
    return await base_server_instance.call_tool(name, arguments)

@server.list_resources()
async def list_resources() -> list[types.Resource]:
    return await base_server_instance.list_resources()

@server.read_resource()
async def read_resource(uri: AnyUrl) -> str:
    return await base_server_instance.read_resource(uri)

# 4. Define the main run function
async def main():
    await base_server_instance.initialize_rpc_client()
    async with mcp.server.stdio.stdio_server() as (read_stream, write_stream):
        await server.run(
            read_stream,
            write_stream,
            server.create_initialization_options(),
        )

if __name__ == "__main__":
    asyncio.run(main())
