#include "fixture_sql.hpp"
#include <iostream>
static Json evaluate(Memory r,const Json& j){
    auto out=detail::model_json(r);auto mode=text(field(j,"mode"));
    try{
        if(mode=="foreign_key"){
            detail::model_put(out,"name",schema_foreign_key_name(r,text(field(j,"source_table")),text(field(j,"source_column")),text(field(j,"target_table")),text(field(j,"target_column")),integer(field(j,"max"))));
        }else if(mode=="validate"){
            Vector<EntityDescriptor> entities(r);for(const auto& e:items(j,"entities"))entities.emplace_back(entity(r,e));
            Vector<const EntityDescriptor*> refs(r);for(const auto& e:entities)refs.push_back(&e);
            validate_schema_identifier_lengths(r,refs,integer(field(j,"max")));detail::model_put(out,"valid",true);
        }else if(mode=="indexes"){
            auto e=entity(r,field(j,"entity"));std::optional<std::size_t> max;if(auto p=optional(j,"max"))if(std::holds_alternative<std::uint64_t>(p->data))max=integer(*p);
            Json::Array specs(r),sqls(r);for(const auto& spec:schema_index_specs(r,e,max)){auto s=detail::model_json(r);detail::model_put(s,"name",spec.name);detail::model_put(s,"table",spec.table);detail::model_put(s,"unique",spec.unique);Json::Array columns(r);for(const auto& c:spec.columns)columns.emplace_back(c);detail::model_put(s,"columns",columns);specs.emplace_back(std::move(s));}
            FixtureDialect dialect;for(const auto& sql:dialect.schema_indexes_sqls(r,e))sqls.emplace_back(sql);
            detail::model_put(out,"specs",specs);detail::model_put(out,"sql",sqls);
        }else throw std::runtime_error("unknown fixture mode");
    }catch(const SqlCompileError& e){detail::model_put(out,"error",e.to_string(r));detail::model_put(out,"object",e.name);detail::model_put(out,"identifier",e.identifier);detail::model_put(out,"actual",e.actual_bytes);detail::model_put(out,"max",e.max_bytes);}
    catch(const std::invalid_argument&){detail::model_put(out,"panic","identifier limit");}
    return out;
}
int main(){GlobalContext global;std::string line;while(std::getline(std::cin,line)){auto user=global.make_user_context();auto r=user->memory_resource();auto input=Json::parse(r,line);if(!input)throw std::runtime_error("invalid fixture");std::cout<<evaluate(r,*input).dump(r)<<'\n';}}
