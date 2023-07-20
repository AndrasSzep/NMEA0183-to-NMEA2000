/* 
by Dr.András Szép under GNU General Public License (GPL).
*/
#ifndef _BoatData_H_
#define _BoatData_H_

struct  sBoatData{
  String SOG ,COG , HeadingT, HeadingM, Speed,
         GPSTime, UTC, Date , // Secs since midnight,
         Latitude, Longitude, Altitude, 
         WaterTemperature , WaterDepth , Offset,
         RPM, Humidity, Pressure, AirTemperature ,
         WindDirectionT, WindDirectionM, WindSpeedK, WindSpeedM,
         WindAngle ;
};

struct tBoatData {
  unsigned long DaysSince1970;   // Days since 1970-01-01
  
  double TrueHeading, MagHeading, SOW, SOG,COG,Variation, Deviation,
         GPSTime, // Secs since midnight,
         Latitude, Longitude, Altitude, HDOP, GeoidalSeparation, DGPSAge,
         WaterTemperature, WaterDepth, Offset,
         RPM, Humidity, Pressure, AirTemperature ,
         WindDirectionT, WindDirectionM, WindSpeedK, WindSpeedM,
         WindAngle ;
  int GPSQualityIndicator, SatelliteCount, DGPSReferenceStationID;
  bool MOBActivated;
  char Status;

public:
  tBoatData() {
    DaysSince1970=19716; 
    TrueHeading=0;
    MagHeading =0;
    SOW = 0;
    SOG=0;
    COG=0; 
    Variation=7.0;
    Deviation =0;
    GPSTime=0;
    Latitude = 45.0;
    Longitude = 6.0;
    Altitude=0;
    HDOP=100000;
    GeoidalSeparation =0;
    DGPSAge=100000;
    WaterTemperature = 10;
    WaterDepth =11;
    Offset = 2.4;
    RPM = 0;
    Humidity = 50; Pressure = 745; AirTemperature =28; WindDirectionT = 10; WindDirectionM =5; WindSpeedK =1; WindSpeedM =1; WindAngle =0;
    GPSQualityIndicator =0;     SatelliteCount=0; 
    DGPSReferenceStationID=0; 
    MOBActivated=false; 
  };
};

#endif // _BoatData_H_
