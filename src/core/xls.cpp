#include <teaql/core/xls.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <algorithm>
#include <stdexcept>

namespace teaql {
namespace {
std::int32_t checked(std::int64_t n) {
    if(n<INT32_MIN || n>INT32_MAX) throw std::overflow_error("Xls coordinate arithmetic overflow");
    return std::int32_t(n);
}
}
XlsBlock::XlsBlock(std::allocator_arg_t, Allocator a, std::string_view name, std::int32_t x, std::int32_t y, const Json& input)
    : page(name,a.resource()),top(y),bottom(y),left(x),right(x),value(a.resource(),input),properties(a.resource()) {}
XlsBlock::XlsBlock(std::allocator_arg_t, Allocator a, const XlsBlock& other)
    : page(other.page,a.resource()),top(other.top),bottom(other.bottom),left(other.left),right(other.right),
      style_refer_block(other.style_refer_block ? detail::make_box<XlsBlock>(a.resource(),*other.style_refer_block) : nullptr),
      value(a.resource(),other.value),properties(a.resource()) {
    for(const auto& [key,item]:other.properties) properties.emplace(String(key,a.resource()),Json(a.resource(),item));
}
XlsBlock XlsBlock::from_context(const XlsBlockBuildContext& context, const Json& value) {
    return {std::allocator_arg,Allocator(context.memory_resource()),context.page,context.x,context.y,value};
}
XlsBlock& XlsBlock::region(std::int32_t l,std::int32_t t,std::int32_t r,std::int32_t b) { left=l;top=t;right=r;bottom=b;return *this; }
XlsBlock& XlsBlock::span(std::int32_t width,std::int32_t height) {
    right=checked(std::int64_t(left)+(width==INT32_MIN ? INT32_MIN : width-1));
    bottom=checked(std::int64_t(top)+(height==INT32_MIN ? INT32_MIN : height-1)); return *this;
}
void XlsBlock::set_property(std::string_view name,const Json& input) {
    Json copy(memory_resource(),input); properties.insert_or_assign(String(name,memory_resource()),std::move(copy));
}
XlsBlock& XlsBlock::style(const XlsBlock& input) {
    auto copy=detail::make_box<XlsBlock>(memory_resource(),input); style_refer_block=std::move(copy); return *this;
}
std::int32_t XlsBlock::width() const { return checked(std::int64_t(checked(std::int64_t(right)-left))+1); }
std::int32_t XlsBlock::height() const { return checked(std::int64_t(checked(std::int64_t(bottom)-top))+1); }
bool XlsBlock::operator==(const XlsBlock& other) const {
    return page==other.page && top==other.top && bottom==other.bottom && left==other.left && right==other.right &&
        bool(style_refer_block)==bool(other.style_refer_block) && (!style_refer_block || *style_refer_block==*other.style_refer_block) &&
        value==other.value && properties==other.properties;
}
Json XlsBlock::to_json_value(Memory r) const {
    auto result=detail::model_json(r);
    detail::model_put(result,"page",page);detail::model_put(result,"top",top);detail::model_put(result,"bottom",bottom);
    detail::model_put(result,"left",left);detail::model_put(result,"right",right);
    if(style_refer_block) detail::model_put(result,"styleReferBlock",style_refer_block->to_json_value(r));
    if(!std::holds_alternative<std::monostate>(value.data)) detail::model_put(result,"value",value);
    if(!properties.empty()) detail::model_put(result,"properties",properties);
    return result;
}
ModelResult<XlsBlock> XlsBlock::from_json_value(Memory r,const Json& input) {
    return detail::decode_model<XlsBlock>(r,[&] {
        detail::ModelReader reader(input,8);
        const auto& page=detail::model_string(*reader.field("page",0),"page");
        auto top=detail::model_integer<std::int32_t>(*reader.field("top",1),"top");
        auto bottom=detail::model_integer<std::int32_t>(*reader.field("bottom",2),"bottom");
        auto left=detail::model_integer<std::int32_t>(*reader.field("left",3),"left");
        auto right=detail::model_integer<std::int32_t>(*reader.field("right",4),"right");
        auto style=reader.field("styleReferBlock",5,true);
        XlsBlock result(std::allocator_arg,Allocator(r),page,left,top,*reader.field("value",6));
        result.right=right;result.bottom=bottom;
        if(style && !std::holds_alternative<std::monostate>(style->data)) result.style_refer_block=detail::make_box<XlsBlock>(r,detail::decoded(from_json_value(r,*style)));
        if(auto properties=reader.field("properties",7,false,true))
            for(const auto& [key,item]:detail::model_object(*properties,"properties")) result.properties.emplace(String(key,r),Json(r,item));
        return result;
    });
}
XlsBlockBuildContext::XlsBlockBuildContext(std::allocator_arg_t, Allocator a,std::string_view name,std::int32_t px,std::int32_t py)
    : page(name,a.resource()),start_x(std::max(px,0)),x(start_x),y(std::max(py,0)) {}
XlsBlockBuildContext::XlsBlockBuildContext(std::allocator_arg_t, Allocator a,const XlsBlockBuildContext& other)
    : page(other.page,a.resource()),start_x(other.start_x),x(other.x),y(other.y) {}
XlsBlockBuildContext XlsBlockBuildContext::next() const { XlsBlockBuildContext result(std::allocator_arg,Allocator(memory_resource()),*this);result.x=checked(std::int64_t(x)+1);return result; }
XlsBlockBuildContext XlsBlockBuildContext::new_line() const { auto result=next_line();result.x=0;return result; }
XlsBlockBuildContext XlsBlockBuildContext::next_line() const { XlsBlockBuildContext result(std::allocator_arg,Allocator(memory_resource()),*this);result.x=start_x;result.y=checked(std::int64_t(y)+1);return result; }
Json XlsBlockBuildContext::to_json_value(Memory r) const {
    auto result=detail::model_json(r);detail::model_put(result,"page",page);detail::model_put(result,"startX",start_x);
    detail::model_put(result,"x",x);detail::model_put(result,"y",y);return result;
}
ModelResult<XlsBlockBuildContext> XlsBlockBuildContext::from_json_value(Memory r,const Json& input) {
    return detail::decode_model<XlsBlockBuildContext>(r,[&] {
        detail::ModelReader reader(input,4);
        XlsBlockBuildContext result(std::allocator_arg,Allocator(r),detail::model_string(*reader.field("page",0),"page"));
        // Serde assigns raw coordinates; only the constructor clamps negatives.
        result.start_x=detail::model_integer<std::int32_t>(*reader.field("startX",1),"startX");
        result.x=detail::model_integer<std::int32_t>(*reader.field("x",2),"x");
        result.y=detail::model_integer<std::int32_t>(*reader.field("y",3),"y");return result;
    });
}
XlsPage::XlsPage(std::allocator_arg_t,Allocator a,const XlsPage& other) : XlsPage(std::allocator_arg,a,other.name) { for(const auto& b:other.blocks) blocks.emplace_back(b); }
const XlsBlock* XlsPage::block_at(std::int32_t x,std::int32_t y) const { for(const auto& block:blocks) if(block.contains(x,y)) return &block;return nullptr; }
Json XlsPage::to_json_value(Memory r) const {
    auto result=detail::model_json(r);detail::model_put(result,"name",name);Json::Array items(r);
    for(const auto& block:blocks) items.emplace_back(block.to_json_value(r));
    detail::model_put(result,"blocks",items);return result;
}
ModelResult<XlsPage> XlsPage::from_json_value(Memory r,const Json& input) {
    return detail::decode_model<XlsPage>(r,[&] {
        detail::ModelReader reader(input,2);XlsPage result(std::allocator_arg,Allocator(r),detail::model_string(*reader.field("name",0),"name"));
        for(const auto& item:detail::model_array(*reader.field("blocks",1),"blocks")) result.blocks.emplace_back(detail::decoded(XlsBlock::from_json_value(r,item)));
        return result;
    });
}
XlsWorkbook::XlsWorkbook(std::allocator_arg_t,Allocator a,const XlsWorkbook& other) : XlsWorkbook(std::allocator_arg,a) { for(const auto& page:other.pages) pages.emplace_back(page); }
const XlsPage* XlsWorkbook::page(std::string_view name) const { for(const auto& page:pages) if(page.name==name) return &page;return nullptr; }
Json XlsWorkbook::to_json_value(Memory r) const {
    auto result=detail::model_json(r);Json::Array items(r);for(const auto& page:pages) items.emplace_back(page.to_json_value(r));detail::model_put(result,"pages",items);return result;
}
ModelResult<XlsWorkbook> XlsWorkbook::from_json_value(Memory r,const Json& input) {
    return detail::decode_model<XlsWorkbook>(r,[&] {
        detail::ModelReader reader(input,1);XlsWorkbook result(std::allocator_arg,Allocator(r));
        for(const auto& item:detail::model_array(*reader.field("pages",0),"pages")) result.pages.emplace_back(detail::decoded(XlsPage::from_json_value(r,item)));
        return result;
    });
}
}
