
inner_diameter = 40;
transverse_shell_width=7;
led_gap = 6.25;

outer_ring_inner_d = inner_diameter + transverse_shell_width + led_gap;

shell_height=8;
rotation = 290;

ARDUINO_SHELL_WIDTH = 3;
ARDUINO_WIDTH = 18;
ARDUINO_HEIGHT = 44;
ARDUINO_HOUSING_HEIGHT = shell_height;


module endpiece(){
    inner_curve = led_gap/2;
    rotate_extrude(angle=180, $fn=200)
    polygon(points=[
        [inner_curve, shell_height], 
        [inner_curve + transverse_shell_width, 0], 
        [inner_curve, 0]
    ]);
}

module theC(){
    
    rotate_extrude(angle=rotation, $fn=200){
    polygon( points=[[inner_diameter,0],[inner_diameter+transverse_shell_width,0],[inner_diameter+transverse_shell_width,shell_height]] );

    polygon(points=[
        [outer_ring_inner_d, shell_height], 
        [outer_ring_inner_d + transverse_shell_width, 0], 
        [outer_ring_inner_d, 0]
    ]);
    }
    translate([outer_ring_inner_d - led_gap/2,0,0])
    rotate([0,0,180])
    endpiece();

    rotate([0,0,rotation])
    translate([outer_ring_inner_d - led_gap/2,0,0])
    endpiece();

}

difference(){
    rotate([0,0,(270-rotation)/2])
    theC();
    rotate([0,0,45+90])
    translate([outer_ring_inner_d + ((ARDUINO_WIDTH+ARDUINO_SHELL_WIDTH) /2),0,ARDUINO_HOUSING_HEIGHT/2])
    cube([ARDUINO_WIDTH,ARDUINO_HEIGHT,ARDUINO_HOUSING_HEIGHT*2], center=true);
    
}

//base
rotate([0,0,45+90])
difference(){
    translate([outer_ring_inner_d + ((ARDUINO_WIDTH+ARDUINO_SHELL_WIDTH) /2),0,ARDUINO_HOUSING_HEIGHT/2])
    cube([ARDUINO_WIDTH+ARDUINO_SHELL_WIDTH,ARDUINO_HEIGHT+ARDUINO_SHELL_WIDTH,ARDUINO_HOUSING_HEIGHT], center=true);
    translate([outer_ring_inner_d + ((ARDUINO_WIDTH+ARDUINO_SHELL_WIDTH) /2),0,ARDUINO_HOUSING_HEIGHT/2])
    cube([ARDUINO_WIDTH,ARDUINO_HEIGHT,ARDUINO_HOUSING_HEIGHT*2], center=true);
}


