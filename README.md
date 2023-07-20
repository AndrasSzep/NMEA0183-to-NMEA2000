/* 
by Dr.András Szép v1.3 3.7.2023 GNU General Public License (GPL).
*/

/*
This is an AI (chatGPT) assisted development for
Arduino ESP32 code to display UDP-broadcasted NMEA0183 messages 
(like from a NMEA0183 simulator https://github.com/panaaj/nmeasimulator )
through a webserver to be seen on any mobile device for free.
Websockets used to autoupdate the data.
Environmental sensors incorporated and data for the last 24hours stored respectively
in the SPIFFS files /pressure, /temperature, /humidity.
The historical environmental data displayed in the background as charts.

Local WiFi attributes are stored at SPIFFS in files named /ssid and /password.
WPS never been tested but assume working.

Implemented OverTheAir update of the data files as well as the code itself on port 8080
(i.e. http://nmea2000.local:8080 ) see config.h . 
*** Arduino IDE 2.0 does not support file upload, this makes much simplier uploading updates 
especially in the client and stored data files.

ToDo: 
      LED lights on M5Atom. Still need some ideas of colors and blinking signals

      incorporate NMEA2000 can bus connection to receive data along with NMEA0183 on UDP.

*/


DBK Depth Below Keel
$--DBK,x.x,f,x.x,M,x.x,F*hh
1) Depth, feet
2) f = feet
3) Depth, meters
4) M = meters
5) Depth, Fathoms
6) F = Fathoms
7) Checksum

DBS Depth Below Surface
$--DBS,x.x,f,x.x,M,x.x,F*hh
1) Depth, feet
2) f = feet
3) Depth, meters
4) M = meters
5) Depth, Fathoms
6) F = Fathoms
7) Checksum

DBT Depth Below Transducer
$--DBT,x.x,f,x.x,M,x.x,F*hh
1) Depth, feet
2) f = feet
3) Depth, meters
4) M = meters
5) Depth, Fathoms
6) F = Fathoms
7) Checksum

DPT Heading – Deviation & Variation
$--DPT,x.x,x.x*hh
1) Depth, meters
2) Offset from transducer;
positive means distance from transducer to water line,
negative means distance from transducer to keel
3) Checksum

GGA Global Positioning System Fix Data. Time, Position and fix related data
for a GPS receiver
$--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
1) Time (UTC)
2) Latitude
3) N or S (North or South)
4) Longitude
5) E or W (East or West)
6) GPS Quality Indicator,
0 - fix not available,
1 - GPS fix,
2 - Differential GPS fix
7) Number of satellites in view, 00 - 12
8) Horizontal Dilution of precision
9) Antenna Altitude above/below mean-sea-level (geoid)
10) Units of antenna altitude, meters
11) Geoidal separation, the difference between the WGS-84 earth
ellipsoid and mean-sea-level (geoid), "-" means mean-sea-level below ellipsoid
12) Units of geoidal separation, meters
13) Age of differential GPS data, time in seconds since last SC104
type 1 or 9 update, null field when DGPS is not used
14) Differential reference station ID, 0000-1023
15) Checksum

GLL Geographic Position – Latitude/Longitude
$--GLL,llll.ll,a,yyyyy.yy,a,hhmmss.ss,A*hh
1) Latitude
2) N or S (North or South)
3) Longitude
4) E or W (East or West)
5) Time (UTC)
6) Status A - Data Valid, V - Data Invalid
7) Checksum

HDG Heading – Deviation & Variation
$--HDG,x.x,x.x,a,x.x,a*hh
1) Magnetic Sensor heading in degrees
2) Magnetic Deviation, degrees
3) Magnetic Deviation direction, E = Easterly, W = Westerly
4) Magnetic Variation degrees
5) Magnetic Variation direction, E = Easterly, W = Westerly
6) Checksum

HDM Heading – Magnetic
$--HDM,x.x,M*hh
1) Heading Degrees, magnetic
2) M = magnetic
3) Checksum

HDT Heading – True
$--HDT,x.x,T*hh
1) Heading Degrees, true
2) T = True
3) Checksum

MWD Wind Direction & Speed
Format unknown


MTW Water Temperature
$--MTW,x.x,C*hh
1) Degrees
2) Unit of Measurement, Celcius
3) Checksum

MWV Wind Speed and Angle
$--MWV,x.x,a,x.x,a*hh
1) Wind Angle, 0 to 360 degrees
2) Reference, R = Relative, T = True
3) Wind Speed
4) Wind Speed Units, K/M/N
5) Status, A - Data Valid
6) Checksum

RMB Recommended Minimum Navigation Information
$--RMB,A,x.x,a,c--c,c--c,llll.ll,a,yyyyy.yy,a,x.x,x.x,x.x,A*hh
1) Status, V = Navigation receiver warning
2) Cross Track error - nautical miles
3) Direction to Steer, Left or Right
4) TO Waypoint ID
5) FROM Waypoint ID
6) Destination Waypoint Latitude
7) N or S
8) Destination Waypoint Longitude
9) E or W
10) Range to destination in nautical miles
11) Bearing to destination in degrees True
12) Destination closing velocity in knots
13) Arrival Status, A = Arrival Circle Entered
14) Checksum

RMC Recommended Minimum Navigation Information
$--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxx,x.x,a*hh
1) Time (UTC)
2) Status, V = Navigation receiver warning
3) Latitude
4) N or S
5) Longitude
6) E or W
7) Speed over ground, knots
8) Track made good, degrees true
9) Date, ddmmyy
10) Magnetic Variation, degrees
11) E or W
12) Checksum

RPM Revolutions
$--RPM,a,x,x.x,x.x,A*hh
1) Source; S = Shaft, E = Engine
2) Engine or shaft number
3) Speed, Revolutions per minute
4) Propeller pitch, % of maximum, "-" means astern
5) Status, A means data is valid
6) Checksum

VWR Relative Wind Speed and Angle
$--VWR,x.x,a,x.x,N,x.x,M,x.x,K*hh
1) Wind direction magnitude in degrees
2) Wind direction Left/Right of bow
3) Speed
4) N = Knots
5) Speed
6) M = Meters Per Second
7) Speed
8) K = Kilometers Per Hour
9) Checksum

ZDA Time & Date – UTC, Day, Month, Year and Local Time Zone
$--ZDA,hhmmss.ss,xx,xx,xxxx,xx,xx*hh
1) Local zone minutes description, same sign as local hours
2) Local zone description, 00 to +/- 13 hours
3) Year
4) Month, 01 to 12
5) Day, 01 to 31
6) Time (UTC)
7) Checksum

