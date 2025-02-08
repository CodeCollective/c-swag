$fn=100;
x = 12;
y = 47.5;
z = 5;

module support(){
    hull(){
       translate([0,0,z])
        sphere(8);
       translate([-10,-23,0])
        scale([8,14,1])
        sphere(4);
       translate([0,-10,0])
        sphere(4);
       translate([8,-x*2,0])
        sphere(7);
    }
}


module base(){
    width=7;
    hull(){
        translate([width, y/2,0])
        support();
        translate([-width, y/2,0])
        mirror([1,0,0])
        support();
    }
    hull(){
        translate([width, -y/2,0])
        mirror([0,1,0])
        support();
        translate([-width, -y/2,0])
        mirror([1,0,0])
        mirror([0,1,0])
        support();
    }
}

ard_height = 30;
difference(){
    base();
    translate([0,0,ard_height/2+3])
    cube([x,y,ard_height], center=true);
    translate([-4,47,ard_height/2+6])
    cube([x,y,ard_height], center=true);
    translate([0,0,-10])
    cube([100,130,20], center=true);
}