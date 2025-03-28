const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Power meter</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
</head>
<body>
  <h3>Grid</h3>
<label for="Grid_power">Power</label> <input type="text" size="9" id="Grid_power" readonly/><br/>
<label for="Grid_power_factor">Power factor</label> <input type="text" size="9" id="Grid_power_factor" readonly/><br/>
<label for="Grid_energy">Total energy</label> &darr;<input type="text" size="9" id="Grid_energy" readonly/> &uarr;<input type="text" size="9" id="Grid_energy_returned" readonly/><br/>
<label for="Grid_today_energy">Today energy</label> &darr;<input type="text" size="9" id="Grid_today_energy" readonly/> &uarr;<input type="text" size="9" id="Grid_today_energy_returned" readonly/><br/>
<label for="Grid_yesterday_energy">Yesterday energy</label> &darr;<input type="text" size="9" id="Grid_yesterday_energy" readonly/> &uarr;<input type="text" size="9" id="Grid_yesterday_energy_returned" readonly/><br/>
  <h3>Solar</h3>
<label for="Solar_power">Power</label> <input type="text" size="9" id="Solar_power" readonly/><br/>
<label for="Solar_power_factor">Power factor</label> <input type="text" size="9" id="Solar_power_factor" readonly/><br/>
<label for="Solar_energy">Total energy</label> &darr;<input type="text" size="9" id="Solar_energy" readonly/> &uarr;<input type="text" size="9" id="Solar_energy_returned" readonly/><br/>
<label for=Solar_today_energy">Today energy</label> &darr;<input type="text" size="9" id="Solar_today_energy" readonly/> &uarr;<input type="text" size="9" id="Solar_today_energy_returned" readonly/><br/>
<label for=Solar_yesterday_energy">Yesterday energy</label> &darr;<input type="text" size="9" id="Solar_yesterday_energy" readonly/> &uarr;<input type="text" size="9" id="Solar_yesterday_energy_returned" readonly/><br/>
  <h3>Global</h3>
<label for="Total_power">Total power</label> <input type="text" size="9" id="Total_power" readonly/><br/>
<label for="Total_energy">Total energy</label> &darr;<input type="text" size="9" id="Total_energy" readonly/> &uarr;<input type="text" size="9" id="Total_energy_returned" readonly/><br/>
<label for="Total_today_energy">Today energy</label> &darr;<input type="text" size="9" id="Total_today_energy" readonly/> &uarr;<input type="text" size="9" id="Total_today_energy_returned" readonly/><br/>
<label for="Total_yesterday_energy">Yesterday energy</label> &darr;<input type="text" size="9" id="Total_yesterday_energy" readonly/> &uarr;<input type="text" size="9" id="Total_yesterday_energy_returned" readonly/><br/>
  <h3>Raw info</h3>
<label for="Uptime">Uptime (days)</label> <input type="text" size="9" id="Uptime" readonly/><br/>
<label for="Energy_forwarded">Forwarded on the network</label> <input type="checkbox" id="Energy_forwarded" disabled/><br/>
<label for="State">OK state %</label> <input type="text" size="9" id="State" readonly/><br/>
<label for="reset">Reset hardware power meter</label>&nbsp;<button type="button" onclick="onHardwareReset()" id="reset" name="reset">Reset</button><br/>
<label for="reset">Reset today power meter</label>&nbsp;<button type="button" onclick="onSoftwareReset()" id="reset" name="reset">Reset</button><br/>
<script>
function onHardwareReset() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?reset=hard", true);
  xhr.send();
}
function onSoftwareReset() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?reset=soft", true);
  xhr.send();
}
function getValues() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = () => {
    if (xhr.readyState === 4) {
      var res = xhr.response.split(',');
      var index = 0;

      document.getElementById("Grid_power").value = res[index++];
      document.getElementById("Grid_power_factor").value = res[index++];
      document.getElementById("Grid_energy").value = res[index++];
      document.getElementById("Grid_energy_returned").value = res[index++];

      document.getElementById("Solar_power").value = res[index++];
      document.getElementById("Solar_power_factor").value = res[index++];
      document.getElementById("Solar_energy").value = res[index++];
      document.getElementById("Solar_energy_returned").value = res[index++];

      document.getElementById("Total_power").value = res[index++];
      document.getElementById("Total_energy").value = res[index++];
      document.getElementById("Total_energy_returned").value = res[index++];

      document.getElementById("Energy_forwarded").checked = (res[index++] == "1");
      document.getElementById("State").value = res[index++];
      document.getElementById("Uptime").value = res[index++];

      document.getElementById("Grid_today_energy").value = res[index++];
      document.getElementById("Grid_today_energy_returned").value = res[index++];
      document.getElementById("Solar_today_energy").value = res[index++];
      document.getElementById("Solar_today_energy_returned").value = res[index++];
      document.getElementById("Total_today_energy").value = res[index++];
      document.getElementById("Total_today_energy_returned").value = res[index++];

      document.getElementById("Grid_yesterday_energy").value = res[index++];
      document.getElementById("Grid_yesterday_energy_returned").value = res[index++];
      document.getElementById("Solar_yesterday_energy").value = res[index++];
      document.getElementById("Solar_yesterday_energy_returned").value = res[index++];
      document.getElementById("Total_yesterday_energy").value = res[index++];
      document.getElementById("Total_yesterday_energy_returned").value = res[index++];
    }
  };
  xhr.open("GET", "/values", true);
  xhr.send();
}

function init() {
  setInterval(getValues, 2000);
}

window.onload = init;
</script>
</body>
</html>
)rawliteral";