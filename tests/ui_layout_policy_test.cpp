#include "DywxUILayoutPolicy.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
int main() {
    using dywx::ReferenceScale;
    using dywx::FontPoints;
    assert(std::abs(ReferenceScale(1280,720)-2.0/3.0)<1e-5);
    assert(std::abs(ReferenceScale(1920,1080)-1.0)<1e-5);
    assert(std::abs(ReferenceScale(2560,1440)-4.0/3.0)<1e-5);
    assert(ReferenceScale(0,0)==1);
    assert(ReferenceScale(-1,720)==1);
    assert(ReferenceScale(std::numeric_limits<float>::quiet_NaN(),720)==1);
    assert(ReferenceScale(std::numeric_limits<float>::infinity(),720)==1);
    assert(std::abs(ReferenceScale(320,180)-2.0/3.0)<1e-8);
    for(float user : {1.f,1.1f}) for(float scale : {2.f/3.f,1.f,4.f/3.f}) {
        for(int requested : {12,14,16,18,20,24,30,68}) {
            int points=FontPoints(requested,scale,user);
            assert(points>=std::lround(requested*user));
            assert(points*96.f/72.f*scale>=18.f*user-1e-4f);
            assert(points>0 && points<=100);
        }
    }
    assert(ReferenceScale(20000,20000)==4.0);
    assert(FontPoints(999,1,1)==128);
    assert(FontPoints(-1,1,1)>0);
    assert(FontPoints(12,std::numeric_limits<double>::quiet_NaN(),1)==FontPoints(12,1,1));
    assert(FontPoints(12,1,std::numeric_limits<double>::quiet_NaN())==FontPoints(12,1,1));
    assert(FontPoints(12,0,1)==FontPoints(12,1,1));
    assert(FontPoints(12,1,0)==FontPoints(12,1,1));
    assert(FontPoints(12,1,2)==FontPoints(12,1,1.1f));
    std::cout << "PASS: 720/1080/1440 layout scale, text floor, user bounds and invalid sizes\n";
}
