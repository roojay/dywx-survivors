#include "DywxUIViewMemory.h"
#include <cassert>
#include <limits>
#include <iostream>
int main() {
    dywx::ViewMemory memory;
    assert(!memory.SameContext("rest"));
    assert(!memory.Begin("rest"));
    memory.SaveScroll("bag",175); memory.SaveScroll("equipped",42);
    memory.SaveFocus("bag:9001");
    assert(memory.Begin("rest"));
    assert(memory.Scroll("bag")==175 && memory.Scroll("equipped")==42);
    assert(memory.Scroll("absent")==0);
    assert(memory.Focus({"bag:9001","bag:9002"})=="bag:9001");
    assert(memory.Focus({"bag:9002"}).empty()); // Removed/disabled current item.
    assert(memory.Focus({}).empty());
    assert(memory.Scroll("bag",80)==80); // Content shrank after filtering.
    assert(memory.Scroll("bag",0)==0);
    memory.SaveScroll("bag",80,50);assert(memory.Scroll("bag")==50);
    assert(memory.Scroll("equipped")==42); // Independent scroll panels.
    memory.SaveScroll("negative",-50);assert(memory.Scroll("negative")==0);
    memory.SaveScroll("huge",1e20);assert(memory.Scroll("huge")==1000000);
    memory.SaveScroll("nan",std::numeric_limits<double>::quiet_NaN());assert(memory.Scroll("nan")==0);
    memory.SaveScroll("inf",std::numeric_limits<double>::infinity());assert(memory.Scroll("inf")==0);
    assert(dywx::ViewMemory::ClampOffset(20,-1)==0);
    assert(dywx::ViewMemory::ClampOffset(20,std::numeric_limits<double>::infinity())==0);
    assert(dywx::ViewMemory::ClampOffset(20,std::numeric_limits<double>::quiet_NaN())==0);
    assert(!memory.Begin("economy"));
    assert(memory.Scroll("bag")==0 && memory.Scroll("equipped")==0);
    assert(memory.Focus({"bag:9001"}).empty());
    memory.SaveScroll("economy",300);memory.SaveFocus("economy:good:12");
    assert(memory.Begin("economy") && memory.Scroll("economy")==300);
    assert(memory.Focus({"economy:good:13"}).empty()); // Never transfer by visual index.
    memory.SaveFocus("");assert(memory.Focus({"economy:good:12"}).empty());
    assert(!memory.Begin("rest"));assert(memory.Scroll("bag")==0); // No cross-context resurrection.
    for(const char* context : {"screen","modal","run-instance","generation","phase","wave","fusion"}) {
        memory.SaveScroll("list",50);memory.SaveFocus("key");
        assert(!memory.Begin(context));
        assert(memory.Scroll("list")==0 && memory.Focus({"key"}).empty());
    }
    std::cout << "PASS: same-context scroll/focus, independent panels, finite bounds, shrinking content and context isolation\n";
}
