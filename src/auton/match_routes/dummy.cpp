#include "main.h"

void dummy() {
    chassis.setPose(0, 0, 0);
    chassis.moveToPoint(0, 6, 200,{},false);
    liftPos(850);
}
