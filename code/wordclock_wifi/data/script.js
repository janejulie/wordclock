var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);
connection.onopen = function () {
  connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {
  console.log('Server: ', e.data);
};
connection.onclose = function () {
  console.log('WebSocket connection closed');
};

function colorChange(){
    connection.send("X");
};

function wordclock(){
    connection.send("W");
};

function allLights(){
    connection.send("A");
};

function none(){
    connection.send("N");
};

function toggleLed(){
    connection.send("L");
};
