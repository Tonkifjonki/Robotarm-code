#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

WebServer server(80);

const char* ssid = "RobotArmController";
const char* password = "123456789";

Servo servoBase;
Servo servoShoulder;
Servo servoElbow;
Servo servoWrist;
Servo servoGrip;
Servo servoExtra;

int basePos = 90;
int shoulderPos = 90;
int elbowPos = 90;
int wristPos = 90;
int gripPos = 70;
int extraPos = 90;

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Robot Arm Control</title>
    <style>
        :root {
            --bg-color: #1a1a1a;
            --text-color: #e0e0e0;
            --joystick-bg: #2a2a2a;
            --joystick-thumb: #666;
            --button-bg: #2d572c;
            --button-bg-release: #572d2c;
            --border-color: #404040;
            --joystick-size-desktop: 200px;
            --joystick-size-mobile: 150px;
            --thumb-size: 30%;
        }

        body {
            margin: 0;
            padding: 10px;
            display: flex;
            flex-direction: column;
            align-items: center;
            background-color: var(--bg-color);
            color: var(--text-color);
            font-family: Arial, sans-serif;
        }

        h2 {
            color: #ffffff;
            margin-bottom: 20px;
        }

        .container {
            display: flex;
            justify-content: center;
            gap: 30px;
            width: 100%;
            max-width: 800px;
            margin: 20px auto;
            position: relative;
            flex-wrap: wrap;
            padding-bottom: 20px;
        }

        .column {
            display: flex;
            flex-direction: column;
            gap: 30px;
            width: 45%;
            align-items: center;
        }

        .joystick-container {
            width: var(--joystick-size-desktop);
            height: var(--joystick-size-desktop);
            background-color: var(--joystick-bg);
            border-radius: 20px;
            position: relative;
            touch-action: none;
        }

        .joystick-thumb {
            position: absolute;
            width: var(--thumb-size);
            height: var(--thumb-size);
            background-color: var(--joystick-thumb);
            border-radius: 50%;
            transform: translate(-50%, -50%);
            box-shadow: 0 2px 6px rgba(0,0,0,0.3);
            transition: transform 0.15s cubic-bezier(0.18, 0.89, 0.32, 1.28);
        }

        .joystick-container:active .joystick-thumb {
            transform: translate(-50%, -50%) scale(0.9);
        }

        .joystick-label {
            position: absolute;
            bottom: -25px;
            width: 100%;
            text-align: center;
            font-size: 0.85em;
            text-shadow: 0 1px 2px rgba(0,0,0,0.3);
        }

        #gripBtn {
            position: relative;
            bottom: auto;
            left: 50%;
            transform: translateX(-50%);
            width: 200px;
            margin: 30px 0;
            height: 60px;
            font-size: 1.2em;
            background-color: var(--button-bg);
            color: white;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            transition: all 0.2s ease;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }

        #gripBtn.release {
            background-color: var(--button-bg-release);
        }

        #gripBtn:active {
            transform: translateX(-50%) scale(0.95);
        }

        @media (max-width: 600px) {
            .container {
                flex-direction: column;
                gap: 20px;
                padding-bottom: 100px;
            }

            .column {
                width: 100%;
                gap: 40px;
            }

            .joystick-container {
                width: var(--joystick-size-mobile);
                height: var(--joystick-size-mobile);
            }

            #gripBtn {
                position: fixed;
                bottom: 20px;
                width: 80%;
                margin: 0;
                height: 50px;
                font-size: 1.1em;
            }

            .joystick-label {
                font-size: 0.8em;
                bottom: -20px;
            }
        }

        @media (max-width: 480px) {
            .joystick-container {
                width: 130px;
                height: 130px;
            }

            .joystick-thumb {
                width: 25%;
                height: 25%;
            }
        }
    </style>
</head>
<body>
    <h2>Robot Arm Control</h2>
    <div class="container">
        <div class="column">
            <div class="joystick-container" id="rotations">
                <div class="joystick-thumb"></div>
                <div class="joystick-label">Rotations (X/Y)</div>
            </div>
            <div class="joystick-container" id="axis1">
                <div class="joystick-thumb"></div>
                <div class="joystick-label">X Axis</div>
            </div>
        </div>
        <div class="column">
            <div class="joystick-container" id="axis2">
                <div class="joystick-thumb"></div>
                <div class="joystick-label">Y Axis</div>
            </div>
            <div class="joystick-container" id="axis3">
                <div class="joystick-thumb"></div>
                <div class="joystick-label">Z Axis</div>
            </div>
        </div>
        <button id="gripBtn">Grip</button>
    </div>

    <script>
        const UPDATE_INTERVAL = 10;

        class Joystick {
            constructor(container, onMove) {
                this.container = container;
                this.thumb = container.querySelector('.joystick-thumb');
                this.onMove = onMove;
                this.isTouching = false;
                this.rect = null;
                this.center = { x: 0, y: 0 };
                this.currentPosition = { x: 0, y: 0 };

                this.init();
            }

            init() {
                this.container.addEventListener('touchstart', (e) => this.handleStart(e));
                this.container.addEventListener('mousedown', (e) => this.handleStart(e));
                document.addEventListener('touchmove', (e) => this.handleMove(e));
                document.addEventListener('mousemove', (e) => this.handleMove(e));
                document.addEventListener('touchend', () => this.handleEnd());
                document.addEventListener('mouseup', () => this.handleEnd());

                this.rect = this.container.getBoundingClientRect();
                this.center = {
                    x: this.rect.width / 2,
                    y: this.rect.height / 2
                };
                this.updateVisualPosition(this.center.x, this.center.y);
            }

            handleStart(e) {
                e.preventDefault();
                this.isTouching = true;
                this.rect = this.container.getBoundingClientRect();
                
                const thumbRect = this.thumb.getBoundingClientRect();
                const startX = thumbRect.left - this.rect.left + thumbRect.width/2;
                const startY = thumbRect.top - this.rect.top + thumbRect.height/2;
                
                this.currentPosition = { x: startX, y: startY };
            }

            handleMove(e) {
                if (!this.isTouching) return;
                e.preventDefault();
                
                const clientX = e.touches?.[0].clientX || e.clientX;
                const clientY = e.touches?.[0].clientY || e.clientY;
                
                let x = Math.max(0, Math.min(clientX - this.rect.left, this.rect.width));
                let y = Math.max(0, Math.min(clientY - this.rect.top, this.rect.height));
                
                this.currentPosition = { x, y };
                this.updateVisualPosition(x, y);
                
                const normalized = {
                    x: (x - this.center.x) / this.center.x,
                    y: (y - this.center.y) / this.center.y
                };
                
                this.onMove(normalized);
            }

            handleEnd() {
                if (!this.isTouching) return;
                this.isTouching = false;
                this.updateVisualPosition(this.center.x, this.center.y);
            }

            updateVisualPosition(x, y) {
                this.thumb.style.left = x + 'px';
                this.thumb.style.top = y + 'px';
            }
        }

        let controlState = {
            rotX: 0, rotY: 0,
            x: 0, y: 0, z: 0,
            grip: 0, lastUpdate: 0
        };

        const joysticks = {
            rotations: new Joystick(document.getElementById('rotations'), pos => {
                controlState.rotX = pos.x;
                controlState.rotY = pos.y;
                sendCombinedData();
            }),
            axis1: new Joystick(document.getElementById('axis1'), pos => {
                controlState.x = pos.x;
                sendCombinedData();
            }),
            axis2: new Joystick(document.getElementById('axis2'), pos => {
                controlState.y = pos.y;
                sendCombinedData();
            }),
            axis3: new Joystick(document.getElementById('axis3'), pos => {
                controlState.z = pos.y;
                sendCombinedData();
            })
        };

        let gripState = false;
        const gripBtn = document.getElementById('gripBtn');

        gripBtn.addEventListener('click', () => {
            gripState = !gripState;
            controlState.grip = gripState ? 1 : 0;
            gripBtn.textContent = gripState ? 'Release' : 'Grip';
            gripBtn.classList.toggle('release');
            gripBtn.style.transform = `translateX(-50%) scale(${gripState ? 0.98 : 1})`;
            sendCombinedData();
        });

        function sendCombinedData() {
            const now = Date.now();
            if (now - controlState.lastUpdate > UPDATE_INTERVAL) {
                const params = new URLSearchParams({
                    rx: controlState.rotX.toFixed(2),
                    ry: controlState.rotY.toFixed(2),
                    x: controlState.x.toFixed(2),
                    y: controlState.y.toFixed(2),
                    z: controlState.z.toFixed(2),
                    g: controlState.grip
                });
                fetch(`/control?${params.toString()}`);
                controlState.lastUpdate = now;
            }
        }
    </script>
</body>
</html>
)=====";

void handleRoot() {
  server.send_P(200, "text/html", webpage);
}

void handleControl() {
  if (server.hasArg("rx")) basePos = mapFloat(server.arg("rx").toFloat(), -1.0, 1.0, 0, 180);
  if (server.hasArg("ry")) shoulderPos = mapFloat(server.arg("ry").toFloat(), -1.0, 1.0, 0, 180);
  if (server.hasArg("x")) elbowPos = mapFloat(server.arg("x").toFloat(), -1.0, 1.0, 0, 180);
  if (server.hasArg("y")) wristPos = mapFloat(server.arg("y").toFloat(), -1.0, 1.0, 0, 180);
  if (server.hasArg("z")) extraPos = mapFloat(server.arg("z").toFloat(), -1.0, 1.0, 0, 180);
  if (server.hasArg("g")) gripPos = server.arg("g").toInt() == 1 ? 180 : 0;

  servoBase.write(basePos);
  servoShoulder.write(shoulderPos);
  servoElbow.write(elbowPos);
  servoWrist.write(wristPos);
  servoGrip.write(gripPos);
  servoExtra.write(extraPos);

  server.send(200, "text/plain", "OK");
}

void setup() {
  WiFi.softAP(ssid, password);
  delay(1000);

  servoBase.attach(13);
  servoShoulder.attach(12);
  servoElbow.attach(14);
  servoWrist.attach(33);
  servoGrip.attach(26);
  servoExtra.attach(25);

  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.begin();
}

void loop() {
  server.handleClient();
}
