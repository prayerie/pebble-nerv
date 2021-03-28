var API_KEY = 'KEY HERE';
var Clay = require('pebble-clay');
// Import the Clay package
// Load our Clay configuration file
var clayConfig = require('./config');
var clay = new Clay(clayConfig);
var watchId;
var colourScheme;
var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function weatherFromLoc(loc) {
  // Construct URL
  var customKey = localStorage.getItem("customKey");
  if (customKey) {
    console.log("USING CUSTOM API KEY!");
    API_KEY = customKey;
  }
  var url = "http://api.openweathermap.org/data/2.5/weather?q=" +
      loc + '&appid=' + API_KEY;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      var temperature = Math.round((json.main.temp - 273.15) * 10);
      var conditions = json.weather[0].main;
      console.log(loc)
      console.log(temperature)
      var dictionary = {
        "TEMPERATURE": temperature,
        "CONDITIONS": conditions
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function weatherFromLatLong(pos) {
  // Construct URL
  var customKey = localStorage.getItem("customKey");
  if (customKey) {
    console.log("USING CUSTOM API KEY!");
    API_KEY = customKey;
  }
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude + '&appid=' + API_KEY;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      var temperature = Math.round((json.main.temp - 273.15)); // * 10, because we want a decimal
      var conditions = json.weather[0].main;      
      
      var dictionary = {
        "TEMPERATURE": temperature,
        "CONDITIONS": conditions
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

if (!localStorage.getItem("customLoc")) {
  watchId = navigator.geolocation.watchPosition(weatherFromLatLong, locationError);

}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  var customLoc = localStorage.getItem("customLoc");
  if (customLoc) {
    console.log("Custom location isn't blank: ");
    weatherFromLoc(customLoc);
  } else {
    console.log("Custom location is blank");
    navigator.geolocation.getCurrentPosition(
      weatherFromLatLong,
      locationError,
      {timeout: 15000, maximumAge: 60000}
    );
  }
  
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
    console.log("Key is: " + localStorage.getItem("customKey"));
    console.log("Location is: " + localStorage.getItem("customLoc"));
    console.log("Colours is: " + localStorage.getItem("colourScheme"));
    colourScheme = localStorage.getItem("colourScheme");
    getWeather();
    
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }
  // Get the keys and values from each config item
  var dict = clay.getSettings(e.response);
  for (var key in dict) {
    console.log("CONFIG ITEM " + key + ": " + dict[key]);
  }
  localStorage.setItem("fahrenheit", dict[10003]);
  localStorage.setItem("hideDotw", dict[10004]);
  localStorage.setItem("japanese", dict[10005]);
  localStorage.setItem("hideTemp", dict[10006]);
  localStorage.setItem("customKey", dict[10007]);
  localStorage.setItem("customLoc", dict[10008]);
  localStorage.setItem("colourScheme", dict[10002]);
  localStorage.setItem("secret", dict[10009]);
  localStorage.setItem("bwBgColor", dict[10010]);
});
