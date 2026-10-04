#pragma once
#include <teaql/core/value.hpp>
#include <span>

namespace teaql {
enum class CheckRule {Required,InvalidType,Min,Max,MinStringLength,MaxStringLength,ContextRootMissing,ContextRootMismatch};
std::string_view wire_id(CheckRule);
std::string_view debug_name(CheckRule);
enum class JsonFieldNamingProfile {CamelCase,SnakeCase,PascalCase};
Result<JsonFieldNamingProfile,String> json_field_naming_from_model_value(Memory,std::string_view);

struct LocationSegment {
    using allocator_type=Allocator;
    enum class Kind {Member,Index};
    std::variant<String,std::size_t> data;
    LocationSegment(std::allocator_arg_t,Allocator a,std::string_view name):data(std::in_place_index<0>,name,a.resource()),resource_(a.resource()) {}
    LocationSegment(std::allocator_arg_t,Allocator a,std::size_t index):data(std::in_place_index<1>,index),resource_(a.resource()) {}
    LocationSegment(std::allocator_arg_t,Allocator a,const LocationSegment& other):data(std::in_place_index<1>,0),resource_(a.resource()) {
        if(other.kind()==Kind::Member)data.emplace<0>(std::get<0>(other.data),a.resource());else data.emplace<1>(std::get<1>(other.data));
    }
    LocationSegment(std::allocator_arg_t,Allocator a,LocationSegment&& other):LocationSegment(detail::move_to(a.resource(),std::move(other))) {}
    LocationSegment(const LocationSegment&)=delete;
    LocationSegment(LocationSegment&&) noexcept=default;
    LocationSegment& operator=(LocationSegment&& other) {return detail::assign_scoped(*this,std::move(other));}
    Kind kind() const noexcept {return Kind(data.index());}
    Memory memory_resource() const noexcept {return resource_;}
    bool operator==(const LocationSegment& other) const {return data==other.data;}
private:
    Memory resource_;
};
struct WireLocationSegment {
    using allocator_type=Allocator;
    enum class Kind {Property,Index};
    std::variant<String,std::size_t> data;
    WireLocationSegment(std::allocator_arg_t,Allocator a,std::string_view name):data(std::in_place_index<0>,name,a.resource()),resource_(a.resource()) {}
    WireLocationSegment(std::allocator_arg_t,Allocator a,std::size_t index):data(std::in_place_index<1>,index),resource_(a.resource()) {}
    WireLocationSegment(std::allocator_arg_t,Allocator a,const WireLocationSegment& other):data(std::in_place_index<1>,0),resource_(a.resource()) {
        if(other.kind()==Kind::Property)data.emplace<0>(std::get<0>(other.data),a.resource());else data.emplace<1>(std::get<1>(other.data));
    }
    WireLocationSegment(std::allocator_arg_t,Allocator a,WireLocationSegment&& other):WireLocationSegment(detail::move_to(a.resource(),std::move(other))) {}
    WireLocationSegment(const WireLocationSegment&)=delete;
    WireLocationSegment(WireLocationSegment&&) noexcept=default;
    WireLocationSegment& operator=(WireLocationSegment&& other) {return detail::assign_scoped(*this,std::move(other));}
    Kind kind() const noexcept {return Kind(data.index());}
    Memory memory_resource() const noexcept {return resource_;}
    bool operator==(const WireLocationSegment& other) const {return data==other.data;}
    Json to_json(Memory) const;
private:
    Memory resource_;
};
class ObjectLocation {
public:
    using allocator_type=Allocator;
    ObjectLocation(std::allocator_arg_t,Allocator a):segments_(a.resource()) {}
    ObjectLocation(std::allocator_arg_t,Allocator a,const ObjectLocation& other):segments_(other.segments_,a.resource()) {}
    ObjectLocation(std::allocator_arg_t,Allocator a,ObjectLocation&& other):ObjectLocation(detail::move_to(a.resource(),std::move(other))) {}
    ObjectLocation(const ObjectLocation&)=delete;
    ObjectLocation(ObjectLocation&&) noexcept=default;
    ObjectLocation& operator=(ObjectLocation&& other){return detail::assign_scoped(*this,std::move(other));}
    static ObjectLocation root(Memory r){return ObjectLocation(std::allocator_arg,Allocator(r));}
    static ObjectLocation hash_root(Memory r,std::string_view name){auto out=root(r);out.member(name);return out;}
    static ObjectLocation array_root(Memory r,std::size_t index){auto out=root(r);out.element(index);return out;}
    ObjectLocation& member(std::string_view name) & {segments_.emplace_back(name);return *this;}
    ObjectLocation&& member(std::string_view name) && {member(name);return std::move(*this);}
    ObjectLocation& element(std::size_t index) & {segments_.emplace_back(index);return *this;}
    ObjectLocation&& element(std::size_t index) && {element(index);return std::move(*this);}
    bool is_root() const noexcept {return segments_.empty();}
    std::size_t level() const noexcept {return segments_.size();}
    std::span<const LocationSegment> segments() const noexcept {return segments_;}
    String model_path(Memory) const;
    String native_path(Memory r) const {return model_path(r);}
    String instance_path(Memory r) const {return instance_path_with(r,JsonFieldNamingProfile::CamelCase);}
    String instance_path_with(Memory,JsonFieldNamingProfile) const;
    String to_string(Memory) const;
    Memory memory_resource() const noexcept {return segments_.get_allocator().resource();}
    bool operator==(const ObjectLocation& other) const {return segments_==other.segments_;}
private:
    Vector<LocationSegment> segments_;
};
struct WireCheckResult {
    using allocator_type=Allocator;
    String rule_id;
    std::optional<String> entity_type;
    Vector<WireLocationSegment> location;
    String instance_path;
    std::optional<String> source_instance_path;
    std::optional<Json> input_value,system_value;
    std::optional<String> message;
    WireCheckResult(std::allocator_arg_t,Allocator a):rule_id(a.resource()),location(a.resource()),instance_path(a.resource()) {}
    WireCheckResult(std::allocator_arg_t,Allocator,const WireCheckResult&);
    WireCheckResult(std::allocator_arg_t,Allocator a,WireCheckResult&& other):WireCheckResult(detail::move_to(a.resource(),std::move(other))) {}
    WireCheckResult(const WireCheckResult&)=delete;
    WireCheckResult(WireCheckResult&&) noexcept=default;
    WireCheckResult& operator=(WireCheckResult&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept {return rule_id.get_allocator().resource();}
    Json to_json(Memory) const;
    bool operator==(const WireCheckResult&) const=default;
};
struct CheckResult {
    using allocator_type=Allocator;
    CheckRule rule;
    ObjectLocation location;
    std::optional<Value> input_value,system_value;
    std::optional<String> message,entity_type,source_instance_path;
    CheckResult(std::allocator_arg_t,Allocator a,CheckRule rule,const ObjectLocation& location):rule(rule),location(std::allocator_arg,a,location) {}
    CheckResult(std::allocator_arg_t,Allocator a,CheckRule rule,ObjectLocation&& location):rule(rule),location(std::allocator_arg,a,std::move(location)) {}
    CheckResult(std::allocator_arg_t,Allocator,const CheckResult&);
    CheckResult(std::allocator_arg_t,Allocator a,CheckResult&& other):CheckResult(detail::move_to(a.resource(),std::move(other))) {}
    CheckResult(const CheckResult&)=delete;
    CheckResult(CheckResult&&) noexcept=default;
    CheckResult& operator=(CheckResult&& other){return detail::assign_scoped(*this,std::move(other));}
    static CheckResult required(Memory r,ObjectLocation location){return CheckResult(std::allocator_arg,Allocator(r),CheckRule::Required,std::move(location));}
    template<class B,class C> static CheckResult min(Memory r,ObjectLocation location,B&& bound,C&& current) {
        CheckResult out(std::allocator_arg,Allocator(r),CheckRule::Min,std::move(location));out.with_system_value(std::forward<B>(bound));out.with_input_value(std::forward<C>(current));return out;
    }
    template<class B,class C> static CheckResult max(Memory r,ObjectLocation location,B&& bound,C&& current) {
        CheckResult out(std::allocator_arg,Allocator(r),CheckRule::Max,std::move(location));out.with_system_value(std::forward<B>(bound));out.with_input_value(std::forward<C>(current));return out;
    }
    template<class C> static CheckResult min_str(Memory r,ObjectLocation location,std::uint64_t bound,C&& current) {
        CheckResult out(std::allocator_arg,Allocator(r),CheckRule::MinStringLength,std::move(location));out.with_system_value(bound);out.with_input_value(std::forward<C>(current));return out;
    }
    template<class C> static CheckResult max_str(Memory r,ObjectLocation location,std::uint64_t bound,C&& current) {
        CheckResult out(std::allocator_arg,Allocator(r),CheckRule::MaxStringLength,std::move(location));out.with_system_value(bound);out.with_input_value(std::forward<C>(current));return out;
    }
    template<class T> CheckResult& with_input_value(T&& value) & {input_value.emplace(memory_resource(),std::forward<T>(value));return *this;}
    template<class T> CheckResult&& with_input_value(T&& value) && {with_input_value(std::forward<T>(value));return std::move(*this);}
    template<class T> CheckResult& with_system_value(T&& value) & {system_value.emplace(memory_resource(),std::forward<T>(value));return *this;}
    template<class T> CheckResult&& with_system_value(T&& value) && {with_system_value(std::forward<T>(value));return std::move(*this);}
    CheckResult& with_message(std::string_view value) & {message.emplace(value,memory_resource());return *this;}
    CheckResult&& with_message(std::string_view value) && {with_message(value);return std::move(*this);}
    CheckResult& with_entity_type(std::string_view value) & {entity_type.emplace(value,memory_resource());return *this;}
    CheckResult&& with_entity_type(std::string_view value) && {with_entity_type(value);return std::move(*this);}
    CheckResult& with_source_instance_path(std::string_view value) & {source_instance_path.emplace(value,memory_resource());return *this;}
    CheckResult&& with_source_instance_path(std::string_view value) && {with_source_instance_path(value);return std::move(*this);}
    Memory memory_resource() const noexcept {return location.memory_resource();}
    String to_string(Memory) const;
    WireCheckResult to_wire(Memory,JsonFieldNamingProfile) const;
    bool operator==(const CheckResult&) const=default;
};
using CheckResults=Vector<CheckResult>;
}
