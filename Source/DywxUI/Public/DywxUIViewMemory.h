#pragma once
// Presentation-only rebuild memory. No engine objects or gameplay commands.
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>
namespace dywx {
class ViewMemory {
public:
    bool SameContext(const std::string& next) const { return initialized_ && context_ == next; }
    bool Begin(const std::string& next) {
        const bool same=SameContext(next);
        if(!same) { offsets_.clear(); focus_.clear(); }
        context_=next;initialized_=true;return same;
    }
    static double ClampOffset(double value,double end=1000000.0) {
        if(!std::isfinite(value) || !std::isfinite(end)) return 0;
        return std::clamp(value,0.0,std::clamp(end,0.0,1000000.0));
    }
    void SaveScroll(const std::string& panel,double value,double end=1000000.0) { offsets_[panel]=ClampOffset(value,end); }
    double Scroll(const std::string& panel,double end=1000000.0) const {
        const auto it=offsets_.find(panel);return it==offsets_.end()?0:ClampOffset(it->second,end);
    }
    void SaveFocus(const std::string& key) { focus_=key; }
    std::string Focus(const std::vector<std::string>& enabledCurrentKeys) const {
        return std::find(enabledCurrentKeys.begin(),enabledCurrentKeys.end(),focus_)!=enabledCurrentKeys.end()?focus_:std::string{};
    }
private:
    bool initialized_=false;
    std::string context_,focus_;
    std::map<std::string,double> offsets_;
};
}
