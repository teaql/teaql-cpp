#include <teaql/runtime/detail/query_keys.hpp>
#include <teaql/core/detail/query_debug.hpp>
#include "fixture_query_keys.hpp"
#include <iostream>
int main(){
    std::string line;
    while(std::getline(std::cin,line)){
        GlobalContext global;auto ctx=global.make_user_context();auto r=ctx->memory_resource();
        auto j=Json::parse(r,line);if(!j)throw std::runtime_error("input JSON");
        auto q=key_query(r,field(*j,"query"));auto canonical=detail::canonical_aggregation_query(r,q);
        auto out=detail::model_json(r);auto ns=text(field(*j,"namespace"));auto query_ns=text(field(*j,"query_namespace"));
        std::optional<std::string_view> user;
        if(auto* v=optional(*j,"user"))if(auto* t=std::get_if<String>(&v->data))user=*t;
        std::optional<detail::PaginationKeyRoot> root;
        if(auto* v=optional(*j,"root"))root=detail::PaginationKeyRoot{text(field(*v,"entity")),integer(field(*v,"id"))};
        detail::model_put(out,"debug",detail::debug_query(r,q));
        detail::model_put(out,"canonical",detail::debug_query(r,canonical));
        detail::model_put(out,"aggregation",detail::aggregation_cache_key(r,ns,query_ns,q));
        detail::model_put(out,"continuous",detail::continuous_page_query_key(r,q,ns,user));
        detail::model_put(out,"ids",detail::id_set_query_key(r,q,ns,user,root));
        detail::model_put(out,"entity_namespace",detail::aggregation_cache_namespace(r,q.entity));
        std::cout<<out.dump(r)<<'\n';
    }
}
