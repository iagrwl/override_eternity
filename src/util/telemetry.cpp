#include "main.h"



void telemetry() {
  std::ofstream file("/usd/log.csv", std::ios::app);
  if (!file.is_open()) return;

  // write header if file empty
  if (file.tellp() == 0)
      file << "time,x,y,theta\n";

  // write one row of data
  file  << pros::millis() << ","
        << chassis.getPose().x << ","
        << chassis.getPose().y << ","
        << chassis.getPose().theta << ",";
  file.close();
}
