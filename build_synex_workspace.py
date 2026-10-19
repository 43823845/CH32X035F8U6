"""
build_synex_workspace.py
通过 Synex MCP Server 自动构建 CH32X035 传感器数据面板
"""
import subprocess
import json
import os
import sys
import time

DISCOVERY_PATH = r"C:\Users\Lenovo\AppData\Local\Temp\Synex\ai_bridge.json"
MCP_EXE = r"D:\Synex_v4.0.1\mcp\SynexMcpServer.exe"

def call_mcp_tool(tool_name, arguments={}):
    proc = subprocess.Popen([MCP_EXE], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    # 1. Initialize
    proc.stdin.write(json.dumps({
        'jsonrpc': '2.0',
        'id': 1,
        'method': 'initialize',
        'params': {
            'protocolVersion': '2024-11-05',
            'capabilities': {},
            'clientInfo': {'name': 'antigravity_builder', 'version': '1.0'}
        }
    }) + '\n')
    proc.stdin.flush()
    proc.stdout.readline()

    # 2. Initialized notification
    proc.stdin.write(json.dumps({'jsonrpc': '2.0', 'method': 'notifications/initialized'}) + '\n')
    proc.stdin.flush()

    # 3. Call tool
    proc.stdin.write(json.dumps({
        'jsonrpc': '2.0',
        'id': 2,
        'method': 'tools/call',
        'params': {
            'name': tool_name,
            'arguments': arguments
        }
    }) + '\n')
    proc.stdin.flush()
    res_line = proc.stdout.readline()
    proc.terminate()
    return json.loads(res_line)

def build_workspace():
    print("[MCP] Connecting to running Synex instance...")
    ping_res = call_mcp_tool("app_ping")
    print("[MCP] Ping status:", ping_res)

    panel_desc = {
        "name": "CH32X035 传感器监控",
        "tiles": [
            {
                "type": "Attitude",
                "title": "3D 姿态监控 (Pitch/Roll)",
                "geometry": {"col": 0, "row": 0, "colSpan": 48, "rowSpan": 36, "zIndex": 1},
                "config": {
                    "inputMode": "euler",
                    "pitchChannel": 1,
                    "rollChannel": 2,
                    "yawChannel": -1,
                    "eulerOrder": "ZYX",
                    "eulerRadians": False,
                    "titleText": "3D 姿态模型 (Pitch/Roll)"
                }
            },
            {
                "type": "Gauge",
                "title": "环境光照度 (Lux)",
                "geometry": {"col": 0, "row": 36, "colSpan": 24, "rowSpan": 18, "zIndex": 2},
                "config": {
                    "boundChannelIndex": 0,
                    "minimumValue": 0,
                    "maximumValue": 2000,
                    "unit": "lx",
                    "customTitleText": "环境光照度 (Lux)"
                }
            },
            {
                "type": "Gauge",
                "title": "屏幕背光亮度 (%)",
                "geometry": {"col": 24, "row": 36, "colSpan": 24, "rowSpan": 18, "zIndex": 3},
                "config": {
                    "boundChannelIndex": 6,
                    "minimumValue": 0,
                    "maximumValue": 100,
                    "unit": "%",
                    "customTitleText": "屏幕背光亮度 (%)"
                }
            },
            {
                "type": "Numeric",
                "title": "单击计数 (TAP1)",
                "geometry": {"col": 48, "row": 36, "colSpan": 16, "rowSpan": 9, "zIndex": 4},
                "config": {
                    "boundChannelIndex": 7,
                    "customTitleText": "单击累计"
                }
            },
            {
                "type": "Numeric",
                "title": "双击计数 (TAP2)",
                "geometry": {"col": 64, "row": 36, "colSpan": 16, "rowSpan": 9, "zIndex": 5},
                "config": {
                    "boundChannelIndex": 8,
                    "customTitleText": "双击累计"
                }
            },
            {
                "type": "Numeric",
                "title": "自由落体 (FF)",
                "geometry": {"col": 80, "row": 36, "colSpan": 16, "rowSpan": 9, "zIndex": 6},
                "config": {
                    "boundChannelIndex": 9,
                    "customTitleText": "自由落体"
                }
            },
            {
                "type": "Numeric",
                "title": "姿态 INT1 中断",
                "geometry": {"col": 48, "row": 45, "colSpan": 24, "rowSpan": 9, "zIndex": 7},
                "config": {
                    "boundChannelIndex": 10,
                    "customTitleText": "INT1 中断计数"
                }
            },
            {
                "type": "Numeric",
                "title": "光敏 INT2 中断",
                "geometry": {"col": 72, "row": 45, "colSpan": 24, "rowSpan": 9, "zIndex": 8},
                "config": {
                    "boundChannelIndex": 11,
                    "customTitleText": "INT2 中断计数"
                }
            }
        ]
    }

    res = call_mcp_tool("auto_build_plot_workspace", panel_desc)
    print("[MCP] Build result:", res)
    return res.get("result", {}).get("isError") is not True

def main():
    daemon = "--daemon" in sys.argv
    max_wait = 180 if daemon else 5
    start = time.time()
    print("[MCP Monitor] Waiting for Synex 'AI 助手连接' to be clicked on top toolbar...")

    while time.time() - start < max_wait:
        if os.path.exists(DISCOVERY_PATH):
            print(f"[MCP Monitor] Discovery file found at {DISCOVERY_PATH}!")
            time.sleep(0.5) # Allow socket to bind
            if build_workspace():
                print("[MCP Monitor] Workspace panel successfully created in Synex!")
                return 0
            else:
                print("[MCP Monitor] Build workspace call returned error.")
                return 1
        time.sleep(1)

    print("[MCP Monitor] Timed out waiting for Synex AI bridge.")
    return 2

if __name__ == "__main__":
    sys.exit(main())
