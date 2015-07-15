var initialised = false;
var CityID = 0, posLat = "0", posLon = "0";
var weatherIcon = {
    "01d" : 'I',
    "02d" : '"',
    "03d" : '!',
    "04d" : '-',
    "09d" : '$',
    "10d" : '+',
    "11d" : 'F',
    "13d" : '9',
    "50d" : '=',
    "01n" : 'N',
    "02n" : '#',
    "03n" : '!',
    "04n" : '-',
    "09n" : '$',
    "10n" : ',',
    "11n" : 'F',
    "13n" : '9',
    "50n" : '>'
};

//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener("ready", function() {
    initialised = true;
	console.log("JavaScript app ready and running!");
	sendMessageToPebble({"JS_READY": 1});		
});
//-----------------------------------------------------------------------------------------------------------------------
function sendMessageToPebble(payload) {
	Pebble.sendAppMessage(payload, 
		function(e) {
			console.log('Successfully delivered message (' + e.payload + ') with transactionId='+ e.data.transactionId);
		},
		function(e) {
			console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ' Error is: ' + e.data.error.message);
		});
}
//-----------------------------------------------------------------------------------------------------------------------
//-- Get current location: http://forums.getpebble.com/discussion/21755/pebble-js-location-to-url
var locationOptions = {
	enableHighAccuracy: true, 
	maximumAge: 10000, 
	timeout: 10000
};
//-----------------------------------------------------------------------------------------------------------------------
function locationSuccess(pos) {
	console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
	posLat = (pos.coords.latitude).toFixed(3);
	posLon = (pos.coords.longitude).toFixed(3);
	
	updateWeather();
}
//-----------------------------------------------------------------------------------------------------------------------
function locationError(err) {
	posLat = "0";
	posLon = "0";
	console.log('location error (' + err.code + '): ' + err.message);
}
//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener('appmessage', function(e) {
	console.log("Got message: " + JSON.stringify(e));
	if ('cityid' in e.payload) {	//Weather Download
		CityID = e.payload.cityid;
		if (CityID === 0)
			navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
		else
			updateWeather();
	}
});
//-----------------------------------------------------------------------------------------------------------------------
function updateWeather() {
	console.log("Updating weather");
	var req = new XMLHttpRequest();
	var URL = "http://api.openweathermap.org/data/2.5/weather?";
	
	if (CityID !== 0)
		URL += "id="+CityID.toString();
	else if (posLat != "0" && posLon != "0")
		URL += "lat=" + posLat + "&lon=" + posLon;
	else
		return; //Error
	
	URL += "&units=metric&lang=en&type=accurate";
	console.log("UpdateURL: " + URL);
	req.open("GET", URL, true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if (req.status == 200) {
				var response = JSON.parse(req.responseText);
				var temp = Math.round(response.main.temp);//-273.15
				var icon = response.weather[0].icon;
				var name = response.name;
				console.log("Got Weather Data for City: " + name + ", Temp: " + temp + ", Icon:" + icon + "/" + weatherIcon[icon]);
				sendMessageToPebble({
					"w_temp": temp,
					"w_icon": weatherIcon[icon]
				});
			}
		}
	};
	req.send(null);
}
//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener("showConfiguration", function() {
    var options = JSON.parse(localStorage.getItem('cas_wv_28de_opt'));
    console.log("read options: " + JSON.stringify(options));
    console.log("showing configuration");
	var uri = 'http://panicman.github.io/config_casiowv58de.html?title=Casio%20WV-58DE%20v2.2';
    if (options !== null) {
        uri +=
			'&inv=' + encodeURIComponent(options.inv) + 
			'&vibr=' + encodeURIComponent(options.vibr) + 
			'&vibr_bt=' + encodeURIComponent(options.vibr_bt) + 
			'&datefmt=' + encodeURIComponent(options.datefmt) + 
			'&weather=' + encodeURIComponent(options.weather) + 
			'&units=' + encodeURIComponent(options.units) + 
			'&cityid=' + encodeURIComponent(options.cityid);
    }
	console.log("Uri: "+uri);
    Pebble.openURL(uri);
});
//-----------------------------------------------------------------------------------------------------------------------
Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response !== "") {
        var options = JSON.parse(decodeURIComponent(e.response));
        console.log("storing options: " + JSON.stringify(options));
        localStorage.setItem('cas_wv_28de_opt', JSON.stringify(options));
        sendMessageToPebble(options);
    } else {
        console.log("no options received");
    }
});
//-----------------------------------------------------------------------------------------------------------------------
