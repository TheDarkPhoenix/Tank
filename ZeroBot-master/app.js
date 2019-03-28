var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var exec = require('child_process').exec, child;
var port = process.env.PORT || 3000;

var SPI = require('pi-spi');
var spi = SPI.initialize("/dev/spidev0.0");
spi.clockSpeed(150000);


app.get('/', function(req, res){
  res.sendfile('Touch.html');
  console.log('HTML sent to client');
});

child = exec("sudo bash start_stream.sh", function(error, stdout, stderr){});

//Whenever someone connects this gets executed
io.on('connection', function(socket){
  console.log('A user connected');
  
  socket.on('pos', function (msx, msy) {
    //console.log('X:' + msx + ' Y: ' + msy);
    //io.emit('posBack', msx, msy);
	
    msx = Math.min(Math.max(parseInt(msx), -255), 255);
    msy = Math.min(Math.max(parseInt(msy), -255), 255);

    if(msx > 0){
      spi.write([0x03, msx]);
      //A1.pwmWrite(msx);
      //A2.pwmWrite(0);
    } else {
      spi.write([0x02, Math.abs(msx)])
      //A1.pwmWrite(0);
      //A2.pwmWrite(Math.abs(msx));
    }

    if(msy > 0){
      spi.write([0x04, msy])
      //B1.pwmWrite(msy);
      //B2.pwmWrite(0);
    } else {
      spi.write([0x05, Math.abs(msy)])
      //B1.pwmWrite(0);
      //B2.pwmWrite(Math.abs(msy));
    }


  });
  
  socket.on('light', function(toggle) {
    //LED.digitalWrite(toggle);    
  });  
  
  socket.on('cam', function(toggle) {
    var numPics = 0;
    console.log('Taking a picture..');
    //Count jpg files in directory to prevent overwriting
    child = exec("find -type f -name '*.jpg' | wc -l", function(error, stdout, stderr){
      numPics = parseInt(stdout)+1;
      // Turn off streamer, take photo, restart streamer
      var command = 'sudo killall mjpg_streamer ; raspistill -o cam' + numPics + '.jpg -n && sudo bash start_stream.sh';
        //console.log("command: ", command);
        child = exec(command, function(error, stdout, stderr){
        io.emit('cam', 1);
      });
    });
    
  });
  
  socket.on('power', function(toggle) {
    child = exec("sudo poweroff");
  });
  
  //Whenever someone disconnects this piece of code is executed
  socket.on('disconnect', function () {
    console.log('A user disconnected');
  });

  setInterval(function(){ // send temperature every 5 sec
    child = exec("cat /sys/class/thermal/thermal_zone0/temp", function(error, stdout, stderr){
      if(error !== null){
         console.log('exec error: ' + error);
      } else {
         var temp = parseFloat(stdout)/1000;
         io.emit('temp', temp);
         console.log('temp', temp);
      }
    });
    //if(!adc.busy){
    //  adc.readADCSingleEnded(0, '4096', '250', function(err, data){ //channel, gain, samples
    //    if(!err){          
    //      voltage = 2*parseFloat(data)/1000;
    //      console.log("ADC: ", voltage);
    //      io.emit('volt', voltage);
    //    }
    //  });
    //}

  }, 5000);

});

http.listen(port, function(){
  console.log('listening on *:' + port);
});
