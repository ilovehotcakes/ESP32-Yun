const char index_html[] = R"rawliteral(<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>ESP32 Yun</title>
</head>
<body>
    <div class="page">
        <div id="home_page" class="glass container hide-hlp">
            <div class="title">
                <h3 class="name-txt white-color">Living Room Window</h3>
                <h3 class="serial-txt">yun-e5c258</h3>
                <div class="dropdown">
                    <input type="checkbox" id="dropdown_checkbox" class="" onclick="dropdown()" unchecked>
                    <label for="dropdown_checkbox" class="dropdown-cbx"></label>
                </div>
            </div>

            <div id="controls" class="controls">
                <div class="slider">
                    <input type="range" id="percent_slider" class="glass" onchange="motorMove(this)" value="%SLIDER%" min="0" max="100" step="1">
                    <span class="tick one-tck"></span>
                    <span class="tick two-tck"></span>
                    <span class="tick three-tck"></span>
                    <span class="tick four-tck"></span>
                    <span class="tick five-tck"></span>
                    <span class="marker zero-mkr">0</span>
                    <span class="marker hundred-mkr">100</span>
                </div>

                <div class="open-stop-close">
                    <button type="button" class="open-btn side-hlp button1" value="0" onclick="motorMove(this)">Open</button>
                    <button type="button" class="stop-btn button1" onclick="motorStop()">STOP</button>
                    <button type="button" class="close-btn side-hlp button1" value="100" onclick="motorMove(this)">Close</button>
                </div>
            </div>
        </div>
    </div>
</body>
</html>)rawliteral";