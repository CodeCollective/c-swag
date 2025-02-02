
inner_diameter = 40;
transverse_shell_X=7;
led_gap = 6;

outer_ring_inner_d = inner_diameter + transverse_shell_X + led_gap;

shell_Z=12;
rotation = 290;
fn=90;
CONDUIT_DIAMETER=3.5;

ARDUINO_SHELL_X = 3;
ARDUINO_SHELL_XY = 3;
FRICTION_OVERLAP = 3.5;
// ignoring the usb connector and reset button, Arduino Nano requires 4mm
ARDUINO_X = 18;
ARDUINO_Y = 44;
ARDUINO_HOUSING_Z = shell_Z;
ARDUINO_R_POSITION = outer_ring_inner_d + ((ARDUINO_X+ARDUINO_SHELL_X) /2)+2;

ARDUINO_HOUSING_STATOR_Z = ARDUINO_HOUSING_Z - ARDUINO_SHELL_XY;

fillet_radius = 2; // Radius of the fillet
plateau_X =2;

module endpiece(){
    inner_curve = led_gap/2;
    rotate_extrude(angle=180, $fn=fn)
    polygon(points=[
        [inner_curve, shell_Z], 
        [inner_curve+plateau_X, shell_Z], 
        [inner_curve + transverse_shell_X, 0],
        [inner_curve, 0]
    ]);
}


module theC(){
    
    rotate_extrude(angle=rotation, $fn=fn){
    polygon( points=[
        [inner_diameter,0],
        [inner_diameter+transverse_shell_X-plateau_X, shell_Z],
        [inner_diameter+transverse_shell_X,shell_Z],
        [inner_diameter+transverse_shell_X,0]
    ]);

    polygon(points=[
        [outer_ring_inner_d, shell_Z], 
        [outer_ring_inner_d+plateau_X, shell_Z], 
        [outer_ring_inner_d + transverse_shell_X, 0], 
        [outer_ring_inner_d, 0]
    ]);
        
    }
    
    rotate([0,0,-5])
    rotate_extrude(angle=rotation+10, $fn=fn){
    polygon(points=[
        [inner_diameter+transverse_shell_X, 0], 
        [inner_diameter+transverse_shell_X, 2], 
        [outer_ring_inner_d , 2], 
        [outer_ring_inner_d , 0]
    ]);
    }
        
    translate([outer_ring_inner_d - led_gap/2,0,0])
    rotate([0,0,180])
    endpiece();

    rotate([0,0,rotation])
    translate([outer_ring_inner_d - led_gap/2,0,0])
    endpiece();
    
}


module fillet_cube_1(dims, fillet_radius=3){
    x = dims[0];
    y = dims[1];
    z = dims[2];
    // Create the filleted cube
    hull() {
        
        // Bottom-left cube
        translate([fillet_radius/2, y/2+fillet_radius, z/2])
        cube([fillet_radius, fillet_radius, z], center=true);
        
        // Bottom-right cylinder
        translate([x - fillet_radius, y/2+fillet_radius, z/2])
        cylinder(r=fillet_radius, h=z, center=true, $fn=fn);
        
        // Bottom-left cube (positioned at the front)
        translate([fillet_radius/2, fillet_radius/2, z/2])
        cube([fillet_radius, fillet_radius, z], center=true);
        
        // Bottom-right cube (positioned at the front)
        translate([x - fillet_radius/2, fillet_radius/2, z/2])
        cube([fillet_radius, fillet_radius, z], center=true);
    }
}


module fillet_cube(dims, fillet_radius=3){
    x = dims[0];
    y = dims[1];
    z = dims[2];
    // Create the filleted cube
    hull() {
        
        // Bottom-left cylinder
        translate([fillet_radius, y/2+fillet_radius, z/2])
        cylinder(r=fillet_radius, h=z, center=true, $fn=fn);
        
        // Bottom-right cylinder
        translate([x - fillet_radius, y/2+fillet_radius, z/2])
        cylinder(r=fillet_radius, h=z, center=true, $fn=fn);
        
        // Bottom-left cube (positioned at the front)
        translate([fillet_radius/2, fillet_radius/2, z/2])
        cube([fillet_radius, fillet_radius, z], center=true);
        
        // Bottom-right cube (positioned at the front)
        translate([x - fillet_radius/2, fillet_radius/2, z/2])
        cube([fillet_radius, fillet_radius, z], center=true);
    }
}


module ardu_housing(z){
    
    cube_X = ARDUINO_X + ARDUINO_SHELL_X;
    cube_Z = ARDUINO_Y + ARDUINO_SHELL_X;
    cube_depth = z;
    // Create the filleted cube
    hull() {
        for (x = [-cube_X/2 + fillet_radius, cube_X/2 - fillet_radius]) {
            for (y = [-cube_Z/2 + fillet_radius, cube_Z/2 - fillet_radius]) {
                translate([x, y, z/2])
                cylinder(r=fillet_radius, h=z, center=true, $fn=fn);
            }
        }
    }
}

ADDITIONAL_FRICTION=0.08;
module cap(){
    difference(){
        ardu_housing(ARDUINO_SHELL_XY);
        // Reset button
        cube([7.5,7.5,20], center=true);
    }
    // COVER JTAG CONNECTIONS
    translate([-ARDUINO_X/2,-ARDUINO_Y/2-ADDITIONAL_FRICTION,ARDUINO_SHELL_XY])
    fillet_cube([ARDUINO_X+ADDITIONAL_FRICTION*2,1.6,FRICTION_OVERLAP], fillet_radius=3);
    
    
    // COVER SOME PINS
    translate([-ARDUINO_X/2-ADDITIONAL_FRICTION,ARDUINO_Y/2+ADDITIONAL_FRICTION,ARDUINO_SHELL_XY])
    mirror([0,1,0])
    fillet_cube_1([1.5,7.5,FRICTION_OVERLAP], fillet_radius=0.75);
    
    translate([ARDUINO_X/2+ADDITIONAL_FRICTION,ARDUINO_Y/2+ADDITIONAL_FRICTION,ARDUINO_SHELL_XY])
    mirror([0,1,0])
    mirror([1,0,0])
    fillet_cube_1([3,7.5,FRICTION_OVERLAP], fillet_radius=1);
}

translate([-12,-12,0])
rotate([0,0,45])
cap();


bore_Z=5;
conduit_z = 1;
module wire_conduit(){
    
    // a radial bore for the wires
    translate([0,0,conduit_z])
    rotate([0,0,180+45])
    rotate_extrude(angle=58, $fn=100)
    translate([outer_ring_inner_d, bore_Z])
    circle(d=CONDUIT_DIAMETER);
    
    translate([0,0,conduit_z])
    // bore to the bore
    rotate([45+90,90])
    translate([-5,2,outer_ring_inner_d-2])
    rotate([40,0,0])
    cylinder(h=10, d=CONDUIT_DIAMETER, $fn=fn);

    
}
difference(){
    rotate([0,0,(270-rotation)/2])
    theC();
    
    // remove the arduino body
    rotate([0,0,45+180])
    translate([ARDUINO_R_POSITION,0,ARDUINO_HOUSING_Z/2])
    cube([ARDUINO_X,ARDUINO_Y,ARDUINO_HOUSING_Z*2], center=true);
    
    // make space for the cover
    rotate([0,0,45+180])
    translate([ARDUINO_R_POSITION,0,9])
    ardu_housing(ARDUINO_HOUSING_STATOR_Z);
    
    wire_conduit();
    
}

micro_usb_X= 8.2;
//base
module arduino_house(){
    rotate([0,0,45+180])
    difference(){
        translate([ARDUINO_R_POSITION,0,0])
        ardu_housing(ARDUINO_HOUSING_STATOR_Z);
        //main arduino body
        translate([ARDUINO_R_POSITION,0,ARDUINO_HOUSING_STATOR_Z/2+ARDUINO_SHELL_XY])
        cube([ARDUINO_X,ARDUINO_Y,ARDUINO_HOUSING_STATOR_Z], center=true);
        
        // side channels
        translate([ARDUINO_R_POSITION-7,0,ARDUINO_HOUSING_STATOR_Z/2+1])
        cube([4,ARDUINO_Y,ARDUINO_HOUSING_STATOR_Z], center=true);
        translate([ARDUINO_R_POSITION + 7,0,ARDUINO_HOUSING_STATOR_Z/2+1])
        cube([4,ARDUINO_Y,ARDUINO_HOUSING_STATOR_Z], center=true);
       
        // USB slot
        translate([ARDUINO_R_POSITION,38,0])
        cube([micro_usb_X,ARDUINO_Y,ARDUINO_HOUSING_STATOR_Z], center=true);
        
        // Screwdriver slot
        translate([ARDUINO_R_POSITION,-23,ARDUINO_HOUSING_Z-2])
        cube([5,4,1+FRICTION_OVERLAP], center=true);
        
        // Reset button
        translate([ARDUINO_R_POSITION,-ARDUINO_Y/2+2.25+14.75,0])
        cube([7.5,4.5,ARDUINO_HOUSING_STATOR_Z], center=true);
    }
}

difference(){
    arduino_house();
    // Radial Bore
    wire_conduit();
}


// clip
attachment_rad=4;
attachment_Z=2;
translate([0,0,shell_Z/2])
rotate([0,0,45])
difference(){
    hull(){
        translate([inner_diameter-5,0,])
        cylinder(attachment_Z,r=attachment_rad, $fn=fn);
        translate([inner_diameter+3,0,0])
        scale([1,9,1.4])
        sphere(d=3,$fn=fn);
    }
    
    translate([inner_diameter-5,0,-1])
    cylinder(attachment_Z+2,d=4.5, $fn=fn);
}

rotate([0,0,45])
translate([inner_diameter+3,0,shell_Z/2])
scale([1.1,5.5,1])
sphere(d=6,$fn=fn);

//spacer
module spacer(){
    spacer_Z=5;
    difference(){
        cylinder(spacer_Z,d=7, $fn=fn);
        translate([0,0,-0.5])
        cylinder(spacer_Z+1,d=5, $fn=fn);

    }
}

spacer();