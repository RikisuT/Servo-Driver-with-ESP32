const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <title>Servo Driver with ESP32</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
        html {
        font-family: Arial;
        display: inline-block;
        background: #000000;
        color: #efefef;
        text-align: center;
    }

    h2 {
        font-size: 3.0rem;
    }

    p {
        font-size: 1.0rem;
    }

    body {
        max-width: 600px;
        margin: 0px auto;
        padding-bottom: 25px;
    }

    button {
        display: inline-block;
        margin: 5px;
        padding: 10px 10px;
        border: 0;
        line-height: 21px;
        cursor: pointer;
        color: #fff;
        background: #4247b7;
        border-radius: 5px;
        font-size: 21px;
        outline: 0;
        width: 100px

        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;

        user-select: none;
    }

    button:hover {
        background: #ff494d
    }

    button:active {
        background: #f21c21
    }

    </style>
</head>

<body>
    <h3>SERVO DRIVER with ESP32</h3>
    <p>
    <span id="IDValue">Click this button to start searching servos.</span>
    <p>
    <label align="center"><button class="button" onclick="send_command(cmd_type.scan_servos, 0);">Start Searching</button></label>
    <p>
    <span id="STSValue">Single servo information.</span>
    <p>
        <label align="center"><button class="button" onclick="send_command(cmd_type.select_servo, select_dir.next);">ID Select+</button></label>
        <label align="center"><button class="button" onclick="send_command(cmd_type.select_servo, select_dir.prev);">ID Select-</button></label>
    <p>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.middle);">Middle</button></label>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.stop);">Stop</button></label>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.release);">Release</button></label>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.torque);">Torque</button></label>
    <p>
        <label align="center"><button class="button" onmousedown="send_command(cmd_type.servo_control, servo_cmd.position_inc);" ontouchstart="send_command(cmd_type.servo_control, servo_cmd.position_inc);" onmouseup="send_command(cmd_type.servo_control, servo_cmd.stop);" ontouchend="send_command(cmd_type.servo_control, servo_cmd.stop);">Position+</button></label>
        <label align="center"><button class="button" onmousedown="send_command(cmd_type.servo_control, servo_cmd.position_dec);" ontouchstart="send_command(cmd_type.servo_control, servo_cmd.position_dec);" onmouseup="send_command(cmd_type.servo_control, servo_cmd.stop);" ontouchend="send_command(cmd_type.servo_control, servo_cmd.stop);">Position-</button></label>
    <p>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.speed_inc);">Speed+</button></label>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.speed_dec);">Speed-</button></label>
    <p>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.id_to_set_inc);">ID to Set+</button></label>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.id_to_set_dec);">ID to Set-</button></label>
    <p>
        <label align="center"><button class="button" onclick="set_middle();">Set Middle Position</button></label>
        <label align="center"><button class="button" onclick="set_new_id();">Set New ID</button></label>
    <p>
        <label align="center"><button class="button" onclick="set_servo_mode();">Set Servo Mode</button></label>
        <label align="center"><button class="button" onclick="set_stepper_mode();">Set Motor Mode</button></label>
    <p>
        <label align="center"><button class="button" id="serial_forwarding" onclick="serial_forwarding();">Start Serial Forwarding</button></label>
    <p>
        <label align="center"><button class="button" onclick="set_role(0);">Normal</button></label>
        <label align="center"><button class="button" onclick="set_role(1);">Leader</button></label>
        <label align="center"><button class="button" onclick="set_role(2);">Follower</button></label>
    <p>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.rainbow_on);">RainbowON</button></label>
        <label align="center"><button class="button" onclick="send_command(cmd_type.servo_control, servo_cmd.rainbow_off);">RainbowOFF</button></label>
    <script>
        const cmd_type = Object.freeze({
            select_servo:  0,
            servo_control: 1,
            scan_servos:   9,
        });

        const servo_cmd = Object.freeze({
            middle:          1,
            stop:            2,
            release:         3,
            torque:          4,
            position_inc:    5,
            position_dec:    6,
            speed_inc:       7,
            speed_dec:       8,
            id_to_set_inc:   9,
            id_to_set_dec:  10,
            set_middle:     11,
            set_servo_mode: 12,
            set_motor_mode: 13,
            serial_fwd_on:  14,
            serial_fwd_off: 15,
            set_new_id:     16,
            role_normal:    17,
            role_leader:    18,
            role_follower:  19,
            rainbow_on:     20,
            rainbow_off:    21,
        });

        const select_dir = Object.freeze({
            next:  1,
            prev: -1,
        });

        var serial_forward_status = false;

        function send_command(type, instruction) {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "cmd?inputT=" + type + "&inputI=" + instruction + "&inputA=0&inputB=0", true);
            xhr.send();
        }

        setInterval(function() {
          get_data();
        }, 300);

        setInterval(function() {
          get_servo_id();
        }, 1500);

        function get_data() {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                  document.getElementById("STSValue").innerHTML =
                  this.responseText;
                }
            };
            xhttp.open("GET", "readSTS", true);
            xhttp.send();
        }

        function get_servo_id() {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                  document.getElementById("IDValue").innerHTML =
                  this.responseText;
                }
            };
            xhttp.open("GET", "readID", true);
            xhttp.send();
        }

        function set_role(mode_num) {
            if (mode_num == 0) {
                if (confirm("Set the role as Normal. Dev won't send or receive data via ESP-NOW.")) {
                    send_command(cmd_type.servo_control, servo_cmd.role_normal);
                }
            }
            if (mode_num == 1) {
                if (confirm("Set the role as Leader. Dev will send data via ESP-NOW.")) {
                    send_command(cmd_type.servo_control, servo_cmd.role_leader);
                }
            }
            if (mode_num == 2) {
                if (confirm("Set the role as Follower. Dev will receive data via ESP-NOW.")) {
                    send_command(cmd_type.servo_control, servo_cmd.role_follower);
                }
            }
        }

        function set_middle() {
            if (confirm("The middle position of the active servo will be set.")) {
                send_command(cmd_type.servo_control, servo_cmd.set_middle);
            }
        }

        function set_servo_mode() {
            if (confirm("The active servo will be set as servoMode.")) {
                send_command(cmd_type.servo_control, servo_cmd.set_servo_mode);
            }
        }

        function set_stepper_mode() {
            if (confirm("The active servo will be set as motorMode.")) {
                send_command(cmd_type.servo_control, servo_cmd.set_motor_mode);
            }
        }

        function set_new_id() {
            if (confirm("A new ID of the active servo will be set.")) {
                send_command(cmd_type.servo_control, servo_cmd.set_new_id);
            }
        }

        function serial_forwarding() {
            if (!serial_forward_status) {
                if (confirm("Do you want to start serial forwarding?")) {
                    send_command(cmd_type.servo_control, servo_cmd.serial_fwd_on);
                    serial_forward_status = true;
                    document.getElementById("serial_forwarding").innerHTML = "Stop Serial Forwarding";
                }
            }
            else {
                if (confirm("Do you want to stop serial forwarding?")) {
                    send_command(cmd_type.servo_control, servo_cmd.serial_fwd_off);
                    serial_forward_status = false;
                    document.getElementById("serial_forwarding").innerHTML = "Start Serial Forwarding";
                }
            }
        }

    </script>
</body>
</html>
)rawliteral";