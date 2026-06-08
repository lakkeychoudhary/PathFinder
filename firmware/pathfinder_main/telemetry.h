#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <WebServer.h>
#include "imu.h"
#include "navigation.h"

extern WebServer server;
extern float targetX, targetY;
extern float currentX, currentY;
extern float targetHeading;
extern bool isNavigating;
extern int baseSpeed;
extern float currentYaw;
extern long lastSensorFront, lastSensorLeft, lastSensorRight;
extern const char* currentStateStr;

void handleRoot() {
  String html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>PathFinder Telemetry Control</title>
  <style>
    :root {
      --bg-color: #0f0f15;
      --panel-bg: rgba(25, 25, 35, 0.75);
      --border-color: rgba(255, 255, 255, 0.1);
      --text-color: #e0e0eb;
      --accent-color: #ff5e57;
      --accent-hover: #e04b45;
      --green: #2ecc71;
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: var(--bg-color);
      color: var(--text-color);
      margin: 0;
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .container {
      max-width: 900px;
      width: 100%;
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 20px;
    }
    @media (max-width: 768px) {
      .container { grid-template-columns: 1fr; }
    }
    .panel {
      background: var(--panel-bg);
      border: 1px solid var(--border-color);
      border-radius: 12px;
      padding: 20px;
      backdrop-filter: blur(10px);
      box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.3);
    }
    h1 {
      grid-column: 1 / -1;
      font-size: 24px;
      margin-bottom: 5px;
      text-align: center;
      letter-spacing: 1px;
    }
    h2 {
      font-size: 18px;
      margin-top: 0;
      border-bottom: 1px solid var(--border-color);
      padding-bottom: 10px;
      color: var(--accent-color);
    }
    .telemetry-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
    .data-card {
      background: rgba(255, 255, 255, 0.03);
      padding: 10px;
      border-radius: 8px;
      border: 1px solid rgba(255, 255, 255, 0.02);
    }
    .label {
      font-size: 11px;
      text-transform: uppercase;
      color: rgba(255, 255, 255, 0.4);
    }
    .value {
      font-size: 18px;
      font-weight: 600;
      margin-top: 4px;
    }
    .btn {
      width: 100%;
      padding: 12px;
      font-size: 14px;
      font-weight: 600;
      background: var(--accent-color);
      border: none;
      border-radius: 6px;
      color: white;
      cursor: pointer;
      transition: background 0.2s;
    }
    .btn:hover { background: var(--accent-hover); }
    input {
      width: calc(100% - 20px);
      padding: 10px;
      margin-bottom: 10px;
      background: rgba(0,0,0,0.2);
      border: 1px solid var(--border-color);
      color: white;
      border-radius: 6px;
    }
    canvas {
      width: 100%;
      height: 250px;
      background: rgba(0,0,0,0.3);
      border-radius: 8px;
      border: 1px solid var(--border-color);
    }
    .controls {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
      margin-top: 15px;
    }
  </style>
</head>
<body>
  <h1>PATHFINDER TESLA-MINI</h1>
  <div style="text-align: center; margin-bottom: 20px; color: rgba(255,255,255,0.5);">System Control & Telemetry Panel</div>
  
  <div class="container">
    <!-- Panel 1: Telemetry -->
    <div class="panel">
      <h2>Live Telemetry</h2>
      <div class="telemetry-grid">
        <div class="data-card">
          <div class="label">System State</div>
          <div class="value" id="state">-</div>
        </div>
        <div class="data-card">
          <div class="label">Heading (Yaw)</div>
          <div class="value" id="yaw">0.0&deg;</div>
        </div>
        <div class="data-card">
          <div class="label">Target Heading</div>
          <div class="value" id="t-yaw">0.0&deg;</div>
        </div>
        <div class="data-card">
          <div class="label">Current Position</div>
          <div class="value" id="pos">X: 0.0, Y: 0.0</div>
        </div>
        <div class="data-card">
          <div class="label">Front Distance</div>
          <div class="value" id="dist-f">0 cm</div>
        </div>
        <div class="data-card">
          <div class="label">Left / Right</div>
          <div class="value" id="dist-lr">L: 0, R: 0</div>
        </div>
      </div>
      
      <h2 style="margin-top: 20px;">Manual Control</h2>
      <div class="controls">
        <div></div>
        <button class="btn" onclick="sendCtrl('forward')">&#9650;</button>
        <div></div>
        <button class="btn" onclick="sendCtrl('left')">&#9664;</button>
        <button class="btn" style="background:#444;" onclick="sendCtrl('stop')">&#9632;</button>
        <button class="btn" onclick="sendCtrl('right')">&#9654;</button>
        <div></div>
        <button class="btn" onclick="sendCtrl('backward')">&#9660;</button>
        <div></div>
      </div>
    </div>

    <!-- Panel 2: Navigation Map -->
    <div class="panel" style="display: flex; flex-direction: column; justify-content: space-between;">
      <div>
        <h2>Target Navigation</h2>
        <form id="targetForm" onsubmit="setTarget(event)">
          <input type="number" step="0.1" id="tx" placeholder="Target X" required>
          <input type="number" step="0.1" id="ty" placeholder="Target Y" required>
          <button type="submit" class="btn">Execute Driving Route</button>
        </form>
      </div>
      <div>
        <h2 style="margin-top: 20px;">Position Map</h2>
        <canvas id="mapCanvas" width="400" height="250"></canvas>
      </div>
    </div>
  </div>

  <script>
    const canvas = document.getElementById('mapCanvas');
    const ctx = canvas.getContext('2d');
    
    function drawMap(cx, cy, tx, ty) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      
      // Draw grid
      ctx.strokeStyle = 'rgba(255,255,255,0.05)';
      ctx.lineWidth = 1;
      for (let x = 0; x < canvas.width; x += 20) {
        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, canvas.height); ctx.stroke();
      }
      for (let y = 0; y < canvas.height; y += 20) {
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(canvas.width, y); ctx.stroke();
      }

      // Map center is offset to (200, 125)
      const zeroX = canvas.width / 2;
      const zeroY = canvas.height / 2;
      const scale = 25; // pixels per unit

      // Draw starting target point
      ctx.fillStyle = '#ff5e57';
      ctx.beginPath();
      ctx.arc(zeroX + tx * scale, zeroY - ty * scale, 6, 0, 2 * Math.PI);
      ctx.fill();

      // Draw vehicle current position
      ctx.fillStyle = '#2ecc71';
      ctx.beginPath();
      ctx.arc(zeroX + cx * scale, zeroY - cy * scale, 8, 0, 2 * Math.PI);
      ctx.fill();
    }

    async function updateData() {
      try {
        const res = await fetch('/telemetry');
        const data = await res.json();
        
        document.getElementById('state').innerText = data.state;
        document.getElementById('yaw').innerHTML = data.yaw.toFixed(1) + '&deg;';
        document.getElementById('t-yaw').innerHTML = data.targetYaw.toFixed(1) + '&deg;';
        document.getElementById('pos').innerText = 'X: ' + data.x.toFixed(1) + ', Y: ' + data.y.toFixed(1);
        document.getElementById('dist-f').innerText = data.distF + ' cm';
        document.getElementById('dist-lr').innerText = 'L: ' + data.distL + ' / R: ' + data.distR;
        
        drawMap(data.x, data.y, data.tx, data.ty);
      } catch (e) {}
    }

    async function setTarget(e) {
      e.preventDefault();
      const x = document.getElementById('tx').value;
      const y = document.getElementById('ty').value;
      await fetch('/setTarget', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: `x=${x}&y=${y}`
      });
    }

    async function sendCtrl(dir) {
      await fetch('/manual', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: `dir=${dir}`
      });
    }

    setInterval(updateData, 250);
  </script>
</body>
</html>
  )rawhtml";
  server.send(200, "text/html", html);
}

void handleTelemetry() {
  String json = "{";
  json += "\"state\":\"" + String(currentStateStr) + "\",";
  json += "\"yaw\":" + String(currentYaw) + ",";
  json += "\"targetYaw\":" + String(targetHeading) + ",";
  json += "\"x\":" + String(currentX) + ",";
  json += "\"y\":" + String(currentY) + ",";
  json += "\"tx\":" + String(targetX) + ",";
  json += "\"ty\":" + String(targetY) + ",";
  json += "\"distF\":" + String(lastSensorFront) + ",";
  json += "\"distL\":" + String(lastSensorLeft) + ",";
  json += "\"distR\":" + String(lastSensorRight);
  json += "}";
  server.send(200, "application/json", json);
}

#endif
