#!/usr/bin/env node
'use strict';

const net = require('net');
const fs = require('fs');
const moment = require('moment');
//Define the listening port
const PORT = 60000;
const ADDRESS = '192.168.0.116';

let server = net.createServer(onClientConnected);
server.listen(PORT, ADDRESS);

function onClientConnected(socket) {
  console.log(`New client: ${socket.remoteAddress}:${socket.remotePort}`);
  socket.destroy();
}
function onClientConnected(socket) {

  // Giving a name to this client
  let clientName = `${socket.remoteAddress}:${socket.remotePort}`;

  // Logging the message on the server
  console.log(`${clientName} connected.`);

  // Triggered on data received by this client
  socket.on('data', (data) => {

    // Logging the message on the server
	 saveString(data.toString());

    // notifing the client
    socket.write('ACK\n');
  });

  // Triggered when this client disconnects
  socket.on('end', () => {
    console.log(`${clientName} disconnected.`);
  });
}

function saveString(data){
	let day = moment().format('DD-MM-YYYY');
	data = moment().format('HH:mm:ss') + " "+ data+"\n";
	fs.appendFile("/volume1/homes/guillemlla/Node/LoRaTCP/logs/"+day,data, function(err){
		if(err) console.log(err);
	console.log(data);
	});
}

console.log(`Server started at: ${ADDRESS}:${PORT}`);
